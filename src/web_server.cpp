/*
 * Mikufy v2.11-nova - Web服务器实现
 *
 * 本文件实现了HTTP服务器功能，负责与前端进行通信。
 * 主要功能包括：
 * - 创建和监听HTTP服务器套接字
 * - 处理客户端HTTP请求（GET、POST等）
 * - 提供文件操作API（读取、写入、删除、重命名等）
 * - 处理静态文件请求
 * - 支持路由机制
 * - 高性能编辑器 API（基于 TextBuffer 虚拟化渲染）
 *
 * Mikufy Project
 */

#include "../headers/web_server.h"
#include "../headers/text_buffer.h"
#include "../headers/terminal_manager.h"
#include "../headers/process_launcher.h"
#include "../headers/terminal_window.h"
#include <iostream>
#include <cstring>
#include <sstream>
#include <algorithm>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/select.h>
#include <sys/types.h>
#include <limits.h>
#include <stdio.h>	/* popen(), pclose() */
#include <sys/wait.h>	/* WIFEXITED(), WEXITSTATUS() */
#include <unordered_map>

/**
 * WebServer::WebServer - Web服务器构造函数
 * @file_manager: 文件管理器指针，用于文件操作
 *
 * 初始化WebServer实例，设置默认参数并注册所有API路由。
 *
 * 注意：构造函数不会自动启动服务器，需要显式调用start()方法。
 */
WebServer::WebServer(FileManager *file_manager)
	: file_manager(file_manager), server_socket(-1),
	  port(WEB_SERVER_PORT), running(false), web_root_path(""),
	  terminal_manager(std::make_unique<TerminalManager>())
{
	register_routes(); /* 注册所有API路由处理器 */

	/* 启动终端管理器 */
	if (terminal_manager) {
		auto result = terminal_manager->start();
		if (!result.has_value()) {
			std::cerr << "启动终端管理器失败: " << result.error()
				  << std::endl;
		}
	}
}

/**
 * WebServer::~WebServer - Web服务器析构函数
 *
 * 清理资源，确保服务器停止运行并关闭所有连接。
 */
WebServer::~WebServer(void)
{
	stop(); /* 停止服务器并释放资源 */

	/* 停止终端管理器 */
	if (terminal_manager)
		terminal_manager->stop();

	/* 清理所有 TextBuffer */
	std::lock_guard<std::mutex> lock(text_buffers_mutex);
	for (auto &pair : text_buffers) {
		delete pair.second;
	}
	text_buffers.clear();
}

/**
 * WebServer::set_web_root_path - 设置web资源根目录的绝对路径
 * @path: web目录的绝对路径
 *
 * 设置web资源文件所在的目录路径，用于确保无论程序从哪个目录启动，
 * 都能正确找到和修改web资源文件（如style.css）。
 */
void WebServer::set_web_root_path(const std::string &path)
{
	web_root_path = path;
}

/**
 * WebServer::set_open_folder_callback - 设置打开文件夹对话框的回调函数
 * @callback: 回调函数，返回用户选择的文件夹路径
 *
 * 设置用于打开文件夹对话框的回调函数。当收到/api/open-folder请求时，
 * 会调用此回调函数获取用户选择的文件夹路径。
 *
 * 注意：此函数是线程安全的，使用互斥锁保护。
 */
void WebServer::set_open_folder_callback(std::function<std::string(void)> callback)
{
	std::lock_guard<std::mutex> lock(mutex);
	open_folder_callback = callback;
}

/**
 * WebServer::start - 启动Web服务器
 * @port_num: 监听端口号
 *
 * 创建并启动HTTP服务器，绑定到指定的端口，开始监听客户端连接。
 *
 * 服务器将绑定到localhost (127.0.0.1)，仅接受本地连接。
 * 服务器以非阻塞模式运行，使用单独的线程处理请求。
 *
 * 返回: 成功返回true，失败返回false
 */
bool WebServer::start(int port_num)
{
	std::lock_guard<std::mutex> lock(mutex);

	if (running)
		return false; /* 服务器已在运行 */

	port = port_num;

	/* 创建TCP socket */
	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket < 0)
		return false;

	/* 设置socket选项，允许地址重用 */
	int opt = 1;
	if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt,
		       sizeof(opt)) < 0) {
		close(server_socket);
		server_socket = -1;
		return false;
	}

	/* 绑定地址到localhost */
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	server_addr.sin_port = htons(port);

	if (bind(server_socket, (struct sockaddr *)&server_addr,
		 sizeof(server_addr)) < 0) {
		close(server_socket);
		server_socket = -1;
		return false;
	}

	/* 开始监听，最大10个待处理连接 */
	if (listen(server_socket, 10) < 0) {
		close(server_socket);
		server_socket = -1;
		return false;
	}

	/* 设置为非阻塞模式，避免阻塞主线程 */
	int flags = fcntl(server_socket, F_GETFL, 0);
	fcntl(server_socket, F_SETFL, flags | O_NONBLOCK);

	running = true;

	/* 启动服务器主循环线程 */
	server_thread = std::thread(&WebServer::server_loop, this);

	return true;
}

/**
 * WebServer::stop - 停止Web服务器
 *
 * 停止服务器运行，关闭所有连接并清理资源。
 * 会等待服务器线程正常结束。
 */
void WebServer::stop(void)
{
	std::lock_guard<std::mutex> lock(mutex);

	if (!running)
		return; /* 服务器未运行 */

	running = false; /* 通知服务器线程退出 */

	/* 关闭服务器socket，中断accept调用 */
	if (server_socket >= 0) {
		close(server_socket);
		server_socket = -1;
	}

	/* 等待服务器线程结束 */
	if (server_thread.joinable())
		server_thread.join();
}

/**
 * WebServer::is_running - 检查服务器是否正在运行
 *
 * 返回: 运行中返回true，否则返回false
 */
bool WebServer::is_running(void) const
{
	return running;
}

/**
 * WebServer::server_loop - 服务器主循环
 *
 * 使用poll系统调用监听客户端连接请求。
 * 使用poll而不是select以避免FD_SETSIZE限制。
 * 每100ms检查一次是否有新连接。
 *
 * 注意：此函数在单独的线程中运行。
 */
void WebServer::server_loop(void)
{
	while (running) {
		/* 检查server_socket是否有效 */
		if (server_socket < 0) {
			std::cerr << "错误: 服务器socket无效" << std::endl;
			break;
		}

		/* 使用poll代替select，避免FD_SETSIZE限制 */
		struct pollfd fds[1];
		fds[0].fd = server_socket;
		fds[0].events = POLLIN; /* 监听可读事件 */
		fds[0].revents = 0;

		int ready = poll(fds, 1, 100); /* 100ms超时 */

		if (ready < 0) {
			if (errno == EINTR)
				continue; /* 被信号中断，继续循环 */
			std::cerr << "poll调用失败: " << strerror(errno)
				  << std::endl;
			break;
		}

		/* 有新连接到达 */
		if (ready > 0 && (fds[0].revents & POLLIN)) {
			int client_socket = accept_client();
			if (client_socket >= 0)
				handle_client(client_socket);
		}
	}
}

/**
 * WebServer::accept_client - 接受客户端连接
 *
 * 接受一个等待中的客户端连接请求。
 *
 * 返回: 成功返回客户端socket描述符，失败返回-1
 */
int WebServer::accept_client(void)
{
	struct sockaddr_in client_addr;
	socklen_t client_addr_len = sizeof(client_addr);

	int client_socket =
		accept(server_socket, (struct sockaddr *)&client_addr,
		       &client_addr_len);
	if (client_socket < 0)
		return -1;

	return client_socket;
}

/**
 * WebServer::handle_client - 处理客户端请求
 * @client_socket: 客户端socket描述符
 *
 * 接收、解析并处理HTTP请求，然后发送响应。
 * 支持GET和POST请求，能够处理带请求体的请求（如POST JSON）。
 *
 * 请求处理流程：
 * 1. 循环接收请求数据，直到收到完整的HTTP请求
 * 2. 解析HTTP请求行、请求头和请求体
 * 3. 根据URL路径查找对应的路由处理器
 * 4. 执行处理器生成HTTP响应
 * 5. 发送响应给客户端
 * 6. 关闭连接
 *
 * 注意：每次处理完请求后关闭连接（HTTP/1.0风格）。
 */
void WebServer::handle_client(int client_socket)
{
	/* 接收请求数据 */
	char buffer[8192];
	std::string request;

	/* 循环接收直到收到完整的请求 */
	while (true) {
		ssize_t bytes_read = recv(client_socket, buffer,
				  sizeof(buffer) - 1, 0);

		if (bytes_read <= 0)
			break; /* 连接关闭或错误 */

		buffer[bytes_read] = '\0';
		request.append(buffer, bytes_read);

		/* 检查是否已经收到完整的请求头 */
		size_t header_end = request.find("\r\n\r\n");
		if (header_end != std::string::npos) {
			/* 查找Content-Length以确定是否还有请求体 */
			size_t content_length_pos =
				request.find("Content-Length:");
			if (content_length_pos != std::string::npos &&
			    content_length_pos < header_end) {
				/* 提取Content-Length的值 */
				size_t colon_pos =
					request.find(':', content_length_pos);
				size_t end_pos =
					request.find("\r\n", colon_pos);
				std::string length_str = request.substr(
					colon_pos + 1,
					end_pos - colon_pos - 1);
				/* 去除首尾空白字符 */
				length_str.erase(0, length_str.find_first_not_of(
							" \t"));
				length_str.erase(length_str.find_last_not_of(
							" \t\r\n") + 1);
				size_t content_length =
					std::stoul(length_str);

				/* 检查是否已经收到完整的body */
				size_t body_start = header_end + 4;
				if (request.length() >=
				    body_start + content_length)
					break; /* 收到完整请求 */
			} else {
				/* 没有Content-Length，可能是GET请求，已经收到完整请求 */
				break;
			}
		} else if (request.length() > 8192) {
			/* 没有找到请求头结束符，并且已经接收了很多数据，可能是错误的请求 */
			break;
		}
	}

	/* 解析HTTP请求 */
	std::string method, path;
	std::map<std::string, std::string> headers;
	std::string body;

	if (!parse_http_request(request, method, path, headers, body)) {
		close(client_socket);
		return;
	}

	std::cout << "收到HTTP请求: " << method << " " << path << std::endl;

	/* 处理请求 */
	HttpResponse response;

	/* 去除查询参数获取路由路径 */
	std::string route_path = path;
	size_t query_pos = path.find('?');
	if (query_pos != std::string::npos)
		route_path = path.substr(0, query_pos);

	/* 查找路由处理器 */
	auto it = routes.find(route_path);
	if (it != routes.end()) {
		std::cout << "路由匹配成功: " << route_path << ", body长度: "
			  << body.length() << std::endl;
		response = it->second(path, headers, body);
	} else {
		/* 处理静态文件请求 */
		response = handle_static_file(path);
	}

	/* 发送HTTP响应 */
	send_http_response(client_socket, response);

	/* 关闭连接 */
	close(client_socket);
}


