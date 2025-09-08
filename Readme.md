
# ROS2 Kinect Unified Workspace

This is a ROS2-based project designed to integrate and operate the Kinect v1 sensor using the libfreenect library. The unified package (`ros2_kinect_unified`) provides a single node for accessing Kinect's audio, RGB, depth, tilt, and LED features within the ROS2 Humble environment all at once, making it suitable for robotics and sensor-driven applications. If you prefer to use individual nodes for each feature, please refer to the branch [ros2-humble](https://github.com/SriharshaShesham/KinectV1-Ros2/tree/ros2-humble)

> **Note:** This setup is tested on Ubuntu 22.04 with ROS2 Humble and Kinect v1 (Model 1517). The workspace includes automatic firmware upload for the Kinect, so manual steps like running `freenect-micview` are not required.

For detailed libfreenect setup instructions, please refer to the [libfreenect README](./libfreenect.Readme.md).

## Workspace Structure

- `ros2_kinect_unified`: Unified ROS2 node for Kinect v1 (audio, RGB, depth, tilt, LED)
- `audio_common` & `audio_common_msgs`: Audio message definitions and utilities
- `ros2_tests`: Test scripts and viewers for all features

## Clean Build Steps

### Start Fresh

```bash
cd ~/ros2_ws
rm -rf build/ install/ log/
```

### Environment Setup

Make sure no ROS 1 is sourced:

```bash
unset AMENT_PREFIX_PATH
unset CMAKE_PREFIX_PATH
```

Source only ROS 2 Humble:

```bash
source /opt/ros/humble/setup.bash
```

### Build the Workspace

```bash
colcon build --symlink-install
```

### Source the Overlay

```bash
source install/setup.bash
```

## Running the Unified Node

Start the unified node:

```bash
ros2 launch ros2_kinect_unified kinect_unified.launch.py
```

Or for dashboard visualization:

```bash
ros2 launch ros2_kinect_unified kinect_dashboard.launch.py
```

## Feature Usage

### LED Control

Send commands to `/kinect/led_cmd`:

```bash
ros2 topic pub --once /kinect/led_cmd std_msgs/String "data: 'red'"
ros2 topic pub --once /kinect/led_cmd std_msgs/String "data: 'blink_green'"
ros2 topic pub --once /kinect/led_cmd std_msgs/String "data: 'green'"
ros2 topic pub --once /kinect/led_cmd std_msgs/String "data: 'off'"
```

Or run all LED tests:

```bash
ros2 run ros2_tests rgb_viewer_unified
```

### Tilt Control

Send commands to `/kinect/tilt_cmd`:

```bash
ros2 topic pub --once /kinect/tilt_cmd std_msgs/String "data: 'center'"
ros2 topic pub --once /kinect/tilt_cmd std_msgs/String "data: 'down'"
ros2 topic pub --once /kinect/tilt_cmd std_msgs/String "data: 'up'"
ros2 topic pub --once /kinect/tilt_cmd std_msgs/String "data: 'set 15'"
ros2 topic pub --once /kinect/tilt_cmd std_msgs/String "data: 'set -15'"
```

Or run all tilt tests:

```bash
ros2 run ros2_tests depth_viewer_unified
```

### Depth Image

View depth image:

```bash
ros2 run ros2_tests depth_viewer_unified
```

### RGB Image

View RGB image:

```bash
ros2 run ros2_tests rgb_viewer_unified
```

### Audio Stream

Capture and save audio:

```bash
ros2 run ros2_tests audio_saver
```

---

**Tip:** All features are accessible from the unified node. Use the test scripts in `ros2_tests` for quick validation and visualization.

For troubleshooting or advanced usage, refer to the individual package documentation and source files in `src/ros2_kinect_unified` and `src/ros2_tests`.
