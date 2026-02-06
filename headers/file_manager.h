/*
 * Mikufy v2.7-nova - 文件管理器头文件
 *
 * 本文件定义了FileManager类的接口，该类负责处理所有与文件系统
 * 相关的操作。FileManager提供了一组统一的API来执行文件的读取、
 * 写入、删除、重命名等操作，并使用libmagic库进行文件类型检测。
 * 该类设计为线程安全，使用互斥锁保护共享资源。
 *
 * 主要功能:
 * - 目录内容读取和遍历
 * - 文件内容读取和写入（支持文本和二进制）
 * - 文件和目录的创建、删除、重命名
 * - 文件信息查询（大小、类型、MIME类型等）
 * - 路径标准化和解析
 * - 二进制文件检测
 *
 * MiraTrive/MikuTrive
 * Author: [Your Name]
 *
 * 本文件遵循Linux内核代码风格规范
 */

#ifndef MIKUFY_FILE_MANAGER_H
#define MIKUFY_FILE_MANAGER_H

/*
 * ============================================================================
 * 头文件包含
 * ============================================================================
 */

#include "main.h"		/* 全局定义和数据结构 */
#include <fstream>		/* std::ifstream, std::ofstream 文件流 */
#include <sstream>		/* std::stringstream 字符串流 */
#include <unordered_set>	/* std::unordered_set 哈希集合 */
#include <unordered_map>	/* std::unordered_map 哈希映射 */
#include <list>			/* std::list 双向链表 */
#include <chrono>		/* std::chrono 时间库 */

/*
 * ============================================================================
 * FileManager类定义
 * ============================================================================
 */

/**
 * FileManager - 文件管理器类
 *
 * 该类封装了所有文件系统操作，提供线程安全的文件访问接口。
 * 使用libmagic库进行文件类型检测，能够区分文本文件和二进制文件。
 * 所有公共方法都是线程安全的，使用互斥锁保护共享资源。
 *
 * 设计特点:
 * - 线程安全：使用std::mutex保护所有文件操作
 * - 禁止拷贝和移动：删除拷贝构造函数和移动操作符
 * - RAII管理：在构造/析构函数中初始化/清理libmagic资源
 * - 错误处理：方法返回bool表示操作是否成功
 *
 * 使用示例:
 * @code
 * FileManager fm;
 * std::vector<FileInfo> files;
 * if (fm.get_directory_contents("/path/to/dir", files)) {
 *     for (const auto& file : files) {
 *         std::cout << file.name << std::endl;
 *     }
 * }
 * @endcode
 */
class FileManager
{
public:
	/**
	 * FileManager - 构造函数
	 *
	 * 初始化FileManager对象，创建libmagic句柄用于文件类型检测。
	 * 构造函数会调用init_magic()方法来初始化libmagic数据库。
	 *
	 * 注意: 如果libmagic初始化失败，后续的文件类型检测功能
	 *       将不可用，但文件读写操作仍可正常工作。
	 */
	FileManager();

	/**
	 * ~FileManager - 析构函数
	 *
	 * 清理FileManager对象，释放libmagic句柄和资源。
	 * 调用cleanup_magic()方法来关闭libmagic数据库。
	 */
	~FileManager(void);

	/* 禁止拷贝构造函数 */
	FileManager(const FileManager &) = delete;

	/* 禁止拷贝赋值操作符 */
	FileManager &operator=(const FileManager &) = delete;

	/* 禁止移动构造函数 */
	FileManager(FileManager &&) = delete;

	/* 禁止移动赋值操作符 */
	FileManager &operator=(FileManager &&) = delete;

	/* ====================================================================
	 * 公共方法 - 目录操作
	 * ==================================================================== */