/**
 * WebServer::parse_http_request - 解析HTTP请求
 * @request: 原始HTTP请求数据
 * @method: 输出参数，存储HTTP方法（GET、POST等）
 * @path: 输出参数，存储请求路径
 * @headers: 输出参数，存储请求头键值对
 * @body: 输出参数，存储请求体内容
 *
 * 解析原始HTTP请求数据，提取请求方法、路径、请求头和请求体。
 * 支持标准HTTP/1.1格式。
 *
 * 优化要点:
 * - 使用string_view避免字符串拷贝（C++17特性）
 * - 优化空白字符去除，使用find_first_not_of和find_last_not_of
 * - 减少不必要的字符串操作
 *
 * 返回: 解析成功返回true，失败返回false
 */
bool WebServer::parse_http_request(
	const std::string &request, std::string &method, std::string &path,
	std::map<std::string, std::string> &headers, std::string &body)
{
	/*
	 * 解析请求行 (如: GET /index.html HTTP/1.1)
	 * 请求行格式: METHOD PATH VERSION
	 */
	const size_t line_end = request.find("\r\n");
	if (line_end == std::string::npos)
		return false;

	const std::string request_line = request.substr(0, line_end);

	/*
	 * 使用istringstream解析请求行
	 * 提取方法和路径
	 */
	std::istringstream line_stream(request_line);
	line_stream >> method >> path;

	/*
	 * 验证方法和路径是否有效
	 */
	if (method.empty() || path.empty())
		return false;

	/*
	 * 查找请求头结束位置（"\r\n\r\n"）
	 */
	const size_t header_end = request.find("\r\n\r\n", line_end);
	if (header_end == std::string::npos)
		return false;

	/*
	 * 解析请求头
	 * 从请求行结束位置开始，到"\r\n\r\n"结束
	 */
	size_t header_pos = line_end + 2; /* 跳过第一个"\r\n" */

	while (header_pos < header_end) {
		/*
		 * 查找当前行的结束位置
		 */
		const size_t line_end_pos = request.find("\r\n", header_pos);
		if (line_end_pos == std::string::npos || line_end_pos >= header_end)
			break;

		/*
		 * 提取当前行
		 */
		const std::string line = request.substr(header_pos,
							line_end_pos - header_pos);

		/*
		 * 查找冒号分隔符
		 */
		const size_t colon_pos = line.find(':');
		if (colon_pos != std::string::npos) {
			/*
			 * 提取键和值
			 */
			std::string key = line.substr(0, colon_pos);
			std::string value = line.substr(colon_pos + 1);

			/*
			 * 优化：去除键的尾部空白字符
			 * 使用find_last_not_of一次性查找
			 */
			const size_t key_end = key.find_last_not_of(" \t\r");
			if (key_end != std::string::npos)
				key.resize(key_end + 1);

			/*
			 * 优化：去除值的首尾空白字符
			 * 使用find_first_not_of和find_last_not_of
			 */
			const size_t value_start = value.find_first_not_of(" \t");
			const size_t value_end = value.find_last_not_of(" \t\r");

			if (value_start != std::string::npos &&
			    value_end != std::string::npos) {
				value = value.substr(value_start,
						    value_end - value_start + 1);
			} else {
				value.clear();
			}

			/*
			 * 添加到请求头映射
			 */
			headers[key] = value;
		}

		/*
		 * 移动到下一行
		 */
		header_pos = line_end_pos + 2;
	}

	/*
	 * 解析请求体（仅POST/PUT请求）
	 * 使用布尔表达式而非嵌套if
	 */
	const bool has_body = (method == "POST" || method == "PUT");

	if (has_body) {
		/*
		 * 查找Content-Length头部
		 */
		const auto it = headers.find("Content-Length");

		/*
		 * 如果找到Content-Length，提取请求体
		 */
		if (it != headers.end()) {
			/*
			 * 解析Content-Length值
			 */
			const size_t content_length = std::stoul(it->second);

			/*
			 * 计算请求体起始位置（跳过"\r\n\r\n"）
			 */
			const size_t body_start = header_end + 4;

			/*
			 * 验证请求体长度是否合法
			 */
			if (body_start + content_length <= request.length()) {
				body = request.substr(body_start, content_length);
			}
		}
	}

	return true;
}

/**
 * WebServer::build_http_response - 构造HTTP响应字符串
 * @response: 响应结构体，包含状态码、状态文本、响应头和响应体
 *
 * 将HttpResponse结构体转换为符合HTTP/1.1规范的响应字符串。
 *
 * 返回: HTTP响应字符串
 */
std::string WebServer::build_http_response(const HttpResponse &response)
{
	std::ostringstream oss;

	/* 状态行 (如: HTTP/1.1 200 OK) */
	oss << "HTTP/1.1 " << response.status_code << " "
	    << response.status_text << "\r\n";

	/* 响应头 */
	for (const auto &header : response.headers)
		oss << header.first << ": " << header.second << "\r\n";

	/* 空行分隔响应头和响应体 */
	oss << "\r\n";

	/* 响应体 */
	oss << response.body;

	return oss.str();
}

/**
 * WebServer::send_http_response - 发送HTTP响应
 * @client_socket: 客户端socket描述符
 * @response: 响应结构体
 *
 * 构造并发送HTTP响应给客户端。
 *
 * 返回: 发送成功返回true，失败返回false
 */
bool WebServer::send_http_response(int client_socket,
		   const HttpResponse &response)
{
	std::string response_str = build_http_response(response);

	ssize_t sent = send(client_socket, response_str.c_str(),
			    response_str.length(), 0);

	return (sent == static_cast<ssize_t>(response_str.length()));
}

/**
 * WebServer::url_decode - URL解码
 * @encoded: URL编码的字符串
 *
 * 将URL编码的字符串解码为原始字符串。
 * 处理%XX格式的十六进制编码和+号表示的空格。
 *
 * 返回: 解码后的字符串
 */
std::string WebServer::url_decode(const std::string &encoded)
{
	std::string decoded;
	decoded.reserve(encoded.length());

	for (size_t i = 0; i < encoded.length(); ++i) {
		if (encoded[i] == '%' && i + 2 < encoded.length()) {
			/* 处理%XX格式的十六进制编码 */
			std::string hex_str = encoded.substr(i + 1, 2);
			char decoded_char = static_cast<char>(
				std::stoi(hex_str, nullptr, 16));
			decoded += decoded_char;
			i += 2;
		} else if (encoded[i] == '+') {
			/* +号表示空格 */
			decoded += ' ';
		} else {
			/* 普通字符直接保留 */
			decoded += encoded[i];
		}
	}

	return decoded;
}

/**
 * WebServer::url_encode - URL编码
 * @decoded: 原始字符串
 *
 * 将字符串进行URL编码，保留字母数字和部分特殊字符。
 * 空格编码为+号，其他特殊字符编码为%XX格式。
 *
 * 返回: URL编码后的字符串
 */
std::string WebServer::url_encode(const std::string &decoded)
{
	std::string encoded;
	encoded.reserve(decoded.length() * 3);

	for (char c : decoded) {
		/* 保留字母数字和部分特殊字符 */
		if (isalnum(static_cast<unsigned char>(c)) || c == '-' ||
		    c == '_' || c == '.' || c == '~') {
			encoded += c;
		} else if (c == ' ') {
			/* 空格编码为+号 */
			encoded += '+';
		} else {
			/* 其他字符编码为%XX格式 */
			char hex[4];
			snprintf(hex, sizeof(hex), "%%%02X",
				 static_cast<unsigned char>(c));
			encoded += hex;
		}
	}

	return encoded;
}

/**
 * WebServer::parse_query_string - 解析URL查询参数
 * @query_string: URL查询字符串（如: path=/home&name=test.txt）
 *
 * 解析URL查询字符串，提取键值对。
 *
 * 返回: 参数映射表
 */
std::map<std::string, std::string>
WebServer::parse_query_string(const std::string &query_string)
{
	std::map<std::string, std::string> params;

	std::istringstream stream(query_string);
	std::string pair;

	while (std::getline(stream, pair, '&')) {
		size_t pos = pair.find('=');
		if (pos != std::string::npos) {
			std::string key = url_decode(pair.substr(0, pos));
			std::string value = url_decode(pair.substr(pos + 1));
			params[key] = value;
		}
	}

	return params;
}

/**
 * WebServer::register_routes - 注册HTTP路由
 *
 * 注册所有API路由及其对应的处理函数。
 * 每个API路由使用lambda表达式绑定到对应的处理方法。
 *
 * API列表：
 * - /api/open-folder: 打开文件夹对话框
 * - /api/directory-contents: 获取目录内容
 * - /api/read-file: 读取文本文件
 * - /api/read-binary-file: 读取二进制文件（图片等）
 * - /api/save-file: 保存文件
 * - /api/create-folder: 创建文件夹
 * - /api/create-file: 创建文件
 * - /api/delete: 删除文件或文件夹
 * - /api/rename: 重命名文件或文件夹
 * - /api/file-info: 获取文件信息
 * - /api/save-all: 批量保存文件
 * - /api/refresh: 刷新文件列表
 * - /api/change-wallpaper: 更换壁纸
 */
