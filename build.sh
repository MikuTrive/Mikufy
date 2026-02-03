#
# Mikufy v2.3(stable) - 一键编译脚本
# 编译整个项目为单个可执行文件
# 支持 Fedora 和 ArchLinux
#

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 项目信息
PROJECT_NAME="Mikufy"
VERSION="2.3(stable)"
OUTPUT_FILE="mikufy"

# 编译器设置
CXX="g++"
CXXFLAGS="-std=c++17 -O2 -Wall -Wextra -Wpedantic"

# 链接库 (Fedora 和 ArchLinux 使用 webkitgtk-6.0 和 gtk4)
LIBS="-lwebkitgtk-6.0 -lgtk-4 -lgobject-2.0 -lglib-2.0 -lmagic -lnlohmann_json"

# 包含路径
INCLUDE_DIRS="-Iheaders"

# 源文件
SRC_FILES="src/main.cpp src/file_manager.cpp src/web_server.cpp src/window_manager.cpp"

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
    echo "  ${PROJECT_NAME} v${VERSION} - 编译脚本"
    echo "========================================"
    echo ""
}

# 检测发行版
detect_distro() {
    print_info "检测系统发行版..."
    
    DISTRO_NAME=$(cat /etc/os-release | grep "^NAME=" | cut -d '"' -f 2)
    print_info "检测到发行版: ${DISTRO_NAME}"
    
    if [[ "$DISTRO_NAME" == *"Arch"* ]] || [[ "$DISTRO_NAME" == *"arch"* ]]; then
        DISTRO="arch"
        PKG_MANAGER="pacman"
        PKG_INSTALL="sudo pacman -S"
        print_success "检测到 ArchLinux"
    elif [[ "$DISTRO_NAME" == *"Fedora"* ]] || [[ "$DISTRO_NAME" == *"fedora"* ]]; then
        DISTRO="fedora"
        PKG_MANAGER="dnf"
        PKG_INSTALL="sudo dnf install"
        print_success "检测到 Fedora"
    else
        print_error "不支持的发行版: ${DISTRO_NAME}"
        print_info "目前仅支持 Fedora 和 ArchLinux"
        exit 1
    fi
}

# 检查依赖
check_dependencies() {
    print_info "检查编译依赖..."

    # 检查g++
    if ! command -v g++ &> /dev/null; then
        print_error "未找到 g++ 编译器"
        if [ "$DISTRO" = "fedora" ]; then
            print_info "请安装: ${PKG_INSTALL} gcc-c++"
        else
            print_info "请安装: ${PKG_INSTALL} base-devel"
        fi
        exit 1
    fi

    # 检查pkg-config
    if ! command -v pkg-config &> /dev/null; then
        print_error "未找到 pkg-config"
        if [ "$DISTRO" = "fedora" ]; then
            print_info "请安装: ${PKG_INSTALL} pkg-config"
        else
            print_info "请安装: ${PKG_INSTALL} pkgconf"
        fi
        exit 1
    fi

    # 检查webkitgtk-6.0
    if ! pkg-config --exists webkitgtk-6.0; then
        print_error "未找到 webkitgtk-6.0"
        if [ "$DISTRO" = "fedora" ]; then
            print_info "请安装: ${PKG_INSTALL} webkitgtk6.0-devel"
        else
            print_info "请安装: ${PKG_INSTALL} webkitgtk6.0"
        fi
        exit 1
    fi

    # 检查gtk-4
    if ! pkg-config --exists gtk4; then
        print_error "未找到 gtk-4"
        if [ "$DISTRO" = "fedora" ]; then
            print_info "请安装: ${PKG_INSTALL} gtk4-devel"
        else
            print_info "请安装: ${PKG_INSTALL} gtk4"
        fi
        exit 1
    fi

    # 检查libmagic
    if ! pkg-config --exists libmagic; then
        print_error "未找到 libmagic"
        if [ "$DISTRO" = "fedora" ]; then
            print_info "请安装: ${PKG_INSTALL} file-devel"
        else
            print_info "请安装: ${PKG_INSTALL} file"
        fi
        exit 1
    fi

    # 检查nlohmann_json
    if ! pkg-config --exists nlohmann_json; then
        print_warning "未找到 nlohmann_json，尝试使用系统头文件..."
        if [ ! -f "/usr/include/nlohmann/json.hpp" ]; then
            print_error "未找到 nlohmann/json.hpp"
            if [ "$DISTRO" = "fedora" ]; then
                print_info "请安装: ${PKG_INSTALL} json-devel"
            else
                print_info "请安装: ${PKG_INSTALL} nlohmann-json"
            fi
            exit 1
        fi
    fi

    print_success "所有依赖检查通过"
}

