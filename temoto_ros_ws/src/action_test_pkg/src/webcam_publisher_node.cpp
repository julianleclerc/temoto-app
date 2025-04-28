#include "action_test_pkg/webcam_publisher_node.hpp"

#include <opencv2/opencv.hpp>
#include <vector>
#include <string>
#include <iostream>

namespace action_test_pkg {

WebcamPublisherNode::WebcamPublisherNode(const rclcpp::NodeOptions & options)
: Node("webcam_publisher_node", options)
{
  // Debug message for node creation
  RCLCPP_INFO(this->get_logger(), "=== STARTING WEBCAM PUBLISHER NODE INITIALIZATION ===");
  
  // Declare parameters with default values
  this->declare_parameter("camera_index", 0);
  this->declare_parameter("frame_width", 640);
  this->declare_parameter("frame_height", 480);
  this->declare_parameter("fps", 30);
  this->declare_parameter("encoding", "bgr8");
  
  // Get parameters
  camera_index_ = this->get_parameter("camera_index").as_int();
  frame_width_ = this->get_parameter("frame_width").as_int();
  frame_height_ = this->get_parameter("frame_height").as_int();
  fps_ = this->get_parameter("fps").as_int();
  encoding_ = this->get_parameter("encoding").as_string();
  
  RCLCPP_INFO(this->get_logger(), "Parameters loaded: camera_index=%d, frame_width=%d, frame_height=%d, fps=%d, encoding=%s", 
              camera_index_, frame_width_, frame_height_, fps_, encoding_.c_str());
  
  // Initialize the camera publisher
  camera_pub_ = this->create_publisher<sensor_msgs::msg::Image>(
    "/cam_feed", 10);
  RCLCPP_INFO(this->get_logger(), "Created publisher on topic: /cam_feed");
  
  // Initialize the webcam
  if (!initializeWebcam()) {
    RCLCPP_ERROR(this->get_logger(), "Failed to initialize webcam! Node will not function properly.");
    return;
  }
  
  // Create timer that calls the callback at the specified fps
  auto timer_period = std::chrono::milliseconds(1000 / fps_);
  timer_ = this->create_wall_timer(
    timer_period, std::bind(&WebcamPublisherNode::timerCallback, this));
  RCLCPP_INFO(this->get_logger(), "Created timer with period %d ms", 1000 / fps_);
  
  RCLCPP_INFO(this->get_logger(), "=== WEBCAM PUBLISHER NODE INITIALIZATION COMPLETE ===");
}

WebcamPublisherNode::~WebcamPublisherNode() {
  RCLCPP_INFO(this->get_logger(), "Webcam Publisher Node shutting down");
  
  // Release the webcam
  if (cap_.isOpened()) {
    cap_.release();
    RCLCPP_INFO(this->get_logger(), "Webcam released");
  }
}

bool WebcamPublisherNode::initializeWebcam() {
  RCLCPP_INFO(this->get_logger(), "Initializing webcam with index %d", camera_index_);
  
  // Open the webcam
  cap_.open(camera_index_);
  if (!cap_.isOpened()) {
    RCLCPP_ERROR(this->get_logger(), "Failed to open webcam with index %d", camera_index_);
    return false;
  }
  
  // Set webcam properties
  cap_.set(cv::CAP_PROP_FRAME_WIDTH, frame_width_);
  cap_.set(cv::CAP_PROP_FRAME_HEIGHT, frame_height_);
  cap_.set(cv::CAP_PROP_FPS, fps_);
  
  // Check if properties were set correctly
  int actual_width = cap_.get(cv::CAP_PROP_FRAME_WIDTH);
  int actual_height = cap_.get(cv::CAP_PROP_FRAME_HEIGHT);
  int actual_fps = cap_.get(cv::CAP_PROP_FPS);
  
  RCLCPP_INFO(this->get_logger(), "Webcam initialized with properties: width=%d, height=%d, fps=%d", 
              actual_width, actual_height, actual_fps);
  
  // Test capture to ensure the webcam is working
  cv::Mat test_frame;
  if (!cap_.read(test_frame)) {
    RCLCPP_ERROR(this->get_logger(), "Failed to capture test frame from webcam");
    cap_.release();
    return false;
  }
  
  RCLCPP_INFO(this->get_logger(), "Successfully captured test frame from webcam: %dx%d, channels=%d", 
              test_frame.cols, test_frame.rows, test_frame.channels());
  
  return true;
}

void WebcamPublisherNode::timerCallback() {
  try {
    // Capture frame from webcam
    cv::Mat frame;
    if (!cap_.read(frame)) {
      RCLCPP_ERROR(this->get_logger(), "Failed to capture frame from webcam");
      return;
    }
    
    // Ensure frame is not empty
    if (frame.empty()) {
      RCLCPP_ERROR(this->get_logger(), "Captured frame is empty");
      return;
    }
    
    // Convert OpenCV image to ROS Image message
    std_msgs::msg::Header header;
    header.stamp = this->now();
    header.frame_id = "camera_frame";
    
    sensor_msgs::msg::Image::SharedPtr img_msg = cv_bridge::CvImage(
      header, encoding_, frame).toImageMsg();
    
    // Publish the image
    camera_pub_->publish(*img_msg);
    
    // Log periodically (every 30 frames) to avoid too much output
    static int frame_count = 0;
    if (++frame_count % 30 == 0) {
      RCLCPP_INFO(this->get_logger(), "Published frame %d: %dx%d", 
                  frame_count, frame.cols, frame.rows);
    }
  } catch (const std::exception &e) {
    RCLCPP_ERROR(this->get_logger(), "Exception in timer callback: %s", e.what());
  }
}

int runWebcamPublisherNode(int argc, char * argv[]) {
  // Initialize ROS
  rclcpp::init(argc, argv);
  
  // Create node options with default parameters
  rclcpp::NodeOptions options;
  
  // Create and spin the webcam publisher node
  auto node = std::make_shared<WebcamPublisherNode>(options);
  
  // Log that the node is running
  RCLCPP_INFO(node->get_logger(), "Webcam publisher node running. Publishing to /cam_feed");
  
  // Spin the node
  rclcpp::spin(node);
  
  // Clean up
  rclcpp::shutdown();
  return 0;
}

} // namespace action_test_pkg

// NO main function here - it's only in webcam_standalone.cpp