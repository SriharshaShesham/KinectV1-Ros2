cmake_minimum_required(VERSION 3.8)
project(ros2_kinect_tilt)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(std_srvs REQUIRED)

pkg_check_modules(FREENECT REQUIRED libfreenect)

include_directories(
  include
  ${FREENECT_INCLUDE_DIRS}
)

link_directories(${FREENECT_LIBRARY_DIRS})
add_executable(ros2_kinect_tilt_node src/ros2_kinect_tilt_node.cpp)
ament_target_dependencies(ros2_kinect_tilt_node rclcpp std_msgs geometry_msgs std_srvs)
target_link_libraries(
  ros2_kinect_tilt_node 
  freenect
  ${FREENECT_LIBRARIES}
  freenect_sync
  )


install(TARGETS
  ros2_kinect_tilt_node
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
