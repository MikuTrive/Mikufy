# Mikufy v2.1

## 项目简介

Mikufy 是一个美观、轻量级、高性能的代码编辑器，专为 Linux 系统设计。采用 C++ 与 Web 集成的架构，兼容 Wayland 和 XWayland 显示服务器，提供现代化的编辑体验。

## 版本信息

- **当前版本**: v2.1
- **平台**: Linux (Wayland/XWayland)
- **开发语言**: C++ (C++17标准)
- **前端技术**: HTML5, CSS3, JavaScript
- **编译器**: GCC 7.0+

## 主要功能

### 核心功能
- 📁 **文件树导航** - 支持子目录展开、折叠、返回上级目录
- 📑 **多标签页编辑** - 支持同时打开多个文件，快速切换
- 🎨 **语法高亮** - 支持 C/C++、JavaScript、TypeScript、Python 等多种语言
- 💾 **自动保存** - 支持手动保存（Ctrl+S）
- 🖼️ **媒体预览** - 支持查看图片（.png, .jpg, .jpeg）、视频（.mp4）、音频（.mp3, .wav）
- 🔄 **快捷键支持** - 丰富的键盘快捷键，提高编辑效率
- 📝 **文件操作** - 支持新建、删除、重命名文件和文件夹
- 🔍 **右键菜单** - 便捷的右键菜单操作

### 快捷键列表
| 快捷键 | 功能 |
|--------|------|
| `Ctrl+S` | 保存所有文件 |
| `F5` | 刷新编辑器内容 |
| `Ctrl+O` | 打开文件夹 |
| `Ctrl+F` | 新建文件夹 |
| `Ctrl+N` | 新建文件 |
| `Tab` | 自动缩进（4格） |

## 项目结构

```
Mikufy/
├── src/                   # C++ 源代码目录
│   ├── main.cpp           # 主程序入口
│   ├── file_manager.cpp   # 文件管理器实现
│   ├── web_server.cpp     # Web服务器实现
│   └── window_manager.cpp # 窗口管理器实现
├── headers/               # C++ 头文件目录
│   ├── main.h             # 主头文件，包含全局定义
│   ├── file_manager.h     # 文件管理器头文件
│   ├── web_server.h       # Web服务器头文件
│   └── window_manager.h   # 窗口管理器头文件
├── web/                   # 前端源代码目录
│   ├── index.html         # HTML结构文件
│   ├── style.css          # CSS样式文件
│   ├── app.js             # JavaScript交互逻辑
│   ├── Icons/             # 图标资源目录
│   ├── Background.png     # 背景图片
│   └── Mikufy.png         # Logo图片
├── build.sh               # 一键编译脚本
├── clean.sh               # 一键清理脚本
├── Explain.txt            # 依赖列表文件
└── README.md              # 本文件
```


## 目录说明

### headers/ 目录（头文件）

头文件目录包含所有 C++ 源代码所需的头文件定义：

#### `main.h`
- **作用**: 主头文件，包含项目级别的全局定义、常量和宏
- **内容**:
  - 全局配置常量
  - 共享的数据结构定义
  - 外部变量声明
- **维护方法**: 
  - 当需要添加新的全局常量或配置时，在此文件中添加
  - 确保与其他头文件没有重复定义

#### `file_manager.h`
- **作用**: 文件管理器头文件，定义文件操作的接口
- **内容**:
  - `FileManager` 类声明
  - 文件信息结构体定义（FileInfo）
  - 文件操作方法声明（读取、写入、删除、重命名等）
- **维护方法**:
  - 添加新的文件操作方法时，先在此声明
  - 修改 FileInfo 结构时，确保与 file_manager.cpp 实现保持一致
  - 添加新的文件类型支持时，更新相关方法签名

#### `web_server.h`
- **作用**: Web服务器头文件，定义HTTP服务器接口
- **内容**:
  - `WebServer` 类声明
  - HTTP请求/响应结构体定义
  - API路由处理器声明
  - 工具函数声明
- **维护方法**:
  - 添加新的 API 端点时，先在此声明处理器方法
  - 修改请求/响应结构时，确保与 web_server.cpp 实现一致
  - 新增路由时，在 initializeRoutes 方法中注册

#### `window_manager.h`
- **作用**: 窗口管理器头文件，定义GTK窗口接口
- **内容**:
  - `WindowManager` 类声明
  - 窗口初始化和事件处理方法声明
  - 前端交互回调函数类型定义
- **维护方法**:
  - 添加新的窗口事件处理时，先在此声明
  - 修改回调函数类型时，确保所有调用方同步更新
  - 新增前端交互功能时，定义对应的回调类型

