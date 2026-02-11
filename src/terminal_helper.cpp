/*
 * Mikufy v2.11-nova - 终端助手程序
 *
 * 这是一个独立的 GTK4 终端助手程序，用于在新窗口中运行命令。
 * 主程序通过命令行参数传递要执行的命令和工作目录。
 *
 * MiraTrive/MikuTrive
 *
 * 本文件遵循Linux内核代码风格规范
 */

#include <gtk/gtk.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/epoll.h>
#include <signal.h>
#include <termios.h>
#include <pty.h>
#include <errno.h>
#include <cstring>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <vector>
#include <string>

/* 全局变量（因为是独立程序，可以使用全局变量） */
static GtkApplication *app = nullptr;
static GtkTextView *text_view = nullptr;
static GtkTextBuffer *text_buffer = nullptr;
static pid_t child_pid = -1;
static int master_fd = -1;
static std::atomic<bool> stop_io_thread(false);
static std::jthread io_thread;
static std::mutex output_mutex;

/**
 * insert_text - 在文本视图中插入文本
 */
static void insert_text(const gchar *text)
{
	GtkTextIter iter;
	gtk_text_buffer_get_end_iter(text_buffer, &iter);
	gtk_text_buffer_insert(text_buffer, &iter, text, -1);

	/* 自动滚动到底部 */
	GtkTextMark *mark = gtk_text_buffer_get_insert(text_buffer);
	gtk_text_view_scroll_mark_onscreen(text_view, mark);
}

/**
 * io_thread_func - IO线程函数，读取PTY输出
 */
static void io_thread_func()
{
	char buffer[8192];
	int epfd = epoll_create1(EPOLL_CLOEXEC);
	struct epoll_event ev;

	if (epfd < 0)
		return;

	ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
	ev.data.fd = master_fd;

	if (epoll_ctl(epfd, EPOLL_CTL_ADD, master_fd, &ev) < 0) {
		close(epfd);
		return;
	}

	bool process_finished = false;

	while (!stop_io_thread && master_fd >= 0) {
		struct epoll_event events[1];
		int nfds = epoll_wait(epfd, events, 1, 10); /* 10ms 超时 */

		if (nfds < 0) {
			if (errno == EINTR)
				continue;
			break;
		}

		if (nfds == 0) {
			/* 超时，继续 */
			continue;
		}

		/* 读取所有可用数据 */
		while (!stop_io_thread && master_fd >= 0) {
			ssize_t bytes_read = read(master_fd, buffer, sizeof(buffer) - 1);

			if (bytes_read <= 0) {
				if (bytes_read < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
					break;
				}
				/* 进程结束 */
				stop_io_thread = true;
				process_finished = true;
				break;
			}

			buffer[bytes_read] = '\0';

			std::lock_guard<std::mutex> lock(output_mutex);

			/* 在 GTK 主线程中插入文本 */
			g_idle_add([](gpointer data) -> gboolean {
				insert_text(static_cast<const gchar *>(data));
				g_free(data);
				return G_SOURCE_REMOVE;
			}, g_strdup(buffer));
		}
	}

	/* 如果进程已结束，显示完成消息 */
	if (process_finished) {
		std::lock_guard<std::mutex> lock(output_mutex);
		g_idle_add([](gpointer data) -> gboolean {
			insert_text(static_cast<const gchar *>(data));
			g_free(data);
			return G_SOURCE_REMOVE;
		}, g_strdup("\n[Command completed - Press ESC to close]\n"));
	}

	close(epfd);
}

/**
 * on_key_pressed - 键盘事件处理
 */
static gboolean on_key_pressed(GtkEventControllerKey *controller,
			       guint keyval, guint keycode,
			       GdkModifierType state, gpointer user_data)
{
	(void)controller;
	(void)keycode;
	(void)state;
	(void)user_data;

	/* ESC 键关闭窗口 */
	if (keyval == GDK_KEY_Escape) {
		GtkWidget *window = gtk_widget_get_ancestor(GTK_WIDGET(text_view),
							    GTK_TYPE_WINDOW);
		if (window) {
			gtk_window_close(GTK_WINDOW(window));
		}
		return TRUE;
	}

	if (master_fd >= 0) {
		if (keyval == GDK_KEY_Return) {
			write(master_fd, "\n", 1);
		} else if (keyval == GDK_KEY_BackSpace) {
			write(master_fd, "\b", 1);
		} else if (keyval >= 32 && keyval <= 126) {
			/* 可打印字符 */
			char c = static_cast<char>(keyval);
			write(master_fd, &c, 1);
		}
	}

	return FALSE;
}

/**
 * on_close_request - 窗口关闭请求
 */
static void on_close_request(GtkWindow *window, gpointer user_data)
{
	(void)window;
	(void)user_data;

	stop_io_thread = true;

	/* 立即终止子进程，不等待 */
	if (child_pid > 0) {
		kill(child_pid, SIGKILL);
		waitpid(child_pid, nullptr, WNOHANG);
	}

	/* 立即关闭PTY */
	if (master_fd >= 0) {
		close(master_fd);
		master_fd = -1;
	}

	/* 立即退出应用 */
	g_application_quit(G_APPLICATION(app));
}

/* 数据传递结构体 */
struct LaunchData {
	std::string command;
	std::string working_dir;
};

/**
 * on_activate - 应用激活
 */
