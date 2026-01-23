# 检测报告
  - 目前Linux版内出了严重的问题：`无法保存文件内写入的文件只能保存创建的文件空客` 此问题将于2月初得到改善！当前UST为`2026/1/24`
  - Windows版同样问题。
  - 在二月初的新版本当中将会新加入以下功能：`能够查看图片预览、视频、音频、以及在线听歌与mp4解码mp3、性能优化、本地服务存储、代码语法高亮`

# Mikufy Code Editor v1.23

Mikufy 是一款类似 VSCode 的代码编辑器，专为 Linux 平台设计，使用 C++ 和 Web 技术构建。

## 版本信息

- **版本**: v1.23
- **发布日期**: 2026/1/23
- **目标平台**: Linux (支持 X11 和 Wayland)
- **开发语言**: C++, HTML, CSS, JavaScript, TypeScript

## 项目结构

### 未编译前的目录结构

```
MikufyUI/
├── mikufy/                    # 主项目目录
│   ├── src/                   # C++ 源代码
│   │   ├── main.cpp           # 主程序入口
│   │   └── CMakeLists.txt     # CMake 构建配置
│   ├── ui/                    # 前端 UI 资源
│   │   ├── index.html         # 主页面
│   │   ├── style.css          # 样式文件
│   │   ├── app.js             # 前端主逻辑
│   │   ├── tsconfig.json      # TypeScript 配置
│   │   ├── package.json       # Node.js 配置
│   │   ├── Background.png     # 背景图
│   │   ├── Mikufy.png         # Logo
│   │   └── Icons/             # 图标文件
│   ├── build.sh               # 一键编译脚本
│   └── clean.sh               # 一键清理脚本
├── Background.png             # 原始背景图
├── Mikufy.png                 # 原始 Logo
├── Icons/                     # 原始图标目录
└── need.txt                   # 需求文档
```

### 编译后的目录结构

```
MikufyUI/mikufy/
├── build/                     # 构建目录（编译时生成）
│   ├── CMakeFiles/            # CMake 生成的配置文件
│   ├── CMakeCache.txt         # CMake 缓存
│   ├── cmake_install.cmake    # 安装规则
│   └── bin/                   # 可执行文件输出目录
│       └── mikufy             # 最终的可执行文件
├── dist/                      # 安装目录（编译时生成）
│   └── bin/                   # 安装的可执行文件
└── mikufy-release/            # 发布目录（编译时生成）
    ├── mikufy                 # 可执行文件
    ├── run.sh                 # 启动脚本
    ├── ui/                    # UI 资源文件（复制自 ui/）
    │   ├── index.html
    │   ├── style.css
    │   ├── app.js
    │   ├── Background.png
    │   ├── Mikufy.png
    │   └── Icons/
    └── README.md              # 这个文件
```

## 目录说明

### mikufy/src/

C++ 源代码目录，包含主程序的核心逻辑：

- **main.cpp**: 主程序入口文件，负责：
  - 初始化 GTK3 窗口
  - 创建 WebKit webview
  - 处理与前端 JavaScript 的通信
  - 实现文件操作（打开、保存、删除、重命名等）
  - 扫描目录结构

- **CMakeLists.txt**: CMake 构建配置文件，定义：
  - 项目名称和版本
  - C++ 编译标准（C++17）
  - 需要的库（GTK3, WebKitGTK）
  - 编译选项

### mikufy/ui/

前端 UI 资源目录，包含所有用户界面相关文件：

- **app.js**: 前端主逻辑，处理：
  - 文件树渲染和交互
  - 标签页管理
  - 与 C++ 后端的通信
  - 键盘快捷键
