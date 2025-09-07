cmake_minimum_required(VERSION 3.8)
project(ros2_kinect_led)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(FREENECT REQUIRED libfreenect)

include_directories(
  ${FREENECT_INCLUDE_DIRS}
)

add_executable(ros2_kinect_led_node src/ros2_kinect_led_node.cpp)

ament_target_dependencies(ros2_kinect_led_node
  rclcpp
  std_msgs
)

target_link_libraries(ros2_kinect_led_node
  ${FREENECT_LIBRARIES}
  freenect_sync
)

install(TARGETS
  ros2_kinect_led_node
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
