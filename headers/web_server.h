/*
 * Mikufy v2.7-nova - Web服务器头文件
 *
 * 本文件定义了WebServer类的接口，该类负责提供HTTP服务和
 * 处理前端请求。WebServer实现了完整的HTTP/1.1服务器功能，
 * 支持静态文件服务和RESTful API接口，通过HTTP协议与前端
 * JavaScript进行通信。
 *
 * 主要功能:
 * - HTTP服务器启动和监听
 * - 请求解析和路由分发
 * - API端点实现（文件操作、目录浏览等）
 * - 静态文件服务（HTML、CSS、JavaScript、图片等）
 * - URL编码/解码
 * - 线程安全的请求处理
 *
 * API端点列表:
 * - GET  /api/open-folder              打开文件夹对话框
 * - GET  /api/directory-contents      获取目录内容
 * - GET  /api/read-file               读取文本文件
 * - GET  /api/read-binary-file        读取二进制文件
 * - POST /api/save-file               保存文件
 * - POST /api/create-folder           创建文件夹
 * - POST /api/create-file             创建文件
 * - POST /api/delete                  删除文件或目录
 * - POST /api/rename                  重命名文件或目录
 * - GET  /api/file-info               获取文件信息
 * - POST /api/save-all                保存所有文件
 * - POST /api/refresh                 刷新文件列表
 * - POST /api/change-wallpaper        更换壁纸
 *
 * 2024 MiraTrive/MikuTrive
 * Author: [Your Name]
 *
 * 本文件遵循Linux内核代码风格规范
 */

#ifndef MIKUFY_WEB_SERVER_H
#define MIKUFY_WEB_SERVER_H

/*
 * ============================================================================
 * 头文件包含
 * ============================================================================
 */

#include "main.h"		/* 全局定义和数据结构 */
#include "file_manager.h"	/* FileManager文件管理器类 */
#include "text_buffer.h"		/* 文本缓冲区类 */

/* 网络编程头文件 */
#include <sys/socket.h>		/* socket(), bind(), listen() */
#include <netinet/in.h>		/* struct sockaddr_in */
#include <arpa/inet.h>		/* inet_addr() */

/*
 * ============================================================================
 * WebServer类定义
 * ============================================================================
 */

/**
 * WebServer - Web服务器类
 *
 * 该类实现了完整的HTTP服务器功能，负责处理前端发送的HTTP请求，
 * 并将请求路由到相应的处理函数。服务器使用非阻塞I/O和多线程模型，
 * 可以高效地处理并发请求。
 *
 * 设计特点:
 * - 非阻塞I/O：使用poll()系统调用实现事件驱动
 * - 多线程：服务器主循环在独立线程中运行
 * - 路由表：使用std::map存储URL到处理器的映射
 * - 线程安全：使用互斥锁保护共享资源
 * - 回调机制：支持打开文件夹对话框的回调
 *
 * 使用示例:
 * @code
 * FileManager fm;
 * WebServer ws(&fm);
 * ws.set_open_folder_callback([]() {
 *     return "/path/to/selected/folder";
 * });
 * if (ws.start(8080)) {
 *     // 服务器运行中...
 *     ws.stop();
 * }
 * @endcode
 */
class WebServer
{
public:
	/**
	 * WebServer - 构造函数
	 *
	 * 初始化WebServer对象，设置文件管理器引用并注册所有路由。
	 * 服务器不会自动启动，需要调用start()方法。
	 *
	 * @file_manager: FileManager对象的指针，用于文件操作
	 */
	WebServer(FileManager *file_manager);

	/**
	 * ~WebServer - 析构函数
	 *
	 * 清理WebServer对象，停止服务器并释放资源。
	 * 调用stop()方法来关闭服务器socket和线程。
	 */
	~WebServer(void);

	/* 禁止拷贝构造函数 */
	WebServer(const WebServer &) = delete;

	/* 禁止拷贝赋值操作符 */
	WebServer &operator=(const WebServer &) = delete;

	/* 禁止移动构造函数 */
	WebServer(WebServer &&) = delete;

	/* 禁止移动赋值操作符 */
	WebServer &operator=(WebServer &&) = delete;

