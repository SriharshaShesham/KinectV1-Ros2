#include <rclcpp/rclcpp.hpp>
#include <audio_common_msgs/msg/audio_data.hpp>
#include <libfreenect/libfreenect.h>
#include <libfreenect_audio.h>
#include <csignal>
#include <vector>
#include <cmath>
#include <climits>

class KinectMicNode : public rclcpp::Node
{
public:
    KinectMicNode()
    : Node("ros2_kinect_mic_node"), running_(true)
    {
        // Declare runtime parameters
        this->declare_parameter<float>("gain", 0.5f);                // default -6 dB
        this->declare_parameter<int>("noise_gate_threshold", 300);   // amplitude threshold

        audio_pub_ = this->create_publisher<audio_common_msgs::msg::AudioData>(
            "kinect/audio", 10);

        // Init Kinect
        if (freenect_init(&f_ctx_, NULL) < 0) {
            RCLCPP_ERROR(this->get_logger(), "freenect_init() failed");
            rclcpp::shutdown();
            return;
        }

        freenect_select_subdevices(f_ctx_, (freenect_device_flags)(FREENECT_DEVICE_AUDIO));

        if (freenect_open_device(f_ctx_, &f_dev_, 0) < 0) {
            RCLCPP_ERROR(this->get_logger(), "Could not open Kinect device");
            freenect_shutdown(f_ctx_);
            rclcpp::shutdown();
            return;
        }

        freenect_set_audio_in_callback(f_dev_, audio_callback_static);
        freenect_set_user(f_dev_, this);

        // Start audio
        if (freenect_start_audio(f_dev_) < 0) {
            RCLCPP_ERROR(this->get_logger(), "Could not start audio");
            freenect_close_device(f_dev_);
            freenect_shutdown(f_ctx_);
            rclcpp::shutdown();
            return;
        }

        // Spin in a separate thread
        audio_thread_ = std::thread([this]() {
            while (running_ && rclcpp::ok()) {
                if (freenect_process_events(f_ctx_) < 0) {
                    RCLCPP_ERROR(this->get_logger(), "Error in freenect_process_events()");
                    break;
                }
            }
        });
    }

    ~KinectMicNode()
    {
        running_ = false;
        if (audio_thread_.joinable()) {
            audio_thread_.join();
        }
        if (f_dev_) {
            freenect_stop_audio(f_dev_);
            freenect_close_device(f_dev_);
        }
        if (f_ctx_) {
            freenect_shutdown(f_ctx_);
        }
    }

private:
    static void audio_callback_static(
    freenect_device *dev,
    int num_samples,
    int32_t *mic1,
    int32_t *mic2,
    int32_t *mic3,
    int32_t *mic4,
    int16_t *cancelled,   // new param
    void *unknown         // new param
) {
    (void)cancelled; // unused
    (void)unknown;   // unused
    KinectMicNode *self = static_cast<KinectMicNode*>(freenect_get_user(dev));
    if (self) {
        self->process_audio(num_samples, mic1, mic2, mic3, mic4);
    }
}

    void process_audio(int num_samples,
                       int32_t *mic1,
                       int32_t *mic2,
                       int32_t *mic3,
                       int32_t *mic4)
    {
        // Get current params
        float gain = this->get_parameter("gain").as_double();
        int noise_gate_threshold = this->get_parameter("noise_gate_threshold").as_int();

        audio_common_msgs::msg::AudioData msg;
        msg.data.reserve(num_samples * 2);

        for (int i = 0; i < num_samples; ++i) {
            // Average the four mic channels
            int32_t mixed = (mic1[i] + mic2[i] + mic3[i] + mic4[i]) / 4;

            // Noise gate: zero out if below threshold
            if (std::abs(mixed) < noise_gate_threshold) {
                mixed = 0;
            }

            // Apply gain and shift
            int32_t scaled = static_cast<int32_t>(mixed * gain) >> 8;

            // Clamp to int16 range
            if (scaled > INT16_MAX) scaled = INT16_MAX;
            if (scaled < INT16_MIN) scaled = INT16_MIN;

            int16_t sample = static_cast<int16_t>(scaled);

            // Little-endian byte order
            msg.data.push_back(static_cast<uint8_t>(sample & 0xFF));
            msg.data.push_back(static_cast<uint8_t>((sample >> 8) & 0xFF));
        }

        audio_pub_->publish(msg);
    }

    freenect_context *f_ctx_{nullptr};
    freenect_device *f_dev_{nullptr};
    rclcpp::Publisher<audio_common_msgs::msg::AudioData>::SharedPtr audio_pub_;
    std::thread audio_thread_;
    bool running_;
};

int main(int argc, char *argv[])
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<KinectMicNode>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
