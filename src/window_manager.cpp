/*
 * Mikufy v2.11-nova - 窗口管理器实现
 *
 * 本文件实现了GTK4窗口和WebKit WebView的创建与管理。
 * 主要功能包括：
 * - 创建和配置GTK4主窗口
 * - 创建和配置WebKit WebView
 * - 加载和显示Web前端页面
 * - 处理键盘事件（F11全屏切换、ESC拦截等）
 * - 提供文件夹选择对话框
 * - 管理GTK主循环
 *
 * Mikufy Project
 */

#include "../headers/window_manager.h"
#include <iostream>
#include <format>

/**
 * WindowManager::WindowManager - 窗口管理器构造函数
 * @web_server: Web服务器指针，用于前端通信
 *
 * 初始化窗口管理器实例，设置默认参数。
 */
WindowManager::WindowManager(WebServer *web_server)
	: web_server(web_server), window(nullptr), web_view(nullptr),
	  dialog_waiting(false), main_loop(nullptr), f11_pressed(false)
{
	g_cond_init(&dialog_cond); /* 初始化条件变量 */
}

/**
 * WindowManager::~WindowManager - 窗口管理器析构函数
 *
 * 清理资源，释放GTK窗口和主循环。
 */
WindowManager::~WindowManager(void)
{
	if (main_loop) {
		g_main_loop_unref(main_loop);
		main_loop = nullptr;
	}
	if (window && GTK_IS_WINDOW(window))
		gtk_window_destroy(GTK_WINDOW(window));
}

/**
 * WindowManager::init - 初始化窗口
 *
 * 初始化GTK4、创建主循环、创建窗口和WebView。
 *
 * 返回: 初始化成功返回true，失败返回false
 */
bool WindowManager::init(void)
{
	/* 初始化 GTK (GTK4: gtk_init 不再需要参数) */
	gtk_init();

	/* 创建主循环 */
	main_loop = g_main_loop_new(nullptr, FALSE);
	if (!main_loop)
		return false;

	/* 创建窗口 */
	if (!create_window()) {
		g_main_loop_unref(main_loop);
		main_loop = nullptr;
		return false;
	}

	/* 创建 WebView */
	if (!create_web_view()) {
		g_main_loop_unref(main_loop);
		main_loop = nullptr;
		return false;
	}

	return true;
}

/**
 * WindowManager::create_window - 创建GTK窗口
 *
 * 创建并配置GTK4主窗口，设置标题、大小和事件处理器。
 *
 * 返回: 创建成功返回true，失败返回false
 */
bool WindowManager::create_window(void)
{
	/* 创建窗口 (GTK4: gtk_window_new 不再需要参数) */
	window = gtk_window_new();

	/* 设置窗口标题 */
	gtk_window_set_title(GTK_WINDOW(window), WINDOW_TITLE);

	/* 设置窗口大小 */
	gtk_window_set_default_size(GTK_WINDOW(window), WINDOW_WIDTH,
				    WINDOW_HEIGHT);

	/* 设置窗口可调整大小 */
	gtk_window_set_resizable(GTK_WINDOW(window), TRUE);

	/* 连接关闭信号 (GTK4: delete-event 改为 close-request) */
	g_signal_connect(window, "close-request", G_CALLBACK(on_window_close),
			 this);

	/* 在窗口级别添加键盘事件控制器（在全屏状态下拦截按键） */
	GtkEventController *window_key_controller = gtk_event_controller_key_new();
	gtk_widget_add_controller(window, window_key_controller);
	gtk_event_controller_set_propagation_phase(window_key_controller,
						    GTK_PHASE_CAPTURE);
	g_signal_connect(window_key_controller, "key-pressed",
			 G_CALLBACK(on_key_press), this);

	return true;
}

/**
 * WindowManager::create_web_view - 创建WebKit WebView
 *
 * 创建并配置WebKit WebView，设置JavaScript支持、全屏功能等。
 *
 * 返回: 创建成功返回true，失败返回false
 */
