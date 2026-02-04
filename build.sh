#
# Mikufy v2.5(stable) - 一键编译脚本
# 编译整个项目为单个可执行文件
# 通过检测包管理器支持任意发行版（dnf、apt、pacman 等）
#

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 项目信息
PROJECT_NAME="Mikufy"
VERSION="2.5(stable)"
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

# 检测包管理器
detect_package_manager() {
    print_info "检测包管理器..."
    
    DISTRO_NAME=$(cat /etc/os-release | grep "^NAME=" | cut -d '"' -f 2)
    ID_LIKE=$(cat /etc/os-release | grep "^ID_LIKE=" | cut -d '"' -f 2)
    print_info "检测到发行版: ${DISTRO_NAME}"
    if [ -n "$ID_LIKE" ]; then
        print_info "基于发行版: ${ID_LIKE}"
    fi
    
    # 优先通过 ID_LIKE 推断包管理器
    if [[ "$ID_LIKE" == *"fedora"* ]] || [[ "$ID_LIKE" == *"rhel"* ]] || [[ "$ID_LIKE" == *"centos"* ]]; then
        PKG_MANAGER="dnf"
        PKG_INSTALL="sudo dnf install"
        PKG_FAMILY="rpm"
        print_success "检测到 dnf 包管理器 (RPM 系)"
    elif [[ "$ID_LIKE" == *"debian"* ]] || [[ "$ID_LIKE" == *"ubuntu"* ]]; then
        PKG_MANAGER="apt"
        PKG_INSTALL="sudo apt install"
        PKG_FAMILY="deb"
        print_success "检测到 apt 包管理器 (DEB 系)"
    elif [[ "$ID_LIKE" == *"arch"* ]]; then
        PKG_MANAGER="pacman"
        PKG_INSTALL="sudo pacman -S"
        PKG_FAMILY="pacman"
        print_success "检测到 pacman 包管理器 (Arch 系)"
    fi
    
    # 如果 ID_LIKE 没有匹配，直接检查包管理器命令
    if [ -z "$PKG_MANAGER" ]; then
        if command -v dnf &> /dev/null; then
            PKG_MANAGER="dnf"
            PKG_INSTALL="sudo dnf install"
            PKG_FAMILY="rpm"
            print_success "检测到 dnf 包管理器"
        elif command -v yum &> /dev/null; then
            PKG_MANAGER="yum"
            PKG_INSTALL="sudo yum install"
            PKG_FAMILY="rpm"
            print_success "检测到 yum 包管理器"
        elif command -v apt &> /dev/null; then
            PKG_MANAGER="apt"
            PKG_INSTALL="sudo apt install"
            PKG_FAMILY="deb"
            print_success "检测到 apt 包管理器"
        elif command -v apt-get &> /dev/null; then
            PKG_MANAGER="apt-get"
            PKG_INSTALL="sudo apt-get install"
            PKG_FAMILY="deb"
            print_success "检测到 apt-get 包管理器"
        elif command -v pacman &> /dev/null; then
            PKG_MANAGER="pacman"
            PKG_INSTALL="sudo pacman -S"
            PKG_FAMILY="pacman"
            print_success "检测到 pacman 包管理器"
        elif command -v zypper &> /dev/null; then
            PKG_MANAGER="zypper"
            PKG_INSTALL="sudo zypper install"
            PKG_FAMILY="rpm"
            print_success "检测到 zypper 包管理器"
        else
            print_error "未找到支持的包管理器"
            print_info "支持的包管理器: dnf、yum、apt、apt-get、pacman、zypper"
            exit 1
        fi
    fi
}