	/**
	 * get_directory_contents - 获取目录内容
	 *
	 * 读取指定目录下的所有文件和子目录，并返回它们的详细信息。
	 * 结果会按照类型排序（目录在前，文件在后），同类项按名称排序。
	 * 该方法会跳过"."和".."特殊目录项。
	 *
	 * @path: 要读取的目录路径（绝对路径或相对路径）
	 * @files: 输出参数，用于存储文件信息列表的引用
	 *
	 * 返回值: 成功返回true，失败返回false
	 *
	 * 注意: 该方法会限制最多读取1000个条目，防止在大目录中卡死。
	 *       如果目录内容超过1000项，后续条目将被忽略。
	 */
	bool get_directory_contents(const std::string &path,
				    std::vector<FileInfo> &files);

	/* ====================================================================
	 * 公共方法 - 文件读取
	 * ==================================================================== */

	/**
	 * read_file - 读取文本文件内容
	 *
	 * 读取指定文件的内容并返回为字符串。该方法仅适用于文本文件，
	 * 对于二进制文件会返回失败。文件内容以字符串形式返回，保留
	 * 原有的换行符和格式。
	 *
	 * @path: 要读取的文件路径
	 * @content: 输出参数，用于存储文件内容的引用
	 *
	 * 返回值: 成功返回true，失败返回false
	 *
	 * 注意: 该方法会自动检测文件类型，如果是二进制文件则拒绝读取。
	 *       大文件读取可能会消耗较多内存。
	 */
	bool read_file(const std::string &path, std::string &content);

	/**
	 * read_file_binary - 读取二进制文件内容
	 *
	 * 读取指定文件的内容并返回为字节数组。该方法适用于所有文件类型，
	 * 包括文本文件和二进制文件。文件内容以字符向量形式返回，每个
	 * 字节对应文件的一个字节。
	 *
	 * @path: 要读取的文件路径
	 * @content: 输出参数，用于存储文件内容的字符向量引用
	 *
	 * 返回值: 成功返回true，失败返回false
	 *
	 * 注意: 该方法不进行文件类型检测，可以读取任何文件。
	 *       大文件读取可能会消耗较多内存。
	 */
	bool read_file_binary(const std::string &path,
			      std::vector<char> &content);

	/* ====================================================================
	 * 公共方法 - 文件写入
	 * ==================================================================== */

	/**
	 * write_file - 写入文件内容
	 *
	 * 将字符串内容写入指定文件。如果文件已存在，其内容将被覆盖；
	 * 如果文件不存在，将创建新文件。写入使用二进制模式，保留
	 * 原始字符编码。
	 *
	 * @path: 要写入的文件路径
	 * @content: 要写入的文件内容字符串
	 *
	 * 返回值: 成功返回true，失败返回false
	 *
	 * 注意: 该方法会截断文件（如果存在），不进行追加操作。
	 *       确保有足够的磁盘空间和文件写入权限。
	 */
	bool write_file(const std::string &path, const std::string &content);

	/* ====================================================================
	 * 公共方法 - 创建操作
	 * ==================================================================== */

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
	 * 注意: 该方法不会递归创建父目录，父目录必须已存在。
	 *       如果需要递归创建，应先创建父目录。
	 */
	bool create_directory(const std::string &path);

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
	 * 注意: 该方法会创建或清空文件，不进行任何写入操作。
	 *       确保有足够的磁盘空间和文件写入权限。
	 */
	bool create_file(const std::string &path);

	/* ====================================================================
	 * 公共方法 - 删除操作
	 * ==================================================================== */

	/**
	 * delete_item - 删除文件或目录
	 *
	 * 删除指定的文件或目录。如果是目录，将递归删除其所有内容。
	 * 该方法会自动判断路径类型并执行相应的删除操作。
	 *
	 * @path: 要删除的文件或目录路径
	 *
	 * 返回值: 成功返回true，失败返回false
	 *
	 * 注意: 删除操作不可恢复，请谨慎使用。
	 *       递归删除可能需要较长时间，取决于目录内容大小。
	 */
	bool delete_item(const std::string &path);

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
	 *       该方法仅供delete_item()内部调用。
	 */
	bool delete_directory_recursive(const std::string &path);

	/* ====================================================================
	 * 公共方法 - 重命名操作
	 * ==================================================================== */

