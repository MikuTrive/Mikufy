/*
 * Mikufy v2.11-nova - 终端管理器实现
 *
 * 本文件实现了TerminalManager类，提供完整的终端进程管理功能。
 *
 * 核心实现:
 * - 使用forkpty创建伪终端
 * - 使用epoll进行事件驱动I/O
 * - 独立IO线程持续读取进程输出
 * - 环形缓冲区高效管理数据
 * - RAII资源管理
 *
 * MiraTrive/MikuTrive
 * Author: [Your Name]
 *
 * 本文件遵循Linux内核代码风格规范
 */

#include "../headers/terminal_manager.h"
#include <iostream>
#include <cstring>
#include <cerrno>
#include <cstdlib>
#include <algorithm>

/*
 * ============================================================================
 * RingBuffer实现
 * ============================================================================
 */

/**
 * RingBuffer::write - 写入数据到缓冲区
 */
size_t RingBuffer::write(const char *data, size_t size)
{
	size_t written = 0;

	while (written < size && !full_) {
		buffer_[head_] = data[written];
		head_ = (head_ + 1) % RING_BUFFER_SIZE;

		if (head_ == tail_)
			full_ = true;

		written++;
	}

	return written;
}

/**
 * RingBuffer::read - 从缓冲区读取数据
 */
size_t RingBuffer::read(char *buffer, size_t size)
{
	size_t read_count = 0;

	while (read_count < size && (!full_ || head_ != tail_)) {
		buffer[read_count] = buffer_[tail_];
		tail_ = (tail_ + 1) % RING_BUFFER_SIZE;
		full_ = false;
		read_count++;
	}

	return read_count;
}

/**
 * RingBuffer::available - 获取可读取的字节数
 */
size_t RingBuffer::available() const
{
	if (full_)
		return RING_BUFFER_SIZE;

	if (head_ >= tail_)
		return head_ - tail_;

	return RING_BUFFER_SIZE - tail_ + head_;
}

/**
 * RingBuffer::capacity - 获取缓冲区容量
 */
size_t RingBuffer::capacity() const
{
	return RING_BUFFER_SIZE;
}

/**
 * RingBuffer::clear - 清空缓冲区
 */
void RingBuffer::clear()
{
	head_ = 0;
	tail_ = 0;
	full_ = false;
}

/*
 * ============================================================================
 * TerminalProcess实现
 * ============================================================================
 */

/**
 * TerminalProcess::TerminalProcess - 构造函数
 */
TerminalProcess::TerminalProcess(pid_t pid, int pty_fd,
				 const std::string &command,
				 const std::string &working_dir)
{
	info_.pid = pid;
	info_.pty_fd = pty_fd;
	info_.command = command;
	info_.working_dir = working_dir;
	info_.is_running = true;
	info_.start_time = time(nullptr);
	info_.exit_code = -1;

	set_nonblocking(pty_fd);
}

/**
 * TerminalProcess::~TerminalProcess - 析构函数
 */
TerminalProcess::~TerminalProcess()
{
	if (info_.pty_fd >= 0)
		close(info_.pty_fd);
}

/**
 * TerminalProcess::set_nonblocking - 设置文件描述符为非阻塞
 */
