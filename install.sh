#
# Mikufy v2.11-nova - 安装脚本
# 适用于所有Linux发行版用户的系统
# 支持系统级安装（需要sudo）和用户级安装（无需sudo）
#

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 项目信息
PROJECT_NAME="Mikufy"
VERSION="2.11-nova"
APP_NAME="mikufy"
EXECUTABLE="${APP_NAME}"
WEB_DIR="web"
DESKTOP_FILE="mikufy.desktop"
ICON_FILE="web/Mikufy.png"

# 安装模式
INSTALL_MODE="user"  # "user" 或 "system"

# 打印函数
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

# 打印标题
print_header() {
    echo ""
    echo "========================================"
    echo "  ${PROJECT_NAME} v${VERSION} - 安装脚本"
    echo "========================================"
    echo ""
}

# 显示帮助信息
show_help() {
    echo "用法: $0 [选项]"
    echo ""
    echo "选项:"
    echo "  -h, --help          显示帮助信息"
    echo "  -u, --user          用户级安装（安装到 ~/.local/，无需sudo）"
    echo "  -s, --system        系统级安装（安装到 /usr/local/，需要sudo）"
    echo "  --uninstall         卸载已安装的mikufy"
    echo ""
    echo "示例:"
    echo "  $0                  用户级安装（默认）"
    echo "  $0 -u               用户级安装"
    echo "  $0 -s               系统级安装"
    echo "  $0 --uninstall      卸载用户级安装"
    echo "  sudo $0 --uninstall 卸载系统级安装"
    echo ""
}

# 检查必要的文件是否存在
check_files() {
    print_info "检查必要的文件..."

    if [ ! -f "${EXECUTABLE}" ]; then
        print_error "未找到可执行文件: ${EXECUTABLE}"
        print_info "请先运行 ./build.sh 编译项目"
        exit 1
    fi

    if [ ! -f "terminal_helper" ]; then
        print_error "未找到可执行文件: terminal_helper"
        print_info "请先运行 ./build.sh 编译项目"
        exit 1
    fi

    if [ ! -d "${WEB_DIR}" ]; then
        print_error "未找到web目录: ${WEB_DIR}"
        exit 1
    fi

    if [ ! -f "${DESKTOP_FILE}" ]; then
        print_error "未找到桌面文件: ${DESKTOP_FILE}"
        exit 1
    fi

    if [ ! -f "${ICON_FILE}" ]; then
        print_error "未找到图标文件: ${ICON_FILE}"
        exit 1
    fi

    print_success "所有必要文件检查通过"
}

