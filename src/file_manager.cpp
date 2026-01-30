/**
 * Mikufy v2.5 - 文件管理器实现
 * 负责所有文件系统操作
 */

#include "../headers/file_manager.h"
#include <algorithm>
#include <iostream>
#include <cerrno>

// 构造函数
FileManager::FileManager() : magicCookie(nullptr) {
    initMagic();
}

// 析构函数
FileManager::~FileManager() {
    cleanupMagic();
}

/**
 * 初始化libmagic
 * 用于检测文件类型
 */
bool FileManager::initMagic() {
    std::lock_guard<std::mutex> lock(mutex);
    
    // 打开libmagic数据库
    magicCookie = magic_open(MAGIC_MIME_TYPE);
    if (!magicCookie) {
        return false;
    }
    
    // 加载默认magic数据库
    if (magic_load(magicCookie, nullptr) != 0) {
        magic_close(magicCookie);
        magicCookie = nullptr;
        return false;
    }
    
    return true;
}

/**
 * 清理libmagic资源
 */
void FileManager::cleanupMagic() {
    std::lock_guard<std::mutex> lock(mutex);
    
    if (magicCookie) {
        magic_close(magicCookie);
        magicCookie = nullptr;
    }
}

/**
 * 获取目录内容
 * @param path 目录路径
 * @param files 输出文件列表
 * @return 是否成功
 */
bool FileManager::getDirectoryContents(const std::string& path, std::vector<FileInfo>& files) {
    std::lock_guard<std::mutex> lock(mutex);
    
    std::cout << "FileManager::getDirectoryContents 开始，路径: " << path << std::endl;
    
    DIR* dir = opendir(path.c_str());
    if (!dir) {
        std::cout << "无法打开目录: " << strerror(errno) << std::endl;
        return false;
    }
    
    std::cout << "成功打开目录，开始读取条目" << std::endl;
    
    struct dirent* entry;
    int count = 0;
    int totalEntries = 0;
    while ((entry = readdir(dir)) != nullptr) {
        totalEntries++;
        std::cout << "读取到条目 #" << totalEntries << ": " << entry->d_name << std::endl;
        
        // 跳过.和..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            std::cout << "  跳过 . 或 .." << std::endl;
            continue;
        }
        
        std::cout << "  创建FileInfo" << std::endl;
        FileInfo info;
        info.name = entry->d_name;
        info.path = path;
        if (info.path.back() != '/') {
            info.path += '/';
        }
        info.path += entry->d_name;
        
        std::cout << "  调用stat: " << info.path << std::endl;
        // 判断是否为目录
        struct stat statBuf;
        if (stat(info.path.c_str(), &statBuf) == 0) {
            info.isDirectory = S_ISDIR(statBuf.st_mode);
            info.size = statBuf.st_size;
            std::cout << "  stat成功，isDirectory=" << info.isDirectory << ", size=" << info.size << std::endl;
        } else {
            std::cout << "  stat失败: " << strerror(errno) << std::endl;
            info.isDirectory = (entry->d_type == DT_DIR);
            info.size = 0;
        }
        
        // 获取MIME类型
        if (!info.isDirectory) {
            std::cout << "  获取MIME类型" << std::endl;
            info.mimeType = getMimeType(info.path);
            info.isBinary = isBinaryFile(info.path);
            std::cout << "  MIME类型: " << info.mimeType << ", isBinary: " << info.isBinary << std::endl;
        } else {
            info.mimeType = "inode/directory";
            info.isBinary = false;
        }
        
        files.push_back(info);
        count++;
        std::cout << "  添加到files列表，当前count=" << count << std::endl;
        
        // 限制读取数量，防止卡死
        if (count >= 1000) {
            std::cout << "  达到读取上限1000，停止读取" << std::endl;
            break;
        }
    }
    
    std::cout << "关闭目录" << std::endl;
    closedir(dir);
    std::cout << "FileManager::getDirectoryContents 完成，找到 " << count << " 个条目" << std::endl;
    return true;
}

/**
 * 读取文件内容
 * @param path 文件路径
 * @param content 输出文件内容
 * @return 是否成功
 */
bool FileManager::readFile(const std::string& path, std::string& content) {
    std::lock_guard<std::mutex> lock(mutex);
    
    // 检查文件是否存在
    if (!isFile(path)) {
        return false;
    }
    
    // 检查是否为二进制文件
    if (isBinaryFile(path)) {
        return false;
    }
    
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    // 读取文件内容
    std::stringstream buffer;
    buffer << file.rdbuf();
    content = buffer.str();
    
    file.close();
    return true;
}

