// SPDX-License-Identifier: GPL-2.0
/*
 * Mikufy v2.3(stable) - 文件管理器实现
 *
 * 本文件实现了FileManager类的所有方法，提供完整的文件系统操作功能。
 * FileManager封装了底层的文件操作API，提供线程安全的接口，并使用
 * libmagic库进行文件类型检测。
 *
 * 主要功能:
 * - 目录遍历和内容读取
 * - 文本文件和二进制文件的读取
 * - 文件写入和创建
 * - 文件和目录的删除（支持递归）
 * - 文件重命名
 * - 文件信息查询
 * - 路径处理和标准化
 * - MIME类型检测
 *
 * 2024 MiraTrive/MikuTrive
 * Author: [Your Name]
 *
 * 本文件遵循Linux内核代码风格规范
 */

/*
 * ============================================================================
 * 头文件包含
 * ============================================================================
 */

#include "../headers/file_manager.h"
#include <algorithm>		/* std::sort, std::find */
#include <iostream>		/* std::cout, std::cerr */
#include <cerrno>		/* errno, strerror() */

/*
 * ============================================================================
 * 构造函数和析构函数
 * ============================================================================
 */

/**
 * FileManager - 构造函数
 *
 * 初始化FileManager对象，设置libmagic句柄为nullptr，并调用
 * init_magic()方法初始化libmagic库。
 */
FileManager::FileManager() : magic_cookie(nullptr)
{
	init_magic();
}

/**
 * ~FileManager - 析构函数
 *
 * 清理FileManager对象，调用cleanup_magic()方法释放libmagic资源。
 */
FileManager::~FileManager(void)
{
	cleanup_magic();
}

/*
 * ============================================================================
 * 私有方法 - libmagic管理
 * ============================================================================
 */

/**
 * init_magic - 初始化libmagic库
 *
 * 打开libmagic数据库并加载默认的magic文件。libmagic用于检测
 * 文件的MIME类型，判断文件是文本文件还是二进制文件。
 *
 * 返回值: 初始化成功返回true，失败返回false
 *
 * 注意: 该方法持有互斥锁，确保线程安全。
 */
bool FileManager::init_magic(void)
{
	std::lock_guard<std::mutex> lock(mutex);

	/*
	 * 打开libmagic数据库
	 * MAGIC_MIME_TYPE标志表示只返回MIME类型字符串
	 */
	magic_cookie = magic_open(MAGIC_MIME_TYPE);
	if (!magic_cookie)
		return false;

	/*
	 * 加载默认magic数据库
	 * nullptr表示使用系统默认的magic文件位置
	 */
	if (magic_load(magic_cookie, nullptr) != 0) {
		/* 加载失败，清理资源 */
		magic_close(magic_cookie);
		magic_cookie = nullptr;
		return false;
	}

	return true;
}

/**
 * cleanup_magic - 清理libmagic资源
 *
 * 关闭libmagic句柄并释放相关资源。
 *
 * 注意: 该方法持有互斥锁，确保线程安全。
 */
void FileManager::cleanup_magic(void)
{
	std::lock_guard<std::mutex> lock(mutex);

	if (magic_cookie) {
		magic_close(magic_cookie);
		magic_cookie = nullptr;
	}
}

/*
 * ============================================================================
 * 公共方法实现 - 目录操作
 * ============================================================================
 */

/**
 * get_directory_contents - 获取目录内容
 *
 * 读取指定目录下的所有文件和子目录，并返回它们的详细信息。
 * 该方法会跳过"."和".."特殊目录，并限制最多读取1000个条目
 * 以防止在大目录中卡死。
 *
 * @path: 要读取的目录路径
 * @files: 输出参数，用于存储文件信息列表的引用
 *
 * 返回值: 成功返回true，失败返回false
 *
 * 注意: 该方法持有互斥锁，确保线程安全。
 */
