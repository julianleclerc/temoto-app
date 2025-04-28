#include "action_test_pkg/inspection_node.hpp"

#include <opencv2/opencv.hpp>
#include <vector>
#include <string>
#include <iostream>

// For base64 encoding
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <sstream>

InspectionNode::InspectionNode(const rclcpp::NodeOptions & options)
: Node("inspection_node", options),
  first_image_published_(false),
  latest_frame_(nullptr)
{
  // Debug message for node creation
  RCLCPP_INFO(this->get_logger(), "=== STARTING INSPECTION NODE INITIALIZATION ===");
  
  // Declare parameters
  this->declare_parameter("object_of_interest", "unknown");
  this->declare_parameter("instructions", "Perform basic inspection");
  
  // Get parameters
  object_of_interest_ = this->get_parameter("object_of_interest").as_string();
  instructions_ = this->get_parameter("instructions").as_string();
  
  RCLCPP_INFO(this->get_logger(), "Parameters loaded: object_of_interest='%s', instructions='%s'", 
              object_of_interest_.c_str(), instructions_.c_str());
  
  // Initialize publishers
  inspection_pub_ = this->create_publisher<std_msgs::msg::String>(
    "chat_interface_feedback", 10);
  RCLCPP_INFO(this->get_logger(), "Created publisher on topic: chat_interface_feedback");
    
  display_pub_ = this->create_publisher<std_msgs::msg::String>(
    "/display_feed", 10);
  RCLCPP_INFO(this->get_logger(), "Created publisher on topic: /display_feed");

  // First, publish black image to indicate initialization
  RCLCPP_INFO(this->get_logger(), "Publishing black initialization image...");
  publishBlackImage();

  // Initialize subscriber
  camera_sub_ = this->create_subscription<sensor_msgs::msg::Image>(
    "/cam_feed", 10, 
    std::bind(&InspectionNode::cameraCallback, this, std::placeholders::_1));
  RCLCPP_INFO(this->get_logger(), "Created subscription to topic: /cam_feed");
    
  RCLCPP_INFO(this->get_logger(), "Inspection Node initialized with object: %s", 
              object_of_interest_.c_str());
  
  RCLCPP_INFO(this->get_logger(), "=== INSPECTION NODE INITIALIZATION COMPLETE ===");
  RCLCPP_INFO(this->get_logger(), "Waiting for camera feed on /cam_feed...");
}

InspectionNode::~InspectionNode() {
  RCLCPP_INFO(this->get_logger(), "Inspection Node shutting down");
}

void InspectionNode::cameraCallback(const sensor_msgs::msg::Image::SharedPtr msg) {
  RCLCPP_INFO(this->get_logger(), "=== CAMERA CALLBACK RECEIVED ===");
  RCLCPP_INFO(this->get_logger(), "Received image: %dx%d, encoding: %s", 
              msg->width, msg->height, msg->encoding.c_str());
  
  // Store the latest frame
  latest_frame_ = msg;
  
  try {
    // Convert ROS image message to OpenCV image - do this only once
    RCLCPP_INFO(this->get_logger(), "Converting ROS image to OpenCV format...");
    cv_bridge::CvImagePtr cv_ptr = cv_bridge::toCvCopy(msg, sensor_msgs::image_encodings::BGR8);
    cv::Mat image = cv_ptr->image;
    RCLCPP_INFO(this->get_logger(), "Image converted successfully: %dx%d, channels: %d", 
                image.cols, image.rows, image.channels());
    
    // Publish the image to display feed first
    RCLCPP_INFO(this->get_logger(), "Publishing image to display feed...");
    try {
      publishImage(image);
    } catch (const std::exception& e) {
      RCLCPP_ERROR(this->get_logger(), "Failed to publish image: %s", e.what());
      // Continue with inspection even if image publishing fails
    }
    
    // Process image with AI
    RCLCPP_INFO(this->get_logger(), "Processing camera image with AI...");
    try {
      std::string inspection_result = performAIInspection(image);
      
      // Publish the inspection result
      RCLCPP_INFO(this->get_logger(), "Publishing inspection results...");
      publishInspectionResult(inspection_result);
    } catch (const std::exception& e) {
      RCLCPP_ERROR(this->get_logger(), "CRITICAL ERROR in AI inspection: %s", e.what());
      // Publish error message to user
      publishInspectionResult("CRITICAL ERROR during inspection: " + std::string(e.what()) + 
                              "\nPlease check system logs and retry.");
      
      // Note: We're not exiting here as that would terminate the ROS node,
      // which is usually not desired behavior in a ROS system
    }
    
    RCLCPP_INFO(this->get_logger(), "Camera frame processed and published successfully");
  } catch (cv_bridge::Exception& e) {
    RCLCPP_ERROR(this->get_logger(), "CRITICAL ERROR: CV Bridge exception: %s", e.what());
    publishInspectionResult("CRITICAL ERROR: Failed to process camera image: " + std::string(e.what()));
  } catch (std::exception& e) {
    RCLCPP_ERROR(this->get_logger(), "CRITICAL ERROR: Exception in camera callback: %s", e.what());
    publishInspectionResult("CRITICAL ERROR: Failed to process camera image: " + std::string(e.what()));
  }
  
  RCLCPP_INFO(this->get_logger(), "=== CAMERA CALLBACK COMPLETED ===");
}