bool WindowManager::create_web_view(void)
{
	std::cout << "开始创建 WebView..." << std::endl;

	/* 创建WebView */
	web_view = WEBKIT_WEB_VIEW(webkit_web_view_new());

	if (!web_view) {
		std::cerr << "WebView 创建失败" << std::endl;
		return false;
	}

	std::cout << "WebView 创建成功" << std::endl;

	/* 设置WebView属性 */
	WebKitSettings *settings = webkit_web_view_get_settings(web_view);

	/* 启用JavaScript */
	webkit_settings_set_enable_javascript(settings, TRUE);

	/* 启用WebGL */
	webkit_settings_set_enable_webgl(settings, TRUE);

	/* 启用平滑滚动 */
	webkit_settings_set_enable_smooth_scrolling(settings, TRUE);

	/* 禁用开发者工具 */
	webkit_settings_set_enable_developer_extras(settings, FALSE);

	/* 启用 WebKit 全屏功能（JavaScript requestFullscreen 需要） */
	webkit_settings_set_enable_fullscreen(settings, TRUE);

	/* 设置不缓存 */
	webkit_settings_set_enable_write_console_messages_to_stdout(settings,
								   TRUE);

	std::cout << "WebView 属性设置完成" << std::endl;

	/* 连接加载完成信号 */
	g_signal_connect(web_view, "load-changed",
			 G_CALLBACK(on_web_view_load_changed), this);

	/* 连接全屏退出信号，记录调试信息 */
	g_signal_connect(web_view, "leave-fullscreen",
			 G_CALLBACK(on_web_view_leave_fullscreen), this);

	std::cout << "WebView 信号连接完成" << std::endl;

	/* 将WebView添加到窗口 (GTK4 API) */
	gtk_window_set_child(GTK_WINDOW(window), GTK_WIDGET(web_view));

	std::cout << "WebView 已添加到窗口" << std::endl;

	/* 显示WebView (GTK4: 使用 gtk_widget_set_visible) */
	gtk_widget_set_visible(GTK_WIDGET(web_view), TRUE);

	/* 创建键盘事件控制器（在全屏状态下拦截特定按键） */
	GtkEventController *key_controller = gtk_event_controller_key_new();
	gtk_event_controller_set_propagation_phase(key_controller,
						     GTK_PHASE_CAPTURE);
	gtk_widget_add_controller(window, key_controller);
	g_signal_connect(key_controller, "key-pressed",
			 G_CALLBACK(on_key_press), this);

	std::cout << "WebView 显示设置完成" << std::endl;

	return true;
}

/**
 * WindowManager::load_frontend_page - 加载前端页面
 *
 * 加载Web前端页面到WebView中。
 * 从本地Web服务器获取页面内容。
 */
void WindowManager::load_frontend_page(void)
{
	std::string url = std::format("http://localhost:{}/", WEB_SERVER_PORT);
	std::cout << std::format("正在加载前端页面: {}", url) << std::endl;
	webkit_web_view_load_uri(web_view, url.c_str());
}

/**
 * WindowManager::show - 显示窗口
 *
 * 显示GTK窗口。
 */
void WindowManager::show(void)
{
	if (window)
		gtk_window_present(GTK_WINDOW(window));
}

/**
 * WindowManager::run - 运行GTK主循环
 *
 * 启动GTK主事件循环，阻塞直到调用quit()。
 */
void WindowManager::run(void)
{
	if (main_loop)
		g_main_loop_run(main_loop);
}

/**
 * WindowManager::close - 关闭窗口
 *
 * 退出GTK主循环并销毁窗口。
 */
void WindowManager::close(void)
{
	if (main_loop && g_main_loop_is_running(main_loop))
		g_main_loop_quit(main_loop);
	if (window && GTK_IS_WINDOW(window)) {
		gtk_window_destroy(GTK_WINDOW(window));
		window = nullptr;
		web_view = nullptr;
	}
}