bool FileManager::get_directory_contents(const std::string &path,
					  std::vector<FileInfo> &files)
{
	std::lock_guard<std::mutex> lock(mutex);

	std::cout << "FileManager::get_directory_contents 开始，路径: "
		  << path << std::endl;

	/* 打开目录 */
	DIR *dir = opendir(path.c_str());
	if (!dir) {
		std::cout << "无法打开目录: " << strerror(errno) << std::endl;
		return false;
	}

	std::cout << "成功打开目录，开始读取条目" << std::endl;

	struct dirent *entry;
	int count = 0;
	int total_entries = 0;

	/* 遍历目录中的所有条目 */
	while ((entry = readdir(dir)) != nullptr) {
		total_entries++;
		std::cout << "读取到条目 #" << total_entries << ": "
			  << entry->d_name << std::endl;

		/*
		 * 跳过当前目录(.)和父目录(..)
		 * 这两个特殊目录不应该显示在文件列表中
		 */
		if (strcmp(entry->d_name, ".") == 0 ||
		    strcmp(entry->d_name, "..") == 0) {
			std::cout << "  跳过 . 或 .." << std::endl;
			continue;
		}

		std::cout << "  创建FileInfo" << std::endl;
		FileInfo info;

		/* 设置文件名 */
		info.name = entry->d_name;

		/* 构建完整路径 */
		info.path = path;
		if (info.path.back() != '/')
			info.path += '/';
		info.path += entry->d_name;

		std::cout << "  调用stat: " << info.path << std::endl;

		/*
		 * 获取文件状态信息
		 * 使用stat()系统调用获取文件的详细属性
		 */
		struct stat stat_buf;
		if (stat(info.path.c_str(), &stat_buf) == 0) {
			/* 判断是否为目录 */
			info.is_directory = S_ISDIR(stat_buf.st_mode);
			/* 获取文件大小 */
			info.size = stat_buf.st_size;
			std::cout << "  stat成功，is_directory="
				  << info.is_directory << ", size="
				  << info.size << std::endl;
		} else {
			/*
			 * stat()失败，使用dirent中的类型信息
			 * 这在某些文件系统中可能不准确
			 */
			std::cout << "  stat失败: " << strerror(errno)
				  << std::endl;
			info.is_directory = (entry->d_type == DT_DIR);
			info.size = 0;
		}

		/*
		 * 获取MIME类型和二进制标志
		 * 仅对文件进行类型检测，目录不处理
		 */
		if (!info.is_directory) {
			std::cout << "  获取MIME类型" << std::endl;
			info.mime_type = get_mime_type(info.path);
			info.is_binary = is_binary_file(info.path);
			std::cout << "  MIME类型: " << info.mime_type
				  << ", is_binary: " << info.is_binary
				  << std::endl;
		} else {
			/* 目录的MIME类型固定为inode/directory */
			info.mime_type = "inode/directory";
			info.is_binary = false;
		}

		/* 将FileInfo添加到结果列表 */
		files.push_back(info);
		count++;
		std::cout << "  添加到files列表，当前count=" << count
			  << std::endl;

		/*
		 * 限制读取数量，防止在超大目录中卡死
		 * 1000个条目是一个合理的上限
		 */
		if (count >= 1000) {
			std::cout << "  达到读取上限1000，停止读取"
				  << std::endl;
			break;
		}
	}

	/* 关闭目录句柄 */
	std::cout << "关闭目录" << std::endl;
	closedir(dir);
	std::cout << "FileManager::get_directory_contents 完成，找到 "
		  << count << " 个条目" << std::endl;

	return true;
}

/*
 * ============================================================================
 * 公共方法实现 - 文件读取
 * ============================================================================
 */

/**
 * read_file - 读取文本文件内容
 *
 * 读取指定文件的内容并返回为字符串。该方法仅适用于文本文件，
 * 对于二进制文件会返回失败。文件内容以字符串形式返回。
 *
 * @path: 要读取的文件路径
 * @content: 输出参数，用于存储文件内容的引用
 *
 * 返回值: 成功返回true，失败返回false
 *
 * 注意: 该方法持有互斥锁，确保线程安全。
 */
bool FileManager::read_file(const std::string &path, std::string &content)
{
	std::lock_guard<std::mutex> lock(mutex);

	/* 检查文件是否存在 */
	if (!is_file(path))
		return false;

	/* 检查是否为二进制文件 */
	if (is_binary_file(path))
		return false;

	/* 打开文件（二进制模式以保留原始编码） */
	std::ifstream file(path, std::ios::binary);
	if (!file.is_open())
		return false;

	/* 读取文件内容到字符串流 */
	std::stringstream buffer;
	buffer << file.rdbuf();
	content = buffer.str();

	/* 关闭文件 */
	file.close();

	return true;
}

/**
 * read_file_binary - 读取二进制文件内容
 *
 * 读取指定文件的内容并返回为字节数组。该方法适用于所有文件类型，
 * 包括文本文件和二进制文件。文件内容以字符向量形式返回。
 *
 * @path: 要读取的文件路径
 * @content: 输出参数，用于存储文件内容的字符向量引用
 *
 * 返回值: 成功返回true，失败返回false
 *
 * 注意: 该方法持有互斥锁，确保线程安全。
 */