void TerminalProcess::set_nonblocking(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

/**
 * TerminalProcess::set_size - 设置终端尺寸
 */
void TerminalProcess::set_size(const TerminalSize &size)
{
	std::lock_guard<std::mutex> lock(mutex_);

	if (info_.pty_fd < 0)
		return;

	struct winsize ws;
	ws.ws_col = size.cols;
	ws.ws_row = size.rows;
	ws.ws_xpixel = 0;
	ws.ws_ypixel = 0;

	ioctl(info_.pty_fd, TIOCSWINSZ, &ws);

	info_.size = size;
}

/**
 * TerminalProcess::get_size - 获取终端尺寸
 */
TerminalSize TerminalProcess::get_size() const
{
	std::lock_guard<std::mutex> lock(mutex_);
	return info_.size;
}

/**
 * TerminalProcess::send_input - 发送输入到进程
 */
std::expected<void, std::string> TerminalProcess::send_input(const std::string &input)
{
	std::lock_guard<std::mutex> lock(mutex_);

	if (info_.pty_fd < 0)
		return std::unexpected("PTY file descriptor is invalid");

	if (!info_.is_running)
		return std::unexpected("Process is not running");

	ssize_t written = write(info_.pty_fd, input.c_str(), input.length());

	if (written < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return std::unexpected("Write would block");

		return std::unexpected(std::format("Write failed: {}", strerror(errno)));
	}

	if (static_cast<size_t>(written) != input.length())
		return std::unexpected("Partial write");

	return {};
}

/**
 * TerminalProcess::read_output - 读取输出（非阻塞）
 *
 * 从环形缓冲区读取已缓存的数据，如果缓冲区为空则尝试从PTY读取一次。
 */
std::expected<TerminalOutput, std::string> TerminalProcess::read_output()
{
	std::lock_guard<std::mutex> lock(mutex_);

	TerminalOutput output;

	if (info_.pty_fd < 0) {
		output.is_eof = true;
		return output;
	}

	/* 如果缓冲区为空，尝试从PTY读取一次 */
	if (output_buffer_.available() == 0) {
		char buffer[8192];
		ssize_t bytes_read;

		bytes_read = read(info_.pty_fd, buffer, sizeof(buffer));

		if (bytes_read > 0) {
			output_buffer_.write(buffer, bytes_read);
		} else if (bytes_read == 0) {
			output.is_eof = true;
			return output;
		} else if (errno != EAGAIN && errno != EWOULDBLOCK) {
			output.is_error = true;
			return output;
		}
	}

	/* 从环形缓冲区读取数据 */
	if (output_buffer_.available() > 0) {
		std::vector<char> data(output_buffer_.available());
		size_t read_count = output_buffer_.read(data.data(), data.size());
		output.stdout_data.assign(data.data(), data.data() + read_count);
	}

	return output;
}

/**
 * TerminalProcess::read_from_pty - 从PTY读取数据到缓冲区（由IO线程调用）
 *
 * 这个函数只从PTY读取数据到环形缓冲区，不从缓冲区读取。
 */
void TerminalProcess::read_from_pty()
{
	std::lock_guard<std::mutex> lock(mutex_);

	if (info_.pty_fd < 0)
		return;

	char buffer[8192];
	ssize_t bytes_read;

	while (true) {
		bytes_read = read(info_.pty_fd, buffer, sizeof(buffer));

		if (bytes_read < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				break;
			break;
		}

		if (bytes_read == 0)
			break;

		output_buffer_.write(buffer, bytes_read);
	}
}

/**
 * TerminalProcess::is_running - 检查进程是否运行
 */
bool TerminalProcess::is_running() const
{
	std::lock_guard<std::mutex> lock(mutex_);

	if (!info_.is_running)
		return false;

	int status;
	pid_t result = waitpid(info_.pid, &status, WNOHANG);

	if (result > 0) {
		info_.is_running = false;
		info_.exit_code = WEXITSTATUS(status);
		return false;
	}

	if (result == -1 && errno == ECHILD) {
		info_.is_running = false;
		return false;
	}

	return true;
}

/**
 * TerminalProcess::get_pid - 获取进程ID
 */
pid_t TerminalProcess::get_pid() const
{
	std::lock_guard<std::mutex> lock(mutex_);
	return info_.pid;
}

/**
 * TerminalProcess::get_pty_fd - 获取PTY文件描述符
 */
int TerminalProcess::get_pty_fd() const
{
	std::lock_guard<std::mutex> lock(mutex_);
	return info_.pty_fd;
}

/**
 * TerminalProcess::get_command - 获取执行的命令
 */
const std::string &TerminalProcess::get_command() const
{
	std::lock_guard<std::mutex> lock(mutex_);
	return info_.command;
}

/**
 * TerminalProcess::get_working_dir - 获取工作目录
 */
const std::string &TerminalProcess::get_working_dir() const
{
	std::lock_guard<std::mutex> lock(mutex_);
	return info_.working_dir;
}

/**
 * TerminalProcess::terminate - 终止进程
 */
void TerminalProcess::terminate()
{
	std::lock_guard<std::mutex> lock(mutex_);

	if (info_.pid > 0) {
		::kill(info_.pid, SIGTERM);
		info_.is_running = false;
	}
}

/**
 * TerminalProcess::kill_process - 强制杀死进程
 */
void TerminalProcess::kill_process()
{
	std::lock_guard<std::mutex> lock(mutex_);

	if (info_.pid > 0) {
		::kill(info_.pid, SIGKILL);
		info_.is_running = false;
	}
}

/*
 * ============================================================================
 * TerminalManager实现
 * ============================================================================
 */

/**
 * TerminalManager::TerminalManager - 构造函数
 */
TerminalManager::TerminalManager()
	: epoll_fd_(-1), running_(false)
{
}

/**
 * TerminalManager::~TerminalManager - 析构函数
 */
TerminalManager::~TerminalManager()
{
	stop();
}

/**
 * TerminalManager::start - 启动终端管理器
 */
std::expected<void, std::string> TerminalManager::start()
{
	if (running_)
		return std::unexpected("TerminalManager is already running");

	epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);

	if (epoll_fd_ < 0)
		return std::unexpected(std::format("epoll_create1 failed: {}", strerror(errno)));

	setup_signal_handler();

	running_ = true;

	io_thread_ = std::jthread([this](std::stop_token token) {
		io_thread_func(token);
	});

	return {};
}