/**
 * 读取二进制文件内容
 * @param path 文件路径
 * @param content 输出文件内容
 * @return 是否成功
 */
bool FileManager::readFileBinary(const std::string& path, std::vector<char>& content) {
    std::lock_guard<std::mutex> lock(mutex);
    
    // 检查文件是否存在
    if (!isFile(path)) {
        return false;
    }
    
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    // 获取文件大小
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    
    // 读取文件内容
    content.resize(fileSize);
    file.read(content.data(), fileSize);
    
    file.close();
    return file.good();
}

/**
 * 写入文件内容
 * @param path 文件路径
 * @param content 文件内容
 * @return 是否成功
 */
bool FileManager::writeFile(const std::string& path, const std::string& content) {
    std::lock_guard<std::mutex> lock(mutex);
    
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        return false;
    }
    
    file << content;
    file.close();
    
    return file.good();
}

/**
 * 创建目录
 * @param path 目录路径
 * @return 是否成功
 */
bool FileManager::createDirectory(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex);
    
    // 使用mkdir创建目录，权限755
    if (mkdir(path.c_str(), 0755) == 0) {
        return true;
    }
    
    // 如果目录已存在，返回true
    return (errno == EEXIST);
}

/**
 * 创建空文件
 * @param path 文件路径
 * @return 是否成功
 */
bool FileManager::createFile(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex);
    
    // 创建文件，权限644
    int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        return false;
    }
    
    close(fd);
    return true;
}

/**
 * 删除文件或目录
 * @param path 路径
 * @return 是否成功
 */
bool FileManager::deleteItem(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex);
    
    if (!pathExists(path)) {
        return false;
    }
    
    if (isDirectory(path)) {
        return deleteDirectoryRecursive(path);
    } else {
        return (unlink(path.c_str()) == 0);
    }
}

/**
 * 递归删除目录
 * @param path 目录路径
 * @return 是否成功
 */
bool FileManager::deleteDirectoryRecursive(const std::string& path) {
    DIR* dir = opendir(path.c_str());
    if (!dir) {
        return false;
    }
    
    struct dirent* entry;
    bool success = true;
    
    while ((entry = readdir(dir)) != nullptr) {
        // 跳过.和..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        std::string itemPath = path;
        if (itemPath.back() != '/') {
            itemPath += '/';
        }
        itemPath += entry->d_name;
        
        struct stat statBuf;
        if (stat(itemPath.c_str(), &statBuf) == 0) {
            if (S_ISDIR(statBuf.st_mode)) {
                // 递归删除子目录
                if (!deleteDirectoryRecursive(itemPath)) {
                    success = false;
                }
            } else {
                // 删除文件
                if (unlink(itemPath.c_str()) != 0) {
                    success = false;
                }
            }
        }
    }
    
    closedir(dir);
    
    // 删除空目录
    if (rmdir(path.c_str()) != 0) {
        success = false;
    }
    
    return success;
}

/**
 * 重命名文件或目录
 * @param oldPath 原路径
 * @param newPath 新路径
 * @return 是否成功
 */
bool FileManager::renameItem(const std::string& oldPath, const std::string& newPath) {
    std::lock_guard<std::mutex> lock(mutex);
    
    return (rename(oldPath.c_str(), newPath.c_str()) == 0);
}

/**
 * 获取文件信息
 * @param path 文件路径
 * @param info 输出文件信息
 * @return 是否成功
 */
bool FileManager::getFileInfo(const std::string& path, FileInfo& info) {
    std::lock_guard<std::mutex> lock(mutex);
    
    struct stat statBuf;
    if (stat(path.c_str(), &statBuf) != 0) {
        return false;
    }
    
    info.name = getFileName(path);
    info.path = path;
    info.isDirectory = S_ISDIR(statBuf.st_mode);
    info.size = statBuf.st_size;
    
    if (!info.isDirectory) {
        info.mimeType = getMimeType(path);
        info.isBinary = isBinaryFile(path);
    } else {
        info.mimeType = "inode/directory";
        info.isBinary = false;
    }
    
    return true;
}

/**
 * 检查路径是否存在
 * @param path 路径
 * @return 是否存在
 */
