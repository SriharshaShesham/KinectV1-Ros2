#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>
#include <libfreenect.h>
#include <libfreenect_sync.h>
#include <unordered_map>

class KinectLedNode : public rclcpp::Node {
public:
    KinectLedNode() : Node("ros2_kinect_led_node") {
        cmd_sub_ = create_subscription<std_msgs::msg::String>(
            "/kinect/led_cmd", 10,
            std::bind(&KinectLedNode::cmdCallback, this, std::placeholders::_1)
        );

        state_pub_ = create_publisher<std_msgs::msg::String>(
            "/kinect/led_state", 10
        );

        RCLCPP_INFO(get_logger(), "Kinect LED node started");
    }

    ~KinectLedNode() {
        freenect_sync_stop();
    }

private:
    void cmdCallback(const std_msgs::msg::String::SharedPtr msg) {
        auto it = led_map_.find(msg->data);
        if (it == led_map_.end()) {
            RCLCPP_WARN(get_logger(), "Unknown LED command: %s", msg->data.c_str());
            return;
        }

        if (freenect_sync_set_led(it->second, 0) != 0) {
            RCLCPP_ERROR(get_logger(), "Failed to set LED");
            return;
        }

        RCLCPP_INFO(get_logger(), "LED set to: %s", msg->data.c_str());

        std_msgs::msg::String state_msg;
        state_msg.data = msg->data;
        state_pub_->publish(state_msg);
    }

    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr cmd_sub_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr state_pub_;

    const std::unordered_map<std::string, freenect_led_options> led_map_ = {
        {"off", LED_OFF},
        {"green", LED_GREEN},
        {"red", LED_RED},
        {"yellow", LED_YELLOW},
        {"blink_green", LED_BLINK_GREEN},
        {"blink_red_yellow", LED_BLINK_RED_YELLOW}
    };
};

int main(int argc, char *argv[]) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<KinectLedNode>());
    rclcpp::shutdown();
    return 0;
}