void WebServer::register_routes(void)
{
	routes["/api/open-folder"] = [this](
			const std::string &path,
			const std::map<std::string, std::string> &headers,
			const std::string &body) {
		return handle_open_folder_dialog(path, headers, body);
	};

	routes["/api/directory-contents"] = [this](
			const std::string &path,
			const std::map<std::string, std::string> &headers,
			const std::string &body) {
		return handle_get_directory_contents(path, headers, body);
	};

	routes["/api/read-file"] = [this](
			const std::string &path,
			const std::map<std::string, std::string> &headers,
			const std::string &body) {
		return handle_read_file(path, headers, body);
	};

	routes["/api/read-binary-file"] = [this](
			const std::string &path,
			const std::map<std::string, std::string> &headers,
			const std::string &body) {
		return handle_read_binary_file(path, headers, body);
	};

	routes["/api/save-file"] = [this](
			const std::string &path,
			const std::map<std::string, std::string> &headers,
			const std::string &body) {
		return handle_save_file(path, headers, body);
	};

	routes["/api/create-folder"] = [this](
			const std::string &path,
			const std::map<std::string, std::string> &headers,
			const std::string &body) {
		return handle_create_folder(path, headers, body);
	};

	routes["/api/create-file"] = [this](
			const std::string &path,
			const std::map<std::string, std::string> &headers,
			const std::string &body) {
		return handle_create_file(path, headers, body);
	};

	routes["/api/delete"] = [this](
			const std::string &path,
			const std::map<std::string, std::string> &headers,
			const std::string &body) {
		return handle_delete(path, headers, body);
	};

	routes["/api/rename"] = [this](
			const std::string &path,
			const std::map<std::string, std::string> &headers,
			const std::string &body) {
		return handle_rename(path, headers, body);
	};

	routes["/api/file-info"] = [this](
			const std::string &path,
			const std::map<std::string, std::string> &headers,
			const std::string &body) {
		return handle_get_file_info(path, headers, body);
	};

	routes["/api/save-all"] = [this](
			const std::string &path,
			const std::map<std::string, std::string> &headers,
			const std::string &body) {
		return handle_save_all(path, headers, body);
	};

	routes["/api/refresh"] = [this](
			const std::string &path,
			const std::map<std::string, std::string> &headers,
			const std::string &body) {
		return handle_refresh(path, headers, body);
	};

	routes["/api/refresh-directory"] = [this](
			const std::string &path,
			const std::map<std::string, std::string> &headers,
			const std::string &body) {
		return handle_refresh_directory(path, headers, body);
	};

	routes["/api/change-wallpaper"] = [this](
			const std::string &path,
			const std::map<std::string, std::string> &headers,
			const std::string &body) {
		return handle_change_wallpaper(path, headers, body);
	};

	routes["/api/get-wallpapers"] = [this](
			const std::string &path,
			const std::map<std::string, std::string> &headers,
			const std::string &body) {
		return handle_get_wallpapers(path, headers, body);
	};

	/*
	 * 高性能编辑器 API 路由（基于 TextBuffer 虚拟化渲染）
	 */
	routes["/api/open-file-virtual"] = [this](
			const std::string &path,
			const std::map<std::string, std::string> &headers,
			const std::string &body) {
		return handle_open_file_virtual(path, headers, body);
	};

	routes["/api/get-lines"] = [this](
			const std::string &path,
			const std::map<std::string, std::string> &headers,
			const std::string &body) {
		return handle_get_lines(path, headers, body);
	};

	routes["/api/get-line-count"] = [this](
			const std::string &path,
			const std::map<std::string, std::string> &headers,
			const std::string &body) {
		return handle_get_line_count(path, headers, body);
	};

	routes["/api/edit-insert"] = [this](
			const std::string &path,
			const std::map<std::string, std::string> &headers,
			const std::string &body) {
		return handle_edit_insert(path, headers, body);
	};

	routes["/api/edit-delete"] = [this](
			const std::string &path,
			const std::map<std::string, std::string> &headers,
			const std::string &body) {
		return handle_edit_delete(path, headers, body);
	};

	routes["/api/edit-replace"] = [this](
			const std::string &path,
			const std::map<std::string, std::string> &headers,
			const std::string &body) {
		return handle_edit_replace(path, headers, body);
	};

	routes["/api/close-file-virtual"] = [this](
			const std::string &path,
			const std::map<std::string, std::string> &headers,
			const std::string &body) {
		return handle_close_file_virtual(path, headers, body);
	};

	/*
	 * 终端 API 路由
	 */
	routes["/api/terminal-info"] = [this](
			const std::string &path,
			const std::map<std::string, std::string> &headers,
			const std::string &body) {
		return handle_terminal_info(path, headers, body);
	};

	routes["/api/terminal-execute"] = [this](
			const std::string &path,
			const std::map<std::string, std::string> &headers,
			const std::string &body) {
		return handle_terminal_execute(path, headers, body);
	};

	routes["/api/terminal-get-output"] = [this](
			const std::string &path,
			const std::map<std::string, std::string> &headers,
			const std::string &body) {
		return handle_terminal_get_output(path, headers, body);
	};

	routes["/api/terminal-send-input"] = [this](
			const std::string &path,
			const std::map<std::string, std::string> &headers,
			const std::string &body) {
		return handle_terminal_send_input(path, headers, body);
	};

	routes["/api/terminal-kill-process"] = [this](
			const std::string &path,
			const std::map<std::string, std::string> &headers,
			const std::string &body) {
		return handle_terminal_kill_process(path, headers, body);
	};
}

/**
 * WebServer::handle_open_folder_dialog - 处理打开文件夹对话框API
 * @path: 请求路径（未使用）
 * @headers: 请求头（未使用）
 * @body: 请求体（未使用）
 *
 * 调用WindowManager提供的文件夹选择对话框，获取用户选择的文件夹路径。
 *
 * 返回: JSON响应，包含success和path字段
 */
HttpResponse WebServer::handle_open_folder_dialog(
	const std::string &path, const std::map<std::string, std::string> &headers,
	const std::string &body)
{
	(void)path;
	(void)headers;
	(void)body;

	HttpResponse response;
	response.status_code = 200;
	response.status_text = "OK";
	response.headers["Content-Type"] = "application/json";

	std::string folder_path;
	try {
		/* 调用回调函数打开文件夹对话框 */
		if (open_folder_callback)
			folder_path = open_folder_callback();
	} catch (const std::exception &e) {
		std::cerr << "打开文件夹对话框异常: " << e.what() << std::endl;
		folder_path = "";
	} catch (...) {
		std::cerr << "打开文件夹对话框发生未知异常" << std::endl;
		folder_path = "";
	}

	json result;
	result["success"] = !folder_path.empty();
	result["path"] = folder_path;

	response.body = result.dump();

	return response;
}

/**
 * WebServer::handle_get_directory_contents - 处理获取目录内容API
 * @path: 请求路径，包含查询参数path
 * @headers: 请求头（未使用）
 * @body: 请求体（未使用）
 *
 * 获取指定目录下的所有文件和子目录列表。
 * 查询参数：path - 目录路径
 *
 * 返回: JSON响应，包含success和files数组
 */
HttpResponse WebServer::handle_get_directory_contents(
	const std::string &path, const std::map<std::string, std::string> &headers,
	const std::string &body)
{
	(void)headers;
	(void)body;

	HttpResponse response;
	response.status_code = 200;
	response.status_text = "OK";
	response.headers["Content-Type"] = "application/json";

	/* 解析查询参数 */
	size_t query_pos = path.find('?');
	std::string query_str = (query_pos != std::string::npos) ?
				path.substr(query_pos + 1) :
				"";
	auto params = parse_query_string(query_str);

	std::string directory_path = params["path"];

	if (directory_path.empty()) {
		json result;
		result["success"] = false;
		result["error"] = "Path parameter is required";
		response.body = result.dump();
		return response;
	}

	std::vector<FileInfo> files;
	bool success = file_manager->get_directory_contents(directory_path,
					     files);

	json result;
	result["success"] = success;

	if (success) {
		json files_array = json::array();
		for (const auto &file : files) {
			json file_obj;
			file_obj["name"] = file.name;
			file_obj["path"] = file.path;
			file_obj["isDirectory"] = file.is_directory;
			file_obj["size"] = file.size;
			file_obj["mimeType"] = file.mime_type;
			file_obj["isBinary"] = file.is_binary;
			files_array.push_back(file_obj);
		}
		result["files"] = files_array;
	}

	response.body = result.dump();

	return response;
}

/**
 * WebServer::handle_read_file - 处理读取文件API
 * @path: 请求路径，包含查询参数path
 * @headers: 请求头（未使用）
 * @body: 请求体（未使用）
 *
 * 读取指定文本文件的内容。
 * 查询参数：path - 文件路径
 *
 * 返回: JSON响应，包含success和content字段
 */
HttpResponse WebServer::handle_read_file(
	const std::string &path, const std::map<std::string, std::string> &headers,
	const std::string &body)
{
	(void)headers;
	(void)body;

	HttpResponse response;
	response.status_code = 200;
	response.status_text = "OK";
	response.headers["Content-Type"] = "application/json";

	/* 解析查询参数 */
	size_t query_pos = path.find('?');
	std::string query_str = (query_pos != std::string::npos) ?
				path.substr(query_pos + 1) :
				"";
	auto params = parse_query_string(query_str);

	std::string file_path = params["path"];
	if (file_path.empty()) {
		json result;
		result["success"] = false;
		result["error"] = "Path parameter is required";
		response.body = result.dump();
		return response;
	}

	std::string content;
	bool success = file_manager->read_file(file_path, content);

	json result;
	result["success"] = success;
	result["content"] = content;

	response.body = result.dump();
	return response;
}

/* 处理读取二进制文件API（用于图片、视频等） */
HttpResponse WebServer::handle_read_binary_file(
	const std::string &path, const std::map<std::string, std::string> &headers,
	const std::string &body)
{
	(void)headers;
	(void)body;

	HttpResponse response;
	response.status_code = 200;
	response.status_text = "OK";

	/* 解析查询参数 */
	size_t query_pos = path.find('?');
	std::string query_str = (query_pos != std::string::npos) ?
				path.substr(query_pos + 1) :
				"";
	auto params = parse_query_string(query_str);

	std::string file_path = params["path"];
	if (file_path.empty()) {
		response.status_code = 400;
		response.status_text = "Bad Request";
		response.body = "Path parameter is required";
		return response;
	}

	/* 获取文件MIME类型 */
	std::string mime_type = file_manager->get_mime_type(file_path);
	response.headers["Content-Type"] = mime_type;

	/* 读取文件内容 */
	std::vector<char> content;
	bool success = file_manager->read_file_binary(file_path, content);

	if (!success) {
		response.status_code = 404;
		response.status_text = "Not Found";
		response.body = "File not found or cannot be read";
		return response;
	}

	response.body = std::string(content.begin(), content.end());
	return response;
}

