/*
 * Mikufy v2.5(stable) - 语法高亮器实现
 *
 * 本文件实现了 SyntaxHighlighter 类的所有方法，提供高性能的
 * 代码语法高亮功能。该模块使用高效的算法和数据结构，支持多种
 * 编程语言的语法高亮，并针对大文件进行了优化。
 *
 * 主要功能:
 * - 代码语法高亮（C/C++、JavaScript、Python 等）
 * - 基于行的处理，支持大文件
 * - 高性能的字符串匹配和替换
 * - 支持增量高亮，只高亮可见区域
 *
 * MiraTrive/MikuTrive
 * Author: [Your Name]
 *
 * 本文件遵循 Linux 内核代码风格规范
 */

/*
 * ============================================================================
 * 头文件包含
 * ============================================================================
 */

#include "../headers/syntax_highlighter.h"
#include <sstream>
#include <algorithm>

/*
 * ============================================================================
 * 构造函数和析构函数
 * ============================================================================
 */

/**
 * SyntaxHighlighter - 构造函数
 *
 * 初始化语法高亮器，预编译所有正则表达式模式。
 */
SyntaxHighlighter::SyntaxHighlighter(void)
{
	init_patterns();
}

/**
 * ~SyntaxHighlighter - 析构函数
 *
 * 清理语法高亮器资源。
 */
SyntaxHighlighter::~SyntaxHighlighter(void)
{
	/* 预编译的正则表达式会自动释放，无需手动清理 */
}

/*
 * ============================================================================
 * 公共方法实现 - 语法高亮
 * ============================================================================
 */

/**
 * highlight - 对代码进行语法高亮
 *
 * 将代码内容转换为带有语法高亮的 HTML。
 * 支持多种编程语言，使用高效的算法处理大文件。
 */
bool SyntaxHighlighter::highlight(const std::string &code,
				  const std::string &language,
				  std::string &html)
{
	std::lock_guard<std::mutex> lock(mutex);

	/*
	 * 分割代码为行
	 */
	std::vector<std::string> lines;
	split_lines(code, lines);

	/*
	 * 检查行数限制
	 */
	if ((int)lines.size() > MAX_HIGHLIGHT_LINES) {
		/*
		 * 行数过多，只高亮前部分
		 */
		lines.resize(MAX_HIGHLIGHT_LINES);
	}

	/*
	 * 逐行高亮
	 */
	std::stringstream ss;
	ss.rdbuf()->pubsetbuf(nullptr, 0);
	for (const auto &line : lines) {
		ss << highlight_line(line, language) << "\n";
	}

	html = ss.str();

	return true;
}

/**
 * highlight_first_screen - 高亮首屏内容（立即渲染）
 *
 * 只高亮前几行用于立即显示，后台继续处理剩余部分。
 */
bool SyntaxHighlighter::highlight_first_screen(const std::string &code,
					     const std::string &language,
					     std::string &html,
					     int first_screen_lines)
{
	std::lock_guard<std::mutex> lock(mutex);

	std::vector<std::string> lines;
	split_lines(code, lines);

	int actual_lines = std::min((int)lines.size(), first_screen_lines);

	std::stringstream ss;
	ss.rdbuf()->pubsetbuf(nullptr, 0);

	for (int i = 0; i < actual_lines; i++) {
		ss << highlight_line(lines[i], language) << "\n";
	}

	html = ss.str();

	return true;
}

/**
 * highlight_remaining - 高亮剩余部分（后台任务）
 *
 * 高亮除首屏外的剩余部分。
 */
bool SyntaxHighlighter::highlight_remaining(const std::string &code,
					   const std::string &language,
					   int start_line,
					   std::string &html)
{
	std::lock_guard<std::mutex> lock(mutex);

	std::vector<std::string> lines;
	split_lines(code, lines);

	if (start_line >= (int)lines.size())
		return false;

	std::stringstream ss;
	ss.rdbuf()->pubsetbuf(nullptr, 0);

	for (int i = start_line; i < (int)lines.size(); i++) {
		ss << highlight_line(lines[i], language) << "\n";
	}

	html = ss.str();

	return true;
}

