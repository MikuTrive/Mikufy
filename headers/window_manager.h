/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Mikufy v2.4(stable) - 窗口管理器头文件
 *
 * 本文件定义了WindowManager类的接口，该类负责管理GTK窗口和
 * WebKitWebView组件。WindowManager创建并维护应用程序的主窗口，
 * 嵌入WebKit WebView以显示前端界面，并处理所有窗口相关的
 * 事件和交互。
 *
 * 主要功能:
 * - GTK窗口创建和管理
 * - WebKit WebView嵌入和配置
 * - 前端页面加载
 * - 窗口事件处理（关闭、全屏、键盘事件）
 * - 文件对话框管理（异步打开）
 * - 主循环控制
 *
 * 设计特点:
 * - 使用GTK4和WebKitGTK 6.0（Fedora 43）
 * - 异步文件对话框：避免阻塞GTK主循环
 * - 全屏管理：支持F11切换全屏，阻止其他方式退出全屏
 * - 线程安全：使用互斥锁保护共享资源
 *
 * MiraTrive/MikuTrive
 * Author: [Your Name]
 *
 * 本文件遵循Linux内核代码风格规范
 */

#ifndef MIKUFY_WINDOW_MANAGER_H
#define MIKUFY_WINDOW_MANAGER_H

/*
 * ============================================================================
 * 头文件包含
 * ============================================================================
 */

#include "main.h"		/* 全局定义和数据结构 */
#include "web_server.h"		/* WebServer类 */

/*
 * ============================================================================
 * WindowManager类定义
 * ============================================================================
 */

/**
 * WindowManager - 窗口管理器类
 *
 * 该类负责创建和管理应用程序的GTK窗口，嵌入WebKit WebView组件
 * 显示前端界面，并处理所有窗口相关的事件。WindowManager与
 * WebServer配合工作，提供完整的图形用户界面。
 *
 * 设计特点:
 * - GTK4兼容：使用GTK4的新API（gtk_window_new无参数等）
 * - WebKit6.0支持：使用WebKitGTK 6.0的WebView组件
 * - 异步对话框：使用g_main_context_invoke避免阻塞
 * - 全屏控制：精确控制全屏进入和退出
 * - 事件处理：处理窗口关闭、页面加载、键盘事件等
 *
 * 使用示例:
 * @code
 * WebServer ws(&fm);
 * WindowManager wm(&ws);
 * if (wm.init()) {
 *     wm.load_frontend_page();
 *     wm.show();
 *     wm.run();
 * }
 * @endcode
 */
class WindowManager
{
public:
	/**
	 * WindowManager - 构造函数
	 *
	 * 初始化WindowManager对象，设置WebServer引用并初始化
	 * GTK相关资源（如条件变量）。
	 *
	 * @web_server: WebServer对象的指针，用于服务器通信
	 */
	WindowManager(WebServer *web_server);

	/**
	 * ~WindowManager - 析构函数
	 *
	 * 清理WindowManager对象，释放GTK资源和主循环对象。
	 */
	~WindowManager(void);

	/* 禁止拷贝构造函数 */
	WindowManager(const WindowManager &) = delete;

	/* 禁止拷贝赋值操作符 */
	WindowManager &operator=(const WindowManager &) = delete;

	/* 禁止移动构造函数 */
	WindowManager(WindowManager &&) = delete;

	/* 禁止移动赋值操作符 */
	WindowManager &operator=(WindowManager &&) = delete;

	/* ====================================================================
	 * 公共方法 - 窗口管理
	 * ==================================================================== */

	/**
	 * init - 初始化窗口
	 *
	 * 初始化GTK环境，创建主循环、窗口和WebView组件。该方法
	 * 完成所有必要的初始化工作，为显示窗口做准备。
	 *
	 * 返回值: 初始化成功返回true，失败返回false
	 *
	 * 注意: 该方法必须在调用show()和run()之前调用。
	 */
	bool init(void);

	/**
	 * show - 显示窗口
	 *
	 * 将窗口显示到屏幕上。窗口会按照之前设置的大小和位置显示。
	 *
	 * 注意: 该方法不会阻塞，窗口将在GTK主循环中显示。
	 */
	void show(void);

	/**
	 * run - 运行GTK主循环
	 *
	 * 启动GTK主事件循环，开始处理窗口事件。该方法会阻塞
	 * 直到窗口关闭或调用close()方法。
	 *
	 * 注意: 该方法会阻塞调用线程，通常在主线程中调用。
	 */
	void run(void);

	/**
	 * close - 关闭窗口
	 *
	 * 关闭窗口并退出GTK主循环。该方法会停止主循环并释放
	 * 窗口资源。
	 *
	 * 注意: 该方法可以在任何线程中调用。
	 */
	void close(void);

	/* ====================================================================
	 * 公共方法 - 页面加载
	 * ==================================================================== */

	/**
	 * load_frontend_page - 加载前端页面
	 *
	 * 在WebView中加载前端页面。页面URL为本地Web服务器地址。
	 *
	 * 注意: WebServer必须已启动并正在监听。
	 */
	void load_frontend_page(void);

	/* ====================================================================
	 * 公共方法 - 文件对话框
	 * ==================================================================== */

