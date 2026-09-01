#include <memory>
#include <string>
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"
#include "ros2_kdl_package/action/execute_trajectory.hpp"
#include "geometry_msgs/msg/point.hpp"

using ExecuteTrajectory = ros2_kdl_package::action::ExecuteTrajectory;
using GoalHandleExecuteTrajectory = rclcpp_action::ClientGoalHandle<ExecuteTrajectory>;

class TrajectoryActionClient : public rclcpp::Node
{
public:
    explicit TrajectoryActionClient(const rclcpp::NodeOptions & options)
    : Node("trajectory_action_client", options)
    {
        this->client_ptr_ = rclcpp_action::create_client<ExecuteTrajectory>(
            this,
            "execute_trajectory"); 
    }

    void send_goal(double x, double y, double z)
    {
        using namespace std::placeholders;

        if (!this->client_ptr_->wait_for_action_server(std::chrono::seconds(10))) {
            RCLCPP_ERROR(this->get_logger(), "Action server unavailable after 10s");
            rclcpp::shutdown();
            return;
        }

        auto goal_msg = ExecuteTrajectory::Goal();
        goal_msg.target_position.x = x;
        goal_msg.target_position.y = y;
        goal_msg.target_position.z = z;

        RCLCPP_INFO(this->get_logger(), "Send goal: [x: %f, y: %f, z: %f]", x, y, z);

        auto send_goal_options = rclcpp_action::Client<ExecuteTrajectory>::SendGoalOptions();
        
        send_goal_options.goal_response_callback =
            std::bind(&TrajectoryActionClient::goal_response_callback, this, _1);
        send_goal_options.feedback_callback =
            std::bind(&TrajectoryActionClient::feedback_callback, this, _1, _2);
        send_goal_options.result_callback =
            std::bind(&TrajectoryActionClient::result_callback, this, _1);
        
        this->client_ptr_->async_send_goal(goal_msg, send_goal_options);
    }

private:
    rclcpp_action::Client<ExecuteTrajectory>::SharedPtr client_ptr_;

    void goal_response_callback(const GoalHandleExecuteTrajectory::SharedPtr & goal_handle)
    {
        if (!goal_handle) {
            RCLCPP_ERROR(this->get_logger(), "Goal rejected from the server");
        } else {
            RCLCPP_INFO(this->get_logger(), "Goal accepted from the server, waiting for the result");
        }
    }

    void feedback_callback(
        GoalHandleExecuteTrajectory::SharedPtr,
        const std::shared_ptr<const ExecuteTrajectory::Feedback> feedback)
    {
        RCLCPP_INFO(this->get_logger(), "Feedback received: Error = %f", feedback->position_error);
    }

    void result_callback(const GoalHandleExecuteTrajectory::WrappedResult & result)
    {
        switch (result.code) {
            case rclcpp_action::ResultCode::SUCCEEDED:
                RCLCPP_INFO(this->get_logger(), "Goal reach! Success: %s", result.result->success ? "true" : "false");
                break;
            case rclcpp_action::ResultCode::ABORTED:
                RCLCPP_ERROR(this->get_logger(), "Goal aborted");
                break;
            case rclcpp_action::ResultCode::CANCELED:
                RCLCPP_ERROR(this->get_logger(), "Goal cancelled");
                break;
            default:
                RCLCPP_ERROR(this->get_logger(), "State unknow");
                break;
        }
        rclcpp::shutdown();
    }
};

int main(int argc, char ** argv)
{
    rclcpp::init(argc, argv);

    if (argc != 4) {
        RCLCPP_ERROR(rclcpp::get_logger("main"), "Uso: ros2 run ros2_kdl_package action_client_node <x> <y> <z>");
        return 1;
    }

    auto action_client = std::make_shared<TrajectoryActionClient>(rclcpp::NodeOptions());
    
    try {
        double x = std::stod(argv[1]);
        double y = std::stod(argv[2]);
        double z = std::stod(argv[3]);
        action_client->send_goal(x, y, z);
    } catch (const std::invalid_argument& e) {
        RCLCPP_ERROR(rclcpp::get_logger("main"), "Invalid arguments. x, y, z need to be number.");
        return 1;
    }

    rclcpp::spin(action_client);
    rclcpp::shutdown();
    return 0;
}
