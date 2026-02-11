#
# Mikufy v2.11-nova - DEB 打包脚本
# 自动构建 DEB 包
# 此脚本由GPT-OSS-20B模型生成

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 项目信息
PROJECT_NAME="Mikufy"
VERSION="2.11.nova"
RELEASE="1"
APP_NAME="mikufy"

# DEB 构建目录
DEB_BUILD_DIR="$HOME/debbuild/mikufy"
DEB_SOURCE_DIR="${DEB_BUILD_DIR}/${APP_NAME}-${VERSION}"

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

    # 检查 dpkg-buildpackage
    if ! command -v dpkg-buildpackage &> /dev/null; then
        missing_deps+=("dpkg-dev")
    fi

    # 检查 debhelper
    if ! command -v dh &> /dev/null; then
        missing_deps+=("debhelper")
    fi

    # 检查 g++
    if ! command -v g++ &> /dev/null; then
        missing_deps+=("g++")
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
        echo "  sudo apt install ${missing_deps[*]}"
        exit 1
    fi

    print_success "所有必要的打包依赖已满足"
}

# 创建 DEB 构建目录结构
create_deb_build_dirs() {
    print_info "创建 DEB 构建目录结构..."

    mkdir -p "${DEB_BUILD_DIR}"

    print_success "DEB 构建目录已创建"
}

