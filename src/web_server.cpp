/**
 * Mikufy v2.5 - Web服务器实现
 * 负责HTTP服务器和前端通信
 */

#include "../headers/web_server.h"
#include <iostream>
#include <cstring>
#include <sstream>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

// 构造函数
WebServer::WebServer(FileManager* fileManager) 
    : fileManager(fileManager), serverSocket(-1), port(WEB_SERVER_PORT), running(false) {
    registerRoutes();
}

// 析构函数
WebServer::~WebServer() {
    stop();
}

/**
 * 设置打开文件夹对话框的回调函数
 */
void WebServer::setOpenFolderCallback(std::function<std::string()> callback) {
    std::lock_guard<std::mutex> lock(mutex);
    openFolderCallback = callback;
}

/**
 * 启动Web服务器
 */
bool WebServer::start(int portNum) {
    std::lock_guard<std::mutex> lock(mutex);
    
    if (running) {
        return false;
    }
    
    port = portNum;
    
    // 创建socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        return false;
    }
    
    // 设置socket选项
    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        close(serverSocket);
        serverSocket = -1;
        return false;
    }
    
    // 绑定地址
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    serverAddr.sin_port = htons(port);
    
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        close(serverSocket);
        serverSocket = -1;
        return false;
    }
    
    // 开始监听
    if (listen(serverSocket, 10) < 0) {
        close(serverSocket);
        serverSocket = -1;
        return false;
    }
    
    // 设置为非阻塞模式
    int flags = fcntl(serverSocket, F_GETFL, 0);
    fcntl(serverSocket, F_SETFL, flags | O_NONBLOCK);
    
    running = true;
    
    // 启动服务器线程
    serverThread = std::thread(&WebServer::serverLoop, this);
    
    return true;
}

/**
 * 停止Web服务器
 */
void WebServer::stop() {
    std::lock_guard<std::mutex> lock(mutex);
    
    if (!running) {
        return;
    }
    
    running = false;
    
    // 关闭服务器socket
    if (serverSocket >= 0) {
        close(serverSocket);
        serverSocket = -1;
    }
    
    // 等待服务器线程结束
    if (serverThread.joinable()) {
        serverThread.join();
    }
}

/**
 * 检查服务器是否正在运行
 */
bool WebServer::isRunning() const {
    return running;
}

/**
 * 服务器主循环
 */
void WebServer::serverLoop() {
    while (running) {
        // 检查serverSocket是否有效
        if (serverSocket < 0) {
            std::cerr << "错误: 服务器socket无效" << std::endl;
            break;
        }

        // 使用poll代替select，避免FD_SETSIZE限制
        struct pollfd fds[1];
        fds[0].fd = serverSocket;
        fds[0].events = POLLIN;
        fds[0].revents = 0;

        int ready = poll(fds, 1, 100); // 100ms超时

        if (ready < 0) {
            if (errno == EINTR) {
                continue;
            }
            std::cerr << "poll调用失败: " << strerror(errno) << std::endl;
            break;
        }

        if (ready > 0 && (fds[0].revents & POLLIN)) {
            // 接受客户端连接
            int clientSocket = acceptClient();
            if (clientSocket >= 0) {
                // 处理客户端请求
                handleClient(clientSocket);
            }
        }
    }
}

/**
 * 接受客户端连接
 */
int WebServer::acceptClient() {
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    
    int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
    if (clientSocket < 0) {
        return -1;
    }
    
    return clientSocket;
}

/**
 * 处理客户端请求
 */
