/*
 * Mikufy v2.11-nova - 终端窗口头文件
 *
 * 本文件定义了TerminalWindow类的接口，用于创建独立的GTK4终端窗口。
 * 使用VteTerminal组件提供完整的终端模拟功能。
 *
 * 核心特性:
 * - GTK4应用窗口，内嵌VteTerminal
 * - 黑色背景，白色文字，等宽字体
 * - 自动聚焦，ESC键关闭窗口并终止进程
 * - 进程退出后显示退出码
 * - RAII资源管理
 * - C++23特性：std::expected, std::unique_ptr
 *
 * MiraTrive/MikuTrive
 *
 * 本文件遵循Linux内核代码风格规范
 */

#ifndef MIKUFY_TERMINAL_WINDOW_H
#define MIKUFY_TERMINAL_WINDOW_H

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
#include <thread>
#include <mutex>
#include <condition_variable>

/*
 * ============================================================================
 * 前向声明
 * ============================================================================
 */

namespace mikufy {

/*
 * ============================================================================
 * 数据结构
 * ============================================================================
 */

/**
 * WindowState - 窗口状态
 */
enum class WindowState {
	ACTIVE,		/* 活动状态 */
	CLOSING,	/* 正在关闭 */
	CLOSED		/* 已关闭 */
};

/*
 * ============================================================================
 * TerminalWindow类 - 终端窗口
 * ============================================================================
 */

/**
 * TerminalWindow - 独立的GTK4终端窗口
 *
 * 使用VteTerminal组件显示CLI程序的输出。
 * 窗口自动聚焦，ESC键关闭窗口并终止进程。
 */
class TerminalWindow {
public:
	/**
	 * TerminalWindow - 构造函数
	 */
	TerminalWindow();

	/**
	 * ~TerminalWindow - 析构函数
	 */
	~TerminalWindow();

	/* 禁止拷贝 */
	TerminalWindow(const TerminalWindow &) = delete;
	TerminalWindow &operator=(const TerminalWindow &) = delete;

	/* 禁止移动 */
	TerminalWindow(TerminalWindow &&) = delete;
	TerminalWindow &operator=(TerminalWindow &&) = delete;

	/**
	 * create_and_show - 创建并显示终端窗口
	 *
	 * @param executable: 可执行文件路径
	 * @param args: 参数列表
	 * @param working_dir: 工作目录
	 * @return bool: 是否成功创建并显示
	 */
	bool create_and_show(const std::string &executable,
			     const std::vector<std::string> &args,
			     const std::string &working_dir = "/");

	/**
	 * wait_for_process - 等待进程结束（阻塞）
	 *
	 * @return int: 进程退出码
	 */
	int wait_for_process();

	/**
	 * is_running - 检查窗口是否还在运行
	 *
	 * @return bool: 是否运行中
	 */
	bool is_running() const;

	/**
	 * get_state - 获取窗口状态
	 *
	 * @return WindowState: 窗口状态
	 */
	WindowState get_state() const;

	/**
	 * get_pid - 获取进程ID
	 *
	 * @return pid_t: 进程ID
	 */
	pid_t get_pid() const;

	/**
	 * close_and_terminate - 关闭窗口并终止进程（公开供回调使用）
	 */
	void close_and_terminate();

private:
	struct Impl;
	std::unique_ptr<Impl> pImpl_;

	/**
	 * setup_application - 设置GTK4应用
	 *
	 * @return bool: 是否成功
	 */
	bool setup_application();

	/**
	 * setup_window - 设置窗口
	 *
	 * @param title: 窗口标题
	 * @return bool: 是否成功
	 */
	bool setup_window(const std::string &title);

	/**
	 * setup_terminal - 设置VTE终端
	 *
	 * @return bool: 是否成功
	 */
	bool setup_terminal();

	/**
	 * spawn_process - 启动进程
	 *
	 * @param executable: 可执行文件路径
	 * @param args: 参数列表
	 * @param working_dir: 工作目录
	 * @return bool: 是否成功
	 */
	bool spawn_process(const std::string &executable,
			   const std::vector<std::string> &args,
			   const std::string &working_dir);
};

} /* namespace mikufy */

#endif /* MIKUFY_TERMINAL_WINDOW_H */
