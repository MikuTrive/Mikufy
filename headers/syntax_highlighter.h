/*
 * Mikufy v2.5(stable) - 语法高亮器头文件
 *
 * 本文件定义了 SyntaxHighlighter 类的接口，该类负责提供高性能的
 * 代码语法高亮功能。该模块使用高效的算法和数据结构，支持多种
 * 编程语言的语法高亮，并针对大文件进行了优化。
 *
 * 主要功能:
 * - 代码语法高亮（C/C++、JavaScript、Python 等）
 * - 基于行的处理，支持大文件
 * - 高性能的字符串匹配和替换
 * - 支持增量高亮，只高亮可见区域
 *
 * 设计特点:
 * - 高性能：使用高效的字符串处理算法
 * - 可扩展：支持添加新的语言定义
 * - 线程安全：使用互斥锁保护共享资源
 *
 * MiraTrive/MikuTrive
 * Author: [Your Name]
 *
 * 本文件遵循 Linux 内核代码风格规范
 */

#ifndef MIKUFY_SYNTAX_HIGHLIGHTER_H
#define MIKUFY_SYNTAX_HIGHLIGHTER_H

/*
 * ============================================================================
 * 头文件包含
 * ============================================================================
 */

#include "main.h"
#include <vector>
#include <string>
#include <regex>

/*
 * ============================================================================
 * 常量定义
 * ============================================================================
 */

/* 最大处理行数，防止处理超大文件 */
#define MAX_HIGHLIGHT_LINES	100000

/*
 * ============================================================================
 * SyntaxHighlighter 类定义
 * ============================================================================
 */

/**
 * SyntaxHighlighter - 语法高亮器类
 *
 * 该类提供代码语法高亮功能，支持多种编程语言。
 * 使用高效的字符串处理算法，针对大文件进行了优化。
 *
 * 设计特点:
 * - 高性能：使用预编译的正则表达式
 * - 增量高亮：支持只高亮可见区域
 * - 线程安全：使用互斥锁保护共享资源
 */
class SyntaxHighlighter {
public:
	/**
	 * SyntaxHighlighter - 构造函数
	 *
	 * 初始化语法高亮器，预编译所有正则表达式模式。
	 */
	SyntaxHighlighter();

	/**
	 * ~SyntaxHighlighter - 析构函数
	 *
	 * 清理语法高亮器资源。
	 */
	~SyntaxHighlighter(void);

	/* 禁止拷贝构造函数 */
	SyntaxHighlighter(const SyntaxHighlighter &) = delete;

	/* 禁止拷贝赋值操作符 */
	SyntaxHighlighter &operator=(const SyntaxHighlighter &) = delete;

	/* 禁止移动构造函数 */
	SyntaxHighlighter(SyntaxHighlighter &&) = delete;

	/* 禁止移动赋值操作符 */
	SyntaxHighlighter &operator=(SyntaxHighlighter &&) = delete;

	/* ====================================================================
	 * 公共方法 - 语法高亮
	 * ==================================================================== */

	/**
	 * highlight - 对代码进行语法高亮
	 *
	 * 将代码内容转换为带有语法高亮的 HTML。
	 * 支持多种编程语言，使用高效的算法处理大文件。
	 *
	 * @code: 要高亮的代码内容
	 * @language: 编程语言（如 "c", "cpp", "javascript", "python" 等）
	 * @html: 输出参数，用于存储高亮后的 HTML 内容
	 *
	 * 返回值: 成功返回 true，失败返回 false
	 */
	bool highlight(const std::string &code,
		       const std::string &language,
		       std::string &html);

	/**
	 * highlight_range - 对代码的指定行范围进行语法高亮
	 *
	 * 只对代码的指定行范围进行语法高亮，用于实现增量高亮。
	 *
	 * @code: 要高亮的代码内容
	 * @language: 编程语言
	 * @start_line: 起始行号（从 0 开始）
	 * @end_line: 结束行号（不包含）
	 * @html: 输出参数，用于存储高亮后的 HTML 内容
	 *
	 * 返回值: 成功返回 true，失败返回 false
	 */
	bool highlight_range(const std::string &code,
			     const std::string &language,
			     int start_line,
			     int end_line,
			     std::string &html);

	/**
	 * detect_language - 自动检测代码语言
	 *
	 * 根据文件扩展名或代码内容自动检测编程语言。
	 *
	 * @filename: 文件名（用于根据扩展名检测）
	 * @code: 代码内容（用于根据内容检测）
	 *
	 * 返回值: 检测到的语言名称（如 "cpp", "python" 等）
	 */
	std::string detect_language(const std::string &filename,
				    const std::string &code);

