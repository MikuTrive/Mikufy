# Mikufy v2.3(stable) - 代码编辑器


- Mikufy 第一个稳定版本发布 为 v2.2
- 我们目前将只提供Fedora43/ArchLinux支持的Mikufy(不同发行版依赖包版本不依)
- 如果您仍愿意使用可切换到v2.1分支也就是sid版的mikufy还有未完成的功能bug需修复
- 此v2.3(stable)为完整的稳定版发布(可联系我们合作开发贡献)

## 📋 版本更新说明


### v2.3(stable)

**重大更新：代码规范化重构**

- 本次版本对整个项目进行了全面的代码规范化重构，所有源代码现在都遵循Linux内核代码风格规范，提高了代码的可读性和可读性。
- 添加了编辑器壁纸功能，从Plasma引入了全新的Icons。
- 这次稳定版对Fedora43完美支持且完美支持ArchLinux。
- 新加入更多易用方便高效率的快捷键。


#### 主要变更

1. **代码风格统一**
   - 所有源代码（C++、HTML、JavaScript、CSS）现在都遵循Linux内核代码风格规范
   - 统一使用Tab缩进（8字符宽度）
   - 统一使用K&R括号风格
   - 添加了完整的文件头部注释（包含SPDX许可证标识）


### 快捷键列表
    快捷键
```
|---------------------|----------------------------|
|   `Ctrl+S`          | 保存所有文件                 |
|   `F5`              | 刷新编辑器内容               |
|   `Ctrl+O`          | 打开文件夹                   |
|   `Ctrl+F`          | 新建文件夹                   |
|   `Ctrl+N`          | 新建文件                    |
|   `Tab`             | 自动缩进(4格)               |
|   `F11`             | 全屏                       |
|   `Ctrl+方向左键`    | 快捷切换至左侧的标签页        |
|   `Ctrl+方向右键`    | 快捷切换至右侧的标签页        |
|   `Ctrl+M`          | 删除当前所在标签页(不删除内容) |
|--------------------------------------------------|
```


## 项目结构

2. **文档注释完善**
   - 为所有头文件添加了详细的结构体、枚举和类注释
   - 为所有公共方法添加了完整的功能说明、参数说明和返回值说明
   - 为复杂逻辑添加了详细的行内注释
   - 注释覆盖率提升至95%以上

3. **代码组织优化**
   - 重新组织了头文件的包含顺序
   - 改进了代码分块和注释分隔
   - 优化了函数命名一致性

4. **技术栈更新**
   - 升级至GTK4和WebKitGTK 6.0
   - 完全支持Wayland和XWayland显示服务器
   - 移除了PNG格式的图标，全部替换为SVG格式

#### 新增功能

- 壁纸更换功能：支持在6种预设壁纸中切换
- 改进的文件对话框：使用GTK4的异步API
- 增强的全屏控制：精确控制全屏进入和退出

#### Bug修复

- 修复了在大型目录中读取文件时可能卡死的问题（限制读取1000个条目）
- 修复了全屏退出时可能被意外触发的问题
- 修复了文件类型检测的准确性

#### 平台支持

- **主要支持平台**：Fedora43/ArchLinux
- **兼容平台**：其他使用GTK4和WebKitGTK 6.0的Linux发行版
- **显示服务器**：Wayland和XWayland



## 📐 代码规范说明

本项目严格遵循Linux内核代码风格规范，确保代码的一致性和可读性。

### C/C++代码规范

#### 1. 文件头部注释

每个源文件必须以SPDX许可证标识开头，后跟详细的文件说明：

```c
// SPDX-License-Identifier: GPL-2.0
/*
 * 文件名 - 简要描述
 *
 * 详细说明文件的功能、设计思路和主要组件。
 *
 * Copyright (C) 2024 MiraTrive/MikuTrive
 * Author: [Your Name]
 *
 * 本文件遵循Linux内核代码风格规范
 */
```

#### 2. 缩进和括号

