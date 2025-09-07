#include "rclcpp/rclcpp.hpp"
#include "kinect_ros2/kinect_ros2_component.hpp"
#include "class_loader/class_loader.hpp"
#include "ament_index_cpp/get_package_prefix.hpp"
#include "rclcpp_components/node_factory.hpp"

std::vector<std::unique_ptr<class_loader::ClassLoader>> _loaders;
std::vector<rclcpp_components::NodeInstanceWrapper> _node_wrappers;

rclcpp::node_interfaces::NodeBaseInterface::SharedPtr get_depth_image_proc_component()
{
  try {
    auto path_prefix = ament_index_cpp::get_package_prefix("depth_image_proc");
    auto loader = std::make_unique<class_loader::ClassLoader>(
      path_prefix + "/lib/libdepth_image_proc.so");

    auto classes = loader->getAvailableClasses<rclcpp_components::NodeFactory>();
    RCLCPP_INFO(rclcpp::get_logger("kinect_ros2_node"), "Available classes:");
    for (const auto &clazz : classes) {
      RCLCPP_INFO(rclcpp::get_logger("kinect_ros2_node"), "  %s", clazz.c_str());
    }

    rclcpp::NodeOptions options;
    options.use_intra_process_comms(true);
    options.arguments({
      "--ros-args",
      "--remap", "image_rect:=depth/image_raw",
      "--remap", "camera_info:=depth/camera_info"
    });

    for (const auto &clazz : classes) {
      if (clazz == "rclcpp_components::NodeFactoryTemplate<depth_image_proc::PointCloudXyzNode>") {
        auto node_factory = loader->createInstance<rclcpp_components::NodeFactory>(clazz);
        auto wrapper = node_factory->create_node_instance(options);
        _node_wrappers.push_back(wrapper);
        _loaders.push_back(std::move(loader));
        return wrapper.get_node_base_interface();
      }
    }

    throw std::runtime_error("depth_image_proc::PointCloudXyzNode not found");
  } catch (const std::exception &e) {
    RCLCPP_ERROR(rclcpp::get_logger("kinect_ros2_node"), "Component loading failed: %s", e.what());
    return nullptr;
  }
}

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::executors::SingleThreadedExecutor exec;

  rclcpp::NodeOptions options;
  options.use_intra_process_comms(true);

  auto kinect_component = std::make_shared<kinect_ros2::KinectRosComponent>(options);
  exec.add_node(kinect_component);

  auto depth_image_proc_component = get_depth_image_proc_component();
  if (depth_image_proc_component) {
    exec.add_node(depth_image_proc_component);
  } else {
    RCLCPP_WARN(rclcpp::get_logger("kinect_ros2_node"), "Skipping depth_image_proc component due to load failure.");
  }

  exec.spin();

  for (auto &wrapper : _node_wrappers) {
    exec.remove_node(wrapper.get_node_base_interface());
  }
  _node_wrappers.clear();
  _loaders.clear();

  rclcpp::shutdown();
  return 0;
}
