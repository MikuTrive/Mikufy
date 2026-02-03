/* Mikufy v2.2(stable) - Web服务器实现
 * 负责HTTP服务器和前端通信
 */

#include "../headers/web_server.h"
#include <iostream>
#include <cstring>
#include <sstream>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

/* 构造函数 */
WebServer::WebServer(FileManager *file_manager)
	: file_manager(file_manager), server_socket(-1),
	  port(WEB_SERVER_PORT), running(false)
{
	register_routes();
}

/* 析构函数 */
WebServer::~WebServer(void)
{
	stop();
}

/* 设置打开文件夹对话框的回调函数 */
void WebServer::set_open_folder_callback(std::function<std::string(void)> callback)
{
	std::lock_guard<std::mutex> lock(mutex);
	open_folder_callback = callback;
}

/* 启动Web服务器 */
bool WebServer::start(int port_num)
{
	std::lock_guard<std::mutex> lock(mutex);

	if (running)
		return false;

	port = port_num;

	/* 创建socket */
	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket < 0)
		return false;

	/* 设置socket选项 */
	int opt = 1;
	if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt,
		       sizeof(opt)) < 0) {
		close(server_socket);
		server_socket = -1;
		return false;
	}

	/* 绑定地址 */
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

	/* 开始监听 */
	if (listen(server_socket, 10) < 0) {
		close(server_socket);
		server_socket = -1;
		return false;
	}

	/* 设置为非阻塞模式 */
	int flags = fcntl(server_socket, F_GETFL, 0);
	fcntl(server_socket, F_SETFL, flags | O_NONBLOCK);

	running = true;

	/* 启动服务器线程 */
	server_thread = std::thread(&WebServer::server_loop, this);

	return true;
}

/* 停止Web服务器 */
void WebServer::stop(void)
{
	std::lock_guard<std::mutex> lock(mutex);

	if (!running)
		return;

	running = false;

	/* 关闭服务器socket */
	if (server_socket >= 0) {
		close(server_socket);
		server_socket = -1;
	}

	/* 等待服务器线程结束 */
	if (server_thread.joinable())
		server_thread.join();
}

/* 检查服务器是否正在运行 */
bool WebServer::is_running(void) const
{
	return running;
}

/* 服务器主循环 */
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
		fds[0].events = POLLIN;
		fds[0].revents = 0;

		int ready = poll(fds, 1, 100); /* 100ms超时 */

		if (ready < 0) {
			if (errno == EINTR)
				continue;
			std::cerr << "poll调用失败: " << strerror(errno)
				  << std::endl;
			break;
		}

		if (ready > 0 && (fds[0].revents & POLLIN)) {
			/* 接受客户端连接 */
			int client_socket = accept_client();
			if (client_socket >= 0)
				handle_client(client_socket);
		}
	}
}

/* 接受客户端连接 */
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

/* 处理客户端请求 */
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
			break;

		buffer[bytes_read] = '\0';
		request.append(buffer, bytes_read);

		/* 检查是否已经收到完整的请求头 */
		size_t header_end = request.find("\r\n\r\n");
		if (header_end != std::string::npos) {
			/* 查找Content-Length */
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

	/* 查找路由 */
	auto it = routes.find(route_path);
	if (it != routes.end()) {
		std::cout << "路由匹配成功: " << route_path << ", body长度: "
			  << body.length() << std::endl;
		response = it->second(path, headers, body);
	} else {
		/* 处理静态文件 */
		response = handle_static_file(path);
	}

	/* 发送响应 */
	send_http_response(client_socket, response);

	/* 关闭连接 */
	close(client_socket);
}

/* 解析HTTP请求 */
bool WebServer::parse_http_request(
	const std::string &request, std::string &method, std::string &path,
	std::map<std::string, std::string> &headers, std::string &body)
{
	std::istringstream stream(request);
	std::string line;

	/* 解析请求行 */
	if (!std::getline(stream, line))
		return false;

	std::istringstream request_line(line);
	request_line >> method >> path;

	if (method.empty() || path.empty())
		return false;

	/* 解析请求头 */
	while (std::getline(stream, line) && line != "\r") {
		size_t pos = line.find(':');
		if (pos != std::string::npos) {
			std::string key = line.substr(0, pos);
			std::string value = line.substr(pos + 1);

			/* 去除首尾空白 */
			while (!key.empty() &&
			       (key.back() == ' ' || key.back() == '\r'))
				key.pop_back();
			while (!value.empty() && (value.front() == ' '))
				value.erase(value.begin());
			while (!value.empty() &&
			       (value.back() == ' ' || value.back() == '\r'))
				value.pop_back();

			headers[key] = value;
		}
	}

	/* 解析请求体 */
	if (method == "POST" || method == "PUT") {
		auto it = headers.find("Content-Length");
		if (it != headers.end()) {
			size_t content_length = std::stoul(it->second);
			std::cout << "POST请求，Content-Length: "
				  << content_length << std::endl;

			/* 从原始request字符串中查找body位置 */
			/* HTTP请求格式: headers\r\n\r\nbody */
			size_t body_start = request.find("\r\n\r\n");
			if (body_start != std::string::npos) {
				body_start += 4; /* 跳过\r\n\r\n */
				if (body_start + content_length <=
				    request.length()) {
					body = request.substr(
						body_start, content_length);
					std::cout
						<< "从request中提取body成功: "
						<< body << std::endl;
				} else {
					std::cout << "body超出request长度"
						  << std::endl;
				}
			} else {
				std::cout
					<< "找不到body分隔符\\r\\n\\r\\n"
					<< std::endl;
			}
		} else {
			std::cout << "POST请求但没有Content-Length头部"
				  << std::endl;
		}
	}

	return true;
}