/**
 * WebServer::handle_save_file - 处理保存文件API
 * @path: 请求路径（未使用）
 * @headers: 请求头（未使用）
 * @body: JSON请求体，包含path和content字段
 *
 * 保存文本文件内容。
 * 请求体：{ "path": "文件路径", "content": "文件内容" }
 *
 * 返回: JSON响应，包含success字段
 */
HttpResponse WebServer::handle_save_file(
	const std::string &path, const std::map<std::string, std::string> &headers,
	const std::string &body)
{
	(void)path;
	(void)headers;

	HttpResponse response;
	response.status_code = 200;
	response.status_text = "OK";
	response.headers["Content-Type"] = "application/json";

	try {
		json request = json::parse(body);
		std::string file_path = request["path"];
		std::string content = request["content"];

		bool success = file_manager->write_file(file_path, content);

		json result;
		result["success"] = success;

		response.body = result.dump();
	} catch (const std::exception &e) {
		json result;
		result["success"] = false;
		result["error"] = e.what();
		response.body = result.dump();
	}

	return response;
}

/* 处理创建文件夹API */
HttpResponse WebServer::handle_create_folder(
	const std::string &path, const std::map<std::string, std::string> &headers,
	const std::string &body)
{
	(void)path;
	(void)headers;

	std::cout << "收到创建文件夹请求，body: " << body << std::endl;

	HttpResponse response;
	response.status_code = 200;
	response.status_text = "OK";
	response.headers["Content-Type"] = "application/json";

	try {
		json request = json::parse(body);
		std::string parent_path = request["parentPath"];
		std::string name = request["name"];

		std::cout << "创建文件夹: parentPath=" << parent_path
			  << ", name=" << name << std::endl;

		std::string folder_path = parent_path;
		if (folder_path.back() != '/')
			folder_path += '/';
		folder_path += name;

		bool success = file_manager->create_directory(folder_path);

		std::cout << "创建文件夹结果: "
			  << (success ? "成功" : "失败") << std::endl;

		json result;
		result["success"] = success;

		response.body = result.dump();
	} catch (const std::exception &e) {
		std::cout << "创建文件夹异常: " << e.what() << std::endl;
		json result;
		result["success"] = false;
		result["error"] = e.what();
		response.body = result.dump();
	}

	return response;
}

/* 处理创建文件API */
HttpResponse WebServer::handle_create_file(
	const std::string &path, const std::map<std::string, std::string> &headers,
	const std::string &body)
{
	(void)path;
	(void)headers;

	std::cout << "收到创建文件请求，body: " << body << std::endl;

	HttpResponse response;
	response.status_code = 200;
	response.status_text = "OK";
	response.headers["Content-Type"] = "application/json";

	try {
		json request = json::parse(body);
		std::string parent_path = request["parentPath"];
		std::string name = request["name"];

		std::cout << "创建文件: parentPath=" << parent_path
			  << ", name=" << name << std::endl;

		std::string file_path = parent_path;
		if (file_path.back() != '/')
			file_path += '/';
		file_path += name;

		bool success = file_manager->create_file(file_path);

		std::cout << "创建文件结果: " << (success ? "成功" : "失败")
			  << std::endl;

		json result;
		result["success"] = success;

		response.body = result.dump();
	} catch (const std::exception &e) {
		std::cout << "创建文件异常: " << e.what() << std::endl;
		json result;
		result["success"] = false;
		result["error"] = e.what();
		response.body = result.dump();
	}

	return response;
}

/* 处理删除API */
HttpResponse WebServer::handle_delete(
	const std::string &path, const std::map<std::string, std::string> &headers,
	const std::string &body)
{
	(void)path;
	(void)headers;
	(void)body;

	HttpResponse response;
	response.status_code = 200;
	response.status_text = "OK";
	response.headers["Content-Type"] = "application/json";

	try {
		json request = json::parse(body);
		std::string item_path = request["path"];

		bool success = file_manager->delete_item(item_path);

		json result;
		result["success"] = success;

		response.body = result.dump();
	} catch (const std::exception &e) {
		json result;
		result["success"] = false;
		result["error"] = e.what();
		response.body = result.dump();
	}

	return response;
}

/* 处理重命名API */
HttpResponse WebServer::handle_rename(
	const std::string &path, const std::map<std::string, std::string> &headers,
	const std::string &body)
{
	(void)path;
	(void)headers;
	(void)body;

	HttpResponse response;
	response.status_code = 200;
	response.status_text = "OK";
	response.headers["Content-Type"] = "application/json";

	try {
		json request = json::parse(body);
		std::string old_path = request["oldPath"];
		std::string new_path = request["newPath"];

		bool success = file_manager->rename_item(old_path, new_path);

		json result;
		result["success"] = success;

		response.body = result.dump();
	} catch (const std::exception &e) {
		json result;
		result["success"] = false;
		result["error"] = e.what();
		response.body = result.dump();
	}

	return response;
}

/* 处理获取文件信息API */
HttpResponse WebServer::handle_get_file_info(
	const std::string &path, const std::map<std::string, std::string> &headers,
	const std::string &body)
{
	(void)headers;
	(void)body;

	HttpResponse response;
	response.status_code = 200;
	response.status_text = "OK";
	response.headers["Content-Type"] = "application/json";

	/* 解析查询参数 */
	size_t query_pos = path.find('?');
	std::string query_str = (query_pos != std::string::npos) ?
				path.substr(query_pos + 1) :
				"";
	auto params = parse_query_string(query_str);

	std::string file_path = params["path"];
	if (file_path.empty()) {
		json result;
		result["success"] = false;
		result["error"] = "Path parameter is required";
		response.body = result.dump();
		return response;
	}

	FileInfo info;
	bool success = file_manager->get_file_info(file_path, info);

	json result;
	result["success"] = success;
	result["isBinary"] = info.is_binary;
	result["mimeType"] = info.mime_type;

	response.body = result.dump();

	return response;
}

/* 处理保存所有文件API */
HttpResponse WebServer::handle_save_all(
	const std::string &path, const std::map<std::string, std::string> &headers,
	const std::string &body)
{
	(void)path;
	(void)headers;
	(void)body;

	HttpResponse response;
	response.status_code = 200;
	response.status_text = "OK";
	response.headers["Content-Type"] = "application/json";

	try {
		json request = json::parse(body);
		json files = request["files"];

		bool all_success = true;

		for (const auto &file : files) {
			std::string file_path = file[0];
			std::string content = file[1];

			/*
			 * 写入文件
			 */
			if (!file_manager->write_file(file_path, content)) {
				std::cerr << "文件写入失败: " << file_path
					  << std::endl;
				all_success = false;
			} else {
				std::cout << "文件保存成功: " << file_path
					  << " (size=" << content.length()
					  << " bytes)" << std::endl;
			}
		}

		json result;
		result["success"] = all_success;

		response.body = result.dump();
	} catch (const std::exception &e) {
		json result;
		result["success"] = false;
		result["error"] = e.what();
		response.body = result.dump();
	}

	return response;
}

/* 处理刷新API */
HttpResponse WebServer::handle_refresh(
	const std::string &path, const std::map<std::string, std::string> &headers,
	const std::string &body)
{
	(void)path;
	(void)headers;
	(void)body;

	HttpResponse response;
	response.status_code = 200;
	response.status_text = "OK";
	response.headers["Content-Type"] = "application/json";

	json result;
	result["success"] = true;

	response.body = result.dump();

	return response;
}

/**
 * WebServer::handle_refresh_directory - 处理增量刷新目录API
 * @path: 请求路径（未使用）
 * @headers: 请求头（未使用）
 * @body: JSON请求体，包含directory字段
 *
 * 只刷新指定目录的内容，用于智能刷新功能。
 *
 * 返回: JSON响应，包含success、directory、files字段
 */
HttpResponse WebServer::handle_refresh_directory(
	const std::string &path, const std::map<std::string, std::string> &headers,
	const std::string &body)
{
	(void)path;
	(void)headers;

	HttpResponse response;
	response.status_code = 200;
	response.status_text = "OK";
	response.headers["Content-Type"] = "application/json";

	try {
		json request = json::parse(body);
		std::string directory = request["directory"];

		if (directory.empty()) {
			json result;
			result["success"] = false;
			result["error"] = "Directory path is required";
			response.body = result.dump();
			return response;
		}

		/* 只刷新指定目录 */
		std::vector<FileInfo> files;
		bool success = file_manager->get_directory_contents(directory, files);

		json result;
		result["success"] = success;
		result["directory"] = directory;

		/* 转换文件信息为JSON数组 */
		json files_array = json::array();
		for (const auto &file : files) {
			json file_obj;
			file_obj["name"] = file.name;
			file_obj["path"] = file.path;
			file_obj["is_directory"] = file.is_directory;
			file_obj["size"] = file.size;
			file_obj["mime_type"] = file.mime_type;
			file_obj["is_binary"] = file.is_binary;
			files_array.push_back(file_obj);
		}
		result["files"] = files_array;

		response.body = result.dump();
	} catch (const json::exception &e) {
		json result;
		result["success"] = false;
		result["error"] = "Invalid JSON request";
		response.body = result.dump();
	} catch (const std::exception &e) {
		json result;
		result["success"] = false;
		result["error"] = e.what();
		response.body = result.dump();
	}

	return response;
}

