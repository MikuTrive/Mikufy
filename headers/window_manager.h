/* Mikufy v2.2(stable) - 窗口管理器头文件
 * 负责GTK窗口和WebKitWebView的创建与管理
 */

#ifndef MIKUFY_WINDOW_MANAGER_H
#define MIKUFY_WINDOW_MANAGER_H

#include "main.h"
#include "web_server.h"

/* 窗口管理器类
 * 管理GTK窗口和WebKit WebView
 */
class WindowManager
{
public:
	WindowManager(WebServer *web_server);
	~WindowManager(void);

	/* 禁止拷贝和移动 */
	WindowManager(const WindowManager &) = delete;
	WindowManager &operator=(const WindowManager &) = delete;
	WindowManager(WindowManager &&) = delete;
	WindowManager &operator=(WindowManager &&) = delete;

	/* 初始化窗口 */
	bool init(void);

	/* 显示窗口 */
	void show(void);

	/* 运行GTK主循环 */
	void run(void);

	/* 关闭窗口 */
	void close(void);

	/* 获取当前工作目录（通过文件对话框选择） */
	std::string get_current_working_directory(void);

	/* 加载前端页面 */
	void load_frontend_page(void);

	/* 异步打开文件夹对话框 */
	std::string open_folder_dialog_async(void);

private:
	WebServer *web_server;
	GtkWidget *window;
	WebKitWebView *web_view;
	std::string current_working_directory;
	std::mutex mutex;
	std::string dialog_result;
	bool dialog_waiting;
	GCond dialog_cond;
	GCond *dialog_ready;
	GMutex dialog_mutex;
	GMainLoop *main_loop;

	/* 创建GTK窗口 */
	bool create_window(void);

	/* 创建WebKit WebView */
	bool create_web_view(void);

	/* 处理窗口关闭事件 (GTK4: close-request 信号) */
	static gboolean on_window_close(GtkWidget *widget,
					gpointer user_data);

	/* 处理WebView加载完成事件 */
	static void on_web_view_load_changed(WebKitWebView *web_view,
					     WebKitLoadEvent load_event,
					     gpointer user_data);
};

#endif /* MIKUFY_WINDOW_MANAGER_H */