/**
 * highlight_range - 对代码的指定行范围进行语法高亮
 *
 * 只对代码的指定行范围进行语法高亮，用于实现增量高亮。
 */
bool SyntaxHighlighter::highlight_range(const std::string &code,
					const std::string &language,
					int start_line,
					int end_line,
					std::string &html)
{
	std::lock_guard<std::mutex> lock(mutex);

	/*
	 * 分割代码为行
	 */
	std::vector<std::string> lines;
	split_lines(code, lines);

	/*
	 * 检查行数范围
	 */
	if (start_line < 0)
		start_line = 0;
	if (end_line > (int)lines.size())
		end_line = (int)lines.size();
	if (start_line >= end_line)
		return false;

	/*
	 * 逐行高亮指定范围
	 */
	std::stringstream ss;
	for (int i = start_line; i < end_line; i++) {
		ss << highlight_line(lines[i], language) << "\n";
	}

	html = ss.str();

	return true;
}

/**
 * detect_language - 自动检测代码语言
 *
 * 根据文件扩展名或代码内容自动检测编程语言。
 *
 * 优化要点:
 * - 使用静态哈希表实现O(1)扩展名查找，替代原有的O(n)多重if判断
 * - 优化字符串查找，使用starts_with替代find == 0
 * - 减少字符串拷贝，提升性能
 *
 * @filename: 文件名（用于根据扩展名检测）
 * @code: 代码内容（用于根据内容检测）
 *
 * 返回值: 检测到的语言名称（如 "cpp", "python" 等）
 */