	/* ====================================================================
	 * 公共方法 - 服务器控制
	 * ==================================================================== */

	/**
	 * start - 启动Web服务器
	 *
	 * 在指定端口启动HTTP服务器，开始监听客户端连接。服务器将
	 * 在独立线程中运行主循环，不会阻塞调用线程。
	 *
	 * @port: 服务器监听端口号（1-65535）
	 *
	 * 返回值: 成功返回true，失败返回false
	 *
	 * 注意: 服务器已在运行时调用此方法将返回false。
	 *       端口必须未被其他程序占用。
	 */
	bool start(int port);

	/**
	 * stop - 停止Web服务器
	 *
	 * 停止HTTP服务器，关闭socket并终止服务器线程。该方法会
	 * 等待服务器线程正常退出。
	 *
	 * 注意: 服务器未运行时调用此方法是安全的。
	 */
	void stop(void);

	/**
	 * is_running - 检查服务器是否正在运行
	 *
	 * 查询服务器的当前运行状态。
	 *
	 * 返回值: 服务器正在运行返回true，否则返回false
	 */
	bool is_running(void) const;

	/* ====================================================================
	 * 公共方法 - 回调设置
	 * ==================================================================== */

	/**
	 * set_open_folder_callback - 设置打开文件夹对话框的回调函数
	 *
	 * 注册一个回调函数，当需要打开文件夹对话框时调用。回调函数
	 * 应该返回用户选择的文件夹路径。如果用户取消选择，应返回
	 * 空字符串。
	 *
	 * @callback: 回调函数，返回字符串类型的文件夹路径
	 *
	 * 注意: 回调函数将在HTTP请求处理的上下文中调用。
	 *       确保回调函数是线程安全的。
	 */
	void set_open_folder_callback(std::function<std::string(void)> callback);

	/**
	 * set_web_root_path - 设置web资源根目录的绝对路径
	 *
	 * 设置web资源文件（包括style.css、Background等）所在的目录路径。
	 * 用于确保无论程序从哪个目录启动，都能正确找到和修改web资源文件。
	 *
	 * @path: web目录的绝对路径
	 */
	void set_web_root_path(const std::string &path);

private:
	/* ====================================================================
	 * 私有成员变量
	 * ==================================================================== */

	FileManager *file_manager;	/* 文件管理器指针 */
	int server_socket;		/* 服务器socket描述符 */
	int port;			/* 服务器监听端口 */
	std::atomic<bool> running;	/* 服务器运行标志 */
	std::thread server_thread;	/* 服务器线程对象 */
	std::mutex mutex;		/* 互斥锁，保护共享资源 */

	/* 打开文件夹对话框回调函数 */
	std::function<std::string(void)> open_folder_callback;

	/* web资源根目录的绝对路径 */
	std::string web_root_path;

	/* HTTP路由表（URL路径 -> 处理函数） */
	std::map<std::string, HttpHandler> routes;

	/* 高性能编辑器相关 */
	std::unordered_map<std::string, TextBuffer *> text_buffers;	/* 文件路径 -> TextBuffer 映射 */
	std::mutex text_buffers_mutex;				/* 保护 text_buffers 的互斥锁 */

	/* ====================================================================
	 * 私有方法 - 服务器主循环
	 * ==================================================================== */

	/**
	 * server_loop - 服务器主循环
	 *
	 * 服务器的主事件循环，在独立线程中运行。使用poll()系统
	 * 调用等待客户端连接，接受连接并处理请求。当running标志
	 * 为false时退出循环。
	 *
	 * 注意: 该方法在服务器线程中运行，不应直接调用。
	 */
	void server_loop(void);

	/**
	 * accept_client - 接受客户端连接
	 *
	 * 接受一个待处理的客户端连接请求。
	 *
	 * 返回值: 成功返回客户端socket描述符，失败返回-1
	 *
	 * 注意: 该方法应在server_loop()中调用。
	 */
	int accept_client(void);

