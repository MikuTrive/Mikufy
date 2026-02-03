#
# Mikufy v2.4(stable) - 一键清理脚本
# 清理编译构建产生的产物
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
    echo "  ${PROJECT_NAME} v${VERSION} - 清理脚本"
    echo "========================================"
    echo ""
}

# 清理编译产物
clean_build() {
    print_info "清理编译产物..."
    
    # 删除可执行文件
    if [ -f "${OUTPUT_FILE}" ]; then
        rm -f "${OUTPUT_FILE}"
        print_success "已删除: ${OUTPUT_FILE}"
    else
        print_warning "未找到: ${OUTPUT_FILE}"
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
}

# 清理临时文件
clean_temp() {
    print_info "清理临时文件..."
    
    # 删除临时文件
    if [ -f "*.tmp" ]; then
        rm -f *.tmp
        print_success "已删除: *.tmp 文件"
    fi
    
    if [ -f "*.log" ]; then
        rm -f *.log
        print_success "已删除: *.log 文件"
    fi
}

# 清理备份文件
clean_backup() {
    print_info "清理备份文件..."
    
    # 删除备份文件
    if ls *.bak 1> /dev/null 2>&1; then
        rm -f *.bak
        print_success "已删除: *.bak 文件"
    fi
    
    if ls *~ 1> /dev/null 2>&1; then
        rm -f *~
        print_success "已删除: *~ 文件"
    fi
}

# 清理IDE文件
clean_ide() {
    print_info "清理IDE文件..."
    
    # 删除.vscode目录
    if [ -d ".vscode" ]; then
        rm -rf .vscode
        print_success "已删除: .vscode/"
    fi
    
    # 删除.clang目录
    if [ -d ".clang" ]; then
        rm -rf .clang
        print_success "已删除: .clang/"
    fi
}

# 显示帮助信息
show_help() {
    echo "用法: $0 [选项]"
    echo ""
    echo "选项:"
    echo "  -h, --help     显示帮助信息"
    echo "  -a, --all      清理所有产物（包括IDE文件）"
    echo "  -b, --build    仅清理编译产物"
    echo "  -t, --temp     仅清理临时文件"
    echo ""
}

# 主函数
main() {
    # 解析命令行参数
    CLEAN_ALL=false
    CLEAN_BUILD_ONLY=false
    CLEAN_TEMP_ONLY=false
    
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_help
                exit 0
                ;;
            -a|--all)
                CLEAN_ALL=true
                shift
                ;;
            -b|--build)
                CLEAN_BUILD_ONLY=true
                shift
                ;;
            -t|--temp)
                CLEAN_TEMP_ONLY=true
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
    
    # 根据选项执行清理
    if [ "$CLEAN_BUILD_ONLY" = true ]; then
        clean_build
    elif [ "$CLEAN_TEMP_ONLY" = true ]; then
        clean_temp
    elif [ "$CLEAN_ALL" = true ]; then
        clean_build
        clean_temp
        clean_backup
        clean_ide
    else
        # 默认清理编译产物和临时文件
        clean_build
        clean_temp
        clean_backup
    fi
    
    print_success "清理完成!"
    exit 0
}

# 执行主函数
main "$@"
