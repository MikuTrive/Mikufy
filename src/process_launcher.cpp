/*
 * Mikufy v2.11-nova - 进程启动器实现
 *
 * 本文件实现了ProcessLauncher类，提供智能进程启动和GUI检测功能。
 *
 * 核心实现:
 * - X11窗口检测：枚举窗口树，检查_NET_WM_PID属性
 * - Wayland连接检测：检查/proc/[pid]/fd中的wayland连接
 * - PTY进程管理：forkpty创建伪终端
 * - GUI进程重启：检测到GUI后终止PTY，重新fork+execve
 * - RAII资源管理
 *
 * MiraTrive/MikuTrive
 *
 * 本文件遵循Linux内核代码风格规范
 */

#include "../headers/process_launcher.h"
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
#include <algorithm>
#include <regex>
#include <dirent.h>

#ifdef HAVE_X11
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#endif

namespace mikufy {

/*
 * ============================================================================
 * ProcessLauncher::Impl 内部实现类
 * ============================================================================
 */

struct ProcessLauncher::Impl {
	uint32_t detection_timeout_ms;	/* 检测超时时间（毫秒） */

	/* 常见GUI程序白名单，用于快速判断 */
	static const std::vector<std::string> gui_programs;

	Impl() : detection_timeout_ms(200) {}

	/**
	 * is_known_gui_program - 快速判断是否是已知GUI程序
	 * @command: 命令字符串
	 * @return bool: 是否是已知GUI程序
	 */
	bool is_known_gui_program(const std::string &command) const
	{
		/* 提取命令的第一个单词（程序名） */
		size_t first_space = command.find_first_of(" \t");
		std::string program_name = command.substr(0, first_space);

		/* 提取程序名中的最后一个路径部分 */
		size_t last_slash = program_name.find_last_of('/');
		if (last_slash != std::string::npos) {
			program_name = program_name.substr(last_slash + 1);
		}

		/* 检查是否在白名单中 */
		for (const auto &name : gui_programs) {
			if (program_name == name)
				return true;
		}

		return false;
	}

	/**
	 * detect_x11_window - 检测X11窗口创建
	 *
	 * @param pid: 进程ID
	 * @return bool: 是否检测到X11窗口
	 */
	bool detect_x11_window(pid_t pid)
	{
#ifdef HAVE_X11
		Display *display = XOpenDisplay(nullptr);
		if (!display)
			return false;

		Window root = DefaultRootWindow(display);
		Atom net_wm_pid = XInternAtom(display, "_NET_WM_PID", True);

		if (net_wm_pid == None) {
			XCloseDisplay(display);
			return false;
		}

		/* 使用迭代方式枚举窗口树，限制深度为3层 */
		std::vector<std::pair<Window, int>> window_stack;
		window_stack.push_back({root, 0});

		while (!window_stack.empty()) {
			auto [win, depth] = window_stack.back();
			window_stack.pop_back();

			/* 限制枚举深度，避免遍历整个窗口树 */
			if (depth > 3)
				continue;

			/* 检查当前窗口的 PID */
			Atom type;
			int format;
			unsigned long nitems, bytes_after;
			unsigned char *prop = nullptr;

			if (XGetWindowProperty(display, win, net_wm_pid, 0, 1,
					       False, XA_CARDINAL, &type, &format,
					       &nitems, &bytes_after, &prop) ==
			    Success) {
				if (prop && type == XA_CARDINAL && nitems == 1) {
					unsigned long *pid_ptr = (unsigned long *)prop;
					if (*pid_ptr == (unsigned long)pid) {
						XFree(prop);
						XCloseDisplay(display);
						return true;
					}
				}
				if (prop)
					XFree(prop);
			}

			/* 添加子窗口到栈，增加深度计数 */
			Window parent, *children = nullptr;
			unsigned int nchildren;

			if (XQueryTree(display, win, &root, &parent,
				       &children, &nchildren)) {
				/* 限制每层最多检查20个子窗口 */
				unsigned int max_children = std::min(nchildren, 20u);
				for (unsigned int i = 0; i < max_children; i++) {
					window_stack.push_back({children[i], depth + 1});
				}
				if (children)
					XFree(children);
			}
		}

		XCloseDisplay(display);
		return false;
#else
		(void)pid;
		return false;
#endif
	}

