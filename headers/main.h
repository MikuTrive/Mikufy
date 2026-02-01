/* Mikufy v2.2(stable) - 主头文件
 * 包含所有必要的头文件和全局声明
 */

#ifndef MIKUFY_MAIN_H
#define MIKUFY_MAIN_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>

/* WebKitGTK 头文件 (WebKitGTK 6.0 for Fedora 43) */
#include <webkit/webkit.h>
#include <gtk/gtk.h>

/* 文件系统相关 */
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>

/* 字符串处理 */
#include <cstring>
#include <cstdlib>

/* 文件类型检测 */
#include <magic.h>

/* JSON处理 */
#include <nlohmann/json.hpp>

using json = nlohmann::json;

/* 版本信息 */
#define MIKUFY_VERSION		"2.2(stable)"
#define MIKUFY_NAME		"Mikufy"

/* 窗口配置 */
#define WINDOW_WIDTH		1400
#define WINDOW_HEIGHT		900
#define WINDOW_TITLE		"Mikufy v2.2(stable) - 代码编辑器"

/* Web服务器端口 */
#define WEB_SERVER_PORT		8080

/* 缓存大小限制（字节） */
#define MAX_CACHE_SIZE		104857600	/* 100MB */

/* 文件类型结构体 */
struct FileInfo {
	std::string name;
	std::string path;
	bool is_directory;
	size_t size;
	std::string mime_type;
	bool is_binary;
};

/* HTTP响应结构体 */
struct HttpResponse {
	int status_code;
	std::string status_text;
	std::map<std::string, std::string> headers;
	std::string body;
};

/* HTTP请求处理函数类型 */
using HttpHandler = std::function<HttpResponse(
			const std::string &,
			const std::map<std::string, std::string> &,
			const std::string &)>;

/* 前端与后端通信的消息类型 */
enum class MessageType {
	OPEN_FOLDER_DIALOG,
	GET_DIRECTORY_CONTENTS,
	READ_FILE,
	SAVE_FILE,
	CREATE_FOLDER,
	CREATE_FILE,
	DELETE_ITEM,
	RENAME_ITEM,
	GET_FILE_INFO,
	SAVE_ALL,
	REFRESH,
	UNKNOWN
};

/* 前端消息结构体 */
struct FrontendMessage {
	MessageType type;
	json data;
};

/* 后端响应结构体 */
struct BackendResponse {
	bool success;
	std::string error;
	json data;
};

#endif /* MIKUFY_MAIN_H */
