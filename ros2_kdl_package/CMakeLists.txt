cmake_minimum_required(VERSION 3.8)
project(ros2_kdl_package)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Dipendenze
find_package(ament_cmake REQUIRED)
find_package(orocos_kdl REQUIRED)
find_package(urdf REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(kdl_parser REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_action REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(rosidl_default_generators REQUIRED)
find_package(std_msgs REQUIRED)
find_package(action_msgs REQUIRED)

# 1. Generazione delle interfacce (aggiungi LinearTraj.action)
rosidl_generate_interfaces(${PROJECT_NAME}
  "action/LinearTraj.action" # Inserito se l'azione esiste nella cartella action/
  DEPENDENCIES geometry_msgs std_msgs sensor_msgs
)

ament_export_dependencies(rosidl_default_runtime)

include_directories(${CMAKE_CURRENT_BINARY_DIR}/rosidl_generator_cpp)

# --- Eseguibile: ros2_kdl_node ---
add_executable(ros2_kdl_node 
  src/ros2_kdl_node.cpp 
  src/kdl_robot.cpp 
  src/kdl_planner.cpp 
  src/kdl_control.cpp
)
target_include_directories(ros2_kdl_node PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
  ${EIGEN3_INCLUDE_DIRS})
target_compile_features(ros2_kdl_node PUBLIC c_std_99 cxx_std_17)
ament_target_dependencies(ros2_kdl_node
  orocos_kdl
  rclcpp
  kdl_parser
  sensor_msgs
  std_msgs
  rclcpp_action
  action_msgs
)
rosidl_target_interfaces(ros2_kdl_node ${PROJECT_NAME} "rosidl_typesupport_cpp")

# --- Eseguibile: trajectory_client ---
add_executable(trajectory_client src/linear_traj_client.cpp)
ament_target_dependencies(trajectory_client 
  rclcpp 
  rclcpp_action
  geometry_msgs
  action_msgs
)
rosidl_target_interfaces(trajectory_client ${PROJECT_NAME} "rosidl_typesupport_cpp")

# --- Eseguibile: action_client_node ---
add_executable(action_client_node src/action_client_node.cpp)
ament_target_dependencies(action_client_node
  rclcpp
  rclcpp_action
  geometry_msgs
  action_msgs
)
rosidl_target_interfaces(action_client_node ${PROJECT_NAME} "rosidl_typesupport_cpp")

# --- Eseguibile: ros2_kdl_node_example ---
add_executable(ros2_kdl_node_example src/ros2_kdl_node_example.cpp)
target_include_directories(ros2_kdl_node_example PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
  ${EIGEN3_INCLUDE_DIRS})
target_compile_features(ros2_kdl_node_example PUBLIC c_std_99 cxx_std_17)
ament_target_dependencies(ros2_kdl_node_example
  orocos_kdl
  rclcpp
)

# Installazione dei target
install(TARGETS
  ros2_kdl_node
  trajectory_client
  action_client_node
  ros2_kdl_node_example
  DESTINATION lib/${PROJECT_NAME}
)

install(
  DIRECTORY 
  launch
  config
  DESTINATION share/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
