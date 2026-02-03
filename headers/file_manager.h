/* Mikufy v2.2(stable) - 文件管理器头文件
 * 负责所有文件系统操作
 */

#ifndef MIKUFY_FILE_MANAGER_H
#define MIKUFY_FILE_MANAGER_H

#include "main.h"
#include <fstream>
#include <sstream>

/* 文件管理器类
 * 处理所有文件系统相关操作，包括读取、写入、删除、重命名等
 */
class FileManager
{
public:
	FileManager();
	~FileManager();

	/* 禁止拷贝和移动 */
	FileManager(const FileManager &) = delete;
	FileManager &operator=(const FileManager &) = delete;
	FileManager(FileManager &&) = delete;
	FileManager &operator=(FileManager &&) = delete;

	/* 获取目录内容 */
	bool get_directory_contents(const std::string &path,
				    std::vector<FileInfo> &files);

	/* 读取文件内容 */
	bool read_file(const std::string &path, std::string &content);

	/* 读取二进制文件内容 */
	bool read_file_binary(const std::string &path,
			      std::vector<char> &content);

	/* 写入文件内容 */
	bool write_file(const std::string &path, const std::string &content);

	/* 创建目录 */
	bool create_directory(const std::string &path);

	/* 创建空文件 */
	bool create_file(const std::string &path);

	/* 删除文件或目录 */
	bool delete_item(const std::string &path);

	/* 重命名文件或目录 */
	bool rename_item(const std::string &oldPath,
			 const std::string &newPath);

	/* 获取文件信息 */
	bool get_file_info(const std::string &path, FileInfo &info);

	/* 检查路径是否存在 */
	bool path_exists(const std::string &path);

	/* 检查是否为目录 */
	bool is_directory(const std::string &path);

	/* 检查是否为文件 */
	bool is_file(const std::string &path);

	/* 获取文件大小 */
	size_t get_file_size(const std::string &path);

	/* 检查文件是否为二进制文件 */
	bool is_binary_file(const std::string &path);

	/* 获取文件MIME类型 */
	std::string get_mime_type(const std::string &path);

	/* 递归删除目录 */
	bool delete_directory_recursive(const std::string &path);

	/* 标准化路径（去除多余的/和.） */
	std::string normalize_path(const std::string &path);

	/* 获取父目录路径 */
	std::string get_parent_directory(const std::string &path);

	/* 获取文件名 */
	std::string get_file_name(const std::string &path);

	/* 获取文件扩展名 */
	std::string get_file_extension(const std::string &filename);

private:
	magic_t magic_cookie;	/* libmagic句柄，用于文件类型检测 */
	std::mutex mutex;	/* 互斥锁，保证线程安全 */

	/* 初始化libmagic */
	bool init_magic(void);

	/* 清理libmagic资源 */
	void cleanup_magic(void);
};

#endif /* MIKUFY_FILE_MANAGER_H */
