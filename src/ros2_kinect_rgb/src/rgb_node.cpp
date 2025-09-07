#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <sensor_msgs/msg/camera_info.hpp>
#include <image_transport/image_transport.hpp>
#include <camera_info_manager/camera_info_manager.hpp>

#include <libfreenect.h>
#include <csignal>
#include <thread>
#include <atomic>

class KinectRGBNode : public rclcpp::Node
{
public:
    KinectRGBNode()
    : Node("kinect_rgb_node"),
      cam_info_manager_(this, "kinect_rgb"),
      running_(true)
    {
        // Load camera calibration if available
        std::string calib_url;
        this->declare_parameter<std::string>("camera_info_url", "");
        this->get_parameter("camera_info_url", calib_url);
        if (!calib_url.empty()) {
            cam_info_manager_.loadCameraInfo(calib_url);
        }

        // ROS 2 style image_transport publisher
        image_pub_ = image_transport::create_publisher(this, "kinect/rgb/image_raw");
        cam_info_pub_ = this->create_publisher<sensor_msgs::msg::CameraInfo>(
            "kinect/rgb/camera_info", 1);

        // Init libfreenect
        if (freenect_init(&f_ctx_, NULL) < 0) {
            RCLCPP_FATAL(this->get_logger(), "freenect_init() failed");
            rclcpp::shutdown();
            return;
        }

        freenect_set_log_level(f_ctx_, FREENECT_LOG_INFO);

        int num_devices = freenect_num_devices(f_ctx_);
        if (num_devices < 1) {
            RCLCPP_FATAL(this->get_logger(), "No Kinect devices found");
            freenect_shutdown(f_ctx_);
            rclcpp::shutdown();
            return;
        }

        if (freenect_open_device(f_ctx_, &f_dev_, 0) < 0) {
            RCLCPP_FATAL(this->get_logger(), "Could not open Kinect device");
            freenect_shutdown(f_ctx_);
            rclcpp::shutdown();
            return;
        }

        // Correct libfreenect v1 API for RGB mode
        freenect_set_video_mode(
            f_dev_,
            freenect_find_video_mode(FREENECT_RESOLUTION_MEDIUM, FREENECT_VIDEO_RGB)
        );

        freenect_set_video_callback(f_dev_, videoCallbackStatic);
        freenect_set_user(f_dev_, this);

        // Start the freenect processing thread
        kinect_thread_ = std::thread(&KinectRGBNode::kinectLoop, this);
    }

    ~KinectRGBNode()
    {
        running_ = false;
        if (kinect_thread_.joinable()) {
            kinect_thread_.join();
        }
        if (f_dev_) {
            freenect_close_device(f_dev_);
        }
        if (f_ctx_) {
            freenect_shutdown(f_ctx_);
        }
    }

private:
    static void videoCallbackStatic(freenect_device *dev, void *video, uint32_t timestamp)
    {
        KinectRGBNode *self = static_cast<KinectRGBNode*>(freenect_get_user(dev));
        self->videoCallback(video, timestamp);
    }

    void videoCallback(void *video, uint32_t /*timestamp*/)
    {
        auto img_msg = sensor_msgs::msg::Image();
        img_msg.header.stamp = this->now();
        img_msg.header.frame_id = "kinect_rgb_optical_frame";
        img_msg.height = 480;
        img_msg.width = 640;
        img_msg.encoding = "rgb8";
        img_msg.is_bigendian = false;
        img_msg.step = img_msg.width * 3;
        img_msg.data.assign(
            static_cast<uint8_t*>(video),
            static_cast<uint8_t*>(video) + img_msg.step * img_msg.height
        );

        image_pub_.publish(img_msg);

        auto cam_info = cam_info_manager_.getCameraInfo();
        cam_info.header = img_msg.header;
        cam_info_pub_->publish(cam_info);
    }

    void kinectLoop()
    {
        freenect_start_video(f_dev_);
        while (running_ && rclcpp::ok()) {
            if (freenect_process_events(f_ctx_) < 0) {
                RCLCPP_ERROR(this->get_logger(), "Error in freenect_process_events()");
                break;
            }
        }
        freenect_stop_video(f_dev_);
    }

    // ROS
    image_transport::Publisher image_pub_;
    rclcpp::Publisher<sensor_msgs::msg::CameraInfo>::SharedPtr cam_info_pub_;
    camera_info_manager::CameraInfoManager cam_info_manager_;

    // Kinect
    freenect_context *f_ctx_{nullptr};
    freenect_device *f_dev_{nullptr};

    // Threading
    std::thread kinect_thread_;
    std::atomic<bool> running_;
};

int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<KinectRGBNode>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
