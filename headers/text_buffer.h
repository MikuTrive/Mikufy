/*
 * Mikufy v2.11-nova - 高性能文本缓冲模块
 *
 * 本文件实现了基于 Piece Table 数据结构的高性能文本缓冲区，
 * 用于支持大文件（10万~50万行）的高效编辑操作。
 *
 * Piece Table 架构：
 *   - Original Buffer: 使用 mmap 映射原始文件内容
 *   - Add Buffer: 存储用户编辑添加的内容
 *   - Piece List: 维护指向两个缓冲区的段引用
 *
 * 主要功能:
 *   - 使用 mmap 高效读取大文件
 *   - O(log n) 的插入和删除操作
 *   - 高效的行范围查询
 *   - 支持大文件的虚拟化渲染
 *
 * MiraTrive/MikuTrive
 * Author: [Your Name]
 *
 * 本文件遵循 Linux 内核代码风格规范
 */

#ifndef MIKUFY_TEXT_BUFFER_H
#define MIKUFY_TEXT_BUFFER_H

/*
 * ============================================================================
 * 头文件包含
 * ============================================================================
 */

#include "main.h"
#include <sys/mman.h>		/* mmap(), munmap() */
#include <sys/stat.h>		/* stat() */
#include <fcntl.h>		/* open(), O_RDONLY */
#include <unistd.h>		/* close() */
#include <deque>			/* std::deque 用于 Piece 列表 */
#include <algorithm>		/* std::lower_bound */
#include <expected>		/* C++23 std::expected 错误处理 */
#include <format>		/* C++23 std::format 字符串格式化 */

/*
 * ============================================================================
 * 常量定义
 * ============================================================================
 */

/* 最大文件大小（1GB），超过此大小将拒绝加载 */
#define MAX_FILE_SIZE		(1024LL * 1024LL * 1024LL)

/* 添加缓冲区初始大小（1MB） */
#define INITIAL_ADD_BUFFER_SIZE	(1024 * 1024)

/* 添加缓冲区扩展大小（1MB） */
#define ADD_BUFFER_GROW_SIZE	(1024 * 1024)

/* Piece 阈值：超过此大小的 Piece 不再合并 */
#define MAX_PIECE_SIZE		(64 * 1024)

/*
 * ============================================================================
 * 数据结构定义
 * ============================================================================
 */

/**
 * PieceType - Piece 类型枚举
 *
 * 定义 Piece 指向的缓冲区类型
 */
enum class PieceType {
	ORIGINAL,	/* 指向原始缓冲区（文件内容） */
	ADD		/* 指向添加缓冲区（用户编辑内容） */
};

/**
 * Piece - 文本段
 *
 * Piece Table 的基本单元，表示文本的一个连续段
 *
 * 成员说明:
 * @type: Piece 类型（ORIGINAL 或 ADD）
 * @offset: 在缓冲区中的起始偏移
 * @length: 段长度（字符数）
 *
 * 设计要点:
 *   - 使用 offset + length 引用缓冲区内容，避免字符串拷贝
 *   - 支持高效的拼接和分割操作
 */
struct Piece {
	PieceType type;		/* Piece 类型 */
	size_t offset;		/* 缓冲区偏移 */
	size_t length;		/* 段长度 */

	/**
	 * 默认构造函数
	 */
	Piece(void) : type(PieceType::ORIGINAL), offset(0), length(0)
	{
	}

	/**
	 * 构造函数
	 */
	Piece(PieceType t, size_t off, size_t len)
		: type(t), offset(off), length(len)
	{
	}
};

/**
 * LineInfo - 行信息
 *
 * 表示一行文本的元数据信息，用于快速定位和渲染
 *
 * 成员说明:
 * @start_index: 行在文本中的起始索引
 * @length: 行长度（字符数）
 * @piece_index: 行起始位置所在的 Piece 索引
 */
struct LineInfo {
	size_t start_index;	/* 行起始索引 */
	size_t length;		/* 行长度 */
	size_t piece_index;	/* 起始 Piece 索引 */
};

/*
 * ============================================================================
 * TextBuffer 类定义
 * ============================================================================
 */

/**
 * TextBuffer - 高性能文本缓冲区
 *
 * 基于 Piece Table 数据结构实现的文本缓冲区，支持大文件的高效编辑。
 *
 * 设计特点:
 *   - 使用 mmap 映射原始文件，避免大文件一次性加载到内存
 *   - Piece Table 架构提供 O(log n) 的插入和删除操作
 *   - 行缓存机制加速行范围查询
 *   - 线程安全：使用互斥锁保护所有操作
 *
 * 使用示例:
 * @code
 * TextBuffer buffer;
 * if (buffer.load_file("/path/to/large/file.txt")) {
 *     std::vector<std::string> lines;
 *     buffer.get_lines(0, 100, lines);  // 获取前100行
 * }
 * @endcode
 */
