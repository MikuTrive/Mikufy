# Mikufy v2.7-nova - 代码编辑器

## 🙏 致谢贡献者

特别感谢 **luozenan** 对本项目的贡献！

- 当前提供 `Debian13`、`Fedora43`、`ArchLinux`、`NixOS` 的完整支持
- 其他大部分 `Debian系`、`RHEL系`、`ArchLinux系` 的衍生版也应支持
- v2.7-nova 为完整的稳定版发布（可联系我们合作开发贡献）

---

## 📋 版本更新说明

### v2.7-nova

**重大更新：高性能架构重构**

本次版本对整个项目进行了全面的架构重构，引入了 Piece Table 文本缓冲区和虚拟滚动技术，支持大文件（10万~50万行）的高效编辑。

#### 核心架构升级

1. **高性能文本缓冲区（Piece Table）**
   - 新增 `text_buffer.cpp/h` 模块
   - 使用 mmap 映射原始文件，避免大文件一次性加载到内存
   - Piece Table 架构提供 O(log n) 的插入和删除操作
   - 行缓存机制加速行范围查询
   - 线程安全：使用互斥锁保护所有操作

2. **虚拟滚动编辑器**
   - 新增 `virtual_editor.js` 虚拟滚动编辑器
   - 基于 Canvas 渲染，使用 requestAnimationFrame 驱动重绘
   - 只渲染可见区域 + 预加载缓冲行
   - 批量数据传输，从 C++ 后端批量获取行数据
   - 支持 10万~50万行文件的流畅滚动（60 FPS）

3. **代码规范化重构**
   - 后端代码（C++）遵循 Linux 内核代码风格规范
   - 统一使用 Tab 缩进（8字符宽度）
   - 统一使用 K&R 括号风格
   - 注释覆盖率提升至 95% 以上

#### 功能增强

- **文件缓存机制**：FileManager 新增 LRU 缓存，提升重复读取性能
- **壁纸更换**：支持在 17 种预设壁纸中切换（可自行加入新壁纸-新壁纸前缀必须index-*）
- **异步文件对话框**：使用 GTK4 的异步 API，避免阻塞 GTK 主循环
- **增强的全屏控制**：精确控制全屏进入和退出，防止意外退出
- **媒体文件预览**：支持图片、视频、音频文件预览

#### Bug 修复

- 修复了在大型目录中读取文件时可能卡死的问题（限制读取 2000 个条目）
- 修复了文件类型检测的准确性（使用哈希表优化）
- 修复了全屏退出时可能被意外触发的问题
- 修复了 Arch Linux 上的 X11 共享内存兼容性问题

#### 平台支持

- **主要支持平台**：Fedora43、ArchLinux、Debian13、NixOS
- **兼容平台**：其他使用 GTK4 和 WebKitGTK 6.0 的 Linux 发行版
- **显示服务器**：Wayland 和 XWayland

---

## 🚀 快捷键列表

| 快捷键 | 功能说明 |
|--------|----------|
| `Ctrl+S` | 保存所有文件 |
| `F5` | 刷新编辑器内容 |
| `Ctrl+O` | 打开工作目录 |
| `Ctrl+F` | 新建子目录 |
| `Ctrl+N` | 新建子文件 |
| `Tab` | 编辑区域自动缩进（4格） |
| `F11` | 切换全屏/窗口化 |
| `Ctrl+方向左键` | 快捷切换左侧标签页 |
| `Ctrl+方向右键` | 快捷切换右侧标签页 |
| `Ctrl+M` | 删除当前所在标签页（不删除内容） |

---

## 📐 项目架构

### 技术栈

#### 后端（C++17）

- **GUI 框架**：GTK4
- **Web 渲染引擎**：WebKitGTK 6.0
- **HTTP 服务器**：HTTP/1.1 服务器（poll I/O 模型）
- **JSON 处理**：nlohmann/json
- **文件类型检测**：libmagic
- **文本缓冲区**：Piece Table 架构

#### 前端（原生 JavaScript）

- **UI 框架**：原生 JavaScript + HTML5 + CSS3
- **编辑器渲染**：Canvas 虚拟滚动
- **通信协议**：HTTP/1.1 JSON API
- **图标**：SVG 矢量图标-少量PNG