# 复制源代码到构建目录
copy_source_files() {
    print_info "复制源代码文件..."

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
    rm -rf .git
    rm -f mikufy.desktop
    rm -f LICENSE
    rm -f install.sh
    rm -f package.nix
    rm -f README.md
    rm -f flake.nix
    rm -f build.sh
    rm -f Explain.txt
    rm -f rpm.sh
    rm -f deb.sh
    rm -rf build
    rm -rf debug
    rm -f mikufy
    rm -f .debug.log.kate-swp

    # 返回原目录
    cd - > /dev/null

    # 清空并重新创建构建目录
    print_info "清空构建目录..."
    rm -rf "${DEB_SOURCE_DIR}"
    mkdir -p "${DEB_SOURCE_DIR}"

    # 复制到最终目录（使用 cp 避免跨设备问题）
    print_info "复制源代码到构建目录..."
    cp -r "${source_dir}"/* "${DEB_SOURCE_DIR}/"

    # 清理临时目录
    rm -rf "${temp_dir}"

    print_success "源代码文件已复制"
}

# 创建上游源码 tarball
create_orig_tarball() {
    print_info "创建上游源码 tarball..."

    local temp_dir=$(mktemp -d)
    local source_dir="${temp_dir}/${APP_NAME}-${VERSION}"

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
    rm -rf .git
    rm -f mikufy.desktop
    rm -f LICENSE
    rm -f install.sh
    rm -f package.nix
    rm -f README.md
    rm -f flake.nix
    rm -f build.sh
    rm -f Explain.txt
    rm -f rpm.sh
    rm -f deb.sh
    rm -rf build
    rm -rf debug
    rm -f mikufy
    rm -f .debug.log.kate-swp

    # 返回原目录
    cd - > /dev/null

    # 创建压缩包
    print_info "创建压缩包: ${APP_NAME}_${VERSION}.orig.tar.gz"
    tar -czf "${DEB_BUILD_DIR}/${APP_NAME}_${VERSION}.orig.tar.gz" -C "${temp_dir}" "${APP_NAME}-${VERSION}"

    # 清理临时目录
    rm -rf "${temp_dir}"

    print_success "上游源码 tarball 已创建"
}

# 创建 debian 目录和文件
create_debian_files() {
    print_info "创建 debian 目录和文件..."

    local debian_dir="${DEB_SOURCE_DIR}/debian"
    mkdir -p "${debian_dir}"

    # 创建 debian/control 文件
    print_info "创建 debian/control..."
    cat > "${debian_dir}/control" << EOF
Source: mikufy
Section: utils
Priority: optional
Maintainer: MiraTrive <mikulxz08@gmail.com>
Build-Depends: debhelper-compat (= 13),
                g++,
                pkg-config,
                libwebkitgtk-6.0-dev,
                libgtk-4-dev,
                libglib2.0-dev,
                libmagic-dev,
                nlohmann-json3-dev
Standards-Version: 4.6.0
Homepage: https://github.com/MikuTrive/Mikufy/tree/mikufy-v2.11-nova

Package: mikufy
Architecture: any
Depends: \${shlibs:Depends},
         \${misc:Depends}
Description: Mikufy - A modern file editor with web interface
 Mikufy is a modern file editor with a beautiful web-based interface.
 It provides syntax highlighting, file management, and a clean user experience.
 .
 Features:
  - High-performance virtual scrolling editor supporting large files
  - Piece Table architecture for efficient text editing
  - Beautiful web-based interface with multiple themes
  - Built-in file manager with preview capabilities
  - Support for various programming languages with syntax highlighting
EOF

    # 创建 debian/rules 文件
    print_info "创建 debian/rules..."
    cat > "${debian_dir}/rules" << 'EOF'
#!/usr/bin/make -f

# 生成编译和链接标志
export CXXFLAGS += $(shell pkg-config --cflags webkitgtk-6.0 gtk4)
export LDFLAGS += $(shell pkg-config --libs webkitgtk-6.0 gtk4) -lmagic

# 添加 nlohmann_json 如果可用
ifeq ($(shell pkg-config --exists nlohmann_json && echo yes),yes)
    export CXXFLAGS += $(shell pkg-config --cflags nlohmann_json)
    export LDFLAGS += $(shell pkg-config --libs nlohmann_json)
endif

%:
	dh $@

override_dh_auto_build:
	# 编译主程序
	g++ -std=c++23 -O2 -Wall -Wextra -Wpedantic \
	    -Iheaders \
	    $(CXXFLAGS) \
	    src/main.cpp \
	    src/file_manager.cpp \
	    src/web_server.cpp \
	    src/window_manager.cpp \
	    src/text_buffer.cpp \
	    src/terminal_manager.cpp \
	    src/process_launcher.cpp \
	    src/terminal_window.cpp \
	    $(LDFLAGS) \
	    -o mikufy

	# 编译 terminal_helper 独立程序
	g++ -std=c++23 -O2 -Wall -Wextra -Wpedantic \
	    -Iheaders \
	    $(CXXFLAGS) \
	    src/terminal_helper.cpp \
	    $(LDFLAGS) \
	    -o terminal_helper

override_dh_auto_clean:
	rm -f mikufy terminal_helper
	rm -rf build debug
EOF
    chmod +x "${debian_dir}/rules"

    # 创建 debian/changelog 文件
    print_info "创建 debian/changelog..."
    cat > "${debian_dir}/changelog" << EOF
mikufy (${VERSION}-1) unstable; urgency=medium

  * Initial package for Mikufy v2.11-nova
  * High-performance virtual scrolling editor
  * Piece Table architecture for efficient text editing
  * Beautiful web-based interface with multiple themes

 -- MiraTrive <mikulxz08@gmail.com>  $(date -R)
EOF

    # 创建 debian/copyright 文件
    print_info "创建 debian/copyright..."
    cat > "${debian_dir}/copyright" << 'EOF'
Format: https://www.debian.org/doc/packaging-manuals/copyright-format/1.0/
Upstream-Name: Mikufy
Upstream-Contact: MiraTrive <mikulxz08@gmail.com>
Source: https://github.com/MikuTrive/Mikufy

Files: *
Copyright: MiraTrive
License: GPL-3.0+
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 .
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 .
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 .
 On Debian systems, the full text of the GPL-3.0 license can be found
 in the file /usr/share/common-licenses/GPL-3.

Files: debian/*
Copyright: MiraTrive
License: GPL-3.0+
EOF

    # 创建 debian/install 文件
    print_info "创建 debian/install..."
    cat > "${debian_dir}/install" << 'EOF'
web/* usr/share/mikufy/web
mikufy usr/share/mikufy
EOF

    # 创建 debian/links 文件
    print_info "创建 debian/links..."
    cat > "${debian_dir}/links" << 'EOF'
usr/share/mikufy/mikufy usr/bin/mikufy
usr/share/mikufy/terminal_helper usr/bin/terminal_helper
EOF

    # 创建 debian/mikufy.dirs 文件
    print_info "创建 debian/mikufy.dirs..."
    cat > "${debian_dir}/mikufy.dirs" << 'EOF'
usr/share/mikufy
usr/share/mikufy/web
usr/share/mikufy/src
usr/share/mikufy/headers
usr/share/icons/hicolor/256x256/apps
usr/share/applications
EOF

    # 创建 debian/postinst 文件
    print_info "创建 debian/postinst..."
    cat > "${debian_dir}/postinst" << 'EOF'
#!/bin/bash
set -e

# 修改 style.css 文件权限，允许所有用户读写
chmod 666 /usr/share/mikufy/web/style.css 2>/dev/null || true

# 更新桌面数据库
update-desktop-database /usr/share/applications 2>/dev/null || true

# 更新图标缓存
gtk-update-icon-cache -f -t /usr/share/icons/hicolor 2>/dev/null || true

exit 0
EOF
    chmod +x "${debian_dir}/postinst"

    # 创建 debian/postrm 文件
    print_info "创建 debian/postrm..."
    cat > "${debian_dir}/postrm" << 'EOF'
#!/bin/bash
set -e

# 更新桌面数据库
update-desktop-database /usr/share/applications 2>/dev/null || true

# 更新图标缓存
gtk-update-icon-cache -f -t /usr/share/icons/hicolor 2>/dev/null || true

exit 0
EOF
    chmod +x "${debian_dir}/postrm"

    # 创建 debian/mikufy.install 文件
    print_info "创建 debian/mikufy.install..."
    cat > "${debian_dir}/mikufy.install" << 'EOF'
web/* usr/share/mikufy/web
web/Mikufy.png usr/share/icons/hicolor/256x256/apps/mikufy.png
mikufy usr/share/mikufy
terminal_helper usr/share/mikufy
EOF

    # 创建 debian/mikufy.manpages 文件（如果有的话）
    touch "${debian_dir}/mikufy.manpages"

    # 创建 debian/source/format 文件
    print_info "创建 debian/source/format..."
    mkdir -p "${debian_dir}/source"
    echo "3.0 (quilt)" > "${debian_dir}/source/format"

    print_success "debian 目录和文件已创建"
}

# 创建桌面文件
create_desktop_file() {
    print_info "创建桌面文件..."

    local debian_dir="${DEB_SOURCE_DIR}/debian"

    cat > "${debian_dir}/mikufy.desktop" << 'EOF'
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
EOF

    print_success "桌面文件已创建"
}

# 构建 DEB 包
build_deb() {
    print_info "开始构建 DEB 包..."

    cd "${DEB_SOURCE_DIR}"

    # 使用 dpkg-buildpackage 构建
    dpkg-buildpackage -us -uc

    if [ $? -eq 0 ]; then
        print_success "DEB 包构建成功！"
    else
        print_error "DEB 包构建失败！"
        exit 1
    fi

    cd - > /dev/null
}

# 查找构建的 DEB 包
find_deb() {
    print_info "查找构建的 DEB 包..."

    local deb_file=$(find "${DEB_BUILD_DIR}" -name "${APP_NAME}_${VERSION}-1_*.deb" | head -n 1)

    if [ -n "${deb_file}" ]; then
        print_success "DEB 包位置: ${deb_file}"
        echo ""
        print_info "您可以使用以下命令安装："
        echo "  sudo dpkg -i ${deb_file}"
        echo "  sudo apt install -f  # 修复依赖问题（如果需要）"
    else
        print_warning "未找到 DEB 包"
    fi
}

# 主函数
main() {
    echo ""
    echo "========================================"
    echo "  ${PROJECT_NAME} v${VERSION} - DEB 打包"
    echo "========================================"
    echo ""

    # 检查依赖
    check_dependencies

    # 创建 DEB 构建目录
    create_deb_build_dirs

    # 复制源代码文件
    copy_source_files

    # 创建上游源码 tarball
    create_orig_tarball

    # 创建 debian 目录和文件
    create_debian_files

    # 创建桌面文件
    create_desktop_file

    # 构建 DEB 包
    build_deb

    # 查找并显示 DEB 包位置
    find_deb

    echo ""
    print_success "打包完成！"
}

# 执行主函数
main "$@"
