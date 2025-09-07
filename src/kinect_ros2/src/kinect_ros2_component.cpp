
#include "kinect_ros2/ofxKinectExtras.h"
#include "kinect_ros2/kinect_ros2_component.hpp"
#include <ament_index_cpp/get_package_share_directory.hpp>
#include <mutex>


using namespace std::chrono_literals;

namespace kinect_ros2
{

  KinectRosComponent::KinectRosComponent(const rclcpp::NodeOptions &options)
      : rclcpp::Node("kinect_ros2", options),
        depth_flag_(false),
        rgb_flag_(false),
        freenect_depth_pointer_(nullptr),
        freenect_rgb_pointer_(nullptr)
  {
    std::string pkg_share = ament_index_cpp::get_package_share_directory("kinect_ros2");

    depth_image_ = cv::Mat::zeros(cv::Size(640, 480), CV_16UC1);
    rgb_image_ = cv::Mat::zeros(cv::Size(640, 480), CV_8UC3);

    depth_info_manager_ = std::make_shared<camera_info_manager::CameraInfoManager>(
        this, "kinect", "file://" + pkg_share + "/cfg/calibration_depth.yaml");

    rgb_info_manager_ = std::make_shared<camera_info_manager::CameraInfoManager>(
        this, "kinect", "file://" + pkg_share + "/cfg/calibration_rgb.yaml");

    depth_info_ = depth_info_manager_->getCameraInfo();
    depth_info_.header.frame_id = "kinect_depth";

    rgb_info_ = rgb_info_manager_->getCameraInfo();
    rgb_info_.header.frame_id = "kinect_rgb";

    depth_pub_ = image_transport::create_camera_publisher(this, "depth/image_raw");
    rgb_pub_ = image_transport::create_camera_publisher(this, "image_raw");

    // Initialize TF broadcaster and publish static transforms
    static_tf_ = std::make_shared<tf2_ros::StaticTransformBroadcaster>(this);

    // Identity transforms for now; replace with measured offsets/rotations when available.
    publish_static_tf("base_link", "kinect_rgb", 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0);
    publish_static_tf("base_link", "kinect_depth", 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0);

    if (freenect_init(&fn_ctx_, nullptr) < 0)
    {
      throw std::runtime_error("Failed to initialize freenect context");
    }

    // // Detect if this is a model 1473 Kinect
    // freenect_device_attributes *attr_list;
    // freenect_list_device_attributes(fn_ctx_, &attr_list);

    // bool is1473 = false;
    // for (auto *attr = attr_list; attr != nullptr; attr = attr->next)
    // {
    //   if (attr->camera_serial && strstr(attr->camera_serial, "1473"))
    //   {
    //     is1473 = true;
    //     break;
    //   }
    // }
    // freenect_free_device_attributes(attr_list);

    // //if (is1473)
    // //{
    //   RCLCPP_INFO(get_logger(), "Kinect model 1473 detected — uploading audio firmware...");
    //   freenect_set_fw_address_nui(fn_ctx_,
    //                               ofxKinectExtras::getFWData1473(),
    //                               ofxKinectExtras::getFWSize1473());

    //   freenect_set_fw_address_k4w(fn_ctx_,
    //                               ofxKinectExtras::getFWDatak4w(),
    //                               ofxKinectExtras::getFWSizek4w());
    // //}
    freenect_set_log_level(fn_ctx_, FREENECT_LOG_FATAL);
    freenect_select_subdevices(fn_ctx_, FREENECT_DEVICE_CAMERA);

    int num_devices = freenect_num_devices(fn_ctx_);
    if (num_devices <= 0)
    {
      freenect_shutdown(fn_ctx_);
      throw std::runtime_error("No Kinect devices found");
    }

    if (freenect_open_device(fn_ctx_, &fn_dev_, 0) < 0)
    {
      freenect_shutdown(fn_ctx_);
      throw std::runtime_error("Failed to open Kinect device");
    }
    // ⬇ Add tilt subscriber here
tilt_sub_ = this->create_subscription<std_msgs::msg::Float64>(
    "tilt_angle", 10,
    [this](const std_msgs::msg::Float64::SharedPtr msg) {
        double angle = msg->data;
        RCLCPP_INFO(this->get_logger(), "Setting tilt to %.2f degrees", angle);
        int ret = freenect_set_tilt_degs(fn_dev_, angle);
        if (ret != 0) {
            RCLCPP_ERROR(this->get_logger(), "Failed to set tilt: %d", ret);
        }
    });

    if (freenect_set_depth_mode(fn_dev_,
                                freenect_find_depth_mode(FREENECT_RESOLUTION_MEDIUM, FREENECT_DEPTH_MM)) < 0)
    {
      freenect_shutdown(fn_ctx_);
      throw std::runtime_error("Failed to set depth mode");
    }

    freenect_set_depth_callback(fn_dev_, depth_cb_static);
    freenect_set_video_callback(fn_dev_, rgb_cb_static);
    freenect_set_user(fn_dev_, this);

    if (freenect_start_depth(fn_dev_) < 0 || freenect_start_video(fn_dev_) < 0)
    {
      freenect_shutdown(fn_ctx_);
      throw std::runtime_error("Failed to start Kinect streams");
    }

    timer_ = create_wall_timer(1ms, std::bind(&KinectRosComponent::timer_callback, this));
  }