/**
 * TerminalManager::stop - 停止终端管理器
 */
void TerminalManager::stop()
{
	if (!running_)
		return;

	running_ = false;

	if (io_thread_.joinable())
		io_thread_.join();

	std::lock_guard<std::mutex> lock(processes_mutex_);

	for (auto &[pid, process] : processes_) {
		process->terminate();
		processes_.erase(pid);
	}

	if (epoll_fd_ >= 0) {
		close(epoll_fd_);
		epoll_fd_ = -1;
	}
}

/**
 * TerminalManager::is_running - 检查是否运行
 */
bool TerminalManager::is_running() const
{
	return running_;
}

/**
 * TerminalManager::execute_command - 执行命令
 */
std::expected<pid_t, std::string> TerminalManager::execute_command(
	const std::string &command, const std::string &working_dir)
{
	if (!running_)
		return std::unexpected("TerminalManager is not running");

	int pty_fd;
	pid_t pid = forkpty(&pty_fd, nullptr, nullptr, nullptr);

	if (pid < 0)
		return std::unexpected(std::format("forkpty failed: {}", strerror(errno)));

	if (pid == 0) {
		/* 重置 SIGCHLD 信号处理，允许子进程创建子进程 */
		struct sigaction sa;
		sa.sa_handler = SIG_DFL;
		sigemptyset(&sa.sa_mask);
		sa.sa_flags = 0;
		sigaction(SIGCHLD, &sa, nullptr);

		if (!working_dir.empty() && working_dir != "/")
			chdir(working_dir.c_str());

		execl("/bin/sh", "sh", "-c", command.c_str(), nullptr);
		_exit(127);
	}

	auto process = std::make_unique<TerminalProcess>(pid, pty_fd,
							 command, working_dir);

	struct epoll_event ev;
	ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
	ev.data.fd = pty_fd;

	if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, pty_fd, &ev) < 0) {
		close(pty_fd);
		kill(pid, SIGKILL);
		return std::unexpected(std::format("epoll_ctl failed: {}", strerror(errno)));
	}

	std::lock_guard<std::mutex> lock(processes_mutex_);
	processes_[pid] = std::move(process);

	return pid;
}

/**
 * TerminalManager::send_input - 发送输入到进程
 */
std::expected<void, std::string> TerminalManager::send_input(
	pid_t pid, const std::string &input)
{
	std::lock_guard<std::mutex> lock(processes_mutex_);

	auto it = processes_.find(pid);

	if (it == processes_.end())
		return std::unexpected("Process not found");

	return it->second->send_input(input);
}

/**
 * TerminalManager::get_output - 获取进程输出
 */
std::expected<TerminalOutput, std::string> TerminalManager::get_output(pid_t pid)
{
	std::lock_guard<std::mutex> lock(processes_mutex_);

	auto it = processes_.find(pid);

	if (it == processes_.end())
		return std::unexpected("Process not found");

	return it->second->read_output();
}