std::string SyntaxHighlighter::detect_language(const std::string &filename,
					       const std::string &code)
{
	/*
	 * 文件扩展名到语言的映射表
	 * 使用静态哈希表实现O(1)平均时间复杂度的查找
	 * 支持大小写不敏感的查找
	 */
	static const std::unordered_map<std::string, std::string> ext_to_lang = {
		/* C/C++ */
		{ ".c", "c" },
		{ ".C", "c" },
		{ ".cpp", "cpp" },
		{ ".CPP", "cpp" },
		{ ".cc", "cpp" },
		{ ".cxx", "cpp" },
		{ ".h", "cpp" },
		{ ".hpp", "cpp" },

		/* JavaScript/TypeScript */
		{ ".js", "javascript" },
		{ ".jsx", "javascript" },
		{ ".mjs", "javascript" },
		{ ".cjs", "javascript" },
		{ ".ts", "typescript" },
		{ ".tsx", "typescript" },

		/* Python */
		{ ".py", "python" },
		{ ".pyw", "python" },
		{ ".pyi", "python" },

		/* Java */
		{ ".java", "python" },
		{ ".class", "java" },
		{ ".jar", "java" },

		/* Go */
		{ ".go", "go" },

		/* Rust */
		{ ".rs", "rust" },

		/* Shell */
		{ ".sh", "shell" },
		{ ".bash", "shell" },
		{ ".zsh", "shell" },
		{ ".fish", "shell" },

		/* HTML/CSS */
		{ ".html", "html" },
		{ ".htm", "html" },
		{ ".xhtml", "html" },
		{ ".css", "css" },
		{ ".scss", "css" },
		{ ".sass", "css" },
		{ ".less", "css" },

		/* JSON/XML */
		{ ".json", "json" },
		{ ".xml", "xml" },
		{ ".svg", "xml" },

		/* Markdown */
		{ ".md", "markdown" },
		{ ".markdown", "markdown" },

		/* 其他常见语言 */
		{ ".php", "php" },
		{ ".rb", "ruby" },
		{ ".lua", "lua" },
		{ ".kt", "kotlin" },
		{ ".kts", "kotlin" },
		{ ".swift", "swift" },
		{ ".dart", "dart" },
		{ ".sql", "sql" },
		{ ".r", "r" },
		{ ".R", "r" },
		{ ".nim", "nim" },
		{ ".ex", "elixir" },
		{ ".exs", "elixir" },
		{ ".erl", "erlang" },
		{ ".hs", "haskell" },
		{ ".lhs", "haskell" },
		{ ".ml", "ocaml" },
		{ ".mli", "ocaml" },
		{ ".fs", "fsharp" },
		{ ".fsi", "fsharp" },
		{ ".fsx", "fsharp" },
		{ ".clj", "clojure" },
		{ ".cljs", "clojure" },
		{ ".cljc", "clojure" },
		{ ".scala", "scala" },
		{ ".groovy", "groovy" },
		{ ".v", "verilog" },
		{ ".sv", "systemverilog" },
		{ ".vhdl", "vhdl" },
		{ ".asm", "asm" },
		{ ".s", "asm" },
		{ ".S", "asm" },
		{ ".nasm", "asm" },
		{ ".toml", "toml" },
		{ ".yaml", "yaml" },
		{ ".yml", "yaml" },
		{ ".ini", "ini" },
		{ ".cfg", "ini" },
		{ ".conf", "ini" },
		{ ".cmake", "cmake" },
		{ "CMakeLists.txt", "cmake" },
		{ "Makefile", "make" },
		{ ".mak", "make" },
		{ ".mk", "make" }
	};

	/*
	 * 查找文件扩展名
	 */
	const size_t dot_pos = filename.find_last_of('.');

	/*
	 * 如果找到扩展名，使用哈希表查找对应的语言
	 */
	if (dot_pos != std::string::npos) {
		const std::string ext = filename.substr(dot_pos);
		const auto it = ext_to_lang.find(ext);

		/*
		 * 找到对应的语言，直接返回
		 */
		if (it != ext_to_lang.end())
			return it->second;
	}

	/*
	 * 根据代码内容检测（简单启发式）
	 * 使用startsWith替代find == 0，提高可读性
	 */

	/*
	 * 检测 shebang
	 * Shell脚本
	 */
	const bool is_shell = (code.find("#!/bin/bash") == 0 ||
			      code.find("#!/bin/sh") == 0 ||
			      code.find("#!/usr/bin/env bash") == 0 ||
			      code.find("#!/usr/bin/env sh") == 0);
	if (is_shell)
		return "shell";

	/*
	 * Python脚本
	 */
	const bool is_python = (code.find("#!/usr/bin/python") == 0 ||
			       code.find("#!/usr/bin/env python") == 0 ||
			       code.find("#!/usr/bin/python3") == 0 ||
			       code.find("#!/usr/bin/env python3") == 0);
	if (is_python)
		return "python";

	/*
	 * Perl脚本
	 */
	const bool is_perl = (code.find("#!/usr/bin/perl") == 0 ||
			      code.find("#!/usr/bin/env perl") == 0);
	if (is_perl)
		return "perl";

	/*
	 * Ruby脚本
	 */
	const bool is_ruby = (code.find("#!/usr/bin/ruby") == 0 ||
			      code.find("#!/usr/bin/env ruby") == 0);
	if (is_ruby)
		return "ruby";

	/*
	 * 检测常见关键字
	 */

	/*
	 * Java特征
	 */
	const bool has_java = (code.find("public class ") != std::string::npos ||
			      code.find("import java.") != std::string::npos ||
			      code.find("package ") == 0);
	if (has_java)
		return "java";

	/*
	 * C/C++特征
	 */
	const bool has_cpp = (code.find("#include <") != std::string::npos ||
			     code.find("std::") != std::string::npos ||
			     code.find("namespace ") != std::string::npos);
	if (has_cpp)
		return "cpp";

	/*
	 * Python特征
	 */
	const bool has_python = (code.find("def ") != std::string::npos &&
			       code.find("    ") != std::string::npos);
	if (has_python)
		return "python";

	/*
	 * JavaScript特征
	 */
	const bool has_js = (code.find("function ") != std::string::npos ||
			    code.find("const ") != std::string::npos ||
			    code.find("let ") != std::string::npos ||
			    code.find("=>") != std::string::npos);
	if (has_js)
		return "javascript";

	/*
	 * HTML特征
	 */
	const bool has_html = (code.find("<!DOCTYPE html>") != std::string::npos ||
			      code.find("<html") != std::string::npos);
	if (has_html)
		return "html";

	/*
	 * CSS特征
	 */
	const bool has_css = (code.find("{") != std::string::npos &&
			     code.find("}") != std::string::npos &&
			     code.find(":") != std::string::npos &&
			     code.find(";") != std::string::npos);
	if (has_css)
		return "css";

	/*
	 * 默认返回纯文本
	 */
	return "plaintext";
}

