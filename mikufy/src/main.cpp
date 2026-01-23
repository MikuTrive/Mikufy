// ==================== 头文件包含 ====================
// GTK+ 图形界面库
#include <gtk/gtk.h>

// WebKitGTK webview 库
#include <webkit2/webkit2.h>

// JavaScriptCore JavaScript 引擎库
#include <jsc/jsc.h>

// C++ 标准库
#include <string>        // 字符串处理
#include <fstream>       // 文件输入输出流
#include <sstream>       // 字符串流
#include <filesystem>    // 文件系统操作 (C++17)
#include <vector>        // 动态数组
#include <map>           // 映射容器
#include <algorithm>     // 算法
#include <cstring>       // C 字符串操作

// 使用 std::filesystem 命名空间简化代码
namespace fs = std::filesystem;

// ==================== 应用信息常量 ====================
/**
 * 应用名称
 * 显示在窗口标题和日志中
 */
const char* APP_NAME = "Mikufy";

/**
 * 应用版本号
 * 遵循语义化版本规范
 */
const char* APP_VERSION = "v1.23";

/**
 * 应用发布日期
 * 格式：YYYY/M/D
 */
const char* APP_RELEASE_DATE = "2026/1/23";

// ==================== 性能配置常量 ====================

/**
 * 最大文件读取大小（字节）
 * 超过此大小的文件将只读取前 N 字节，避免加载大文件导致卡顿
 * 默认：10 MB
 */
static const size_t MAX_FILE_SIZE = 10 * 1024 * 1024;

/**
 * 大文件预览大小（字节）
 * 超过最大文件大小的文件，只读取前 N 字节作为预览
 * 默认：100 KB
 */
static const size_t LARGE_FILE_PREVIEW_SIZE = 100 * 1024;

// ==================== 全局变量 ====================

/**
 * 主窗口指针
 * 存储 GTK 窗口对象的引用
 * 用于窗口操作和事件处理
 */
static GtkWidget* main_window = nullptr;

/**
 * webview 指针
 * 存储 WebKitWebView 对象的引用
 * 用于显示 HTML/CSS/JavaScript 界面
 */
static WebKitWebView* web_view = nullptr;

/**
 * 当前工作区路径
 * 存储用户选择的根目录路径
 * 所有文件操作都基于此路径
 */
static std::string current_workspace;

/**
 * 文件内容缓存
 * 用于缓存已打开文件的内容
 * 实现自动保存功能
 * 键：文件路径
 * 值：文件内容
 */
static std::map<std::string, std::string> file_content_cache;

/**
 * 窗口默认宽度（像素）
 */
static const int DEFAULT_WINDOW_WIDTH = 1200;

/**
 * 窗口默认高度（像素）
 */
static const int DEFAULT_WINDOW_HEIGHT = 800;

// ==================== 工具函数 ====================

/**
 * 获取应用资源目录
 * 
 * 查找包含 UI 文件（index.html, style.css, app.js 等）的目录
 * 支持多种部署方式：
 * - 与可执行文件同目录
 * - 可执行文件上级目录
 * - mikufy/ui 子目录
 * - 当前目录
 * 
 * @return UI 目录的绝对路径，找不到时返回 "./ui"
 */
