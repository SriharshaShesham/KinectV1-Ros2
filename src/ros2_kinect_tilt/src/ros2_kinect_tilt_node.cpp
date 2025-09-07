#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>
#include <std_srvs/srv/trigger.hpp>
#include <libfreenect.h>
#include <libfreenect_sync.h>
#include <sstream>
#include <cmath>

class Ros2KinectTiltNode : public rclcpp::Node {
public:
    Ros2KinectTiltNode()
    : Node("ros2_kinect_tilt_node"),
      current_tilt_(0),
      at_max_up_(false),
      at_max_down_(false)
    {
        cmd_sub_ = this->create_subscription<std_msgs::msg::String>(
            "/kinect/tilt_cmd", 10,
            std::bind(&Ros2KinectTiltNode::cmdCallback, this, std::placeholders::_1)
        );

        state_pub_ = this->create_publisher<std_msgs::msg::String>(
            "/kinect/tilt_state", 10
        );

        feedback_pub_ = this->create_publisher<std_msgs::msg::String>(
            "/kinect/tilt_feedback", 10
        );

        limit_srv_ = this->create_service<std_srvs::srv::Trigger>(
            "/kinect/tilt_limit_status",
            std::bind(&Ros2KinectTiltNode::limitService, this,
                      std::placeholders::_1, std::placeholders::_2)
        );

        timer_ = this->create_wall_timer(
            std::chrono::seconds(1),
            std::bind(&Ros2KinectTiltNode::publishState, this)
        );

        RCLCPP_INFO(this->get_logger(), "Ros2 Kinect Tilt Node started");
    }

    ~Ros2KinectTiltNode() {
        freenect_sync_stop();
    }

private:
    static constexpr int MAX_UP_DEG = 30;
    static constexpr int MIN_DOWN_DEG = -30;
    const int step_ = 5;

    void cmdCallback(const std_msgs::msg::String::SharedPtr msg) {
        std::istringstream iss(msg->data);
        std::string cmd;
        iss >> cmd;

        int desired = current_tilt_;

        if (cmd == "up") {
            desired += step_;
        } else if (cmd == "down") {
            desired -= step_;
        } else if (cmd == "center") {
            desired = 0;
        } else if (cmd == "set") {
            int angle;
            iss >> angle;
            desired = angle;
        } else {
            RCLCPP_WARN(this->get_logger(), "Unknown command: %s", cmd.c_str());
            return;
        }

        // Clamp to limits
        int clamped = std::max(MIN_DOWN_DEG, std::min(MAX_UP_DEG, desired));

        // If command tries to go past limits, notify
        if (desired != clamped) {
            std_msgs::msg::String fb;
            fb.data = (desired > MAX_UP_DEG)
                          ? "Limit reached: MAX UP"
                          : "Limit reached: MAX DOWN";
            feedback_pub_->publish(fb);
            RCLCPP_WARN(this->get_logger(), "%s", fb.data.c_str());
        }

        current_tilt_ = clamped;

        if (freenect_sync_set_tilt_degs(current_tilt_, 0)) {
            RCLCPP_ERROR(this->get_logger(), "Failed to set tilt");
        } else {
            RCLCPP_INFO(this->get_logger(), "Tilt set to %d degrees", current_tilt_);
            // Publish updated state immediately after a successful set
            publishState();
        }
    }

    void publishState() {
        freenect_raw_tilt_state *state = nullptr;
        if (freenect_sync_get_tilt_state(&state, 0)) {
            RCLCPP_ERROR(this->get_logger(), "Failed to get tilt state");
            return;
        }

        double ax, ay, az;
        freenect_get_mks_accel(state, &ax, &ay, &az);
        double angle_deg = freenect_get_tilt_degs(state);

        // Update limit flags based on actual device state
        bool was_max_up = at_max_up_;
        bool was_max_down = at_max_down_;
        at_max_up_ = angle_deg >= MAX_UP_DEG - 0.5;     // small hysteresis
        at_max_down_ = angle_deg <= MIN_DOWN_DEG + 0.5;

        std_msgs::msg::String msg;
        std::ostringstream oss;
        oss << "Tilt: " << static_cast<int>(std::round(angle_deg))
            << " deg, Accel: " << ax << ", " << ay << ", " << az
            << ", Limit: "
            << (at_max_up_ ? "MAX_UP" : (at_max_down_ ? "MAX_DOWN" : "NONE"));
        msg.data = oss.str();

        state_pub_->publish(msg);

        // Notify on transitions to a limit
        if (!was_max_up && at_max_up_) {
            std_msgs::msg::String fb; fb.data = "Reached MAX UP";
            feedback_pub_->publish(fb);
            RCLCPP_INFO(this->get_logger(), "%s", fb.data.c_str());
        }
        if (!was_max_down && at_max_down_) {
            std_msgs::msg::String fb; fb.data = "Reached MAX DOWN";
            feedback_pub_->publish(fb);
            RCLCPP_INFO(this->get_logger(), "%s", fb.data.c_str());
        }
    }

    void limitService(const std::shared_ptr<std_srvs::srv::Trigger::Request>,
                      std::shared_ptr<std_srvs::srv::Trigger::Response> res)
    {
        freenect_raw_tilt_state *state = nullptr;
        if (freenect_sync_get_tilt_state(&state, 0)) {
            res->success = false;
            res->message = "Failed to read tilt state";
            return;
        }
        double angle_deg = freenect_get_tilt_degs(state);
        if (angle_deg >= MAX_UP_DEG - 0.5) {
            res->success = true;
            res->message = "MAX UP";
        } else if (angle_deg <= MIN_DOWN_DEG + 0.5) {
            res->success = true;
            res->message = "MAX DOWN";
        } else {
            res->success = false;
            res->message = "Within range";
        }
    }

    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr cmd_sub_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr state_pub_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr feedback_pub_;
    rclcpp::TimerBase::SharedPtr timer_;
    rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr limit_srv_;

    int current_tilt_;
    bool at_max_up_;
    bool at_max_down_;
};

int main(int argc, char *argv[]) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<Ros2KinectTiltNode>());
    rclcpp::shutdown();
    return 0;
}