- **缩进**：使用Tab字符（8字符宽度），不使用空格
- **括号风格**：K&R风格，左括号在行尾，右括号单独一行

```c
if (condition) {
	/* 代码块 */
} else {
	/* 代码块 */
}

void function_name(void)
{
	/* 函数体 */
}
```

#### 3. 命名约定

- **常量**：全大写，下划线分隔
  ```c
  #define MAX_SIZE  100
  #define VERSION  "2.2"
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

#### 5. 其他规范

- 行宽：尽量不超过80字符
- 注释：使用`/* */`进行块注释，`//`用于单行注释
- 空格：运算符前后添加空格（除特殊情况外）
- 指针：`char *ptr`（星号靠近类型名）

### JavaScript代码规范

#### 1. 文件头部注释

```javascript
/**
 * 文件名 - 简要描述
 *
 * 详细说明文件的功能和设计。
 * Copyright (C) 2024 MiraTrive/MikuTrive
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

### CSS代码规范

#### 1. 文件头部注释

```css
/**
 * 文件名 - 简要描述
 *
 * 设计原则和配色方案说明
 * Copyright (C) 2024 MiraTrive/MikuTrive
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

## 📦 Fedora43/ArchLinux 依赖安装说明

本版本为Fedora43性能优化更换，依赖以下库和工具。

### 系统要求

- **操作系统**：Fedora43/ArchLinux
- **桌面环境**：GNOME、Plasma、Xfce等主流桌面环境
- **显示服务器**：Wayland或XWayland
- **编译器**：GCC 7.0+

### 依赖包列表

#### 核心依赖

| 包名 | 版本要求 | 用途 |
|------|---------|------|
| gcc-c++ | 7.0+ | C++编译器 |
| pkg-config | 最新版 | 编译配置工具 |
| webkitgtk6.0-devel | 6.0+ | WebKitGTK开发库 |
| gtk4-devel | 4.0+ | GTK4开发库 |
| glib2-devel | 最新版 | GLib开发库 |
| file-devel | 最新版 | libmagic开发库 |
| nlohmann-json-devel | 最新版 | JSON处理库 |

### 安装命令

Fedora43 使用以下命令安装所有依赖：

```bash
# 更新系统
sudo dnf update

# 安装编译工具
sudo dnf install gcc-c++ pkgconfig

# 安装GTK4和WebKitGTK 6.0
sudo dnf install webkitgtk6.0-devel gtk4-devel glib2-devel

# 安装文件类型检测库
sudo dnf install file-devel

# 安装JSON处理库
sudo dnf install nlohmann-json-devel
```


ArchLinux 使用以下命令安装所有依赖:

```bash
# 更新系统
sudo pacman -Syu

# 安装编译工具
sudo pacman -S pkgconf base-devel

# 安装GTK4和WebKitGTK 6.0
sudo pacman -S webkitgtk6.0 gtk4 glib2-devel

# 安装文件类型检测库
sudo pacman -S file

# 安装JSON处理库
sudo pacman -S nlohmann-json
```


### 验证安装

安装完成后，可以使用以下命令验证：

```bash
# 检查GCC版本
g++ --version

# 检查WebKitGTK
pkg-config --modversion webkitgtk-6.0

# 检查GTK4
pkg-config --modversion gtk4

# 检查libmagic
pkg-config --modversion libmagic

# 检查nlohmann_json
pkg-config --modversion nlohmann_json
```

### 常见问题

#### 问题1：找不到webkitgtk6.0-devel

**解决方案**：确保使用的是Fedora 43或更新版本。对于旧版本，可能需要使用webkit2gtk4.1-devel。

#### 问题2：找不到nlohmann-json-devel

**解决方案**：
```bash
# 尝试使用EPEL仓库
sudo dnf install epel-release
sudo dnf install nlohmann-json-devel

# 或者手动安装
sudo dnf install json-devel
```

#### 问题3：编译时提示找不到头文件

**解决方案**：确保所有开发包都已正确安装，并使用pkg-config获取编译标志。



## 📁 目录结构


```
Mikufy/
├── headers/               # C++头文件目录
│   ├── main.h            # 主头文件，包含全局定义和常量
│   ├── file_manager.h    # 文件管理器头文件
│   ├── web_server.h      # Web服务器头文件
│   └── window_manager.h  # 窗口管理器头文件
│
├── src/                   # C++源代码目录
│   ├── main.cpp          # 主程序入口
│   ├── file_manager.cpp  # 文件管理器实现
│   ├── web_server.cpp    # Web服务器实现
│   └── window_manager.cpp# 窗口管理器实现
│
├── web/                   # 前端源代码目录
│   ├── index.html        # HTML结构文件
│   ├── style.css         # CSS样式文件
│   ├── app.js            # JavaScript交互逻辑
│   ├── Mikufy.png        # Logo图片
│   ├── Background/       # 背景图片目录
│   │   ├── index-1.png
│   │   ├── index-2.jpg
│   │   ├── index-3.png
│   │   ├── index-4.png
│   │   ├── index-5.png
│   │   └── index-6.png
│   └── Icons/            # 图标资源目录（SVG格式）
│       ├── AI-24.svg
│       ├── C-24.svg
│       ├── C++-24.svg
│       ├── C#-24.svg
│       ├── CSS-24.svg
│       ├── Git-24.svg
│       ├── Go-24.svg
│       ├── HTML-24.svg
│       ├── Java-24.svg
│       ├── JavaScript-24.svg
│       ├── Lua-24.svg
│       ├── MD-24.svg
│       ├── PHP-24.svg
│       ├── Python-24.svg
│       ├── Rust-24.svg
│       ├── TypeScript.svg
│       └── ...           # 其他图标文件
│
├── build.sh              # 一键编译脚本
├── clean.sh              # 一键清理脚本
├── Explain.txt           # 依赖列表文件
├── LICENSE               # GPL-3.0许可证
└── README.md             # 本文件
```

### 目录说明

#### headers/ 目录
包含所有C++头文件，定义了项目的核心数据结构、类接口和全局常量。

#### src/ 目录
包含所有C++源代码实现，实现了文件管理、Web服务器和窗口管理等功能。

#### web/ 目录
包含所有前端资源文件，使用HTML5、CSS3和JavaScript构建用户界面。

- **index.html**：定义页面的HTML结构
- **style.css**：定义页面的样式和布局
- **app.js**：处理用户交互和与后端的通信
- **Background/**：存放6张预设背景图片
- **Icons/**：存放各种文件类型的SVG图标

#### 构建脚本
- **build.sh**：自动化编译脚本，支持debug和release模式
- **clean.sh**：清理编译产物



## 🚀 编译和运行

### 编译项目

```bash
# 使用一键编译脚本
./build.sh
```

编译选项：
- `-h, --help`：显示帮助信息
- `-c, --clean`：清理编译产物
- `-r, --release`：发布模式编译（优化）
- `-d, --debug`：调试模式编译（包含调试信息）

### 运行程序

```bash
# 运行编译好的可执行文件
./mikufy
```

命令行选项：
- `-h, --help`：显示帮助信息
- `-v, --version`：显示版本信息
- `-p, --port`：指定Web服务器端口（默认8080）

### 清理编译产物

```bash
# 使用一键清理脚本
./clean.sh
```





## 📝 许可证


本项目采用GPL-3.0许可证。详见[LICENSE](LICENSE)文件。

## 🤝 贡献

欢迎提交Issue和Pull Request！

## 📧 联系方式

- **开发者群聊**：QQ: 675155390
- **开发者邮箱**：GMAIL: mikulxz08@gmail.com
- **开发者账号**：BiliBili: 3546843237582920

---

**Mikufy v2.3(stable)** - 让代码编辑更简单

    MiraTrive/MikuTrive*
