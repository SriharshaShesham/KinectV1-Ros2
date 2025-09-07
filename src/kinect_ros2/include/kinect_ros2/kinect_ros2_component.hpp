#pragma once

#include <rclcpp/rclcpp.hpp>
#include <image_transport/image_transport.hpp>
#include <camera_info_manager/camera_info_manager.hpp>
#include <cv_bridge/cv_bridge.h>
#include <sensor_msgs/msg/image.hpp>
#include <sensor_msgs/msg/camera_info.hpp>
#include <libfreenect.h>
#include <opencv2/opencv.hpp>
#include <mutex>
#include <std_msgs/msg/float64.hpp>


// TF2
#include <geometry_msgs/msg/transform_stamped.hpp>
#include <tf2_ros/static_transform_broadcaster.hpp>

namespace kinect_ros2
{

class KinectRosComponent : public rclcpp::Node
{
public:
  explicit KinectRosComponent(const rclcpp::NodeOptions & options);
  ~KinectRosComponent();

private:
  // Timer
  rclcpp::TimerBase::SharedPtr timer_;
  // Subscriber for tilt control
rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr tilt_sub_;

  void timer_callback();

  // Freenect context and device
  freenect_context * fn_ctx_;
  freenect_device * fn_dev_;

  // Image transport publishers
  image_transport::CameraPublisher depth_pub_;
  image_transport::CameraPublisher rgb_pub_;

  // Camera info managers
  std::shared_ptr<camera_info_manager::CameraInfoManager> depth_info_manager_;
  std::shared_ptr<camera_info_manager::CameraInfoManager> rgb_info_manager_;
  sensor_msgs::msg::CameraInfo depth_info_;
  sensor_msgs::msg::CameraInfo rgb_info_;

  // Image buffers
  cv::Mat depth_image_;
  cv::Mat rgb_image_;

  // Flags and pointers
  bool depth_flag_;
  bool rgb_flag_;
  uint16_t * freenect_depth_pointer_;
  uint8_t * freenect_rgb_pointer_;

  std::mutex mutex_;

  // Static TF broadcaster
  std::shared_ptr<tf2_ros::StaticTransformBroadcaster> static_tf_;

  // Helper to publish static TFs
  void publish_static_tf(const std::string & parent, const std::string & child,
                         double x, double y, double z,
                         double qx, double qy, double qz, double qw);


  // Static callbacks for libfreenect
  static void depth_cb_static(freenect_device * dev, void * depth_ptr, uint32_t timestamp);
  static void rgb_cb_static(freenect_device * dev, void * rgb_ptr, uint32_t timestamp);

  // Instance methods called by static callbacks
  void depth_cb(void * depth_ptr);
  void rgb_cb(void * rgb_ptr);
};

}  // namespace kinect_ros2
