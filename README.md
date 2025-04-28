# Temoto App Launcher

Simple launcher script for the Temoto application.

## First Time Setup

Make the script executable:
```
chmod +x launch_temoto.sh
```

If you encounter backend errors, run the fix script once:
```
chmod +x fix_backend.sh
./fix_backend.sh
```

## Usage

* `./launch_temoto.sh` - Launches both webapp and ROS
* `./launch_temoto.sh --webapp` - Launches only the webapp
* `./launch_temoto.sh --nl_hri` - Launches only the ROS package
* `./launch_temoto.sh --debug` - Shows all console output for debugging
* `./launch_temoto.sh --help` - Shows usage information

## Run Action Engine
```bash
ros2 run temoto_action_engine_ros2 action_engine_node \
--actor-name David \
--actions-path temoto_demo_resources
```

## Logs

All logs are stored in the `logs/` directory.