### 核心模块

#### 1. WindowManager（窗口管理器）
- **职责**：创建和管理 GTK 窗口，嵌入 WebKit WebView
- **特性**：
  - GTK4 兼容，使用异步文件对话框
  - 全屏管理：支持 F11 切换，阻止其他方式退出全屏
  - 线程安全：使用互斥锁保护共享资源

#### 2. WebServer（HTTP 服务器）
- **职责**：提供 HTTP 服务，处理前端请求
- **特性**：
  - 非阻塞 I/O：使用 poll() 系统调用实现事件驱动
  - 多线程：服务器主循环在独立线程中运行
  - 路由表：使用 std::map 存储 URL 到处理器的映射
  - 高性能编辑器 API：基于 TextBuffer 虚拟化渲染

#### 3. FileManager（文件管理器）
- **职责**：封装文件系统操作
- **特性**：
  - 线程安全：使用 std::mutex 保护所有文件操作
  - 文件缓存：LRU 缓存机制（100MB 限制）
  - 文件类型检测：使用 libmagic 库
  - 目录遍历优化：限制读取 2000 个条目

#### 4. TextBuffer（高性能文本缓冲区）
- **职责**：基于 Piece Table 的高性能文本编辑
- **特性**：
  - mmap 映射原始文件，支持大文件快速加载
  - Piece Table 架构：O(log n) 的插入和删除操作
  - 行缓存机制：加速行范围查询
  - 线程安全：使用互斥锁保护所有操作

---

## 📦 目录结构

### 编译前目录结构

```
Mikufy/
├── headers/               # C++ 头文件目录
│   ├── main.h            # 主头文件，包含全局定义和常量
│   ├── file_manager.h    # 文件管理器头文件
│   ├── web_server.h      # Web 服务器头文件
│   ├── window_manager.h  # 窗口管理器头文件
│   └── text_buffer.h     # 文本缓冲区头文件（v2.7 新增）
│
├── src/                   # C++ 源代码目录
│   ├── main.cpp          # 主程序入口
│   ├── file_manager.cpp  # 文件管理器实现
│   ├── web_server.cpp    # Web 服务器实现
│   ├── window_manager.cpp# 窗口管理器实现
│   └── text_buffer.cpp   # 文本缓冲区实现（v2.7 新增）
│
├── web/                   # 前端源代码目录
│   ├── index.html        # HTML 结构文件
│   ├── style.css         # CSS 样式文件
│   ├── app.js            # JavaScript 交互逻辑
│   ├── virtual_editor.js # 虚拟滚动编辑器（v2.7 新增）
│   ├── Mikufy.png        # Logo 图片
│   ├── Background/       # 背景图片目录
│   │   ├── index-1.png ~ index-17.png  # 17 张预设壁纸
│   └── Icons/            # 图标资源目录（SVG 格式）
│       ├── AI-24.svg, C-24.svg, C++-24.svg, ...  # 各种文件类型图标
│
├── build.sh              # 一键编译脚本
├── install.sh            # 桌面应用程序安装脚本
├── mikufy.desktop        # 桌面文件（Linux 应用程序）
├── flake.nix             # NixOS 框架配置
├── package.nix           # NixOS 构建配方
├── LICENSE               # GPL-3.0 许可证
├── Explain.txt           # 依赖列表文件
└── README.md             # 本文件
```

### 编译后目录结构

```
Mikufy/
├── headers/                # C++ 头文件目录（编译时不改变）
├── src/                    # C++ 源代码目录（编译时不改变）
├── web/                    # 前端源代码目录（编译时不改变）
├── build/                  # 编译产物目录（v2.7 新增）
│   ├── main.o              # 主程序目标文件
│   ├── file_manager.o      # 文件管理器目标文件
│   ├── web_server.o        # Web 服务器目标文件
│   ├── window_manager.o    # 窗口管理器目标文件
│   ├── text_buffer.o       # 文本缓冲区目标文件
├── debug/                  # 调试日志目录
│   └── debug.log           # 编译日志
│
├── mikufy                  # 编译生成的可执行文件
├── headers/                # 头文件（未改变）
├── build.sh                # 编译脚本（未改变）
├── install.sh              # 安装脚本（未改变）
├── ...                     # 其他文件（未改变）
```

