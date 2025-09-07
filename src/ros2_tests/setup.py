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
    maintainer_email='sriharsha@sheshams.in',
    description='Subscriber to Kinect RGB topic for display/verification',
    license='MIT',
    entry_points={
        'console_scripts': [
        'rgb_viewer = ros2_tests.rgb_viewer:main',
        'depth_viewer = ros2_tests.depth_viewer:main',
        'mic_listener = ros2_tests.mic_listener:main',
        'tilt_test = ros2_tests.tilt_test:main',
        'led_test = ros2_tests.led_test:main'
        ],
    },
)