static void on_activate(GtkApplication *app, gpointer user_data)
{
	LaunchData *data = static_cast<LaunchData *>(user_data);
	const char *command = data->command.c_str();
	const char *working_dir = data->working_dir.c_str();

	/* 创建PTY并启动进程（先创建PTY，再创建窗口，提高响应速度） */
	master_fd = -1;
	child_pid = forkpty(&master_fd, nullptr, nullptr, nullptr);

	if (child_pid < 0) {
		/* 创建失败，显示错误窗口 */
		GtkWidget *window = gtk_application_window_new(app);
		gtk_window_set_title(GTK_WINDOW(window), "Terminal");
		gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
		GtkWidget *label = gtk_label_new("Error: forkpty failed");
		gtk_window_set_child(GTK_WINDOW(window), label);
		gtk_window_present(GTK_WINDOW(window));
		return;
	}

	if (child_pid == 0) {
		/* 子进程 */
		setsid();

		/* 重置 SIGCHLD 信号处理，允许子进程创建子进程 */
		struct sigaction sa;
		sa.sa_handler = SIG_DFL;
		sigemptyset(&sa.sa_mask);
		sa.sa_flags = 0;
		sigaction(SIGCHLD, &sa, nullptr);

		/* 设置环境变量禁用输出缓冲 */
		setenv("PYTHONUNBUFFERED", "1", 1);
		setenv("FORCE_COLOR", "1", 1);

		/* 切换工作目录 */
		if (working_dir && strlen(working_dir) > 0) {
			chdir(working_dir);
		}

		/* 使用 bash 而不是 sh，以获得更好的兼容性 */
		execl("/bin/bash", "bash", "-c", command, nullptr);
		_exit(127);
	}

	/* 设置PTY为非阻塞 */
	int flags = fcntl(master_fd, F_GETFL, 0);
	fcntl(master_fd, F_SETFL, flags | O_NONBLOCK);

	/* 创建窗口 - 最小化初始化，快速显示 */
	GtkWidget *window = gtk_application_window_new(app);
	gtk_window_set_title(GTK_WINDOW(window), "Terminal");
	gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

	/* 创建文本视图 */
	text_view = GTK_TEXT_VIEW(gtk_text_view_new());
	gtk_text_view_set_editable(text_view, FALSE);
	gtk_text_view_set_cursor_visible(text_view, FALSE);
	gtk_text_view_set_wrap_mode(text_view, GTK_WRAP_CHAR);

	/* 添加CSS类 */
	gtk_widget_add_css_class(GTK_WIDGET(text_view), "terminal-text");

	text_buffer = gtk_text_view_get_buffer(text_view);

	/* ESC键处理 */
	GtkEventController *key_controller = gtk_event_controller_key_new();
	g_signal_connect(key_controller, "key-pressed",
			 G_CALLBACK(on_key_pressed), nullptr);
	gtk_widget_add_controller(GTK_WIDGET(text_view), key_controller);

	/* 创建滚动窗口 */
	GtkScrolledWindow *scrolled_window =
		GTK_SCROLLED_WINDOW(gtk_scrolled_window_new());
	gtk_scrolled_window_set_child(scrolled_window, GTK_WIDGET(text_view));

	gtk_window_set_child(GTK_WINDOW(window), GTK_WIDGET(scrolled_window));

	/* 窗口关闭处理 */
	g_signal_connect(window, "close-request",
			 G_CALLBACK(on_close_request), nullptr);

	/* 启动IO线程 */
	io_thread = std::jthread(io_thread_func);

	/* 立即显示窗口 */
	gtk_window_present(GTK_WINDOW(window));

	/* 延迟初始化提示文本，不影响窗口显示速度 */
	g_idle_add_once([](gpointer user_data) -> void {
		LaunchData *d = static_cast<LaunchData *>(user_data);
		if (text_buffer) {
			std::string prompt = std::format("[Working Directory: {}]\n[Command: {}]\n\n",
							d->working_dir.c_str(), d->command.c_str());
			gtk_text_buffer_insert_at_cursor(text_buffer, prompt.c_str(), -1);
		}
	}, data);
}

/**
 * main - 主函数
 */
int main(int argc, char **argv)
{
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <command> [working_dir]\n", argv[0]);
		return 1;
	}

	const char *command = argv[1];
	const char *working_dir = (argc >= 3) ? argv[2] : "/";

	/* 创建GTK应用 - 使用 NON_UNIQUE 标志提高启动速度 */
	app = gtk_application_new("com.mikufy.terminal-helper",
				  G_APPLICATION_NON_UNIQUE);

	if (!app) {
		fprintf(stderr, "Error: Failed to create GTK application\n");
		return 1;
	}

	/* 此GTK4独立终端进程面板 */
	GtkCssProvider *css_provider = gtk_css_provider_new();
	gtk_css_provider_load_from_string(
		css_provider,
		".terminal-text { background-color: #000000; color: #ffffff; "
		"font-family: monospace; }");

	gtk_style_context_add_provider_for_display(
		gdk_display_get_default(),
		GTK_STYLE_PROVIDER(css_provider),
		GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

	g_object_unref(css_provider);

	/* 创建数据传递结构体 */
	LaunchData *data = new LaunchData{command, working_dir};

	/* 连接激活信号进行串口 */
	g_signal_connect_data(app, "activate", G_CALLBACK(on_activate),
			     data, [](gpointer user_data, GClosure *) {
				     LaunchData *d = static_cast<LaunchData *>(user_data);
				     delete d;
			     }, GConnectFlags(0));

	/* 运行应用（不传递命令行参数） */
	int status = g_application_run(G_APPLICATION(app), 0, nullptr);

	g_object_unref(app);

	return status;
}