# 用户级安装
install_user() {
    print_info "开始用户级安装..."

    # 定义安装路径（使用大写 MIKUFY）
    INSTALL_DIR="${HOME}/.local"
    BIN_DIR="${INSTALL_DIR}/bin"
    APP_DIR="${INSTALL_DIR}/share/MIKUFY"
    ICONS_DIR="${INSTALL_DIR}/share/icons/hicolor/256x256/apps"

    # 创建目录
    print_info "创建必要的目录..."
    mkdir -p "${BIN_DIR}"
    mkdir -p "${APP_DIR}"
    mkdir -p "${ICONS_DIR}"

    # 将可执行文件和web目录都放在APP_DIR下
    print_info "安装可执行文件和web目录到 ${APP_DIR}..."
    cp "${EXECUTABLE}" "${APP_DIR}/"
    chmod +x "${APP_DIR}/${EXECUTABLE}"
    cp "terminal_helper" "${APP_DIR}/"
    chmod +x "${APP_DIR}/terminal_helper"
    mkdir -p "${APP_DIR}/web"
    cp -r "${WEB_DIR}"/* "${APP_DIR}/web/"
    chmod -R 755 "${APP_DIR}/web"
    /* 确保 style.css 有写权限 */
    chmod 644 "${APP_DIR}/web/style.css"
    print_success "可执行文件和web目录已安装（包含 Background 和 Icons）"

    # 创建启动包装脚本（直接执行可执行文件，不需要cd）
    print_info "创建启动脚本到 ${BIN_DIR}..."
    cat > "${BIN_DIR}/${APP_NAME}" << EOF
#!/bin/bash
exec "${APP_DIR}/${EXECUTABLE}" "\$@"
EOF
    chmod +x "${BIN_DIR}/${APP_NAME}"
    print_success "启动脚本已创建"

    # 创建 terminal_helper 符号链接
    print_info "创建 terminal_helper 符号链接到 ${BIN_DIR}..."
    ln -sf ../share/MIKUFY/terminal_helper "${BIN_DIR}/terminal_helper"
    print_success "terminal_helper 符号链接已创建"

    # 复制图标文件
    print_info "安装应用图标..."
    cp "${ICON_FILE}" "${ICONS_DIR}/${APP_NAME}.png"
    chmod 644 "${ICONS_DIR}/${APP_NAME}.png"
    print_success "应用图标已安装"

    # 复制desktop文件到桌面
    DESKTOP_DIR=""
    if [ -d "${HOME}/Desktop" ]; then
        DESKTOP_DIR="${HOME}/Desktop"
    elif [ -d "${HOME}/桌面" ]; then
        DESKTOP_DIR="${HOME}/桌面"
    fi

    if [ -n "${DESKTOP_DIR}" ]; then
        print_info "创建桌面快捷方式到 ${DESKTOP_DIR}..."
        cat "${DESKTOP_FILE}" | sed "s|Exec=${APP_NAME}|Exec=${BIN_DIR}/${APP_NAME}|g" | sed "s|Icon=${APP_NAME}|Icon=${ICONS_DIR}/${APP_NAME}.png|g" > "${DESKTOP_DIR}/${DESKTOP_FILE}"
        chmod +x "${DESKTOP_DIR}/${DESKTOP_FILE}"
        print_success "桌面快捷方式已创建"
    else
        print_warning "未找到桌面目录（Desktop 或 桌面），跳过创建桌面快捷方式"
    fi

    # 更新图标缓存
    if command -v gtk-update-icon-cache &> /dev/null; then
        gtk-update-icon-cache -f -t "${INSTALL_DIR}/share/icons/hicolor" 2>/dev/null || true
    fi

    print_success "用户级安装完成！"
    echo ""
    print_info "安装路径:"
    echo "  启动脚本:   ${BIN_DIR}/${APP_NAME}"
    echo "  应用目录:   ${APP_DIR}"
    echo "  可执行文件: ${APP_DIR}/${EXECUTABLE}"
    echo "  Web资源:    ${APP_DIR}/web"
    echo "    - Background: ${APP_DIR}/web/Background/"
    echo "    - Icons: ${APP_DIR}/web/Icons/"
    echo "  应用图标:   ${ICONS_DIR}/${APP_NAME}.png"
    echo ""
    print_info "使用方法:"
    echo "  1. 确保 ${BIN_DIR} 在 PATH 中"
    echo "  2. 运行: ${APP_NAME}"
    echo "  3. 或双击桌面上的快捷方式"
    echo ""
    print_warning "如果 ${BIN_DIR} 不在 PATH 中，请将其添加到 ~/.bashrc 或 ~/.zshrc:"
    echo "  export PATH=\"\${PATH}:${BIN_DIR}\""
}

