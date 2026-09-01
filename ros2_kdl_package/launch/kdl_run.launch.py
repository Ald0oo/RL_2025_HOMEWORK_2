import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration

def generate_launch_description():

    pkg_dir = get_package_share_directory('ros2_kdl_package') 
    config_file = os.path.join(pkg_dir, 'config', 'kdl_params.yaml')

    ctrl_arg = DeclareLaunchArgument(
        'ctrl',
        default_value='velocity_ctrl', 
        description='Type of velocity controller to use (velocity_ctrl | velocity_ctrl_null)'
    )

    ctrl_param = LaunchConfiguration('ctrl')

    cmd_interface_arg = DeclareLaunchArgument(
        'cmd_interface',
        default_value='position', 
        description='Type of interface (position | velocity | effort)'
    )

    cmd_interface_param = LaunchConfiguration('cmd_interface')

    kdl_node = Node(
        package='ros2_kdl_package',      
        executable='ros2_kdl_node',    
        name='ros2_kdl_node',          
        output='screen',
        parameters=[
            config_file,
            {'ctrl': ctrl_param},
            {'cmd_interface': cmd_interface_param}
            ]       
    )

    return LaunchDescription([
        ctrl_arg,
        cmd_interface_arg,
        kdl_node
    ])