/* 构造HTTP响应 */
std::string WebServer::build_http_response(const HttpResponse &response)
{
	std::ostringstream oss;

	/* 状态行 */
	oss << "HTTP/1.1 " << response.status_code << " "
	    << response.status_text << "\r\n";

	/* 响应头 */
	for (const auto &header : response.headers)
		oss << header.first << ": " << header.second << "\r\n";

	/* 空行 */
	oss << "\r\n";

	/* 响应体 */
	oss << response.body;

	return oss.str();
}

/* 发送HTTP响应 */
bool WebServer::send_http_response(int client_socket,
			   const HttpResponse &response)
{
	std::string response_str = build_http_response(response);

	ssize_t sent = send(client_socket, response_str.c_str(),
			    response_str.length(), 0);

	return (sent == static_cast<ssize_t>(response_str.length()));
}

/* URL解码 */
std::string WebServer::url_decode(const std::string &encoded)
{
	std::string decoded;
	decoded.reserve(encoded.length());

	for (size_t i = 0; i < encoded.length(); ++i) {
		if (encoded[i] == '%' && i + 2 < encoded.length()) {
			std::string hex_str = encoded.substr(i + 1, 2);
			char decoded_char = static_cast<char>(
				std::stoi(hex_str, nullptr, 16));
			decoded += decoded_char;
			i += 2;
		} else if (encoded[i] == '+') {
			decoded += ' ';
		} else {
			decoded += encoded[i];
		}
	}

	return decoded;
}

/* URL编码 */
std::string WebServer::url_encode(const std::string &decoded)
{
	std::string encoded;
	encoded.reserve(decoded.length() * 3);

	for (char c : decoded) {
		if (isalnum(static_cast<unsigned char>(c)) || c == '-' ||
		    c == '_' || c == '.' || c == '~') {
			encoded += c;
		} else if (c == ' ') {
			encoded += '+';
		} else {
			char hex[4];
			snprintf(hex, sizeof(hex), "%%%02X",
				 static_cast<unsigned char>(c));
			encoded += hex;
		}
	}

	return encoded;
}

/* 解析查询参数 */
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

/* 注册HTTP路由 */
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
}

/* 处理打开文件夹对话框API */
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

/* 处理获取目录内容API */
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
	std::cout << "收到获取目录内容请求: " << path << std::endl;
	std::cout << "解析出的目录路径: " << directory_path << std::endl;

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

	std::cout << "获取目录内容结果: " << (success ? "成功" : "失败")
		  << ", 文件数量: " << files.size() << std::endl;

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

	std::cout << "返回的JSON: " << response.body << std::endl;

	return response;
}

/* 处理读取文件API */
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

/* 处理保存文件API */
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

			if (!file_manager->write_file(file_path, content))
				all_success = false;
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

/* 处理静态文件请求 */
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
	std::string full_path = "web" + file_path;

	std::cout << "静态文件请求: " << path << " -> " << full_path
		  << std::endl;

	/* 读取文件 */
	std::string content = read_static_file(full_path);

	if (content.empty()) {
		/* 文件不存在 */
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

/* 读取静态文件内容 */
std::string WebServer::read_static_file(const std::string &file_path)
{
	std::ifstream file(file_path, std::ios::binary);

	if (!file.is_open())
		return "";

	std::stringstream buffer;
	buffer << file.rdbuf();

	file.close();

	return buffer.str();
}

/* 获取文件的MIME类型（用于HTTP响应） */
std::string WebServer::get_http_mime_type(const std::string &file_path)
{
	std::string ext =
		file_path.substr(file_path.find_last_of('.') + 1);

	static const std::map<std::string, std::string> mime_types = {
		{ "html", "text/html" },
		{ "css", "text/css" },
		{ "js", "application/javascript" },
		{ "json", "application/json" },
		{ "png", "image/png" },
		{ "jpg", "image/jpeg" },
		{ "jpeg", "image/jpeg" },
		{ "gif", "image/gif" },
		{ "svg", "image/svg+xml" },
		{ "ico", "image/x-icon" },
		{ "woff", "font/woff" },
		{ "woff2", "font/woff2" },
		{ "ttf", "font/ttf" },
		{ "eot", "application/vnd.ms-fontobject" },
		{ "mp4", "video/mp4" },
		{ "mp3", "audio/mpeg" },
		{ "wav", "audio/wav" }
	};

	auto it = mime_types.find(ext);
	if (it != mime_types.end())
		return it->second;

	return "application/octet-stream";
}
