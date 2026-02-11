/*
 * Mikufy v2.11-nova - 终端管理器头文件
 *
 * 本文件定义了TerminalManager类的接口，该类负责管理交互式终端进程。
 * 使用PTY（伪终端）、epoll事件驱动和非阻塞I/O，支持交互式程序（vim、bash等）。
 *
 * 核心特性:
 * - PTY支持：forkpty创建伪终端，正确处理终端控制序列
 * - epoll事件驱动：高性能I/O多路复用，支持大量并发进程
 * - 非阻塞I/O：所有I/O操作都设置为非阻塞模式
 * - 独立IO线程：使用std::jthread持续读取进程输出，不阻塞UI线程
 * - 环形缓冲区：高效管理高频输出，支持万行级别输出
 * - RAII资源管理：自动清理资源，无内存泄漏
 * - C++23特性：std::expected, std::jthread, concepts, std::format
 *
 * MiraTrive/MikuTrive
 * Author: [Your Name]
 *
 * 本文件遵循Linux内核代码风格规范
 */

#ifndef MIKUFY_TERMINAL_MANAGER_H
#define MIKUFY_TERMINAL_MANAGER_H

/*
 * ============================================================================
 * 头文件包含
 * ============================================================================
 */

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <expected>
#include <format>
#include <ranges>
#include <concepts>
#include <array>

/*
 * ============================================================================
 * 系统头文件
 * ============================================================================
 */

#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <termios.h>
#include <pty.h>
#include <time.h>

/*
 * ============================================================================
 * 常量定义
 * ============================================================================
 */

/* 最大事件数 */
#define MAX_EPOLL_EVENTS	64

/* 缓冲区大小（环形缓冲区） */
#define RING_BUFFER_SIZE	(1 << 20) /* 1MB */

/* 超时时间（毫秒） */
#define EPOLL_TIMEOUT_MS	100

/* 终端尺寸 */
#define TERMINAL_DEFAULT_COLS	80
#define TERMINAL_DEFAULT_ROWS	24

/*
 * ============================================================================
 * 前向声明
 * ============================================================================
 */

class TerminalManager;

/*
 * ============================================================================
 * 数据结构定义
 * ============================================================================
 */

/**
 * TerminalSize - 终端尺寸结构体
 */
struct TerminalSize {
	int cols;	/* 列数 */
	int rows;	/* 行数 */

	TerminalSize() : cols(TERMINAL_DEFAULT_COLS), rows(TERMINAL_DEFAULT_ROWS) {}
	TerminalSize(int c, int r) : cols(c), rows(r) {}
};

/**
 * TerminalOutput - 终端输出数据
 */
struct TerminalOutput {
	std::string stdout_data;	/* 标准输出 */
	std::string stderr_data;	/* 标准错误 */
	bool is_eof;			/* 是否到达EOF */
	bool is_error;			/* 是否发生错误 */
};

/**
 * ProcessInfo - 进程信息
 */
struct ProcessInfo {
	pid_t pid;			/* 进程ID */
	int pty_fd;			/* PTY文件描述符 */
	std::string command;		/* 执行的命令 */
	std::string working_dir;	/* 工作目录 */
	TerminalSize size;		/* 终端尺寸 */
	bool is_running;		/* 是否正在运行 */
	time_t start_time;		/* 启动时间 */
	int exit_code;			/* 退出码 */

	ProcessInfo() : pid(-1), pty_fd(-1), is_running(false),
			start_time(0), exit_code(-1) {}
};

/*
 * ============================================================================
 * RingBuffer类 - 环形缓冲区
 * ============================================================================
 */

/**
 * RingBuffer - 固定大小环形缓冲区
 *
 * 使用无锁设计，支持高效的高频数据写入和读取。
 */
class RingBuffer {
public:
	RingBuffer() : head_(0), tail_(0), full_(false) {}

	/**
	 * write - 写入数据到缓冲区
	 */
	size_t write(const char *data, size_t size);

	/**
	 * read - 从缓冲区读取数据
	 */
	size_t read(char *buffer, size_t size);

	/**
	 * available - 获取可读取的字节数
	 */
	size_t available() const;

	/**
	 * capacity - 获取缓冲区容量
	 */
	size_t capacity() const;

	/**
	 * clear - 清空缓冲区
	 */
	void clear();

private:
	std::array<char, RING_BUFFER_SIZE> buffer_;
	size_t head_;		/* 写指针 */
	size_t tail_;		/* 读指针 */
	bool full_;		/* 是否已满 */
};

/*
 * ============================================================================
 * TerminalProcess类 - 终端进程
 * ============================================================================
 */

/**
 * TerminalProcess - 终端进程管理类
 *
 * 使用RAII管理进程资源，自动清理。
 */
class TerminalProcess {
public:
	TerminalProcess(pid_t pid, int pty_fd, const std::string &command,
			const std::string &working_dir);
	~TerminalProcess();

	/* 禁止拷贝 */
	TerminalProcess(const TerminalProcess &) = delete;
	TerminalProcess &operator=(const TerminalProcess &) = delete;

	/* 禁止移动 */
	TerminalProcess(TerminalProcess &&) = delete;
	TerminalProcess &operator=(TerminalProcess &&) = delete;

	/**
	 * set_size - 设置终端尺寸
	 */
	void set_size(const TerminalSize &size);

	/**
	 * get_size - 获取终端尺寸
	 */
	TerminalSize get_size() const;

	/**
	 * send_input - 发送输入到进程
	 */
	std::expected<void, std::string> send_input(const std::string &input);

