/* Mikufy v2.2(stable) - 窗口管理器实现
 * 负责GTK窗口和WebKit WebView的创建与管理
 */

#include "../headers/window_manager.h"
#include <iostream>

/* 构造函数 */
WindowManager::WindowManager(WebServer *web_server)
	: web_server(web_server), window(nullptr), web_view(nullptr),
	  dialog_waiting(false), main_loop(nullptr)
{
	g_cond_init(&dialog_cond);
}

/* 析构函数 */
WindowManager::~WindowManager(void)
{
	if (main_loop) {
		g_main_loop_unref(main_loop);
		main_loop = nullptr;
	}
	if (window && GTK_IS_WINDOW(window))
		gtk_window_destroy(GTK_WINDOW(window));
}

/* 初始化窗口 */
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

/* 创建GTK窗口 */
bool WindowManager::create_window(void)
{
	/* 创建窗口 (GTK4: gtk_window_new 不再需要参数) */
	window = gtk_window_new();

	/* 设置窗口标题 */
	gtk_window_set_title(GTK_WINDOW(window), WINDOW_TITLE);

	/* 设置窗口大小 */
	gtk_window_set_default_size(GTK_WINDOW(window), WINDOW_WIDTH,
				    WINDOW_HEIGHT);

	/* 设置窗口位置居中 (GTK4 中已移除此功能，默认居中) */

	/* 设置窗口可调整大小 */
	gtk_window_set_resizable(GTK_WINDOW(window), TRUE);

	/* 连接关闭信号 (GTK4: delete-event 改为 close-request) */
	g_signal_connect(window, "close-request", G_CALLBACK(on_window_close),
			 this);

	return true;
}

/* 创建WebKit WebView */
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

	/* 禁用开发者工具 */
	webkit_settings_set_enable_developer_extras(settings, FALSE);

	/* 设置不缓存 */
	webkit_settings_set_enable_write_console_messages_to_stdout(settings,
								   TRUE);

	std::cout << "WebView 属性设置完成" << std::endl;

	/* 连接加载完成信号 */
	g_signal_connect(web_view, "load-changed",
			 G_CALLBACK(on_web_view_load_changed), this);

	std::cout << "WebView 信号连接完成" << std::endl;

	/* 将WebView添加到窗口 (GTK4 API) */
	gtk_window_set_child(GTK_WINDOW(window), GTK_WIDGET(web_view));

	std::cout << "WebView 已添加到窗口" << std::endl;

	/* 显示WebView (GTK4: 使用 gtk_widget_set_visible) */
	gtk_widget_set_visible(GTK_WIDGET(web_view), TRUE);

	std::cout << "WebView 显示设置完成" << std::endl;

	return true;
}

/* 加载前端页面 */
void WindowManager::load_frontend_page(void)
{
	std::string url = "http://localhost:" +
			  std::to_string(WEB_SERVER_PORT) + "/";
	std::cout << "正在加载前端页面: " << url << std::endl;
	webkit_web_view_load_uri(web_view, url.c_str());
}

/* 显示窗口 */
void WindowManager::show(void)
{
	if (window)
		gtk_window_present(GTK_WINDOW(window));
}

/* 运行GTK主循环 */
void WindowManager::run(void)
{
	if (main_loop)
		g_main_loop_run(main_loop);
}

/* 关闭窗口 */
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

/* 获取当前工作目录（通过文件对话框选择） */
std::string WindowManager::get_current_working_directory(void)
{
	/* 使用异步方式打开对话框，避免在HTTP请求处理中阻塞GTK主循环 */
	return open_folder_dialog_async();
}

/* 异步打开文件夹对话框 */
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

/* 处理窗口关闭事件 (GTK4: close-request 信号) */
gboolean WindowManager::on_window_close(GtkWidget *widget,
					gpointer user_data)
{
	(void)widget;

	WindowManager *manager = static_cast<WindowManager *>(user_data);

	/* 保存所有文件
	 * 这里可以通过JavaScript调用前端的保存函数
	 * 或者直接调用WebServer的API
	 */

	/* 退出主循环 */
	if (manager->main_loop && g_main_loop_is_running(manager->main_loop))
		g_main_loop_quit(manager->main_loop);

	/* 关闭窗口 */
	manager->close();

	return TRUE; /* GTK4: 返回 TRUE 阻止默认行为 */
}

/* 处理WebView加载完成事件 */
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
			std::cout << "WebView URI: " << uri << std::endl;
	}
}