/* 处理更换壁纸API */
HttpResponse WebServer::handle_change_wallpaper(
	const std::string &path, const std::map<std::string, std::string> &headers,
	const std::string &body)
{
	(void)path;
	(void)headers;

	HttpResponse response;
	response.status_code = 200;
	response.status_text = "OK";
	response.headers["Content-Type"] = "application/json";

	try {
		json request = json::parse(body);
		std::string filename = request["filename"];

		// 验证文件名格式
		if (filename.empty() || filename.find("index-") != 0) {
			json result;
			result["success"] = false;
			result["error"] = "Invalid wallpaper filename";
			response.body = result.dump();
			return response;
		}

		// 检查文件后缀
		std::string ext = filename.substr(filename.find_last_of('.'));
		if (ext != ".png" && ext != ".jpg" && ext != ".jpeg") {
			json result;
			result["success"] = false;
			result["error"] = "Invalid file extension";
			response.body = result.dump();
			return response;
		}

		// 读取style.css文件
		std::string cssPath;
		if (!web_root_path.empty()) {
			cssPath = web_root_path + "/style.css";
		} else {
			cssPath = "web/style.css";
		}
		std::string cssContent;
		bool success = file_manager->read_file(cssPath, cssContent);

		if (!success) {
			json result;
			result["success"] = false;
			result["error"] = "Failed to read style.css from: " + cssPath;
			response.body = result.dump();
			return response;
		}

		// 替换背景图片
		std::string oldPattern = "background-image: url('Background/";
		std::string newPattern = "background-image: url('Background/" + filename;

		size_t pos = cssContent.find(oldPattern);
		if (pos == std::string::npos) {
			json result;
			result["success"] = false;
			result["error"] = "Background pattern not found in style.css";
			response.body = result.dump();
			return response;
		}

		size_t endPos = cssContent.find("')", pos);
		if (endPos == std::string::npos) {
			json result;
			result["success"] = false;
			result["error"] = "Background pattern not found in style.css";
			response.body = result.dump();
			return response;
		}

		// 替换背景图片文件名
		cssContent.replace(pos, endPos - pos + 2, newPattern + "')");

		// 写回style.css文件
		success = file_manager->write_file(cssPath, cssContent);

		if (!success) {
			json result;
			result["success"] = false;
			result["error"] = "Failed to write style.css";
			response.body = result.dump();
			return response;
		}

		json result;
		result["success"] = true;
		result["message"] = "Wallpaper changed successfully";

		response.body = result.dump();
	} catch (const std::exception &e) {
		json result;
		result["success"] = false;
		result["error"] = e.what();
		response.body = result.dump();
	}

	return response;
}

/**
 * WebServer::handle_get_wallpapers - 处理获取壁纸列表API
 * @path: 请求路径（未使用）
 * @headers: 请求头（未使用）
 * @body: 请求体（未使用）
 *
 * 扫描web/Background目录，返回所有可用的壁纸文件列表。
 * 支持的图片格式：png, jpg, jpeg, gif, webp, svg, bmp
 *
 * 优化要点:
 * - 使用静态哈希表存储支持的图片扩展名，实现O(1)查找
 * - 优化扩展名提取，避免不必要的字符串转换
 * - 预分配JSON数组空间，减少重新分配
 *
 * 返回: JSON响应，包含success和wallpapers数组
 */
HttpResponse WebServer::handle_get_wallpapers(
	const std::string &path, const std::map<std::string, std::string> &headers,
	const std::string &body)
{
	(void)path;
	(void)headers;
	(void)body;

	HttpResponse response;
	response.status_code = 200;
	response.status_text = "OK";
	response.headers["Content-Type"] = "application/json";

	/*
	 * Background目录路径
	 */
	const std::string bg_dir = web_root_path.empty() ?
				   "web/Background" : web_root_path + "/Background";

	/*
	 * 检查目录是否存在
	 */
	DIR *dir = opendir(bg_dir.c_str());
	if (!dir) {
		json result;
		result["success"] = false;
		result["error"] = "Failed to open Background directory: " + bg_dir;
		response.body = result.dump();
		return response;
	}
	closedir(dir);

	/*
	 * 支持的图片扩展名（小写）
	 * 使用静态哈希表实现O(1)平均时间复杂度的查找
	 * 首次调用时初始化，后续调用直接使用
	 */
	static const std::unordered_set<std::string> image_extensions = {
		".png",
		".jpg",
		".jpeg",
		".gif",
		".webp",
		".svg",
		".bmp"
	};

	/*
	 * 获取目录内容
	 */
	std::vector<FileInfo> files;
	const bool success = file_manager->get_directory_contents(bg_dir, files);

	if (!success) {
		json result;
		result["success"] = false;
		result["error"] = "Failed to read Background directory: " + bg_dir;
		result["path"] = bg_dir;
		result["wallpapers"] = json::array();
		response.body = result.dump();
		return response;
	}

	/*
	 * 筛选图片文件
	 * 预分配足够的空间以减少重新分配
	 */
	json wallpapers = json::array();
	wallpapers.get_ref<json::array_t&>().reserve(files.size());

	/*
	 * 遍历文件列表，筛选图片文件
	 */
	for (const auto &file : files) {
		/*
		 * 跳过目录
		 * 使用布尔表达式而非if嵌套
		 */
		if (file.is_directory)
			continue;

		/*
		 * 获取文件扩展名（小写）
		 * 优化：先获取扩展名位置，避免重复查找
		 */
		const size_t dot_pos = file.name.find_last_of('.');

		/*
		 * 没有点号，不是图片文件
		 */
		if (dot_pos == std::string::npos)
			continue;

		/*
		 * 点号在开头（隐藏文件），跳过
		 */
		if (dot_pos == 0)
			continue;

		/*
		 * 提取扩展名并转换为小写
		 * 使用std::transform一次性转换，避免循环
		 */
		std::string ext = file.name.substr(dot_pos);
		std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

		/*
		 * 使用哈希表查找，O(1)时间复杂度
		 * 查找失败返回end()迭代器
		 */
		const bool is_image = (image_extensions.find(ext) !=
				       image_extensions.end());

		/*
		 * 如果是支持的图片格式，添加到结果列表
		 */
		if (is_image) {
			json wallpaper;
			wallpaper["filename"] = file.name;
			wallpaper["path"] = "Background/" + file.name;
			wallpapers.push_back(std::move(wallpaper));
		}
	}

	/*
	 * 按文件名排序
	 * 使用lambda表达式定义比较函数
	 */
	std::sort(wallpapers.begin(), wallpapers.end(),
		  [](const json &a, const json &b) {
			  return a["filename"] < b["filename"];
		  });

	/*
	 * 构造响应
	 */
	json result;
	result["success"] = true;
	result["wallpapers"] = std::move(wallpapers);

	response.body = result.dump();

	return response;
}

/**
 * WebServer::handle_static_file - 处理静态文件请求
 * @path: 请求路径
 *
 * 处理静态文件请求，从web目录提供HTML、CSS、JS、图片等静态资源。
 * 根路径"/"会自动重定向到index.html。
 *
 * 返回: HTTP响应
 */
HttpResponse WebServer::handle_static_file(const std::string &path)
{
	HttpResponse response;

	/* 解析路径 */
	std::string file_path = path;

	/* 移除查询字符串 */
	size_t query_pos = file_path.find('?');
	if (query_pos != std::string::npos)
		file_path = file_path.substr(0, query_pos);

	/* 处理根路径 */
	if (file_path == "/" || file_path == "")
		file_path = "/index.html";

	/* 构建完整的文件路径 */
	std::string full_path;
	if (!web_root_path.empty()) {
		full_path = web_root_path + file_path;
	} else {
		full_path = "web" + file_path;
	}

	std::cout << "静态文件请求: " << path << " -> " << full_path
		  << " (web_root_path: " << (web_root_path.empty() ? "empty" : web_root_path) << ")"
		  << std::endl;

	/* 读取文件 */
	std::string content = read_static_file(full_path);

	if (content.empty()) {
		/* 文件不存在，返回404错误 */
		std::cout << "文件不存在: " << full_path << std::endl;
		response.status_code = 404;
		response.status_text = "Not Found";
		response.headers["Content-Type"] = "text/plain";
		response.body = "404 Not Found: " + full_path;
		return response;
	}

	std::cout << "文件读取成功: " << full_path << " (大小: "
		  << content.size() << " bytes)" << std::endl;

	response.status_code = 200;
	response.status_text = "OK";
	response.headers["Content-Type"] = get_http_mime_type(full_path);
	response.body = content;

	return response;
}

/**
 * WebServer::read_static_file - 读取静态文件内容
 * @file_path: 文件路径
 *
 * 以二进制模式读取静态文件内容。
 *
 * 返回: 文件内容，失败返回空字符串
 */
std::string WebServer::read_static_file(const std::string &file_path)
{
	std::ifstream file(file_path, std::ios::binary);

	if (!file.is_open())
		return ""; /* 文件打开失败 */

	std::stringstream buffer;
	buffer << file.rdbuf();

	file.close();

	return buffer.str();
}

/**
 * WebServer::get_http_mime_type - 获取文件的MIME类型
 * @file_path: 文件路径
 *
 * 根据文件扩展名确定MIME类型，用于HTTP响应头。
 *
 * 优化要点:
 * - 使用unordered_map实现O(1)平均时间复杂度的查找
 * - 将扩展名统一转换为小写，支持大小写不敏感的查找
 * - 优化字符串处理，减少不必要的拷贝
 *
 * 返回: MIME类型字符串
 */
