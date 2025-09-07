#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from std_msgs.msg import String
import time

class KinectLedTester(Node):
    def __init__(self):
        super().__init__('kinect_led_tester')
        self.pub = self.create_publisher(String, '/kinect/led_cmd', 10)

        # All supported LED modes in test order
        self.led_modes = [
            'off',
            'green',
            'red',
            'yellow',
            'blink_green',
            'blink_red_yellow'
        ]

        self.get_logger().info("Starting LED test sequence...")
        self.run_test()

    def run_test(self):
        msg = String()
        for mode in self.led_modes:
            msg.data = mode
            self.get_logger().info(f"Setting LED to: {mode}")
            self.pub.publish(msg)
            time.sleep(2.0)  # Wait so you can see the change

        self.get_logger().info("LED test sequence complete.")
        rclpy.shutdown()

def main(args=None):
    rclpy.init(args=args)
    KinectLedTester()
    rclpy.spin(KinectLedTester())  # Will exit after shutdown in run_test()

if __name__ == '__main__':
    main()