	/**
	 * handle_client - 处理客户端请求
	 *
	 * 读取并处理客户端发送的HTTP请求，构造响应并发送回客户端。
	 * 处理完成后关闭客户端连接。
	 *
	 * @client_socket: 客户端socket描述符
	 *
	 * 注意: 该方法会自动关闭客户端socket。
	 */
	void handle_client(int client_socket);

	/* ====================================================================
	 * 私有方法 - HTTP协议处理
	 * ==================================================================== */

	/**
	 * parse_http_request - 解析HTTP请求
	 *
	 * 解析原始HTTP请求字符串，提取方法、路径、请求头和请求体。
	 *
	 * @request: 原始HTTP请求字符串
	 * @method: 输出参数，存储HTTP方法（GET/POST等）
	 * @path: 输出参数，存储请求路径
	 * @headers: 输出参数，存储请求头键值对
	 * @body: 输出参数，存储请求体内容
	 *
	 * 返回值: 解析成功返回true，失败返回false
	 *
	 * 注意: 该方法支持GET和POST请求。
	 */
	bool parse_http_request(const std::string &request,
				std::string &method, std::string &path,
				std::map<std::string, std::string> &headers,
				std::string &body);

	/**
	 * build_http_response - 构造HTTP响应
	 *
	 * 根据HttpResponse结构体构造完整的HTTP响应字符串。
	 *
	 * @response: HttpResponse结构体
	 *
	 * 返回值: HTTP响应字符串
	 */
	std::string build_http_response(const HttpResponse &response);

	/**
	 * send_http_response - 发送HTTP响应
	 *
	 * 将HTTP响应发送给客户端。
	 *
	 * @client_socket: 客户端socket描述符
	 * @response: HttpResponse结构体
	 *
	 * 返回值: 发送成功返回true，失败返回false
	 */
	bool send_http_response(int client_socket,
				const HttpResponse &response);

	/* ====================================================================
	 * 私有方法 - URL处理
	 * ==================================================================== */

	/**
	 * url_decode - URL解码
	 *
	 * 将URL编码的字符串解码为普通字符串。处理%XX格式的十六进制
	 * 编码和+号表示的空格。
	 *
	 * @encoded: URL编码的字符串
	 *
	 * 返回值: 解码后的字符串
	 */
	std::string url_decode(const std::string &encoded);

	/**
	 * url_encode - URL编码
	 *
	 * 将普通字符串编码为URL格式。非字母数字字符将被编码为%XX格式，
	 * 空格被编码为+号。
	 *
	 * @decoded: 普通字符串
	 *
	 * 返回值: URL编码的字符串
	 */
	std::string url_encode(const std::string &decoded);

	/**
	 * parse_query_string - 解析查询参数
	 *
	 * 解析URL查询字符串（?后面的部分），提取参数键值对。
	 *
	 * @query_string: 查询字符串（不含?）
	 *
	 * 返回值: 参数键值对映射
	 */
	std::map<std::string, std::string> parse_query_string(
				const std::string &query_string);

	/* ====================================================================
	 * 私有方法 - 路由管理
	 * ==================================================================== */

	/**
	 * register_routes - 注册HTTP路由
	 *
	 * 将所有API端点注册到路由表中。该方法在构造函数中调用。
	 */
	void register_routes(void);

	/* ====================================================================
	 * 私有方法 - API处理函数
	 * ==================================================================== */

	/**
	 * handle_open_folder_dialog - 处理打开文件夹对话框API
	 *
	 * 调用注册的回调函数打开文件夹选择对话框，返回用户选择的
	 * 文件夹路径。
	 */
	HttpResponse handle_open_folder_dialog(
			const std::string &path,
			const std::map<std::string, std::string> &headers,
			const std::string &body);

	/**
	 * handle_get_directory_contents - 处理获取目录内容API
	 *
	 * 获取指定目录下的所有文件和子目录列表。
	 */
	HttpResponse handle_get_directory_contents(
			const std::string &path,
			const std::map<std::string, std::string> &headers,
			const std::string &body);

	/**
	 * handle_read_file - 处理读取文件API
	 *
	 * 读取文本文件内容并返回。
	 */
	HttpResponse handle_read_file(
			const std::string &path,
			const std::map<std::string, std::string> &headers,
			const std::string &body);

