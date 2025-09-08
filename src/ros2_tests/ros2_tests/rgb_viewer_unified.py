#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from sensor_msgs.msg import Image
from cv_bridge import CvBridge
import cv2

class RGBViewerUnified(Node):
    def __init__(self):
        super().__init__('rgb_viewer_unified')
        self.bridge = CvBridge()
        self.subscription = self.create_subscription(
            Image,
            '/kinect/rgb/image_raw',
            self.listener_callback,
            10
        )
        self.get_logger().info("Unified RGB Viewer started. Waiting for images...")

    def listener_callback(self, msg):
        try:
            self.get_logger().debug(f"RGB msg: {msg.header}")
            cv_image = self.bridge.imgmsg_to_cv2(msg, desired_encoding='rgb8')
            cv_image_bgr = cv2.cvtColor(cv_image, cv2.COLOR_RGB2BGR)
            cv2.imshow("Unified Kinect RGB Stream", cv_image_bgr)
            cv2.waitKey(1)
        except Exception as e:
            self.get_logger().error(f"Failed to convert RGB image: {e}")

def main(args=None):
    rclpy.init(args=args)
    node = RGBViewerUnified()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()
        cv2.destroyAllWindows()

if __name__ == '__main__':
    main()