	/**
	 * rename_item - 重命名文件或目录
	 *
	 * 将文件或目录从旧路径移动到新路径。可以在同一文件系统内
	 * 移动文件，也可以仅重命名。如果目标路径已存在，操作将失败。
	 *
	 * @old_path: 原始文件或目录路径
	 * @new_path: 新的文件或目录路径
	 *
	 * 返回值: 成功返回true，失败返回false
	 *
	 * 注意: 跨文件系统移动可能需要额外的处理。
	 *       确保目标路径的父目录已存在且有写入权限。
	 */
	bool rename_item(const std::string &old_path,
			 const std::string &new_path);

	/* ====================================================================
	 * 公共方法 - 文件信息查询
	 * ==================================================================== */

	/**
	 * get_file_info - 获取文件信息
	 *
	 * 查询指定文件或目录的详细信息，包括名称、路径、大小、类型、
	 * MIME类型等。信息将填充到FileInfo结构体中。
	 *
	 * @path: 要查询的文件或目录路径
	 * @info: 输出参数，用于存储文件信息的引用
	 *
	 * 返回值: 成功返回true，失败返回false
	 *
	 * 注意: 对于目录，size字段将为0。
	 *       MIME类型使用libmagic库检测。
	 */
	bool get_file_info(const std::string &path, FileInfo &info);

	/* ====================================================================
	 * 公共方法 - 路径查询
	 * ==================================================================== */

	/**
	 * path_exists - 检查路径是否存在
	 *
	 * 检查指定的文件或目录路径是否存在。
	 *
	 * @path: 要检查的路径
	 *
	 * 返回值: 路径存在返回true，不存在返回false
	 */
	bool path_exists(const std::string &path);

	/**
	 * is_directory - 检查是否为目录
	 *
	 * 判断指定路径是否为目录。
	 *
	 * @path: 要检查的路径
	 *
	 * 返回值: 是目录返回true，不是目录返回false
	 */
	bool is_directory(const std::string &path);

	/**
	 * is_file - 检查是否为文件
	 *
	 * 判断指定路径是否为普通文件。
	 *
	 * @path: 要检查的路径
	 *
	 * 返回值: 是文件返回true，不是文件返回false
	 */
	bool is_file(const std::string &path);

	/**
	 * get_file_size - 获取文件大小
	 *
	 * 获取指定文件的大小（字节数）。
	 *
	 * @path: 文件路径
	 *
	 * 返回值: 文件大小（字节），失败返回0
	 */
	size_t get_file_size(const std::string &path);

	/* ====================================================================
	 * 公共方法 - 文件类型检测
	 * ==================================================================== */

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
	 * 注意: 该方法依赖于libmagic库的正确初始化。
	 *       空文件被认为是文本文件。
	 */
	bool is_binary_file(const std::string &path);

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
	 * 注意: 该方法依赖于libmagic库的正确初始化。
	 *       对于未知类型，返回默认的octet-stream类型。
	 */
	std::string get_mime_type(const std::string &path);

	/* ====================================================================
	 * 公共方法 - 路径处理
	 * ==================================================================== */

	/**
	 * normalize_path - 标准化路径
	 *
	 * 清理路径字符串，移除多余的斜杠，确保路径格式规范。
	 * 例如："path//to///dir" -> "path/to/dir"
	 *
	 * @path: 要标准化的路径字符串
	 *
	 * 返回值: 标准化后的路径字符串
	 */
	std::string normalize_path(const std::string &path);

	/**
	 * get_parent_directory - 获取父目录路径
	 *
	 * 从指定路径提取父目录路径。例如：
	 * "/path/to/file.txt" -> "/path/to"
	 * "/path/to/dir/" -> "/path/to"
	 *
	 * @path: 文件或目录路径
	 *
	 * 返回值: 父目录路径字符串
	 */
	std::string get_parent_directory(const std::string &path);

	/**
	 * get_file_name - 获取文件名
	 *
	 * 从路径中提取文件名部分（不含路径）。例如：
	 * "/path/to/file.txt" -> "file.txt"
	 *
	 * @path: 文件路径
	 *
	 * 返回值: 文件名字符串
	 */
	std::string get_file_name(const std::string &path);