# 获取编译标志
get_compile_flags() {
    # 使用pkg-config获取编译和链接标志 (Fedora 和 ArchLinux 使用 webkitgtk-6.0 和 gtk4)
    CXXFLAGS+=" $(pkg-config --cflags webkitgtk-6.0 gtk4)"
    LDFLAGS="$(pkg-config --libs webkitgtk-6.0 gtk4)"

    # 添加libmagic
    LDFLAGS+=" -lmagic"

    # 添加nlohmann_json
    if pkg-config --exists nlohmann_json; then
        CXXFLAGS+=" $(pkg-config --cflags nlohmann_json)"
        LDFLAGS+=" $(pkg-config --libs nlohmann_json)"
    fi
}

# 编译项目
compile() {
    print_info "开始编译项目..."
    
    # 编译命令
    COMPILE_CMD="${CXX} ${CXXFLAGS} ${INCLUDE_DIRS} ${SRC_FILES} -o ${OUTPUT_FILE} ${LDFLAGS}"
    
    print_info "编译命令:"
    echo "${COMPILE_CMD}"
    echo ""
    
    # 执行编译
    if ${COMPILE_CMD}; then
        print_success "编译成功!"
        print_info "输出文件: ${OUTPUT_FILE}"
        print_info "缓存存储在: ~/.local/share/mikufy"
        
        # 设置可执行权限
        chmod +x ${OUTPUT_FILE}
        
        return 0
    else
        print_error "编译失败!"
        return 1
    fi
}

# 清理临时文件
cleanup() {
    print_info "清理临时文件..."
    
    # 删除build目录
    if [ -d "build" ]; then
        rm -rf build
        print_success "临时文件已清理"
    fi
}

# 显示帮助信息
show_help() {
    echo "用法: $0 [选项]"
    echo ""
    echo "选项:"
    echo "  -h, --help     显示帮助信息"
    echo "  -c, --clean    清理编译产物"
    echo "  -r, --release  发布模式编译 (优化)"
    echo "  -d, --debug    调试模式编译 (包含调试信息)"
    echo ""
}

# 主函数
main() {
    # 解析命令行参数
    CLEAN_ONLY=false
    DEBUG_MODE=false
    
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_help
                exit 0
                ;;
            -c|--clean)
                CLEAN_ONLY=true
                shift
                ;;
            -r|--release)
                CXXFLAGS="-std=c++17 -O3 -Wall -Wextra -Wpedantic -DNDEBUG"
                shift
                ;;
            -d|--debug)
                CXXFLAGS="-std=c++17 -g -Wall -Wextra -Wpedantic -DDEBUG"
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
    
    # 检测发行版
    detect_distro
    
    # 如果只是清理
    if [ "$CLEAN_ONLY" = true ]; then
        cleanup
        print_success "清理完成"
        exit 0
    fi
    
    # 检查依赖
    check_dependencies
    
    # 获取编译标志
    get_compile_flags
    
    # 编译项目
    if compile; then
        print_success "构建完成!"
        echo ""
        print_info "运行: ./${OUTPUT_FILE}"
        exit 0
    else
        print_error "构建失败!"
        exit 1
    fi
}

# 执行主函数
main "$@"
