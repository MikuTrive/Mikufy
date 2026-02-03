/* Mikufy v2.2(stable) - 文件管理器实现
 * 负责所有文件系统操作
 */

#include "../headers/file_manager.h"
#include <algorithm>
#include <iostream>
#include <cerrno>

/* 构造函数 */
FileManager::FileManager() : magic_cookie(nullptr)
{
	init_magic();
}

/* 析构函数 */
FileManager::~FileManager(void)
{
	cleanup_magic();
}

/* 初始化libmagic
 * 用于检测文件类型
 */
bool FileManager::init_magic(void)
{
	std::lock_guard<std::mutex> lock(mutex);

	/* 打开libmagic数据库 */
	magic_cookie = magic_open(MAGIC_MIME_TYPE);
	if (!magic_cookie)
		return false;

	/* 加载默认magic数据库 */
	if (magic_load(magic_cookie, nullptr) != 0) {
		magic_close(magic_cookie);
		magic_cookie = nullptr;
		return false;
	}

	return true;
}

/* 清理libmagic资源 */
void FileManager::cleanup_magic(void)
{
	std::lock_guard<std::mutex> lock(mutex);

	if (magic_cookie) {
		magic_close(magic_cookie);
		magic_cookie = nullptr;
	}
}

/* 获取目录内容 */
bool FileManager::get_directory_contents(const std::string &path,
					  std::vector<FileInfo> &files)
{
	std::lock_guard<std::mutex> lock(mutex);

	std::cout << "FileManager::get_directory_contents 开始，路径: "
		  << path << std::endl;

	DIR *dir = opendir(path.c_str());
	if (!dir) {
		std::cout << "无法打开目录: " << strerror(errno) << std::endl;
		return false;
	}

	std::cout << "成功打开目录，开始读取条目" << std::endl;

	struct dirent *entry;
	int count = 0;
	int total_entries = 0;

	while ((entry = readdir(dir)) != nullptr) {
		total_entries++;
		std::cout << "读取到条目 #" << total_entries << ": "
			  << entry->d_name << std::endl;

		/* 跳过.和.. */
		if (strcmp(entry->d_name, ".") == 0 ||
		    strcmp(entry->d_name, "..") == 0) {
			std::cout << "  跳过 . 或 .." << std::endl;
			continue;
		}

		std::cout << "  创建FileInfo" << std::endl;
		FileInfo info;
		info.name = entry->d_name;
		info.path = path;
		if (info.path.back() != '/')
			info.path += '/';
		info.path += entry->d_name;

		std::cout << "  调用stat: " << info.path << std::endl;
		/* 判断是否为目录 */
		struct stat stat_buf;
		if (stat(info.path.c_str(), &stat_buf) == 0) {
			info.is_directory = S_ISDIR(stat_buf.st_mode);
			info.size = stat_buf.st_size;
			std::cout << "  stat成功，is_directory="
				  << info.is_directory << ", size="
				  << info.size << std::endl;
		} else {
			std::cout << "  stat失败: " << strerror(errno)
				  << std::endl;
			info.is_directory = (entry->d_type == DT_DIR);
			info.size = 0;
		}

		/* 获取MIME类型 */
		if (!info.is_directory) {
			std::cout << "  获取MIME类型" << std::endl;
			info.mime_type = get_mime_type(info.path);
			info.is_binary = is_binary_file(info.path);
			std::cout << "  MIME类型: " << info.mime_type
				  << ", is_binary: " << info.is_binary
				  << std::endl;
		} else {
			info.mime_type = "inode/directory";
			info.is_binary = false;
		}

		files.push_back(info);
		count++;
		std::cout << "  添加到files列表，当前count=" << count
			  << std::endl;

		/* 限制读取数量，防止卡死 */
		if (count >= 1000) {
			std::cout << "  达到读取上限1000，停止读取"
				  << std::endl;
			break;
		}
	}

	std::cout << "关闭目录" << std::endl;
	closedir(dir);
	std::cout << "FileManager::get_directory_contents 完成，找到 "
		  << count << " 个条目" << std::endl;
	return true;
}