bool FileManager::pathExists(const std::string& path) {
    struct stat statBuf;
    return (stat(path.c_str(), &statBuf) == 0);
}

/**
 * 检查是否为目录
 * @param path 路径
 * @return 是否为目录
 */
bool FileManager::isDirectory(const std::string& path) {
    struct stat statBuf;
    if (stat(path.c_str(), &statBuf) != 0) {
        return false;
    }
    return S_ISDIR(statBuf.st_mode);
}

/**
 * 检查是否为文件
 * @param path 路径
 * @return 是否为文件
 */
bool FileManager::isFile(const std::string& path) {
    struct stat statBuf;
    if (stat(path.c_str(), &statBuf) != 0) {
        return false;
    }
    return S_ISREG(statBuf.st_mode);
}

/**
 * 获取文件大小
 * @param path 文件路径
 * @return 文件大小（字节），失败返回0
 */
size_t FileManager::getFileSize(const std::string& path) {
    struct stat statBuf;
    if (stat(path.c_str(), &statBuf) != 0) {
        return 0;
    }
    return statBuf.st_size;
}

/**
 * 判断文件是否为二进制文件
 * @param path 文件路径
 * @return 是否为二进制文件
 */
bool FileManager::isBinaryFile(const std::string& path) {
    // 注意：不要在这里获取锁，因为调用者可能已经持有锁
    
    std::string mimeType = getMimeType(path);
    
    // 空文件不是二进制文件
    if (mimeType == "inode/x-empty" || mimeType == "inode/x-emptyfile") {
        return false;
    }
    
    // 检查MIME类型是否以text/开头
    if (mimeType.substr(0, 5) == "text/") {
        return false;
    }
    
    // 检查常见的文本MIME类型
    const std::vector<std::string> textMimeTypes = {
        "application/json",
        "application/xml",
        "application/javascript",
        "application/x-sh",
        "application/x-python",
        "application/x-perl",
        "application/x-ruby",
        "application/x-php",
        "application/x-c",
        "application/x-csrc",
        "application/x-c++",
        "application/x-c++src",
        "application/x-cpp",
        "application/x-h",
        "application/x-header",
        "application/x-httpd-php"
    };
    
    for (const auto& type : textMimeTypes) {
        if (mimeType == type) {
            return false;
        }
    }
    
    // 其他情况认为是二进制文件
    return true;
}

/**
 * 获取文件MIME类型
 * @param path 文件路径
 * @return MIME类型
 */
std::string FileManager::getMimeType(const std::string& path) {
    // 注意：不要在这里获取锁，因为调用者可能已经持有锁
    
    if (!magicCookie) {
        return "application/octet-stream";
    }
    
    const char* mimeType = magic_file(magicCookie, path.c_str());
    if (!mimeType) {
        return "application/octet-stream";
    }
    
    return std::string(mimeType);
}

/**
 * 标准化路径（去除多余的/和.）
 * @param path 路径
 * @return 标准化后的路径
 */
std::string FileManager::normalizePath(const std::string& path) {
    std::string normalized = path;
    
    // 替换多个连续的/为单个/
    size_t pos = 0;
    while ((pos = normalized.find("//", pos)) != std::string::npos) {
        normalized.replace(pos, 2, "/");
    }
    
    // 去除末尾的/
    while (!normalized.empty() && normalized.back() == '/' && normalized.size() > 1) {
        normalized.pop_back();
    }
    
    return normalized;
}

/**
 * 获取父目录路径
 * @param path 路径
 * @return 父目录路径
 */
std::string FileManager::getParentDirectory(const std::string& path) {
    size_t pos = path.find_last_of('/');
    if (pos == std::string::npos) {
        return ".";
    }
    
    if (pos == 0) {
        return "/";
    }
    
    return path.substr(0, pos);
}

/**
 * 获取文件名
 * @param path 路径
 * @return 文件名
 */
std::string FileManager::getFileName(const std::string& path) {
    size_t pos = path.find_last_of('/');
    if (pos == std::string::npos) {
        return path;
    }
    
    return path.substr(pos + 1);
}

/**
 * 获取文件扩展名
 * @param filename 文件名
 * @return 扩展名（包含点）
 */
std::string FileManager::getFileExtension(const std::string& filename) {
    size_t pos = filename.find_last_of('.');
    if (pos == std::string::npos) {
        return "";
    }
    
    return filename.substr(pos);
}