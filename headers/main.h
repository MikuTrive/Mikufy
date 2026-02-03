/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Mikufy v2.3(stable) - 主头文件
 *
 * 这是Mikufy代码编辑器的主头文件，包含所有必要的头文件包含、
 * 全局常量定义、数据结构声明和类型定义。该文件作为整个项目
 * 的核心配置文件，定义了应用程序的版本信息、窗口配置、服务器
 * 端口以及前后端通信所需的数据结构。
 *
 * MiraTrive/MikuTrive
 * Author: [Your Name]
 *
 * 本文件遵循Linux内核代码风格规范：
 * - 缩进使用Tab字符（8字符宽度）
 * - 函数定义使用K&R括号风格
 * - 常量使用全大写下划线分隔
 * - 每个结构体和枚举都有详细注释
 */

#ifndef MIKUFY_MAIN_H
#define MIKUFY_MAIN_H

/*
 * ============================================================================
 * 标准库头文件包含
 * ============================================================================
 */

#include <string>		/* std::string 字符串类型 */
#include <vector>		/* std::vector 动态数组 */
#include <map>			/* std::map 关联容器 */
#include <memory>		/* std::unique_ptr 智能指针 */
#include <functional>		/* std::function 函数对象 */
#include <thread>		/* std::thread 线程支持 */
#include <mutex>		/* std::mutex 互斥锁 */
#include <atomic>		/* std::atomic 原子操作 */
#include <condition_variable>	/* std::condition_variable 条件变量 */

/*
 * ============================================================================
 * GTK和WebKit头文件包含（Fedora 43使用WebKitGTK 6.0和GTK4）
 * ============================================================================
 */
#include <webkit/webkit.h>	/* WebKitWebKit WebView组件 */
#include <gtk/gtk.h>		/* GTK4 图形用户界面库 */

/*
 * ============================================================================
 * 系统调用和文件系统头文件
 * ============================================================================
 */
#include <sys/stat.h>		/* stat() 文件状态查询 */
#include <sys/types.h>		/* pid_t, size_t等系统类型定义 */
#include <dirent.h>		/* opendir(), readdir() 目录操作 */
#include <unistd.h>		/* close(), fork()等Unix系统调用 */
#include <fcntl.h>		/* open(), O_RDONLY等文件控制标志 */

/*
 * ============================================================================
 * 字符串和实用工具头文件
 * ============================================================================
 */
#include <cstring>		/* strlen(), strcpy()等C字符串函数 */
#include <cstdlib>		/* malloc(), free()等内存管理函数 */

/*
 * ============================================================================
 * 第三方库头文件
 * ============================================================================
 */

/*
 * libmagic - 文件类型检测库
 * 用于根据文件内容检测文件的MIME类型，判断文件是否为二进制文件
 */
#include <magic.h>

/*
 * nlohmann/json - 现代C++ JSON库
 * 用于JSON数据的序列化和反序列化，处理前后端通信的数据格式
 */
#include <nlohmann/json.hpp>

/* 使用nlohmann命名空间下的json类型 */
using json = nlohmann::json;

/*
 * ============================================================================
 * 版本信息宏定义
 * ============================================================================
 */

/* 应用程序名称 */
#define MIKUFY_NAME		"Mikufy"

/* 当前版本号 */
#define MIKUFY_VERSION		"2.3(stable)"

/*
 * ============================================================================
 * 窗口配置常量
 * ============================================================================
 */

/* 窗口默认宽度（像素） */
#define WINDOW_WIDTH		1400

/* 窗口默认高度（像素） */
#define WINDOW_HEIGHT		900

/* 窗口标题文本 */
#define WINDOW_TITLE		"Mikufy v2.3(stable) - 代码编辑器"

/*
 * ============================================================================
 * Web服务器配置常量
 * ============================================================================
 */

/* Web服务器监听端口号 */
#define WEB_SERVER_PORT		8080

/*
 * ============================================================================
 * 缓存配置常量
 * ============================================================================
 */

/* 缓存大小限制（字节），约100MB */
#define MAX_CACHE_SIZE		104857600

/*
 * ============================================================================
 * 数据结构定义
 * ============================================================================
 */