std::string InspectionNode::performAIInspection(const cv::Mat& image) {
  RCLCPP_INFO(this->get_logger(), "=== STARTING AI INSPECTION PROCESS ===");
  
  // Check if OpenAI API key is set
  const char* api_key = std::getenv("OPENAI_API_KEY");
  if (api_key == nullptr || strlen(api_key) == 0) {
      RCLCPP_ERROR(this->get_logger(), "OPENAI_API_KEY environment variable not set!");
      throw std::runtime_error("OPENAI_API_KEY environment variable not set. Unable to perform AI inspection.");
  }
  
  RCLCPP_INFO(this->get_logger(), "Setting up AI configuration parameters...");
  // Set up the AI configuration
  float temperature = 0.7f;
  int max_tokens = 1024;
  float frequency_penalty = 0.0f;
  float presence_penalty = 0.0f;
  
  RCLCPP_INFO(this->get_logger(), "AI config: temp=%.1f, max_tokens=%d, freq_penalty=%.1f, presence_penalty=%.1f",
              temperature, max_tokens, frequency_penalty, presence_penalty);
  
  // Create the messages for the AI in correct format for GPT-4o
  RCLCPP_INFO(this->get_logger(), "Creating AI message structure...");
  std::vector<ai_core::Message> messages;
  
  // System message to define the AI's role
  messages.push_back({
    "system", 
    "You are an advanced computer vision system designed for image inspection. Analyze exactly what you see in the camera feed image and determine if there are any arnomalities."
  });
  RCLCPP_INFO(this->get_logger(), "Added system message to AI prompt");
  
  // User message with inspection instructions and the image
  std::stringstream prompt;
  prompt << "This is a real-time camera feed image. Analyze what's specifically visible in this image.\n\n";
  prompt << "Object to inspect: " << object_of_interest_ << "\n";
  prompt << "Instructions: " << instructions_ << "\n\n";
  prompt << "Describe:\n";
  prompt << "1. What you can see in this specific camera image\n";
  prompt << "2. If the target object (" << object_of_interest_ << ") is visible\n";
  prompt << "3. Any issues or anomalies";
  
  messages.push_back({
    "user",
    prompt.str()
  });
  
  RCLCPP_INFO(this->get_logger(), "Added user prompt with object='%s' and instructions", 
              object_of_interest_.c_str());
  
  // Call the AI Image Prompt function with the user prompt and image
  RCLCPP_INFO(this->get_logger(), "Sending image to AI service...");
  std::string ai_response = ai_core::AIImagePrompt(
    messages,
    image,
    temperature,
    max_tokens,
    frequency_penalty,
    presence_penalty
  );
  
  RCLCPP_INFO(this->get_logger(), "AI response received (length: %zu bytes)", ai_response.size());
  
  // Log a short preview of the response to help with debugging
  std::string preview = ai_response;
  if (preview.length() > 100) {
      preview = preview.substr(0, 97) + "...";
  }
  RCLCPP_INFO(this->get_logger(), "Response preview: %s", preview.c_str());
  
  // Check if there was an error in the JSON response
  try {
    json response_json = json::parse(ai_response);
    
    // Check for our custom error format
    if (response_json.contains("success") && !response_json["success"].get<bool>()) {
      std::string error_msg = response_json["message"].get<std::string>();
      RCLCPP_ERROR(this->get_logger(), "AI Error: %s", error_msg.c_str());
      throw std::runtime_error("AI service error: " + error_msg);
    }
    
    // Check for OpenAI API error format
    if (response_json.contains("error")) {
      std::string error_message = "Unknown API error";
      if (response_json["error"].contains("message")) {
        error_message = response_json["error"]["message"].get<std::string>();
      }
      RCLCPP_ERROR(this->get_logger(), "OpenAI API Error: %s", error_message.c_str());
      throw std::runtime_error("OpenAI API error: " + error_message);
    }
  } catch (const json::exception&) {
    // If the response is not JSON, it's probably a valid text response
    // Log a preview of the response
    std::string preview = ai_response;
    if (preview.length() > 100) {
      preview = preview.substr(0, 97) + "...";
    }
    RCLCPP_INFO(this->get_logger(), "Response appears to be valid text. Preview: %s", preview.c_str());
    // This is not an error case
  }
  
  RCLCPP_INFO(this->get_logger(), "AI inspection completed successfully");
  RCLCPP_INFO(this->get_logger(), "=== AI INSPECTION PROCESS COMPLETED ===");
  return ai_response;
}

