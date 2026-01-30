/**
 * Mikufy v2.5 - Web服务器头文件
 * 负责HTTP服务器和WebSocket通信
 */

#ifndef MIKUFY_WEB_SERVER_H
#define MIKUFY_WEB_SERVER_H

#include "main.h"
#include "file_manager.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/**
 * Web服务器类
 * 提供HTTP服务，处理前端请求
 */
class WebServer {
public:
    WebServer(FileManager* fileManager);
    ~WebServer();
    
    // 禁止拷贝和移动
    WebServer(const WebServer&) = delete;
    WebServer& operator=(const WebServer&) = delete;
    WebServer(WebServer&&) = delete;
    WebServer& operator=(WebServer&&) = delete;
    
    /**
     * 启动Web服务器
     * @param port 端口号
     * @return 是否成功
     */
    bool start(int port);
    
    /**
     * 停止Web服务器
     */
    void stop();
    
    /**
     * 检查服务器是否正在运行
     * @return 是否正在运行
     */
    bool isRunning() const;
    
    /**
     * 设置打开文件夹对话框的回调函数
     * @param callback 回调函数
     */
    void setOpenFolderCallback(std::function<std::string()> callback);

private:
    FileManager* fileManager;
    int serverSocket;
    int port;
    std::atomic<bool> running;
    std::thread serverThread;
    std::mutex mutex;
    
    // 打开文件夹对话框回调
    std::function<std::string()> openFolderCallback;
    
    // HTTP路由表
    std::map<std::string, HttpHandler> routes;
    
    /**
     * 服务器主循环
     */
    void serverLoop();
    
    /**
     * 接受客户端连接
     * @return 客户端socket，失败返回-1
     */
    int acceptClient();
    
    /**
     * 处理客户端请求
     * @param clientSocket 客户端socket
     */
    void handleClient(int clientSocket);
    
    /**
     * 解析HTTP请求
     * @param request 请求字符串
     * @param method 输出HTTP方法
     * @param path 输出请求路径
     * @param headers 输出请求头
     * @param body 输出请求体
     * @return 是否成功
     */
    bool parseHttpRequest(const std::string& request, std::string& method, std::string& path, 
                          std::map<std::string, std::string>& headers, std::string& body);
    
    /**
     * 构造HTTP响应
     * @param response 响应结构体
     * @return 响应字符串
     */
    std::string buildHttpResponse(const HttpResponse& response);
    
    /**
     * 发送HTTP响应
     * @param clientSocket 客户端socket
     * @param response 响应结构体
     * @return 是否成功
     */
    bool sendHttpResponse(int clientSocket, const HttpResponse& response);
    
    /**
     * URL解码
     * @param encoded 编码的URL
     * @return 解码后的URL
     */
    std::string urlDecode(const std::string& encoded);
    
    /**
     * URL编码
     * @param decoded 解码的URL
     * @return 编码后的URL
     */
    std::string urlEncode(const std::string& decoded);
    
    /**
     * 解析查询参数
     * @param queryString 查询字符串
     * @return 参数映射
     */
    std::map<std::string, std::string> parseQueryString(const std::string& queryString);
    
    /**
     * 注册HTTP路由
     */
    void registerRoutes();
    
    // === API处理函数 ===
    
    /**
     * 处理打开文件夹对话框API
     */
    HttpResponse handleOpenFolderDialog(const std::string& path, 
                                        const std::map<std::string, std::string>& headers, 
                                        const std::string& body);
    
    /**
     * 处理获取目录内容API
     */
    HttpResponse handleGetDirectoryContents(const std::string& path, 
                                            const std::map<std::string, std::string>& headers, 
                                            const std::string& body);
    
    /**
     * 处理读取文件API
     */
    HttpResponse handleReadFile(const std::string& path, 
                                const std::map<std::string, std::string>& headers, 
                                const std::string& body);
    
    /**
     * 处理读取二进制文件API（用于图片、视频等）
     */
    HttpResponse handleReadBinaryFile(const std::string& path, 
                                       const std::map<std::string, std::string>& headers, 
                                       const std::string& body);
    
    /**
     * 处理保存文件API
     */
    HttpResponse handleSaveFile(const std::string& path, 
                                const std::map<std::string, std::string>& headers, 
                                const std::string& body);
    
    /**
     * 处理创建文件夹API
     */
    HttpResponse handleCreateFolder(const std::string& path, 
                                    const std::map<std::string, std::string>& headers, 
                                    const std::string& body);
    
    /**
     * 处理创建文件API
     */
    HttpResponse handleCreateFile(const std::string& path, 
                                  const std::map<std::string, std::string>& headers, 
                                  const std::string& body);
    
    /**
     * 处理删除API
     */
    HttpResponse handleDelete(const std::string& path, 
                              const std::map<std::string, std::string>& headers, 
                              const std::string& body);
    
    /**
     * 处理重命名API
     */
    HttpResponse handleRename(const std::string& path, 
                              const std::map<std::string, std::string>& headers, 
                              const std::string& body);
    
    /**
     * 处理获取文件信息API
     */
    HttpResponse handleGetFileInfo(const std::string& path, 
                                   const std::map<std::string, std::string>& headers, 
                                   const std::string& body);
    
    /**
     * 处理保存所有文件API
     */
    HttpResponse handleSaveAll(const std::string& path, 
                               const std::map<std::string, std::string>& headers, 
                               const std::string& body);
    
    /**
     * 处理刷新API
     */
    HttpResponse handleRefresh(const std::string& path, 
                               const std::map<std::string, std::string>& headers, 
                               const std::string& body);
    
    /**
     * 处理静态文件请求
     */
    HttpResponse handleStaticFile(const std::string& path);
    
    /**
     * 读取静态文件内容
     * @param filePath 文件路径
     * @return 文件内容
     */
    std::string readStaticFile(const std::string& filePath);
    
    /**
     * 获取文件的MIME类型（用于HTTP响应）
     * @param filePath 文件路径
     * @return MIME类型
     */
    std::string getHttpMimeType(const std::string& filePath);
};

#endif // MIKUFY_WEB_SERVER_H