**头文件维护注意事项**:
1. 使用头文件保护（`#ifndef`）防止重复包含
2. 保持头文件接口简洁，实现细节放在 .cpp 文件中
3. 添加新方法时，同步更新 .cpp 文件中的实现
4. 使用 `extern` 声明跨文件使用的全局变量
5. 保持代码注释完整，说明每个类和方法的作用

### src/ 目录（源代码）

源代码目录包含所有 C++ 源代码实现文件：

#### `main.cpp`
- **作用**: 主程序入口，负责初始化和启动应用程序
- **内容**:
  - 主函数 `main()`
  - 信号处理函数
  - 全局对象初始化
  - 应用程序生命周期管理

#### `file_manager.cpp`
- **作用**: 文件管理器实现，处理所有文件系统操作
- **内容**:
  - 文件读写操作
- **目录遍历和管理**
- **文件类型识别（使用 libmagic）**
- **文件和目录的创建、删除、重命名**

#### `web_server.cpp`
- **作用**: Web服务器实现，提供HTTP API 接口
- **内容**:
  - HTTP服务器启动和监听
- 请求解析和路由分发
- API 端点实现（文件读写、目录操作等）
- 静态文件服务**

#### `window_manager.cpp`
- **作用**: 窗口管理器实现，处理GTK窗口和前端交互
- **内容**:
  - GTK窗口创建和配置
- WebKit WebView 嵌入
- 文件对话框处理
- 前端回调函数实现

**源代码维护注意事项**:
1. 保持代码风格一致，遵循项目约定
2. 每个函数添加完整注释，说明作用、参数、返回值
3. 避免过长的函数，适当拆分以提高可维护性
4. 使用 RAII 原则管理资源，防止内存泄漏
5. 添加错误处理和日志输出

### web/ 目录（前端代码）

前端代码目录包含所有前端资源文件：

#### `index.html`
- **作用**: HTML结构文件，定义编辑器界面布局
- **内容**:
  - 整体页面布局结构
  - 左侧面板（文件树、工具栏、路径显示）
  - 标签页区域
  - 代码编辑区域
  - 右键菜单
  - 新建文件/文件夹对话框

#### `style.css`
- **作用**: CSS样式文件，定义界面样式和视觉效果
- **内容**:
  - 整体布局样式
  - 组件样式（按钮、对话框、滚动条等）
  - 颜色主题配置
  - 响应式布局

#### `app.js`
- **作用**: JavaScript交互逻辑，处理用户交互和API调用
- **内容**:
  - DOM元素管理
  - 状态管理（文件树、标签页、缓存等）
  - API调用封装
  - 事件监听和处理
  - 语法高亮实现
  - 文件操作逻辑

#### `Icons/` 目录
- **作用**: 图标资源目录，包含所有界面图标
- **内容**: 各种文件类型图标（如 C-24.png, Python-24.png 等）

#### `Background.png`
- **作用**: 背景图片，编辑器的主背景图

#### `Mikufy.png`
- **作用**: Logo图片，编辑器的品牌标识

**前端代码维护注意事项**:
1. 保持代码结构清晰，模块化设计
2. 使用语义化的 HTML 和 CSS
3. JavaScript 函数添加注释，说明功能和使用方法
4. 图标文件使用时通过相对路径引用（`../Icons/xxx.png`）
5. 新增功能时，同步更新 HTML、CSS 和 JavaScript

## 依赖安装

### Ubuntu 25.10

```bash
sudo apt update
sudo apt install -y g++ pkg-config libwebkit2gtk-4.1-dev libgtk-3-dev libglib2.0-dev libmagic-dev nlohmann-json3-dev
```

### Debian / Ubuntu（通用）

```bash
sudo apt update
sudo apt install -y g++ pkg-config libwebkit2gtk-4.1-dev libgtk-3-dev libglib2.0-dev libmagic-dev nlohmann-json3-dev
```

### Arch Linux

```bash
sudo pacman -S gcc pkgconf webkit2gtk-4.1 gtk3 glib2 libmagic nlohmann-json
```

### Fedora / RHEL / CentOS

```bash
sudo dnf install gcc-c++ pkgconfig webkit2gtk4.1-devel gtk3-devel glib2-devel libmagic-devel nlohmann-json-devel
```

### OpenSUSE

```bash
sudo zypper install gcc-c++ pkgconfig webkit2gtk-4.1-devel gtk3-devel glib2-devel libmagic-devel nlohmann_json-devel
```

## 编译和运行

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

### 清理编译产物

```bash
# 使用一键清理脚本
./clean.sh
```

### 安装建议声明