void WebServer::handleClient(int clientSocket) {
    // 接收请求数据
    char buffer[8192];
    std::string request;
    
    // 循环接收直到收到完整的请求
    while (true) {
        ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        
        if (bytesRead <= 0) {
            break;
        }
        
        buffer[bytesRead] = '\0';
        request.append(buffer, bytesRead);
        
        // 检查是否已经收到完整的请求头
        size_t headerEnd = request.find("\r\n\r\n");
        if (headerEnd != std::string::npos) {
            // 查找Content-Length
            size_t contentLengthPos = request.find("Content-Length:");
            if (contentLengthPos != std::string::npos && contentLengthPos < headerEnd) {
                // 提取Content-Length的值
                size_t colonPos = request.find(':', contentLengthPos);
                size_t endPos = request.find("\r\n", colonPos);
                std::string lengthStr = request.substr(colonPos + 1, endPos - colonPos - 1);
                lengthStr.erase(0, lengthStr.find_first_not_of(" \t"));
                lengthStr.erase(lengthStr.find_last_not_of(" \t\r\n") + 1);
                size_t contentLength = std::stoul(lengthStr);
                
                // 检查是否已经收到完整的body
                size_t bodyStart = headerEnd + 4;
                if (request.length() >= bodyStart + contentLength) {
                    break; // 收到完整请求
                }
            } else {
                // 没有Content-Length，可能是GET请求，已经收到完整请求
                break;
            }
        } else if (request.length() > 8192) {
            // 没有找到请求头结束符，并且已经接收了很多数据，可能是错误的请求
            break;
        }
    }
    
    // 解析HTTP请求
    std::string method, path;
    std::map<std::string, std::string> headers;
    std::string body;
    
    if (!parseHttpRequest(request, method, path, headers, body)) {
        close(clientSocket);
        return;
    }
    
    // 处理请求
    HttpResponse response;

    // 去除查询参数获取路由路径
    std::string routePath = path;
    size_t queryPos = path.find('?');
    if (queryPos != std::string::npos) {
        routePath = path.substr(0, queryPos);
    }

    // 查找路由
    auto it = routes.find(routePath);
    if (it != routes.end()) {
        std::cout << "路由匹配成功: " << routePath << ", body长度: " << body.length() << std::endl;
        response = it->second(path, headers, body);
    } else {
        // 处理静态文件
        response = handleStaticFile(path);
    }

    // 发送响应
    sendHttpResponse(clientSocket, response);

    // 关闭连接
    close(clientSocket);
}

/**
 * 解析HTTP请求
 */
bool WebServer::parseHttpRequest(const std::string& request, std::string& method, 
                                  std::string& path, std::map<std::string, std::string>& headers, 
                                  std::string& body) {
    std::istringstream stream(request);
    std::string line;
    
    // 解析请求行
    if (!std::getline(stream, line)) {
        return false;
    }
    
    std::istringstream requestLine(line);
    requestLine >> method >> path;
    
    if (method.empty() || path.empty()) {
        return false;
    }
    
    // 解析请求头
    while (std::getline(stream, line) && line != "\r") {
        size_t pos = line.find(':');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            
            // 去除首尾空白
            while (!key.empty() && (key.back() == ' ' || key.back() == '\r')) {
                key.pop_back();
            }
            while (!value.empty() && (value.front() == ' ')) {
                value.erase(value.begin());
            }
            while (!value.empty() && (value.back() == ' ' || value.back() == '\r')) {
                value.pop_back();
            }
            
            headers[key] = value;
        }
    }
    
    // 解析请求体
    if (method == "POST" || method == "PUT") {
        auto it = headers.find("Content-Length");
        if (it != headers.end()) {
            size_t contentLength = std::stoul(it->second);
            std::cout << "POST请求，Content-Length: " << contentLength << std::endl;
            
            // 从原始request字符串中查找body位置
            // HTTP请求格式: headers\r\n\r\nbody
            size_t bodyStart = request.find("\r\n\r\n");
            if (bodyStart != std::string::npos) {
                bodyStart += 4; // 跳过\r\n\r\n
                if (bodyStart + contentLength <= request.length()) {
                    body = request.substr(bodyStart, contentLength);
                    std::cout << "从request中提取body成功: " << body << std::endl;
                } else {
                    std::cout << "body超出request长度" << std::endl;
                }
            } else {
                std::cout << "找不到body分隔符\\r\\n\\r\\n" << std::endl;
            }
        } else {
            std::cout << "POST请求但没有Content-Length头部" << std::endl;
        }
    }
    
    return true;
}