### 目录变化说明

- **build/**：v2.7 新增的编译产物目录，用于存放中间目标文件（.o）
- **mikufy**：编译生成的可执行文件，位于项目根目录
- 其他目录（headers/、src/、web/）在编译过程中不发生改变

---

## 🔧 依赖安装说明

### 系统要求

- **操作系统**：Fedora43、ArchLinux、Debian13、NixOS
- **桌面环境**：GNOME、Plasma、Xfce 等主流桌面环境
- **显示服务器**：Wayland 或 XWayland
- **编译器**：GCC 7.0+（支持 C++17）

### 依赖包列表

#### 核心依赖

| 包名 | 版本要求 | 用途 |
|------|---------|------|
| gcc-c++ | 7.0+ | C++ 编译器 |
| pkg-config | 最新版 | 编译配置工具 |
| webkitgtk6.0-devel | 6.0+ | WebKitGTK 开发库 |
| gtk4-devel | 4.0+ | GTK4 开发库 |
| glib2-devel | 最新版 | GLib 开发库 |
| file-devel | 最新版 | libmagic 开发库 |
| nlohmann-json-devel | 最新版 | JSON 处理库 |

### 安装命令

#### Fedora43（其他 Fedora 系）

```bash
# 更新系统
sudo dnf update

# 安装编译工具
sudo dnf install gcc-c++ pkgconfig

# 安装 GTK4 和 WebKitGTK 6.0
sudo dnf install webkitgtk6.0-devel gtk4-devel glib2-devel

# 安装文件类型检测库
sudo dnf install file-devel file-libs

# 安装 JSON 处理库
sudo dnf install nlohmann-json-devel
```

#### ArchLinux

```bash
# 更新系统
sudo pacman -Syu

# 安装编译工具
sudo pacman -S pkgconf base-devel

# 安装 GTK4 和 WebKitGTK 6.0
sudo pacman -S webkitgtk6.0 gtk4 glib2-devel

# 安装文件类型检测库
sudo pacman -S file

# 安装 JSON 处理库
sudo pacman -S nlohmann-json
```

#### Debian13（其他 Debian 系）

```bash
# 更新系统
sudo apt update && sudo apt upgrade

# 安装编译工具
sudo apt install pkg-config g++

# 安装 GTK4 和 WebKitGTK 6.0
sudo apt install libwebkitgtk-6.0-dev libgtk-4-dev libglib2.0-dev

# 安装文件类型检测库
sudo apt install file libmagic-dev

# 安装 JSON 处理库
sudo apt install nlohmann-json3-dev
```

#### NixOS

```nix
inputs = {
  mikufy-github = {
    url = "github:MikuTrive/Mikufy";
    inputs.nixpkgs.follows = "nixpkgs";
  };
};

environment.systemPackages = [
  inputs.mikufy-github.packages.${pkgs.stdenv.hostPlatform.system}.mikufy
]
```

### 验证安装

```bash
# 检查 GCC 版本
g++ --version

# 检查 WebKitGTK
pkg-config --modversion webkitgtk-6.0

# 检查 GTK4
pkg-config --modversion gtk4

# 检查 libmagic
pkg-config --modversion libmagic

# 检查 nlohmann_json
pkg-config --modversion nlohmann_json
```

---

## 🚀 编译和运行

### 编译项目

```bash
# 使用一键编译脚本
./build.sh
```

### 运行程序

```bash
# 运行编译好的可执行文件
./mikufy
```

#### 命令行选项

```bash
./mikufy -h
用法: ./mikufy [选项]

选项:
  -h, --help     显示帮助信息
  -v, --version  显示版本信息
  -p, --port     指定 Web 服务器端口（默认: 8080）
```


---

## 🛠️ 代码规范

### C/C++ 代码规范

#### 1. 文件头部注释

每个源文件必须包含详细的文件说明：

```c
/*
 * 文件名 - 简要描述
 *
 * 详细说明文件的功能、设计思路和主要组件。
 *
 * MiraTrive/MikuTrive
 * Author: [Your Name]
 *
 * 本文件遵循 Linux 内核代码风格规范
 */
