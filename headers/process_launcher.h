/*
 * Mikufy v2.11-nova - 进程启动器头文件
 *
 * 本文件定义了ProcessLauncher类的接口，用于智能启动进程并检测GUI程序。
 * 支持运行时检测（X11窗口创建 + Wayland连接检测），自动区分CLI和GUI程序。
 *
 * 核心特性:
 * - 运行时GUI检测：X11窗口检测 + Wayland连接检测
 * - 可配置超时时间（默认500ms）
 * - CLI程序：返回PTY文件描述符，用于终端显示
 * - GUI程序：简单fork+execve，无PTY
 * - RAII资源管理
 * - C++23特性：std::expected, std::optional, std::jthread
 *
 * MiraTrive/MikuTrive
 *
 * 本文件遵循Linux内核代码风格规范
 */

#ifndef MIKUFY_PROCESS_LAUNCHER_H
#define MIKUFY_PROCESS_LAUNCHER_H

/*
 * ============================================================================
 * 头文件包含
 * ============================================================================
 */

#include <string>
#include <vector>
#include <optional>
#include <expected>
#include <memory>
#include <functional>
#include <chrono>

/*
 * ============================================================================
 * 前向声明
 * ============================================================================
 */

namespace mikufy {

/*
 * ============================================================================
 * 枚举类型
 * ============================================================================
 */

/**
 * ProcessType - 进程类型
 */
enum class ProcessType {
	CLI,	/* CLI程序 */
	GUI,	/* GUI程序 */
	UNKNOWN	/* 未知类型 */
};

/*
 * ============================================================================
 * 数据结构
 * ============================================================================
 */

/**
 * LaunchResult - 启动结果
 */
struct LaunchResult {
	pid_t pid;			/* 进程ID */
	int pty_fd;			/* PTY文件描述符（仅CLI程序有效） */
	ProcessType type;		/* 进程类型 */
	bool success;			/* 是否成功启动 */
	std::string error_message;	/* 错误信息 */

	LaunchResult() : pid(-1), pty_fd(-1), type(ProcessType::UNKNOWN),
			 success(false) {}
};

/*
 * ============================================================================
 * ProcessLauncher类 - 进程启动器
 * ============================================================================
 */

/**
 * ProcessLauncher - 进程启动和GUI检测器
 *
 * 使用运行时检测方法区分CLI和GUI程序：
 * 1. 先在PTY中启动进程
 * 2. 在超时时间内检测X11窗口或Wayland连接
 * 3. 如果检测到GUI窗口：终止PTY进程，重新用fork+execve启动
 * 4. 如果未检测到GUI窗口：继续在PTY中运行
 */
class ProcessLauncher {
public:
	/**
	 * ProcessLauncher - 构造函数
	 */
	ProcessLauncher();

	/**
	 * ~ProcessLauncher - 析构函数
	 */
	~ProcessLauncher();

	/* 禁止拷贝 */
	ProcessLauncher(const ProcessLauncher &) = delete;
	ProcessLauncher &operator=(const ProcessLauncher &) = delete;

	/* 禁止移动 */
	ProcessLauncher(ProcessLauncher &&) = delete;
	ProcessLauncher &operator=(ProcessLauncher &&) = delete;

	/**
	 * launch_with_detection - 检测并启动进程
	 *
	 * @param executable: 可执行文件路径（绝对路径或相对路径）
	 * @param args: 参数列表
	 * @param working_dir: 工作目录
	 * @return LaunchResult: 启动结果，包含PID、PTY fd和进程类型
	 */
	std::expected<LaunchResult, std::string> launch_with_detection(
		const std::string &executable,
		const std::vector<std::string> &args,
		const std::string &working_dir = "/"
	);

	/**
	 * set_detection_timeout - 设置GUI检测超时时间
	 *
	 * @param timeout_ms: 超时时间（毫秒）
	 */
	void set_detection_timeout(uint32_t timeout_ms);

	/**
	 * get_detection_timeout - 获取GUI检测超时时间
	 *
	 * @return uint32_t: 超时时间（毫秒）
	 */
	uint32_t get_detection_timeout() const;

	/**
	 * spawn_cli_in_pty - 直接在PTY中启动CLI程序（跳过检测）
	 *
	 * @param executable: 可执行文件路径
	 * @param args: 参数列表
	 * @param working_dir: 工作目录
	 * @return LaunchResult: 启动结果
	 */
	std::expected<LaunchResult, std::string> spawn_cli_in_pty(
		const std::string &executable,
		const std::vector<std::string> &args,
		const std::string &working_dir = "/"
	);

	/**
	 * spawn_gui_direct - 直接启动GUI程序（无PTY）
	 *
	 * @param executable: 可执行文件路径
	 * @param args: 参数列表
	 * @param working_dir: 工作目录
	 * @return LaunchResult: 启动结果
	 */
	std::expected<LaunchResult, std::string> spawn_gui_direct(
		const std::string &executable,
		const std::vector<std::string> &args,
		const std::string &working_dir = "/"
	);

private:
	struct Impl;
	std::unique_ptr<Impl> pImpl_;
};

} /* namespace mikufy */

#endif /* MIKUFY_PROCESS_LAUNCHER_H */