/**
 * 构造HTTP响应
 */
std::string WebServer::buildHttpResponse(const HttpResponse& response) {
    std::ostringstream oss;
    
    // 状态行
    oss << "HTTP/1.1 " << response.statusCode << " " << response.statusText << "\r\n";
    
    // 响应头
    for (const auto& header : response.headers) {
        oss << header.first << ": " << header.second << "\r\n";
    }
    
    // 空行
    oss << "\r\n";
    
    // 响应体
    oss << response.body;
    
    return oss.str();
}

/**
 * 发送HTTP响应
 */
bool WebServer::sendHttpResponse(int clientSocket, const HttpResponse& response) {
    std::string responseStr = buildHttpResponse(response);
    
    ssize_t sent = send(clientSocket, responseStr.c_str(), responseStr.length(), 0);
    
    return (sent == static_cast<ssize_t>(responseStr.length()));
}

/**
 * URL解码
 */
std::string WebServer::urlDecode(const std::string& encoded) {
    std::string decoded;
    decoded.reserve(encoded.length());
    
    for (size_t i = 0; i < encoded.length(); ++i) {
        if (encoded[i] == '%' && i + 2 < encoded.length()) {
            std::string hexStr = encoded.substr(i + 1, 2);
            char decodedChar = static_cast<char>(std::stoi(hexStr, nullptr, 16));
            decoded += decodedChar;
            i += 2;
        } else if (encoded[i] == '+') {
            decoded += ' ';
        } else {
            decoded += encoded[i];
        }
    }
    
    return decoded;
}

/**
 * URL编码
 */
std::string WebServer::urlEncode(const std::string& decoded) {
    std::string encoded;
    encoded.reserve(decoded.length() * 3);
    
    for (char c : decoded) {
        if (isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded += c;
        } else if (c == ' ') {
            encoded += '+';
        } else {
            char hex[4];
            snprintf(hex, sizeof(hex), "%%%02X", static_cast<unsigned char>(c));
            encoded += hex;
        }
    }
    
    return encoded;
}

/**
 * 解析查询参数
 */
std::map<std::string, std::string> WebServer::parseQueryString(const std::string& queryString) {
    std::map<std::string, std::string> params;
    
    std::istringstream stream(queryString);
    std::string pair;
    
    while (std::getline(stream, pair, '&')) {
        size_t pos = pair.find('=');
        if (pos != std::string::npos) {
            std::string key = urlDecode(pair.substr(0, pos));
            std::string value = urlDecode(pair.substr(pos + 1));
            params[key] = value;
        }
    }
    
    return params;
}

/**
 * 注册HTTP路由
 */
void WebServer::registerRoutes() {
    routes["/api/open-folder"] = [this](const std::string& path, const std::map<std::string, std::string>& headers, const std::string& body) {
        return handleOpenFolderDialog(path, headers, body);
    };
    
    routes["/api/directory-contents"] = [this](const std::string& path, const std::map<std::string, std::string>& headers, const std::string& body) {
        return handleGetDirectoryContents(path, headers, body);
    };
    
    routes["/api/read-file"] = [this](const std::string& path, const std::map<std::string, std::string>& headers, const std::string& body) {
        return handleReadFile(path, headers, body);
    };
    
    routes["/api/read-binary-file"] = [this](const std::string& path, const std::map<std::string, std::string>& headers, const std::string& body) {
        return handleReadBinaryFile(path, headers, body);
    };
    
    routes["/api/save-file"] = [this](const std::string& path, const std::map<std::string, std::string>& headers, const std::string& body) {
        return handleSaveFile(path, headers, body);
    };
    
    routes["/api/create-folder"] = [this](const std::string& path, const std::map<std::string, std::string>& headers, const std::string& body) {
        return handleCreateFolder(path, headers, body);
    };
    
    routes["/api/create-file"] = [this](const std::string& path, const std::map<std::string, std::string>& headers, const std::string& body) {
        return handleCreateFile(path, headers, body);
    };
    
    routes["/api/delete"] = [this](const std::string& path, const std::map<std::string, std::string>& headers, const std::string& body) {
        return handleDelete(path, headers, body);
    };
    
    routes["/api/rename"] = [this](const std::string& path, const std::map<std::string, std::string>& headers, const std::string& body) {
        return handleRename(path, headers, body);
    };
    
    routes["/api/file-info"] = [this](const std::string& path, const std::map<std::string, std::string>& headers, const std::string& body) {
        return handleGetFileInfo(path, headers, body);
    };
    
    routes["/api/save-all"] = [this](const std::string& path, const std::map<std::string, std::string>& headers, const std::string& body) {
        return handleSaveAll(path, headers, body);
    };
    
    routes["/api/refresh"] = [this](const std::string& path, const std::map<std::string, std::string>& headers, const std::string& body) {
        return handleRefresh(path, headers, body);
    };
}

