#!/usr/bin/env bash
set -e

WS_DIR="$(cd "$(dirname "$0")" && pwd)"
PKG_NAME="cilent"

# 避免混用 rolling/humble
for _v in AMENT_PREFIX_PATH CMAKE_PREFIX_PATH LD_LIBRARY_PATH PYTHONPATH; do
  _old="${!_v:-}"
  if [ -n "$_old" ]; then
    _new="$(echo "$_old" | tr ':' '\n' | grep -v '/opt/ros/rolling' | paste -sd ':' -)"
    export "${_v}=${_new}"
  fi
done

source /opt/ros/humble/setup.bash
source "${WS_DIR}/../rm_server/setup.bash"
export RMW_IMPLEMENTATION=rmw_fastrtps_cpp

if [ -f "${WS_DIR}/install/setup.bash" ]; then
  source "${WS_DIR}/install/setup.bash"
  exec ros2 run "${PKG_NAME}" topic_listener
else
  exec "${WS_DIR}/build_${PKG_NAME}/topic_listener"
fi
