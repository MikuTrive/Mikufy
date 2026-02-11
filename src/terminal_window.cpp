/*
 * Mikufy v2.11-nova - 终端窗口实现
 *
 * 本文件实现了TerminalWindow类，提供独立的GTK4终端窗口功能。
 * 使用 GtkTextView + forkpty 实现基础终端模拟（不使用 VTE，兼容 GTK4）。
 *
 * 核心实现:
 * - GTK4应用窗口创建
 * - GtkTextView 显示终端输出
 * - forkpty创建伪终端
 * - ESC键处理
 * - 进程终止和窗口关闭
 * - RAII资源管理
 *
 * MiraTrive/MikuTrive
 *
 * 本文件遵循Linux内核代码风格规范
 */

#include "../headers/terminal_window.h"
#include <gtk/gtk.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <termios.h>
#include <pty.h>
#include <errno.h>
#include <cstring>
#include <filesystem>
#include <thread>
#include <condition_variable>
#include <atomic>

namespace mikufy {

/* GTK 回调函数（在类外定义，避免头文件中的类型声明问题） */
static gboolean on_key_pressed(GtkEventControllerKey *controller,
			       guint keyval, guint keycode,
			       GdkModifierType state, gpointer user_data)
{
	(void)controller;
	(void)keycode;
	(void)state;

	auto *window = static_cast<TerminalWindow *>(user_data);

	if (keyval == GDK_KEY_Escape) {
		/* ESC键：关闭窗口并终止进程 */
		window->close_and_terminate();
	}

	return FALSE;
}

static void on_close_request(GtkWindow *window, gpointer user_data)
{
	(void)window;

	auto *term_window = static_cast<TerminalWindow *>(user_data);
	term_window->close_and_terminate();
}

static void on_activate(GtkApplication *app, gpointer user_data)
{
	(void)app;
	(void)user_data;

	/* 窗口已在setup_window中创建和设置 */
	/* 这里不需要额外操作 */
}

/*
 * ============================================================================
 * TerminalWindow::Impl 内部实现类
 * ============================================================================
 */

struct TerminalWindow::Impl {
	GtkApplication *app;
	GtkApplicationWindow *window;
	GtkTextView *text_view;
	GtkTextBuffer *text_buffer;
	GtkWidget *label;	/* 用于显示退出码 */

	pid_t child_pid;
	int master_fd;
	int exit_status;
	WindowState state;

	std::mutex mutex;
	std::condition_variable cv;
	std::atomic<bool> stop_io_thread;
	std::jthread io_thread;

	std::string executable;
	std::vector<std::string> args;
	std::string working_dir;

	Impl() : app(nullptr), window(nullptr), text_view(nullptr),
		 text_buffer(nullptr), label(nullptr), child_pid(-1),
		 master_fd(-1), exit_status(-1), state(WindowState::CLOSED),
		 stop_io_thread(false)
	{
	}

