#!/bin/bash

###############################################################################
# Mikufy Code Editor v1.23 - 一键清理脚本
# 版本: v1.23
# 发布日期: 2026/1/23
#
# 此脚本用于清理 Mikufy 代码编辑器的编译构建产物
###############################################################################

# 设置脚本在遇到错误时继续执行（清理脚本不应因错误而中断）
set +e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 打印带颜色的消息
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 获取脚本所在目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# 构建目录和输出目录（在 mikufy 目录下）
BUILD_DIR="$SCRIPT_DIR/build"
DIST_DIR="$SCRIPT_DIR/dist"
RELEASE_DIR="$SCRIPT_DIR/mikufy-release"

# 打印欢迎信息
echo ""
echo "========================================"
echo "  Mikufy Code Editor v1.23 清理脚本"
echo "  发布日期: 2026/1/2"
echo "========================================"
echo ""

# 询问用户确认
print_warning "此操作将删除以下目录及其中的所有文件:"
echo "  - $BUILD_DIR (构建目录)"
echo "  - $DIST_DIR (安装目录)"
echo "  - $RELEASE_DIR (发布目录)"
echo ""
read -p "确定要继续吗？(y/N): " -n 1 -r
echo ""

if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    print_info "清理操作已取消"
    exit 0
fi

echo ""

# 清理构建目录
print_info "清理构建目录 ($BUILD_DIR)..."
if [ -d "$BUILD_DIR" ]; then
    rm -rf "$BUILD_DIR"
    print_success "构建目录已清理"
else
    print_warning "构建目录不存在，跳过"
fi

# 清理安装目录
print_info "清理安装目录 ($DIST_DIR)..."
if [ -d "$DIST_DIR" ]; then
    rm -rf "$DIST_DIR"
    print_success "安装目录已清理"
else
    print_warning "安装目录不存在，跳过"
fi

# 清理发布目录
print_info "清理发布目录 ($RELEASE_DIR)..."
if [ -d "$RELEASE_DIR" ]; then
    rm -rf "$RELEASE_DIR"
    print_success "发布目录已清理"
else
    print_warning "发布目录不存在，跳过"
fi

# 清理 CMake 缓存文件（如果存在）
print_info "清理 CMake 缓存文件..."
CMAKE_CACHE_FILES=(
    "$SCRIPT_DIR/CMakeCache.txt"
    "$SCRIPT_DIR/CMakeFiles"
    "$SCRIPT_DIR/cmake_install.cmake"
    "$SCRIPT_DIR/Makefile"
)

for file in "${CMAKE_CACHE_FILES[@]}"; do
    if [ -e "$file" ]; then
        rm -rf "$file"
        print_success "已删除: $file"
    fi
done

# 打印清理完成信息
echo ""
echo "========================================"
echo "  清理完成！"
echo "========================================"
echo ""
print_success "所有编译构建产物已清理完成"
echo ""
print_info "要重新编译，请运行:"
echo "  cd $SCRIPT_DIR"
echo "  ./build.sh"
echo ""
