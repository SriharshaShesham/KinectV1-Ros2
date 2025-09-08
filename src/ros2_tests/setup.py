from setuptools import setup

package_name = 'ros2_tests'

setup(
    name=package_name,
    version='0.1.0',
    packages=[package_name],
    data_files=[
        ('share/ament_index/resource_index/packages', ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='Sriharsha',
    maintainer_email='you@example.com',
    description='Subscriber to Kinect RGB topic for display/verification',
    license='MIT',
    entry_points={
        'console_scripts': [
        'rgb_viewer_unified = ros2_tests.rgb_viewer_unified:main',
        'depth_viewer_unified = ros2_tests.depth_viewer_unified:main',
        'mic_listener_unified = ros2_tests.mic_listener_unified:main',
        'audio_saver = ros2_tests.audio_saver:main',
        'tilt_test_unified = ros2_tests.tilt_test_unified:main',
        'led_test_unified = ros2_tests.led_test_unified:main'
        ],
    },
)
