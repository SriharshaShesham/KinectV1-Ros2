#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from std_msgs.msg import String
from std_srvs.srv import Trigger

class TiltTest(Node):
    def __init__(self):
        super().__init__('tilt_test')

        # Publishers & Subscribers
        self.cmd_pub = self.create_publisher(String, '/kinect/tilt_cmd', 10)
        self.state_sub = self.create_subscription(
            String, '/kinect/tilt_state', self.state_cb, 10)
        self.feedback_sub = self.create_subscription(
            String, '/kinect/tilt_feedback', self.feedback_cb, 10)

        # Service client
        self.limit_cli = self.create_client(Trigger, '/kinect/tilt_limit_status')

        self.state_msgs = []
        self.feedback_msgs = []

        self.get_logger().info("TiltTest node started — beginning test sequence in 2s...")
        self.create_timer(2.0, self.move_up)  # start after 2s

    # ---------------- Callbacks ----------------
    def state_cb(self, msg):
        self.state_msgs.append(msg.data)
        self.get_logger().info(f"[STATE] {msg.data}")

    def feedback_cb(self, msg):
        self.feedback_msgs.append(msg.data)
        self.get_logger().info(f"[FEEDBACK] {msg.data}")

    # ---------------- Command + Service ----------------
    def send_cmd(self, cmd_str):
        msg = String()
        msg.data = cmd_str
        self.cmd_pub.publish(msg)
        self.get_logger().info(f"Sent command: {cmd_str}")
        self.call_limit_service()

    def call_limit_service(self):
        if not self.limit_cli.wait_for_service(timeout_sec=0.5):
            self.get_logger().warn("Limit status service not available")
            return
        req = Trigger.Request()
        future = self.limit_cli.call_async(req)
        future.add_done_callback(self.limit_response_cb)

    def limit_response_cb(self, future):
        try:
            result = future.result()
            self.get_logger().info(
                f"[LIMIT STATUS] success={result.success}, message='{result.message}'"
            )
        except Exception as e:
            self.get_logger().error(f"Service call failed: {e}")

    # ---------------- Sequence Steps ----------------
    def move_up(self):
        self.send_cmd("up")
        self.create_timer(5.0, self.move_center_from_up)

    def move_center_from_up(self):
        self.send_cmd("center")
        self.create_timer(5.0, self.move_down)

    def move_down(self):
        self.send_cmd("down")
        self.create_timer(5.0, self.move_center_from_down)

    def move_center_from_down(self):
        self.send_cmd("center")
        self.get_logger().info("Sequence complete.")
        self.get_logger().info(f"States: {self.state_msgs}")
        self.get_logger().info(f"Feedbacks: {self.feedback_msgs}")
        rclpy.shutdown()

# ---------------- Main ----------------
def main(args=None):
    rclpy.init(args=args)
    node = TiltTest()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        if rclpy.ok():
            rclpy.shutdown()

if __name__ == '__main__':
    main()
