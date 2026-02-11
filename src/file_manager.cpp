/*
 * Mikufy v2.11-nova - 文件管理器实现
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
 * MiraTrive/MikuTrive
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
#include <format>		/* C++23 std::format */

/*
 * ============================================================================
 * 构造函数和析构函数
 * ============================================================================
 */

/**
 * FileManager - 构造函数
 *
 * 初始化FileManager对象，设置libmagic句柄为nullptr，并调用
 * init_magic()方法初始化libmagic库。同时初始化文件缓存。
 */
FileManager::FileManager() : magic_cookie(nullptr), cache_size(0)
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

	/* 打开目录 */
	DIR *dir = opendir(path.c_str());
	if (!dir)
		return false;

	struct dirent *entry;
	int count = 0;

	/*
	 * 遍历目录中的所有条目
	 * 优化：减少系统调用，延迟获取 MIME 类型
	 */
	while ((entry = readdir(dir)) != nullptr) {
		/*
		 * 跳过当前目录(.)和父目录(..)
		 */
		if (strcmp(entry->d_name, ".") == 0 ||
		    strcmp(entry->d_name, "..") == 0)
			continue;

		/*
		 * 跳过隐藏文件（以.开头）
		 */
		if (entry->d_name[0] == '.' &&
		    strcmp(entry->d_name, ".") != 0 &&
		    strcmp(entry->d_name, "..") != 0)
			continue;

		FileInfo info;
		info.name = entry->d_name;

		/* 构建完整路径 */
		info.path = path;
		if (info.path.back() != '/')
			info.path += '/';
		info.path += entry->d_name;

		/*
		 * 获取文件状态信息
		 * 优先使用 dirent 的 d_type 字段，避免 stat 调用
		 */
		if (entry->d_type != DT_UNKNOWN) {
			/* 使用 dirent 提供的类型信息 */
			info.is_directory = (entry->d_type == DT_DIR);
			if (entry->d_type == DT_REG) {
				/* 只对常规文件调用 stat 获取大小 */
				struct stat stat_buf;
				if (stat(info.path.c_str(), &stat_buf) == 0)
					info.size = stat_buf.st_size;
				else
					info.size = 0;
			} else {
				info.size = 0;
			}
		} else {
			/* d_type 未知，必须调用 stat */
			struct stat stat_buf;
			if (stat(info.path.c_str(), &stat_buf) == 0) {
				info.is_directory = S_ISDIR(stat_buf.st_mode);
				info.size = stat_buf.st_size;
			} else {
				info.is_directory = false;
				info.size = 0;
			}
		}

		/*
		 * 延迟获取 MIME 类型
		 * 目录不需要检测，文件只在需要时检测（懒加载）
		 */
		if (info.is_directory) {
			info.mime_type = "inode/directory";
			info.is_binary = false;
		} else {
			/* 暂时不获取 MIME 类型，设置为空字符串 */
			info.mime_type = "";
			info.is_binary = false;
		}

		/* 将FileInfo添加到结果列表 */
		files.push_back(info);
		count++;

		/*
		 * 限制读取数量，防止在超大目录中卡死
		 */
		if (count >= MAX_DIR_ENTRIES)
			break;
	}

	/* 关闭目录句柄 */
	closedir(dir);

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
 * 优化要点:
 * - 使用LRU缓存机制，缓存已读取的文件内容
 * - 缓存命中时直接返回，避免磁盘I/O
 * - 缓存未命中时读取文件并缓存
 * - 使用内存预分配，减少重新分配次数
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

	/*
	 * 检查文件是否存在
	 */
	if (!is_file(path))
		return false;

	/*
	 * 尝试从缓存中获取文件内容
	 * 缓存命中时直接返回，大幅提升性能
	 */
	if (get_cached_file(path, content))
		return true;

	/*
	 * 缓存未命中，从磁盘读取文件
	 */

	/*
	 * 获取文件大小
	 * 防止读取超大文件导致内存溢出
	 */
	const size_t file_size = get_file_size(path);

	/*
	 * 检查文件大小是否超过限制
	 */
	if (file_size > MAX_FILE_READ_SIZE) {
		std::cerr << std::format("文件过大（{} 字节），超过限制 {} 字节",
					file_size, MAX_FILE_READ_SIZE) << std::endl;
		return false;
	}

	/*
	 * 检查是否为二进制文件
	 */
	if (is_binary_file(path))
		return false;

	/*
	 * 打开文件（二进制模式以保留原始编码）
	 */
	std::ifstream file(path, std::ios::binary);

	/*
	 * 检查文件是否成功打开
	 */
	if (!file.is_open())
		return false;

	/*
	 * 预分配内存
	 * 避免多次重新分配，提高性能
	 */
	try {
		content.reserve(file_size);
	} catch (const std::bad_alloc &) {
		std::cerr << "内存分配失败，文件大小: " << file_size << std::endl;
		file.close();
		return false;
	}

	/*
	 * 读取文件内容到字符串流
	 */
	std::stringstream buffer;
	buffer << file.rdbuf();
	content = buffer.str();

	/*
	 * 关闭文件
	 */
	file.close();

	/*
	 * 将文件内容缓存
	 * 使用移动语义避免字符串拷贝
	 */
	cache_file(path, content, file_size);

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
 * 优化要点:
 * - 文件写入后使缓存失效，确保缓存一致性
 * - 避免缓存与磁盘内容不一致
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

	/*
	 * 检查文件是否成功打开
	 */
	if (!file.is_open())
		return false;

	/*
	 * 写入内容
	 */
	file << content;

	/*
	 * 关闭文件
	 */
	file.close();

	/*
	 * 检查写入是否成功
	 */
	if (!file.good())
		return false;

	/*
	 * 使缓存失效
	 * 文件已修改，需要清除旧缓存
	 * 下次读取时将重新从磁盘加载
	 */
	invalidate_cache(path);

	return true;
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
 * 使用哈希表实现O(1)时间复杂度的查找，替代原有的O(n)循环查找。
 * 这对频繁的文件类型检测有显著性能提升。
 *
 * @path: 文件路径
 *
 * 返回值: 是二进制文件返回true，是文本文件返回false
 *
 * 注意: 该方法不获取锁，因为调用者可能已经持有锁。
 */