	/**
	 * detect_wayland_connection - 检测Wayland连接
	 *
	 * @param pid: 进程ID
	 * @return bool: 是否检测到Wayland连接
	 */
	bool detect_wayland_connection(pid_t pid)
	{
		std::string fd_path = std::format("/proc/{}/fd", pid);

		DIR *dir = opendir(fd_path.c_str());
		if (!dir)
			return false;

		struct dirent *entry;
		bool has_wayland = false;

		while ((entry = readdir(dir)) != nullptr) {
			if (entry->d_type != DT_LNK && entry->d_type != DT_UNKNOWN)
				continue;

			std::string link_path =
				std::format("{}/{}", fd_path, entry->d_name);
			char target[256] = {0};
			ssize_t len = readlink(link_path.c_str(), target,
					       sizeof(target) - 1);

			if (len > 0) {
				std::string target_str(target, len);
				if (target_str.find("wayland-0") != std::string::npos ||
				    target_str.find("wl_display") != std::string::npos) {
					has_wayland = true;
					break;
				}
			}
		}

		closedir(dir);
		return has_wayland;
	}

	/**
	 * spawn_in_pty - 在PTY中启动进程
	 *
	 * @param executable: 可执行文件路径
	 * @param args: 参数列表
	 * @param working_dir: 工作目录
	 * @param pty_fd_out: 输出参数，PTY文件描述符
	 *
	 * @return std::expected<pid_t, std::string>: PID或错误信息
	 */
	std::expected<pid_t, std::string> spawn_in_pty(
		const std::string &executable,
		const std::vector<std::string> &args,
		const std::string &working_dir,
		int *pty_fd_out)
	{
		int pty_fd;
		pid_t pid = forkpty(&pty_fd, nullptr, nullptr, nullptr);

		if (pid < 0)
			return std::unexpected(std::format("forkpty failed: {}",
							   strerror(errno)));

		if (pid == 0) {
			/* 子进程 */
			setsid();

			/* 切换工作目录 */
			if (!working_dir.empty() && working_dir != "/")
				if (chdir(working_dir.c_str()) < 0)
					_exit(127);

			/* 准备execve参数 */
			std::vector<char *> argv;
			argv.push_back(strdup(executable.c_str()));
			for (const auto &arg : args)
				argv.push_back(strdup(arg.c_str()));
			argv.push_back(nullptr);

			/* 执行 */
			execvp(executable.c_str(), argv.data());

			/* 如果execve失败，清理并退出 */
			for (char *arg : argv)
				if (arg)
					free(arg);
			_exit(127);
		}

		/* 设置PTY为非阻塞 */
		int flags = fcntl(pty_fd, F_GETFL, 0);
		fcntl(pty_fd, F_SETFL, flags | O_NONBLOCK);

		if (pty_fd_out)
			*pty_fd_out = pty_fd;

		return pid;
	}