bool FileManager::read_file_binary(const std::string &path,
				   std::vector<char> &content)
{
	std::lock_guard<std::mutex> lock(mutex);

	/* 检查文件是否存在 */
	if (!is_file(path))
		return false;

	/* 打开文件（二进制模式） */
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

	/* 关闭文件 */
	file.close();

	return file.good();
}

/*
 * ============================================================================
 * 公共方法实现 - 文件写入
 * ============================================================================
 */

/**
 * write_file - 写入文件内容
 *
 * 将字符串内容写入指定文件。如果文件已存在，其内容将被覆盖；
 * 如果文件不存在，将创建新文件。写入使用二进制模式。
 *
 * @path: 要写入的文件路径
 * @content: 要写入的文件内容字符串
 *
 * 返回值: 成功返回true，失败返回false
 *
 * 注意: 该方法持有互斥锁，确保线程安全。
 */
bool FileManager::write_file(const std::string &path,
			     const std::string &content)
{
	std::lock_guard<std::mutex> lock(mutex);

	/*
	 * 打开文件（二进制模式、截断模式）
	 * std::ios::trunc表示如果文件存在，先清空内容
	 */
	std::ofstream file(path, std::ios::binary | std::ios::trunc);
	if (!file.is_open())
		return false;

	/* 写入内容 */
	file << content;

	/* 关闭文件 */
	file.close();

	return file.good();
}

/*
 * ============================================================================
 * 公共方法实现 - 创建操作
 * ============================================================================
 */

/**
 * create_directory - 创建目录
 *
 * 在指定路径创建新目录。如果目录已存在，该方法返回成功。
 * 目录权限设置为755（所有者可读写执行，组和其他用户可读执行）。
 *
 * @path: 要创建的目录路径
 *
 * 返回值: 成功返回true，失败返回false
 *
 * 注意: 该方法持有互斥锁，确保线程安全。
 */
bool FileManager::create_directory(const std::string &path)
{
	std::lock_guard<std::mutex> lock(mutex);

	/*
	 * 使用mkdir系统调用创建目录
	 * 权限0755: rwxr-xr-x
	 */
	if (mkdir(path.c_str(), 0755) == 0)
		return true;

	/*
	 * 如果目录已存在，返回true
	 * EEXIST表示文件已存在
	 */
	return (errno == EEXIST);
}

/**
 * create_file - 创建空文件
 *
 * 在指定路径创建空文件。如果文件已存在，其内容将被清空。
 * 文件权限设置为644（所有者可读写，组和其他用户只读）。
 *
 * @path: 要创建的文件路径
 *
 * 返回值: 成功返回true，失败返回false
 *
 * 注意: 该方法持有互斥锁，确保线程安全。
 */
bool FileManager::create_file(const std::string &path)
{
	std::lock_guard<std::mutex> lock(mutex);

	/*
	 * 使用open系统调用创建文件
	 * O_WRONLY: 只写模式
	 * O_CREAT: 如果文件不存在则创建
	 * O_TRUNC: 如果文件存在则截断
	 * 权限0644: rw-r--r--
	 */
	int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd == -1)
		return false;

	/* 关闭文件描述符 */
	close(fd);

	return true;
}

/*
 * ============================================================================
 * 公共方法实现 - 删除操作
 * ============================================================================
 */

/**
 * delete_item - 删除文件或目录
 *
 * 删除指定的文件或目录。如果是目录，将递归删除其所有内容。
 *
 * @path: 要删除的文件或目录路径
 *
 * 返回值: 成功返回true，失败返回false
 *
 * 注意: 该方法持有互斥锁，确保线程安全。
 */
bool FileManager::delete_item(const std::string &path)
{
	std::lock_guard<std::mutex> lock(mutex);

	/* 检查路径是否存在 */
	if (!path_exists(path))
		return false;

	/* 根据类型选择删除方法 */
	if (is_directory(path))
		return delete_directory_recursive(path);
	else
		return (unlink(path.c_str()) == 0);
}

/**
 * delete_directory_recursive - 递归删除目录
 *
 * 递归删除指定目录及其所有子目录和文件。该方法会先删除
 * 目录中的所有内容，最后删除目录本身。
 *
 * @path: 要删除的目录路径
 *
 * 返回值: 成功返回true，失败返回false
 *
 * 注意: 这是一个危险操作，删除的内容无法恢复。
 */