bool FileManager::is_binary_file(const std::string &path)
{
	/*
	 * 静态哈希表存储所有已知的文本MIME类型
	 * 使用unordered_set实现O(1)平均时间复杂度的查找
	 * 该表在首次调用时初始化，后续调用直接使用，无需重复创建
	 */
	static const std::unordered_set<std::string> text_mime_types = {
		/* text/ 前缀的文本类型（部分常见类型） */
		"text/plain",
		"text/html",
		"text/css",
		"text/javascript",
		"text/xml",
		"text/markdown",
		"text/x-c",
		"text/x-c++",
		"text/x-cpp",
		"text/x-csrc",
		"text/x-c++src",
		"text/x-h",
		"text/x-h++",
		"text/x-chdr",
		"text/x-makefile",
		"text/x-toml",
		"text/x-ini",
		"text/x-markdown",
		"text/yaml",

		/* JSON及相关 */
		"application/json",
		"application/x-json",

		/* XML相关 */
		"application/xml",
		"text/xml",

		/* JavaScript相关 */
		"application/javascript",
		"text/javascript",

		/* Shell脚本 */
		"application/x-sh",
		"application/x-shellscript",
		"text/x-shellscript",

		/* Python */
		"application/x-python",
		"text/x-python",

		/* Perl */
		"application/x-perl",
		"text/x-perl",

		/* Ruby */
		"application/x-ruby",
		"text/x-ruby",

		/* PHP */
		"application/x-php",
		"application/x-httpd-php",
		"text/x-php",

		/* C/C++源代码 */
		"application/x-c",
		"application/x-csrc",
		"application/x-c++",
		"application/x-c++src",
		"application/x-cpp",
		"application/x-h",
		"application/x-header",

		/* 配置文件 */
		"application/yaml",
		"application/x-yaml",
		"application/x-toml",
		"application/x-ini",

		/* Makefile */
		"application/x-makescript",
		"text/x-makefile",

		/* SQL */
		"application/x-sql",
		"text/x-sql",

		/* 其他文本格式 */
		"application/x-wmf",
		"application/x-rss+xml"
	};

	std::string mime_type = get_mime_type(path);

	/*
	 * 空文件不是二进制文件
	 * libmagic可能返回不同的空文件类型字符串
	 * 使用比较操作而非if判断，提高可读性
	 */
	const bool is_empty = (mime_type == "inode/x-empty" ||
			      mime_type == "inode/x-emptyfile");
	if (is_empty)
		return false;

	/*
	 * 检查MIME类型是否以text/开头
	 * text/ *类型通常表示文本文件
	 * 使用比较而非substr以避免不必要的字符串拷贝
	 */
	const bool starts_with_text = (mime_type.size() >= 5 &&
				      mime_type[0] == 't' &&
				      mime_type[1] == 'e' &&
				      mime_type[2] == 'x' &&
				      mime_type[3] == 't' &&
				      mime_type[4] == '/');
	if (starts_with_text)
		return false;

	/*
	 * 使用哈希表查找已知文本类型
	 * unordered_set::find()平均时间复杂度为O(1)
	 * 相比原有循环查找有显著性能提升
	 */
	return (text_mime_types.find(mime_type) == text_mime_types.end());
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
 * 优化要点:
 * - 使用原地修改而非重复的find/replace，减少内存分配
 * - 使用双指针算法一次遍历完成处理，时间复杂度O(n)
 * - 预分配结果空间，避免多次重新分配
 *
 * @path: 要标准化的路径字符串
 *
 * 返回值: 标准化后的路径字符串
 */
std::string FileManager::normalize_path(const std::string &path)
{
	/*
	 * 空路径或单个/路径直接返回
	 */
	if (path.empty())
		return "";

	if (path == "/")
		return "/";

	/*
	 * 预分配结果字符串空间，避免多次重新分配
	 * 最坏情况下结果长度等于输入长度
	 */
	std::string normalized;
	normalized.reserve(path.size());

	/*
	 * 双指针算法：一次遍历处理所有斜杠
	 * write_pos: 写入位置指针
	 * skip_slash: 标记是否需要跳过下一个斜杠
	 */
	size_t write_pos = 0;
	bool skip_slash = false;

	/*
	 * 处理根目录开头的/
	 */
	if (path[0] == '/') {
		normalized += '/';
		write_pos = 1;
		skip_slash = true;
	}

	/*
	 * 遍历路径中的每个字符
	 */
	for (size_t i = write_pos; i < path.size(); ++i) {
		/*
		 * 遇到斜杠
		 */
		if (path[i] == '/') {
			/*
			 * 如果前一个字符也是斜杠，跳过当前斜杠
			 * 使用状态标记而非if/else嵌套
			 */
			if (skip_slash)
				continue;

			/*
			 * 添加斜杠并设置跳过标记
			 */
			normalized += '/';
			skip_slash = true;
		} else {
			/*
			 * 非斜杠字符直接添加
			 */
			normalized += path[i];
			skip_slash = false;
		}
	}

	/*
	 * 去除末尾的/（根目录除外）
	 */
	if (normalized.size() > 1 && normalized.back() == '/')
		normalized.pop_back();

	return normalized;
}

/**
 * get_parent_directory - 获取父目录路径
 *
 * 从指定路径提取父目录路径。
 *
 * 优化要点:
 * - 使用find_last_of一次查找
 * - 避免不必要的字符串拷贝
 *
 * @path: 文件或目录路径
 *
 * 返回值: 父目录路径字符串
 */
std::string FileManager::get_parent_directory(const std::string &path)
{
	/*
	 * 查找最后一个斜杠位置
	 */
	const size_t pos = path.find_last_of('/');

	/*
	 * 没有找到斜杠，返回当前目录
	 */
	if (pos == std::string::npos)
		return ".";

	/*
	 * 根目录的父目录是根目录
	 */
	if (pos == 0)
		return "/";

	/*
	 * 返回斜杠之前的部分
	 */
	return path.substr(0, pos);
}

/**
 * get_file_name - 获取文件名
 *
 * 从路径中提取文件名部分（不含路径）。
 *
 * 优化要点:
 * - 使用find_last_of一次查找
 * - 如果没有路径分隔符，直接返回整个字符串
 *
 * @path: 文件路径
 *
 * 返回值: 文件名字符串
 */
std::string FileManager::get_file_name(const std::string &path)
{
	/*
	 * 查找最后一个斜杠位置
	 */
	const size_t pos = path.find_last_of('/');

	/*
	 * 没有找到斜杠，返回整个路径
	 */
	if (pos == std::string::npos)
		return path;

	/*
	 * 返回斜杠之后的部分
	 */
	return path.substr(pos + 1);
}

/**
 * get_file_extension - 获取文件扩展名
 *
 * 从文件名中提取扩展名（包含点号）。
 *
 * 优化要点:
 * - 使用find_last_of一次查找
 * - 处理隐藏文件和文件名中多个点的情况
 *
 * @filename: 文件名（不含路径）
 *
 * 返回值: 扩展名字符串，无扩展名返回空字符串
 */
std::string FileManager::get_file_extension(const std::string &filename)
{
	/*
	 * 查找最后一个点号位置
	 */
	const size_t pos = filename.find_last_of('.');

	/*
	 * 没有点号，返回空字符串
	 */
	if (pos == std::string::npos)
		return "";

	/*
	 * 点号在开头（隐藏文件），不算扩展名
	 * 例如：".gitignore" -> ""
	 */
	if (pos == 0)
		return "";

	/*
	 * 返回点号及之后的部分
	 * 例如："archive.tar.gz" -> ".gz"
	 */
	return filename.substr(pos);
}

/*
 * ============================================================================
 * 缓存管理方法实现
 * ============================================================================
 */

/**
 * get_cached_file - 从缓存中获取文件内容
 *
 * 尝试从缓存中获取指定路径的文件内容。
 * 如果缓存命中，更新LRU链表，将文件移到链表头部。
 *
 * 优化要点:
 * - 使用哈希表实现O(1)查找
 * - 使用LRU链表管理访问顺序
 * - 缓存命中时更新时间戳和LRU链表
 *
 * @path: 文件路径
 * @content: 输出参数，存储缓存的内容
 *
 * 返回值: 缓存命中返回true，未命中返回false
 *
 * 注意: 该方法需要持有互斥锁才能调用。
 */
bool FileManager::get_cached_file(const std::string &path, std::string &content)
{
	/*
	 * 在哈希表中查找缓存条目
	 */
	const auto it = file_cache.find(path);

	/*
	 * 缓存未命中
	 */
	if (it == file_cache.end())
		return false;

	/*
	 * 缓存命中，更新时间戳
	 */
	it->second.timestamp = std::chrono::system_clock::now();

	/*
	 * 更新LRU链表
	 * 从原位置移除，插入到头部
	 */
	file_cache_lru.remove(path);
	file_cache_lru.push_front(path);

	/*
	 * 返回缓存内容
	 */
	content = it->second.content;

	return true;
}

/**
 * cache_file - 将文件内容缓存
 *
 * 将文件内容添加到缓存中。
 * 如果缓存已满，按照LRU策略淘汰最久未使用的文件。
 *
 * 优化要点:
 * - 使用LRU策略管理缓存，淘汰最久未使用的条目
 * - 预先检查缓存大小，避免频繁的淘汰操作
 * - 使用移动语义减少字符串拷贝
 *
 * @path: 文件路径
 * @content: 文件内容
 * @size: 文件大小（字节）
 *
 * 注意: 该方法需要持有互斥锁才能调用。
 */
void FileManager::cache_file(const std::string &path, const std::string &content,
			     size_t size)
{
	/*
	 * 检查文件是否已在缓存中
	 */
	const auto it = file_cache.find(path);

	/*
	 * 如果文件已在缓存中，先移除旧的条目
	 */
	if (it != file_cache.end()) {
		/*
		 * 从LRU链表中移除
		 */
		file_cache_lru.remove(path);

		/*
		 * 从缓存大小中减去旧文件大小
		 */
		cache_size -= it->second.size;

		/*
		 * 从哈希表中移除
		 */
		file_cache.erase(it);
	}

	/*
	 * 检查是否需要淘汰缓存
	 * 如果新文件大小超过缓存限制，直接拒绝缓存
	 */
	if (size > MAX_CACHE_SIZE)
		return;

	/*
	 * 检查缓存空间是否足够
	 * 如果不足，淘汰最久未使用的文件
	 */
	if (cache_size + size > MAX_CACHE_SIZE)
		evict_cache(cache_size + size - MAX_CACHE_SIZE);

	/*
	 * 创建新的缓存条目
	 */
	FileCacheEntry entry;
	entry.content = content;
	entry.timestamp = std::chrono::system_clock::now();
	entry.size = size;

	/*
	 * 添加到哈希表
	 */
	file_cache[path] = std::move(entry);

	/*
	 * 添加到LRU链表头部
	 */
	file_cache_lru.push_front(path);

	/*
	 * 更新缓存大小
	 */
	cache_size += size;
}

/**
 * evict_cache - 淘汰缓存条目
 *
 * 按照LRU策略淘汰最久未使用的缓存条目，直到缓存大小低于限制。
 *
 * 优化要点:
 * - 从LRU链表尾部开始淘汰，保证淘汰最久未使用的条目
 * - 同时更新哈希表和LRU链表
 * - 循环淘汰直到满足大小要求
 *
 * @required_size: 需要释放的空间大小（字节）
 *
 * 注意: 该方法需要持有互斥锁才能调用。
 */
void FileManager::evict_cache(size_t required_size)
{
	/*
	 * 循环淘汰，直到释放足够的空间
	 */
	while (!file_cache_lru.empty() && required_size > 0) {
		/*
		 * 获取LRU链表尾部（最久未使用）的文件路径
		 */
		const std::string lru_path = file_cache_lru.back();

		/*
		 * 在哈希表中查找该文件
		 */
		const auto it = file_cache.find(lru_path);

		/*
		 * 如果找到，从缓存中移除
		 */
		if (it != file_cache.end()) {
			/*
			 * 减少缓存大小
			 */
			cache_size -= it->second.size;

			/*
			 * 减少需要释放的空间
			 */
			if (it->second.size <= required_size) {
				required_size -= it->second.size;
			} else {
				required_size = 0;
			}

			/*
			 * 从哈希表中移除
			 */
			file_cache.erase(it);
		}

		/*
		 * 从LRU链表中移除
		 */
		file_cache_lru.pop_back();
	}
}

/**
 * invalidate_cache - 使指定文件的缓存失效
 *
 * 使指定文件的缓存失效，下次读取时将重新从磁盘加载。
 * 在文件被修改后应调用此方法。
 *
 * @path: 文件路径
 *
 * 注意: 该方法需要持有互斥锁才能调用。
 */
void FileManager::invalidate_cache(const std::string &path)
{
	/*
	 * 在哈希表中查找缓存条目
	 */
	const auto it = file_cache.find(path);

	/*
	 * 如果找到，从缓存中移除
	 */
	if (it != file_cache.end()) {
		/*
		 * 减少缓存大小
		 */
		cache_size -= it->second.size;

		/*
		 * 从哈希表中移除
		 */
		file_cache.erase(it);

		/*
		 * 从LRU链表中移除
		 */
		file_cache_lru.remove(path);
	}
}

/**
 * clear_cache - 清空所有缓存
 *
 * 清空所有文件缓存，释放内存。
 *
 * 注意: 该方法需要持有互斥锁才能调用。
 */
void FileManager::clear_cache(void)
{
	/*
	 * 清空哈希表
	 */
	file_cache.clear();

	/*
	 * 清空LRU链表
	 */
	file_cache_lru.clear();

	/*
	 * 重置缓存大小
	 */
	cache_size = 0;
}
