#!/bin/bash

###############################################################################
# Mikufy Code Editor v1.23 - 一键编译脚本
# 版本: v1.23
# 发布日期: 2026/1/23
#
# 此脚本用于编译 Mikufy 代码编辑器
# 使用 CMake 构建系统，生成单个可执行二进制文件
###############################################################################

# 设置脚本在遇到错误时立即退出
set -e

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

# 构建目录（在 mikufy 目录下）
BUILD_DIR="$SCRIPT_DIR/build"
INSTALL_DIR="$SCRIPT_DIR/dist"

# 打印欢迎信息
echo ""
echo "========================================"
echo "  Mikufy Code Editor v1.23 编译脚本"
echo "  发布日期: 2026/1/23"
echo "========================================"
echo ""

# 检查必要的依赖
print_info "检查编译依赖..."

# 检查 CMake
if ! command -v cmake &> /dev/null; then
    print_error "未找到 CMake，请先安装 CMake"
    print_info "安装命令: sudo pacman -S cmake (Arch Linux)"
    exit 1
fi

# 检查 GCC 或 Clang
if ! command -v g++ &> /dev/null && ! command -v clang++ &> /dev/null; then
    print_error "未找到 C++ 编译器 (g++ 或 clang++)"
    print_info "安装命令: sudo pacman -S base-devel (Arch Linux)"
    exit 1
fi

# 检查 pkg-config
if ! command -v pkg-config &> /dev/null; then
    print_error "未找到 pkg-config"
    print_info "安装命令: sudo pacman -S pkg-config (Arch Linux)"
    exit 1
fi

# 检查 GTK3
if ! pkg-config --exists gtk+-3.0; then
    print_error "未找到 GTK3 开发库"
    print_info "安装命令: sudo pacman -S gtk3 (Arch Linux)"
    exit 1
fi

# 检查 WebKitGTK
if ! pkg-config --exists webkit2gtk-4.0; then
    print_error "未找到 WebKitGTK 开发库"
    print_info "安装命令: sudo pacman -S webkit2gtk (Arch Linux)"
    exit 1
fi

print_success "所有依赖已就绪"

# 清理旧的构建目录
print_info "清理旧的构建文件..."
if [ -d "$BUILD_DIR" ]; then
    rm -rf "$BUILD_DIR"
fi

# 创建构建目录
print_info "创建构建目录..."
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# 运行 CMake 配置
print_info "运行 CMake 配置..."
cmake ../src \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR"

# 编译项目
print_info "开始编译项目..."
make -j$(nproc)

print_success "编译完成！"

# 创建可分发的打包结构
print_info "创建可分发的打包结构..."
PACKAGE_DIR="$SCRIPT_DIR/mikufy-release"
mkdir -p "$PACKAGE_DIR"

# 复制可执行文件
cp "$BUILD_DIR/bin/mikufy" "$PACKAGE_DIR/"
chmod +x "$PACKAGE_DIR/mikufy"

# 复制 UI 文件
cp -r "$SCRIPT_DIR/ui" "$PACKAGE_DIR/"

# 创建启动脚本
cat > "$PACKAGE_DIR/run.sh" << 'EOF'
#!/bin/bash

# Mikufy Code Editor v1.0 - 启动脚本
# 此脚本确保 mikufy 从正确的目录启动

# 获取脚本所在目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# 切换到脚本目录
cd "$SCRIPT_DIR"

# 启动 mikufy
./mikufy
EOF

chmod +x "$PACKAGE_DIR/run.sh"

# 创建 README
cat > "$PACKAGE_DIR/README.md" << 'EOF'
# Mikufy Code Editor v1.0

## 简介

Mikufy 是一款类似 VSCode 的代码编辑器，专为 Linux 平台设计。

## 版本信息

- 版本: v1.0
- 发布日期: 2026/1/22

## 系统要求

- Linux 系统（支持 X11 或 Wayland）
- GTK3
- WebKitGTK

## 安装依赖

### Arch Linux / Manjaro

```bash
sudo pacman -S gtk3 webkit2gtk
```

### Ubuntu / Debian

```bash
sudo apt-get install libgtk-3-dev libwebkit2gtk-4.0-dev
```

### Fedora

```bash
sudo dnf install gtk3-devel webkit2gtk3-devel
```

## 运行

### 方法 1: 使用启动脚本（推荐）

```bash
./run.sh
```

### 方法 2: 直接运行

```bash
./mikufy
```

## 功能特性

- 打开工作区文件夹
- 文件树浏览（支持隐藏文件）
- 创建文件和文件夹
- 重命名和删除文件/文件夹
- 代码编辑
- 自动保存

## 键盘快捷键

- `Ctrl+S`: 保存文件
- `Ctrl+N`: 新建文件
- `F2`: 重命名选中的文件/文件夹
- `Delete`: 删除选中的文件/文件夹
- `Enter`: 打开选中的文件

## 项目结构

```
mikufy-release/
├── mikufy          # 可执行文件
├── run.sh          # 启动脚本
├── ui/             # UI 资源文件
│   ├── index.html
│   ├── style.css
│   ├── app.js
│   ├── Background.png
│   ├── Mikufy.png
│   └── Icons/      # 图标文件
└── README.md       # 本文件
```

## 开发

Mikufy 使用以下技术栈开发：

- **前端**: HTML, CSS, JavaScript, TypeScript
- **后端**: C++17
- **GUI 框架**: GTK3
- **Web 引擎**: WebKitGTK

## 许可证

MIT License

## 联系方式

如有问题或建议，请联系开发团队。
EOF

# 打印编译结果摘要
echo ""
echo "========================================"
echo "  编译完成！"
echo "========================================"
echo ""
print_success "可执行文件: $PACKAGE_DIR/mikufy"
print_success "启动脚本: $PACKAGE_DIR/run.sh"
print_success "UI 文件: $PACKAGE_DIR/ui/"
print_success "README: $PACKAGE_DIR/README.md"
echo ""
print_info "要运行 Mikufy，请执行:"
echo "  cd $PACKAGE_DIR"
echo "  ./run.sh"
echo ""
print_info "或者直接运行:"
echo "  $PACKAGE_DIR/mikufy"
echo ""
print_success "编译成功！"
echo ""