/**
 * 处理打开文件夹对话框API
 */
HttpResponse WebServer::handleOpenFolderDialog(const std::string& path,
                                                const std::map<std::string, std::string>& headers,
                                                const std::string& body) {
    (void)path;
    (void)headers;
    (void)body;

    HttpResponse response;
    response.statusCode = 200;
    response.statusText = "OK";
    response.headers["Content-Type"] = "application/json";

    std::string folderPath;
    try {
        if (openFolderCallback) {
            folderPath = openFolderCallback();
        }
    } catch (const std::exception& e) {
        std::cerr << "打开文件夹对话框异常: " << e.what() << std::endl;
        folderPath = "";
    } catch (...) {
        std::cerr << "打开文件夹对话框发生未知异常" << std::endl;
        folderPath = "";
    }

    json result;
    result["success"] = !folderPath.empty();
    result["path"] = folderPath;

    response.body = result.dump();

    return response;
}

/**
 * 处理获取目录内容API
 */
HttpResponse WebServer::handleGetDirectoryContents(const std::string& path,
                                                    const std::map<std::string, std::string>& headers,
                                                    const std::string& body) {
    (void)headers;
    (void)body;

    HttpResponse response;
    response.statusCode = 200;
    response.statusText = "OK";
    response.headers["Content-Type"] = "application/json";

    // 解析查询参数
    size_t queryPos = path.find('?');
    std::string queryStr = (queryPos != std::string::npos) ? path.substr(queryPos + 1) : "";
    auto params = parseQueryString(queryStr);

    std::string directoryPath = params["path"];
    std::cout << "收到获取目录内容请求: " << path << std::endl;
    std::cout << "解析出的目录路径: " << directoryPath << std::endl;

    if (directoryPath.empty()) {
        json result;
        result["success"] = false;
        result["error"] = "Path parameter is required";
        response.body = result.dump();
        return response;
    }

    std::vector<FileInfo> files;
    bool success = fileManager->getDirectoryContents(directoryPath, files);

    std::cout << "获取目录内容结果: " << (success ? "成功" : "失败") << ", 文件数量: " << files.size() << std::endl;

    json result;
    result["success"] = success;

    if (success) {
        json filesArray = json::array();
        for (const auto& file : files) {
            json fileObj;
            fileObj["name"] = file.name;
            fileObj["path"] = file.path;
            fileObj["isDirectory"] = file.isDirectory;
            fileObj["size"] = file.size;
            fileObj["mimeType"] = file.mimeType;
            fileObj["isBinary"] = file.isBinary;
            filesArray.push_back(fileObj);
        }
        result["files"] = filesArray;
    }

    response.body = result.dump();

    std::cout << "返回的JSON: " << response.body << std::endl;

    return response;
}

/**
 * 处理读取文件API
 */