/* 读取文件内容 */
bool FileManager::read_file(const std::string &path, std::string &content)
{
	std::lock_guard<std::mutex> lock(mutex);

	/* 检查文件是否存在 */
	if (!is_file(path))
		return false;

	/* 检查是否为二进制文件 */
	if (is_binary_file(path))
		return false;

	std::ifstream file(path, std::ios::binary);
	if (!file.is_open())
		return false;

	/* 读取文件内容 */
	std::stringstream buffer;
	buffer << file.rdbuf();
	content = buffer.str();

	file.close();
	return true;
}

/* 读取二进制文件内容 */
bool FileManager::read_file_binary(const std::string &path,
				   std::vector<char> &content)
{
	std::lock_guard<std::mutex> lock(mutex);

	/* 检查文件是否存在 */
	if (!is_file(path))
		return false;

	std::ifstream file(path, std::ios::binary);
	if (!file.is_open())
		return false;

	/* 获取文件大小 */
	file.seekg(0, std::ios::end);
	size_t file_size = file.tellg();
	file.seekg(0, std::ios::beg);

	/* 读取文件内容 */
	content.resize(file_size);
	file.read(content.data(), file_size);

	file.close();
	return file.good();
}

/* 写入文件内容 */
bool FileManager::write_file(const std::string &path,
			     const std::string &content)
{
	std::lock_guard<std::mutex> lock(mutex);

	std::ofstream file(path, std::ios::binary | std::ios::trunc);
	if (!file.is_open())
		return false;

	file << content;
	file.close();

	return file.good();
}

/* 创建目录 */
bool FileManager::create_directory(const std::string &path)
{
	std::lock_guard<std::mutex> lock(mutex);

	/* 使用mkdir创建目录，权限755 */
	if (mkdir(path.c_str(), 0755) == 0)
		return true;

	/* 如果目录已存在，返回true */
	return (errno == EEXIST);
}

/* 创建空文件 */
bool FileManager::create_file(const std::string &path)
{
	std::lock_guard<std::mutex> lock(mutex);

	/* 创建文件，权限644 */
	int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd == -1)
		return false;

	close(fd);
	return true;
}

/* 删除文件或目录 */
bool FileManager::delete_item(const std::string &path)
{
	std::lock_guard<std::mutex> lock(mutex);

	if (!path_exists(path))
		return false;

	if (is_directory(path))
		return delete_directory_recursive(path);
	else
		return (unlink(path.c_str()) == 0);
}

/* 递归删除目录 */
bool FileManager::delete_directory_recursive(const std::string &path)
{
	DIR *dir = opendir(path.c_str());
	if (!dir)
		return false;

	struct dirent *entry;
	bool success = true;

	while ((entry = readdir(dir)) != nullptr) {
		/* 跳过.和.. */
		if (strcmp(entry->d_name, ".") == 0 ||
		    strcmp(entry->d_name, "..") == 0)
			continue;

		std::string item_path = path;
		if (item_path.back() != '/')
			item_path += '/';
		item_path += entry->d_name;

		struct stat stat_buf;
		if (stat(item_path.c_str(), &stat_buf) == 0) {
			if (S_ISDIR(stat_buf.st_mode)) {
				/* 递归删除子目录 */
				if (!delete_directory_recursive(item_path))
					success = false;
			} else {
				/* 删除文件 */
				if (unlink(item_path.c_str()) != 0)
					success = false;
			}
		}
	}

	closedir(dir);

	/* 删除空目录 */
	if (rmdir(path.c_str()) != 0)
		success = false;

	return success;
}

/* 重命名文件或目录 */
bool FileManager::rename_item(const std::string &old_path,
			      const std::string &new_path)
{
	std::lock_guard<std::mutex> lock(mutex);

	return (rename(old_path.c_str(), new_path.c_str()) == 0);
}