void InspectionNode::publishBlackImage() {
  RCLCPP_INFO(this->get_logger(), "=== PUBLISHING BLACK INITIALIZATION IMAGE ===");
  
  if (!first_image_published_) {
    // Create a simple black image
    RCLCPP_INFO(this->get_logger(), "Creating 640x480 black image...");
    cv::Mat black_image(480, 640, CV_8UC3, cv::Scalar(0, 0, 0));
    
    // Add text to indicate initialization
    RCLCPP_INFO(this->get_logger(), "Adding initialization text to image...");
    cv::putText(black_image, "Inspection Display Initialised", 
                cv::Point(80, 240), cv::FONT_HERSHEY_SIMPLEX, 
                1.0, cv::Scalar(255, 255, 255), 2);
    
    // Publish the black image
    RCLCPP_INFO(this->get_logger(), "Publishing black image to /display_feed...");
    publishImage(black_image);
    
    // Publish initialization message
    RCLCPP_INFO(this->get_logger(), "Publishing initialization message to chat_interface_feedback...");
    publishInspectionResult("Inspection system initialized. Waiting for camera input.");
    
    first_image_published_ = true;
    
    RCLCPP_INFO(this->get_logger(), "Black initialization image published successfully");
  } else {
    RCLCPP_INFO(this->get_logger(), "Black image already published, skipping");
  }
  
  RCLCPP_INFO(this->get_logger(), "=== BLACK IMAGE PUBLISHING COMPLETED ===");
}

void InspectionNode::publishInspectionResult(const std::string& inspection) {
  RCLCPP_INFO(this->get_logger(), "=== PUBLISHING INSPECTION RESULT ===");
  
  try {
    json j;
    j["targets"] = {"David"};
    j["type"] = "response";
    j["message"] = inspection;
    
    std::string json_str = j.dump();
    RCLCPP_INFO(this->get_logger(), "Created JSON message for chat_interface_feedback: %s", 
                json_str.length() > 100 ? (json_str.substr(0, 97) + "...").c_str() : json_str.c_str());
    
    std_msgs::msg::String msg;
    msg.data = json_str;
    
    RCLCPP_INFO(this->get_logger(), "Publishing message to chat_interface_feedback (size: %zu bytes)", 
                msg.data.size());
    inspection_pub_->publish(msg);
    RCLCPP_INFO(this->get_logger(), "Message published successfully to chat_interface_feedback");
  } catch (const std::exception& e) {
    RCLCPP_ERROR(this->get_logger(), "ERROR publishing inspection result: %s", e.what());
  }
  
  RCLCPP_INFO(this->get_logger(), "=== INSPECTION RESULT PUBLISHED ===");
}

void InspectionNode::publishImage(const cv::Mat& image) {
  RCLCPP_INFO(this->get_logger(), "=== PUBLISHING IMAGE TO DISPLAY FEED ===");
  
  try {
    // Encode the image to base64
    RCLCPP_INFO(this->get_logger(), "Encoding image to base64 (image size: %dx%d)...", 
                image.cols, image.rows);
    std::string encoded_image = encodeImageToBase64(image);
    RCLCPP_INFO(this->get_logger(), "Image encoded to base64 (encoded size: %zu bytes)", 
                encoded_image.size());
    
    // Create JSON message
    RCLCPP_INFO(this->get_logger(), "Creating JSON message for display_feed...");
    std::string json_msg = "{\"target\":\"David\",\"name\":\"inspection\",\"image\":\"" 
                          + encoded_image + "\"}";
    
    RCLCPP_INFO(this->get_logger(), "JSON message created (total size: %zu bytes)", json_msg.size());
    
    // Publish the message
    std_msgs::msg::String msg;
    msg.data = json_msg;
    
    RCLCPP_INFO(this->get_logger(), "Publishing message to /display_feed...");
    display_pub_->publish(msg);
    RCLCPP_INFO(this->get_logger(), "Message published successfully to /display_feed");
  } catch (const std::exception& e) {
    RCLCPP_ERROR(this->get_logger(), "ERROR publishing image: %s", e.what());
  }
  
  RCLCPP_INFO(this->get_logger(), "=== IMAGE PUBLISHING COMPLETED ===");
}

std::string InspectionNode::encodeImageToBase64(const cv::Mat& image) {
  RCLCPP_INFO(this->get_logger(), "=== ENCODING IMAGE TO BASE64 ===");
  
  try {
    // Use the ai_core implementation for encoding
    RCLCPP_INFO(this->get_logger(), "Calling ai_core encoding function...");
    std::string result = ai_core::encodeImageToBase64(image);
    RCLCPP_INFO(this->get_logger(), "Image encoded successfully to base64 (size: %zu bytes)", result.size());
    
    // Print the first few characters
    if (result.size() > 20) {
      RCLCPP_INFO(this->get_logger(), "Encoded data begins with: %s...", result.substr(0, 20).c_str());
    }
    
    RCLCPP_INFO(this->get_logger(), "=== BASE64 ENCODING COMPLETED ===");
    return result;
  } catch (const std::exception& e) {
    RCLCPP_ERROR(this->get_logger(), "Error encoding image to base64: %s", e.what());
    RCLCPP_INFO(this->get_logger(), "=== BASE64 ENCODING FAILED ===");
    return "";
  }
}