# 系统级安装
install_system() {
    print_info "开始系统级安装..."

    # 检查是否有sudo权限
    if [ "$EUID" -ne 0 ]; then
        print_error "系统级安装需要root权限，请使用: sudo $0 -s"
        exit 1
    fi

    # 定义安装路径
    INSTALL_DIR="/usr/local"
    BIN_DIR="${INSTALL_DIR}/bin"
    APP_DIR="${INSTALL_DIR}/share/${APP_NAME}"
    ICONS_DIR="${INSTALL_DIR}/share/icons/hicolor/256x256/apps"

    # 创建目录
    print_info "创建必要的目录..."
    mkdir -p "${BIN_DIR}"
    mkdir -p "${APP_DIR}"
    mkdir -p "${ICONS_DIR}"

    # 将可执行文件和web目录都放在APP_DIR下
    print_info "安装可执行文件和web目录到 ${APP_DIR}..."
    cp "${EXECUTABLE}" "${APP_DIR}/"
    chmod +x "${APP_DIR}/${EXECUTABLE}"
    cp "terminal_helper" "${APP_DIR}/"
    chmod +x "${APP_DIR}/terminal_helper"
    mkdir -p "${APP_DIR}/web"
    cp -r "${WEB_DIR}"/* "${APP_DIR}/web/"
    chmod -R 755 "${APP_DIR}/web"
    /* 确保 style.css 有写权限 */
    chmod 644 "${APP_DIR}/web/style.css"
    print_success "可执行文件和web目录已安装（包含 Background 和 Icons）"

    # 创建启动包装脚本（直接执行可执行文件，不需要cd）
    print_info "创建启动脚本到 ${BIN_DIR}..."
    cat > "${BIN_DIR}/${APP_NAME}" << EOF
#!/bin/bash
exec "${APP_DIR}/${EXECUTABLE}" "\$@"
EOF
    chmod +x "${BIN_DIR}/${APP_NAME}"
    print_success "启动脚本已创建"

    # 创建 terminal_helper 符号链接
    print_info "创建 terminal_helper 符号链接到 ${BIN_DIR}..."
    ln -sf ../share/mikufy/terminal_helper "${BIN_DIR}/terminal_helper"
    print_success "terminal_helper 符号链接已创建"

    # 复制图标文件
    print_info "安装应用图标..."
    cp "${ICON_FILE}" "${ICONS_DIR}/${APP_NAME}.png"
    chmod 644 "${ICONS_DIR}/${APP_NAME}.png"
    print_success "应用图标已安装"

    # 复制desktop文件到桌面（检查 sudo 用户的桌面目录）
    if [ -n "${SUDO_USER}" ]; then
        SUDO_HOME="/home/${SUDO_USER}"
        DESKTOP_DIR=""
        if [ -d "${SUDO_HOME}/Desktop" ]; then
            DESKTOP_DIR="${SUDO_HOME}/Desktop"
        elif [ -d "${SUDO_HOME}/桌面" ]; then
            DESKTOP_DIR="${SUDO_HOME}/桌面"
        fi

        if [ -n "${DESKTOP_DIR}" ]; then
            print_info "创建桌面快捷方式到 ${DESKTOP_DIR}..."
            cat "${DESKTOP_FILE}" | sed "s|Exec=${APP_NAME}|Exec=${BIN_DIR}/${APP_NAME}|g" | sed "s|Icon=${APP_NAME}|Icon=${ICONS_DIR}/${APP_NAME}.png|g" > "${DESKTOP_DIR}/${DESKTOP_FILE}"
            chown "${SUDO_USER}:${SUDO_USER}" "${DESKTOP_DIR}/${DESKTOP_FILE}"
            chmod +x "${DESKTOP_DIR}/${DESKTOP_FILE}"
            print_success "桌面快捷方式已创建"
        else
            print_warning "未找到桌面目录（Desktop 或 桌面），跳过创建桌面快捷方式"
        fi
    fi

    # 更新图标缓存
    if command -v gtk-update-icon-cache &> /dev/null; then
        gtk-update-icon-cache -f -t "${INSTALL_DIR}/share/icons/hicolor" 2>/dev/null || true
    fi

    print_success "系统级安装完成！"
    echo ""
    print_info "安装路径:"
    echo "  启动脚本:   ${BIN_DIR}/${APP_NAME}"
    echo "  应用目录:   ${APP_DIR}"
    echo "  可执行文件: ${APP_DIR}/${EXECUTABLE}"
    echo "  Web资源:    ${APP_DIR}/web"
    echo "    - Background: ${APP_DIR}/web/Background/"
    echo "    - Icons: ${APP_DIR}/web/Icons/"
    echo "  应用图标:   ${ICONS_DIR}/${APP_NAME}.png"
    echo ""
    print_info "使用方法:"
    echo "  1. 直接运行: ${APP_NAME}"
    echo "  2. 或双击桌面上的快捷方式"
}

