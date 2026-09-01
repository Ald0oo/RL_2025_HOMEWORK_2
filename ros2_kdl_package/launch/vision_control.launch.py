import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription, RegisterEventHandler, SetEnvironmentVariable
from launch.event_handlers import OnProcessExit 
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import Command, FindExecutable, PathJoinSubstitution 
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare

def generate_launch_description():

    pkg_ros_gz_sim = get_package_share_directory('ros_gz_sim')
    pkg_iiwa_description = get_package_share_directory('iiwa_description')
    pkg_ros2_kdl_package = get_package_share_directory('ros2_kdl_package') 

    my_initial_positions_file = os.path.join(
        pkg_ros2_kdl_package, 
        'config', 
        'my_initial_positions.yaml'
    )

    model_path = os.path.join(pkg_iiwa_description, 'gazebo', 'models')
    if 'GAZEBO_MODEL_PATH' in os.environ:
        gz_model_path = os.environ['GAZEBO_MODEL_PATH'] + ':' + model_path
    else:
        gz_model_path = model_path

    set_model_path = SetEnvironmentVariable(
        name='GAZEBO_MODEL_PATH',
        value=gz_model_path
    )

    aruco_model_sdf = os.path.join(pkg_iiwa_description, 'gazebo', 'worlds', 'aruco_world.world') 

    gazebo_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(pkg_ros_gz_sim, 'launch', 'gz_sim.launch.py')
        ),
        launch_arguments={'gz_args': '-r ' + aruco_model_sdf, 'publish_clock': 'true'}.items(),
    )

    xacro_file_name = 'iiwa.config.xacro'
    xacro = os.path.join(get_package_share_directory('iiwa_description'), "config", xacro_file_name)
    
    robot_description_param = {"robot_description": Command([
        'xacro ', xacro, 
        ' command_interface:=velocity',
        ' use_sim:=true',
        ' initial_positions_file:=', my_initial_positions_file 
    ])}

    load_robot_description_node = Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        output='screen',
        parameters=[robot_description_param, {"use_sim_time": True}],
    )

    spawn_entity_node = Node(
        package='ros_gz_sim',
        executable='create',
        output='screen',
        arguments=['-topic', '/robot_description', '-name', 'iiwa'],
    )

    clock_bridge = Node(
         package="ros_gz_bridge",
         executable="parameter_bridge",
         arguments=['/clock@rosgraph_msgs/msg/Clock[gz.msgs.Clock'],
         output="screen",
    )
    
    camera_bridge = Node(
        package='ros_gz_bridge',
        executable='parameter_bridge',
        arguments=[
            '/camera@sensor_msgs/msg/Image@gz.msgs.Image',
            '/camera_info@sensor_msgs/msg/CameraInfo@gz.msgs.CameraInfo',
            '--ros-args', 
            '-r', '/camera:=/stereo/left/image_rect_color',
            '-r', '/camera_info:=/stereo/left/camera_info'
        ],
        output='screen',
    )

    set_pose_bridge = Node(
        package='ros_gz_bridge',
        executable='parameter_bridge',
        name='gz_bridge',
        arguments=[
            '/world/default/set_pose@ros_gz_interfaces/srv/SetEntityPose',
        ],
        output='screen'
    )
    
    joint_state_broadcaster_node = Node(
        package="controller_manager",
        executable="spawner",
        arguments=["joint_state_broadcaster", "--controller-manager", "/controller_manager"],
    )

    robot_controller_spawner = Node(
        package="controller_manager",
        executable="spawner",
        arguments=["velocity_controller", "--controller-manager", "/controller_manager"],
    )
    
    spawn_jsb_handler = RegisterEventHandler(
        event_handler=OnProcessExit(
            target_action=spawn_entity_node,
            on_exit=[joint_state_broadcaster_node],
        )
    )

    spawn_controller_handler = RegisterEventHandler(
        event_handler=OnProcessExit(
            target_action=joint_state_broadcaster_node,
            on_exit=[robot_controller_spawner],
        )
    )

    kdl_control_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(pkg_ros2_kdl_package, 'launch', 'kdl_run.launch.py')
        ),
        launch_arguments={
            'cmd_interface': 'velocity',
            'ctrl': 'vision'
        }.items()
    )

    return LaunchDescription([
        set_model_path,
        gazebo_launch,
        load_robot_description_node,
        spawn_entity_node,
        clock_bridge,
        camera_bridge,
        spawn_jsb_handler,
        spawn_controller_handler,
        kdl_control_launch,
        set_pose_bridge 
    ])