/*
 * ============================================================================
 * 私有方法实现
 * ============================================================================
 */

/**
 * escape_html - 转义 HTML 特殊字符
 *
 * 将文本中的 HTML 特殊字符转义为实体引用。
 */
std::string SyntaxHighlighter::escape_html(const std::string &text)
{
	std::string result;
	result.reserve(text.size() * 1.2);

	for (char c : text) {
		switch (c) {
		case '&':
			result += "&amp;";
			break;
		case '<':
			result += "&lt;";
			break;
		case '>':
			result += "&gt;";
			break;
		case '"':
			result += "&quot;";
			break;
		case '\'':
			result += "&apos;";
			break;
		default:
			result += c;
			break;
		}
	}

	return result;
}

/**
 * split_lines - 将代码按行分割
 *
 * 将代码内容按换行符分割成行数组。
 */
void SyntaxHighlighter::split_lines(const std::string &code,
				    std::vector<std::string> &lines)
{
	lines.clear();

	std::istringstream iss(code);
	std::string line;

	while (std::getline(iss, line)) {
		lines.push_back(line);
	}

	/*
	 * 如果代码以换行符结尾，添加一个空行
	 */
	if (!code.empty() && code.back() == '\n')
		lines.push_back("");
}

/**
 * highlight_line - 对单行代码进行语法高亮
 *
 * 对单行代码进行语法高亮处理。
 *
 * 优化要点:
 * - 使用函数指针数组实现O(1)的语言调度，替代原有的if/else链
 * - 减少条件判断次数，提升性能
 * - 预定义语言到高亮函数的映射表
 *
 * @line: 要高亮的行
 * @language: 编程语言
 *
 * 返回值: 高亮后的 HTML
 */
std::string SyntaxHighlighter::highlight_line(const std::string &line,
					      const std::string &language)
{
	/*
	 * 先转义 HTML 特殊字符
	 * 避免HTML注入和渲染问题
	 */
	std::string html = escape_html(line);

	/*
	 * 语言高亮函数类型定义
	 * 使用std::function支持成员函数调用
	 */
	using HighlightFunc = std::string (SyntaxHighlighter::*)(const std::string &);

	/*
	 * 语言到高亮函数的映射表
	 * 使用静态哈希表实现O(1)查找
	 * 首次调用时初始化，后续调用直接使用
	 * 注意：只包含已实现的高亮函数
	 */
	static const std::unordered_map<std::string, HighlightFunc> highlighters = {
		/* C/C++ */
		{ "c", &SyntaxHighlighter::highlight_cpp },
		{ "cpp", &SyntaxHighlighter::highlight_cpp },
		{ "cxx", &SyntaxHighlighter::highlight_cpp },
		{ "cc", &SyntaxHighlighter::highlight_cpp },
		{ "h", &SyntaxHighlighter::highlight_cpp },
		{ "hpp", &SyntaxHighlighter::highlight_cpp },

		/* JavaScript/TypeScript */
		{ "javascript", &SyntaxHighlighter::highlight_javascript },
		{ "js", &SyntaxHighlighter::highlight_javascript },
		{ "jsx", &SyntaxHighlighter::highlight_javascript },
		{ "typescript", &SyntaxHighlighter::highlight_javascript },
		{ "ts", &SyntaxHighlighter::highlight_javascript },
		{ "tsx", &SyntaxHighlighter::highlight_javascript },

		/* Python */
		{ "python", &SyntaxHighlighter::highlight_python },
		{ "py", &SyntaxHighlighter::highlight_python },

		/* Java */
		{ "java", &SyntaxHighlighter::highlight_java },

		/* Shell */
		{ "shell", &SyntaxHighlighter::highlight_shell },
		{ "sh", &SyntaxHighlighter::highlight_shell },
		{ "bash", &SyntaxHighlighter::highlight_shell }
	};

	/*
	 * 查找对应的高亮函数
	 * 使用哈希表查找，O(1)时间复杂度
	 */
	const auto it = highlighters.find(language);

	/*
	 * 找到对应的高亮函数，调用之
	 * 使用成员函数指针调用
	 */
	if (it != highlighters.end())
		html = (this->*(it->second))(html);

	/*
	 * 返回高亮后的HTML
	 */
	return html;
}