	/**
	 * handle_read_binary_file - 处理读取二进制文件API
	 *
	 * 读取二进制文件内容（图片、视频等）并返回。
	 */
	HttpResponse handle_read_binary_file(
			const std::string &path,
			const std::map<std::string, std::string> &headers,
			const std::string &body);

	/**
	 * handle_save_file - 处理保存文件API
	 *
	 * 将内容保存到指定文件。
	 */
	HttpResponse handle_save_file(
			const std::string &path,
			const std::map<std::string, std::string> &headers,
			const std::string &body);

	/**
	 * handle_create_folder - 处理创建文件夹API
	 *
	 * 在指定路径创建新文件夹。
	 */
	HttpResponse handle_create_folder(
			const std::string &path,
			const std::map<std::string, std::string> &headers,
			const std::string &body);

	/**
	 * handle_create_file - 处理创建文件API
	 *
	 * 在指定路径创建新文件。
	 */
	HttpResponse handle_create_file(
			const std::string &path,
			const std::map<std::string, std::string> &headers,
			const std::string &body);

	/**
	 * handle_delete - 处理删除API
	 *
	 * 删除指定的文件或目录。
	 */
	HttpResponse handle_delete(
			const std::string &path,
			const std::map<std::string, std::string> &headers,
			const std::string &body);

	/**
	 * handle_rename - 处理重命名API
	 *
	 * 重命名文件或目录。
	 */
	HttpResponse handle_rename(
			const std::string &path,
			const std::map<std::string, std::string> &headers,
			const std::string &body);

	/**
	 * handle_get_file_info - 处理获取文件信息API
	 *
	 * 获取文件的详细信息（类型、MIME等）。
	 */
	HttpResponse handle_get_file_info(
			const std::string &path,
			const std::map<std::string, std::string> &headers,
			const std::string &body);

	/**
	 * handle_save_all - 处理保存所有文件API
	 *
	 * 批量保存多个文件。
	 */
	HttpResponse handle_save_all(
			const std::string &path,
			const std::map<std::string, std::string> &headers,
			const std::string &body);

	/**
	 * handle_refresh - 处理刷新API
	 *
	 * 刷新文件列表，返回成功响应。
	 */
	HttpResponse handle_refresh(
			const std::string &path,
			const std::map<std::string, std::string> &headers,
			const std::string &body);

	/**
	 * handle_change_wallpaper - 处理更换壁纸API
	 *
	 * 更换编辑器背景壁纸。
	 */
	HttpResponse handle_change_wallpaper(
			const std::string &path,
			const std::map<std::string, std::string> &headers,
			const std::string &body);

	/**
	 * handle_get_wallpapers - 处理获取壁纸列表API
	 *
	 * 获取所有可用的壁纸文件列表。
	 */
	HttpResponse handle_get_wallpapers(
			const std::string &path,
			const std::map<std::string, std::string> &headers,
			const std::string &body);

	/* ====================================================================
	 * 私有方法 - 高性能编辑器 API
	 * ==================================================================== */

	/**
	 * handle_open_file_virtual - 使用虚拟化方式打开文件
	 *
	 * 使用 TextBuffer (Piece Table) 架构打开文件，支持大文件的高效编辑。
	 * 返回文件的元数据（总行数、总字符数），不返回全部内容。
	 *
	 * @path: 请求路径（未使用）
	 * @headers: 请求头（未使用）
	 * @body: JSON请求体，包含path字段
	 *
	 * 返回: JSON响应，包含success、totalLines、totalChars、language字段
	 */
	HttpResponse handle_open_file_virtual(
			const std::string &path,
			const std::map<std::string, std::string> &headers,
			const std::string &body);

	/**
	 * handle_get_lines - 获取指定行范围的内容
	 *
	 * 从 TextBuffer 中获取指定行范围的文本内容，用于虚拟滚动渲染。
	 * 只返回可见区域的行，大幅减少数据传输量。
	 *
	 * @path: 请求路径（未使用）
	 * @headers: 请求头（未使用）
	 * @body: JSON请求体，包含path、start_line、end_line字段
	 *
	 * 返回: JSON响应，包含success、lines、language字段
	 */
	HttpResponse handle_get_lines(
			const std::string &path,
			const std::map<std::string, std::string> &headers,
			const std::string &body);

