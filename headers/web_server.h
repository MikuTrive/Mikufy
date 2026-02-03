/* Mikufy v2.2(stable) - Web服务器头文件
 * 负责HTTP服务器和WebSocket通信
 */

#ifndef MIKUFY_WEB_SERVER_H
#define MIKUFY_WEB_SERVER_H

#include "main.h"
#include "file_manager.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* Web服务器类
 * 提供HTTP服务，处理前端请求
 */
class WebServer
{
public:
	WebServer(FileManager *file_manager);
	~WebServer();

	/* 禁止拷贝和移动 */
	WebServer(const WebServer &) = delete;
	WebServer &operator=(const WebServer &) = delete;
	WebServer(WebServer &&) = delete;
	WebServer &operator=(WebServer &&) = delete;

	/* 启动Web服务器 */
	bool start(int port);

	/* 停止Web服务器 */
	void stop(void);

	/* 检查服务器是否正在运行 */
	bool is_running(void) const;

	/* 设置打开文件夹对话框的回调函数 */
	void set_open_folder_callback(std::function<std::string(void)> callback);

private:
	FileManager *file_manager;
	int server_socket;
	int port;
	std::atomic<bool> running;
	std::thread server_thread;
	std::mutex mutex;

	/* 打开文件夹对话框回调 */
	std::function<std::string(void)> open_folder_callback;

	/* HTTP路由表 */
	std::map<std::string, HttpHandler> routes;

	/* 服务器主循环 */
	void server_loop(void);

	/* 接受客户端连接 */
	int accept_client(void);

	/* 处理客户端请求 */
	void handle_client(int client_socket);

	/* 解析HTTP请求 */
	bool parse_http_request(const std::string &request,
				std::string &method, std::string &path,
				std::map<std::string, std::string> &headers,
				std::string &body);

	/* 构造HTTP响应 */
	std::string build_http_response(const HttpResponse &response);

	/* 发送HTTP响应 */
	bool send_http_response(int client_socket,
				const HttpResponse &response);

	/* URL解码 */
	std::string url_decode(const std::string &encoded);

	/* URL编码 */
	std::string url_encode(const std::string &decoded);

	/* 解析查询参数 */
	std::map<std::string, std::string> parse_query_string(
				const std::string &query_string);

	/* 注册HTTP路由 */
	void register_routes(void);

	/* === API处理函数 === */

	/* 处理打开文件夹对话框API */
	HttpResponse handle_open_folder_dialog(
			const std::string &path,
			const std::map<std::string, std::string> &headers,
			const std::string &body);

	/* 处理获取目录内容API */
	HttpResponse handle_get_directory_contents(
			const std::string &path,
			const std::map<std::string, std::string> &headers,
			const std::string &body);

	/* 处理读取文件API */
	HttpResponse handle_read_file(
			const std::string &path,
			const std::map<std::string, std::string> &headers,
			const std::string &body);

	/* 处理读取二进制文件API（用于图片、视频等） */
	HttpResponse handle_read_binary_file(
			const std::string &path,
			const std::map<std::string, std::string> &headers,
			const std::string &body);

	/* 处理保存文件API */
	HttpResponse handle_save_file(
			const std::string &path,
			const std::map<std::string, std::string> &headers,
			const std::string &body);

	/* 处理创建文件夹API */
	HttpResponse handle_create_folder(
			const std::string &path,
			const std::map<std::string, std::string> &headers,
			const std::string &body);

	/* 处理创建文件API */
	HttpResponse handle_create_file(
			const std::string &path,
			const std::map<std::string, std::string> &headers,
			const std::string &body);

	/* 处理删除API */
	HttpResponse handle_delete(
			const std::string &path,
			const std::map<std::string, std::string> &headers,
			const std::string &body);

	/* 处理重命名API */
	HttpResponse handle_rename(
			const std::string &path,
			const std::map<std::string, std::string> &headers,
			const std::string &body);

	/* 处理获取文件信息API */
	HttpResponse handle_get_file_info(
			const std::string &path,
			const std::map<std::string, std::string> &headers,
			const std::string &body);

	/* 处理保存所有文件API */
	HttpResponse handle_save_all(
			const std::string &path,
			const std::map<std::string, std::string> &headers,
			const std::string &body);

	/* 处理刷新API */
	HttpResponse handle_refresh(
			const std::string &path,
			const std::map<std::string, std::string> &headers,
			const std::string &body);

	/* 处理静态文件请求 */
	HttpResponse handle_static_file(const std::string &path);

	/* 读取静态文件内容 */
	std::string read_static_file(const std::string &file_path);

	/* 获取文件的MIME类型（用于HTTP响应） */
	std::string get_http_mime_type(const std::string &file_path);
};

#endif /* MIKUFY_WEB_SERVER_H */