std::string WebServer::get_http_mime_type(const std::string &file_path)
{
	/*
	 * 查找最后一个点号位置
	 */
	const size_t dot_pos = file_path.find_last_of('.');

	/*
	 * 没有点号，返回默认类型
	 */
	if (dot_pos == std::string::npos)
		return "application/octet-stream";

	/*
	 * 提取扩展名（小写）
	 * 跳过点号本身
	 */
	std::string ext = file_path.substr(dot_pos + 1);
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

	/*
	 * 常见文件扩展名与MIME类型映射
	 * 使用unordered_map实现O(1)平均时间复杂度的查找
	 * 相比原有std::map的O(log n)有显著性能提升
	 */
	static const std::unordered_map<std::string, std::string> mime_types = {
		/* 文本文件 */
		{ "html", "text/html" },
		{ "htm", "text/html" },
		{ "css", "text/css" },
		{ "js", "application/javascript" },
		{ "json", "application/json" },
		{ "xml", "application/xml" },
		{ "txt", "text/plain" },
		{ "md", "text/markdown" },

		/* 图片文件 */
		{ "png", "image/png" },
		{ "jpg", "image/jpeg" },
		{ "jpeg", "image/jpeg" },
		{ "gif", "image/gif" },
		{ "webp", "image/webp" },
		{ "svg", "image/svg+xml" },
		{ "bmp", "image/bmp" },
		{ "ico", "image/x-icon" },

		/* 字体文件 */
		{ "woff", "font/woff" },
		{ "woff2", "font/woff2" },
		{ "ttf", "font/ttf" },
		{ "otf", "font/otf" },
		{ "eot", "application/vnd.ms-fontobject" },

		/* 音频文件 */
		{ "mp3", "audio/mpeg" },
		{ "wav", "audio/wav" },
		{ "ogg", "audio/ogg" },
		{ "flac", "audio/flac" },
		{ "aac", "audio/aac" },

		/* 视频文件 */
		{ "mp4", "video/mp4" },
		{ "webm", "video/webm" },
		{ "ogg", "video/ogg" },
		{ "avi", "video/x-msvideo" },
		{ "mov", "video/quicktime" },

		/* 压缩文件 */
		{ "zip", "application/zip" },
		{ "rar", "application/vnd.rar" },
		{ "tar", "application/x-tar" },
		{ "gz", "application/gzip" },
		{ "7z", "application/x-7z-compressed" },

		/* 其他文件 */
		{ "pdf", "application/pdf" },
		{ "doc", "application/msword" },
		{ "docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document" },
		{ "xls", "application/vnd.ms-excel" },
		{ "xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet" }
	};

	/*
	 * 使用哈希表查找，O(1)时间复杂度
	 * find()返回迭代器，未找到返回end()
	 */
	const auto it = mime_types.find(ext);

	/*
	 * 找到返回对应的MIME类型，否则返回默认类型
	 */
	return (it != mime_types.end()) ? it->second : "application/octet-stream";
}

/*
 * ============================================================================
 * 辅助函数 - HTML 转义和语言检测
 * ============================================================================
 */

/**
 * WebServer::escape_html - 转义 HTML 特殊字符
 *
 * 将文本中的 HTML 特殊字符转义为实体引用。
 *
 * @text: 要转义的文本
 *
 * 返回值: 转义后的文本
 */
std::string WebServer::escape_html(const std::string &text)
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
 * WebServer::detect_language_simple - 简化的语言检测函数
 *
 * 根据文件扩展名检测编程语言。
 *
 * @filename: 文件名
 *
 * 返回值: 检测到的语言名称
 */
