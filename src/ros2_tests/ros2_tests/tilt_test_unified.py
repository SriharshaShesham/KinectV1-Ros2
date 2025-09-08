#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from std_msgs.msg import Float64

class TiltTestUnified(Node):
    def __init__(self):
        super().__init__('tilt_test_unified')
        self.pub = self.create_publisher(Float64, '/kinect/tilt_angle', 10)
        self.get_logger().info("Tilt test (unified) started. Enter tilt angles in degrees (-30 to +30). Ctrl+C to exit.")
        self.timer = self.create_timer(0.1, self.prompt_once)
        self.prompted = False

    def prompt_once(self):
        if not self.prompted:
            try:
                angle = float(input("Enter tilt angle: "))
                msg = Float64()
                msg.data = angle
                self.pub.publish(msg)
                self.get_logger().info(f"Sent tilt angle: {angle}°")
            except ValueError:
                self.get_logger().error("Invalid input. Please enter a number.")
            self.prompted = True
            self.timer.cancel()

def main(args=None):
    rclpy.init(args=args)
    node = TiltTestUnified()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()

if __name__ == '__main__':
    main()