	/**
	 * handle_get_line_count - 获取总行数
	 *
	 * 获取当前打开文件的总行数。
	 *
	 * @path: 请求路径（未使用）
	 * @headers: 请求头（未使用）
	 * @body: JSON请求体，包含path字段
	 *
	 * 返回: JSON响应，包含success、totalLines字段
	 */
	HttpResponse handle_get_line_count(
			const std::string &path,
			const std::map<std::string, std::string> &headers,
			const std::string &body);

	/**
	 * handle_edit_insert - 插入文本
	 *
	 * 在指定位置插入文本到 TextBuffer 中。
	 *
	 * @path: 请求路径（未使用）
	 * @headers: 请求头（未使用）
	 * @body: JSON请求体，包含path、position、text字段
	 *
	 * 返回: JSON响应，包含success、newTotalLines字段
	 */
	HttpResponse handle_edit_insert(
			const std::string &path,
			const std::map<std::string, std::string> &headers,
			const std::string &body);

	/**
	 * handle_edit_delete - 删除文本范围
	 *
	 * 删除指定范围的文本。
	 *
	 * @path: 请求路径（未使用）
	 * @headers: 请求头（未使用）
	 * @body: JSON请求体，包含path、start_position、end_position字段
	 *
	 * 返回: JSON响应，包含success、newTotalLines字段
	 */
	HttpResponse handle_edit_delete(
			const std::string &path,
			const std::map<std::string, std::string> &headers,
			const std::string &body);

	/**
	 * handle_edit_replace - 替换文本范围
	 *
	 * 将指定范围的文本替换为新文本。
	 *
	 * @path: 请求路径（未使用）
	 * @headers: 请求头（未使用）
	 * @body: JSON请求体，包含path、start_position、end_position、text字段
	 *
	 * 返回: JSON响应，包含success、newTotalLines字段
	 */
	HttpResponse handle_edit_replace(
			const std::string &path,
			const std::map<std::string, std::string> &headers,
			const std::string &body);

	/**
	 * handle_close_file_virtual - 关闭虚拟文件
	 *
	 * 关闭并释放 TextBuffer 资源。
	 *
	 * @path: 请求路径（未使用）
	 * @headers: 请求头（未使用）
	 * @body: JSON请求体，包含path字段
	 *
	 * 返回: JSON响应，包含success字段
	 */
	HttpResponse handle_close_file_virtual(
			const std::string &path,
			const std::map<std::string, std::string> &headers,
			const std::string &body);

	/* ====================================================================
	 * 私有方法 - 静态文件服务
	 * ==================================================================== */

	/**
	 * handle_static_file - 处理静态文件请求
	 *
	 * 处理对静态资源文件（HTML、CSS、JS、图片等）的请求。
	 *
	 * @path: 请求的文件路径
	 *
	 * 返回值: HttpResponse结构体
	 */
	HttpResponse handle_static_file(const std::string &path);

	/**
	 * read_static_file - 读取静态文件内容
	 *
	 * 从磁盘读取静态文件的内容。
	 *
	 * @file_path: 文件路径（相对于web目录）
	 *
	 * 返回值: 文件内容字符串，失败返回空字符串
	 */
	std::string read_static_file(const std::string &file_path);

	/**
	 * get_http_mime_type - 获取文件的MIME类型
	 *
	 * 根据文件扩展名返回对应的MIME类型，用于设置HTTP响应的
	 * Content-Type头部。
	 *
	 * @file_path: 文件路径
	 *
	 * 返回值: MIME类型字符串
	 */
	std::string get_http_mime_type(const std::string &file_path);

	/* ====================================================================
	 * 私有方法 - 辅助函数
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
	 * detect_language_simple - 简化的语言检测函数
	 *
	 * 根据文件扩展名检测编程语言。
	 *
	 * @filename: 文件名
	 *
	 * 返回值: 检测到的语言名称
	 */
	std::string detect_language_simple(const std::string &filename);
};

#endif /* MIKUFY_WEB_SERVER_H */
