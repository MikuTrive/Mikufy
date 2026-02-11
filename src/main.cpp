/*
 * Mikufy v2.11-nova - 主程序入口
 *
 * 这是Mikufy代码编辑器的主程序文件，包含应用程序的入口点main()函数。
 * 该文件负责：
 * - 解析命令行参数
 * - 初始化各个子系统（文件管理器、Web服务器、窗口管理器）
 * - 设置信号处理器（SIGINT、SIGTERM）
 * - 管理应用程序的生命周期
 *
 * MiraTrive/MikuTrive
 * Author: [Your Name]
 *
 * 本文件遵循Linux内核代码风格规范
 */

/*
 * ============================================================================
 * 头文件包含
 * ============================================================================
 */

#include "../headers/main.h"
#include "../headers/file_manager.h"
#include "../headers/web_server.h"
#include "../headers/window_manager.h"

#include <iostream>		/* std::cout, std::cerr */
#include <csignal>		/* signal(), SIGINT, SIGTERM */
#include <cstdlib>		/* std::atoi(), exit() */
#include <limits.h>		/* PATH_MAX */
#include <unistd.h>		/* readlink(), getcwd() */
#include <dirent.h>		/* opendir(), closedir() */
#include <cstring>		/* getcwd() */
#include <format>		/* C++23 std::format */

/*
 * ============================================================================
 * 全局变量声明
 * ============================================================================
 */

/*
 * 全局对象指针
 * 这些指针在整个应用程序生命周期中有效，用于跨模块访问核心对象
 */
static FileManager *g_file_manager = NULL;	/* 文件管理器指针 */
static WebServer *g_web_server = NULL;		/* Web服务器指针 */
static WindowManager *g_window_manager = NULL;	/* 窗口管理器指针 */

/*
 * ============================================================================
 * 信号处理函数
 * ============================================================================
 */

/**
 * signal_handler - 信号处理函数
 *
 * 处理SIGINT（Ctrl+C）和SIGTERM信号，实现应用程序的优雅关闭。
 * 该函数会停止Web服务器、关闭窗口，然后退出程序。
 *
 * 设计要点:
 * - 使用静态变量防止重复处理
 * - 首次调用时优雅关闭，再次调用时强制退出
 * - 使用_exit()而不是exit()以避免GTK清理阻塞
 *
 * @signal: 信号编号（SIGINT或SIGTERM）
 */
static void signal_handler(int signal)
{
	std::cout << std::format("\n收到信号 {}，正在关闭...", signal)
		  << std::endl;

	/* 静态变量防止重复处理 */
	static bool terminating = false;
	if (terminating) {
		/*
		 * 如果已经在终止中，强制退出
		 * 这可能发生在用户连续按下Ctrl+C的情况下
		 */
		_exit(1);
	}
	terminating = true;

	/* 停止Web服务器 */
	if (g_web_server)
		g_web_server->stop();

	/* 关闭窗口 */
	if (g_window_manager)
		g_window_manager->close();

	/* 立即退出，不等待GTK清理 */
	_exit(0);
}

/*
 * ============================================================================
 * 辅助函数
 * ============================================================================
 */

/**
 * print_welcome - 打印欢迎信息
 *
 * 显示应用程序的名称和版本信息，提供友好的启动提示。
 */
static void print_welcome(void)
{
	std::cout << "========================================" << std::endl;
	std::cout << "  Mikufy v2.11-nova - Code Editor" << std::endl;
	std::cout << "========================================" << std::endl;
}

/**
 * print_usage - 打印使用帮助
 *
 * 显示命令行参数的使用说明，包括所有可用的选项及其用途。
 *
 * @program_name: 程序名称（argv[0]）
 */
static void print_usage(const char *program_name)
{
	std::cout << "用法: " << program_name << " [选项]" << std::endl;
	std::cout << std::endl;
	std::cout << "选项:" << std::endl;
	std::cout << "  -h, --help     显示帮助信息" << std::endl;
	std::cout << "  -v, --version  显示版本信息" << std::endl;
	std::cout << "  -p, --port     指定Web服务器端口 (默认: 8080)"
		  << std::endl;
	std::cout << std::endl;
}

/**
 * print_version - 打印版本信息
 *
 * 显示应用程序的名称、版本号和构建日期。
 */
/*
 * get_executable_path - 获取可执行文件所在目录的绝对路径
 *
 * 通过读取 /proc/self/exe 获取可执行文件的完整路径，然后
 * 提取目录部分。这对于确定程序安装后的资源文件位置非常重要。
 *
 * 返回值: 可执行文件所在目录的绝对路径，失败返回空字符串
 */
