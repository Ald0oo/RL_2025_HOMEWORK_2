import os
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, RegisterEventHandler
from launch.conditions import IfCondition
from launch.event_handlers import OnProcessExit
from launch.substitutions import Command, FindExecutable, LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare

def generate_launch_description():
    # 1. Dichiarazione degli argomenti di avvio
    declared_arguments = [
        DeclareLaunchArgument(
            'use_sim',
            default_value='true',
            description='Avvia il robot in modalità simulazione Gazebo.',
        ),
        DeclareLaunchArgument(
            'description_package',
            default_value='iiwa_description',
            description='Pacchetto contenente le descrizioni URDF/xacro.',
        ),
        DeclareLaunchArgument(
            'description_file',
            default_value='iiwa_spawn.urdf.xacro',
            description='File xacro principale del robot da spawnare.',
        ),
        DeclareLaunchArgument(
            'runtime_config_package',
            default_value='iiwa_description',
            description='Pacchetto contenente le configurazioni dei controller.',
        ),
        DeclareLaunchArgument(
            'controllers_file',
            default_value='iiwa_controllers.yaml',
            description='File YAML di configurazione per ros2_control.',
        ),
        DeclareLaunchArgument(
            'prefix',
            default_value='iiwa_',
            description='Prefisso dei giunti (corrisponde a iiwa_spawn.urdf.xacro).',
        ),
        DeclareLaunchArgument(
            'namespace',
            default_value='iiwa',
            description='Namespace per i nodi del robot iiwa.',
        ),
        DeclareLaunchArgument(
            'robot_controller',
            default_value='iiwa_arm_controller',
            description='Controller da avviare per il mantenimento della posizione.',
        ),
        DeclareLaunchArgument(
            'initial_positions_file',
            default_value='initial_positions.yaml',
            description='File con le posizioni iniziali dei giunti.',
        ),
        DeclareLaunchArgument(
            'command_interface',
            default_value='position',
            description='Interfaccia di comando (position, velocity, effort).',
        ),
        DeclareLaunchArgument('x', default_value='2.0', description='Coordinata X di spawn'),
        DeclareLaunchArgument('y', default_value='2.0', description='Coordinata Y di spawn'),
        DeclareLaunchArgument('z', default_value='0.1', description='Coordinata Z di spawn'),
    ]

    # Inizializzazione delle configurazioni
    use_sim = LaunchConfiguration('use_sim')
    description_package = LaunchConfiguration('description_package')
    description_file = LaunchConfiguration('description_file')
    runtime_config_package = LaunchConfiguration('runtime_config_package')
    controllers_file = LaunchConfiguration('controllers_file')
    prefix = LaunchConfiguration('prefix')
    namespace = LaunchConfiguration('namespace')
    robot_controller = LaunchConfiguration('robot_controller')
    initial_positions_file = LaunchConfiguration('initial_positions_file')
    command_interface = LaunchConfiguration('command_interface')
    x = LaunchConfiguration('x')
    y = LaunchConfiguration('y')
    z = LaunchConfiguration('z')

    # Generazione della descrizione URDF/XACRO
    robot_description_content = Command([
        PathJoinSubstitution([FindExecutable(name='xacro')]),
        ' ',
        PathJoinSubstitution([FindPackageShare(description_package), 'urdf', description_file]),
        ' ',
        'prefix:=', prefix,
        ' ',
        'use_sim:=', use_sim,
        ' ',
        'command_interface:=', command_interface,
        ' ',
        'initial_positions_file:=', initial_positions_file,
        ' ',
        'description_package:=', description_package,
        ' ',
        'runtime_config_package:=', runtime_config_package,
        ' ',
        'controllers_file:=', controllers_file,
        ' ',
        'namespace:=', namespace,
    ])

    robot_description = {'robot_description': robot_description_content}

    # Percorso del file YAML per ros2_control
    robot_controllers = PathJoinSubstitution([
        FindPackageShare(runtime_config_package),
        'config',
        controllers_file,
    ])

    # Nodo Robot State Publisher (pubblica il robot_description)
    robot_state_pub_node = Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        namespace=namespace,
        output='screen',
        parameters=[robot_description, {'use_sim_time': use_sim}],
    )

    # Nodo per caricare lo spawn del modello in Gazebo Sim
    spawn_entity = Node(
        package='ros_gz_sim',
        executable='create',
        output='screen',
        arguments=[
            '-topic', [namespace, '/robot_description'],
            '-name', 'iiwa',
            '-x', x,
            '-y', y,
            '-z', z,
            '-allow_renaming', 'true',
        ],
        condition=IfCondition(use_sim),
    )

    # Spawner per joint_state_broadcaster
    joint_state_broadcaster_spawner = Node(
        package='controller_manager',
        executable='spawner',
        arguments=[
            'joint_state_broadcaster',
            '--controller-manager', [namespace, '/controller_manager'],
            '--param-file', robot_controllers,
        ],
    )

    # Spawner per iiwa_arm_controller (mantiene attiva la posizione)
    robot_controller_spawner = Node(
        package='controller_manager',
        executable='spawner',
        arguments=[
            robot_controller,
            '--controller-manager', [namespace, '/controller_manager'],
            '--param-file', robot_controllers,
        ],
    )

    # --- Sequenza d'avvio coordinata ---
    # 1. Quando lo spawn del robot in Gazebo si completa, si avvia il joint_state_broadcaster
    delay_joint_state_broadcaster_after_spawn = RegisterEventHandler(
        event_handler=OnProcessExit(
            target_action=spawn_entity,
            on_exit=[joint_state_broadcaster_spawner],
        )
    )

    # 2. Quando il joint_state_broadcaster è attivo, si avvia iiwa_arm_controller
    delay_robot_controller_after_joint_state_broadcaster = RegisterEventHandler(
        event_handler=OnProcessExit(
            target_action=joint_state_broadcaster_spawner,
            on_exit=[robot_controller_spawner],
        )
    )

    nodes = [
        robot_state_pub_node,
        spawn_entity,
        delay_joint_state_broadcaster_after_spawn,
        delay_robot_controller_after_joint_state_broadcaster,
    ]

    return LaunchDescription(declared_arguments + nodes)