/**
 * WindowManager::get_current_working_directory - 获取当前工作目录
 *
 * 通过文件对话框让用户选择工作文件夹。
 * 使用异步方式打开对话框，避免在HTTP请求处理中阻塞GTK主循环。
 *
 * 返回: 用户选择的文件夹路径，取消或失败返回空字符串
 */
std::string WindowManager::get_current_working_directory(void)
{
	/* 使用异步方式打开对话框，避免在HTTP请求处理中阻塞GTK主循环 */
	return open_folder_dialog_async();
}

/**
 * WindowManager::open_folder_dialog_async - 异步打开文件夹对话框
 *
 * 使用GTK4的异步API打开文件夹选择对话框。
 * 使用轮询方式等待对话框完成，避免阻塞GTK主循环。
 *
 * 返回: 用户选择的文件夹路径，取消或失败返回空字符串
 */
std::string WindowManager::open_folder_dialog_async(void)
{
	/* 检查窗口是否有效 */
	if (!window || !GTK_IS_WINDOW(window) ||
	    !gtk_widget_get_visible(window))
		return "";

	dialog_result = "";
	dialog_waiting = true;

	/* 获取主上下文 */
	GMainContext *context = g_main_context_default();

	/* 使用g_main_context_invoke在主线程中执行对话框 */
	g_main_context_invoke(context, [](gpointer user_data) -> gboolean {
		WindowManager *manager = static_cast<WindowManager *>(user_data);

		/* GTK4 使用 GtkFileDialog (新的异步 API) */
		GtkFileDialog *dialog = gtk_file_dialog_new();
		gtk_file_dialog_set_title(dialog, "选择工作文件夹");
		gtk_file_dialog_set_modal(dialog, TRUE);

		/* 显示选择文件夹对话框 */
		gtk_file_dialog_select_folder(
			dialog, GTK_WINDOW(manager->window), nullptr,
			[](GObject *source, GAsyncResult *result,
			   gpointer user_data) {
				GtkFileDialog *dialog = GTK_FILE_DIALOG(source);
				WindowManager *manager =
					static_cast<WindowManager *>(user_data);

				GFile *file =
					gtk_file_dialog_select_folder_finish(
						dialog, result, nullptr);
				if (file) {
					char *filename = g_file_get_path(file);
					if (filename) {
						manager->dialog_result = filename;
						g_free(filename);
					}
					g_object_unref(file);
				}

				/* 发送完成信号 */
				manager->dialog_waiting = false;

				g_object_unref(dialog);
			},
			manager);

		return G_SOURCE_REMOVE;
	}, this);

	/* 等待对话框完成 */
	while (dialog_waiting)
		usleep(100000);

	return dialog_result;
}

/**
 * WindowManager::on_window_close - 处理窗口关闭事件
 * @widget: 触发事件的GtkWidget（未使用）
 * @user_data: 用户数据，指向WindowManager实例
 *
 * 处理窗口关闭请求，退出主循环并销毁窗口。
 *
 * 返回: TRUE阻止默认行为
 */
gboolean WindowManager::on_window_close(GtkWidget *widget,
					gpointer user_data)
{
	(void)widget;

	WindowManager *manager = static_cast<WindowManager *>(user_data);

	/* 退出主循环 */
	if (manager->main_loop && g_main_loop_is_running(manager->main_loop))
		g_main_loop_quit(manager->main_loop);

	/* 关闭窗口 */
	manager->close();

	return TRUE; /* GTK4: 返回 TRUE 阻止默认行为 */
}

/**
 * WindowManager::on_web_view_load_changed - 处理WebView加载完成事件
 * @web_view: WebView实例（未使用）
 * @load_event: 加载事件类型
 * @user_data: 用户数据（未使用）
 *
 * 处理WebView页面加载状态变化事件，记录加载过程。
 */
