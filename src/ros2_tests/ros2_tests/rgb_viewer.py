import rclpy
from rclpy.node import Node
from sensor_msgs.msg import Image
from cv_bridge import CvBridge
import cv2

class RGBViewer(Node):
    def __init__(self):
        super().__init__('rgb_viewer')
        self.bridge = CvBridge()
        self.subscription = self.create_subscription(
            Image,
            '/kinect/rgb/image_raw',
            self.listener_callback,
            10
        )
        self.get_logger().info("RGB Viewer node started. Waiting for images...")

    def listener_callback(self, msg):
        try:
            # Convert ROS Image to OpenCV image
            cv_image = self.bridge.imgmsg_to_cv2(msg, desired_encoding='rgb16')
            # Convert RGB to BGR for OpenCV display
            cv_image_bgr = cv2.cvtColor(cv_image, cv2.COLOR_RGB2BGR)
            cv2.imshow("Kinect RGB Stream", cv_image_bgr)
            cv2.waitKey(1)
        except Exception as e:
            self.get_logger().error(f"Failed to convert image: {e}")

def main(args=None):
    rclpy.init(args=args)
    node = RGBViewer()
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