class TextBuffer
{
public:
	/**
	 * TextBuffer - 构造函数
	 *
	 * 初始化文本缓冲区，预分配添加缓冲区。
	 */
	TextBuffer(void);

	/**
	 * ~TextBuffer - 析构函数
	 *
	 * 清理资源，释放 mmap 映射和缓冲区。
	 */
	~TextBuffer(void);

	/* 禁止拷贝构造函数 */
	TextBuffer(const TextBuffer &) = delete;

	/* 禁止拷贝赋值操作符 */
	TextBuffer &operator=(const TextBuffer &) = delete;

	/* 禁止移动构造函数 */
	TextBuffer(TextBuffer &&) = delete;

	/* 禁止移动赋值操作符 */
	TextBuffer &operator=(TextBuffer &&) = delete;

	/* ====================================================================
	 * 文件加载
	 * ==================================================================== */

	/**
	 * load_file - 使用 mmap 加载文件
	 *
	 * 使用 mmap 系统调用映射文件到内存，支持大文件的快速加载。
	 * 原始文件内容将被映射到只读内存区域。
	 *
	 * @path: 文件路径
	 *
	 * 返回值: 加载成功返回true，失败返回false
	 *
	 * 注意: 此方法会清空之前的缓冲区内容。
	 */
	bool load_file(const std::string &path);

	/**
	 * close - 关闭文件映射
	 *
	 * 释放 mmap 映射的资源，清空缓冲区。
	 */
	void close(void);

	/* ====================================================================
	 * 文本查询
	 * ==================================================================== */

	/**
	 * get_line_count - 获取总行数
	 *
	 * 返回当前缓冲区中的总行数。
	 *
	 * 返回值: 总行数
	 */
	size_t get_line_count(void);

	/**
	 * get_char_count - 获取总字符数
	 *
	 * 返回当前缓冲区中的总字符数（包括换行符）。
	 *
	 * 返回值: 总字符数
	 */
	size_t get_char_count(void);

	/**
	 * get_line - 获取指定行的内容
	 *
	 * 获取指定行号的文本内容（不包含换行符）。
	 *
	 * @line: 行号（从0开始）
	 * @content: 输出参数，存储行内容
	 *
	 * 返回值: 成功返回true，失败返回false
	 *
	 * 注意: 此方法不返回换行符。
	 */
	bool get_line(size_t line, std::string &content);

	/**
	 * get_lines - 获取指定行范围的内容
	 *
	 * 获取从 start_line 到 end_line 的所有行内容。
	 * 批量获取行内容，减少重复计算开销。
	 *
	 * @start_line: 起始行号（包含）
	 * @end_line: 结束行号（不包含）
	 * @lines: 输出参数，存储行内容数组
	 *
	 * 返回值: 成功返回true，失败返回false
	 *
	 * 注意: 此方法不返回换行符。
	 */
	bool get_lines(size_t start_line, size_t end_line,
		       std::vector<std::string> &lines);

	/**
	 * get_text - 获取指定索引范围的文本
	 *
	 * 获取从 start_pos 到 end_pos 的文本内容。
	 *
	 * @start_pos: 起始位置（包含）
	 * @end_pos: 结束位置（不包含）
	 * @text: 输出参数，存储文本内容
	 *
	 * 返回值: 成功返回true，失败返回false
	 */
	bool get_text(size_t start_pos, size_t end_pos, std::string &text);

	/* ====================================================================
	 * 编辑操作
	 * ==================================================================== */

	/**
	 * insert - 插入文本
	 *
	 * 在指定位置插入文本。
	 *
	 * @pos: 插入位置（字符索引）
	 * @text: 要插入的文本
	 *
	 * 返回值: 成功返回true，失败返回false
	 *
	 * 注意: 插入位置必须在有效范围内（0 到 get_char_count()）。
	 */
	bool insert(size_t pos, const std::string &text);

	/**
	 * delete_range - 删除指定范围
	 *
	 * 删除从 start_pos 到 end_pos 的文本。
	 *
	 * @start_pos: 起始位置（包含）
	 * @end_pos: 结束位置（不包含）
	 *
	 * 返回值: 成功返回true，失败返回false
	 *
	 * 注意: 删除位置必须在有效范围内。
	 */
	bool delete_range(size_t start_pos, size_t end_pos);

	/**
	 * replace - 替换指定范围
	 *
	 * 将从 start_pos 到 end_pos 的文本替换为新文本。
	 *
	 * @start_pos: 起始位置（包含）
	 * @end_pos: 结束位置（不包含）
	 * @text: 新文本
	 *
	 * 返回值: 成功返回true，失败返回false
	 *
	 * 注意: 相当于 delete_range 后 insert。
	 */
	bool replace(size_t start_pos, size_t end_pos, const std::string &text);

private:
	/* ====================================================================
	 * 私有成员变量
	 * ==================================================================== */

