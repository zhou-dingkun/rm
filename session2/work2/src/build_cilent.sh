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

if command -v colcon >/dev/null 2>&1; then
  cd "${WS_DIR}"
  colcon build --packages-select "${PKG_NAME}"
  echo "构建完成: source ${WS_DIR}/install/setup.bash"
else
  echo "未检测到 colcon，使用 CMake 直接构建"
  mkdir -p "${WS_DIR}/build_${PKG_NAME}"
  cd "${WS_DIR}/build_${PKG_NAME}"
  cmake "${WS_DIR}/${PKG_NAME}"
  cmake --build . -j"$(nproc)"
  echo "构建完成: ${WS_DIR}/build_${PKG_NAME}/topic_listener"
fi