- **tsconfig.json**: TypeScript 配置（用于开发）
- **package.json**: Node.js 配置（用于开发）
- **Background.png**: 背景图
- **Mikufy.png**: 应用 Logo
- **Icons/**: 所有图标文件

### Icons/

图标文件目录，包含各种文件类型和操作图标：

**文件类型图标：**
- C-24.png, C++-24.png, C#-24.png, Java-24.png, Python-24.png 等
- HTML-24.png, CSS-24.png, JS.png, Typescript-24.png 等
- Other-24.png: 默认图标

**操作图标：**
- Folder-Open.png: 打开文件夹
- Folder-New.png: 新建文件夹
- File-New.png: 新建文件
- File-Rename.png: 重命名
- Trash.png: 删除
- Spread-out.png: 展开子目录
- Put-away.png: 收起子目录
- Close.png: 关闭标签页
- save.png: 保存文件

### build/

编译时生成的构建目录：

- **CMakeFiles/**: CMake 生成的配置和缓存文件
- **CMakeCache.txt**: CMake 缓存文件
- **cmake_install.cmake**: 安装规则
- **bin/mikufy**: 最终的可执行文件

### mikufy-release/

编译后生成的发布目录，包含所有运行所需的文件：

- **mikufy**: 可执行文件
- **run.sh**: 启动脚本
- **ui/**: UI 资源文件（复制自 ui/）
- **README.md**: 这个文件

## 系统依赖

### 依赖说明

- **cmake**: 构建系统工具
- **gcc/g++** 或 **clang/clang++**: C++ 编译器
- **gtk3**: GTK+ 3 图形界面库
- **webkit2gtk**: WebKitGTK webview 引擎
- **pkg-config**: 库配置工具
- 开发者所用ArchLinux，包名可能不同！请自行查找


### 一键编译

```bash
cd mikufy
./build.sh
```

### 手动编译

```bash
cd mikufy
mkdir build
cd build
cmake ../src
make
```

### 安装Plasma应用桌面程序

```
cd mikufy
./desktop.sh
```


### 编译选项

- **Debug 模式**（用于调试）:
  ```bash
  cmake ../src -DCMAKE_BUILD_TYPE=Debug
  ```

- **Release 模式**（默认，优化性能）:
  ```bash
  cmake ../src -DCMAKE_BUILD_TYPE=Release
  ```

## 调试方法

### 编译 Debug 版本

```bash
cd mikufy
mkdir build-debug
cd build-debug
cmake ../src -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-g -O0"
make
```

### 使用 GDB 调试

```bash
gdb ./build/bin/mikufy
```

常用 GDB 命令：
- `break main` - 在 main 函数设置断点
- `run` - 运行程序
- `next` - 单步执行
- `print variable` - 打印变量值
- `backtrace` - 查看调用栈

### 启用详细日志

在 `mikufy/src/main.cpp` 中添加调试输出：

```cpp
g_printerr("调试信息: %s\n", message);
```

### 前端调试

在终端中直接运行可执行文件查看终端输出信息来进行快速调试

## 运行程序

### 方法 1: 使用启动脚本（推荐）

```bash
cd mikufy/mikufy-release
./run.sh
```

### 方法 2: 直接运行

```bash
cd mikufy/mikufy-release
./mikufy
```

### 方法 3: 运行编译输出

```bash
cd mikufy/build/bin
./mikufy
```

## 配置

### 修改窗口大小

编辑 `mikufy/src/main.cpp`，修改以下常量：

```cpp
const int DEFAULT_WINDOW_WIDTH = 1200;  // 默认窗口宽度
const int DEFAULT_WINDOW_HEIGHT = 800;  // 默认窗口高度
```


### 修改主题颜色

编辑 `mikufy/ui/style.css`，修改颜色值。

### 添加新的文件类型图标

1. 将图标文件放入 `mikufy/ui/Icons/`
2. 在 `mikufy/ui/app.js` 的 `FILE_ICON_MAP` 中添加映射：

```javascript
const FILE_ICON_MAP = {
    '.你的扩展名': 'Icons/你的图标.png',
    // ...
};
```

### 一键清理

```bash
cd mikufy
./clean.sh
```

### 手动清理

```bash
cd mikufy
rm -rf build dist mikufy-release
rm -f CMakeCache.txt CMakeFiles Makefile cmake_install.cmake
```

## 功能特性

- ✔ 打开工作区文件夹
- ✔ 文件树浏览（支持隐藏文件）
- ✔ 文件树折叠/展开
- ✔ 创建文件和文件夹
- ✔ 重命名和删除文件/文件夹
- ✔ 代码编辑
- ✔ 标签页管理
- ✔ 行号显示
- ✔ 自定义背景图

## 键盘快捷键

- `Ctrl+S` - 保存文件
- `Ctrl+N` - 新建文件
- `F2` - 重命名选中的文件/文件夹
- `Delete` - 删除选中的文件/文件夹
- `Enter` - 打开选中的文件
- `Tab` - 插入缩进（4 个空格）

## 开发

### 技术栈

- **后端**: C++
- **前端**: HTML, CSS, JavaScript, TypeScript
- **GUI 框架**: GTK3
- **Web 引擎**: WebKitGTK

### 代码规范

- C++ 代码遵循 Google C++ Style Guide
- JavaScript 代码遵循 ESLint 规范
- 所有函数和关键代码都有详细注释

### 贡献

欢迎提交 Issue 和 Pull Request。

## 许可证

GPL-3.0 License

## 联系方式

如有问题或建议，请联系我[mikulxz08@gmail]。