/**
 * TerminalManager::set_terminal_size - 设置终端尺寸
 */
std::expected<void, std::string> TerminalManager::set_terminal_size(
	pid_t pid, const TerminalSize &size)
{
	std::lock_guard<std::mutex> lock(processes_mutex_);

	auto it = processes_.find(pid);

	if (it == processes_.end())
		return std::unexpected("Process not found");

	it->second->set_size(size);

	return {};
}

/**
 * TerminalManager::terminate_process - 终止进程
 */
std::expected<void, std::string> TerminalManager::terminate_process(pid_t pid)
{
	std::lock_guard<std::mutex> lock(processes_mutex_);

	auto it = processes_.find(pid);

	if (it == processes_.end())
		return std::unexpected("Process not found");

	it->second->terminate();

	return {};
}

/**
 * TerminalManager::kill_process - 杀死进程
 */
std::expected<void, std::string> TerminalManager::kill_process(pid_t pid)
{
	std::lock_guard<std::mutex> lock(processes_mutex_);

	auto it = processes_.find(pid);

	if (it == processes_.end())
		return std::unexpected("Process not found");

	it->second->kill_process();

	return {};
}

/**
 * TerminalManager::get_process_info - 获取进程信息
 */
std::expected<ProcessInfo, std::string> TerminalManager::get_process_info(pid_t pid)
{
	std::lock_guard<std::mutex> lock(processes_mutex_);

	auto it = processes_.find(pid);

	if (it == processes_.end())
		return std::unexpected("Process not found");

	ProcessInfo info;
	info.pid = it->second->get_pid();
	info.command = it->second->get_command();
	info.working_dir = it->second->get_working_dir();
	info.size = it->second->get_size();
	info.is_running = it->second->is_running();

	return info;
}

/**
 * TerminalManager::get_all_processes - 获取所有进程信息
 */
std::vector<ProcessInfo> TerminalManager::get_all_processes()
{
	std::lock_guard<std::mutex> lock(processes_mutex_);

	std::vector<ProcessInfo> result;

	for (const auto &[pid, process] : processes_) {
		ProcessInfo info;
		info.pid = process->get_pid();
		info.command = process->get_command();
		info.working_dir = process->get_working_dir();
		info.size = process->get_size();
		info.is_running = process->is_running();
		result.push_back(info);
	}

	return result;
}

/**
 * TerminalManager::set_output_callback - 设置输出回调
 */
void TerminalManager::set_output_callback(OutputCallback callback)
{
	output_callback_ = callback;
}

/**
 * TerminalManager::io_thread_func - IO线程函数
 */
void TerminalManager::io_thread_func(std::stop_token token)
{
	while (running_ && !token.stop_requested()) {
		handle_epoll_events();
		cleanup_finished_processes();
	}
}

/**
 * TerminalManager::handle_epoll_events - 处理epoll事件
 */
void TerminalManager::handle_epoll_events()
{
	struct epoll_event events[MAX_EPOLL_EVENTS];

	int nfds = epoll_wait(epoll_fd_, events, MAX_EPOLL_EVENTS, EPOLL_TIMEOUT_MS);

	if (nfds < 0) {
		if (errno == EINTR)
			return;

		std::cerr << std::format("epoll_wait failed: {}", strerror(errno))
			  << std::endl;
		return;
	}

	for (int i = 0; i < nfds; ++i) {
		int fd = events[i].data.fd;

		std::lock_guard<std::mutex> lock(processes_mutex_);

		for (auto &[pid, process] : processes_) {
			if (process->get_pty_fd() == fd) {
						/* IO线程只从PTY读取数据到缓冲区，不调用read_output() */
						process->read_from_pty();
			
						/* 获取进程状态，检查是否结束 */
						bool is_eof = false;
						if (!process->is_running())
							is_eof = true;
			
						if (is_eof) {
							epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
						}
			
						break;
					}		}
	}
}

/**
 * TerminalManager::cleanup_finished_processes - 清理已完成的进程
 */