	/**
	 * get_file_extension - 获取文件扩展名
	 *
	 * 从文件名中提取扩展名（包含点号）。例如：
	 * "file.txt" -> ".txt"
	 * "archive.tar.gz" -> ".gz"
	 *
	 * @filename: 文件名（不含路径）
	 *
	 * 返回值: 扩展名字符串，无扩展名返回空字符串
	 */
	std::string get_file_extension(const std::string &filename);

private:
	/* ====================================================================
	 * 私有成员变量
	 * ==================================================================== */

	magic_t magic_cookie;	/* libmagic句柄，用于文件类型检测 */
	std::mutex mutex;	/* 互斥锁，保证线程安全 */

	/*
	 * 文件内容缓存相关
	 * 使用LRU（最近最少使用）缓存策略，提升重复读取同一文件的性能
	 */
	struct FileCacheEntry {
		std::string content;		/* 文件内容 */
		std::chrono::system_clock::time_point timestamp;	/* 缓存时间戳 */
		size_t size;			/* 文件大小（字节） */
	};

	std::unordered_map<std::string, FileCacheEntry> file_cache;	/* 文件缓存哈希表 */
	std::list<std::string> file_cache_lru;	/* LRU链表，记录访问顺序 */
	size_t cache_size;		/* 当前缓存大小（字节） */

	/* ====================================================================
	 * 私有方法 - libmagic管理
	 * ==================================================================== */

	/**
	 * init_magic - 初始化libmagic库
	 *
	 * 打开libmagic数据库并加载默认的magic文件。该方法在
	 * 构造函数中调用，用于初始化文件类型检测功能。
	 *
	 * 返回值: 成功返回true，失败返回false
	 *
	 * 注意: 该方法需要持有互斥锁才能调用。
	 */
	bool init_magic(void);

	/**
	 * cleanup_magic - 清理libmagic资源
	 *
	 * 关闭libmagic句柄并释放相关资源。该方法在析构函数中调用。
	 *
	 * 注意: 该方法需要持有互斥锁才能调用。
	 */
	void cleanup_magic(void);

	/* ====================================================================
	 * 私有方法 - 缓存管理
	 * ==================================================================== */

	/**
	 * get_cached_file - 从缓存中获取文件内容
	 *
	 * 尝试从缓存中获取指定路径的文件内容。
	 * 如果缓存命中，更新LRU链表，将文件移到链表头部。
	 *
	 * @path: 文件路径
	 * @content: 输出参数，存储缓存的内容
	 *
	 * 返回值: 缓存命中返回true，未命中返回false
	 *
	 * 注意: 该方法需要持有互斥锁才能调用。
	 */
	bool get_cached_file(const std::string &path, std::string &content);

	/**
	 * cache_file - 将文件内容缓存
	 *
	 * 将文件内容添加到缓存中。
	 * 如果缓存已满，按照LRU策略淘汰最久未使用的文件。
	 *
	 * @path: 文件路径
	 * @content: 文件内容
	 * @size: 文件大小（字节）
	 *
	 * 注意: 该方法需要持有互斥锁才能调用。
	 */
	void cache_file(const std::string &path, const std::string &content,
			size_t size);

	/**
	 * evict_cache - 淘汰缓存条目
	 *
	 * 按照LRU策略淘汰最久未使用的缓存条目，直到缓存大小低于限制。
	 *
	 * @required_size: 需要释放的空间大小（字节）
	 *
	 * 注意: 该方法需要持有互斥锁才能调用。
	 */
	void evict_cache(size_t required_size);

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
	void invalidate_cache(const std::string &path);

	/**
	 * clear_cache - 清空所有缓存
	 *
	 * 清空所有文件缓存，释放内存。
	 *
	 * 注意: 该方法需要持有互斥锁才能调用。
	 */
	void clear_cache(void);
};

#endif /* MIKUFY_FILE_MANAGER_H */
