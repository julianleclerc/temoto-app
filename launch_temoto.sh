#!/bin/bash

# Get the directory where the script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Set paths relative to the script location
TEMOTO_APP_DIR="${SCRIPT_DIR}"
TEMOTO_ACTION_DIR="${TEMOTO_APP_DIR}/temoto_action_assistant"
TEMOTO_ROS_WS="${TEMOTO_APP_DIR}/temoto_ros_ws"

# Store PIDs for cleanup
FRONTEND_PID=""
BACKEND_PID=""
ROS_PID=""

# Function to handle Ctrl+C and cleanup
cleanup() {
    echo "Cleaning up and terminating all processes..."
    
    # Kill any processes we started
    if [ -n "$FRONTEND_PID" ] && ps -p $FRONTEND_PID > /dev/null; then
        echo "Killing frontend process (PID: $FRONTEND_PID)"
        kill -9 $FRONTEND_PID 2>/dev/null || true
    fi
    
    if [ -n "$BACKEND_PID" ] && ps -p $BACKEND_PID > /dev/null; then
        echo "Killing backend process (PID: $BACKEND_PID)"
        kill -9 $BACKEND_PID 2>/dev/null || true
    fi
    
    if [ -n "$ROS_PID" ] && ps -p $ROS_PID > /dev/null; then
        echo "Killing ROS process (PID: $ROS_PID)"
        kill -9 $ROS_PID 2>/dev/null || true
    fi
    
    # Kill any remaining npm/node processes related to our directory
    echo "Checking for remaining processes..."
    pkill -f "node.*$TEMOTO_ACTION_DIR" 2>/dev/null || true
    
    # Kill any Python backend processes
    pkill -f "python3.*app.py" 2>/dev/null || true
    
    # Check if port 4000 is still in use
    if netstat -tuln 2>/dev/null | grep -q ":4000 "; then
        echo "Port 4000 still in use, attempting to free it..."
        fuser -k 4000/tcp 2>/dev/null || true
    fi
    
    echo "Cleanup complete."
    exit 0
}

# Check if port 4000 is in use before starting
check_port() {
    local port=$1
    if netstat -tuln 2>/dev/null | grep -q ":$port "; then
        echo "WARNING: Port $port is already in use."
        echo "Attempting to free the port..."
        fuser -k $port/tcp 2>/dev/null
        sleep 2
        if netstat -tuln 2>/dev/null | grep -q ":$port "; then
            echo "ERROR: Failed to free port $port. Please manually stop the process using this port."
            echo "You can use: fuser -k $port/tcp"
            exit 1
        else
            echo "Port $port successfully freed."
        fi
    fi
}

# Set trap for Ctrl+C and other termination signals
trap cleanup SIGINT SIGTERM EXIT