	/**
	 * read_output - 读取输出（非阻塞）
	 */
	std::expected<TerminalOutput, std::string> read_output();

	/**
	 * read_from_pty - 从PTY读取数据到缓冲区（由IO线程调用）
	 */
	void read_from_pty();

	/**
	 * is_running - 检查进程是否运行
	 */
	bool is_running() const;

	/**
	 * get_pid - 获取进程ID
	 */
	pid_t get_pid() const;

	/**
	 * get_pty_fd - 获取PTY文件描述符
	 */
	int get_pty_fd() const;

	/**
	 * get_command - 获取执行的命令
	 */
	const std::string &get_command() const;

	/**
	 * get_working_dir - 获取工作目录
	 */
	const std::string &get_working_dir() const;

	/**
	 * terminate - 终止进程
	 */
	void terminate();

	/**
	 * kill_process - 强制杀死进程
	 */
	void kill_process();

private:
	mutable ProcessInfo info_;
	RingBuffer output_buffer_;
	RingBuffer error_buffer_;
	mutable std::mutex mutex_;

	/**
	 * set_nonblocking - 设置文件描述符为非阻塞
	 */
	void set_nonblocking(int fd);
};

/*
 * ============================================================================
 * TerminalManager类 - 终端管理器
 * ============================================================================
 */

/**
 * TerminalManager - 终端管理器
 *
 * 管理所有终端进程，使用epoll事件驱动和独立IO线程。
 */
class TerminalManager {
public:
	/**
	 * OutputCallback - 输出回调函数类型
	 */
	using OutputCallback = std::function<void(pid_t, const TerminalOutput &)>;

	TerminalManager();
	~TerminalManager();

	/* 禁止拷贝 */
	TerminalManager(const TerminalManager &) = delete;
	TerminalManager &operator=(const TerminalManager &) = delete;

	/* 禁止移动 */
	TerminalManager(TerminalManager &&) = delete;
	TerminalManager &operator=(TerminalManager &&) = delete;

	/**
	 * start - 启动终端管理器
	 */
	std::expected<void, std::string> start();

	/**
	 * stop - 停止终端管理器
	 */
	void stop();

	/**
	 * is_running - 检查是否运行
	 */
	bool is_running() const;

	/**
	 * execute_command - 执行命令
	 */
	std::expected<pid_t, std::string> execute_command(
			const std::string &command,
			const std::string &working_dir = "/");

	/**
	 * send_input - 发送输入到进程
	 */
	std::expected<void, std::string> send_input(
			pid_t pid, const std::string &input);

	/**
	 * get_output - 获取进程输出
	 */
	std::expected<TerminalOutput, std::string> get_output(pid_t pid);

	/**
	 * set_terminal_size - 设置终端尺寸
	 */
	std::expected<void, std::string> set_terminal_size(
			pid_t pid, const TerminalSize &size);

	/**
	 * terminate_process - 终止进程
	 */
	std::expected<void, std::string> terminate_process(pid_t pid);

	/**
	 * kill_process - 杀死进程
	 */
	std::expected<void, std::string> kill_process(pid_t pid);

	/**
	 * get_process_info - 获取进程信息
	 */
	std::expected<ProcessInfo, std::string> get_process_info(pid_t pid);

	/**
	 * get_all_processes - 获取所有进程信息
	 */
	std::vector<ProcessInfo> get_all_processes();

	/**
	 * set_output_callback - 设置输出回调
	 */
	void set_output_callback(OutputCallback callback);

private:
	/* epoll文件描述符 */
	int epoll_fd_;

	/* 运行标志 */
	std::atomic<bool> running_;

	/* IO线程 */
	std::jthread io_thread_;

	/* 进程映射表 */
	std::unordered_map<pid_t, std::unique_ptr<TerminalProcess>> processes_;

	/* 互斥锁 */
	std::mutex processes_mutex_;

	/* 输出回调 */
	OutputCallback output_callback_;

	/**
	 * io_thread_func - IO线程函数
	 */
	void io_thread_func(std::stop_token token);

	/**
	 * handle_epoll_events - 处理epoll事件
	 */
	void handle_epoll_events();

	/**
	 * cleanup_finished_processes - 清理已完成的进程
	 */
	void cleanup_finished_processes();

	/**
	 * setup_signal_handler - 设置SIGCHLD信号处理器
	 */
	void setup_signal_handler();
};

/*
 * ============================================================================
 * 新增命名空间：智能进程启动
 * ============================================================================
 */

namespace mikufy::smart_process {

/**
 * ExecutableCommand - 解析后的可执行命令
 */
struct ExecutableCommand {
	std::string executable;		/* 可执行文件路径 */
	std::vector<std::string> args; /* 参数列表 */
	bool is_valid;			/* 是否有效 */

	ExecutableCommand() : is_valid(false) {}
};

/**
 * parse_executable_command - 解析 ./xxx 命令
 *
 * @param command: 命令字符串
 * @return ExecutableCommand: 解析结果
 */
ExecutableCommand parse_executable_command(const std::string &command);

/**
 * launch_with_detection - 检测并启动进程
 *
 * @param command: 命令字符串
 * @param working_dir: 工作目录
 * @return std::expected<pid_t, std::string>: PID或错误信息
 */
std::expected<pid_t, std::string> launch_with_detection(
	const std::string &command,
	const std::string &working_dir = "/");

} /* namespace mikufy::smart_process */

#endif /* MIKUFY_TERMINAL_MANAGER_H */
