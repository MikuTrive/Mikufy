#!/bin/bash

# 获取脚本所在目录的绝对路径
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$SCRIPT_DIR"
PARENT_DIR="$(dirname "$PROJECT_ROOT")"

echo "脚本目录: $SCRIPT_DIR"
echo "上级目录: $PARENT_DIR"

# 检查文件是否存在
if [ ! -f "$PROJECT_ROOT/mikufy-release/mikufy" ]; then
    echo "错误: 找不到可执行文件 $PROJECT_ROOT/mikufy-release/mikufy"
    exit 1
fi

# 图标路径
ICON_PATH="$PARENT_DIR/Mikufy.png"

if [ ! -f "$ICON_PATH" ]; then
    echo "警告: 找不到图标文件 $ICON_PATH"
    ICON_PATH=""
else
    echo "找到图标: $ICON_PATH"
fi

# 1. 创建可执行文件的符号链接
echo "创建可执行文件链接..."
sudo ln -sf "$PROJECT_ROOT/mikufy-release/mikufy" /usr/local/bin/mikufy

# 2. 创建桌面文件（直接使用原始图标路径）
echo "创建桌面快捷方式..."
DESKTOP_FILE="$HOME/Desktop/Mikufy.desktop"

cat > "$DESKTOP_FILE" << EOF
[Desktop Entry]
Categories=CodeEditor;
Comment=Local Mikufy
Exec=/usr/local/bin/mikufy
Icon=$ICON_PATH
Name=Mikufy
StartupNotify=true
Terminal=false
Type=Application
EOF

chmod +x "$DESKTOP_FILE"

# 3. 验证
echo ""
echo "安装完成！"
echo "程序链接: /usr/local/bin/mikufy -> $PROJECT_ROOT/mikufy-release/mikufy"
echo "桌面文件: $DESKTOP_FILE"
echo "图标路径: ${ICON_PATH:-未设置}"
