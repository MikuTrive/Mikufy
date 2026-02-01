#
# Mikufy v2.2(stable) - 一键桌面程序安装脚本
# 用于桌面程序快速启动
#

PROJECT_NAME="Mikufy"
VERSION="2.2(stable)"

echo ""
echo "================================================"
echo "  ${PROJECT_NAME} v${VERSION} - 桌面程序安装脚本"
echo "================================================"
echo ""

set -e

APP_NAME="Mikufy"
INSTALL_DIR="$HOME/.local/share/MikufyUI"
PNG_SRC="./web/Mikufy.png"
BIN_SRC="./mikufy"

PNG_DST="$INSTALL_DIR/Mikufy.png"
BIN_DST="$INSTALL_DIR/mikufy"

mkdir -p "$INSTALL_DIR"

if [[ ! -f "$PNG_SRC" ]]; then
    echo "❌ 未找到 $PNG_SRC"
    exit 1
fi

if [[ ! -f "$BIN_SRC" ]]; then
    echo "❌ 未找到 $BIN_SRC"
    exit 1
fi

cp "$PNG_SRC" "$PNG_DST"
cp "$BIN_SRC" "$BIN_DST"

chmod +x "$BIN_DST"

if [[ -d "$HOME/桌面" ]]; then
    DESKTOP_DIR="$HOME/桌面"
elif [[ -d "$HOME/Desktop" ]]; then
    DESKTOP_DIR="$HOME/Desktop"
else
    echo "❌ 未找到 桌面 或 Desktop 目录"
    exit 1
fi

DESKTOP_FILE="$DESKTOP_DIR/Mikufy.desktop"

cat > "$DESKTOP_FILE" <<EOF
[Desktop Entry]
Version=1.0
Type=Application
Name=Mikufy
Comment=Mikufy UI
Exec=$BIN_DST
Icon=$PNG_DST
Terminal=false
Categories=AudioVideo;Utility;
EOF

chmod +x "$DESKTOP_FILE"

echo " 安装完成"
echo " 程序位置: $BIN_DST"
echo " 图标位置: $PNG_DST"
echo " 桌面文件: $DESKTOP_FILE"