	/**
	 * spawn_direct - 直接启动进程（无PTY）
	 *
	 * @param executable: 可执行文件路径
	 * @param args: 参数列表
	 * @param working_dir: 工作目录
	 * @return std::expected<pid_t, std::string>: PID或错误信息
	 */
	std::expected<pid_t, std::string> spawn_direct(
		const std::string &executable,
		const std::vector<std::string> &args,
		const std::string &working_dir)
	{
		pid_t pid = fork();

		if (pid < 0)
			return std::unexpected(std::format("fork failed: {}",
							   strerror(errno)));

		if (pid == 0) {
			/* 子进程 */
			setsid();

			/* 切换工作目录 */
			if (!working_dir.empty() && working_dir != "/")
				if (chdir(working_dir.c_str()) < 0)
					_exit(127);

			/* 准备execve参数 */
			std::vector<char *> argv;
			argv.push_back(strdup(executable.c_str()));
			for (const auto &arg : args)
				argv.push_back(strdup(arg.c_str()));
			argv.push_back(nullptr);

			/* 关闭所有文件描述符（除了stdin/stdout/stderr） */
			for (int fd = 3; fd < sysconf(_SC_OPEN_MAX); fd++)
				close(fd);

			/* 重定向stdin/stdout/stderr到/dev/null */
			freopen("/dev/null", "r", stdin);
			freopen("/dev/null", "w", stdout);
			freopen("/dev/null", "w", stderr);

			/* 执行 */
			execvp(executable.c_str(), argv.data());

			/* 如果execve失败，清理并退出 */
			for (char *arg : argv)
				if (arg)
					free(arg);
			_exit(127);
		}

		return pid;
	}