static std::string get_executable_path(void)
{
	char exe_path[PATH_MAX];
	ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);

	if (len == -1) {
		return "";
	}

	exe_path[len] = '\0';

	/* 提取目录部分 */
	std::string path(exe_path);
	size_t pos = path.find_last_of('/');
	if (pos != std::string::npos) {
		return path.substr(0, pos);
	}

	return "";
}

static void print_version(void)
{
	std::cout << MIKUFY_NAME << " v" << MIKUFY_VERSION << std::endl;
	std::cout << "构建日期: " << __DATE__ << " " << __TIME__ << std::endl;
}

/*
 * ============================================================================
 * 主函数
 * ============================================================================
 */

/**
 * main - 主函数
 *
 * 应用程序的入口点，负责初始化所有子系统并启动应用程序。
 * 该函数执行以下步骤：
 * 1. 解析命令行参数
 * 2. 注册信号处理器
 * 3. 创建并初始化文件管理器
 * 4. 创建并启动Web服务器
 * 5. 创建并初始化窗口管理器
 * 6. 加载前端页面
 * 7. 显示窗口
 * 8. 运行GTK主循环
 * 9. 清理资源并退出
 *
 * 命令行参数:
 * - -h, --help: 显示帮助信息并退出
 * - -v, --version: 显示版本信息并退出
 * - -p, --port: 指定Web服务器端口
 *
 * @argc: 命令行参数数量
 * @argv: 命令行参数数组
 *
 * 返回值: 成功返回0，失败返回非零值
 */
