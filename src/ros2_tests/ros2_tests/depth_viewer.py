import rclpy
from rclpy.node import Node
from sensor_msgs.msg import Image
from cv_bridge import CvBridge
import cv2
import numpy as np

class DepthViewer(Node):
    def __init__(self):
        super().__init__('depth_viewer')
        self.bridge = CvBridge()
        self.subscription = self.create_subscription(
            Image,
            '/kinect/depth/image_raw',
            self.listener_callback,
            10
        )
        self.get_logger().info("Depth Viewer node started. Waiting for depth images...")

    def listener_callback(self, msg):
        try:
            # Convert ROS Image to OpenCV image
            cv_image = self.bridge.imgmsg_to_cv2(msg, desired_encoding='passthrough')

            # Normalize depth for display (0..255)
            depth_normalized = cv2.normalize(cv_image, None, 0, 255, cv2.NORM_MINMAX)
            depth_normalized = np.uint8(depth_normalized)

            # Apply a colormap for better visualization
            depth_colored = cv2.applyColorMap(depth_normalized, cv2.COLORMAP_JET)

            cv2.imshow("Kinect Depth Stream", depth_colored)
            cv2.waitKey(1)
        except Exception as e:
            self.get_logger().error(f"Failed to process depth image: {e}")

def main(args=None):
    rclpy.init(args=args)
    node = DepthViewer()
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