/**
 * FileInfo - 文件信息结构体
 *
 * 该结构体用于存储单个文件或目录的详细信息，包括名称、路径、
 * 大小、类型等元数据。该结构体在文件浏览和文件操作中广泛使用。
 *
 * 成员说明:
 * @name: 文件或目录的名称（不含路径）
 * @path: 文件或目录的完整绝对路径
 * @is_directory: 标识是否为目录（true=目录，false=文件）
 * @size: 文件大小（字节），目录的大小为0
 * @mime_type: 文件的MIME类型字符串，使用libmagic检测
 * @is_binary: 标识是否为二进制文件（true=二进制，false=文本）
 */
struct FileInfo {
	std::string name;		/* 文件/目录名称 */
	std::string path;		/* 完整路径 */
	bool is_directory;		/* 是否为目录 */
	size_t size;			/* 文件大小（字节） */
	std::string mime_type;		/* MIME类型 */
	bool is_binary;			/* 是否为二进制文件 */
};

/**
 * HttpResponse - HTTP响应结构体
 *
 * 该结构体用于表示HTTP协议的响应数据，包含状态码、状态文本、
 * 响应头和响应体。Web服务器使用该结构体构造对前端请求的响应。
 *
 * 成员说明:
 * @status_code: HTTP状态码（如200表示成功，404表示未找到）
 * @status_text: HTTP状态文本描述（如"OK"、"Not Found"）
 * @headers: HTTP响应头的键值对映射
 * @body: HTTP响应体内容（字符串形式）
 */
struct HttpResponse {
	int status_code;						/* 状态码 */
	std::string status_text;					/* 状态文本 */
	std::map<std::string, std::string> headers;	/* 响应头映射 */
	std::string body;						/* 响应体内容 */
};

/**
 * HttpHandler - HTTP请求处理函数类型定义
 *
 * 这是一个函数类型别名，定义了HTTP请求处理器的签名。所有
 * API端点处理函数都必须符合此签名。该类型用于构建路由表，
 * 将URL路径映射到对应的处理函数。
 *
 * 参数说明:
 * @path: 请求的URL路径（可能包含查询参数）
 * @headers: 请求头键值对映射
 * @body: 请求体内容（POST/PUT请求）
 *
 * 返回值: HttpResponse结构体，包含处理后的响应数据
 */
using HttpHandler = std::function<HttpResponse(
			const std::string &,
			const std::map<std::string, std::string> &,
			const std::string &)>;

/**
 * MessageType - 前端与后端通信的消息类型枚举
 *
 * 该枚举定义了前端JavaScript与C++后端之间所有可能的通信消息类型。
 * 每种消息类型对应一个特定的操作或功能。前端通过发送不同类型的
 * 消息请求后端执行相应的操作。
 */
enum class MessageType {
	OPEN_FOLDER_DIALOG,	/* 请求打开文件夹选择对话框 */
	GET_DIRECTORY_CONTENTS,	/* 请求获取指定目录的内容列表 */
	READ_FILE,		/* 请求读取文件内容 */
	SAVE_FILE,		/* 请求保存文件内容 */
	CREATE_FOLDER,		/* 请求创建新文件夹 */
	CREATE_FILE,		/* 请求创建新文件 */
	DELETE_ITEM,		/* 请求删除文件或文件夹 */
	RENAME_ITEM,		/* 请求重命名文件或文件夹 */
	GET_FILE_INFO,		/* 请求获取文件详细信息 */
	SAVE_ALL,		/* 请求保存所有已打开的文件 */
	REFRESH,		/* 请求刷新文件列表 */
	UNKNOWN			/* 未知的消息类型 */
};

/**
 * FrontendMessage - 前端消息结构体
 *
 * 该结构体封装了从前端发送到后端的消息数据。每个消息包含一个
 * 消息类型和关联的数据载荷（JSON格式）。
 *
 * 成员说明:
 * @type: 消息类型，枚举值MessageType
 * @data: 消息数据，JSON对象，包含操作所需的具体参数
 */
struct FrontendMessage {
	MessageType type;	/* 消息类型 */
	json data;		/* 消息数据（JSON格式） */
};

/**
 * BackendResponse - 后端响应结构体
 *
 * 该结构体封装了后端对前端请求的响应数据。每个响应包含操作是否
 * 成功的标志、错误信息（如果失败）和返回的数据。
 *
 * 成员说明:
 * @success: 操作是否成功（true=成功，false=失败）
 * @error: 错误信息字符串，仅在success为false时有意义
 * @data: 响应数据，JSON对象，包含操作结果或请求的数据
 */
struct BackendResponse {
	bool success;		/* 操作是否成功 */
	std::string error;	/* 错误信息 */
	json data;		/* 响应数据 */
};

#endif /* MIKUFY_MAIN_H */