int main(int argc, char *argv[])
{
	int port = WEB_SERVER_PORT;	/* Web服务器端口 */

	/*
	 * ====================================================================
	 * 第一步：解析命令行参数
	 * ====================================================================
	 */
	for (int i = 1; i < argc; ++i) {
		std::string arg = argv[i];

		/* 显示帮助信息 */
		if (arg == "-h" || arg == "--help") {
			print_usage(argv[0]);
			return 0;
		}
		/* 显示版本信息 */
		else if (arg == "-v" || arg == "--version") {
			print_version();
			return 0;
		}
		/* 设置端口 */
		else if (arg == "-p" || arg == "--port") {
			/* 检查是否有端口号参数 */
			if (i + 1 < argc) {
				port = std::atoi(argv[++i]);
				/* 验证端口号范围 */
				if (port <= 0 || port > 65535) {
					std::cerr << "错误: 无效的端口号" << std::endl;
					return 1;
				}
			} else {
				std::cerr << "错误: 缺少端口号参数" << std::endl;
				return 1;
			}
		}
		/* 未知参数 */
		else {
			std::cerr << std::format("错误: 未知选项 {}", arg) << std::endl;
			print_usage(argv[0]);
			return 1;
		}
	}

	/*
	 * ====================================================================
	 * 第二步：打印欢迎信息
	 * ====================================================================
	 */
	print_welcome();

	/*
	 * ====================================================================
	 * 第三步：注册信号处理器
	 * ====================================================================
	 */
	signal(SIGINT, signal_handler);	/* 处理Ctrl+C */
	signal(SIGTERM, signal_handler);	/* 处理kill命令 */

	/*
	 * ====================================================================
	 * 第四步：初始化各个子系统
	 * ====================================================================
	 */

	try {
		/*
		 * ----------------------------------------------------------------
		 * 1. 创建文件管理器
		 * ----------------------------------------------------------------
		 * FileManager负责所有文件系统操作，包括文件读写、目录遍历等
		 */
		std::cout << "正在初始化文件管理器..." << std::endl;
		g_file_manager = new FileManager();
		if (!g_file_manager) {
			std::cerr << "错误: 无法创建文件管理器"
				  << std::endl;
			return 1;
		}
		std::cout << "文件管理器初始化完成" << std::endl;

		/*
		 * ----------------------------------------------------------------
		 * 2. 创建Web服务器
		 * ----------------------------------------------------------------
		 * WebServer提供HTTP服务，处理前端请求
		 */
		std::cout << std::format("正在启动Web服务器 (端口: {})...", port)
			  << std::endl;
		g_web_server = new WebServer(g_file_manager);
		if (!g_web_server) {
			std::cerr << "错误: 无法创建Web服务器" << std::endl;
			delete g_file_manager;
			return 1;
		}

		/*
		 * 设置web资源根目录
		 * 使用可执行文件所在目录
		 */
		std::string exec_dir = get_executable_path();
		std::string web_root;

		if (!exec_dir.empty()) {
			web_root = exec_dir + "/web";
			std::cout << "可执行文件目录: " << exec_dir << std::endl;
			std::cout << "Web资源目录: " << web_root << std::endl;

			/* 验证目录是否存在 */
			DIR *dir = opendir(web_root.c_str());
			if (dir) {
				closedir(dir);
				std::cout << "Web资源目录验证成功" << std::endl;
			} else {
				std::cout << "警告: Web资源目录不存在: " << web_root << std::endl;
				/* 后备：使用当前工作目录 */
				char cwd[PATH_MAX];
				if (getcwd(cwd, sizeof(cwd)) != NULL) {
					web_root = std::string(cwd) + "/web";
					std::cout << "使用当前工作目录: " << web_root << std::endl;
				} else {
					web_root = "web";
					std::cout << "使用相对路径" << std::endl;
				}
			}
		} else {
			std::cout << "警告: 无法获取可执行文件路径" << std::endl;
			/* 使用当前工作目录 */
			char cwd[PATH_MAX];
			if (getcwd(cwd, sizeof(cwd)) != NULL) {
				web_root = std::string(cwd) + "/web";
				std::cout << "使用当前工作目录: " << web_root << std::endl;
			} else {
				web_root = "web";
				std::cout << "使用相对路径" << std::endl;
			}
		}

		g_web_server->set_web_root_path(web_root);

		/*
		 * 设置打开文件夹对话框的回调函数
		 * 当前端请求打开文件夹时，通过此回调调用WindowManager
		 */
		g_web_server->set_open_folder_callback([]() -> std::string {
			try {
				if (g_window_manager)
					return g_window_manager->
						get_current_working_directory();
			} catch (const std::exception &e) {
				std::cerr << "回调函数异常: " << e.what()
					  << std::endl;
			} catch (...) {
				std::cerr << "回调函数发生未知异常"
					  << std::endl;
			}
			return "";
		});

		/*
		 * 启动Web服务器
		 * 服务器将在独立线程中运行，处理HTTP请求
		 */
		if (!g_web_server->start(port)) {
			std::cerr << "错误: 无法启动Web服务器" << std::endl;
			delete g_web_server;
			delete g_file_manager;
			return 1;
		}
		std::cout << "Web服务器启动成功" << std::endl;

		/*
		 * ----------------------------------------------------------------
		 * 3. 创建窗口管理器
		 * ----------------------------------------------------------------
		 * WindowManager负责GTK窗口和WebView的管理
		 */
		std::cout << "正在初始化窗口..." << std::endl;
		g_window_manager = new WindowManager(g_web_server);
		if (!g_window_manager) {
			std::cerr << "错误: 无法创建窗口管理器" << std::endl;
			g_web_server->stop();
			delete g_web_server;
			delete g_file_manager;
			return 1;
		}

		/*
		 * 初始化窗口
		 * 创建GTK窗口和WebView组件
		 */
		if (!g_window_manager->init()) {
			std::cerr << "错误: 无法初始化窗口" << std::endl;
			delete g_window_manager;
			g_web_server->stop();
			delete g_web_server;
			delete g_file_manager;
			return 1;
		}
		std::cout << "窗口初始化完成" << std::endl;

		/*
		 * ----------------------------------------------------------------
		 * 4. 加载前端页面
		 * ----------------------------------------------------------------
		 * 在WebView中加载前端HTML页面
		 */
		std::cout << "正在加载前端页面..." << std::endl;
		g_window_manager->load_frontend_page();
		std::cout << "前端页面加载完成" << std::endl;

		/*
		 * ----------------------------------------------------------------
		 * 5. 显示窗口
		 * ----------------------------------------------------------------
		 * 将窗口显示到屏幕上
		 */
		std::cout << "正在显示窗口..." << std::endl;
		g_window_manager->show();
		std::cout << "Mikufy 已启动!" << std::endl;
		std::cout << std::endl;

		/*
		 * ----------------------------------------------------------------
		 * 6. 运行GTK主循环
		 * ----------------------------------------------------------------
		 * 启动GTK主事件循环，开始处理窗口事件
		 * 该方法会阻塞直到窗口关闭
		 */
		g_window_manager->run();

	} catch (const std::exception &e) {
		/* 捕获并处理异常 */
		std::cerr << "错误: " << e.what() << std::endl;

		/* 清理资源 */
		delete g_window_manager;
		delete g_web_server;
		delete g_file_manager;

		return 1;
	}

	/*
	 * ====================================================================
	 * 第五步：清理资源并退出
	 * ====================================================================
	 */

	/* 释放所有对象 */
	delete g_window_manager;
	delete g_web_server;
	delete g_file_manager;

	std::cout << "Mikufy 已关闭" << std::endl;

	return 0;
}