void TerminalManager::cleanup_finished_processes()
{
	std::lock_guard<std::mutex> lock(processes_mutex_);

	auto it = processes_.begin();

	while (it != processes_.end()) {
		if (!it->second->is_running()) {
			/* 不向用户输出进程信息 */
			/* std::cout << std::format("Process {} finished", it->first) << std::endl; */

			int fd = it->second->get_pty_fd();
			if (fd >= 0)
				epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);

			it = processes_.erase(it);
		} else {
			++it;
		}
	}
}

/**
 * TerminalManager::setup_signal_handler - 设置SIGCHLD信号处理器
 */
void TerminalManager::setup_signal_handler()
{
	struct sigaction sa;
	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_NOCLDSTOP | SA_NOCLDWAIT;

	sigaction(SIGCHLD, &sa, nullptr);
}

/*
 * ============================================================================
 * 智能进程启动实现
 * ============================================================================
 */

#include "../headers/process_launcher.h"
#include "../headers/terminal_window.h"
#include <regex>
#include <filesystem>
#include <thread>

namespace mikufy::smart_process {

/**
 * parse_executable_command - 解析 ./xxx 命令
 */
ExecutableCommand parse_executable_command(const std::string &command)
{
	ExecutableCommand result;

	/* 检查是否以 ./ 开头 */
	static std::regex exec_regex(R"(^\s*(\./[^/\s]+)(?:\s+(.*))?$)");
	std::smatch match;

	if (!std::regex_match(command, match, exec_regex))
		return result;

	result.executable = match[1].str();
	result.is_valid = true;

	/* 解析参数 */
	if (match[2].matched) {
		std::string args_str = match[2].str();

		/* 简单的参数分割（支持引号） */
		bool in_quotes = false;
		std::string current_arg;

		for (char c : args_str) {
			if (c == '"') {
				in_quotes = !in_quotes;
			} else if (c == ' ' && !in_quotes) {
				if (!current_arg.empty()) {
					result.args.push_back(current_arg);
					current_arg.clear();
				}
			} else {
				current_arg += c;
			}
		}

		if (!current_arg.empty())
			result.args.push_back(current_arg);
	}

	return result;
}

/**
 * launch_with_detection - 检测并启动进程
 */
std::expected<pid_t, std::string> launch_with_detection(
	const std::string &command,
	const std::string &working_dir)
{
	/* 检查是否是 ./xxx 命令 */
	auto parsed = parse_executable_command(command);

	if (parsed.is_valid) {
		/* 是 ./xxx 命令 */
		std::filesystem::path exec_path(parsed.executable);

		/* 首先尝试在工作目录中查找 */
		std::filesystem::path full_path = std::filesystem::path(working_dir) / exec_path;

		if (!std::filesystem::exists(full_path))
			return std::unexpected(std::format("File not found: {}",
							   parsed.executable));

		/* 获取绝对路径 */
		try {
			exec_path = std::filesystem::canonical(full_path);
		} catch (const std::filesystem::filesystem_error &e) {
			return std::unexpected(
				std::format("Cannot resolve path: {}", e.what()));
		}

		/* 检查是否可执行 */
		if (access(exec_path.c_str(), X_OK) != 0)
			return std::unexpected(std::format("Not executable: {}",
							   exec_path.c_str()));

		/* 防止路径穿越 */
		std::string exec_str = exec_path.string();
		if (exec_str.find("..") != std::string::npos)
			return std::unexpected("Path traversal detected");

		/* 创建进程启动器 */
		ProcessLauncher launcher;

		/* 检测并启动进程 */
		auto launch_result = launcher.launch_with_detection(
			exec_str, parsed.args, working_dir);

		if (!launch_result)
			return std::unexpected(launch_result.error());

		/* 根据检测结果处理 */
		if (launch_result->type == ProcessType::CLI) {
			/* CLI程序：使用 terminal_helper 独立程序运行 */
			/* 终止 PTY 进程 */
			close(launch_result->pty_fd);
			kill(launch_result->pid, SIGTERM);

			/* 快速等待 */
			for (int i = 0; i < 10; i++) {
				int status;
				pid_t result = waitpid(launch_result->pid, &status, WNOHANG);
				if (result == launch_result->pid)
					break;
				usleep(10000); /* 10ms * 10 = 100ms */
			}

			kill(launch_result->pid, SIGKILL);
			waitpid(launch_result->pid, nullptr, WNOHANG);

			/* 构建命令字符串 */
			std::string cmd = exec_str;
			for (const auto &arg : parsed.args) {
				cmd += " " + arg;
			}

			/* 启动 terminal_helper 独立程序 */
			pid_t helper_pid = fork();

			if (helper_pid < 0)
				return std::unexpected("fork failed");

			if (helper_pid == 0) {
				/* 子进程：执行 terminal_helper */
				/* 优先从当前目录查找，然后检查系统安装路径 */
				const char *helper_path = nullptr;

				if (access("./terminal_helper", X_OK) == 0) {
					helper_path = "./terminal_helper";
				} else if (access("/usr/local/bin/terminal_helper", X_OK) == 0) {
					helper_path = "/usr/local/bin/terminal_helper";
				} else if (access("/usr/bin/terminal_helper", X_OK) == 0) {
					helper_path = "/usr/bin/terminal_helper";
				} else if (access("/usr/share/mikufy/terminal_helper", X_OK) == 0) {
					/* RPM/DEB包安装路径 */
					helper_path = "/usr/share/mikufy/terminal_helper";
				} else {
					/* 用户级安装路径 */
					const char *home = getenv("HOME");
					if (home) {
						char user_path[512];
						snprintf(user_path, sizeof(user_path), "%s/.local/share/MIKUFY/terminal_helper", home);
						if (access(user_path, X_OK) == 0) {
							helper_path = user_path;
						} else if (access("/opt/mikufy/terminal_helper", X_OK) == 0) {
							/* 可选安装路径 */
							helper_path = "/opt/mikufy/terminal_helper";
						} else {
							_exit(127);
						}
					} else {
						_exit(127);
					}
				}

				/* 使用 posix_spawn 加速启动 */
				char *argv[] = { (char *)"terminal_helper",
						(char *)cmd.c_str(),
						(char *)working_dir.c_str(),
						nullptr };
				execv(helper_path, argv);
				_exit(127);
			}

			/* 返回 terminal_helper 的 PID */
			return helper_pid;
		} else {
			/* GUI程序：已直接启动，返回PID */
			return launch_result->pid;
		}
	} else {
		/* 不是 ./xxx 命令，直接使用 terminal_helper 运行 */
		/* 比如：python3 hello.py, ls -la 等命令 */

		/* 启动 terminal_helper 独立程序 */
		pid_t helper_pid = fork();

		if (helper_pid < 0)
			return std::unexpected("fork failed");

		if (helper_pid == 0) {
			/* 子进程：执行 terminal_helper */
			/* 优先从当前目录查找，然后检查系统安装路径 */
			const char *helper_path = nullptr;

			if (access("./terminal_helper", X_OK) == 0) {
				helper_path = "./terminal_helper";
			} else if (access("/usr/local/bin/terminal_helper", X_OK) == 0) {
				helper_path = "/usr/local/bin/terminal_helper";
			} else if (access("/usr/bin/terminal_helper", X_OK) == 0) {
				helper_path = "/usr/bin/terminal_helper";
			} else if (access("/usr/share/mikufy/terminal_helper", X_OK) == 0) {
				/* RPM/DEB包安装路径 */
				helper_path = "/usr/share/mikufy/terminal_helper";
			} else {
				/* 用户级安装路径 */
				const char *home = getenv("HOME");
				if (home) {
					char user_path[512];
					snprintf(user_path, sizeof(user_path), "%s/.local/share/MIKUFY/terminal_helper", home);
					if (access(user_path, X_OK) == 0) {
						helper_path = user_path;
					} else if (access("/opt/mikufy/terminal_helper", X_OK) == 0) {
						/* 可选安装路径 */
						helper_path = "/opt/mikufy/terminal_helper";
					} else {
						_exit(127);
					}
				} else {
					_exit(127);
				}
			}

			/* 使用 posix_spawn 加速启动 */
			char *argv[] = { (char *)"terminal_helper",
					(char *)command.c_str(),
					(char *)working_dir.c_str(),
					nullptr };
			execv(helper_path, argv);
			_exit(127);
		}

		return helper_pid;
	}
}

} /* namespace mikufy::smart_process */