- 如果只想正常使用此Mikufy编辑器
- 可以创建一个Desktop文件到~/Desktop或~/.local/share/applications或/usr/share/applications当中
- 如果要自行开发新功能与前后端输出信息, 则直接在终端当前运行此可执行二进制文件
- 如果不喜欢此编辑器的背景图请到源代码根目录下的web子目录内
- 将Background.png改为喜欢的并命名为Background.png

## 编译选项说明

- **标准**: `-std=c++17` (C++17标准)
- **优化级别**: `-O2` (优化编译)
- **警告级别**: `-Wall -Wextra -Wpedantic` (显示所有警告)
- **链接库**: WebKitGTK, GTK+, GLib, libmagic 等

## 系统要求

- **操作系统**: Linux
- **显示服务器**: Wayland 或  XWayland
- **桌面环境**: Gnome, Plasma, Xfce, Niri, Hyprland 等主流桌面环境
- **最低 GCC 版本**: 7.0
- **RAM**: 建议 512MB 以上
- **磁盘空间**: 建议 100MB 以上

## 功能特性

### 支持的文件类型

#### 文本文件
- C/C++ (.c, .cpp, .h, .hpp)
- JavaScript / TypeScript (.js, .ts, .jsx, .tsx)
- Python (.py)
- Java (.java)
- Shell脚本 (.sh)
- HTML/CSS/JavaScript (.html, .css, .js)
- JSON/XML (.json, .xml)
- Markdown (.md)
- 其他文本文件

#### 媒体文件
- 图片 (.png, .jpg, .jpeg)
- 视频 (.mp4)
- 音频 (.mp3, .wav)

### 文件图标映射

程序根据文件扩展名自动显示对应的图标，支持的文件类型包括：
- AI 文件 (.ai)
- C/C++ 文件 (.c, .cpp, .h, .hpp)
- C# 文件 (.cs)
- Go 文件 (.go)
- Java 文件 (.java)
- JavaScript/TypeScript (.js, .ts)
- Python 文件 (.py)
- Rust 文件 (.rs)
- Kotlin 文件 (.kt)
- Lua 文件 (.lua)
- Shell 脚本 (.sh)
- HTML/CSS/JavaScript (.html, .css, .js)
- JSON/XML (.json, .xml)
- Markdown (.md)
- 图片文件 (.png, .jpg, .jpeg)
- 视频文件 (.mp4)
- 音频文件 (.mp3, .wav)
- 配置文件 (.conf, .config, .theme, .d)
- Git 相关文件 (.git, .gitattributes, .gitignore)
- 压缩包 (.zip, .tar, .gz 等)
- 其他文件

## 开发和维护

### 添加新功能

1. **C++ 后端功能**:
   - 在 `headers/` 中添加或修改头文件声明
   - 在 `src/` 中实现对应功能
   - 在 `web_server.cpp` 中注册新的 API 路由
   - 在 `window_manager.cpp` 中添加必要的回调处理

2. **前端功能**:
   - 在 `web/app.js` 中添加或修改 JavaScript 逻辑
   - 在 `web/style.css` 中添加或修改样式
   - 在 `web/index.html` 中修改 HTML 结构

3. **新增图标**:
   - 将图标文件放入 `web/Icons/` 目录
   - 在 `web/app.js` 的 `AppState.iconMap` 中添加图标映射

### 代码规范

- 使用 C++17 标准
- 遵循 Google C++ Style Guide
- 添加完整的函数注释
- 使用 RAII 原则管理资源
- 保持代码缩进一致（4空格）
- 避免使用魔法数字，使用命名常量

## 常见问题

### Quest: 编译时提示找不到头文件
A: 确保已安装所有依赖，特别是 `libwebkit2gtk-4.1-dev` 和 `nlohmann-json3-dev`

### Quest: 运行时提示找不到 libwebkit2gtk-4.1.so
A: 安装 WebKitGTK 运行时库：`sudo apt install libwebkit2gtk-4.1-43`

### Quest: 语法高亮不生效
A: 确保文件扩展名正确，程序通过扩展名判断文件类型

### Quest: 无法打开某些文件
A: 检查文件权限，确保程序有读取权限

### Quest: 媒体文件无法播放
A: 确保系统已安装对应的编解码器

## 许可证

GPL-3.0 License

## 贡献

欢迎提交 Issue 和 Pull Request！

## 联系方式

开发者群聊: [QQ: 675155390]

开发者邮箱: [GMAIL: mikulxz08@gmail.com]

开发者账号: [BiliBili: 3546843237582920]

---

**Mikufy v2.1** - 让代码编辑更简单
