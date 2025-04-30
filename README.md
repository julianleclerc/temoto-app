# Temoto App Launcher

A simple launcher script for the Temoto application.

## Install

### Get the dependencies
```bash
sudo apt install libboost-all-dev
```

### Download this repository
```bash
git clone --recursive https://github.com/julianleclerc/temoto-app
```

## First Time Setup

### Move to correct git rep
- **temoto_action_assistant**: `julian-devel` - Latest Chat HRI page setup for web application
- **temoto_demo_ressources**: `julian-dev` - Template actions, graphs and data used for demo (place needed actions inside demo/src)
- **temoto_action_engine_ros2**: `origin/main` - Temoto Action Engine ROS2 Package
- **temoto_nl_hri**: `origin/main` - ROS2 package for enabling Natural Language interaction for Temoto Action Engine

### Build ROS Workspace
```bash
cd temoto_ros_ws
source /opt/ros/<YOUR-ROS2-DISTRO>/setup.bash
colcon build
source install/setup.bash
```

### Build Actions
```bash
cd demo
colcon build
source install/setup.bash
```

## Usage

From the temoto-app repository root:

| Command | Description |
|---------|-------------|
| `./launch_temoto.sh` | Launches both webapp and ROS |
| `./launch_temoto.sh --webapp` | Launches only the webapp |
| `./launch_temoto.sh --nl_hri` | Launches only the ROS package |
| `./launch_temoto.sh --debug` | Shows all console output for debugging |
| `./launch_temoto.sh --help` | Shows usage information |

## Customization

### Modifying Map and Data
- **Items Database**: `demo/src/data/items.json` - List of objects and objects within map
- **Map Image**: `demo/src/data/map.pgm` - Map of the environment
- **Map Properties**: `demo/src/data/map.yaml` - Properties of the map

### Adding Custom Content
- **Actions**: Add actions to `demo/src/actions/` 
- **Graphs**: Add graphs to `demo/src/graphs/`

## Running Components

### Action Engine
```bash
ros2 run temoto_action_engine_ros2 action_engine_node \
  --actor-name David \
  --actions-path demo/src
```

### Initialize Display Panel
Publish the first inspection image for topic discovery in the display panel:
```bash
ros2 topic pub -1 /webapp/display_feed/David/inspection sensor_msgs/msg/CompressedImage "{header: {stamp: {sec: 0, nanosec: 0}, frame_id: ''}, format: 'jpeg', data: []}"
```


## Logs

All application logs are stored in the `logs/` directory.
