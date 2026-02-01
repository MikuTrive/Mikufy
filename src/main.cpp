/* Mikufy v2.2(stable) - 主程序入口
 * 代码编辑器应用程序
 *
 * 版本: v2.2
 * 平台: Fedora43 Linux (支持Wayland和XWayland)
 * 作者: MiraTrive/MikuTrive
 */

#include "../headers/main.h"
#include "../headers/file_manager.h"
#include "../headers/web_server.h"
#include "../headers/window_manager.h"

#include <iostream>
#include <csignal>
#include <cstdlib>

/* 全局对象指针 */
static FileManager *g_file_manager = nullptr;
static WebServer *g_web_server = nullptr;
static WindowManager *g_window_manager = nullptr;

/* 信号处理函数
 * 处理SIGINT和SIGTERM信号，优雅地关闭应用程序
 */
static void signal_handler(int signal)
{
	std::cout << "\n收到信号 " << signal << "，正在关闭..." << std::endl;

	static bool terminating = false;
	if (terminating) {
		/* 如果已经在终止中，强制退出 */
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

/* 打印欢迎信息 */
static void print_welcome(void)
{
	std::cout << "========================================" << std::endl;
	std::cout << "  Mikufy v2.2(stable) - Code Editor" << std::endl;
	std::cout << "========================================" << std::endl;
}

/* 打印使用帮助 */
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

/* 打印版本信息 */
static void print_version(void)
{
	std::cout << MIKUFY_NAME << " v" << MIKUFY_VERSION << std::endl;
	std::cout << "构建日期: " << __DATE__ << " " << __TIME__ << std::endl;
}

/* 主函数
 * 程序入口点
 */
int main(int argc, char *argv[])
{
	/* 解析命令行参数 */
	int port = WEB_SERVER_PORT;

	for (int i = 1; i < argc; ++i) {
		std::string arg = argv[i];

		if (arg == "-h" || arg == "--help") {
			print_usage(argv[0]);
			return 0;
		} else if (arg == "-v" || arg == "--version") {
			print_version();
			return 0;
		} else if (arg == "-p" || arg == "--port") {
			if (i + 1 < argc) {
				port = std::atoi(argv[++i]);
				if (port <= 0 || port > 65535) {
					std::cerr << "错误: 无效的端口号"
						  << std::endl;
					return 1;
				}
			} else {
				std::cerr << "错误: 缺少端口号参数"
					  << std::endl;
				return 1;
			}
		} else {
			std::cerr << "错误: 未知选项 " << arg << std::endl;
			print_usage(argv[0]);
			return 1;
		}
	}

	/* 打印欢迎信息 */
	print_welcome();

	/* 注册信号处理器 */
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

	try {
		/* 1. 创建文件管理器 */
		std::cout << "正在初始化文件管理器..." << std::endl;
		g_file_manager = new FileManager();
		if (!g_file_manager) {
			std::cerr << "错误: 无法创建文件管理器"
				  << std::endl;
			return 1;
		}
		std::cout << "文件管理器初始化完成" << std::endl;

		/* 2. 创建Web服务器 */
		std::cout << "正在启动Web服务器 (端口: " << port << ")..."
			  << std::endl;
		g_web_server = new WebServer(g_file_manager);
		if (!g_web_server) {
			std::cerr << "错误: 无法创建Web服务器" << std::endl;
			delete g_file_manager;
			return 1;
		}

		/* 设置打开文件夹对话框的回调函数 */
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

		/* 启动Web服务器 */
		if (!g_web_server->start(port)) {
			std::cerr << "错误: 无法启动Web服务器" << std::endl;
			delete g_web_server;
			delete g_file_manager;
			return 1;
		}
		std::cout << "Web服务器启动成功" << std::endl;

		/* 3. 创建窗口管理器 */
		std::cout << "正在初始化窗口..." << std::endl;
		g_window_manager = new WindowManager(g_web_server);
		if (!g_window_manager) {
			std::cerr << "错误: 无法创建窗口管理器" << std::endl;
			g_web_server->stop();
			delete g_web_server;
			delete g_file_manager;
			return 1;
		}

		/* 初始化窗口 */
		if (!g_window_manager->init()) {
			std::cerr << "错误: 无法初始化窗口" << std::endl;
			delete g_window_manager;
			g_web_server->stop();
			delete g_web_server;
			delete g_file_manager;
			return 1;
		}
		std::cout << "窗口初始化完成" << std::endl;

		/* 4. 加载前端页面 */
		std::cout << "正在加载前端页面..." << std::endl;
		g_window_manager->load_frontend_page();
		std::cout << "前端页面加载完成" << std::endl;

		/* 5. 显示窗口 */
		std::cout << "正在显示窗口..." << std::endl;
		g_window_manager->show();
		std::cout << "Mikufy 已启动!" << std::endl;
		std::cout << std::endl;

		/* 6. 运行GTK主循环 */
		g_window_manager->run();

	} catch (const std::exception &e) {
		std::cerr << "错误: " << e.what() << std::endl;

		/* 清理资源 */
		delete g_window_manager;
		delete g_web_server;
		delete g_file_manager;

		return 1;
	}

	/* 清理资源 */
	delete g_window_manager;
	delete g_web_server;
	delete g_file_manager;

	std::cout << "Mikufy 已关闭" << std::endl;

	return 0;
}