void WindowManager::on_web_view_load_changed(WebKitWebView *web_view,
					     WebKitLoadEvent load_event,
					     gpointer user_data)
{
	(void)user_data;

	if (load_event == WEBKIT_LOAD_FINISHED) {
		std::cout << "WebView 页面加载完成" << std::endl;
	} else if (load_event == WEBKIT_LOAD_STARTED) {
		std::cout << "WebView 开始加载页面" << std::endl;
	} else if (load_event == WEBKIT_LOAD_REDIRECTED) {
		std::cout << "WebView 页面重定向" << std::endl;
	} else if (load_event == WEBKIT_LOAD_COMMITTED) {
		std::cout << "WebView 页面提交" << std::endl;
		const gchar *uri = webkit_web_view_get_uri(web_view);
		if (uri)
			std::cout << std::format("WebView URI: {}", uri) << std::endl;
	}
}

/**
 * WindowManager::on_web_view_leave_fullscreen - 处理WebView全屏退出事件
 * @web_view: WebView实例（未使用）
 * @user_data: 用户数据，指向WindowManager实例
 *
 * 处理WebView退出全屏事件。
 * 根据f11_pressed标志判断是否允许退出全屏。
 *
 * 返回: TRUE阻止退出全屏，FALSE允许退出
 */
gboolean WindowManager::on_web_view_leave_fullscreen(WebKitWebView *web_view,
						    gpointer user_data)
{
	WindowManager *manager = static_cast<WindowManager *>(user_data);
	(void)web_view;

	std::cout << std::format("WebView全屏退出事件，F11按键={}",
				 manager->f11_pressed ? "是" : "否") << std::endl;

	/* 如果是 F11 导致的，允许退出；否则阻止退出 */
	if (manager->f11_pressed) {
		manager->f11_pressed = false; /* 重置标志 */
		return FALSE; /* 允许退出全屏 */
	}

	/* 阻止退出全屏（如 F 键导致的） */
	return TRUE;
}

/**
 * WindowManager::on_key_press - 处理键盘按键事件
 * @widget: 触发事件的GtkWidget（未使用）
 * @keyval: 按键值
 * @keycode: 硬件按键码（未使用）
 * @state: 修饰键状态（未使用）
 * @user_data: 用户数据，指向WindowManager实例
 *
 * 处理键盘按键事件，主要处理F11全屏切换和ESC键拦截。
 * 在全屏状态下拦截ESC键，防止浏览器默认行为。
 *
 * 返回: TRUE阻止默认行为，FALSE允许默认行为
 */
gboolean WindowManager::on_key_press(GtkWidget *widget, guint keyval,
				     guint keycode, GdkModifierType state,
				     gpointer user_data)
{
	(void)widget;
	(void)keycode;
	(void)state;

	/* 获取 WindowManager 实例 */
	WindowManager *manager = static_cast<WindowManager *>(user_data);

	/* 检查窗口是否有效 */
	if (!manager || !manager->window || !GTK_IS_WINDOW(manager->window)) {
		return FALSE;
	}

	/* 检查窗口是否全屏 */
	gboolean is_fullscreen = gtk_window_is_fullscreen(GTK_WINDOW(manager->window));

	/* 检测 F11 键（用于全屏切换） */
	if (keyval == GDK_KEY_F11) {
		std::cout << "检测到F11按键" << std::endl;
		manager->f11_pressed = true;

		/* 立即切换窗口全屏状态 */
		if (is_fullscreen) {
			gtk_window_unfullscreen(GTK_WINDOW(manager->window));
			std::cout << "退出全屏" << std::endl;
		} else {
			gtk_window_fullscreen(GTK_WINDOW(manager->window));
			std::cout << "进入全屏" << std::endl;
		}

		return TRUE; /* 阻止默认行为，我们已经处理了 */
	}

	/* 只在全屏状态下拦截ESC键（浏览器默认用来退出全屏的按键） */
	if (is_fullscreen && keyval == GDK_KEY_Escape) {
		std::cout << "全屏状态下拦截ESC键" << std::endl;
		return TRUE; /* 返回TRUE阻止默认行为 */
	}

	/* 其他键不拦截，允许正常输入 */
	return FALSE; /* 允许默认行为 */
}