std::string get_ui_directory() {
    // 获取可执行文件的完整路径
    char exe_path[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", exe_path, PATH_MAX);
    if (count == -1) {
        // 无法获取可执行文件路径，返回默认路径
        return "./ui";
    }
    
    // 确保字符串以 null 结尾
    exe_path[count] = '\0';
    
    // 提取可执行文件所在目录
    std::string exe_path_str(exe_path);
    std::string exe_dir = exe_path_str.substr(0, exe_path_str.find_last_of('/'));
    
    // 定义所有可能的 UI 目录位置
    // 支持多种部署方式，提高兼容性
    std::vector<std::string> possible_paths = {
        exe_dir + "/ui",                    // 可执行文件同目录下的 ui
        exe_dir + "/../ui",                 // 可执行文件上一级目录下的 ui
        exe_dir + "/../../ui",              // 可执行文件上两级目录下的 ui
        exe_dir + "/mikufy/ui",             // mikufy/ui 目录
        exe_dir + "/../mikufy/ui",          // 上一级目录下的 mikufy/ui
        exe_dir + "/../../mikufy/ui",       // 上两级目录下的 mikufy/ui
        "./ui",                             // 当前目录下的 ui
        "./mikufy/ui"                       // 当前目录下的 mikufy/ui
    };
    
    // 遍历所有可能的路径，查找第一个存在的目录
    for (const auto& path : possible_paths) {
        // 检查目录是否存在
        if (fs::exists(path) && fs::is_directory(path)) {
            // 检查 index.html 是否存在
            std::string index_path = path + "/index.html";
            if (fs::exists(index_path)) {
                return path;
            }
        }
    }
    
    // 所有路径都找不到，打印警告并返回默认路径
    g_printerr("警告: 无法找到 UI 目录，尝试过的路径:\n");
    for (const auto& path : possible_paths) {
        g_printerr("  - %s\n", path.c_str());
    }
    return "./ui";
}

/**
 * 读取文件内容
 * 
 * 以文本模式读取指定文件的内容
 * 
 * @param filepath 文件的完整路径
 * @return 文件内容字符串，读取失败返回空字符串
 */
std::string read_file_content(const std::string& filepath) {
    // 打开文件（文本模式）
    std::ifstream file(filepath);
    
    // 检查文件是否成功打开
    if (!file.is_open()) {
        return "";
    }
    
    // 使用字符串流读取文件内容
    std::stringstream buffer;
    buffer << file.rdbuf();
    
    // 返回文件内容
    return buffer.str();
}

/**
 * 写入文件内容
 * 
 * 以文本模式写入内容到指定文件
 * 如果文件不存在则创建，如果存在则覆盖
 * 
 * @param filepath 文件的完整路径
 * @param content 要写入的内容
 * @return 写入成功返回 true，失败返回 false
 */
bool write_file_content(const std::string& filepath, const std::string& content) {
    // 打开文件（文本模式，覆盖写入）
    std::ofstream file(filepath);
    
    // 检查文件是否成功打开
    if (!file.is_open()) {
        return false;
    }
    
    // 写入内容
    file << content;
    
    // 检查写入是否成功
    return file.good();
}

/**
 * 文件信息结构体
 * 
 * 用于存储文件或目录的元数据信息
 * 支持递归构建文件树
 */
struct FileInfo {
    std::string name;                    // 文件名或目录名
    std::string path;                    // 完整路径
    bool is_directory;                   // 是否为目录
    bool is_hidden;                      // 是否为隐藏文件（以 . 开头）
    std::vector<FileInfo> children;      // 子文件/目录列表（仅目录有）
};

/**
 * 递归扫描目录获取文件树（性能优化版）
 * 
 * 扫描指定目录及其所有子目录，构建完整的文件树结构
 * 支持隐藏文件过滤和排序
 * 
 * 性能优化：
 * - 延迟加载：只扫描第一层目录，子目录的 children 为空
 * - 只在展开时才加载子目录内容
 * - 减少内存占用和 JSON 序列化时间
 * 
 * @param dir_path 要扫描的目录路径
 * @param show_hidden 是否包含隐藏文件（默认为 true）
 * @param max_depth 最大扫描深度（0 表示只扫描当前层，1 表示扫描一层子目录，以此类推）
 * @return 文件信息列表，包含所有文件和子目录
 */
std::vector<FileInfo> scan_directory(const std::string& dir_path, bool show_hidden = true, int max_depth = 0) {
    std::vector<FileInfo> files;
    
    try {
        // 使用 directory_iterator 而不是 recursive_directory_iterator，避免递归扫描
        // 只扫描当前层，子目录的 children 为空，延迟加载
        for (const auto& entry : fs::directory_iterator(dir_path, fs::directory_options::skip_permission_denied)) {
            // 获取文件名和完整路径
            std::string name = entry.path().filename().string();
            std::string path = entry.path().string();
            
            // 跳过 .git 目录（版本控制目录）
            if (name == ".git") continue;
            
            // 检查是否为隐藏文件（以 . 开头）
            bool is_hidden = !name.empty() && name[0] == '.';
            
            // 如果不显示隐藏文件且当前是隐藏文件，跳过
            if (!show_hidden && is_hidden) continue;
            
            // 创建文件信息对象
            FileInfo info;
            info.name = name;
            info.path = path;
            info.is_directory = entry.is_directory();
            info.is_hidden = is_hidden;
            
            // 性能优化：如果 max_depth > 0，才递归扫描子目录
            // 默认 max_depth = 0，表示只扫描当前层，子目录的 children 为空
            // 这样可以大幅减少扫描时间和内存占用
            if (info.is_directory && max_depth > 0) {
                try {
                    info.children = scan_directory(path, show_hidden, max_depth - 1);
                } catch (const std::exception& e) {
                    g_printerr("扫描子目录失败: %s - %s\n", path.c_str(), e.what());
                    // 子目录扫描失败，仍然添加该目录，只是 children 为空
                }
            }
            // 如果是目录但 max_depth = 0，children 保持为空（延迟加载）
            // 前端会在用户展开时请求加载子目录
            
            // 添加到文件列表
            files.push_back(info);
        }
    } catch (const fs::filesystem_error& e) {
        g_printerr("扫描目录失败: %s - %s\n", dir_path.c_str(), e.what());
    } catch (const std::exception& e) {
        g_printerr("扫描目录失败: %s - %s\n", dir_path.c_str(), e.what());
    }
    
    // 排序文件列表：
    // 1. 目录在前，文件在后
    // 2. 同类型按字母顺序排序
    std::sort(files.begin(), files.end(), [](const FileInfo& a, const FileInfo& b) {
        // 类型不同：目录优先
        if (a.is_directory != b.is_directory) {
            return a.is_directory > b.is_directory;
        }
        // 类型相同：按名称排序
        return a.name < b.name;
    });
    
    return files;
}

/**
 * 扫描单个目录的内容（用于延迟加载）
 * 
 * 只扫描指定目录的第一层，不递归扫描子目录
 * 用于用户展开文件夹时加载其内容
 * 
 * @param dir_path 要扫描的目录路径
 * @param show_hidden 是否包含隐藏文件（默认为 true）
 * @return 文件信息列表
 */
std::vector<FileInfo> scan_directory_lazy(const std::string& dir_path, bool show_hidden = true) {
    return scan_directory(dir_path, show_hidden, 0);
}

/**
 * 转义 JSON 字符串中的特殊字符
 * 
 * 将字符串中的特殊字符转换为 JSON 转义序列
 * 支持的转义字符：
 * - " -> \"
 * - \ -> \\
 * - \n -> \n
 * - \r -> \r
 * - \t -> \t
 * - 控制字符（0-31）-> \uXXXX
 * 
 * @param str 原始字符串
 * @return 转义后的字符串
 */
std::string escape_json_string(const std::string& str) {
    std::string result;
    
    // 遍历字符串中的每个字符
    for (char c : str) {
        switch (c) {
            case '"':  // 双引号
                result += "\\\"";
                break;
            case '\\': // 反斜杠
                result += "\\\\";
                break;
            case '\n': // 换行符
                result += "\\n";
                break;
            case '\r': // 回车符
                result += "\\r";
                break;
            case '\t': // 制表符
                result += "\\t";
                break;
            default:
                // 控制字符（ASCII 0-31）使用 Unicode 转义
                if (c >= 0 && c < 32) {
                    char buf[8];
                    snprintf(buf, sizeof(buf), "\\u%04x", (unsigned int)c);
                    result += buf;
                } else {
                    // 普通字符直接添加
                    result += c;
                }
        }
    }
    
    return result;
}

/**
 * 将文件信息列表转换为 JSON 字符串
 * 
 * 递归转换文件树为 JSON 格式
 * JSON 结构：
 * [
 *   {
 *     "name": "文件名",
 *     "path": "完整路径",
 *     "isDirectory": true/false,
 *     "isHidden": true/false,
 *     "children": [...] // 仅目录有
 *   },
 *   ...
 * ]
 * 
 * @param files 文件信息列表
 * @return JSON 格式的字符串
 */
std::string files_to_json(const std::vector<FileInfo>& files) {
    std::string json = "[";
    
    // 遍历所有文件/目录
    for (size_t i = 0; i < files.size(); i++) {
        const auto& file = files[i];
        
        // 构建文件信息 JSON 对象
        json += "{";
        json += "\"name\":\"" + escape_json_string(file.name) + "\",";
        json += "\"path\":\"" + escape_json_string(file.path) + "\",";
        json += "\"isDirectory\":" + std::string(file.is_directory ? "true" : "false") + ",";
        json += "\"isHidden\":" + std::string(file.is_hidden ? "true" : "false");
        
        // 如果是目录且有子项，递归添加子项
        if (file.is_directory && !file.children.empty()) {
            json += ",\"children\":" + files_to_json(file.children);
        }
        
        json += "}";
        
        // 添加逗号分隔符（最后一个不加）
        if (i < files.size() - 1) {
            json += ",";
        }
    }
    
    json += "]";
    return json;
}

/**
 * 创建 JSON 响应
 * 
 * 构建标准格式的 JSON 响应
 * 格式：{"success": true/false, ...data}
 * 
 * @param success 操作是否成功
 * @param data 要添加的数据字符串（JSON 格式，不含花括号）
 * @return JSON 响应字符串
 */
std::string create_json_response(bool success, const std::string& data) {
    std::string json = "{";
    
    // 添加 success 字段
    json += "\"success\":" + std::string(success ? "true" : "false");
    
    // 如果有数据，添加数据字段
    if (!data.empty()) {
        json += "," + data;
    }
    
    json += "}";
    return json;
}

// ==================== 前端消息处理 ====================

/**
 * 处理来自前端的 JavaScript 消息
 * 
 * 这是前端与后端通信的核心函数
 * 前端通过 window.webkit.messageHandlers.backend.postMessage() 发送消息
 * 消息格式：{"action": "操作类型", "data": {...}}
 * 
 * 支持的操作：
 * - minimize: 最小化窗口
 * - maximize: 最大化/还原窗口
 * - close: 关闭窗口
 * - openFolder: 打开文件夹选择对话框
 * - getFileTree: 获取文件树
 * - openFile: 打开文件
 * - saveFile: 保存文件
 * - createFolder: 创建文件夹
 * - createFile: 创建文件
 * - rename: 重命名文件或文件夹
 * - delete: 删除文件或文件夹
 * - checkWorkspace: 检查工作区
 * 
 * @param manager 用户内容管理器（未使用）
 * @param js_result JavaScript 结果对象
 * @param user_data 用户数据（未使用）
 */
void handle_javascript_message(WebKitUserContentManager* manager, WebKitJavascriptResult* js_result, gpointer user_data) {
    // 抑制未使用参数警告
    (void)manager;
    (void)user_data;
    
    // 获取 JavaScript 值对象
    JSCValue* js_value = webkit_javascript_result_get_js_value(js_result);
    if (!js_value) {
        g_printerr("错误: 无法获取 JavaScript 值\n");
        return;
    }
    
    // 检查是否为字符串类型
    if (!jsc_value_is_string(js_value)) {
        g_printerr("错误: JavaScript 值不是字符串类型\n");
        return;
    }
    
    // 将 JavaScript 值转换为 C 字符串
    gchar* json_str = jsc_value_to_string(js_value);
    if (!json_str) {
        g_printerr("错误: 无法从 JavaScript 值获取字符串\n");
        return;
    }
    
    // 打印接收到的消息（用于调试）
    g_print("收到前端消息: %s\n", json_str);
    
    // 将 C 字符串转换为 C++ 字符串
    std::string msg_str(json_str);
    g_free(json_str);
    
    // 解析 JSON 消息
    // 注意：这里使用简单的字符串解析，实际项目应使用 JSON 库（如 nlohmann/json）
    std::string action;
    std::string data;
    
    // 提取 action 字段
    size_t action_pos = msg_str.find("\"action\":\"");
    if (action_pos != std::string::npos) {
        action_pos += 10; // 跳过 "action":
        size_t action_end = msg_str.find("\"", action_pos);
        if (action_end != std::string::npos) {
            action = msg_str.substr(action_pos, action_end - action_pos);
        }
    }
    
    // 响应字符串
    std::string response;
    
    // ==================== 根据不同的 action 执行相应操作 ====================
    
    if (action == "minimize") {
        // 最小化窗口
        gtk_window_iconify(GTK_WINDOW(main_window));
        response = create_json_response(true, "");
    }
    else if (action == "maximize") {
        // 最大化/还原窗口
        if (gtk_window_is_maximized(GTK_WINDOW(main_window))) {
            // 如果已最大化，则还原
            gtk_window_unmaximize(GTK_WINDOW(main_window));
        } else {
            // 如果未最大化，则最大化
            gtk_window_maximize(GTK_WINDOW(main_window));
        }
        response = create_json_response(true, "");
    }
    else if (action == "close") {
        // 关闭窗口
        gtk_window_close(GTK_WINDOW(main_window));
        return;  // 不需要发送响应，窗口即将关闭
    }
    else if (action == "openFolder") {
        // 打开文件夹选择对话框
        g_print("打开文件夹选择对话框...\n");
        
        // 创建文件选择对话框
        GtkWidget* dialog = gtk_file_chooser_dialog_new(
            "选择工作区文件夹",              // 对话框标题
            GTK_WINDOW(main_window),         // 父窗口
            GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, // 选择文件夹模式
            "取消", GTK_RESPONSE_CANCEL,     // 取消按钮
            "选择", GTK_RESPONSE_ACCEPT,     // 确认按钮
            nullptr                          // 结束标记
        );
        
        // 运行对话框并等待用户响应
        gint result = gtk_dialog_run(GTK_DIALOG(dialog));
        if (result == GTK_RESPONSE_ACCEPT) {
            // 用户选择了文件夹
            char* folder = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
            current_workspace = folder;
            g_free(folder);
            
            g_print("选择的文件夹: %s\n", current_workspace.c_str());
            
            // 返回成功响应和文件夹路径
            response = create_json_response(true, "\"path\":\"" + escape_json_string(current_workspace) + "\"");
        } else {
            // 用户取消了选择
            g_print("用户取消了文件夹选择\n");
            response = create_json_response(false, "");
        }
        
        // 销毁对话框
        gtk_widget_destroy(dialog);
    }
    else if (action == "getFileTree") {
        // 获取文件树（只扫描第一层，延迟加载子目录）
        if (!current_workspace.empty()) {
            // 扫描工作区目录（max_depth = 0，只扫描第一层）
            std::vector<FileInfo> files = scan_directory(current_workspace, true, 0);
            
            // 转换为 JSON
            std::string files_json = files_to_json(files);
            
            // 返回文件树数据
            response = create_json_response(true, "\"files\":" + files_json);
        } else {
            // 没有选择工作区
            response = create_json_response(false, "\"error\":\"No workspace selected\"");
        }
    }
    else if (action == "getSubDirectory") {
        // 获取子目录内容（延迟加载）
        size_t path_pos = msg_str.find("\"path\":\"");
        if (path_pos != std::string::npos) {
            path_pos += 8; // 跳过 "path":
            size_t path_end = msg_str.find("\"", path_pos);
            if (path_end != std::string::npos) {
                // 提取目录路径
                std::string dir_path = msg_str.substr(path_pos, path_end - path_pos);
                
                // 扫描子目录内容（只扫描第一层）
                std::vector<FileInfo> files = scan_directory_lazy(dir_path, true);
                
                // 转换为 JSON
                std::string files_json = files_to_json(files);
                
                // 返回子目录内容
                response = create_json_response(true, "\"files\":" + files_json);
            }
        } else {
            response = create_json_response(false, "\"error\":\"Invalid path\"");
        }
    }
    else if (action == "openFile") {
        // 打开文件
        size_t path_pos = msg_str.find("\"path\":\"");
        if (path_pos != std::string::npos) {
            path_pos += 8; // 跳过 "path":
            size_t path_end = msg_str.find("\"", path_pos);
            if (path_end != std::string::npos) {
                // 提取文件路径
                std::string filepath = msg_str.substr(path_pos, path_end - path_pos);
                
                // 检查文件大小
                try {
                    uintmax_t file_size = fs::file_size(filepath);
                    
                    if (file_size > MAX_FILE_SIZE) {
                        // 文件过大，只读取前 N 字节作为预览
                        std::ifstream file(filepath);
                        if (file.is_open()) {
                            std::string preview;
                            preview.resize(LARGE_FILE_PREVIEW_SIZE);
                            file.read(&preview[0], LARGE_FILE_PREVIEW_SIZE);
                            preview.resize(file.gcount());
                            
                            // 添加提示信息
                            preview += "\n\n\n... 文件过大（" + std::to_string(file_size / 1024 / 1024) + " MB），只显示前 " + 
                                       std::to_string(LARGE_FILE_PREVIEW_SIZE / 1024) + " KB 预览 ...\n\n" +
                                       "如需完整内容，请使用其他工具打开。";
                            
                            std::string escaped_content = escape_json_string(preview);
                            response = create_json_response(true, "\"content\":\"" + escaped_content + "\",\"isPreview\":true");
                        } else {
                            response = create_json_response(false, "\"error\":\"Failed to open file\"");
                        }
                    } else {
                        // 文件大小正常，读取完整内容
                        std::string content = read_file_content(filepath);
                        std::string escaped_content = escape_json_string(content);
                        response = create_json_response(true, "\"content\":\"" + escaped_content + "\",\"isPreview\":false");
                    }
                } catch (const std::exception& e) {
                    response = create_json_response(false, "\"error\":\"" + escape_json_string(e.what()) + "\"");
                }
            }
        }
    }
    else if (action == "saveFile") {
        // 保存文件
        size_t path_pos = msg_str.find("\"path\":\"");
        size_t content_pos = msg_str.find("\"content\":\"");
        
        if (path_pos != std::string::npos && content_pos != std::string::npos) {
            // 提取文件路径
            path_pos += 8; // 跳过 "path":
            size_t path_end = msg_str.find("\"", path_pos);
            
            // 提取文件内容
            content_pos += 11; // 跳过 "content":
            size_t content_end = msg_str.find("\"", content_pos);
            
            if (path_end != std::string::npos && content_end != std::string::npos) {
                std::string filepath = msg_str.substr(path_pos, path_end - path_pos);
                std::string content = msg_str.substr(content_pos, content_end - content_pos);
                
                // 解码 JSON 转义的字符串
                std::string decoded_content;
                bool escape = false;
                for (size_t i = 0; i < content.size(); i++) {
                    if (escape) {
                        // 处理转义字符
                        if (content[i] == 'n') decoded_content += '\n';
                        else if (content[i] == 'r') decoded_content += '\r';
                        else if (content[i] == 't') decoded_content += '\t';
                        else if (content[i] == '\\') decoded_content += '\\';
                        else if (content[i] == '"') decoded_content += '"';
                        else decoded_content += content[i];
                        escape = false;
                    } else if (content[i] == '\\') {
                        // 遇到反斜杠，标记下一个字符为转义
                        escape = true;
                    } else {
                        // 普通字符直接添加
                        decoded_content += content[i];
                    }
                }
                
                // 写入文件
                bool success = write_file_content(filepath, decoded_content);
                response = create_json_response(success, success ? "" : "\"error\":\"Failed to save file\"");
            }
        }
    }
    else if (action == "createFolder") {
        // 创建文件夹
        size_t path_pos = msg_str.find("\"path\":\"");
        if (path_pos != std::string::npos) {
            path_pos += 8; // 跳过 "path":
            size_t path_end = msg_str.find("\"", path_pos);
            if (path_end != std::string::npos) {
                // 提取文件夹路径
                std::string folder_path = msg_str.substr(path_pos, path_end - path_pos);
                
                // 创建文件夹
                bool success = fs::create_directory(folder_path);
                response = create_json_response(success, success ? "" : "\"error\":\"Failed to create folder\"");
            }
        }
    }
    else if (action == "createFile") {
        // 创建文件
        size_t path_pos = msg_str.find("\"path\":\"");
        if (path_pos != std::string::npos) {
            path_pos += 8; // 跳过 "path":
            size_t path_end = msg_str.find("\"", path_pos);
            if (path_end != std::string::npos) {
                // 提取文件路径
                std::string file_path = msg_str.substr(path_pos, path_end - path_pos);
                
                // 创建空文件
                bool success = write_file_content(file_path, "");
                response = create_json_response(success, success ? "" : "\"error\":\"Failed to create file\"");
            }
        }
    }
    else if (action == "rename") {
        // 重命名文件或文件夹
        size_t old_path_pos = msg_str.find("\"oldPath\":\"");
        size_t new_path_pos = msg_str.find("\"newPath\":\"");
        
        if (old_path_pos != std::string::npos && new_path_pos != std::string::npos) {
            // 提取旧路径
            old_path_pos += 11; // 跳过 "oldPath":
            size_t old_path_end = msg_str.find("\"", old_path_pos);
            
            // 提取新路径
            new_path_pos += 11; // 跳过 "newPath":
            size_t new_path_end = msg_str.find("\"", new_path_pos);
            
            if (old_path_end != std::string::npos && new_path_end != std::string::npos) {
                std::string old_path = msg_str.substr(old_path_pos, old_path_end - old_path_pos);
                std::string new_path = msg_str.substr(new_path_pos, new_path_end - new_path_pos);
                
                try {
                    // 重命名文件或文件夹
                    fs::rename(old_path, new_path);
                    response = create_json_response(true, "");
                } catch (const std::exception& e) {
                    // 捕获异常并返回错误信息
                    response = create_json_response(false, "\"error\":\"" + escape_json_string(e.what()) + "\"");
                }
            }
        }
    }
    else if (action == "delete") {
        // 删除文件或文件夹
        size_t path_pos = msg_str.find("\"path\":\"");
        size_t is_dir_pos = msg_str.find("\"isDirectory\":");
        
        if (path_pos != std::string::npos && is_dir_pos != std::string::npos) {
            // 提取路径
            path_pos += 8; // 跳过 "path":
            size_t path_end = msg_str.find("\"", path_pos);
            
            // 提取是否为目录标志
            is_dir_pos += 15; // 跳过 "isDirectory":
            
            if (path_end != std::string::npos) {
                std::string file_path = msg_str.substr(path_pos, path_end - path_pos);
                bool is_dir = (msg_str.substr(is_dir_pos, 4) == "true");
                
                try {
                    if (is_dir) {
                        // 删除目录及其所有内容
                        fs::remove_all(file_path);
                    } else {
                        // 删除文件
                        fs::remove(file_path);
                    }
                    response = create_json_response(true, "");
                } catch (const std::exception& e) {
                    // 捕获异常并返回错误信息
                    response = create_json_response(false, "\"error\":\"" + escape_json_string(e.what()) + "\"");
                }
            }
        }
    }
    else if (action == "checkWorkspace") {
        // 检查工作区
        if (!current_workspace.empty()) {
            // 有工作区，返回工作区路径
            response = create_json_response(true, "\"workspace\":\"" + escape_json_string(current_workspace) + "\"");
        } else {
            // 没有工作区
            response = create_json_response(false, "");
        }
    }
    else {
        // 未知操作
        response = create_json_response(false, "\"error\":\"Unknown action\"");
    }
    
    // ==================== 发送响应回前端 ====================
    
    if (!response.empty()) {
        g_print("发送响应到前端: %s\n", response.c_str());
        
        // 构建调用前端回调函数的 JavaScript 代码
        std::string js_code = "if (window.backendCallback) { window.backendCallback(" + response + "); }";
        
        // 执行 JavaScript 代码
        webkit_web_view_evaluate_javascript(
            web_view,
            js_code.c_str(),
            -1,              // 字符串长度（-1 表示自动计算）
            nullptr,         // 世界名称
            nullptr,         // 源代码（用于调试）
            nullptr,         // 行号（用于调试）
            nullptr,         // 列号（用于调试）
            nullptr          // 回调函数
        );
    }
}

/**
 * 向前端发送消息
 * 
 * 主动向前端发送消息
 * 调用前端的 receiveFromBackend 函数
 * 
 * @param message JSON 格式的消息
 */
void send_message_to_frontend(const std::string& message) {
    // 构建调用前端函数的 JavaScript 代码
    std::string js_code = "receiveFromBackend('" + escape_json_string(message) + "');";
    
    // 执行 JavaScript 代码
    webkit_web_view_evaluate_javascript(
        web_view,
        js_code.c_str(),
        -1,              // 字符串长度（-1 表示自动计算）
        nullptr,         // 世界名称
        nullptr,         // 源代码（用于调试）
        nullptr,         // 行号（用于调试）
        nullptr,         // 列号（用于调试）
        nullptr          // 回调函数
    );
}

// ==================== 窗口事件处理 ====================

/**
 * 窗口关闭事件处理
 * 
 * 当用户点击窗口关闭按钮或调用 gtk_window_close() 时触发
 * 执行清理工作并退出应用
 * 
 * @param widget 窗口部件（未使用）
 * @param event 事件对象（未使用）
 * @param user_data 用户数据（未使用）
 * @return FALSE 表示继续关闭窗口
 */
gboolean on_window_close(GtkWidget* widget, GdkEvent* event, gpointer user_data) {
    // 抑制未使用参数警告
    (void)widget;
    (void)event;
    (void)user_data;
    
    // 保存当前工作区路径（可选）
    // 可以在这里添加保存配置的逻辑
    // 例如：保存到配置文件 ~/.config/mikufy/config.json
    
    // 退出 GTK 主循环
    gtk_main_quit();
    
    return FALSE;
}

/**
 * 窗口大小改变事件处理
 * 
 * 当窗口大小改变时触发
 * 可以用于保存窗口大小配置
 * 
 * @param widget 窗口部件（未使用）
 * @param allocation 新的窗口大小分配信息（未使用）
 * @param user_data 用户数据（未使用）
 */
void on_window_size_allocate(GtkWidget* widget, GtkAllocation* allocation, gpointer user_data) {
    // 抑制未使用参数警告
    (void)widget;
    (void)allocation;
    (void)user_data;
    
    // 可以在这里保存窗口大小
    // 例如：保存到配置文件
}

// ==================== 主函数 ====================

/**
 * 主函数
 * 
 * 程序的入口点
 * 负责：
 * 1. 初始化 GTK
 * 2. 创建主窗口
 * 3. 创建 webview
 * 4. 加载 UI 文件
 * 5. 进入主循环
 * 
 * @param argc 命令行参数数量
 * @param argv 命令行参数数组
 * @return 程序退出码（0 表示成功）
 */
int main(int argc, char* argv[]) {
    // ==================== 初始化 GTK ====================
    gtk_init(&argc, &argv);
    
    // ==================== 创建主窗口 ====================
    main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    
    // 设置窗口标题
    gtk_window_set_title(GTK_WINDOW(main_window), (std::string(APP_NAME) + " " + APP_VERSION).c_str());
    
    // 设置窗口默认大小
    gtk_window_set_default_size(GTK_WINDOW(main_window), DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);
    
    // 设置窗口位置（屏幕中央）
    gtk_window_set_position(GTK_WINDOW(main_window), GTK_WIN_POS_CENTER);
    
    // 移除系统窗口装饰（标题栏、边框等）
    // 使用自定义标题栏提供更好的视觉效果
    gtk_window_set_decorated(GTK_WINDOW(main_window), FALSE);
    
    // ==================== 设置窗口图标 ====================
    std::string ui_dir = get_ui_directory();
    std::string icon_path = ui_dir + "/Mikufy.png";
    if (fs::exists(icon_path)) {
        gtk_window_set_icon_from_file(GTK_WINDOW(main_window), icon_path.c_str(), nullptr);
    }
    
    // ==================== 创建用户内容管理器 ====================
    // 用于与 JavaScript 通信
    WebKitUserContentManager* content_manager = webkit_user_content_manager_new();
    
    // 注册 JavaScript 消息处理器
    // 前端可以通过 window.webkit.messageHandlers.backend.postMessage() 发送消息
    webkit_user_content_manager_register_script_message_handler(content_manager, "backend");
    
    // 连接消息接收信号
    g_signal_connect(content_manager, "script-message-received::backend", G_CALLBACK(handle_javascript_message), nullptr);
    
    // ==================== 创建 webview ====================
    // 创建滚动窗口容器
    GtkWidget* scrolled_window = gtk_scrolled_window_new(nullptr, nullptr);
    
    // 创建 webview（使用用户内容管理器）
    web_view = WEBKIT_WEB_VIEW(webkit_web_view_new_with_user_content_manager(content_manager));
    
    // ==================== 配置 webview ====================
    WebKitSettings* settings = webkit_web_view_get_settings(web_view);
    
    // 将 console.log 输出到 stdout（用于调试）
    webkit_settings_set_enable_write_console_messages_to_stdout(settings, TRUE);
    
    // 禁止 JavaScript 自动打开窗口
    webkit_settings_set_javascript_can_open_windows_automatically(settings, FALSE);
    
    // ==================== 性能优化设置 ====================
    // 启用页面缓存（提高性能）
    webkit_settings_set_enable_page_cache(settings, TRUE);
    
    // 启用平滑滚动
    webkit_settings_set_enable_smooth_scrolling(settings, TRUE);
    
    // 禁用开发者工具（生产环境）
    webkit_settings_set_enable_developer_extras(settings, FALSE);
    
    // 允许 JavaScript 访问剪贴板
    webkit_settings_set_javascript_can_access_clipboard(settings, TRUE);
    
    // ==================== 构建窗口布局 ====================
    // 将 webview 添加到滚动窗口
    gtk_container_add(GTK_CONTAINER(scrolled_window), GTK_WIDGET(web_view));
    
    // 将滚动窗口添加到主窗口
    gtk_container_add(GTK_CONTAINER(main_window), scrolled_window);
    
    // ==================== 连接窗口事件 ====================
    // 窗口关闭事件
    g_signal_connect(main_window, "delete-event", G_CALLBACK(on_window_close), nullptr);
    
    // 窗口大小改变事件
    g_signal_connect(main_window, "size-allocate", G_CALLBACK(on_window_size_allocate), nullptr);
    
    // ==================== 显示窗口 ====================
    // 先显示所有部件，再加载 UI，减少启动卡顿
    gtk_widget_show_all(main_window);
    
    // 处理所有待处理的 GTK 事件
    // 确保窗口完全显示后再加载 UI
    while (gtk_events_pending()) {
        gtk_main_iteration();
    }
    
    // ==================== 加载 UI 文件 ====================
    std::string index_path = ui_dir + "/index.html";
    if (fs::exists(index_path)) {
        // 构建 file:// URI
        std::string file_uri = "file://" + index_path;
        
        // 加载 UI 文件
        webkit_web_view_load_uri(web_view, file_uri.c_str());
    } else {
        // UI 文件不存在，打印错误并退出
        g_printerr("错误: 找不到 index.html 文件: %s\n", index_path.c_str());
        return 1;
    }
    
    // ==================== 打印启动信息 ====================
    g_print("%s %s (%s)\n", APP_NAME, APP_VERSION, APP_RELEASE_DATE);
    g_print("工作目录: %s\n", ui_dir.c_str());
    g_print("UI 文件路径: %s\n", index_path.c_str());
    
    // ==================== 进入主循环 ====================
    gtk_main();
    
    // 正常退出
    return 0;
}
