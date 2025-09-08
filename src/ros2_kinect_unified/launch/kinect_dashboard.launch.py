from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import ExecuteProcess
import os

def generate_launch_description():
    pkg_share = os.path.join(
        os.path.dirname(__file__), '..', 'config'
    )
    rviz_config = os.path.join(pkg_share, 'kinect_dashboard.rviz')

    return LaunchDescription([
        # Start unified Kinect driver
        Node(
            package='ros2_kinect_unified',
            executable='ros2_kinect_unified_node',
            name='kinect_driver',
            output='screen'
        ),
        # Start RViz2 with dashboard config
        ExecuteProcess(
            cmd=['rviz2', '-d', rviz_config],
            output='screen'
        )
    ])