# Function to start the webapp components
start_webapp() {
    echo "Starting webapp components..."
    
    # Create log directory if it doesn't exist
    mkdir -p "$TEMOTO_APP_DIR/logs"

    # Start the backend first to ensure it's ready when frontend connects
    BACKEND_LOG="$TEMOTO_APP_DIR/logs/backend.log"
    cd "$TEMOTO_ACTION_DIR/backend" || { echo "Backend directory not found: $TEMOTO_ACTION_DIR/backend"; exit 1; }
    echo "Launching backend: python3 app.py --runtime --chat"
    
    # Modify the backend code to include allow_unsafe_werkzeug=True
    # First, check if we need to make a backup
    if [ ! -f "$TEMOTO_ACTION_DIR/backend/app.py.bak" ]; then
        echo "Creating backup of original app.py"
        cp "$TEMOTO_ACTION_DIR/backend/app.py" "$TEMOTO_ACTION_DIR/backend/app.py.bak"
    fi
    
    # Check if the fix is already applied
    if ! grep -q "allow_unsafe_werkzeug=True" "$TEMOTO_ACTION_DIR/backend/app.py"; then
        echo "Modifying backend code to work with newer Flask-SocketIO versions..."
        # Use sed to modify the line containing socketio.run
        sed -i 's/socketio.run(app, host=.*)/socketio.run(app, host='"'"'0.0.0.0'"'"', port=4000, allow_unsafe_werkzeug=True)/' "$TEMOTO_ACTION_DIR/backend/app.py"
    fi
    
    if [ "$DEBUG_MODE" = true ]; then
        # Run in foreground with output to console when in debug mode
        echo "Running backend in debug mode (foreground)"
        python3 app.py --runtime --chat &
        BACKEND_PID=$!
    else
        # Run in background with output to log file
        python3 app.py --runtime --chat > "$BACKEND_LOG" 2>&1 &
        BACKEND_PID=$!
    fi
    
    echo "Backend started with PID: $BACKEND_PID"
    
    # Give the backend a moment to initialize
    echo "Waiting for backend to initialize..."
    sleep 5
    
    if ! ps -p $BACKEND_PID > /dev/null; then
        echo "ERROR: Backend process failed to start or terminated."
        if [ -f "$BACKEND_LOG" ]; then
            echo "Last 20 lines of backend log:"
            tail -n 20 "$BACKEND_LOG"
            echo "Full logs available at: $BACKEND_LOG"
        fi
        exit 1
    else
        echo "Backend running with PID: $BACKEND_PID"
    fi

    # Start the frontend
    FRONTEND_LOG="$TEMOTO_APP_DIR/logs/frontend.log"
    cd "$TEMOTO_ACTION_DIR" || { echo "Frontend directory not found: $TEMOTO_ACTION_DIR"; exit 1; }
    echo "Launching frontend: npm start"
    
    if [ "$DEBUG_MODE" = true ]; then
        # Run in foreground with output to console when in debug mode
        echo "Running frontend in debug mode (foreground)"
        npm start &
        FRONTEND_PID=$!
    else
        # Run in background with output to log file
        npm start > "$FRONTEND_LOG" 2>&1 &
        FRONTEND_PID=$!
    fi
    
    echo "Frontend started with PID: $FRONTEND_PID"
    
    # Check if frontend process is running
    echo "Waiting for frontend to initialize..."
    sleep 3
    
    if ! ps -p $FRONTEND_PID > /dev/null; then
        echo "ERROR: Frontend process failed to start or terminated."
        if [ -f "$FRONTEND_LOG" ]; then
            echo "Last 20 lines of frontend log:"
            tail -n 20 "$FRONTEND_LOG"
            echo "Full logs available at: $FRONTEND_LOG"
        fi
        exit 1
    else
        echo "Frontend running with PID: $FRONTEND_PID"
    fi
    
    echo "Webapp started successfully."
    echo "Backend PID: $BACKEND_PID, Frontend PID: $FRONTEND_PID"
    echo "Logs available at: $TEMOTO_APP_DIR/logs/"
    
    # Print URLs
    echo "Frontend should be available at: http://localhost:3000"
    echo "Backend should be available at: http://localhost:5000"
}

# Function to start the ROS package
start_ros() {
    echo "Starting ROS components..."
    
    # Create log directory if it doesn't exist
    mkdir -p "$TEMOTO_APP_DIR/logs"
    ROS_LOG="$TEMOTO_APP_DIR/logs/ros.log"
    
    # Source the ROS workspace
    echo "Sourcing ROS workspace"
    source "$TEMOTO_ROS_WS/install/setup.bash" || { echo "Failed to source ROS workspace: $TEMOTO_ROS_WS/install/setup.bash"; exit 1; }

    # Check if ROS is properly sourced
    if ! command -v ros2 &> /dev/null; then
        echo "ERROR: ros2 command not found after sourcing workspace"
        echo "Please check that your ROS installation is working correctly"
        echo "You might need to source the ROS installation first:"
        echo "  For ROS2 Foxy: source /opt/ros/foxy/setup.bash"
        echo "  For ROS2 Humble: source /opt/ros/humble/setup.bash"
        echo "  For ROS2 Iron: source /opt/ros/iron/setup.bash"
        exit 1
    fi
    
    # Print ROS environment info in debug mode
    if [ "$DEBUG_MODE" = true ]; then
        echo "=== ROS Environment Information ==="
        echo "ROS_DISTRO: $ROS_DISTRO"
        echo "ROS_VERSION: $ROS_VERSION"
        echo "ROS_PYTHON_VERSION: $ROS_PYTHON_VERSION"
        echo "ROS_DOMAIN_ID: $ROS_DOMAIN_ID"
        echo "=== End ROS Environment Information ==="
    fi

    # Launch the ROS package
    echo "Launching ROS package: ros2 launch temoto_nl_interface launch_nl_interface.py"
    
    if [ "$DEBUG_MODE" = true ]; then
        # Run in foreground with output to console when in debug mode
        ros2 launch temoto_nl_interface launch_nl_interface.py
    else
        # If we're only launching ROS, run in foreground
        # If launching both components, run in background
        if [ "$LAUNCH_WEBAPP" = true ]; then
            ros2 launch temoto_nl_interface launch_nl_interface.py > "$ROS_LOG" 2>&1 &
            ROS_PID=$!
            echo "ROS package started with PID: $ROS_PID"
            echo "ROS logs available at: $ROS_LOG"
        else
            # Run in foreground
            ros2 launch temoto_nl_interface launch_nl_interface.py
        fi
    fi
    
    echo "ROS package started successfully."
}