bool FileManager::delete_directory_recursive(const std::string &path)
{
	/* 打开目录 */
	DIR *dir = opendir(path.c_str());
	if (!dir)
		return false;

	struct dirent *entry;
	bool success = true;

	/* 遍历目录中的所有条目 */
	while ((entry = readdir(dir)) != nullptr) {
		/*
		 * 跳过当前目录(.)和父目录(..)
		 * 必须跳过这两个，否则会导致无限递归
		 */
		if (strcmp(entry->d_name, ".") == 0 ||
		    strcmp(entry->d_name, "..") == 0)
			continue;

		/* 构建完整路径 */
		std::string item_path = path;
		if (item_path.back() != '/')
			item_path += '/';
		item_path += entry->d_name;

		/* 获取文件状态 */
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

	/* 关闭目录句柄 */
	closedir(dir);

	/* 删除空目录 */
	if (rmdir(path.c_str()) != 0)
		success = false;

	return success;
}

/*
 * ============================================================================
 * 公共方法实现 - 重命名操作
 * ============================================================================
 */

/**
 * rename_item - 重命名文件或目录
 *
 * 将文件或目录从旧路径移动到新路径。
 *
 * @old_path: 原始文件或目录路径
 * @new_path: 新的文件或目录路径
 *
 * 返回值: 成功返回true，失败返回false
 *
 * 注意: 该方法持有互斥锁，确保线程安全。
 */
bool FileManager::rename_item(const std::string &old_path,
			      const std::string &new_path)
{
	std::lock_guard<std::mutex> lock(mutex);

	return (rename(old_path.c_str(), new_path.c_str()) == 0);
}

/*
 * ============================================================================
 * 公共方法实现 - 文件信息查询
 * ============================================================================
 */

/**
 * get_file_info - 获取文件信息
 *
 * 查询指定文件或目录的详细信息，包括名称、路径、大小、类型、
 * MIME类型等。
 *
 * @path: 要查询的文件或目录路径
 * @info: 输出参数，用于存储文件信息的引用
 *
 * 返回值: 成功返回true，失败返回false
 *
 * 注意: 该方法持有互斥锁，确保线程安全。
 */
bool FileManager::get_file_info(const std::string &path, FileInfo &info)
{
	std::lock_guard<std::mutex> lock(mutex);

	/* 获取文件状态 */
	struct stat stat_buf;
	if (stat(path.c_str(), &stat_buf) != 0)
		return false;

	/* 填充基本信息 */
	info.name = get_file_name(path);
	info.path = path;
	info.is_directory = S_ISDIR(stat_buf.st_mode);
	info.size = stat_buf.st_size;

	/* 填充MIME类型信息 */
	if (!info.is_directory) {
		info.mime_type = get_mime_type(path);
		info.is_binary = is_binary_file(path);
	} else {
		info.mime_type = "inode/directory";
		info.is_binary = false;
	}

	return true;
}

/*
 * ============================================================================
 * 公共方法实现 - 路径查询
 * ============================================================================
 */

/**
 * path_exists - 检查路径是否存在
 *
 * 检查指定的文件或目录路径是否存在。
 *
 * @path: 要检查的路径
 *
 * 返回值: 路径存在返回true，不存在返回false
 */
bool FileManager::path_exists(const std::string &path)
{
	struct stat stat_buf;
	return (stat(path.c_str(), &stat_buf) == 0);
}

/**
 * is_directory - 检查是否为目录
 *
 * 判断指定路径是否为目录。
 *
 * @path: 要检查的路径
 *
 * 返回值: 是目录返回true，不是目录返回false
 */
bool FileManager::is_directory(const std::string &path)
{
	struct stat stat_buf;
	if (stat(path.c_str(), &stat_buf) != 0)
		return false;
	return S_ISDIR(stat_buf.st_mode);
}

/**
 * is_file - 检查是否为文件
 *
 * 判断指定路径是否为普通文件。
 *
 * @path: 要检查的路径
 *
 * 返回值: 是文件返回true，不是文件返回false
 */
bool FileManager::is_file(const std::string &path)
{
	struct stat stat_buf;
	if (stat(path.c_str(), &stat_buf) != 0)
		return false;
	return S_ISREG(stat_buf.st_mode);
}

/**
 * get_file_size - 获取文件大小
 *
 * 获取指定文件的大小（字节数）。
 *
 * @path: 文件路径
 *
 * 返回值: 文件大小（字节），失败返回0
 */
size_t FileManager::get_file_size(const std::string &path)
{
	struct stat stat_buf;
	if (stat(path.c_str(), &stat_buf) != 0)
		return 0;
	return stat_buf.st_size;
}

/*
 * ============================================================================
 * 公共方法实现 - 文件类型检测
 * ============================================================================
 */

/**
 * is_binary_file - 判断文件是否为二进制文件
 *
 * 根据文件的MIME类型判断是否为二进制文件。常见的文本类型
 * （如text/ *、application/json等）将被识别为文本文件。
 *
 * @path: 文件路径
 *
 * 返回值: 是二进制文件返回true，是文本文件返回false
 *
 * 注意: 该方法不获取锁，因为调用者可能已经持有锁。
 */
bool FileManager::is_binary_file(const std::string &path)
{
	std::string mime_type = get_mime_type(path);

	/*
	 * 空文件不是二进制文件
	 * libmagic可能返回不同的空文件类型字符串
	 */
	if (mime_type == "inode/x-empty" ||
	    mime_type == "inode/x-emptyfile")
		return false;

	/*
	 * 检查MIME类型是否以text/开头
	 * text/ *类型通常表示文本文件
	 */
	if (mime_type.substr(0, 5) == "text/")
		return false;

	/*
	 * 检查常见的文本MIME类型
	 * 这些类型虽然不以text/开头，但实际上是文本文件
	 */
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
		"text/x-c",
		"text/x-csrc",
		"application/x-c++",
		"application/x-c++src",
		"text/x-c++",
		"text/x-c++src",
		"application/x-cpp",
		"text/x-cpp",
		"application/x-h",
		"application/x-header",
		"text/x-h",
		"text/x-h++",
		"text/x-chdr",
		"application/x-httpd-php",
		"text/plain",
		"text/x-makefile",
		"application/x-makescript",
		"application/yaml",
		"text/yaml",
		"application/x-toml",
		"text/x-toml",
		"application/x-ini",
		"text/x-ini",
		"text/markdown",
		"text/x-markdown",
		"application/x-shellscript"
	};

	/* 检查是否在已知文本类型列表中 */
	for (const auto &type : text_mime_types) {
		if (mime_type == type)
			return false;
	}

	/* 其他情况认为是二进制文件 */
	return true;
}

