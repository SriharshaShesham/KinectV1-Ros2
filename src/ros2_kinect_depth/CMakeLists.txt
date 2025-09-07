cmake_minimum_required(VERSION 3.8)
project(ros2_kinect_depth)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(image_transport REQUIRED)
find_package(camera_info_manager REQUIRED)

pkg_check_modules(FREENECT REQUIRED libfreenect)

include_directories(
  include
  ${FREENECT_INCLUDE_DIRS}
)

link_directories(${FREENECT_LIBRARY_DIRS})

add_executable(depth_node src/depth_node.cpp)
ament_target_dependencies(depth_node rclcpp sensor_msgs image_transport camera_info_manager)
target_link_libraries(depth_node freenect)
install(TARGETS depth_node DESTINATION lib/${PROJECT_NAME})


if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # comment the line when a copyright and license is added to all source files
  set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # comment the line when this package is in a git repo and when
  # a copyright and license is added to all source files
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
