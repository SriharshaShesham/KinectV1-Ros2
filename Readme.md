# Ros2 Workspace

This is a ROS2-based project designed to integrate and operate the Kinect v1 sensor using the libfreenect library. It provides a platform for accessing Kinect's audio, RGB, depth, tilt, and LED features within the ROS2 Humble environment, making it suitable for robotics and sensor-driven applications.

Not sure how long this is going to work. I wanted to use the Kinect v1 on ROS2 Humble, and was successfully able to get it working. 


Here is my setup details:
- **Operating System**: Ubuntu 22.04
- **ROS2 Distribution**: Humble Hawksbill
- **Kinect Model**: Kinect v1 (kinect for windows: Model 1517)

Good news is I have also added the code to automatically upload the firmware when the Kinect is plugged in, so you don't have to manually run `freenect-micview` first.

For detailed libfreenect setup instructions, please refer to the [libfreenect README](./libfreenect.Readme.md).


Once libfreenect is set up, you can build this workspace and start using the Kinect v1 with ROS2.
I have created individual ROS2 packages for each of the Kinect's features, allowing you to use only what you need.


## Steps for Clean Building

### Start Fresh



```bash
# create your workspace
mkdir ~/Ros2-KinectV1

cd ~/Ros2-KinectV1
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


# Running the Nodes

## Led Control:

### Terminal 1:
```bash
ros2 run ros2_kinect_led ros2_kinect_led_node

```
### Terminal 2:
```bash
# For solid red
ros2 topic pub --once /kinect/led_cmd std_msgs/String "data: 'red'"

# For blinking green
ros2 topic pub --once /kinect/led_cmd std_msgs/String "data: 'blink_green'"

# For solid green
ros2 topic pub --once /kinect/led_cmd std_msgs/String "data: 'green'"

# Turn off
ros2 topic pub --once /kinect/led_cmd std_msgs/String "data: 'off'"
``` 

Or use single command to test all these using the tests package by running the following command:

```bash
ros2 run ros2_tests led_test
```

## Tilt Control:
### Terminal 1:
```bash
ros2 run ros2_kinect_tilt ros2_kinect_tilt_node

```

### Terminal 2:
```bash
# Tilt to center
ros2 topic pub --once /kinect/tilt_cmd std_msgs/String "data: 'center'"

# Tilt down
ros2 topic pub --once /kinect/tilt_cmd std_msgs/String "data: 'down'"

# Tilt up
ros2 topic pub --once /kinect/tilt_cmd std_msgs/String "data: 'up'"

# Tilt to 15 degrees (positive angle is up)
ros2 topic pub --once /kinect/tilt_cmd std_msgs/String "data: 'set 15'"

# Tilt to -15 degrees (negative angle is down)
ros2 topic pub --once /kinect/tilt_cmd std_msgs/String "data: 'set -15'"
``` 

Or use single command to test all these using the tests package by running the following command:

```bash
ros2 run ros2_tests tilt_test
```

## Depth Image:

### Terminal 1:
```bash
ros2 run ros2_kinect_depth depth_node
```

### Terminal 2:
```bash
# View depth image
ros2 run ros2_tests depth_viewer
```


## RGB Image:
### Terminal 1:
```bash
ros2 run ros2_kinect_rgb rgb_node
```

### Terminal 2:
```bash
# View RGB image
ros2 run ros2_tests rgb_viewer
```

## Audio Stream:
### Terminal 1:
```bash
ros2 run ros2_kinect_mic_node ros2_kinect_mic_node        
``` 

### Terminal 2:
```bash
# Captures audio for 15 seconds and saves to mic_output.wav, while also trying to playing it back in real-time
ros2 run ros2_tests mic_listener
```