	/**
	 * highlight_first_screen - 高亮首屏内容（立即渲染）
	 *
	 * 只高亮前几行用于立即显示，后台继续处理剩余部分。
	 *
	 * @code: 要高亮的代码内容
	 * @language: 编程语言
	 * @html: 输出参数，用于存储首屏高亮后的 HTML 内容
	 * @first_screen_lines: 首屏行数（默认50行）
	 *
	 * 返回值: 成功返回 true，失败返回 false
	 */
	bool highlight_first_screen(const std::string &code,
				     const std::string &language,
				     std::string &html,
				     int first_screen_lines = 50);

	/**
	 * highlight_remaining - 高亮剩余部分（后台任务）
	 *
	 * 高亮除首屏外的剩余部分。
	 *
	 * @code: 要高亮的代码内容
	 * @language: 编程语言
	 * @start_line: 起始行号
	 * @html: 输出参数，用于存储剩余部分高亮后的 HTML 内容
	 *
	 * 返回值: 成功返回 true，失败返回 false
	 */
	bool highlight_remaining(const std::string &code,
			       const std::string &language,
			       int start_line,
			       std::string &html);

private:
	/* ====================================================================
	 * 私有成员变量
	 * ==================================================================== */

	std::mutex mutex;	/* 互斥锁，保证线程安全 */

	/* 正则表达式模式（预编译） */
	/* C/C++ 预编译正则表达式 */
	std::regex pattern_cpp_string;	/* 字符串 */
	std::regex pattern_cpp_preproc;	/* 预处理指令 */
	std::regex pattern_cpp_keyword;	/* 关键字 */
	std::regex pattern_cpp_number;	/* 数字 */
	std::regex pattern_cpp_type;	/* 类型 */
	std::regex pattern_cpp_operator;	/* 运算符 */
	std::regex pattern_cpp_macro;	/* 宏定义 */
	std::regex pattern_cpp_attribute;	/* 属性 */
	std::regex pattern_cpp_template;	/* 模板参数 */
	std::regex pattern_cpp_lambda;	/* Lambda表达式 */

	/* JavaScript/TypeScript 预编译正则表达式 */
	std::regex pattern_js_string;	/* 字符串 */
	std::regex pattern_js_keyword;	/* 关键字 */
	std::regex pattern_js_number;	/* 数字 */
	std::regex pattern_js_comment;	/* 注释 */
	std::regex pattern_js_class;	/* 类名（大写开头） */
	std::regex pattern_js_function;	/* 函数定义和调用 */
	std::regex pattern_js_property;	/* 属性访问 */

	/* Python 预编译正则表达式 */
	std::regex pattern_py_string;	/* 字符串 */
	std::regex pattern_py_keyword;	/* 关键字 */
	std::regex pattern_py_number;	/* 数字 */
	std::regex pattern_py_comment;	/* 注释 */
	std::regex pattern_py_decorator;	/* 装饰器 */
	std::regex pattern_py_builtins;	/* 内置函数 */

	/* Java 预编译正则表达式 */
	std::regex pattern_java_string;	/* 字符串 */
	std::regex pattern_java_keyword;	/* 关键字 */
	std::regex pattern_java_number;	/* 数字 */
	std::regex pattern_java_annotation;	/* 注解 */
	std::regex pattern_java_type;	/* 类型/类名 */

	/* Shell 预编译正则表达式 */
	std::regex pattern_shell_string;	/* 字符串 */
	std::regex pattern_shell_keyword;	/* 关键字 */
	std::regex pattern_shell_variable;	/* 变量 */
	std::regex pattern_shell_comment;	/* 注释 */

	/* Go 预编译正则表达式 */
	std::regex pattern_go_string;	/* 字符串 */
	std::regex pattern_go_keyword;	/* 关键字 */
	std::regex pattern_go_number;	/* 数字 */
	std::regex pattern_go_comment;	/* 注释 */
	std::regex pattern_go_builtin;	/* 内置函数 */
	std::regex pattern_go_type;	/* 类型 */

	/* Rust 预编译正则表达式 */
	std::regex pattern_rust_string;	/* 字符串 */
	std::regex pattern_rust_keyword;	/* 关键字 */
	std::regex pattern_rust_number;	/* 数字 */
	std::regex pattern_rust_comment;	/* 注释 */
	std::regex pattern_rust_macro;	/* 宏 */
	std::regex pattern_rust_lifetime;	/* 生命周期 */

	/* Kotlin 预编译正则表达式 */
	std::regex pattern_kotlin_string;	/* 字符串 */
	std::regex pattern_kotlin_keyword;	/* 关键字 */
	std::regex pattern_kotlin_number;	/* 数字 */
	std::regex pattern_kotlin_comment;	/* 注释 */
	std::regex pattern_kotlin_annotation;	/* 注解 */

