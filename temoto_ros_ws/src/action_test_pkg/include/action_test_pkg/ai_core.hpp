#ifndef ACTION_TEST_PKG_AI_CORE_HPP_
#define ACTION_TEST_PKG_AI_CORE_HPP_

#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <opencv2/opencv.hpp>

using json = nlohmann::json;

namespace ai_core {

struct Message {
    std::string role;
    std::string content;
};

// AI Core configuration
struct AIConfig {
    float temperature;
    int max_tokens;
    float frequency_penalty;
    float presence_penalty;
};

// Get API key from environment variable
std::string getApiKey();

// Process text with language model
std::string AILanguagePrompt(
    const std::vector<Message>& messages,
    float temperature,
    int max_tokens,
    float frequency_penalty,
    float presence_penalty);

// Process image with vision model
std::string AIImagePrompt(
    const std::vector<Message>& messages,
    const cv::Mat& image,
    float temperature,
    int max_tokens,
    float frequency_penalty,
    float presence_penalty);

// Helper to encode image to base64
std::string encodeImageToBase64(const cv::Mat& image);

// Helper to create a message with image content
Message createImageMessage(const std::string& text, const std::string& base64Image);

}  // namespace ai_core

#endif  // ACTION_TEST_PKG_AI_CORE_HPP_