# 卸载用户级安装
uninstall_user() {
    print_info "开始卸载用户级安装..."

    INSTALL_DIR="${HOME}/.local"
    BIN_DIR="${INSTALL_DIR}/bin"
    APP_DIR="${INSTALL_DIR}/share/${APP_NAME}"
    ICONS_DIR="${INSTALL_DIR}/share/icons/hicolor/256x256/apps"

    # 删除文件
    print_info "删除已安装的文件..."
    rm -f "${BIN_DIR}/${APP_NAME}"
    rm -f "${BIN_DIR}/terminal_helper"
    rm -rf "${APP_DIR}"
    rm -f "${ICONS_DIR}/${APP_NAME}.png"
    rm -f "${HOME}/Desktop/${DESKTOP_FILE}"
    rm -f "${HOME}/桌面/${DESKTOP_FILE}"
    rm -f "${INSTALL_DIR}/share/applications/${DESKTOP_FILE}"

    # 更新图标缓存
    if command -v gtk-update-icon-cache &> /dev/null; then
        gtk-update-icon-cache -f -t "${INSTALL_DIR}/share/icons/hicolor" 2>/dev/null || true
    fi

    print_success "用户级卸载完成！"
}

# 卸载系统级安装
uninstall_system() {
    print_info "开始卸载系统级安装..."

    # 检查是否有sudo权限
    if [ "$EUID" -ne 0 ]; then
        print_error "系统级卸载需要root权限，请使用: sudo $0 --uninstall"
        exit 1
    fi

    INSTALL_DIR="/usr/local"
    BIN_DIR="${INSTALL_DIR}/bin"
    APP_DIR="${INSTALL_DIR}/share/${APP_NAME}"
    ICONS_DIR="${INSTALL_DIR}/share/icons/hicolor/256x256/apps"

    # 删除文件
    print_info "删除已安装的文件..."
    rm -f "${BIN_DIR}/${APP_NAME}"
    rm -f "${BIN_DIR}/terminal_helper"
    rm -rf "${APP_DIR}"
    rm -f "${ICONS_DIR}/${APP_NAME}.png"
    rm -f "${INSTALL_DIR}/share/applications/${DESKTOP_FILE}"

    # 删除桌面快捷方式
    if [ -n "${SUDO_USER}" ]; then
        SUDO_HOME="/home/${SUDO_USER}"
        rm -f "${SUDO_HOME}/Desktop/${DESKTOP_FILE}"
        rm -f "${SUDO_HOME}/桌面/${DESKTOP_FILE}"
    fi

    # 更新图标缓存
    if command -v gtk-update-icon-cache &> /dev/null; then
        gtk-update-icon-cache -f -t "${INSTALL_DIR}/share/icons/hicolor" 2>/dev/null || true
    fi

    print_success "系统级卸载完成！"
}

# 主函数
main() {
    # 解析命令行参数
    UNINSTALL=false
    INSTALL_MODE="user"

    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_help
                exit 0
                ;;
            -u|--user)
                INSTALL_MODE="user"
                shift
                ;;
            -s|--system)
                INSTALL_MODE="system"
                shift
                ;;
            --uninstall)
                UNINSTALL=true
                shift
                ;;
            *)
                print_error "未知选项: $1"
                show_help
                exit 1
                ;;
        esac
    done

    # 打印标题
    print_header

    # 卸载模式
    if [ "$UNINSTALL" = true ]; then
        if [ "$INSTALL_MODE" = "system" ]; then
            uninstall_system
        else
            uninstall_user
        fi
        exit 0
    fi

    # 检查文件
    check_files

    # 安装
    if [ "$INSTALL_MODE" = "system" ]; then
        install_system
    else
        install_user
    fi
}

# 执行主函数
main "$@"