	/* 原始文件映射 */
	char *mmap_data;		/* mmap 映射的内存指针 */
	size_t mmap_size;		/* mmap 映射的大小 */
	int mmap_fd;			/* 文件描述符 */

	/* 添加缓冲区 */
	char *add_buffer;		/* 添加缓冲区指针 */
	size_t add_buffer_size;	/* 添加缓冲区总大小 */
	size_t add_buffer_used;	/* 添加缓冲区已使用大小 */

	/* Piece 列表 */
	std::deque<Piece> pieces;	/* Piece 双端队列 */

	/* 行缓存 */
	std::vector<LineInfo> line_cache;	/* 行信息缓存 */
	bool line_cache_valid;			/* 行缓存是否有效 */

	/* 统计信息 */
	size_t line_count;			/* 总行数 */
	size_t char_count;			/* 总字符数 */

	/* 互斥锁 */
	std::mutex mutex;			/* 保护所有操作 */

	/* 文件路径 */
	std::string file_path;		/* 当前加载的文件路径 */

	/* ====================================================================
	 * 私有方法 - 缓冲区管理
	 * ==================================================================== */

	/**
	 * grow_add_buffer - 扩展添加缓冲区
	 *
	 * 当添加缓冲区空间不足时，扩展缓冲区大小。
	 *
	 * @required_size: 需要的空间大小
	 *
	 * 返回值: 成功返回true，失败返回false
	 */
	bool grow_add_buffer(size_t required_size);

	/**
	 * append_to_add_buffer - 追加内容到添加缓冲区
	 *
	 * 将文本追加到添加缓冲区，并返回添加的位置和长度。
	 *
	 * @text: 要追加的文本
	 * @offset: 输出参数，存储添加位置的偏移
	 *
	 * 返回值: 成功返回true，失败返回false
	 */
	bool append_to_add_buffer(const std::string &text, size_t &offset);

	/* ====================================================================
	 * 私有方法 - Piece 操作
	 * ==================================================================== */

	/**
	 * split_piece - 分割 Piece
	 *
	 * 在指定位置分割 Piece，将一个 Piece 分割为两个。
	 *
	 * @index: Piece 索引
	 * @offset: 分割位置（相对于 Piece 起始）
	 *
	 * 返回值: 分割后的索引（第二个 Piece 的索引）
	 */
	size_t split_piece(size_t index, size_t offset);

	/**
	 * merge_pieces - 合并相邻的 Piece
	 *
	 * 合并类型相同且连续的相邻 Piece，减少 Piece 列表大小。
	 *
	 * @index: 起始 Piece 索引
	 */
	void merge_pieces(size_t index);

	/**
	 * find_piece_for_position - 查找包含指定位置的 Piece
	 *
	 * 查找包含字符位置 pos 的 Piece。
	 *
	 * @pos: 字符位置
	 * @piece_index: 输出参数，存储 Piece 索引
	 * @offset_in_piece: 输出参数，存储在 Piece 内的偏移
	 *
	 * 返回值: 成功返回true，失败返回false
	 */
	bool find_piece_for_position(size_t pos, size_t &piece_index,
				     size_t &offset_in_piece);

	/* ====================================================================
	 * 私有方法 - 行管理
	 * ==================================================================== */

	/**
	 * rebuild_line_cache - 重建行缓存
	 *
	 * 遍历所有 Piece，重建行信息缓存。
	 *
	 * 返回值: 成功返回true，失败返回false
	 */
	bool rebuild_line_cache(void);

	/**
	 * find_line_for_position - 查找包含指定位置的行
	 *
	 * 查找包含字符位置 pos 的行号。
	 *
	 * @pos: 字符位置
	 *
	 * 返回值: 行号（从0开始）
	 */
	size_t find_line_for_position(size_t pos);

	/**
	 * get_piece_text - 获取 Piece 的文本内容
	 *
	 * 从指定 Piece 中提取文本内容。
	 *
	 * @piece: Piece 引用
	 * @text: 输出参数，存储文本内容
	 *
	 * 返回值: 成功返回true，失败返回false
	 */
	bool get_piece_text(const Piece &piece, std::string &text);

	/**
	 * get_char - 获取指定位置的字符
	 *
	 * 获取字符位置 pos 的字符。
	 *
	 * @pos: 字符位置
	 * @ch: 输出参数，存储字符
	 *
	 * 返回值: 成功返回true，失败返回false
	 */
	bool get_char(size_t pos, char &ch);
};

#endif /* MIKUFY_TEXT_BUFFER_H */
