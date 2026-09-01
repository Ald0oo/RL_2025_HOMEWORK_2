#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"
#include "ros2_kdl_package/action/execute_trajectory.hpp"

class TrajectoryActionClient : public rclcpp::Node {
public:
  using ExecuteTrajectory = ros2_kdl_package::action::ExecuteTrajectory;
  using GoalHandleExecuteTrajectory = rclcpp_action::ClientGoalHandle<ExecuteTrajectory>;

  explicit TrajectoryActionClient(const rclcpp::NodeOptions & options = rclcpp::NodeOptions())
  : Node("ros2_kdl_action_client", options)
  {
    this->client_ptr_ = rclcpp_action::create_client<ExecuteTrajectory>(
      this,
      "execute_trajectory");
  }

  void send_goal()
  {
    if (!this->client_ptr_->wait_for_action_server(std::chrono::seconds(10))) {
      RCLCPP_ERROR(this->get_logger(), "Action server non disponibile!");
      return;
    }

    auto goal_msg = ExecuteTrajectory::Goal();
    goal_msg.target_position.x = 0.5;
    goal_msg.target_position.y = 0.2;
    goal_msg.target_position.z = 0.4;

    auto send_goal_options = rclcpp_action::Client<ExecuteTrajectory>::SendGoalOptions();
    
    // Callback per la ricezione del FEEDBACK
    send_goal_options.feedback_callback =
      std::bind(&TrajectoryActionClient::feedback_callback, this, std::placeholders::_1, std::placeholders::_2);

    // Callback per la ricezione del RISULTATO FINALE
    send_goal_options.result_callback =
      std::bind(&TrajectoryActionClient::result_callback, this, std::placeholders::_1);

    this->client_ptr_->async_send_goal(goal_msg, send_goal_options);
  }

private:
  rclcpp_action::Client<ExecuteTrajectory>::SharedPtr client_ptr_;

  void feedback_callback(
    GoalHandleExecuteTrajectory::SharedPtr,
    const std::shared_ptr<const ExecuteTrajectory::Feedback> feedback)
  {
    RCLCPP_INFO(this->get_logger(), "Feedback - Position Error: %.4f", feedback->position_error);
  }

  void result_callback(const GoalHandleExecuteTrajectory::WrappedResult & result)
  {
    switch (result.code) {
      case rclcpp_action::ResultCode::SUCCEEDED:
        RCLCPP_INFO(this->get_logger(), "Azione completata con successo!");
        break;
      default:
        RCLCPP_ERROR(this->get_logger(), "Azione fallita o annullata.");
        break;
    }
    rclcpp::shutdown();
  }
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<TrajectoryActionClient>();
  node->send_goal();
  rclcpp::spin(node);
  return 0;
}