/**
 * get_mime_type - 获取文件MIME类型
 *
 * 使用libmagic库检测文件的MIME类型。MIME类型用于标识
 * 文件的类型和格式。
 *
 * @path: 文件路径
 *
 * 返回值: MIME类型字符串，失败返回"application/octet-stream"
 *
 * 注意: 该方法不获取锁，因为调用者可能已经持有锁。
 */
std::string FileManager::get_mime_type(const std::string &path)
{
	/* 检查libmagic是否已初始化 */
	if (!magic_cookie)
		return "application/octet-stream";

	/* 使用libmagic检测文件类型 */
	const char *mime_type = magic_file(magic_cookie, path.c_str());
	if (!mime_type)
		return "application/octet-stream";

	return std::string(mime_type);
}

/*
 * ============================================================================
 * 公共方法实现 - 路径处理
 * ============================================================================
 */

/**
 * normalize_path - 标准化路径
 *
 * 清理路径字符串，移除多余的斜杠，确保路径格式规范。
 *
 * @path: 要标准化的路径字符串
 *
 * 返回值: 标准化后的路径字符串
 */
std::string FileManager::normalize_path(const std::string &path)
{
	std::string normalized = path;

	/*
	 * 替换多个连续的/为单个/
	 * 例如："path//to///dir" -> "path/to/dir"
	 */
	size_t pos = 0;
	while ((pos = normalized.find("//", pos)) != std::string::npos)
		normalized.replace(pos, 2, "/");

	/*
	 * 去除末尾的/
	 * 但保留根目录的/
	 */
	while (!normalized.empty() && normalized.back() == '/' &&
	       normalized.size() > 1)
		normalized.pop_back();

	return normalized;
}

/**
 * get_parent_directory - 获取父目录路径
 *
 * 从指定路径提取父目录路径。
 *
 * @path: 文件或目录路径
 *
 * 返回值: 父目录路径字符串
 */
std::string FileManager::get_parent_directory(const std::string &path)
{
	size_t pos = path.find_last_of('/');
	if (pos == std::string::npos)
		return ".";

	if (pos == 0)
		return "/";

	return path.substr(0, pos);
}

/**
 * get_file_name - 获取文件名
 *
 * 从路径中提取文件名部分（不含路径）。
 *
 * @path: 文件路径
 *
 * 返回值: 文件名字符串
 */
std::string FileManager::get_file_name(const std::string &path)
{
	size_t pos = path.find_last_of('/');
	if (pos == std::string::npos)
		return path;

	return path.substr(pos + 1);
}

/**
 * get_file_extension - 获取文件扩展名
 *
 * 从文件名中提取扩展名（包含点号）。
 *
 * @filename: 文件名（不含路径）
 *
 * 返回值: 扩展名字符串，无扩展名返回空字符串
 */
std::string FileManager::get_file_extension(const std::string &filename)
{
	size_t pos = filename.find_last_of('.');
	if (pos == std::string::npos)
		return "";

	return filename.substr(pos);
}