std::string WebServer::detect_language_simple(const std::string &filename)
{
	/* 文件扩展名到语言的映射表 */
	static const std::unordered_map<std::string, std::string> ext_to_lang = {
		{ ".c", "c" },
		{ ".cpp", "cpp" },
		{ ".h", "cpp" },
		{ ".hpp", "cpp" },
		{ ".js", "javascript" },
		{ ".ts", "typescript" },
		{ ".py", "python" },
		{ ".java", "java" },
		{ ".go", "go" },
		{ ".rs", "rust" },
		{ ".sh", "shell" },
		{ ".html", "html" },
		{ ".css", "css" },
		{ ".json", "json" },
		{ ".xml", "xml" },
		{ ".md", "markdown" },
		{ ".php", "php" },
		{ ".rb", "ruby" },
		{ ".lua", "lua" },
		{ ".kt", "kotlin" },
		{ ".swift", "swift" },
		{ ".dart", "dart" },
		{ ".sql", "sql" },
		{ ".r", "r" },
		{ ".nim", "nim" },
		{ ".ex", "elixir" },
		{ ".erl", "erlang" },
		{ ".hs", "haskell" },
		{ ".ml", "ocaml" },
		{ ".fs", "fsharp" },
		{ ".clj", "clojure" },
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

	/* 查找文件扩展名 */
	const size_t dot_pos = filename.find_last_of('.');
	if (dot_pos != std::string::npos) {
		const std::string ext = filename.substr(dot_pos);
		const auto it = ext_to_lang.find(ext);
		if (it != ext_to_lang.end())
			return it->second;
	}

	return "plaintext";
}

/*
 * ============================================================================
 * 终端 API 实现
 * ============================================================================
 */

/**
 * WebServer::handle_terminal_info - 处理获取终端信息API
 * @path: 请求路径（未使用）
 * @headers: 请求头（未使用）
 * @body: JSON请求体，包含path字段
 *
 * 获取当前用户名、主机名和当前路径信息。
 *
 * 返回: JSON响应，包含success、user、hostname、path、isRoot字段
 */
HttpResponse WebServer::handle_terminal_info(
	const std::string &path, const std::map<std::string, std::string> &headers,
	const std::string &body)
{
	(void)path;
	(void)headers;

	HttpResponse response;
	response.status_code = 200;
	response.status_text = "OK";
	response.headers["Content-Type"] = "application/json";

	try {
		json request = json::parse(body);
		std::string terminal_path = request["path"];

		// 获取用户名
		char *user = getenv("USER");
		if (!user)
			user = getenv("USERNAME");
		if (!user)
			user = (char *)"unknown";

		// 获取主机名
		char hostname[256];
		if (gethostname(hostname, sizeof(hostname)) != 0)
			strcpy(hostname, "localhost");

		// 检查是否为root用户
		bool is_root = (getuid() == 0);

		// 验证路径是否存在
		if (terminal_path.empty())
			terminal_path = "/";

		json result;
		result["success"] = true;
		result["user"] = std::string(user);
		result["hostname"] = std::string(hostname);
		result["path"] = terminal_path;
		result["isRoot"] = is_root;

		response.body = result.dump();
	} catch (const std::exception &e) {
		json result;
		result["success"] = false;
		result["error"] = e.what();
		response.body = result.dump();
	}

	return response;
}

/**
 * WebServer::handle_terminal_execute - 处理执行终端命令API
 * @path: 请求路径（未使用）
 * @headers: 请求头（未使用）
 * @body: JSON请求体，包含command和path字段
 *
 * 在指定路径下执行shell命令。
 * 普通命令在终端视图中执行，需要独立窗口的程序在GTK4窗口中运行。
 *
 * 返回: JSON响应，包含success、output、error、newPath、isRoot、pid字段
 */
HttpResponse WebServer::handle_terminal_execute(
	const std::string &path, const std::map<std::string, std::string> &headers,
	const std::string &body)
{
	(void)path;
	(void)headers;

	HttpResponse response;
	response.status_code = 200;
	response.status_text = "OK";
	response.headers["Content-Type"] = "application/json";

	try {
		json request = json::parse(body);
		std::string command = request["command"];
		std::string terminal_path = request["path"];

		if (terminal_path.empty())
			terminal_path = "/";

		/* 判断是否需要独立窗口运行 */
		bool needs_independent_window = false;

		/* 1. 检查是否是 ./xxx 格式的可执行文件 */
		if (command.find("./") == 0) {
			auto parsed = mikufy::smart_process::parse_executable_command(command);
			if (parsed.is_valid)
				needs_independent_window = true;
		}

		/* 2. 检查是否是 python/python3/node 等解释器运行的脚本 */
		if (!needs_independent_window) {
			static std::vector<std::string> interpreters = {
				"python3 ", "python ", "python3\t", "python\t",
				"node ", "node\t", "ruby ", "ruby\t", "perl ", "perl\t"
			};
			for (const auto &prefix : interpreters) {
				if (command.compare(0, prefix.length(), prefix) == 0) {
					needs_independent_window = true;
					break;
				}
			}
		}

		pid_t pid = -1;
		std::string output;
		std::string new_path = terminal_path;

		if (needs_independent_window) {
			/* 使用 terminal_helper 在独立窗口中运行 */
			auto smart_result = mikufy::smart_process::launch_with_detection(
				command, terminal_path);

			if (!smart_result.has_value()) {
				json result;
				result["success"] = false;
				result["error"] = smart_result.error();
				result["path"] = terminal_path;
				result["isRoot"] = (getuid() == 0);
				response.body = result.dump();
				return response;
			}

			pid = smart_result.value();
			output = "Process started in independent terminal window.";
		} else {
			/* 使用终端管理器执行普通命令 */
			if (!terminal_manager) {
				json result;
				result["success"] = false;
				result["error"] = "Terminal manager not initialized";
				result["path"] = terminal_path;
				result["isRoot"] = (getuid() == 0);
				response.body = result.dump();
				return response;
			}

			auto pid_result = terminal_manager->execute_command(
				command, terminal_path);

			if (!pid_result.has_value()) {
				json result;
				result["success"] = false;
				result["error"] = pid_result.error();
				result["path"] = terminal_path;
				result["isRoot"] = (getuid() == 0);
				response.body = result.dump();
				return response;
			}

			pid = pid_result.value();

			/* 处理cd命令的特殊情况 - 在执行后立即处理路径变化 */
			if (command.substr(0, 2) == "cd") {
				/* 提取目标路径 */
				std::string target_path = command.substr(2);
				size_t start = target_path.find_first_not_of(" \t");
				if (start != std::string::npos) {
					size_t end = target_path.find_last_not_of(" \t");
					target_path = target_path.substr(start, end - start + 1);

					/* 使用expand_path函数展开路径 */
					std::string expanded = expand_path(target_path, terminal_path);
					if (!expanded.empty()) {
						/* 验证路径是否存在且可访问 */
						if (access(expanded.c_str(), F_OK) == 0) {
							new_path = expanded;
						}
					}
				}
			}

			/* 读取进程输出（优化版：智能等待而非固定轮询） */
			const int max_attempts = 3;  /* 减少到3次尝试 */
			const int delay_ms = 50;     /* 减少等待时间到50ms */

			for (int attempt = 0; attempt < max_attempts; ++attempt) {
				auto output_result = terminal_manager->get_output(pid);
				if (output_result.has_value()) {
					output += output_result->stdout_data;
				}

				/* 检查进程是否已结束 */
				auto info_result = terminal_manager->get_process_info(pid);
				if (info_result.has_value() && !info_result->is_running) {
					/* 进程已结束，再读取一次确保获取剩余输出 */
					output_result = terminal_manager->get_output(pid);
					if (output_result.has_value())
						output += output_result->stdout_data;
					break;
				}

				/* 如果还没有输出或进程还在运行，短暂等待后继续 */
				if (attempt < max_attempts - 1)
					usleep(delay_ms * 1000);
			}
		}

		json result;
		result["success"] = true;
		result["output"] = output;
		result["path"] = terminal_path;
		result["newPath"] = new_path;
		result["isRoot"] = (getuid() == 0);
		result["pid"] = pid;
		result["interactive"] = needs_independent_window;
		result["smart_launch"] = needs_independent_window;
		result["should_refresh"] = should_refresh_after_command(command);

		response.body = result.dump();
		return response;

	} catch (const json::exception &e) {
		json result;
		result["success"] = false;
		result["error"] = "Invalid JSON request";
		result["output"] = "";
		response.body = result.dump();
		return response;
	} catch (const std::exception &e) {
		json result;
		result["success"] = false;
		result["error"] = e.what();
		result["output"] = "";
		response.body = result.dump();
		return response;
	}
}
/*
 * ============================================================================
 * 高性能编辑器 API 实现（基于 TextBuffer 虚拟化渲染）
 * ============================================================================
 */

/*
 * ============================================================================
 * 高性能编辑器 API 实现（基于 TextBuffer 虚拟化渲染）
 * ============================================================================
 */

/**
 * WebServer::handle_open_file_virtual - 使用虚拟化方式打开文件
 *
 * 使用 TextBuffer (Piece Table) 架构打开文件，支持大文件的高效编辑。
 */
HttpResponse WebServer::handle_open_file_virtual(
	const std::string &path, const std::map<std::string, std::string> &headers,
	const std::string &body)
{
	(void)path;
	(void)headers;

	HttpResponse response;
	response.status_code = 200;
	response.status_text = "OK";
	response.headers["Content-Type"] = "application/json";

	try {
		json request = json::parse(body);
		std::string file_path = request["path"];

		if (file_path.empty()) {
			json result;
			result["success"] = false;
			result["error"] = "Path parameter is required";
			response.body = result.dump();
			return response;
		}

		/*
		 * 检查是否已经打开
		 */
		{
			std::lock_guard<std::mutex> lock(text_buffers_mutex);
			auto it = text_buffers.find(file_path);
			if (it != text_buffers.end()) {
				/*
							 * 文件已打开，直接返回信息
							 */
							json result;
							result["success"] = true;
							result["totalLines"] = it->second->get_line_count();
							result["totalChars"] = it->second->get_char_count();
							result["language"] = "plaintext";
							response.body = result.dump();
							return response;
						}		}

		/*
		 * 创建新的 TextBuffer
		 */
		TextBuffer *buffer = new TextBuffer();

		/*
		 * 加载文件
		 */
		if (!buffer->load_file(file_path)) {
			delete buffer;
			json result;
			result["success"] = false;
			result["error"] = "Failed to load file";
			response.body = result.dump();
			return response;
		}

		/*
		 * 保存到 map
		 */
		{
			std::lock_guard<std::mutex> lock(text_buffers_mutex);
			text_buffers[file_path] = buffer;
		}

		json result;
		result["success"] = true;
		result["totalLines"] = buffer->get_line_count();
		result["totalChars"] = buffer->get_char_count();
		result["language"] = "plaintext";

		response.body = result.dump();

		std::cout << "虚拟打开文件成功: " << file_path << ", 行数: "
			  << buffer->get_line_count() << std::endl;

	} catch (const std::exception &e) {
		json result;
		result["success"] = false;
		result["error"] = e.what();
		response.body = result.dump();
	}

	return response;
}

/**
 * WebServer::handle_get_lines - 获取指定行范围的内容
 *
 * 从 TextBuffer 中获取指定行范围的文本内容，用于虚拟滚动渲染。
 */
HttpResponse WebServer::handle_get_lines(
	const std::string &path, const std::map<std::string, std::string> &headers,
	const std::string &body)
{
	(void)path;
	(void)headers;

	HttpResponse response;
	response.status_code = 200;
	response.status_text = "OK";
	response.headers["Content-Type"] = "application/json";

	try {
		json request = json::parse(body);
		std::string file_path = request["path"];
		size_t start_line = request["start_line"];
		size_t end_line = request["end_line"];

		if (file_path.empty()) {
			json result;
			result["success"] = false;
			result["error"] = "Path parameter is required";
			response.body = result.dump();
			return response;
		}

		/*
		 * 获取 TextBuffer
		 */
		TextBuffer *buffer = nullptr;
		{
			std::lock_guard<std::mutex> lock(text_buffers_mutex);
			auto it = text_buffers.find(file_path);
			if (it == text_buffers.end()) {
				json result;
				result["success"] = false;
				result["error"] = "File not opened";
				response.body = result.dump();
				return response;
			}
			buffer = it->second;
		}

		/*
		 * 获取行内容
		 */
		std::vector<std::string> lines;
		if (!buffer->get_lines(start_line, end_line, lines)) {
			json result;
			result["success"] = false;
			result["error"] = "Failed to get lines";
			response.body = result.dump();
			return response;
	}

	/*
		 * 返回结果
		 */
		json result;
		result["success"] = true;
		result["startLine"] = start_line;
		result["endLine"] = end_line;
		result["lines"] = json::array();
		result["language"] = "plaintext";

		for (const auto &line : lines) {
			result["lines"].push_back(line);
		}

		response.body = result.dump();

	} catch (const std::exception &e) {
		json result;
		result["success"] = false;
		result["error"] = e.what();
		response.body = result.dump();
	}

	return response;
}

/**
 * WebServer::handle_get_line_count - 获取总行数
 *
 * 获取当前打开文件的总行数。
 */
HttpResponse WebServer::handle_get_line_count(
	const std::string &path, const std::map<std::string, std::string> &headers,
	const std::string &body)
{
	(void)path;
	(void)headers;

	HttpResponse response;
	response.status_code = 200;
	response.status_text = "OK";
	response.headers["Content-Type"] = "application/json";

	try {
		json request = json::parse(body);
		std::string file_path = request["path"];

		if (file_path.empty()) {
			json result;
			result["success"] = false;
			result["error"] = "Path parameter is required";
			response.body = result.dump();
			return response;
		}

		/*
		 * 获取 TextBuffer
		 */
		TextBuffer *buffer = nullptr;
		{
			std::lock_guard<std::mutex> lock(text_buffers_mutex);
			auto it = text_buffers.find(file_path);
			if (it == text_buffers.end()) {
				json result;
				result["success"] = false;
				result["error"] = "File not opened";
				response.body = result.dump();
				return response;
			}
			buffer = it->second;
		}

		/*
		 * 获取行数
		 */
		size_t line_count = buffer->get_line_count();

		json result;
		result["success"] = true;
		result["totalLines"] = line_count;

		response.body = result.dump();

	} catch (const std::exception &e) {
		json result;
		result["success"] = false;
		result["error"] = e.what();
		response.body = result.dump();
	}

	return response;
}

/**
 * WebServer::handle_edit_insert - 插入文本
 *
 * 在指定位置插入文本到 TextBuffer 中。
 */
HttpResponse WebServer::handle_edit_insert(
	const std::string &path, const std::map<std::string, std::string> &headers,
	const std::string &body)
{
	(void)path;
	(void)headers;

	HttpResponse response;
	response.status_code = 200;
	response.status_text = "OK";
	response.headers["Content-Type"] = "application/json";

	try {
		json request = json::parse(body);
		std::string file_path = request["path"];
		size_t position = request["position"];
		std::string text = request["text"];

		if (file_path.empty()) {
			json result;
			result["success"] = false;
			result["error"] = "Path parameter is required";
			response.body = result.dump();
			return response;
		}

		/*
		 * 获取 TextBuffer
		 */
		TextBuffer *buffer = nullptr;
		{
			std::lock_guard<std::mutex> lock(text_buffers_mutex);
			auto it = text_buffers.find(file_path);
			if (it == text_buffers.end()) {
				json result;
				result["success"] = false;
				result["error"] = "File not opened";
				response.body = result.dump();
				return response;
			}
			buffer = it->second;
		}

		/*
		 * 插入文本
		 */
		if (!buffer->insert(position, text)) {
			json result;
			result["success"] = false;
			result["error"] = "Failed to insert text";
			response.body = result.dump();
			return response;
		}

		/*
		 * 返回结果
		 */
		json result;
		result["success"] = true;
		result["newTotalLines"] = buffer->get_line_count();

		response.body = result.dump();

	} catch (const std::exception &e) {
		json result;
		result["success"] = false;
		result["error"] = e.what();
		response.body = result.dump();
	}

	return response;
}

/**
 * WebServer::handle_edit_delete - 删除文本范围
 *
 * 删除指定范围的文本。
 */
HttpResponse WebServer::handle_edit_delete(
	const std::string &path, const std::map<std::string, std::string> &headers,
	const std::string &body)
{
	(void)path;
	(void)headers;

	HttpResponse response;
	response.status_code = 200;
	response.status_text = "OK";
	response.headers["Content-Type"] = "application/json";

	try {
		json request = json::parse(body);
		std::string file_path = request["path"];
		size_t start_position = request["start_position"];
		size_t end_position = request["end_position"];

		if (file_path.empty()) {
			json result;
			result["success"] = false;
			result["error"] = "Path parameter is required";
			response.body = result.dump();
			return response;
		}

		/*
		 * 获取 TextBuffer
		 */
		TextBuffer *buffer = nullptr;
		{
			std::lock_guard<std::mutex> lock(text_buffers_mutex);
			auto it = text_buffers.find(file_path);
			if (it == text_buffers.end()) {
				json result;
				result["success"] = false;
				result["error"] = "File not opened";
				response.body = result.dump();
				return response;
			}
			buffer = it->second;
		}

		/*
		 * 删除文本
		 */
		if (!buffer->delete_range(start_position, end_position)) {
			json result;
			result["success"] = false;
			result["error"] = "Failed to delete text";
			response.body = result.dump();
			return response;
		}

		/*
		 * 返回结果
		 */
		json result;
		result["success"] = true;
		result["newTotalLines"] = buffer->get_line_count();

		response.body = result.dump();

	} catch (const std::exception &e) {
		json result;
		result["success"] = false;
		result["error"] = e.what();
		response.body = result.dump();
	}

	return response;
}

/**
 * WebServer::handle_edit_replace - 替换文本范围
 *
 * 将指定范围的文本替换为新文本。
 */
HttpResponse WebServer::handle_edit_replace(
	const std::string &path, const std::map<std::string, std::string> &headers,
	const std::string &body)
{
	(void)path;
	(void)headers;

	HttpResponse response;
	response.status_code = 200;
	response.status_text = "OK";
	response.headers["Content-Type"] = "application/json";

	try {
		json request = json::parse(body);
		std::string file_path = request["path"];
		size_t start_position = request["start_position"];
		size_t end_position = request["end_position"];
		std::string text = request["text"];

		if (file_path.empty()) {
			json result;
			result["success"] = false;
			result["error"] = "Path parameter is required";
			response.body = result.dump();
			return response;
		}

		/*
		 * 获取 TextBuffer
		 */
		TextBuffer *buffer = nullptr;
		{
			std::lock_guard<std::mutex> lock(text_buffers_mutex);
			auto it = text_buffers.find(file_path);
			if (it == text_buffers.end()) {
				json result;
				result["success"] = false;
				result["error"] = "File not opened";
				response.body = result.dump();
				return response;
			}
			buffer = it->second;
		}

		/*
		 * 替换文本
		 */
		if (!buffer->replace(start_position, end_position, text)) {
			json result;
			result["success"] = false;
			result["error"] = "Failed to replace text";
			response.body = result.dump();
			return response;
		}

		/*
		 * 返回结果
		 */
		json result;
		result["success"] = true;
		result["newTotalLines"] = buffer->get_line_count();

		response.body = result.dump();

	} catch (const std::exception &e) {
		json result;
		result["success"] = false;
		result["error"] = e.what();
		response.body = result.dump();
	}

	return response;
}

/**
 * WebServer::handle_close_file_virtual - 关闭虚拟文件
 *
 * 关闭并释放 TextBuffer 资源。
 */
HttpResponse WebServer::handle_close_file_virtual(
	const std::string &path, const std::map<std::string, std::string> &headers,
	const std::string &body)
{
	(void)path;
	(void)headers;

	HttpResponse response;
	response.status_code = 200;
	response.status_text = "OK";
	response.headers["Content-Type"] = "application/json";

	try {
		json request = json::parse(body);
		std::string file_path = request["path"];

		if (file_path.empty()) {
			json result;
			result["success"] = false;
			result["error"] = "Path parameter is required";
			response.body = result.dump();
			return response;
		}

		/*
		 * 查找并删除 TextBuffer
		 */
		bool found = false;
		{
			std::lock_guard<std::mutex> lock(text_buffers_mutex);
			auto it = text_buffers.find(file_path);
			if (it != text_buffers.end()) {
				delete it->second;
				text_buffers.erase(it);
				found = true;
			}
		}

		json result;
		result["success"] = found;

		if (found) {
			std::cout << "关闭虚拟文件: " << file_path << std::endl;
		}

		response.body = result.dump();

	} catch (const std::exception &e) {
		json result;
		result["success"] = false;
		result["error"] = e.what();
		response.body = result.dump();
	}

	return response;
}

/**
 * WebServer::handle_terminal_get_output - 获取交互式进程的输出
 */
HttpResponse WebServer::handle_terminal_get_output(
	const std::string &path, const std::map<std::string, std::string> &headers,
	const std::string &body)
{
	(void)path;
	(void)headers;

	HttpResponse response;
	response.status_code = 200;
	response.status_text = "OK";
	response.headers["Content-Type"] = "application/json";

	try {
		json request = json::parse(body);
		pid_t pid = request["pid"];

		if (!terminal_manager) {
			json result;
			result["success"] = false;
			result["error"] = "Terminal manager not initialized";
			result["is_running"] = false;
			response.body = result.dump();
			return response;
		}

		auto output_result = terminal_manager->get_output(pid);

		if (!output_result.has_value()) {
			json result;
			result["success"] = false;
			result["error"] = output_result.error();
			result["is_running"] = false;
			result["pid"] = pid;
			response.body = result.dump();
			return response;
		}

		TerminalOutput output = output_result.value();

		auto info_result = terminal_manager->get_process_info(pid);
		bool is_running = false;

		if (info_result.has_value())
			is_running = info_result.value().is_running;

		json result;
		result["success"] = true;
		result["output"] = output.stdout_data;
		result["error"] = output.stderr_data;
		result["is_running"] = is_running;
		result["pid"] = pid;

		response.body = result.dump();

	} catch (const std::exception &e) {
		json result;
		result["success"] = false;
		result["error"] = e.what();
		response.body = result.dump();
	}

	return response;
}

/**
 * WebServer::handle_terminal_send_input - 向交互式进程发送输入
 */
HttpResponse WebServer::handle_terminal_send_input(
	const std::string &path, const std::map<std::string, std::string> &headers,
	const std::string &body)
{
	(void)path;
	(void)headers;

	HttpResponse response;
	response.status_code = 200;
	response.status_text = "OK";
	response.headers["Content-Type"] = "application/json";

	try {
		json request = json::parse(body);
		pid_t pid = request["pid"];
		std::string input = request["input"];

		if (!terminal_manager) {
			json result;
			result["success"] = false;
			result["error"] = "Terminal manager not initialized";
			response.body = result.dump();
			return response;
		}

		auto result_obj = terminal_manager->send_input(pid, input);

		json result;
		result["success"] = result_obj.has_value();
		result["pid"] = pid;

		if (!result_obj.has_value())
			result["error"] = result_obj.error();

		response.body = result.dump();

	} catch (const std::exception &e) {
		json result;
		result["success"] = false;
		result["error"] = e.what();
		response.body = result.dump();
	}

	return response;
}

/**
 * WebServer::handle_terminal_kill_process - 终止交互式进程
 */
HttpResponse WebServer::handle_terminal_kill_process(
	const std::string &path, const std::map<std::string, std::string> &headers,
	const std::string &body)
{
	(void)path;
	(void)headers;

	HttpResponse response;
	response.status_code = 200;
	response.status_text = "OK";
	response.headers["Content-Type"] = "application/json";

	try {
		json request = json::parse(body);
		pid_t pid = request["pid"];

		if (!terminal_manager) {
			json result;
			result["success"] = false;
			result["error"] = "Terminal manager not initialized";
			response.body = result.dump();
			return response;
		}

		auto result_obj = terminal_manager->terminate_process(pid);

		json result;
		result["success"] = result_obj.has_value();
		result["pid"] = pid;
		result["message"] = "Process terminated";

		if (!result_obj.has_value())
			result["error"] = result_obj.error();

		response.body = result.dump();
	} catch (const std::exception &e) {
		json result;
		result["success"] = false;
		result["error"] = e.what();
		response.body = result.dump();
	}

	return response;
}

/**
 * WebServer::should_refresh_after_command - 判断命令执行后是否需要刷新文件树
 * @command: 要检测的命令字符串
 *
 * 检测命令是否是文件操作或编译命令，如果是则返回true。
 * 这用于自动刷新文件树功能。
 *
 * 返回值: 需要刷新返回true，否则返回false
 */
bool WebServer::should_refresh_after_command(const std::string &command)
{
	/* 需要刷新的命令前缀列表 */
	static const std::vector<std::string> refresh_commands = {
		"mkdir ", "rm ", "mv ", "cp ", "touch ", "ln ",
		"gcc ", "g++ ", "clang ", "clang++ ", "rustc ", "cc ",
		"make ", "cmake ", "ninja ", "ninja-build ",
		"cargo ", "cargo build", "cargo run", "cargo install",
		"npm ", "npm run", "npm install", "npm build",
		"pip ", "pip install", "pip3 ", "pip3 install",
		"yarn ", "yarn install", "yarn build",
		"pnpm ", "pnpm install", "pnpm build",
		"go build", "go run", "go install",
		"javac ", "java -jar",
		"gradle ", "gradle build", "gradlew ",
		"mvn ", "mvn compile", "mvn package", "mvn install",
		"meson ", "meson build",
		"bazel ", "bazel build",
		"npx ",
		"bun ", "bun install", "bun run",
		"poetry ", "poetry install", "poetry build",
		"composer ", "composer install"
	};

	/* 精确匹配 */
	for (const auto &cmd : refresh_commands) {
		if (command == cmd)
			return true;
		/* 前缀匹配 */
		if (command.compare(0, cmd.length(), cmd) == 0)
			return true;
	}

	/* 特殊关键字检测 */
	if (command.find(" build") != std::string::npos ||
	    command.find(" install") != std::string::npos ||
	    command.find(" compile") != std::string::npos) {
		return true;
	}

	return false;
}

/**
 * WebServer::expand_path - 展开路径中的特殊符号
 * @path: 要展开的路径
 * @base_dir: 基准目录（用于解析相对路径）
 *
 * 处理路径中的~（家目录）、.（当前目录）、..（上级目录）等特殊符号。
 *
 * 返回值: 展开后的绝对路径，失败返回空字符串
 */
std::string WebServer::expand_path(const std::string &path, const std::string &base_dir)
{
	std::string result = path;

	/* 处理~符号（家目录） */
	if (result == "~") {
		char *home = getenv("HOME");
		if (home)
			result = std::string(home);
	} else if (!result.empty() && result[0] == '~') {
		char *home = getenv("HOME");
		if (home)
			result = std::string(home) + result.substr(1);
	}

	/* 处理空路径 */
	if (result.empty())
		return "";

	/* 处理绝对路径 */
	if (result[0] == '/') {
		char *real_path = realpath(result.c_str(), nullptr);
		if (real_path) {
			std::string expanded = std::string(real_path);
			free(real_path);
			return expanded;
		}
		return "";
	}

	/* 处理相对路径 */
	std::string full_path = base_dir;
	if (!full_path.empty() && full_path.back() != '/')
		full_path += "/";
	full_path += result;

	/* 规范化路径 */
	char *real_path = realpath(full_path.c_str(), nullptr);
	if (real_path) {
		std::string expanded = std::string(real_path);
		free(real_path);
		return expanded;
	}

	return "";
}
