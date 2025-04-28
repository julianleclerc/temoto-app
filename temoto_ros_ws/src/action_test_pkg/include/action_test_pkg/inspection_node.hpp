#ifndef ACTION_TEST_PKG_INSPECTION_NODE_HPP_
#define ACTION_TEST_PKG_INSPECTION_NODE_HPP_

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <std_msgs/msg/string.hpp>
#include <cv_bridge/cv_bridge.h>
#include <image_transport/image_transport.hpp>
#include <nlohmann/json.hpp>
#include "action_test_pkg/ai_core.hpp"

using json = nlohmann::json;

class InspectionNode : public rclcpp::Node {
public:
  InspectionNode(const rclcpp::NodeOptions & options = rclcpp::NodeOptions());
  ~InspectionNode();

private:
  // Callback for camera subscription
  void cameraCallback(const sensor_msgs::msg::Image::SharedPtr msg);
  
  // Method to perform AI-based inspection
  std::string performAIInspection(const cv::Mat& image);
  
  // Method to publish inspection results to the user
  void publishInspectionResult(const std::string& inspection);
  
  // Method to publish image to display feed
  void publishImage(const cv::Mat& image);
  
  // Method to create and publish a black image on startup
  void publishBlackImage();
  
  // Method to encode image to base64
  std::string encodeImageToBase64(const cv::Mat& image);
  
  // Latest camera frame
  sensor_msgs::msg::Image::SharedPtr latest_frame_;
  
  // Subscriber for camera feed
  rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr camera_sub_;
  
  // Publisher for inspection results
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr inspection_pub_;
  
  // Publisher for display feed
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr display_pub_;
  
  // Parameters
  std::string object_of_interest_;
  std::string instructions_;
  bool first_image_published_;
};

#endif // ACTION_TEST_PKG_INSPECTION_NODE_HPP_