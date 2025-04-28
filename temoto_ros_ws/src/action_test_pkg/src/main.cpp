#include "action_test_pkg/inspection_node.hpp"
#include <rclcpp/rclcpp.hpp>

int main(int argc, char * argv[])
{
  // Initialize ROS
  rclcpp::init(argc, argv);
  
  // Create the inspection node
  auto node = std::make_shared<InspectionNode>();
  
  // Spin the node
  rclcpp::spin(node);
  
  // Shutdown ROS
  rclcpp::shutdown();
  
  return 0;
}