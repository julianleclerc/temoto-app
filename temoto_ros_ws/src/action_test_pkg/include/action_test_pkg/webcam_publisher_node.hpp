#ifndef ACTION_TEST_PKG_WEBCAM_PUBLISHER_NODE_HPP
#define ACTION_TEST_PKG_WEBCAM_PUBLISHER_NODE_HPP

#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/image.hpp"
#include "cv_bridge/cv_bridge.h"
#include <opencv2/opencv.hpp>

namespace action_test_pkg {

class WebcamPublisherNode : public rclcpp::Node
{
public:
  WebcamPublisherNode(const rclcpp::NodeOptions & options = rclcpp::NodeOptions());
  virtual ~WebcamPublisherNode();

private:
  // Timer for capturing and publishing images
  rclcpp::TimerBase::SharedPtr timer_;
  
  // Publisher for the camera feed
  rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr camera_pub_;
  
  // OpenCV video capture
  cv::VideoCapture cap_;
  
  // Camera parameters
  int camera_index_;
  int frame_width_;
  int frame_height_;
  int fps_;
  std::string encoding_;
  
  // Timer callback for capturing and publishing images
  void timerCallback();
  
  // Initialize webcam
  bool initializeWebcam();
};

/**
 * @brief Create and run a WebcamPublisherNode instance
 * 
 * This function is intended to be called from a separate process to run
 * a standalone webcam publisher node that streams webcam images to /cam_feed
 * 
 * @param argc Command line argument count
 * @param argv Command line arguments
 * @return int Return code
 */
int runWebcamPublisherNode(int argc, char * argv[]);

} // namespace action_test_pkg

#endif // ACTION_TEST_PKG_WEBCAM_PUBLISHER_NODE_HPP