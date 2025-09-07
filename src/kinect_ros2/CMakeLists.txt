cmake_minimum_required(VERSION 3.5)
project(kinect_ros2)

# Dependencies

find_package(camera_info_manager REQUIRED)
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_components REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(image_transport REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(class_loader REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(geometry_msgs REQUIRED)
include_directories(${cv_bridge_INCLUDE_DIRS})

# Freenect headers and libs
find_path(FREENECT_INCLUDE_DIR
    NAMES libfreenect.h
    PATH_SUFFIXES libfreenect
    PATHS /usr/local/include /usr/include
)

find_library(FREENECT_LIBRARY
    NAMES freenect
    PATHS /usr/local/lib /usr/lib
)

include_directories(
  ${FREENECT_INCLUDE_DIR}
  include
)

add_executable(kinect_ros2_node
  src/kinect_ros2_node.cpp
  src/kinect_ros2_component.cpp
  src/ofxKinectExtras.cpp 
)

ament_target_dependencies(kinect_ros2_node
  rclcpp
  rclcpp_components
  sensor_msgs
  std_msgs
  camera_info_manager
  image_transport
  cv_bridge
  class_loader
  tf2_ros
  geometry_msgs
)

# Add this line to ensure cv_bridge headers are found
target_include_directories(kinect_ros2_node PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

target_link_libraries(kinect_ros2_node
  ${FREENECT_LIBRARY}
)

install(TARGETS
  kinect_ros2_node
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY cfg/
  DESTINATION share/${PROJECT_NAME}/cfg
)


ament_package()
