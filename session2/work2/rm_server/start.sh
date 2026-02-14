#!/bin/bash

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)

print_info() { echo -e "\033[34mℹ️  $1\033[0m"; }
print_success() { echo -e "\033[32m✅ $1\033[0m"; }
print_warning() { echo -e "\033[33m⚠️  $1\033[0m"; }
print_error() { echo -e "\033[31m❌ $1\033[0m"; }

print_info "🔧 正在加载环境配置（source setup.bash）..."
if [ -f "${SCRIPT_DIR}/setup.bash" ]; then
    source "${SCRIPT_DIR}/setup.bash"
else
    print_error "❌ 错误：找不到 setup.bash 文件（路径：${SCRIPT_DIR}/setup.bash）"
    exit 1
fi

TARGET_DIR="${SCRIPT_DIR}/rm_server/lib"
print_info "📂 切换到程序目录：${TARGET_DIR}"
if [ -d "${TARGET_DIR}" ]; then
    cd "${TARGET_DIR}" || {
        print_error "❌ 错误：无法切换到目录 ${TARGET_DIR}"
        exit 1
    }
else
    print_error "❌ 错误：目标目录不存在 ${TARGET_DIR}"
    exit 1
fi

PROGRAM="./rm_server_node"

print_info "🚀 启动 rm_server_node 程序..."
if [ ! -x "${PROGRAM}" ]; then
    print_warning "警告：rm_server_node 文件存在，但无执行权限"

    print_info "当前文件权限：$(ls -l "${PROGRAM}" | awk '{print $1}')"
  
    print_info "尝试自动添加执行权限..."
    if chmod +x "${PROGRAM}"; then
        print_success "成功添加执行权限！"

        if [ -x "${PROGRAM}" ]; then
            print_info "重新启动 rm_server_node 程序..."
            ./rm_server_node
        else
            print_error "错误：添加权限后仍无执行权限，请手动检查文件属性"
            exit 1
        fi
    else
        print_error "错误：无法添加执行权限（可能是用户权限不足）"
        print_info "请手动执行以下命令添加权限：chmod +x ${TARGET_DIR}/rm_server_node"
        exit 1
    fi
else

    print_success "程序权限检查通过，开始运行..."
    ./rm_server_node
fi