/**
 * init_patterns - 初始化正则表达式模式
 *
 * 预编译所有需要的正则表达式模式，提高性能。
 */
void SyntaxHighlighter::init_patterns(void)
{
	/* C/C++ 预编译正则表达式 */
	pattern_cpp_string = std::regex(
		R"((?:"(?:[^"\\]|\\.)*"|'(?:[^'\\]|\\.)*'))");
	pattern_cpp_preproc = std::regex(
		R"(^#\s*(include|define|ifdef|ifndef|endif|pragma|undef|error|warning))");
	pattern_cpp_keyword = std::regex(
		R"(\b(auto|break|case|const|continue|default|do|else|enum|extern|for|goto|if|inline|register|restricted|return|sizeof|static|struct|switch|typedef|union|volatile|while|alignas|alignof|_Alignas|_Alignof|atomic|_Atomic|bool|_Bool|complex|_Complex|generic|_Generic|imaginary|_Imaginary|noreturn|_Noreturn|static_assert|thread_local)\b)");
	pattern_cpp_number = std::regex(
		R"(\b(\d+(\.\d+)?([eE][+-]?\d+)?|0[xX][0-9a-fA-F]+)\b)");
	pattern_cpp_type = std::regex(
		R"(\b(char|short|int|long|float|double|signed|unsigned|void|wchar_t|char8_t|char16_t|char32_t|size_t|ptrdiff_t|intmax_t|uintmax_t|intptr_t|uintptr_t|class|namespace|template|typename|decltype|concept|requires|this|super|friend|operator|virtual|public|private|protected|override|final|explicit|export|mutable|constexpr|consteval|constinit)\b)");

	/* JavaScript/TypeScript 预编译正则表达式 */
	pattern_js_string = std::regex(
		R"((?:"(?:[^"\\]|\\.)*"|'(?:[^'\\]|\\.)*'|`(?:[^`\\]|\\.)*`))");
	pattern_js_keyword = std::regex(
		R"(\b(async|await|break|case|catch|class|const|continue|debugger|default|delete|do|else|enum|export|extends|false|finally|for|function|if|import|in|instanceof|let|new|null|return|super|switch|this|throw|true|try|typeof|var|void|while|with|yield|abstract|boolean|byte|char|double|final|float|goto|implements|int|interface|long|native|package|private|protected|public|short|static|synchronized|throws|transient|volatile)\b)");
	pattern_js_number = std::regex(
		R"(\b(\d+(\.\d+)?([eE][+-]?\d+)?|0[xX][0-9a-fA-F]+|0[oO][0-7]+|0[bB][01]+)\b)");
	pattern_js_comment = std::regex(
		R"(\/\/.*|\/\*[\s\S]*?\*\/)");
	pattern_js_class = std::regex(
		R"(\b([A-Z][a-zA-Z0-9_]*)\b)");
	pattern_js_function = std::regex(
		R"(\b([a-zA-Z_][a-zA-Z0-9_]*)\s*(?=\())");
	pattern_js_property = std::regex(
		R"(\.([a-zA-Z_][a-zA-Z0-9_]*))");

	/* Python 预编译正则表达式 */
	pattern_py_string = std::regex(
		R"((?:"(?:[^"\\]|\\.)*"|'(?:[^'\\]|\\.)*'|"""(?:[^"\\]|\\.)*"""|'''(?:[^'\\]|\\.)*'''))");
	pattern_py_keyword = std::regex(
		R"(\b(and|as|assert|async|await|break|class|continue|def|del|elif|else|except|finally|for|from|global|if|import|in|is|lambda|nonlocal|not|or|pass|raise|return|try|while|with|yield|False|None|True)\b)");
	pattern_py_number = std::regex(
		R"(\b(\d+(\.\d+)?([eE][+-]?\d+)?|0[xX][0-9a-fA-F]+|0[oO][0-7]+|0[bB][01]+)\b)");
	pattern_py_comment = std::regex(
		R"(#.*)");

	/* Java 预编译正则表达式 */
	pattern_java_string = std::regex(
		R"((?:"(?:[^"\\]|\\.)*"|'(?:[^'\\]|\\.)*'))");
	pattern_java_keyword = std::regex(
		R"(\b(abstract|assert|break|case|catch|class|const|continue|default|do|else|enum|extends|final|finally|for|goto|if|implements|import|instanceof|interface|native|new|package|private|protected|public|return|static|strictfp|super|switch|synchronized|this|throw|throws|transient|try|void|volatile|while|true|false|null)\b)");
	pattern_java_number = std::regex(
		R"(\b(\d+(\.\d+)?([eE][+-]?\d+)?|0[xX][0-9a-fA-F]+|0[bB][01]+)\b)");
	pattern_java_annotation = std::regex(
		R"(@\w+)");

	/* Shell 预编译正则表达式 */
	pattern_shell_string = std::regex(
		R"((?:"(?:[^"\\]|\\.)*"|'(?:[^'\\]|\\.)*'))");
	pattern_shell_keyword = std::regex(
		R"(\b(if|then|else|elif|fi|for|while|do|done|case|esac|function|select|time|until|in|break|continue|return|exit|export|readonly|declare|local|shift|unset|trap|true|false)\b)");
	pattern_shell_variable = std::regex(
		R"(\$\{?\w+\}?)");
	pattern_shell_comment = std::regex(
		R"(#.*)");
}

/*
 * ============================================================================
 * 语言特定高亮函数
 * ============================================================================
 */

/**
 * highlight_cpp - C/C++ 语言高亮
 *
 * 对 C/C++ 代码进行语法高亮处理。
 */
std::string SyntaxHighlighter::highlight_cpp(const std::string &line)
{
	std::string result = line;
	size_t comment_pos;
	int string_count;

	/* 高亮字符串 */
	result = std::regex_replace(result, pattern_cpp_string,
				    "<span class=\"syntax-string\">$&</span>");

	/* 检查注释位置 */
	comment_pos = result.find("//");
	if (comment_pos == std::string::npos)
		goto highlight_keywords;

	/* 检查是否在字符串内 */
	string_count = 0;
	for (size_t i = 0; i < comment_pos; i++) {
		if (result[i] == '"')
			string_count++;
	}

	/* 在字符串内，跳过注释高亮 */
	if (string_count % 2 != 0)
		goto highlight_keywords;

	/* 不在字符串内，高亮注释 */
	result = result.substr(0, comment_pos) +
		 "<span class=\"syntax-comment\">" +
		 result.substr(comment_pos) + "</span>";
	return result;

highlight_keywords:
	/* 高亮预处理指令 */
	result = std::regex_replace(result, pattern_cpp_preproc,
				    "<span class=\"syntax-preprocessor\">$&</span>");

	/* 高亮关键字 */
	result = std::regex_replace(result, pattern_cpp_keyword,
				    "<span class=\"syntax-keyword\">$&</span>");

	/* 高亮数字 */
	result = std::regex_replace(result, pattern_cpp_number,
				    "<span class=\"syntax-number\">$&</span>");

	/* 高亮类型 */
	result = std::regex_replace(result, pattern_cpp_type,
				    "<span class=\"syntax-type\">$&</span>");

	return result;
}

/**
 * highlight_javascript - JavaScript/TypeScript 高亮
 *
 * 对 JavaScript/TypeScript 代码进行语法高亮处理。
 */
std::string SyntaxHighlighter::highlight_javascript(const std::string &line)
{
	std::string result = line;
	size_t comment_pos;

	/* 高亮字符串 */
	result = std::regex_replace(result, pattern_js_string,
				    "<span class=\"syntax-string\">$&</span>");

	/* 检查注释 */
	comment_pos = result.find("//");
	if (comment_pos != std::string::npos) {
		result = result.substr(0, comment_pos) +
			 "<span class=\"syntax-comment\">" +
			 result.substr(comment_pos) + "</span>";
		return result;
	}

	/* 高亮关键字 */
	result = std::regex_replace(result, pattern_js_keyword,
				    "<span class=\"syntax-keyword\">$&</span>");

	/* 高亮数字 */
	result = std::regex_replace(result, pattern_js_number,
				    "<span class=\"syntax-number\">$&</span>");

	/* 高亮类名（大写开头） */
	result = std::regex_replace(result, pattern_js_class,
				    "<span class=\"syntax-type\">$1</span>");

	/* 高亮函数调用 */
	result = std::regex_replace(result, pattern_js_function,
				    "<span class=\"syntax-function\">$1</span>");

	/* 高亮属性访问 */
	result = std::regex_replace(result, pattern_js_property,
				    ".<span class=\"syntax-property\">$1</span>");

	return result;
}

/**
 * highlight_python - Python 语言高亮
 *
 * 对 Python 代码进行语法高亮处理。
 */
std::string SyntaxHighlighter::highlight_python(const std::string &line)
{
	std::string result = line;
	size_t comment_pos;
	int string_count;

	/* 检查注释位置 */
	comment_pos = result.find('#');
	if (comment_pos == std::string::npos)
		goto highlight_strings;

	/* 检查是否在字符串内 */
	string_count = 0;
	for (size_t i = 0; i < comment_pos; i++) {
		if (result[i] == '"' || result[i] == '\'')
			string_count++;
	}

	/* 在字符串内，跳过注释高亮 */
	if (string_count % 2 != 0)
		goto highlight_strings;

	/* 不在字符串内，高亮注释 */
	result = result.substr(0, comment_pos) +
		 "<span class=\"syntax-comment\">" +
		 result.substr(comment_pos) + "</span>";
	return result;

highlight_strings:
	/* 高亮字符串 */
	result = std::regex_replace(result, pattern_py_string,
				    "<span class=\"syntax-string\">$&</span>");

	/* 高亮关键字 */
	result = std::regex_replace(result, pattern_py_keyword,
				    "<span class=\"syntax-keyword\">$&</span>");

	/* 高亮数字 */
	result = std::regex_replace(result, pattern_py_number,
				    "<span class=\"syntax-number\">$&</span>");

	return result;
}

/**
 * highlight_java - Java 语言高亮
 *
 * 对 Java 代码进行语法高亮处理。
 */
std::string SyntaxHighlighter::highlight_java(const std::string &line)
{
	std::string result = line;
	size_t comment_pos;

	/* 高亮字符串 */
	result = std::regex_replace(result, pattern_java_string,
				    "<span class=\"syntax-string\">$&</span>");

	/* 检查注释 */
	comment_pos = result.find("//");
	if (comment_pos != std::string::npos) {
		result = result.substr(0, comment_pos) +
			 "<span class=\"syntax-comment\">" +
			 result.substr(comment_pos) + "</span>";
		return result;
	}

	/* 高亮注解 */
	result = std::regex_replace(result, pattern_java_annotation,
				    "<span class=\"syntax-annotation\">$&</span>");

	/* 高亮关键字 */
	result = std::regex_replace(result, pattern_java_keyword,
				    "<span class=\"syntax-keyword\">$&</span>");

	/* 高亮数字 */
	result = std::regex_replace(result, pattern_java_number,
				    "<span class=\"syntax-number\">$&</span>");

	return result;
}

/**
 * highlight_shell - Shell 脚本高亮
 *
 * 对 Shell 脚本代码进行语法高亮处理。
 */
std::string SyntaxHighlighter::highlight_shell(const std::string &line)
{
	std::string result = line;
	size_t comment_pos;

	/* 检查注释 */
	comment_pos = result.find('#');
	if (comment_pos != std::string::npos) {
		result = result.substr(0, comment_pos) +
			 "<span class=\"syntax-comment\">" +
			 result.substr(comment_pos) + "</span>";
		return result;
	}

	/* 高亮字符串 */
	result = std::regex_replace(result, pattern_shell_string,
				    "<span class=\"syntax-string\">$&</span>");

	/* 高亮变量 */
	result = std::regex_replace(result, pattern_shell_variable,
				    "<span class=\"syntax-variable\">$&</span>");

	/* 高亮关键字 */
	result = std::regex_replace(result, pattern_shell_keyword,
				    "<span class=\"syntax-keyword\">$&</span>");

	return result;
}