	~Impl()
	{
		/* 停止IO线程 */
		stop_io_thread = true;
		if (io_thread.joinable())
			io_thread.join();

		/* 清理资源 */
		if (master_fd >= 0)
			close(master_fd);

		if (app)
			g_object_unref(app);
	}
};

/*
 * ============================================================================
 * TerminalWindow 实现
 * ============================================================================
 */

/**
 * TerminalWindow::TerminalWindow - 构造函数
 */
TerminalWindow::TerminalWindow() : pImpl_(std::make_unique<Impl>())
{
}

/**
 * TerminalWindow::~TerminalWindow - 析构函数
 */
TerminalWindow::~TerminalWindow()
{
	/* 确保窗口已关闭 */
	if (pImpl_->state != WindowState::CLOSED)
		close_and_terminate();
}

/**
 * TerminalWindow::setup_application - 设置GTK4应用
 */
bool TerminalWindow::setup_application()
{
	/* 创建GTK4应用 */
	pImpl_->app = gtk_application_new("com.mikufy.terminal",
					  G_APPLICATION_DEFAULT_FLAGS);

	if (!pImpl_->app)
		return false;

	return true;
}

/**
 * TerminalWindow::setup_window - 设置窗口
 */
bool TerminalWindow::setup_window(const std::string &title)
{
	/* 创建应用窗口 */
	pImpl_->window = GTK_APPLICATION_WINDOW(
		gtk_application_window_new(pImpl_->app));

	if (!pImpl_->window)
		return false;

	gtk_window_set_title(GTK_WINDOW(pImpl_->window), title.c_str());
	gtk_window_set_default_size(GTK_WINDOW(pImpl_->window), 800, 600);
	gtk_window_set_resizable(GTK_WINDOW(pImpl_->window), TRUE);

	/* 设置黑色背景（使用 GtkStyleProvider） */
	GtkCssProvider *css_provider = gtk_css_provider_new();
	gtk_css_provider_load_from_string(
		css_provider,
		"window { background-color: #000000; } "
		"textview { background-color: #000000; color: #ffffff; "
		"font-family: monospace; font-size: 12px; } "
		"text { color: #ffffff; font-family: monospace; "
		"font-size: 12px; } "
		"label { color: #ffffff; font-family: monospace; "
		"font-size: 12px; padding: 10px; }");

	gtk_style_context_add_provider_for_display(
		gdk_display_get_default(),
		GTK_STYLE_PROVIDER(css_provider),
		GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

	g_object_unref(css_provider);

	/* 窗口关闭处理 */
	g_signal_connect(pImpl_->window, "close-request",
			 G_CALLBACK(on_close_request), this);

	/* 窗口显示处理 */
	g_signal_connect(pImpl_->app, "activate",
			 G_CALLBACK(on_activate), this);

	return true;
}

/**
 * TerminalWindow::setup_terminal - 设置终端（GtkTextView）
 */
bool TerminalWindow::setup_terminal()
{
	/* 创建文本视图 */
	pImpl_->text_view = GTK_TEXT_VIEW(gtk_text_view_new());

	if (!pImpl_->text_view)
		return false;

	/* 设置属性 */
	gtk_text_view_set_editable(pImpl_->text_view, FALSE);
	gtk_text_view_set_cursor_visible(pImpl_->text_view, FALSE);
	gtk_text_view_set_wrap_mode(pImpl_->text_view, GTK_WRAP_CHAR);
	gtk_text_view_set_left_margin(pImpl_->text_view, 5);
	gtk_text_view_set_right_margin(pImpl_->text_view, 5);
	gtk_text_view_set_top_margin(pImpl_->text_view, 5);
	gtk_text_view_set_bottom_margin(pImpl_->text_view, 5);

	/* 获取文本缓冲区 */
	pImpl_->text_buffer = gtk_text_view_get_buffer(pImpl_->text_view);

	/* ESC键处理 */
	GtkEventController *key_controller = gtk_event_controller_key_new();
	g_signal_connect(key_controller, "key-pressed",
			 G_CALLBACK(on_key_pressed), this);
	gtk_widget_add_controller(GTK_WIDGET(pImpl_->text_view),
				  key_controller);

	/* 创建滚动窗口 */
	GtkScrolledWindow *scrolled_window =
		GTK_SCROLLED_WINDOW(gtk_scrolled_window_new());
	gtk_scrolled_window_set_child(scrolled_window,
				       GTK_WIDGET(pImpl_->text_view));

	gtk_window_set_child(GTK_WINDOW(pImpl_->window),
			     GTK_WIDGET(scrolled_window));

	return true;
}

/**
 * TerminalWindow::spawn_process - 启动进程
 */
bool TerminalWindow::spawn_process(const std::string &executable,
				   const std::vector<std::string> &args,
				   const std::string &working_dir)
{
	/* 准备参数 */
	std::vector<char *> argv;
	argv.push_back(strdup(executable.c_str()));
	for (const auto &arg : args)
		argv.push_back(strdup(arg.c_str()));
	argv.push_back(nullptr);

	/* 创建伪终端 */
	struct winsize ws = {24, 80, 0, 0};
	pid_t pid = forkpty(&pImpl_->master_fd, nullptr, nullptr, &ws);

	if (pid < 0) {
		/* 清理参数 */
		for (char *arg : argv)
			if (arg)
				free(arg);
		return false;
	}

	if (pid == 0) {
		/* 子进程 */
		setsid();

		/* 切换工作目录 */
		if (!working_dir.empty() && working_dir != "/")
			if (chdir(working_dir.c_str()) < 0)
				_exit(127);

		/* 执行 */
		execvp(executable.c_str(), argv.data());

		/* 清理参数 */
		for (char *arg : argv)
			if (arg)
				free(arg);

		_exit(127);
	}

	/* 清理参数 */
	for (char *arg : argv)
		if (arg)
			free(arg);

	/* 设置PTY为非阻塞 */
	int flags = fcntl(pImpl_->master_fd, F_GETFL, 0);
	fcntl(pImpl_->master_fd, F_SETFL, flags | O_NONBLOCK);

	pImpl_->child_pid = pid;

	/* 启动IO线程读取PTY输出 */
	pImpl_->io_thread = std::jthread([this]() {
		char buffer[8192];
		while (!pImpl_->stop_io_thread && pImpl_->master_fd >= 0) {
			ssize_t bytes_read =
				read(pImpl_->master_fd, buffer, sizeof(buffer));

			if (bytes_read > 0) {
				/* 将输出添加到文本视图 */
				gtk_text_buffer_begin_user_action(
					pImpl_->text_buffer);

				GtkTextIter iter;
				gtk_text_buffer_get_end_iter(pImpl_->text_buffer,
							       &iter);

				gtk_text_buffer_insert(pImpl_->text_buffer,
						      &iter, buffer,
						      bytes_read);

				gtk_text_buffer_end_user_action(
					pImpl_->text_buffer);

				/* 自动滚动到底部 */
				GtkTextMark *mark =
					gtk_text_buffer_get_mark(
						pImpl_->text_buffer, "insert");
				gtk_text_view_scroll_mark_onscreen(
					pImpl_->text_view, mark);
			} else if (bytes_read < 0 && errno != EAGAIN &&
				   errno != EWOULDBLOCK) {
				break;
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	});

	return true;
}

/**
 * TerminalWindow::create_and_show - 创建并显示终端窗口
 */
bool TerminalWindow::create_and_show(const std::string &executable,
				     const std::vector<std::string> &args,
				     const std::string &working_dir)
{
	pImpl_->executable = executable;
	pImpl_->args = args;
	pImpl_->working_dir = working_dir;
	pImpl_->state = WindowState::ACTIVE;

	/* 设置应用 */
	if (!setup_application())
		return false;

	/* 设置窗口 */
	std::string title = std::format("Terminal - {}", executable);
	if (!setup_window(title))
		return false;

	/* 设置终端 */
	if (!setup_terminal())
		return false;

	/* 启动进程 */
	if (!spawn_process(executable, args, working_dir))
		return false;

	/* 运行应用（阻塞，直到窗口关闭） */
	g_application_run(G_APPLICATION(pImpl_->app), 0, nullptr);

	pImpl_->state = WindowState::CLOSED;

	return true;
}

/**
 * TerminalWindow::wait_for_process - 等待进程结束
 */
int TerminalWindow::wait_for_process()
{
	std::unique_lock<std::mutex> lock(pImpl_->mutex);

	/* 等待进程退出或窗口关闭 */
	pImpl_->cv.wait(lock, [this]() {
		return pImpl_->child_pid == -1 ||
		       pImpl_->state == WindowState::CLOSED;
	});

	return pImpl_->exit_status;
}

/**
 * TerminalWindow::is_running - 检查窗口是否还在运行
 */
bool TerminalWindow::is_running() const
{
	return pImpl_->state == WindowState::ACTIVE;
}

/**
 * TerminalWindow::get_state - 获取窗口状态
 */
WindowState TerminalWindow::get_state() const
{
	return pImpl_->state;
}

/**
 * TerminalWindow::get_pid - 获取进程ID
 */
pid_t TerminalWindow::get_pid() const
{
	return pImpl_->child_pid;
}

/**
 * TerminalWindow::close_and_terminate - 关闭窗口并终止进程
 */
void TerminalWindow::close_and_terminate()
{
	std::lock_guard<std::mutex> lock(pImpl_->mutex);

	if (pImpl_->state == WindowState::CLOSED)
		return;

	pImpl_->state = WindowState::CLOSING;
	pImpl_->stop_io_thread = true;

	/* 终止子进程 */
	if (pImpl_->child_pid > 0) {
		kill(pImpl_->child_pid, SIGTERM);

		/* 等待1秒 */
		auto start = std::chrono::steady_clock::now();
		while (std::chrono::duration_cast<std::chrono::milliseconds>(
			       std::chrono::steady_clock::now() - start)
			       .count() < 1000) {
			int status;
			pid_t result = waitpid(pImpl_->child_pid, &status,
					       WNOHANG);
			if (result == pImpl_->child_pid) {
				if (WIFEXITED(status))
					pImpl_->exit_status =
						WEXITSTATUS(status);
				else if (WIFSIGNALED(status))
					pImpl_->exit_status =
						128 + WTERMSIG(status);
				pImpl_->child_pid = -1;
				break;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}

		/* 如果进程仍未退出，强制杀死 */
		if (pImpl_->child_pid > 0) {
			kill(pImpl_->child_pid, SIGKILL);
			waitpid(pImpl_->child_pid, nullptr, 0);
			pImpl_->child_pid = -1;
			pImpl_->exit_status = -1;
		}
	}

	/* 关闭窗口 */
	if (pImpl_->window) {
		gtk_window_close(GTK_WINDOW(pImpl_->window));
		gtk_window_destroy(GTK_WINDOW(pImpl_->window));
		pImpl_->window = nullptr;
	}

	pImpl_->state = WindowState::CLOSED;

	/* 通知等待的线程 */
	pImpl_->cv.notify_all();
}

} /* namespace mikufy */