HttpResponse WebServer::handleReadFile(const std::string& path,
                                        const std::map<std::string, std::string>& headers,
                                        const std::string& body) {
    (void)headers;
    (void)body;

    HttpResponse response;
    response.statusCode = 200;
    response.statusText = "OK";
    response.headers["Content-Type"] = "application/json";
    
    // 解析查询参数
    size_t queryPos = path.find('?');
    std::string queryStr = (queryPos != std::string::npos) ? path.substr(queryPos + 1) : "";
    auto params = parseQueryString(queryStr);
    
    std::string filePath = params["path"];
    if (filePath.empty()) {
        json result;
        result["success"] = false;
        result["error"] = "Path parameter is required";
        response.body = result.dump();
        return response;
    }
    
    std::string content;
    bool success = fileManager->readFile(filePath, content);
    
    json result;
    result["success"] = success;
    result["content"] = content;
    
    response.body = result.dump();
    return response;
}

/**
 * 处理读取二进制文件API（用于图片、视频等）
 */
HttpResponse WebServer::handleReadBinaryFile(const std::string& path,
                                             const std::map<std::string, std::string>& headers,
                                             const std::string& body) {
    (void)headers;
    (void)body;

    HttpResponse response;
    response.statusCode = 200;
    response.statusText = "OK";
    
    // 解析查询参数
    size_t queryPos = path.find('?');
    std::string queryStr = (queryPos != std::string::npos) ? path.substr(queryPos + 1) : "";
    auto params = parseQueryString(queryStr);
    
    std::string filePath = params["path"];
    if (filePath.empty()) {
        response.statusCode = 400;
        response.statusText = "Bad Request";
        response.body = "Path parameter is required";
        return response;
    }
    
    // 获取文件MIME类型
    std::string mimeType = fileManager->getMimeType(filePath);
    response.headers["Content-Type"] = mimeType;
    
    // 读取文件内容
    std::vector<char> content;
    bool success = fileManager->readFileBinary(filePath, content);
    
    if (!success) {
        response.statusCode = 404;
        response.statusText = "Not Found";
        response.body = "File not found or cannot be read";
        return response;
    }
    
    response.body = std::string(content.begin(), content.end());
    return response;
}

/**
 * 处理保存文件API
 */
HttpResponse WebServer::handleSaveFile(const std::string& path,
                                        const std::map<std::string, std::string>& headers,
                                        const std::string& body) {
    (void)path;
    (void)headers;

    HttpResponse response;
    response.statusCode = 200;
    response.statusText = "OK";
    response.headers["Content-Type"] = "application/json";
    
    try {
        json request = json::parse(body);
        std::string filePath = request["path"];
        std::string content = request["content"];
        
        bool success = fileManager->writeFile(filePath, content);
        
        json result;
        result["success"] = success;
        
        response.body = result.dump();
    } catch (const std::exception& e) {
        json result;
        result["success"] = false;
        result["error"] = e.what();
        response.body = result.dump();
    }
    
    return response;
}

/**
 * 处理创建文件夹API
 */
HttpResponse WebServer::handleCreateFolder(const std::string& path,
                                            const std::map<std::string, std::string>& headers,
                                            const std::string& body) {
    (void)path;
    (void)headers;

    std::cout << "收到创建文件夹请求，body: " << body << std::endl;

    HttpResponse response;
    response.statusCode = 200;
    response.statusText = "OK";
    response.headers["Content-Type"] = "application/json";
    
    try {
        json request = json::parse(body);
        std::string parentPath = request["parentPath"];
        std::string name = request["name"];
        
        std::cout << "创建文件夹: parentPath=" << parentPath << ", name=" << name << std::endl;
        
        std::string folderPath = parentPath;
        if (folderPath.back() != '/') {
            folderPath += '/';
        }
        folderPath += name;
        
        bool success = fileManager->createDirectory(folderPath);
        
        std::cout << "创建文件夹结果: " << (success ? "成功" : "失败") << std::endl;
        
        json result;
        result["success"] = success;
        
        response.body = result.dump();
    } catch (const std::exception& e) {
        std::cout << "创建文件夹异常: " << e.what() << std::endl;
        json result;
        result["success"] = false;
        result["error"] = e.what();
        response.body = result.dump();
    }
    
    return response;
}