	/* Ruby 预编译正则表达式 */
	std::regex pattern_ruby_string;	/* 字符串 */
	std::regex pattern_ruby_keyword;	/* 关键字 */
	std::regex pattern_ruby_number;	/* 数字 */
	std::regex pattern_ruby_comment;	/* 注释 */
	std::regex pattern_ruby_symbol;	/* 符号 */

	/* Lua 预编译正则表达式 */
	std::regex pattern_lua_string;	/* 字符串 */
	std::regex pattern_lua_keyword;	/* 关键字 */
	std::regex pattern_lua_number;	/* 数字 */
	std::regex pattern_lua_comment;	/* 注释 */
	std::regex pattern_lua_builtin;	/* 内置函数 */

	/* PHP 预编译正则表达式 */
	std::regex pattern_php_string;	/* 字符串 */
	std::regex pattern_php_keyword;	/* 关键字 */
	std::regex pattern_php_number;	/* 数字 */
	std::regex pattern_php_comment;	/* 注释 */
	std::regex pattern_php_variable;	/* 变量 */

	/* Assembly 预编译正则表达式 */
	std::regex pattern_asm_comment;	/* 注释 */
	std::regex pattern_asm_label;	/* 标签 */
	std::regex pattern_asm_directive;	/* 指令 */
	std::regex pattern_asm_register;	/* 寄存器 */
	std::regex pattern_asm_number;	/* 数字 */

	/* HTML 预编译正则表达式 */
	std::regex pattern_html_tag;	/* 标签 */
	std::regex pattern_html_attribute;	/* 属性 */
	std::regex pattern_html_string;	/* 字符串 */
	std::regex pattern_html_comment;	/* 注释 */

	/* CSS 预编译正则表达式 */
	std::regex pattern_css_selector;	/* 选择器 */
	std::regex pattern_css_property;	/* 属性 */
	std::regex pattern_css_value;	/* 值 */
	std::regex pattern_css_comment;	/* 注释 */

	/* JSON 预编译正则表达式 */
	std::regex pattern_json_key;	/* 键 */
	std::regex pattern_json_string;	/* 字符串 */
	std::regex pattern_json_number;	/* 数字 */
	std::regex pattern_json_boolean;	/* 布尔值 */
	std::regex pattern_json_null;	/* null */

	/* Makefile 预编译正则表达式 */
	std::regex pattern_make_target;	/* 目标 */
	std::regex pattern_make_variable;	/* 变量 */
	std::regex pattern_make_comment;	/* 注释 */

	/* CMake 预编译正则表达式 */
	std::regex pattern_cmake_command;	/* 命令 */
	std::regex pattern_cmake_variable;	/* 变量 */
	std::regex pattern_cmake_comment;	/* 注释 */

	/* ====================================================================
	 * 私有方法 - 语言处理
	 * ==================================================================== */

	/**
	 * escape_html - 转义 HTML 特殊字符
	 *
	 * 将文本中的 HTML 特殊字符转义为实体引用。
	 *
	 * @text: 要转义的文本
	 *
	 * 返回值: 转义后的文本
	 */
	std::string escape_html(const std::string &text);

	/**
	 * split_lines - 将代码按行分割
	 *
	 * 将代码内容按换行符分割成行数组。
	 *
	 * @code: 要分割的代码内容
	 * @lines: 输出参数，用于存储分割后的行数组
	 */
	void split_lines(const std::string &code,
			std::vector<std::string> &lines);

	/**
	 * highlight_line - 对单行代码进行语法高亮
	 *
	 * 对单行代码进行语法高亮处理。
	 *
	 * @line: 要高亮的行
	 * @language: 编程语言
	 *
	 * 返回值: 高亮后的 HTML
	 */
	std::string highlight_line(const std::string &line,
				   const std::string &language);

	/**
	 * init_patterns - 初始化正则表达式模式
	 *
	 * 预编译所有需要的正则表达式模式，提高性能。
	 */
	void init_patterns(void);

	/* ====================================================================
	 * 私有方法 - 语言特定高亮
	 * ==================================================================== */

	/**
	 * highlight_cpp - C/C++ 语言高亮
	 *
	 * 对 C/C++ 代码进行语法高亮处理。
	 *
	 * @line: 要高亮的行
	 *
	 * 返回值: 高亮后的 HTML
	 */
	std::string highlight_cpp(const std::string &line);

