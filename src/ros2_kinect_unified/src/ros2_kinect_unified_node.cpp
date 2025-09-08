#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <sensor_msgs/image_encodings.hpp>
#include <audio_common_msgs/msg/audio_data.hpp>
#include <std_msgs/msg/float64.hpp>
#include <std_msgs/msg/int32.hpp>

#include <libfreenect.h>
#include <libfreenect_audio.h>

#include <thread>
#include <vector>
#include <cstring>

class KinectUnifiedNode : public rclcpp::Node
{
public:
    KinectUnifiedNode()
    : Node("kinect_unified_node")
    {
        // Publishers
        rgb_pub_   = create_publisher<sensor_msgs::msg::Image>("kinect/rgb/image_raw", 10);
        depth_pub_ = create_publisher<sensor_msgs::msg::Image>("kinect/depth/image_raw", 10);
        audio_pub_ = create_publisher<audio_common_msgs::msg::AudioData>("kinect/audio", 10);

        // Subscribers for control
        tilt_sub_ = create_subscription<std_msgs::msg::Float64>(
            "kinect/tilt_angle", 10,
            [this](const std_msgs::msg::Float64::SharedPtr msg) {
                freenect_set_tilt_degs(dev_, msg->data);
            }
        );

        led_sub_ = create_subscription<std_msgs::msg::Int32>(
            "kinect/led", 10,
            [this](const std_msgs::msg::Int32::SharedPtr msg) {
                freenect_set_led(dev_, static_cast<freenect_led_options>(msg->data));
            }
        );

        // Init freenect
        if (freenect_init(&f_ctx_, NULL) < 0) {
            RCLCPP_FATAL(get_logger(), "freenect_init() failed");
            rclcpp::shutdown();
        }
        freenect_set_log_level(f_ctx_, FREENECT_LOG_INFO);
        freenect_select_subdevices(f_ctx_, (freenect_device_flags)(FREENECT_DEVICE_CAMERA | FREENECT_DEVICE_AUDIO));

        if (freenect_open_device(f_ctx_, &dev_, 0) < 0) {
            RCLCPP_FATAL(get_logger(), "Could not open Kinect");
            rclcpp::shutdown();
        }

        freenect_set_user(dev_, this);

        // Set callbacks
        freenect_set_video_callback(dev_, &KinectUnifiedNode::video_cb);
        freenect_set_depth_callback(dev_, &KinectUnifiedNode::depth_cb);
        freenect_set_audio_in_callback(dev_, &KinectUnifiedNode::audio_cb);

        // Set modes
        freenect_frame_mode rgb_mode = freenect_find_video_mode(FREENECT_RESOLUTION_MEDIUM, FREENECT_VIDEO_RGB);
        freenect_frame_mode depth_mode = freenect_find_depth_mode(FREENECT_RESOLUTION_MEDIUM, FREENECT_DEPTH_MM);
        freenect_set_video_mode(dev_, rgb_mode);
        freenect_set_depth_mode(dev_, depth_mode);

        // Start streams
        freenect_start_video(dev_);
        freenect_start_depth(dev_);
        freenect_start_audio(dev_); // FIXED

        // Processing thread
        kinect_thread_ = std::thread([this]() {
            while (rclcpp::ok()) {
                if (freenect_process_events(f_ctx_) < 0) break;
            }
        });
    }

    ~KinectUnifiedNode()
    {
        freenect_stop_video(dev_);
        freenect_stop_depth(dev_);
        freenect_stop_audio(dev_); // FIXED
        freenect_close_device(dev_);
        freenect_shutdown(f_ctx_);
        if (kinect_thread_.joinable()) kinect_thread_.join();
    }

private:
    // RGB callback
    static void video_cb(freenect_device *dev, void *video, uint32_t /*timestamp*/)
    {
        auto *node = static_cast<KinectUnifiedNode*>(freenect_get_user(dev));
        sensor_msgs::msg::Image msg;
        msg.header.stamp = node->now();
        msg.header.frame_id = "kinect_rgb_optical_frame";
        msg.height = 480;
        msg.width = 640;
        msg.encoding = sensor_msgs::image_encodings::RGB8;
        msg.step = msg.width * 3;
        msg.data.resize(msg.step * msg.height);
        std::memcpy(msg.data.data(), video, msg.data.size());
        node->rgb_pub_->publish(msg);
    }

    // Depth callback
    static void depth_cb(freenect_device *dev, void *depth, uint32_t /*timestamp*/)
    {
        auto *node = static_cast<KinectUnifiedNode*>(freenect_get_user(dev));
        sensor_msgs::msg::Image msg;
        msg.header.stamp = node->now();
        msg.header.frame_id = "kinect_depth_optical_frame";
        msg.height = 480;
        msg.width = 640;
        msg.encoding = sensor_msgs::image_encodings::TYPE_16UC1;
        msg.step = msg.width * 2;
        msg.data.resize(msg.step * msg.height);
        std::memcpy(msg.data.data(), depth, msg.data.size());
        node->depth_pub_->publish(msg);
    }

    // Correct signature for your libfreenect version
    static void audio_cb(freenect_device *dev, int num_samples,
                        int32_t *mic1, int32_t *mic2,
                        int32_t *mic3, int32_t *mic4,
                        int16_t *cancel, void *unknown)
    {
        auto *node = static_cast<KinectUnifiedNode*>(freenect_get_user(dev));
        audio_common_msgs::msg::AudioData msg;
        msg.data.resize(num_samples * sizeof(int16_t) * 4);
        int16_t *out = reinterpret_cast<int16_t*>(msg.data.data());

        const float gain = 4.0f; // adjust as needed

        for (int i = 0; i < num_samples; ++i) {
            int32_t s1 = static_cast<int16_t>(mic1[i] >> 16);
            int32_t s2 = static_cast<int16_t>(mic2[i] >> 16);
            int32_t s3 = static_cast<int16_t>(mic3[i] >> 16);
            int32_t s4 = static_cast<int16_t>(mic4[i] >> 16);

            // Apply gain with clipping
            out[i*4 + 0] = static_cast<int16_t>(std::max(std::min(static_cast<int>(s1 * gain), 32767), -32768));
            out[i*4 + 1] = static_cast<int16_t>(std::max(std::min(static_cast<int>(s2 * gain), 32767), -32768));
            out[i*4 + 2] = static_cast<int16_t>(std::max(std::min(static_cast<int>(s3 * gain), 32767), -32768));
            out[i*4 + 3] = static_cast<int16_t>(std::max(std::min(static_cast<int>(s4 * gain), 32767), -32768));
        }

        node->audio_pub_->publish(msg);
    }



    freenect_context *f_ctx_{nullptr};
    freenect_device *dev_{nullptr};

    rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr rgb_pub_, depth_pub_;
    rclcpp::Publisher<audio_common_msgs::msg::AudioData>::SharedPtr audio_pub_;
    rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr tilt_sub_;
    rclcpp::Subscription<std_msgs::msg::Int32>::SharedPtr led_sub_;

    std::thread kinect_thread_;
};

int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<KinectUnifiedNode>());
    rclcpp::shutdown();
    return 0;
}