/**
 * 处理创建文件API
 */
HttpResponse WebServer::handleCreateFile(const std::string& path,
                                          const std::map<std::string, std::string>& headers,
                                          const std::string& body) {
    (void)path;
    (void)headers;

    std::cout << "收到创建文件请求，body: " << body << std::endl;

    HttpResponse response;
    response.statusCode = 200;
    response.statusText = "OK";
    response.headers["Content-Type"] = "application/json";
    
    try {
        json request = json::parse(body);
        std::string parentPath = request["parentPath"];
        std::string name = request["name"];
        
        std::cout << "创建文件: parentPath=" << parentPath << ", name=" << name << std::endl;
        
        std::string filePath = parentPath;
        if (filePath.back() != '/') {
            filePath += '/';
        }
        filePath += name;
        
        bool success = fileManager->createFile(filePath);
        
        std::cout << "创建文件结果: " << (success ? "成功" : "失败") << std::endl;
        
        json result;
        result["success"] = success;
        
        response.body = result.dump();
    } catch (const std::exception& e) {
        std::cout << "创建文件异常: " << e.what() << std::endl;
        json result;
        result["success"] = false;
        result["error"] = e.what();
        response.body = result.dump();
    }
    
    return response;
}

/**
 * 处理删除API
 */
HttpResponse WebServer::handleDelete(const std::string& path,
                                      const std::map<std::string, std::string>& headers,
                                      const std::string& body) {
    (void)path;
    (void)headers;
    (void)body;

    HttpResponse response;
    response.statusCode = 200;
    response.statusText = "OK";
    response.headers["Content-Type"] = "application/json";
    
    try {
        json request = json::parse(body);
        std::string itemPath = request["path"];
        
        bool success = fileManager->deleteItem(itemPath);
        
        json result;
        result["success"] = success;
        
        response.body = result.dump();
    } catch (const std::exception& e) {
        json result;
        result["success"] = false;
        result["error"] = e.what();
        response.body = result.dump();
    }
    
    return response;
}

/**
 * 处理重命名API
 */
HttpResponse WebServer::handleRename(const std::string& path,
                                      const std::map<std::string, std::string>& headers,
                                      const std::string& body) {
    (void)path;
    (void)headers;
    (void)body;

    HttpResponse response;
    response.statusCode = 200;
    response.statusText = "OK";
    response.headers["Content-Type"] = "application/json";
    
    try {
        json request = json::parse(body);
        std::string oldPath = request["oldPath"];
        std::string newPath = request["newPath"];
        
        bool success = fileManager->renameItem(oldPath, newPath);
        
        json result;
        result["success"] = success;
        
        response.body = result.dump();
    } catch (const std::exception& e) {
        json result;
        result["success"] = false;
        result["error"] = e.what();
        response.body = result.dump();
    }
    
    return response;
}

/**
 * 处理获取文件信息API
 */
HttpResponse WebServer::handleGetFileInfo(const std::string& path,
                                           const std::map<std::string, std::string>& headers,
                                           const std::string& body) {
    (void)headers;
    (void)body;

    HttpResponse response;
    response.statusCode = 200;
    response.statusText = "OK";
    response.headers["Content-Type"] = "application/json";
    
    // 解析查询参数
    size_t queryPos = path.find('?');
    std::string queryStr = (queryPos != std::string::npos) ? path.substr(queryPos + 1) : "";
    auto params = parseQueryString(queryStr);
    
    std::string filePath = params["path"];
    if (filePath.empty()) {
        json result;
        result["success"] = false;
        result["error"] = "Path parameter is required";
        response.body = result.dump();
        return response;
    }
    
    FileInfo info;
    bool success = fileManager->getFileInfo(filePath, info);
    
    json result;
    result["success"] = success;
    result["isBinary"] = info.isBinary;
    result["mimeType"] = info.mimeType;
    
    response.body = result.dump();
    
    return response;
}

