#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from std_msgs.msg import Int32

LED_MODES = {
    0: "OFF",
    1: "GREEN",
    2: "RED",
    3: "YELLOW",
    4: "BLINK_GREEN",
    5: "BLINK_RED_YELLOW"
}

class LEDTestUnified(Node):
    def __init__(self):
        super().__init__('led_test_unified')
        self.pub = self.create_publisher(Int32, '/kinect/led', 10)
        self.get_logger().info("LED test (unified) started. Enter LED mode number. Ctrl+C to exit.")
        self.get_logger().info(f"Modes: {LED_MODES}")
        self.timer = self.create_timer(0.1, self.prompt_once)
        self.prompted = False

    def prompt_once(self):
        if not self.prompted:
            try:
                mode = int(input(f"Enter LED mode {list(LED_MODES.keys())}: "))
                if mode not in LED_MODES:
                    self.get_logger().error("Invalid mode number.")
                else:
                    msg = Int32()
                    msg.data = mode
                    self.pub.publish(msg)
                    self.get_logger().info(f"Set LED to mode {mode} ({LED_MODES[mode]})")
            except ValueError:
                self.get_logger().error("Invalid input. Please enter an integer.")
            self.prompted = True
            self.timer.cancel()

def main(args=None):
    rclpy.init(args=args)
    node = LEDTestUnified()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()

if __name__ == '__main__':
    main()
