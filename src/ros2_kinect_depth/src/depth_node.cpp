#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <sensor_msgs/msg/camera_info.hpp>
#include <image_transport/image_transport.hpp>
#include <camera_info_manager/camera_info_manager.hpp>

#include <libfreenect.h>
#include <thread>
#include <atomic>
#include <cstring>

class KinectDepthNode : public rclcpp::Node
{
public:
    KinectDepthNode()
    : Node("kinect_depth_node"),
      cam_info_manager_(this, "kinect_depth"),
      running_(true)
    {
        this->declare_parameter<std::string>("camera_info_url", "");
        this->declare_parameter<std::string>("encoding", "16UC1");
        this->get_parameter("camera_info_url", camera_info_url_);
        this->get_parameter("encoding", encoding_);

        if (!camera_info_url_.empty()) {
            cam_info_manager_.loadCameraInfo(camera_info_url_);
        }

        image_pub_ = image_transport::create_publisher(this, "kinect/depth/image_raw");
        cam_info_pub_ = this->create_publisher<sensor_msgs::msg::CameraInfo>(
            "kinect/depth/camera_info", 1);

        if (freenect_init(&f_ctx_, NULL) < 0) {
            RCLCPP_FATAL(this->get_logger(), "freenect_init() failed");
            rclcpp::shutdown();
            return;
        }

        freenect_set_log_level(f_ctx_, FREENECT_LOG_INFO);

        if (freenect_num_devices(f_ctx_) < 1) {
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

        freenect_set_depth_mode(
            f_dev_,
            freenect_find_depth_mode(FREENECT_RESOLUTION_MEDIUM, FREENECT_DEPTH_11BIT)
        );

        freenect_set_depth_callback(f_dev_, depthCallbackStatic);
        freenect_set_user(f_dev_, this);

        kinect_thread_ = std::thread(&KinectDepthNode::kinectLoop, this);
    }

    ~KinectDepthNode()
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
    static void depthCallbackStatic(freenect_device *dev, void *depth, uint32_t timestamp)
    {
        KinectDepthNode *self = static_cast<KinectDepthNode*>(freenect_get_user(dev));
        self->depthCallback(depth, timestamp);
    }

    void depthCallback(void *depth, uint32_t /*timestamp*/)
    {
        auto img_msg = sensor_msgs::msg::Image();
        img_msg.header.stamp = this->now();
        img_msg.header.frame_id = "kinect_depth_optical_frame";
        img_msg.height = 480;
        img_msg.width = 640;

        if (encoding_ == "16UC1") {
            img_msg.encoding = "16UC1";
            img_msg.is_bigendian = false;
            img_msg.step = img_msg.width * 2;
            img_msg.data.resize(img_msg.step * img_msg.height);
            std::memcpy(img_msg.data.data(), depth, img_msg.data.size());
        } else { // 32FC1
            img_msg.encoding = "32FC1";
            img_msg.is_bigendian = false;
            img_msg.step = img_msg.width * 4;
            img_msg.data.resize(img_msg.step * img_msg.height);
            const uint16_t *depth_mm = static_cast<uint16_t*>(depth);
            float *depth_m = reinterpret_cast<float*>(img_msg.data.data());
            for (size_t i = 0; i < img_msg.width * img_msg.height; ++i) {
                depth_m[i] = static_cast<float>(depth_mm[i]);
            }
        }

        image_pub_.publish(img_msg);

        auto cam_info = cam_info_manager_.getCameraInfo();
        cam_info.header = img_msg.header;
        cam_info_pub_->publish(cam_info);
    }

    void kinectLoop()
    {
        freenect_start_depth(f_dev_);
        while (running_ && rclcpp::ok()) {
            if (freenect_process_events(f_ctx_) < 0) {
                RCLCPP_ERROR(this->get_logger(), "Error in freenect_process_events()");
                break;
            }
        }
        freenect_stop_depth(f_dev_);
    }

    // ROS
    image_transport::Publisher image_pub_;
    rclcpp::Publisher<sensor_msgs::msg::CameraInfo>::SharedPtr cam_info_pub_;
    camera_info_manager::CameraInfoManager cam_info_manager_;

    // Kinect
    freenect_context *f_ctx_{nullptr};
    freenect_device *f_dev_{nullptr};

    // Params
    std::string camera_info_url_;
    std::string encoding_;

    // Threading
    std::thread kinect_thread_;
    std::atomic<bool> running_;
};

int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<KinectDepthNode>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