/**
 * 处理保存所有文件API
 */
HttpResponse WebServer::handleSaveAll(const std::string& path,
                                       const std::map<std::string, std::string>& headers,
                                       const std::string& body) {
    (void)path;
    (void)headers;
    (void)body;

    HttpResponse response;
    response.statusCode = 200;
    response.statusText = "OK";
    response.headers["Content-Type"] = "application/json";
    
    try {
        json request = json::parse(body);
        json files = request["files"];
        
        bool allSuccess = true;
        
        for (const auto& file : files) {
            std::string filePath = file[0];
            std::string content = file[1];
            
            if (!fileManager->writeFile(filePath, content)) {
                allSuccess = false;
            }
        }
        
        json result;
        result["success"] = allSuccess;
        
        response.body = result.dump();
    } catch (const std::exception& e) {
        json result;
        result["success"] = false;
        result["error"] = e.what();
        response.body = result.dump();
    }
    
    return response;
}

/**
 * 处理刷新API
 */
HttpResponse WebServer::handleRefresh(const std::string& path,
                                       const std::map<std::string, std::string>& headers,
                                       const std::string& body) {
    (void)path;
    (void)headers;
    (void)body;

    HttpResponse response;
    response.statusCode = 200;
    response.statusText = "OK";
    response.headers["Content-Type"] = "application/json";
    
    json result;
    result["success"] = true;
    
    response.body = result.dump();
    
    return response;
}

/**
 * 处理静态文件请求
 */
HttpResponse WebServer::handleStaticFile(const std::string& path) {
    HttpResponse response;
    
    // 解析路径
    std::string filePath = path;
    
    // 移除查询字符串
    size_t queryPos = filePath.find('?');
    if (queryPos != std::string::npos) {
        filePath = filePath.substr(0, queryPos);
    }
    
    // 处理根路径
    if (filePath == "/" || filePath == "") {
        filePath = "/index.html";
    }
    
    // 构建完整的文件路径
    std::string fullPath = "web" + filePath;
    
    // 读取文件
    std::string content = readStaticFile(fullPath);
    
    if (content.empty()) {
        // 文件不存在
        response.statusCode = 404;
        response.statusText = "Not Found";
        response.headers["Content-Type"] = "text/plain";
        response.body = "404 Not Found";
        return response;
    }
    
    response.statusCode = 200;
    response.statusText = "OK";
    response.headers["Content-Type"] = getHttpMimeType(fullPath);
    response.body = content;
    
    return response;
}

/**
 * 读取静态文件内容
 */
std::string WebServer::readStaticFile(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    
    if (!file.is_open()) {
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    
    file.close();
    
    return buffer.str();
}

/**
 * 获取文件的MIME类型（用于HTTP响应）
 */
std::string WebServer::getHttpMimeType(const std::string& filePath) {
    std::string ext = filePath.substr(filePath.find_last_of('.') + 1);
    
    static const std::map<std::string, std::string> mimeTypes = {
        {"html", "text/html"},
        {"css", "text/css"},
        {"js", "application/javascript"},
        {"json", "application/json"},
        {"png", "image/png"},
        {"jpg", "image/jpeg"},
        {"jpeg", "image/jpeg"},
        {"gif", "image/gif"},
        {"svg", "image/svg+xml"},
        {"ico", "image/x-icon"},
        {"woff", "font/woff"},
        {"woff2", "font/woff2"},
        {"ttf", "font/ttf"},
        {"eot", "application/vnd.ms-fontobject"},
        {"mp4", "video/mp4"},
        {"mp3", "audio/mpeg"},
        {"wav", "audio/wav"}
    };
    
    auto it = mimeTypes.find(ext);
    if (it != mimeTypes.end()) {
        return it->second;
    }
    
    return "application/octet-stream";
}