/* 获取文件信息 */
bool FileManager::get_file_info(const std::string &path, FileInfo &info)
{
	std::lock_guard<std::mutex> lock(mutex);

	struct stat stat_buf;
	if (stat(path.c_str(), &stat_buf) != 0)
		return false;

	info.name = get_file_name(path);
	info.path = path;
	info.is_directory = S_ISDIR(stat_buf.st_mode);
	info.size = stat_buf.st_size;

	if (!info.is_directory) {
		info.mime_type = get_mime_type(path);
		info.is_binary = is_binary_file(path);
	} else {
		info.mime_type = "inode/directory";
		info.is_binary = false;
	}

	return true;
}

/* 检查路径是否存在 */
bool FileManager::path_exists(const std::string &path)
{
	struct stat stat_buf;
	return (stat(path.c_str(), &stat_buf) == 0);
}

/* 检查是否为目录 */
bool FileManager::is_directory(const std::string &path)
{
	struct stat stat_buf;
	if (stat(path.c_str(), &stat_buf) != 0)
		return false;
	return S_ISDIR(stat_buf.st_mode);
}

/* 检查是否为文件 */
bool FileManager::is_file(const std::string &path)
{
	struct stat stat_buf;
	if (stat(path.c_str(), &stat_buf) != 0)
		return false;
	return S_ISREG(stat_buf.st_mode);
}

/* 获取文件大小 */
size_t FileManager::get_file_size(const std::string &path)
{
	struct stat stat_buf;
	if (stat(path.c_str(), &stat_buf) != 0)
		return 0;
	return stat_buf.st_size;
}

/* 判断文件是否为二进制文件 */
bool FileManager::is_binary_file(const std::string &path)
{
	/* 注意：不要在这里获取锁，因为调用者可能已经持有锁 */

	std::string mime_type = get_mime_type(path);

	/* 空文件不是二进制文件 */
	if (mime_type == "inode/x-empty" ||
	    mime_type == "inode/x-emptyfile")
		return false;

	/* 检查MIME类型是否以text/开头 */
	if (mime_type.substr(0, 5) == "text/")
		return false;

	/* 检查常见的文本MIME类型 */
	const std::vector<std::string> text_mime_types = {
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

	for (const auto &type : text_mime_types) {
		if (mime_type == type)
			return false;
	}

	/* 其他情况认为是二进制文件 */
	return true;
}

/* 获取文件MIME类型 */
std::string FileManager::get_mime_type(const std::string &path)
{
	/* 注意：不要在这里获取锁，因为调用者可能已经持有锁 */

	if (!magic_cookie)
		return "application/octet-stream";

	const char *mime_type = magic_file(magic_cookie, path.c_str());
	if (!mime_type)
		return "application/octet-stream";

	return std::string(mime_type);
}

/* 标准化路径（去除多余的/和.） */
std::string FileManager::normalize_path(const std::string &path)
{
	std::string normalized = path;

	/* 替换多个连续的/为单个/ */
	size_t pos = 0;
	while ((pos = normalized.find("//", pos)) != std::string::npos)
		normalized.replace(pos, 2, "/");

	/* 去除末尾的/ */
	while (!normalized.empty() && normalized.back() == '/' &&
	       normalized.size() > 1)
		normalized.pop_back();

	return normalized;
}

/* 获取父目录路径 */
std::string FileManager::get_parent_directory(const std::string &path)
{
	size_t pos = path.find_last_of('/');
	if (pos == std::string::npos)
		return ".";

	if (pos == 0)
		return "/";

	return path.substr(0, pos);
}

/* 获取文件名 */
std::string FileManager::get_file_name(const std::string &path)
{
	size_t pos = path.find_last_of('/');
	if (pos == std::string::npos)
		return path;

	return path.substr(pos + 1);
}

/* 获取文件扩展名 */
std::string FileManager::get_file_extension(const std::string &filename)
{
	size_t pos = filename.find_last_of('.');
	if (pos == std::string::npos)
		return "";

	return filename.substr(pos);
}