# Parse command line arguments
LAUNCH_WEBAPP=false
LAUNCH_ROS=false
DEBUG_MODE=false

# If no arguments provided, launch both
if [ $# -eq 0 ]; then
    LAUNCH_WEBAPP=true
    LAUNCH_ROS=true
else
    # Process arguments
    for arg in "$@"; do
        case $arg in
            --webapp)
                LAUNCH_WEBAPP=true
                ;;
            --nl_hri)
                LAUNCH_ROS=true
                ;;
            --debug)
                DEBUG_MODE=true
                echo "Debug mode enabled - all logs will be printed to console"
                ;;
            --help)
                echo "Usage: $0 [options]"
                echo "Options:"
                echo "  --webapp    Launch only the web application"
                echo "  --nl_hri    Launch only the ROS package"
                echo "  --debug     Show all log output to console"
                echo "  --help      Show this help message"
                echo ""
                echo "If no options are provided, both webapp and ROS package will be launched."
                exit 0
                ;;
            *)
                echo "Unknown option: $arg"
                echo "Use --help for usage information."
                exit 1
                ;;
        esac
    done
fi

# Print environment info in debug mode
if [ "$DEBUG_MODE" = true ]; then
    echo "=== Environment Information ==="
    echo "Script directory: $SCRIPT_DIR"
    echo "Temoto app directory: $TEMOTO_APP_DIR"
    echo "Action assistant directory: $TEMOTO_ACTION_DIR"
    echo "ROS workspace directory: $TEMOTO_ROS_WS"
    echo "Current working directory: $(pwd)"
    echo "Python version: $(python3 --version)"
    echo "Node version: $(node --version)"
    echo "NPM version: $(npm --version)"
    echo "=== End Environment Information ==="
fi

# Print status message
echo "=== Temoto Launcher ==="
echo "Starting requested components..."
if [ "$LAUNCH_WEBAPP" = true ] && [ "$LAUNCH_ROS" = true ]; then
    echo "Mode: Full system (webapp + ROS)"
elif [ "$LAUNCH_WEBAPP" = true ]; then
    echo "Mode: Webapp only"
elif [ "$LAUNCH_ROS" = true ]; then
    echo "Mode: ROS only"
fi
echo "===================="

# Check for existing processes before starting
if command -v pkill &>/dev/null; then
    echo "Checking for existing Temoto processes..."
    # Kill any existing processes that might interfere
    pkill -f "node.*$TEMOTO_ACTION_DIR" 2>/dev/null || true
    pkill -f "python3.*app.py" 2>/dev/null || true
    # Give processes time to terminate
    sleep 2
fi

# Launch components based on arguments
if [ "$LAUNCH_WEBAPP" = true ]; then
    start_webapp
fi

if [ "$LAUNCH_ROS" = true ]; then
    start_ros
fi

# If only launching webapp or both components in background, wait for processes
if [ "$LAUNCH_WEBAPP" = true ] || ([ "$LAUNCH_WEBAPP" = true ] && [ "$LAUNCH_ROS" = true ] && [ "$DEBUG_MODE" = false ]); then
    echo ""
    echo "All components started. Press Ctrl+C to stop all processes."
    wait
fi

echo "All processes have terminated."