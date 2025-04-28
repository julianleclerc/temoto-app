cmake_minimum_required(VERSION 3.8)
project(action_test_pkg)

# Default to C++17
if(NOT CMAKE_CXX_STANDARD)
 set(CMAKE_CXX_STANDARD 17)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
 add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(image_transport REQUIRED)
find_package(OpenCV REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(rclcpp_components REQUIRED)
find_package(CURL REQUIRED)

# Include directories
include_directories(
 include
 ${OpenCV_INCLUDE_DIRS}
)

# Declare the inspection node executable
add_executable(inspection_node
 src/main.cpp
 src/inspection_node.cpp
 src/ai_core.cpp
)

target_link_libraries(inspection_node
 ${OpenCV_LIBRARIES}
 ${CURL_LIBRARIES}
)

ament_target_dependencies(inspection_node
 rclcpp
 sensor_msgs
 std_msgs
 cv_bridge
 image_transport
 nlohmann_json
 rclcpp_components
)

# Declare the webcam publisher node executable
add_executable(webcam_publisher
 src/webcam_standalone.cpp
 src/webcam_publisher_node.cpp
)

target_link_libraries(webcam_publisher
 ${OpenCV_LIBRARIES}
)

ament_target_dependencies(webcam_publisher
 rclcpp
 sensor_msgs
 std_msgs
 cv_bridge
)

# Install the executables
install(TARGETS
 inspection_node
 webcam_publisher
 DESTINATION lib/${PROJECT_NAME}
)

# Install header files
install(
 DIRECTORY include/
 DESTINATION include
)

if(BUILD_TESTING)
 find_package(ament_lint_auto REQUIRED)
 ament_lint_auto_find_test_dependencies()
endif()

ament_package()