# 检查依赖
check_dependencies() {
    print_info "检查编译依赖..."

    # 检查g++
    if ! command -v g++ &> /dev/null; then
        print_error "未找到 g++ 编译器"
        if [ "$PKG_FAMILY" = "rpm" ]; then
            print_info "请安装: ${PKG_INSTALL} gcc-c++"
        elif [ "$PKG_FAMILY" = "pacman" ]; then
            print_info "请安装: ${PKG_INSTALL} base-devel"
        elif [ "$PKG_FAMILY" = "deb" ]; then
            print_info "请安装: ${PKG_INSTALL} g++"
        fi
        exit 1
    fi

    # 检查pkg-config
    if ! command -v pkg-config &> /dev/null; then
        print_error "未找到 pkg-config"
        if [ "$PKG_FAMILY" = "rpm" ]; then
            print_info "请安装: ${PKG_INSTALL} pkg-config"
        elif [ "$PKG_FAMILY" = "pacman" ]; then
            print_info "请安装: ${PKG_INSTALL} pkgconf"
        elif [ "$PKG_FAMILY" = "deb" ]; then
            print_info "请安装: ${PKG_INSTALL} pkg-config"
        fi
        exit 1
    fi

    # 检查webkitgtk-6.0
    if ! pkg-config --exists webkitgtk-6.0; then
        print_error "未找到 webkitgtk-6.0"
        if [ "$PKG_FAMILY" = "rpm" ]; then
            print_info "请安装: ${PKG_INSTALL} webkitgtk6.0-devel"
        elif [ "$PKG_FAMILY" = "pacman" ]; then
            print_info "请安装: ${PKG_INSTALL} webkitgtk6.0"
        elif [ "$PKG_FAMILY" = "deb" ]; then
            print_info "请安装: ${PKG_INSTALL} libwebkitgtk-6.0-dev"
        fi
        exit 1
    fi

    # 检查gtk-4
    if ! pkg-config --exists gtk4; then
        print_error "未找到 gtk-4"
        if [ "$PKG_FAMILY" = "rpm" ]; then
            print_info "请安装: ${PKG_INSTALL} gtk4-devel"
        elif [ "$PKG_FAMILY" = "pacman" ]; then
            print_info "请安装: ${PKG_INSTALL} gtk4"
        elif [ "$PKG_FAMILY" = "deb" ]; then
            print_info "请安装: ${PKG_INSTALL} libgtk-4-dev"
        fi
        exit 1
    fi

    # 检查libmagic
    if ! pkg-config --exists libmagic; then
        print_error "未找到 libmagic"
        if [ "$PKG_FAMILY" = "rpm" ]; then
            print_info "请安装: ${PKG_INSTALL} file-devel"
        elif [ "$PKG_FAMILY" = "pacman" ]; then
            print_info "请安装: ${PKG_INSTALL} file"
        elif [ "$PKG_FAMILY" = "deb" ]; then
            print_info "请安装: ${PKG_INSTALL} libmagic-dev"
        fi
        exit 1
    fi

    # 检查nlohmann_json
    if ! pkg-config --exists nlohmann_json; then
        print_warning "未找到 nlohmann_json，尝试使用系统头文件..."
        if [ ! -f "/usr/include/nlohmann/json.hpp" ]; then
            print_error "未找到 nlohmann/json.hpp"
            if [ "$PKG_FAMILY" = "rpm" ]; then
                print_info "请安装: ${PKG_INSTALL} json-devel"
            elif [ "$PKG_FAMILY" = "pacman" ]; then
                print_info "请安装: ${PKG_INSTALL} nlohmann-json"
            elif [ "$PKG_FAMILY" = "deb" ]; then
                print_info "请安装: ${PKG_INSTALL} nlohmann-json3-dev"
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

# 清理临时文件和编译产物
cleanup() {
    print_info "清理编译产物和临时文件..."

    # 删除可执行文件
    if [ -f "${OUTPUT_FILE}" ]; then
        rm -f "${OUTPUT_FILE}"
        print_success "已删除: ${OUTPUT_FILE}"
    fi

    # 删除.o文件
    if ls *.o 1> /dev/null 2>&1; then
        rm -f *.o
        print_success "已删除: *.o 文件"
    fi

    # 删除.so文件
    if ls *.so 1> /dev/null 2>&1; then
        rm -f *.so
        print_success "已删除: *.so 文件"
    fi

    # 删除.a文件
    if ls *.a 1> /dev/null 2>&1; then
        rm -f *.a
        print_success "已删除: *.a 文件"
    fi

    # 删除build目录
    if [ -d "build" ]; then
        rm -rf build
        print_success "已删除: build/"
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
    
    # 检测包管理器
    detect_package_manager
    
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
