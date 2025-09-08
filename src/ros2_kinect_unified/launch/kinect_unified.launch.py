from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    return LaunchDescription([
        Node(
            package='ros2_kinect_unified',
            executable='ros2_kinect_unified_node',
            name='ros2_kinect_unified',
            output='screen'
        )
    ])