	/**
	 * get_current_working_directory - 获取当前工作目录
	 *
	 * 通过文件对话框让用户选择文件夹，返回用户选择的路径。
	 * 该方法使用异步方式打开对话框，避免阻塞GTK主循环。
	 *
	 * 返回值: 用户选择的文件夹路径，取消返回空字符串
	 *
	 * 注意: 该方法会阻塞直到用户完成选择。
	 */
	std::string get_current_working_directory(void);

private:
	/* ====================================================================
	 * 私有成员变量
	 * ==================================================================== */

	WebServer *web_server;		/* WebServer指针 */
	GtkWidget *window;		/* GTK窗口对象 */
	WebKitWebView *web_view;	/* WebKit WebView对象 */
	std::string current_working_directory;	/* 当前工作目录 */
	std::mutex mutex;		/* 互斥锁，保护共享资源 */

	/* 对话框相关变量 */
	std::string dialog_result;	/* 对话框结果 */
	bool dialog_waiting;		/* 对话框等待标志 */
	GCond dialog_cond;		/* 条件变量 */
	GCond *dialog_ready;		/* 对话框就绪条件 */
	GMutex dialog_mutex;		/* 对话框互斥锁 */
	GMainLoop *main_loop;		/* GTK主循环对象 */

	/* 全屏控制标志 */
	bool f11_pressed;		/* F11按键标志 */

	/* ====================================================================
	 * 私有方法 - 窗口创建
	 * ==================================================================== */

	/**
	 * create_window - 创建GTK窗口
	 *
	 * 创建GTK主窗口，设置标题、大小和其他属性，并连接
	 * 信号处理器。
	 *
	 * 返回值: 创建成功返回true，失败返回false
	 */
	bool create_window(void);

	/**
	 * create_web_view - 创建WebKit WebView
	 *
	 * 创建WebKit WebView组件，配置JavaScript、全屏等设置，
	 * 并嵌入到窗口中。
	 *
	 * 返回值: 创建成功返回true，失败返回false
	 */
	bool create_web_view(void);

	/* ====================================================================
	 * 私有方法 - 对话框管理
	 * ==================================================================== */

	/**
	 * open_folder_dialog_async - 异步打开文件夹对话框
	 *
	 * 使用异步方式打开文件夹选择对话框，避免阻塞GTK主循环。
	 * 该方法使用g_main_context_invoke在主线程中创建对话框。
	 *
	 * 返回值: 用户选择的文件夹路径，取消返回空字符串
	 *
	 * 注意: 该方法会阻塞直到对话框关闭。
	 */
	std::string open_folder_dialog_async(void);

	/* ====================================================================
	 * 私有方法 - 事件处理函数（静态）
	 * ==================================================================== */

	/**
	 * on_window_close - 窗口关闭事件处理器
	 *
	 * GTK4的close-request信号处理函数，在用户尝试关闭窗口时调用。
	 * 该方法会退出GTK主循环并关闭窗口。
	 *
	 * @widget: 触发信号的GtkWidget对象
	 * @user_data: 用户数据（WindowManager指针）
	 *
	 * 返回值: 返回TRUE阻止默认关闭行为
	 */
	static gboolean on_window_close(GtkWidget *widget,
					gpointer user_data);

	/**
	 * on_web_view_load_changed - WebView加载状态变化处理器
	 *
	 * WebView的load-changed信号处理函数，在页面加载状态改变时调用。
	 * 该方法记录页面加载的各个阶段（开始、提交、完成等）。
	 *
	 * @web_view: WebView对象
	 * @load_event: 加载事件类型
	 * @user_data: 用户数据
	 */
	static void on_web_view_load_changed(WebKitWebView *web_view,
					     WebKitLoadEvent load_event,
					     gpointer user_data);

	/**
	 * on_web_view_leave_fullscreen - WebView退出全屏事件处理器
	 *
	 * WebView的leave-fullscreen信号处理函数，在WebView退出全屏时调用。
	 * 该方法根据f11_pressed标志决定是否允许退出全屏。
	 *
	 * @web_view: WebView对象
	 * @user_data: 用户数据（WindowManager指针）
	 *
	 * 返回值: 返回TRUE阻止退出全屏，FALSE允许退出
	 */
	static gboolean on_web_view_leave_fullscreen(WebKitWebView *web_view,
						    gpointer user_data);

	/**
	 * on_key_press - 键盘按键事件处理器
	 *
	 * GTK4的key-pressed信号处理函数，在键盘按键时调用。该方法
	 * 处理F11全屏切换和ESC键拦截等特殊按键。
	 *
	 * @widget: 触发信号的GtkWidget对象
	 * @keyval: 按键的键值
	 * @keycode: 按键的硬件代码
	 * @state: 修饰键状态（Ctrl、Shift等）
	 * @user_data: 用户数据（WindowManager指针）
	 *
	 * 返回值: 返回TRUE阻止默认行为，FALSE允许默认行为
	 */
	static gboolean on_key_press(GtkWidget *widget, guint keyval,
				     guint keycode, GdkModifierType state,
				     gpointer user_data);
};

#endif /* MIKUFY_WINDOW_MANAGER_H */