	/**
	 * wait_for_process - 等待进程结束
	 *
	 * @param pid: 进程ID
	 * @param timeout_ms: 超时时间（毫秒）
	 * @return std::expected<int, std::string>: 退出码或错误信息
	 */
	std::expected<int, std::string> wait_for_process(pid_t pid,
							  uint32_t timeout_ms)
	{
		auto start_time = std::chrono::steady_clock::now();

		while (true) {
			int status;
			pid_t result = waitpid(pid, &status, WNOHANG);

			if (result > 0) {
				/* 进程已结束 */
				if (WIFEXITED(status))
					return WEXITSTATUS(status);
				if (WIFSIGNALED(status))
					return 128 + WTERMSIG(status);
				return -1;
			}

			if (result == -1 && errno == ECHILD)
				return -1;

			/* 检查超时 */
			auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
					       std::chrono::steady_clock::now() -
					       start_time)
					       .count();
			if (elapsed >= timeout_ms)
				return std::unexpected("Timeout waiting for process");

			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	}
};

/* GUI程序白名单定义 */
const std::vector<std::string> ProcessLauncher::Impl::gui_programs = {
	"firefox", "chrome", "chromium", "chromium-browser",
	"google-chrome", "brave", "vivaldi", "opera", "edge",
	"nautilus", "dolphin", "thunar", "pcmanfm", "caja",
	"gedit", "kate", "kwrite", "mousepad", "leafpad",
	"eog", "feh", "ristretto", "nomacs", "geeqie",
	"vlc", "mpv", "totem", "smplayer", "parole",
	"evince", "okular", "mupdf", "zathura", "xpdf",
	"libreoffice", "soffice", "writer", "calc", "impress",
	"gimp", "inkscape", "krita", "shotwell",
	"blender", "krita", "inkscape",
	"thunderbird", "evolution", "geary",
	"discord", "telegram-desktop", "slack", "teams",
	"vscode", "code", "vim", "gvim", "neovim", "nvim-qt",
	"sublime_text", "atom", "brackets",
	"qtcreator", "kdevelop", "anjuta",
	"codeblocks", "geany", "mousepad",
	"filezilla", "transmission-gtk", "qbittorrent",
	"steam", "lutris", "heroic", "protonup-qt",
	"obsidian", "notion-app", "anytype"
};

/*
 * ============================================================================
 * ProcessLauncher 公共接口实现
 * ============================================================================
 */

/**
 * ProcessLauncher::ProcessLauncher - 构造函数
 */
ProcessLauncher::ProcessLauncher() : pImpl_(std::make_unique<Impl>())
{
}

/**
 * ProcessLauncher::~ProcessLauncher - 析构函数
 */
ProcessLauncher::~ProcessLauncher() = default;

/**
 * ProcessLauncher::launch_with_detection - 检测并启动进程
 */
std::expected<LaunchResult, std::string> ProcessLauncher::launch_with_detection(
	const std::string &executable,
	const std::vector<std::string> &args,
	const std::string &working_dir)
{
	LaunchResult result;

	/* 步骤1: 在PTY中启动进程进行检测 */
	int pty_fd;
	auto detection_pid = pImpl_->spawn_in_pty(executable, args, working_dir,
						  &pty_fd);

	if (!detection_pid)
		return std::unexpected(detection_pid.error());

	result.pid = *detection_pid;
	result.pty_fd = pty_fd;

	auto start_time = std::chrono::steady_clock::now();
	bool is_gui = false;

	/* 步骤2: 在超时时间内检测窗口创建 */
	while (true) {
		/* 检查进程是否已退出 */
		int status;
		pid_t wait_result = waitpid(result.pid, &status, WNOHANG);

		if (wait_result == result.pid) {
			/* 进程已退出，说明是CLI程序（快速退出） */
			result.type = ProcessType::CLI;
			result.success = true;
			return result;
		}

		if (wait_result == -1 && errno == ECHILD) {
			result.type = ProcessType::CLI;
			result.success = true;
			return result;
		}

		/* 检测X11窗口 */
		if (pImpl_->detect_x11_window(result.pid)) {
			is_gui = true;
			break;
		}

		/* 检测Wayland连接 */
		if (pImpl_->detect_wayland_connection(result.pid)) {
			is_gui = true;
			break;
		}

		/* 检查超时 */
		auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
				       std::chrono::steady_clock::now() -
				       start_time)
				       .count();
		if (elapsed >= pImpl_->detection_timeout_ms)
			break;

		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	/* 步骤3: 根据检测结果处理 */
	if (is_gui) {
		/* GUI程序：终止PTY进程，重新用fork+execve启动 */
		close(result.pty_fd);
		result.pty_fd = -1;

		kill(result.pid, SIGTERM);
		pImpl_->wait_for_process(result.pid, 1000);
		kill(result.pid, SIGKILL);
		waitpid(result.pid, nullptr, WNOHANG);

		/* 重新启动GUI程序（无PTY） */
		auto gui_pid = pImpl_->spawn_direct(executable, args, working_dir);

		if (!gui_pid)
			return std::unexpected(gui_pid.error());

		result.pid = *gui_pid;
		result.pty_fd = -1;
		result.type = ProcessType::GUI;
		result.success = true;
	} else {
		/* CLI程序：使用已有的PTY */
		result.type = ProcessType::CLI;
		result.success = true;
	}

	return result;
}

/**
 * ProcessLauncher::set_detection_timeout - 设置GUI检测超时时间
 */
void ProcessLauncher::set_detection_timeout(uint32_t timeout_ms)
{
	pImpl_->detection_timeout_ms = timeout_ms;
}

/**
 * ProcessLauncher::get_detection_timeout - 获取GUI检测超时时间
 */
uint32_t ProcessLauncher::get_detection_timeout() const
{
	return pImpl_->detection_timeout_ms;
}

/**
 * ProcessLauncher::spawn_cli_in_pty - 直接在PTY中启动CLI程序
 */
std::expected<LaunchResult, std::string> ProcessLauncher::spawn_cli_in_pty(
	const std::string &executable,
	const std::vector<std::string> &args,
	const std::string &working_dir)
{
	LaunchResult result;

	int pty_fd;
	auto pid = pImpl_->spawn_in_pty(executable, args, working_dir, &pty_fd);

	if (!pid)
		return std::unexpected(pid.error());

	result.pid = *pid;
	result.pty_fd = pty_fd;
	result.type = ProcessType::CLI;
	result.success = true;

	return result;
}

/**
 * ProcessLauncher::spawn_gui_direct - 直接启动GUI程序
 */
std::expected<LaunchResult, std::string> ProcessLauncher::spawn_gui_direct(
	const std::string &executable,
	const std::vector<std::string> &args,
	const std::string &working_dir)
{
	LaunchResult result;

	auto pid = pImpl_->spawn_direct(executable, args, working_dir);

	if (!pid)
		return std::unexpected(pid.error());

	result.pid = *pid;
	result.pty_fd = -1;
	result.type = ProcessType::GUI;
	result.success = true;

	return result;
}

} /* namespace mikufy */
