cmake_minimum_required(VERSION 3.8)
project(ros2_kinect_unified)

# Default to C++14
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(audio_common_msgs REQUIRED)

pkg_check_modules(FREENECT REQUIRED libfreenect)

include_directories(
  include
  ${FREENECT_INCLUDE_DIRS}
)

link_directories(${FREENECT_LIBRARY_DIRS})
add_executable(ros2_kinect_unified_node src/ros2_kinect_unified_node.cpp)
target_link_libraries(ros2_kinect_unified_node freenect)
install(TARGETS ros2_kinect_unified_node DESTINATION lib/${PROJECT_NAME})


ament_target_dependencies(ros2_kinect_unified_node
  rclcpp
  sensor_msgs
  std_msgs
  audio_common_msgs
)



install(TARGETS
  ros2_kinect_unified_node
  DESTINATION lib/${PROJECT_NAME}
)

# ✅ Install launch files
install(DIRECTORY launch
  DESTINATION share/${PROJECT_NAME}
)

# ✅ Install config files (RViz, params, etc.)
install(DIRECTORY config
  DESTINATION share/${PROJECT_NAME}
)

ament_package()