  KinectRosComponent::~KinectRosComponent()
  {
    RCLCPP_INFO(get_logger(), "Shutting down Kinect...");
    freenect_stop_depth(fn_dev_);
    freenect_stop_video(fn_dev_);
    freenect_close_device(fn_dev_);
    freenect_shutdown(fn_ctx_);
  }

  void KinectRosComponent::publish_static_tf(const std::string &parent, const std::string &child,
                                             double x, double y, double z,
                                             double qx, double qy, double qz, double qw)
  {
    geometry_msgs::msg::TransformStamped t;
    t.header.stamp = now();
    t.header.frame_id = parent;
    t.child_frame_id = child;
    t.transform.translation.x = x;
    t.transform.translation.y = y;
    t.transform.translation.z = z;
    t.transform.rotation.x = qx;
    t.transform.rotation.y = qy;
    t.transform.rotation.z = qz;
    t.transform.rotation.w = qw;
    static_tf_->sendTransform(t);
  }

  void KinectRosComponent::depth_cb(void *depth_ptr)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (depth_flag_)
      return;

    if (freenect_depth_pointer_ != static_cast<uint16_t *>(depth_ptr))
    {
      depth_image_ = cv::Mat(480, 640, CV_16UC1, depth_ptr);
      freenect_depth_pointer_ = static_cast<uint16_t *>(depth_ptr);
    }

    depth_flag_ = true;
  }

  void KinectRosComponent::rgb_cb(void *rgb_ptr)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (rgb_flag_)
      return;

    if (freenect_rgb_pointer_ != static_cast<uint8_t *>(rgb_ptr))
    {
      rgb_image_ = cv::Mat(480, 640, CV_8UC3, rgb_ptr);
      freenect_rgb_pointer_ = static_cast<uint8_t *>(rgb_ptr);
    }

    rgb_flag_ = true;
  }

  void KinectRosComponent::depth_cb_static(freenect_device *dev, void *depth_ptr, uint32_t)
  {
    auto self = static_cast<KinectRosComponent *>(freenect_get_user(dev));
    self->depth_cb(depth_ptr);
  }

  void KinectRosComponent::rgb_cb_static(freenect_device *dev, void *rgb_ptr, uint32_t)
  {
    auto self = static_cast<KinectRosComponent *>(freenect_get_user(dev));
    self->rgb_cb(rgb_ptr);
  }

  void KinectRosComponent::timer_callback()
  {
    freenect_process_events(fn_ctx_);
    auto stamp = now();

    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (depth_flag_)
      {
        depth_info_.header.stamp = stamp;
        auto msg = cv_bridge::CvImage(depth_info_.header, "16UC1", depth_image_).toImageMsg();
        depth_pub_.publish(*msg, depth_info_);
        depth_flag_ = false;
      }

      if (rgb_flag_)
      {
        rgb_info_.header.stamp = stamp;
        auto msg = cv_bridge::CvImage(rgb_info_.header, "rgb8", rgb_image_).toImageMsg();
        rgb_pub_.publish(*msg, rgb_info_);
        rgb_flag_ = false;
      }
    }
  }

} // namespace kinect_ros2

#include "rclcpp_components/register_node_macro.hpp"
RCLCPP_COMPONENTS_REGISTER_NODE(kinect_ros2::KinectRosComponent)