```

#### 2. 缩进和格式

- 使用 Tab 缩进（8 字符宽度）
- 使用 K&R 括号风格
- 行宽尽量不超过 80 字符

```c
int function_name(int param1, char *param2)
{
	/* 实现 */
	if (condition) {
		do_something();
	} else {
		do_other();
	}
	return 0;
}
```

#### 3. 命名约定

- **常量**：全大写，下划线分隔
  ```c
  #define MAX_SIZE  100
  #define VERSION  "2.7-nova"
  ```

- **变量和函数**：小写，下划线分隔
  ```c
  int file_size;
  void get_file_info(const char *path);
  ```

- **类型定义**：小写，下划线分隔
  ```c
  struct file_info {
      char *name;
      size_t size;
  };
  ```

#### 4. 函数注释

每个公共函数前必须添加详细注释：

```c
/**
 * function_name - 函数简要描述
 *
 * 详细说明函数的功能、实现思路和注意事项。
 *
 * @param param1: 参数1说明
 * @param param2: 参数2说明
 *
 * 返回值: 返回值说明
 *
 * 注意: 任何重要的注意事项
 */
int function_name(int param1, char *param2)
{
	/* 实现 */
	return 0;
}
```

### JavaScript 代码规范

#### 1. 文件头部注释

```javascript
/**
 * 文件名 - 简要描述
 *
 * 详细说明文件的功能和设计。
 * MiraTrive/MikuTrive
 */
```

#### 2. 函数注释

```javascript
/**
 * 函数名 - 简要描述
 *
 * @param {类型} paramName - 参数说明
 * @returns {类型} 返回值说明
 */
function functionName(paramName) {
    // 实现
}
```

#### 3. 命名约定

- 变量和函数：camelCase（小驼峰）
- 常量：UPPER_SNAKE_CASE
- 类/构造函数：PascalCase（大驼峰）

### CSS 代码规范

#### 1. 文件头部注释

```css
/**
 * 文件名 - 简要描述
 *
 * 设计原则和配色方案说明
 * MiraTrive/MikuTrive
 */
```

#### 2. 规则注释

```css
/* ============================================================================
 * 模块名称
 * ============================================================================
 */

/* 类名 - 简要说明 */
.class-name {
    /* 属性值说明 */
    property: value;
}
```

#### 3. 命名约定

- 类名：kebab-case（短横线分隔）
- ID：kebab-case
- 变量：kebab-case

---

## 🤝 贡献指南

### 贡献流程

1. **Fork 项目**：在 GitHub 上 Fork 本仓库
2. **创建分支**：从 `main` 或其他版本分支创建你的特性分支
3. **遵循代码规范**：确保你的代码符合上述规范
4. **测试**：测试你的修改，确保没有引入新的 bug
5. **提交 PR**：提交 Pull Request，详细描述你的修改，并说明修改的Mikufy版本

### 开发建议

1. **保持简洁**：代码应该简洁、清晰、易于理解
2. **添加注释**：为复杂逻辑添加注释，解释"为什么"而非"是什么"
3. **编写测试**：为新功能添加测试（如果有测试框架）
4. **更新文档**：修改代码后更新相关文档
5. **提交前检查**：使用 `./build.sh` 确保代码能正常编译

### 常见任务

#### 添加新功能

1. 在对应的头文件中声明新的类或函数
2. 在源文件中实现
3. 添加详细的注释
4. 如果涉及 API 变更，更新前端代码

#### 修复 Bug

1. 找到 bug 的根本原因
2. 修复代码
3. 添加回归测试（如果有）
4. 提交时描述 bug 的原因和修复方法

#### 更新依赖

1. 修改 `Explain.txt` 或依赖相关文档
2. 确保在不同平台上都能正常工作
3. 更新安装说明

---

## 📧 联系方式

- **开发者群聊**：QQ: 675155390
- **开发者邮箱**：GMAIL: mikulxz08@gmail.com
- **开发者账号**：BiliBili: 3546843237582920

---

## 📝 许可证

本项目采用 GPL-3.0 许可证。详见 [LICENSE](LICENSE) 文件。

---

**Mikufy v2.7-nova** - 让代码编辑更简单

MiraTrive/MikuTrive

对Mikufy的NixOS用户群体支持的贡献者 - luozenan
