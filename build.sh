#!/bin/bash

# Mikufy v2.7-nova

PROJECT_NAME="Mikufy"
VERSION="v2.7-nova"
OUTPUT_FILE="mikufy"
BUILD_DIR="build"
DEBUG_DIR="debug"
DEBUG_LOG="${DEBUG_DIR}/debug.log"

# 编译器设置
CXX="g++"
CXXFLAGS="-std=c++17 -O2 -Wall -Wextra -Wpedantic -c"

# 包含路径
INCLUDE_DIRS="-Iheaders"

# 源文件列表
SRC_FILES=(
    "src/main.cpp"
    "src/file_manager.cpp"
    "src/web_server.cpp"
    "src/window_manager.cpp"
    "src/text_buffer.cpp"
)

# 颜色定义
COLOR_RESET='\033[0m'
COLOR_GREEN='\033[32m'
COLOR_RED='\033[31m'
COLOR_ORANGE='\033[33m'

# 获取编译标志
get_compile_flags() {
    # 使用pkg-config获取编译和链接标志
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

# 获取CPU核心数信息
get_cpu_info() {
    # 获取逻辑核心数（线程数）
    local threads=$(nproc 2>/dev/null || echo 4)

    # 获取物理核心数
    local cores=$(lscpu -p 2>/dev/null | grep -v '^#' 2>/dev/null | sort -u -t, -k 2,4 2>/dev/null | wc -l 2>/dev/null)

    # 如果无法获取物理核心数，使用逻辑核心数
    if [ "$cores" -eq 0 ] || [ -z "$cores" ]; then
        cores=$threads
    fi

    echo "$cores $threads"
}

# 创建debug目录和日志文件
init_debug_log() {
    mkdir -p ${DEBUG_DIR}
    echo "========================================" > ${DEBUG_LOG}
    echo "Mikufy v${VERSION} 编译日志" >> ${DEBUG_LOG}
    echo "时间: $(date)" >> ${DEBUG_LOG}
    echo "CPU: ${CORES} 核心 ${THREADS} 线程" >> ${DEBUG_LOG}
    echo "========================================" >> ${DEBUG_LOG}
    echo "" >> ${DEBUG_LOG}
}

# 写入日志
log_message() {
    echo "$1" >> ${DEBUG_LOG}
}

# 获取编译标志
get_compile_flags

# 获取CPU信息
read CORES THREADS <<< $(get_cpu_info)

# 初始化调试日志
init_debug_log

# 清理旧的构建文件
echo -e "${COLOR_ORANGE}<INFO>${COLOR_RESET} 清理旧的构建产物..."
rm -rf ${BUILD_DIR}
rm -f ${OUTPUT_FILE}

# 创建构建目录
mkdir -p ${BUILD_DIR}

# 计算总文件数
TOTAL_FILES=${#SRC_FILES[@]}

echo ""
echo -e "${COLOR_ORANGE}<INFO>${COLOR_RESET} 开始编译 ${PROJECT_NAME} v${VERSION}..."
echo -e "${COLOR_ORANGE}<INFO>${COLOR_RESET} 使用 ${CORES} 核心 ${THREADS} 线程进行编译"
echo ""

# 准备编译任务
declare -a PIDS
declare -a OBJ_FILES
CURRENT_FILE=0

# 启动所有编译任务（并行）
for src_file in "${SRC_FILES[@]}"; do
    CURRENT_FILE=$((CURRENT_FILE + 1))
    base_name=$(basename "$src_file" .cpp)
    obj_file="${BUILD_DIR}/${base_name}.o"

    # 在后台启动编译任务，将输出重定向到日志文件
    (
        echo "编译: ${src_file}" >> ${DEBUG_LOG}
        ${CXX} ${CXXFLAGS} ${INCLUDE_DIRS} ${src_file} -o ${obj_file} ${LDFLAGS} >> ${DEBUG_LOG} 2>&1
        echo $? > "${obj_file}.status"
    ) &

    PIDS+=($!)
    OBJ_FILES+=("${obj_file}")
done

# 等待所有任务完成并显示进度
COMPLETED=0
while [ $COMPLETED -lt $TOTAL_FILES ]; do
    for i in "${!PIDS[@]}"; do
        if [ "${PIDS[$i]}" != "0" ]; then
            # 检查进程是否还在运行
            if ! kill -0 ${PIDS[$i]} 2>/dev/null; then
                # 进程已完成
                wait ${PIDS[$i]}
                EXIT_CODE=$?

                # 检查临时状态文件
                if [ -f "${OBJ_FILES[$i]}.status" ]; then
                    EXIT_CODE=$(cat "${OBJ_FILES[$i]}.status")
                    rm -f "${OBJ_FILES[$i]}.status"
                fi

                if [ $EXIT_CODE -ne 0 ]; then
                    echo -e "${COLOR_RED}[ERROR]${COLOR_RESET} 编译失败：${SRC_FILES[$i]}"
                    echo -e "${COLOR_RED}[ERROR]${COLOR_RESET} 详细错误信息已保存到 ${DEBUG_LOG}"
                    echo ""
                    # 终止所有剩余的编译任务
                    for pid in "${PIDS[@]}"; do
                        if [ "$pid" != "0" ]; then
                            kill $pid 2>/dev/null
                        fi
                    done
                    exit 1
                fi

                COMPLETED=$((COMPLETED + 1))
                echo -e "${COLOR_ORANGE}<INFO>${COLOR_RESET} [${COMPLETED}/${TOTAL_FILES}] 已完成编译 ${SRC_FILES[$i]}"
                PIDS[$i]="0"
            fi
        fi
    done
    sleep 0.1
done

echo ""
echo -e "${COLOR_ORANGE}<INFO>${COLOR_RESET} 正在链接生成 ${OUTPUT_FILE}..."

# 收集所有目标文件
OBJ_FILES_STR=""
for obj_file in "${OBJ_FILES[@]}"; do
    OBJ_FILES_STR="${OBJ_FILES_STR} ${obj_file}"
done

# 链接所有目标文件，错误信息也写入日志
echo "链接: ${OUTPUT_FILE}" >> ${DEBUG_LOG}
if ${CXX} ${OBJ_FILES_STR} -o ${OUTPUT_FILE} ${LDFLAGS} >> ${DEBUG_LOG} 2>&1; then
    echo ""
    echo -e "[${COLOR_GREEN}OK${COLOR_RESET}] 编译成功！"
    chmod +x ${OUTPUT_FILE}
    echo ""
    echo "编译日志已保存到: ${DEBUG_LOG}"
    echo ""
else
    echo ""
    echo -e "${COLOR_RED}[ERROR]${COLOR_RESET} 链接失败！"
    echo -e "${COLOR_RED}[ERROR]${COLOR_RESET} 详细错误信息已保存到 ${DEBUG_LOG}"
    echo ""
    exit 1
fi