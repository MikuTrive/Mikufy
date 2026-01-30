/**
 * Mikufy v2.5 - 文件管理器头文件
 * 负责所有文件系统操作
 */

#ifndef MIKUFY_FILE_MANAGER_H
#define MIKUFY_FILE_MANAGER_H

#include "main.h"
#include <fstream>
#include <sstream>

/**
 * 文件管理器类
 * 处理所有文件系统相关操作，包括读取、写入、删除、重命名等
 */
class FileManager {
public:
    FileManager();
    ~FileManager();
    
    // 禁止拷贝和移动
    FileManager(const FileManager&) = delete;
    FileManager& operator=(const FileManager&) = delete;
    FileManager(FileManager&&) = delete;
    FileManager& operator=(FileManager&&) = delete;
    
    /**
     * 获取目录内容
     * @param path 目录路径
     * @param files 输出文件列表
     * @return 是否成功
     */
    bool getDirectoryContents(const std::string& path, std::vector<FileInfo>& files);
    
    /**
     * 读取文件内容
     * @param path 文件路径
     * @param content 输出文件内容
     * @return 是否成功
     */
    bool readFile(const std::string& path, std::string& content);
    
    /**
     * 读取二进制文件内容
     * @param path 文件路径
     * @param content 输出文件内容
     * @return 是否成功
     */
    bool readFileBinary(const std::string& path, std::vector<char>& content);
    
    /**
     * 写入文件内容
     * @param path 文件路径
     * @param content 文件内容
     * @return 是否成功
     */
    bool writeFile(const std::string& path, const std::string& content);
    
    /**
     * 创建目录
     * @param path 目录路径
     * @return 是否成功
     */
    bool createDirectory(const std::string& path);
    
    /**
     * 创建空文件
     * @param path 文件路径
     * @return 是否成功
     */
    bool createFile(const std::string& path);
    
    /**
     * 删除文件或目录
     * @param path 路径
     * @return 是否成功
     */
    bool deleteItem(const std::string& path);
    
    /**
     * 重命名文件或目录
     * @param oldPath 原路径
     * @param newPath 新路径
     * @return 是否成功
     */
    bool renameItem(const std::string& oldPath, const std::string& newPath);
    
    /**
     * 获取文件信息
     * @param path 文件路径
     * @param info 输出文件信息
     * @return 是否成功
     */
    bool getFileInfo(const std::string& path, FileInfo& info);
    
    /**
     * 检查路径是否存在
     * @param path 路径
     * @return 是否存在
     */
    bool pathExists(const std::string& path);
    
    /**
     * 检查是否为目录
     * @param path 路径
     * @return 是否为目录
     */
    bool isDirectory(const std::string& path);
    
    /**
     * 检查是否为文件
     * @param path 路径
     * @return 是否为文件
     */
    bool isFile(const std::string& path);
    
    /**
     * 获取文件大小
     * @param path 文件路径
     * @return 文件大小（字节），失败返回0
     */
    size_t getFileSize(const std::string& path);
    
    /**
     * 检查文件是否为二进制文件
     * @param path 文件路径
     * @return 是否为二进制文件
     */
    bool isBinaryFile(const std::string& path);
    
    /**
     * 获取文件MIME类型
     * @param path 文件路径
     * @return MIME类型
     */
    std::string getMimeType(const std::string& path);
    
    /**
     * 递归删除目录
     * @param path 目录路径
     * @return 是否成功
     */
    bool deleteDirectoryRecursive(const std::string& path);
    
    /**
     * 标准化路径（去除多余的/和.）
     * @param path 路径
     * @return 标准化后的路径
     */
    std::string normalizePath(const std::string& path);
    
    /**
     * 获取父目录路径
     * @param path 路径
     * @return 父目录路径
     */
    std::string getParentDirectory(const std::string& path);
    
    /**
     * 获取文件名
     * @param path 路径
     * @return 文件名
     */
    std::string getFileName(const std::string& path);
    
    /**
     * 获取文件扩展名
     * @param filename 文件名
     * @return 扩展名（包含点）
     */
    std::string getFileExtension(const std::string& filename);

private:
    magic_t magicCookie; // libmagic句柄，用于文件类型检测
    std::mutex mutex; // 互斥锁，保证线程安全
    
    /**
     * 初始化libmagic
     */
    bool initMagic();
    
    /**
     * 清理libmagic资源
     */
    void cleanupMagic();
};

#endif // MIKUFY_FILE_MANAGER_H