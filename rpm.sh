#!/bin/bash

#
# Mikufy v2.7-nova - RPM 打包脚本
# 自动构建 RPM 包
#

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 项目信息
PROJECT_NAME="Mikufy"
VERSION="2.7.nova"
RELEASE="1"
APP_NAME="mikufy"
SPEC_FILE="${APP_NAME}.spec"

# RPM 构建目录
RPM_BUILD_DIR="$HOME/rpmbuild"
RPM_SOURCES_DIR="${RPM_BUILD_DIR}/SOURCES"
RPM_SPECS_DIR="${RPM_BUILD_DIR}/SPECS"
RPM_RPMS_DIR="${RPM_BUILD_DIR}/RPMS"
RPM_BUILDROOT="${RPM_BUILD_DIR}/BUILDROOT"

# 源代码压缩包名称
SOURCE_TARBALL="${APP_NAME}-${VERSION}.tar.gz"

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

# 检查必要的命令是否存在
check_dependencies() {
    print_info "检查打包依赖..."

    local missing_deps=()

    # 检查 rpm-build
    if ! command -v rpmbuild &> /dev/null; then
        missing_deps+=("rpm-build")
    fi

    # 检查 gcc-c++
    if ! command -v g++ &> /dev/null; then
        missing_deps+=("gcc-c++")
    fi

    # 检查 pkg-config
    if ! command -v pkg-config &> /dev/null; then
        missing_deps+=("pkg-config")
    fi

    # 检查 tar
    if ! command -v tar &> /dev/null; then
        missing_deps+=("tar")
    fi

    if [ ${#missing_deps[@]} -gt 0 ]; then
        print_error "缺少必要的打包依赖："
        for dep in "${missing_deps[@]}"; do
            echo "  - $dep"
        done
        echo ""
        print_info "请运行以下命令安装依赖："
        echo "  sudo dnf install ${missing_deps[*]}"
        exit 1
    fi

    print_success "所有必要的打包依赖已满足"
}

# 创建 RPM 构建目录结构
create_rpm_build_dirs() {
    print_info "创建 RPM 构建目录结构..."

    mkdir -p "${RPM_SOURCES_DIR}"
    mkdir -p "${RPM_SPECS_DIR}"
    mkdir -p "${RPM_BUILD_DIR}/BUILD"
    mkdir -p "${RPM_BUILD_DIR}/RPMS"
    mkdir -p "${RPM_BUILD_DIR}/SRPMS"

    print_success "RPM 构建目录已创建"
}

# 创建源代码压缩包
create_source_tarball() {
    print_info "创建源代码压缩包..."

    local temp_dir=$(mktemp -d)
    local source_dir="${temp_dir}/${APP_NAME}-${VERSION}"

    print_info "临时目录: ${temp_dir}"

    # 创建项目目录
    mkdir -p "${source_dir}"

    # 复制需要打包的文件和目录
    print_info "复制源代码文件..."

    # 复制所有文件和目录
    cp -r . "${source_dir}/"

    # 进入源代码目录进行清理
    cd "${source_dir}"

    # 删除不需要打包的文件
    print_info "清理不需要打包的文件..."
    rm -f mikufy.desktop
    rm -f LICENSE
    rm -f install.sh
    rm -f package.nix
    rm -f README.md
    rm -f flake.nix
    rm -f build.sh
    rm -f Explain.txt
    rm -f rpm.sh
    rm -rf build
    rm -rf debug
    rm -f mikufy
    rm -f .debug.log.kate-swp

    # 返回原目录
    cd - > /dev/null

    # 创建压缩包 (使用小写名称以匹配 spec 文件)
    print_info "创建压缩包: ${SOURCE_TARBALL}"
    tar -czf "${RPM_SOURCES_DIR}/${SOURCE_TARBALL}" -C "${temp_dir}" "${APP_NAME}-${VERSION}"

    # 清理临时目录
    rm -rf "${temp_dir}"

    print_success "源代码压缩包已创建: ${RPM_SOURCES_DIR}/${SOURCE_TARBALL}"
}

# 创建 SPEC 文件
create_spec_file() {
    print_info "创建 SPEC 文件..."

    # 生成日期字符串
    local changelog_date=$(date +'%a %b %d %Y')

    cat > "${RPM_SPECS_DIR}/${SPEC_FILE}" << EOF
# 禁用调试包生成
%global debug_package %{nil}

Name:           mikufy
Version:        2.7.nova
Release:        1%{?dist}
Summary:        Mikufy - A modern file editor with web interface

License:        MIT
URL:            https://github.com/MikuTrive/Mikufy
Source0:        %{name}-%{version}.tar.gz

# 构建依赖
BuildRequires:  gcc-c++
BuildRequires:  pkg-config
BuildRequires:  webkitgtk6.0-devel
BuildRequires:  gtk4-devel
BuildRequires:  file-devel
BuildRequires:  glib2-devel
BuildRequires:  file-libs
BuildRequires:  nlohmann-json-devel

# 运行时依赖
Requires:       webkitgtk6.0
Requires:       gtk4
Requires:       glib2
Requires:       file-libs

%description
Mikufy is a modern file editor with a beautiful web-based interface.
It provides syntax highlighting, file management, and a clean user experience.

%prep
%setup -q

%build
# 获取编译和链接标志
export CXXFLAGS="\$(pkg-config --cflags webkitgtk-6.0 gtk4)"
export LDFLAGS="\$(pkg-config --libs webkitgtk-6.0 gtk4) -lmagic"

# 添加 nlohmann_json 如果可用
if pkg-config --exists nlohmann_json 2>/dev/null; then
    CXXFLAGS+=" \$(pkg-config --cflags nlohmann_json)"
    LDFLAGS+=" \$(pkg-config --libs nlohmann_json)"
fi

# 编译项目
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic \\
    -Iheaders \\
    \${CXXFLAGS} \\
    src/main.cpp \\
    src/file_manager.cpp \\
    src/web_server.cpp \\
    src/window_manager.cpp \\
    src/text_buffer.cpp \\
    \${LDFLAGS} \\
    -o mikufy

%install
# 创建安装目录
mkdir -p %{buildroot}%{_datadir}/mikufy
mkdir -p %{buildroot}%{_datadir}/mikufy/web
mkdir -p %{buildroot}%{_bindir}
mkdir -p %{buildroot}%{_datadir}/icons/hicolor/256x256/apps

# 安装可执行文件
install -m 755 mikufy %{buildroot}%{_datadir}/mikufy/

# 安装 web 资源（包括 Background 和 Icons 子目录）
cp -r web/* %{buildroot}%{_datadir}/mikufy/web/
chmod -R 755 %{buildroot}%{_datadir}/mikufy/web
chmod 644 %{buildroot}%{_datadir}/mikufy/web/style.css

# 安装图标
install -m 644 web/Mikufy.png %{buildroot}%{_datadir}/icons/hicolor/256x256/apps/mikufy.png

# 安装头文件（可选）
mkdir -p %{buildroot}%{_includedir}/mikufy
cp -r headers/* %{buildroot}%{_includedir}/mikufy/

# 安装源文件（可选）
mkdir -p %{buildroot}%{_datadir}/mikufy/src
cp -r src/* %{buildroot}%{_datadir}/mikufy/src/

# 创建启动脚本
cat > %{buildroot}%{_bindir}/mikufy << 'SCRIPT_EOF'
#!/bin/bash
exec %{_datadir}/mikufy/mikufy "\$@"
SCRIPT_EOF
chmod +x %{buildroot}%{_bindir}/mikufy

# 创建 desktop 文件
mkdir -p %{buildroot}%{_datadir}/applications
cat > %{buildroot}%{_datadir}/applications/mikufy.desktop << 'DESKTOP_EOF'
[Desktop Entry]
Name=Mikufy
Name[zh_CN]=Miku 编辑器
GenericName=File Editor
GenericName[zh_CN]=文件编辑器
Comment=A modern file editor with web interface
Comment[zh_CN]=一个现代化的文件编辑器，具有网页界面
Exec=mikufy
Icon=mikufy
Terminal=false
Type=Application
Categories=Utility;TextEditor;Development;
StartupNotify=true
DESKTOP_EOF

%files
%{_bindir}/mikufy
%{_datadir}/mikufy
%{_datadir}/icons/hicolor/256x256/apps/mikufy.png
%{_datadir}/applications/mikufy.desktop
%{_includedir}/mikufy

%post
# 修改 style.css 文件权限，允许所有用户读写
chmod 666 %{_datadir}/mikufy/web/style.css 2>/dev/null || true

%changelog
* ${changelog_date} Builder <builder@example.com> - 2.7.nova-1
- Initial package for Mikufy v2.7-nova
EOF

    print_success "SPEC 文件已创建: ${RPM_SPECS_DIR}/${SPEC_FILE}"
}

# 构建 RPM 包
build_rpm() {
    print_info "开始构建 RPM 包..."

    cd "${RPM_BUILD_DIR}"

    # 清理旧的构建文件
    rm -rf BUILD BUILDROOT

    # 执行 rpmbuild
    rpmbuild -ba "${RPM_SPECS_DIR}/${SPEC_FILE}"

    if [ $? -eq 0 ]; then
        print_success "RPM 包构建成功！"
    else
        print_error "RPM 包构建失败！"
        exit 1
    fi

    cd - > /dev/null
}

# 查找构建的 RPM 包
find_rpm() {
    print_info "查找构建的 RPM 包..."

    local rpm_file=$(find "${RPM_RPMS_DIR}" -name "${APP_NAME}-${VERSION}-${RELEASE}*.rpm" | head -n 1)

    if [ -n "${rpm_file}" ]; then
        print_success "RPM 包位置: ${rpm_file}"
        echo ""
        print_info "您可以使用以下命令安装："
        echo "  sudo dnf install ${rpm_file}"
    else
        print_warning "未找到 RPM 包"
    fi
}

# 主函数
main() {
    echo ""
    echo "========================================"
    echo "  ${PROJECT_NAME} v${VERSION} - RPM 打包"
    echo "========================================"
    echo ""

    # 检查依赖
    check_dependencies

    # 创建 RPM 构建目录
    create_rpm_build_dirs

    # 创建源代码压缩包
    create_source_tarball

    # 创建 SPEC 文件
    create_spec_file

    # 构建 RPM 包
    build_rpm

    # 查找并显示 RPM 包位置
    find_rpm

    echo ""
    print_success "打包完成！"
}

# 执行主函数
main "$@"
