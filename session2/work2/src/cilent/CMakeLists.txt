cmake_minimum_required(VERSION 3.8)
project(cilent)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)

# fallback: use local rm_server install under this workspace
set(_rm_server_prefix "${CMAKE_CURRENT_SOURCE_DIR}/../../rm_server/rm_server")
if(NOT rm_server_DIR)
  set(_rm_server_config "${_rm_server_prefix}/share/rm_server/cmake/rm_serverConfig.cmake")
  if(EXISTS "${_rm_server_config}")
    list(PREPEND CMAKE_PREFIX_PATH "${_rm_server_prefix}")
    message(STATUS "Use local rm_server prefix: ${_rm_server_prefix}")
  endif()
endif()

find_package(rm_server REQUIRED)

add_executable(topic_listener src/topic_listener.cpp)
ament_target_dependencies(topic_listener
  rclcpp
  std_msgs
  rm_server
)

# ensure runtime can find rm_server shared libraries even when underlay is not sourced
set(_rm_server_lib "${_rm_server_prefix}/lib")
if(_rm_server_prefix AND EXISTS "${_rm_server_lib}")
  set_target_properties(topic_listener PROPERTIES
    BUILD_RPATH "${_rm_server_lib}"
    INSTALL_RPATH "${_rm_server_lib}"
  )
endif()

install(TARGETS
  topic_listener
  DESTINATION lib/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # comment the line when a copyright and license is added to all source files
  set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # comment the line when this package is in a git repo and when
  # a copyright and license is added to all source files
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