	/**
	 * highlight_javascript - JavaScript/TypeScript 高亮
	 *
	 * 对 JavaScript/TypeScript 代码进行语法高亮处理。
	 *
	 * @line: 要高亮的行
	 *
	 * 返回值: 高亮后的 HTML
	 */
	std::string highlight_javascript(const std::string &line);

	/**
	 * highlight_python - Python 语言高亮
	 *
	 * 对 Python 代码进行语法高亮处理。
	 *
	 * @line: 要高亮的行
	 *
	 * 返回值: 高亮后的 HTML
	 */
	std::string highlight_python(const std::string &line);

	/**
	 * highlight_java - Java 语言高亮
	 *
	 * 对 Java 代码进行语法高亮处理。
	 *
	 * @line: 要高亮的行
	 *
	 * 返回值: 高亮后的 HTML
	 */
	std::string highlight_java(const std::string &line);

	/**
	 * highlight_shell - Shell 脚本高亮
	 *
	 * 对 Shell 脚本代码进行语法高亮处理。
	 *
	 * @line: 要高亮的行
	 *
	 * 返回值: 高亮后的 HTML
	 */
	std::string highlight_shell(const std::string &line);

	/**
	 * highlight_go - Go 语言高亮
	 *
	 * 对 Go 代码进行语法高亮处理。
	 *
	 * @line: 要高亮的行
	 *
	 * 返回值: 高亮后的 HTML
	 */
	std::string highlight_go(const std::string &line);

	/**
	 * highlight_rust - Rust 语言高亮
	 *
	 * 对 Rust 代码进行语法高亮处理。
	 *
	 * @line: 要高亮的行
	 *
	 * 返回值: 高亮后的 HTML
	 */
	std::string highlight_rust(const std::string &line);

	/**
	 * highlight_kotlin - Kotlin 语言高亮
	 *
	 * 对 Kotlin 代码进行语法高亮处理。
	 *
	 * @line: 要高亮的行
	 *
	 * 返回值: 高亮后的 HTML
	 */
	std::string highlight_kotlin(const std::string &line);

	/**
	 * highlight_ruby - Ruby 语言高亮
	 *
	 * 对 Ruby 代码进行语法高亮处理。
	 *
	 * @line: 要高亮的行
	 *
	 * 返回值: 高亮后的 HTML
	 */
	std::string highlight_ruby(const std::string &line);

	/**
	 * highlight_lua - Lua 语言高亮
	 *
	 * 对 Lua 代码进行语法高亮处理。
	 *
	 * @line: 要高亮的行
	 *
	 * 返回值: 高亮后的 HTML
	 */
	std::string highlight_lua(const std::string &line);

	/**
	 * highlight_php - PHP 语言高亮
	 *
	 * 对 PHP 代码进行语法高亮处理。
	 *
	 * @line: 要高亮的行
	 *
	 * 返回值: 高亮后的 HTML
	 */
	std::string highlight_php(const std::string &line);

	/**
	 * highlight_asm - Assembly 语言高亮
	 *
	 * 对汇编代码进行语法高亮处理。
	 *
	 * @line: 要高亮的行
	 *
	 * 返回值: 高亮后的 HTML
	 */
	std::string highlight_asm(const std::string &line);

	/**
	 * highlight_html - HTML 语言高亮
	 *
	 * 对 HTML 代码进行语法高亮处理。
	 *
	 * @line: 要高亮的行
	 *
	 * 返回值: 高亮后的 HTML
	 */
	std::string highlight_html(const std::string &line);

	/**
	 * highlight_css - CSS 语言高亮
	 *
	 * 对 CSS 代码进行语法高亮处理。
	 *
	 * @line: 要高亮的行
	 *
	 * 返回值: 高亮后的 HTML
	 */
	std::string highlight_css(const std::string &line);

	/**
	 * highlight_json - JSON 语言高亮
	 *
	 * 对 JSON 代码进行语法高亮处理。
	 *
	 * @line: 要高亮的行
	 *
	 * 返回值: 高亮后的 HTML
	 */
	std::string highlight_json(const std::string &line);

	/**
	 * highlight_make - Makefile 语言高亮
	 *
	 * 对 Makefile 代码进行语法高亮处理。
	 *
	 * @line: 要高亮的行
	 *
	 * 返回值: 高亮后的 HTML
	 */
	std::string highlight_make(const std::string &line);

	/**
	 * highlight_cmake - CMake 语言高亮
	 *
	 * 对 CMake 代码进行语法高亮处理。
	 *
	 * @line: 要高亮的行
	 *
	 * 返回值: 高亮后的 HTML
	 */
	std::string highlight_cmake(const std::string &line);
};

#endif /* MIKUFY_SYNTAX_HIGHLIGHTER_H */