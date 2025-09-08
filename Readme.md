
# ROS2 Kinect Unified Workspace

This is a ROS2-based project designed to integrate and operate the Kinect v1 sensor using the libfreenect library. The unified package (`ros2_kinect_unified`) provides a single node for accessing Kinect's audio, RGB, depth, tilt, and LED features within the ROS2 Humble environment all at once, making it suitable for robotics and sensor-driven applications. If you prefer to use individual nodes for each feature, please refer to the branch [ros2-humble](https://github.com/SriharshaShesham/KinectV1-Ros2/tree/ros2-humble)

> **Note:** This setup is tested on Ubuntu 22.04 with ROS2 Humble and Kinect v1 (Model 1517). The workspace includes automatic firmware upload for the Kinect, so manual steps like running `freenect-micview` are not required.

For detailed libfreenect setup instructions, please refer to the [libfreenect README](https://github.com/SriharshaShesham/KinectV1-Ros2/blob/ros2-humble/libfreenect.Readme.md).

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
ros2 run ros2_kinect_unified ros2_kinect_unified_node
```


## Tests

### LED Control

Send commands to `/kinect/led`:

```bash
ros2 topic pub --once /kinect/led std_msgs/Int32 "data: 2"   # red
ros2 topic pub --once /kinect/led std_msgs/Int32 "data: 4"   # blink green
ros2 topic pub --once /kinect/led std_msgs/Int32 "data: 1"   # green
ros2 topic pub --once /kinect/led std_msgs/Int32 "data: 0"   # off

```

Or testing programatically:

```bash
ros2 ros2 run ros2_tests led_test_unified
```

### Tilt Control

Send commands to `/kinect/tilt_cmd`:

```bash
ros2 topic pub --once /kinect/tilt_angle std_msgs/Float64 "data: 15.0"   # tilt up
ros2 topic pub --once /kinect/tilt_angle std_msgs/Float64 "data: -10.0"  # tilt down
ros2 topic pub --once /kinect/tilt_angle std_msgs/Float64 "data: 0.0"    # level

```

Or testing programatically:

```bash
ros2 run ros2_tests tilt_test_unified
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

Play audio stream: 

```bash
aplay kinect_capture.wav
```