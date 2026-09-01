# RL_2025_HOMEWORK_2
Control your robot

## Available Packages in this Repository
* `aruco_ros`
* `ros2_iiwa`
* `ros2_kdl_package`
  
## Getting Started
``` bash
git clone https://github.com/Ald0oo/RL_HOMEWORK_2.git
colcon build 
source install/setup.bash
```
## Usage
1. Kinematic control
To start the iiwa robot with velocity interface run the command:
``` bash
ros2 launch iiwa_bringup iiwa.launch.py command_interface:="velocity" robot_controller:="velocity_controller"
```
Then, in another terminal, start the action server:
``` bash
ros2 launch ros2_kdl_package kdl_run.launch.py cmd_interface:="velocity" ctrl:=<type>
```
Where type can be velocity_ctrl and velocity_ctrl_null. To default is velocity_ctrl.

And, in another terminal, run the action client with a goal:
``` bash
ros2 run ros2_kdl_package action_client_node x y z
```
## 2. Vision-based control
To start the iiwa robot with a vision control in a gazebo world with an aruco marker run:
``` bash
ros2 launch ros2_kdl_package vision_control.launch.py
```
In another terminal visualize the camera:
``` bash
ros2 run rqt_image_view rqt_image_view
```
To detect the aruco, in another terminal, start the aruco_ros package:
``` bash
ros2 launch aruco_ros single.launch.py marker_size:=0.1 marker_id:=23
```
Now you can move the aruco directly in gazebo, or via terminal with this command:
``` bash
ros2 service call /world/default/set_pose ros_gz_interfaces/srv/SetEntityPose "{
  entity: {
    name: 'aruco_marker',  
    type: 1           
  },
  pose: {
    position: {x: 0.5, y: -0.8, z: 1.0},
    orientation: {x: 0.0, y: 0.0, z: 0.0, w: 1.0}
  }
}"
```
