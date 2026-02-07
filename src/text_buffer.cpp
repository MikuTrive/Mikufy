/*
 * Mikufy v2.7-nova - 高性能文本缓冲模块实现
 *
 * 本文件实现了 TextBuffer 类的所有方法。
 *
 * MiraTrive/MikuTrive
 * Author: [Your Name]
 */

/*
 * ============================================================================
 * 头文件包含
 * ============================================================================
 */

#include "../headers/text_buffer.h"
#include <iostream>
#include <cstring>

/*
 * ============================================================================
 * 构造函数和析构函数
 * ============================================================================
 */

/**
 * TextBuffer - 构造函数
 *
 * 初始化文本缓冲区，预分配添加缓冲区。
 */
TextBuffer::TextBuffer(void)
	: mmap_data(nullptr)
	, mmap_size(0)
	, mmap_fd(-1)
	, add_buffer(nullptr)
	, add_buffer_size(INITIAL_ADD_BUFFER_SIZE)
	, add_buffer_used(0)
	, line_cache_valid(false)
	, line_count(0)
	, char_count(0)
{
	/*
	 * 预分配添加缓冲区
	 */
	add_buffer = new char[add_buffer_size];
	if (!add_buffer) {
		std::cerr << "无法分配添加缓冲区内存" << std::endl;
		add_buffer_size = 0;
	}
}

/**
 * ~TextBuffer - 析构函数
 *
 * 清理资源，释放 mmap 映射和缓冲区。
 */
TextBuffer::~TextBuffer(void)
{
	close();
	if (add_buffer) {
		delete[] add_buffer;
		add_buffer = nullptr;
	}
}

/*
 * ============================================================================
 * 文件加载
 * ============================================================================
 */

/**
 * load_file - 使用 mmap 加载文件
 *
 * 使用 mmap 系统调用映射文件到内存，支持大文件的快速加载。
 */
bool TextBuffer::load_file(const std::string &path)
{
	std::lock_guard<std::mutex> lock(mutex);

	/*
	 * 先关闭之前的映射
	 */
	close();

	/*
	 * 打开文件
	 */
	mmap_fd = open(path.c_str(), O_RDONLY);
	if (mmap_fd < 0) {
		std::cerr << "无法打开文件: " << path << std::endl;
		return false;
	}

	/*
	 * 获取文件大小
	 */
	struct stat st;
	if (fstat(mmap_fd, &st) < 0) {
		std::cerr << "无法获取文件大小: " << path << std::endl;
		::close(mmap_fd);
		mmap_fd = -1;
		return false;
	}

	mmap_size = st.st_size;

	/*
	 * 检查文件大小限制
	 */
	if (mmap_size > MAX_FILE_SIZE) {
		std::cerr << "文件过大: " << mmap_size << " 字节"
			  << std::endl;
		::close(mmap_fd);
		mmap_fd = -1;
		return false;
	}

	/*
	 * mmap 映射文件
	 */
	mmap_data = static_cast<char *>(
		mmap(nullptr, mmap_size, PROT_READ, MAP_PRIVATE, mmap_fd, 0));

	if (mmap_data == MAP_FAILED) {
		std::cerr << "mmap 失败: " << strerror(errno) << std::endl;
		::close(mmap_fd);
		mmap_fd = -1;
		mmap_data = nullptr;
		return false;
	}

	/*
	 * 创建初始 Piece（指向整个原始缓冲区）
	 */
	pieces.clear();
	if (mmap_size > 0) {
		pieces.push_back(Piece(PieceType::ORIGINAL, 0, mmap_size));
	}

	/*
	 * 保存文件路径
	 */
	file_path = path;

	/*
	 * 重建行缓存
	 */
	rebuild_line_cache();

	/*
	 * 计算统计信息
	 */
	line_count = line_cache.size();
	char_count = mmap_size;

	std::cout << "文件加载成功: " << path << ", 大小: " << mmap_size
		  << " 字节, 行数: " << line_count << std::endl;

	return true;
}

/**
 * close - 关闭文件映射
 *
 * 释放 mmap 映射的资源，清空缓冲区。
 */
void TextBuffer::close(void)
{
	std::lock_guard<std::mutex> lock(mutex);

	/*
	 * 解除 mmap 映射
	 */
	if (mmap_data && mmap_data != MAP_FAILED) {
		munmap(mmap_data, mmap_size);
		mmap_data = nullptr;
	}

	/*
	 * 关闭文件描述符
	 */
	if (mmap_fd >= 0) {
		::close(mmap_fd);
		mmap_fd = -1;
	}

	/*
	 * 重置大小
	 */
	mmap_size = 0;

	/*
	 * 清空 Piece 列表
	 */
	pieces.clear();

	/*
	 * 清空行缓存
	 */
	line_cache.clear();
	line_cache_valid = false;

	/*
	 * 重置统计信息
	 */
	line_count = 0;
	char_count = 0;

	/*
	 * 重置添加缓冲区
	 */
	add_buffer_used = 0;

	/*
	 * 清空文件路径
	 */
	file_path.clear();
}

/*
 * ============================================================================
 * 文本查询
 * ============================================================================
 */

size_t TextBuffer::get_line_count(void)
{
	std::lock_guard<std::mutex> lock(mutex);
	return line_count;
}

/**
 * get_char_count - 获取总字符数
 *
 * 获取文件的总字符数（包含换行符）。
 *
 * 返回值: 文件的总字符数
 */
size_t TextBuffer::get_char_count(void)
{
	std::lock_guard<std::mutex> lock(mutex);
	return char_count;
}

/**
 * get_line - 获取指定行的内容
 *
 * 获取指定行号的文本内容（不包含换行符）。
 */
bool TextBuffer::get_line(size_t line, std::string &content)
{
	std::lock_guard<std::mutex> lock(mutex);

	if (!line_cache_valid || line >= line_cache.size())
		return false;

	const LineInfo &info = line_cache[line];

	/*
	 * 获取行内容
	 */
	return get_text(info.start_index, info.start_index + info.length,
		       content);
}

/**
 * get_lines - 获取指定行范围的内容
 *
 * 获取从 start_line 到 end_line 的所有行内容。
 */
bool TextBuffer::get_lines(size_t start_line, size_t end_line,
			   std::vector<std::string> &lines)
{
	std::lock_guard<std::mutex> lock(mutex);

	if (!line_cache_valid)
		return false;

	/*
	 * 修正行号范围
	 */
	if (start_line >= line_cache.size())
		start_line = line_cache.size();
	if (end_line > line_cache.size())
		end_line = line_cache.size();
	if (start_line >= end_line)
		return false;

	/*
	 * 预分配空间
	 */
	lines.reserve(end_line - start_line);

	/*
	 * 逐行获取内容
	 */
	for (size_t i = start_line; i < end_line; ++i) {
		std::string line_content;
		if (!get_line(i, line_content))
			return false;
		lines.push_back(std::move(line_content));
	}

	return true;
}

/**
 * get_text - 获取指定索引范围的文本
 *
 * 获取从 start_pos 到 end_pos 的文本内容。
 */
bool TextBuffer::get_text(size_t start_pos, size_t end_pos,
			  std::string &text)
{
	std::lock_guard<std::mutex> lock(mutex);

	/*
	 * 检查范围有效性
	 */
	size_t total_chars = get_char_count();
	if (start_pos > total_chars)
		start_pos = total_chars;
	if (end_pos > total_chars)
		end_pos = total_chars;
	if (start_pos >= end_pos)
		return false;

	/*
	 * 查找起始 Piece
	 */
	size_t piece_index, offset_in_piece;
	if (!find_piece_for_position(start_pos, piece_index, offset_in_piece))
		return false;

	/*
	 * 遍历 Pieces 提取文本
	 */
	text.clear();
	text.reserve(end_pos - start_pos);

	size_t current_pos = start_pos;

	while (piece_index < pieces.size() && current_pos < end_pos) {
		const Piece &piece = pieces[piece_index];

		/*
		 * 计算当前 Piece 的可用范围
		 */
		size_t piece_start = offset_in_piece;
		size_t piece_end = piece.length;
		size_t copy_start = piece_start;
		size_t copy_end = piece_end;

		/*
		 * 调整到请求范围内
		 */
		if (current_pos < start_pos) {
			copy_start = offset_in_piece - (start_pos - current_pos);
		}

		size_t remaining = end_pos - current_pos;
		if (copy_end - copy_start > remaining) {
			copy_end = copy_start + remaining;
		}

		/*
		 * 提取文本
		 */
		const char *buffer = (piece.type == PieceType::ORIGINAL) ?
					     mmap_data :
					     add_buffer;
		size_t buffer_offset = piece.offset + copy_start;
		size_t copy_length = copy_end - copy_start;

		text.append(buffer + buffer_offset, copy_length);

		/*
		 * 更新位置
		 */
		current_pos += copy_length;
		piece_index++;
		offset_in_piece = 0;
	}

	return true;
}

/*
 * ============================================================================
 * 编辑操作
 * ============================================================================
 */

/**
 * insert - 插入文本
 *
 * 在指定位置插入文本。
 */
bool TextBuffer::insert(size_t pos, const std::string &text)
{
	std::lock_guard<std::mutex> lock(mutex);

	if (text.empty())
		return true;

	/*
	 * 检查位置有效性
	 */
	size_t total_chars = get_char_count();
	if (pos > total_chars)
		pos = total_chars;

	/*
	 * 查找插入位置的 Piece
	 */
	size_t piece_index, offset_in_piece;
	if (!find_piece_for_position(pos, piece_index, offset_in_piece))
		return false;

	/*
	 * 如果不在 Piece 开头，需要分割
	 */
	if (offset_in_piece > 0) {
		split_piece(piece_index, offset_in_piece);
	}

	/*
	 * 追加文本到添加缓冲区
	 */
	size_t add_offset;
	if (!append_to_add_buffer(text, add_offset))
		return false;

	/*
	 * 创建新 Piece 并插入
	 */
	Piece new_piece(PieceType::ADD, add_offset, text.length());
	pieces.insert(pieces.begin() + piece_index, new_piece);

	/*
	 * 尝试合并相邻 Pieces
	 */
	merge_pieces(piece_index);

	/*
	 * 重建行缓存
	 */
	rebuild_line_cache();

	return true;
}

/**
 * delete_range - 删除指定范围
 *
 * 删除从 start_pos 到 end_pos 的文本。
 */
bool TextBuffer::delete_range(size_t start_pos, size_t end_pos)
{
	std::lock_guard<std::mutex> lock(mutex);

	/*
	 * 检查范围有效性
	 */
	size_t total_chars = get_char_count();
	if (start_pos >= total_chars)
		return true;
	if (end_pos > total_chars)
		end_pos = total_chars;
	if (start_pos >= end_pos)
		return true;

	/*
	 * 查找起始 Piece
	 */
	size_t start_piece, start_offset;
	if (!find_piece_for_position(start_pos, start_piece, start_offset))
		return false;

	/*
	 * 查找结束 Piece
	 */
	size_t end_piece, end_offset;
	if (!find_piece_for_position(end_pos, end_piece, end_offset))
		return false;

	/*
	 * 分割起始 Piece（如果需要）
	 */
	if (start_offset > 0) {
		split_piece(start_piece, start_offset);
		start_piece++;
	}

	/*
	 * 分割结束 Piece（如果需要）
	 */
	if (end_offset > 0 && end_offset <
				pieces[end_piece].length) {
		split_piece(end_piece, end_offset);
	}

	/*
	 * 删除中间的 Pieces
	 */
	if (end_piece > start_piece) {
		pieces.erase(pieces.begin() + start_piece,
			     pieces.begin() + end_piece);
	}

	/*
	 * 重建行缓存
	 */
	rebuild_line_cache();

	return true;
}

/**
 * replace - 替换指定范围
 *
 * 将从 start_pos 到 end_pos 的文本替换为新文本。
 */
bool TextBuffer::replace(size_t start_pos, size_t end_pos,
			 const std::string &text)
{
	std::lock_guard<std::mutex> lock(mutex);

	/*
	 * 先删除，再插入
	 */
	if (!delete_range(start_pos, end_pos))
		return false;

	if (!text.empty() && !insert(start_pos, text))
		return false;

	return true;
}

/*
 * ============================================================================
 * 私有方法 - 缓冲区管理
 * ============================================================================
 */

/**
 * grow_add_buffer - 扩展添加缓冲区
 *
 * 当添加缓冲区空间不足时，扩展缓冲区大小。
 */
bool TextBuffer::grow_add_buffer(size_t required_size)
{
	/*
	 * 计算新大小
	 */
	size_t new_size = add_buffer_size + ADD_BUFFER_GROW_SIZE;
	while (new_size < add_buffer_used + required_size) {
		new_size += ADD_BUFFER_GROW_SIZE;
	}

	/*
	 * 分配新缓冲区
	 */
	char *new_buffer = new char[new_size];
	if (!new_buffer) {
		std::cerr << "无法扩展添加缓冲区" << std::endl;
		return false;
	}

	/*
	 * 复制旧内容
	 */
	memcpy(new_buffer, add_buffer, add_buffer_used);

	/*
	 * 释放旧缓冲区
	 */
	delete[] add_buffer;

	/*
	 * 更新指针和大小
	 */
	add_buffer = new_buffer;
	add_buffer_size = new_size;

	return true;
}

/**
 * append_to_add_buffer - 追加内容到添加缓冲区
 *
 * 将文本追加到添加缓冲区，并返回添加的位置和长度。
 */
bool TextBuffer::append_to_add_buffer(const std::string &text, size_t &offset)
{
	/*
	 * 检查空间是否足够
	 */
	if (add_buffer_used + text.length() > add_buffer_size) {
		if (!grow_add_buffer(text.length()))
			return false;
	}

	/*
	 * 保存偏移
	 */
	offset = add_buffer_used;

	/*
	 * 追加文本
	 */
	memcpy(add_buffer + add_buffer_used, text.data(), text.length());
	add_buffer_used += text.length();

	return true;
}

/*
 * ============================================================================
 * 私有方法 - Piece 操作
 * ============================================================================
 */

/**
 * split_piece - 分割 Piece
 *
 * 在指定位置分割 Piece，将一个 Piece 分割为两个。
 */
size_t TextBuffer::split_piece(size_t index, size_t offset)
{
	if (index >= pieces.size())
		return index;

	Piece &piece = pieces[index];

	/*
	 * 如果在开头或结尾，不需要分割
	 */
	if (offset == 0 || offset >= piece.length)
		return index;

	/*
	 * 创建新的 Piece（后半部分）
	 */
	Piece second_piece(piece.type, piece.offset + offset,
			   piece.length - offset);

	/*
	 * 修改原 Piece（前半部分）
	 */
	piece.length = offset;

	/*
	 * 插入新 Piece
	 */
	pieces.insert(pieces.begin() + index + 1, second_piece);

	return index + 1;
}

/**
 * merge_pieces - 合并相邻的 Piece
 *
 * 合并类型相同且连续的相邻 Piece，减少 Piece 列表大小。
 */
void TextBuffer::merge_pieces(size_t index)
{
	if (index == 0 || index >= pieces.size())
		return;

	Piece &prev = pieces[index - 1];
	Piece &curr = pieces[index];

	/*
	 * 检查是否可以合并
	 */
	if (prev.type != curr.type)
		return;

	/*
	 * 检查是否连续
	 */
	if (prev.offset + prev.length != curr.offset)
		return;

	/*
	 * 合并
	 */
	prev.length += curr.length;
	pieces.erase(pieces.begin() + index);
}

/**
 * find_piece_for_position - 查找包含指定位置的 Piece
 *
 * 查找包含字符位置 pos 的 Piece。
 */
bool TextBuffer::find_piece_for_position(size_t pos, size_t &piece_index,
				 size_t &offset_in_piece)
{
	/*
	 * 边界情况
	 */
	if (pos == 0) {
		piece_index = 0;
		offset_in_piece = 0;
		return !pieces.empty();
	}

	/*
	 * 线性搜索（对于小列表足够快）
	 * 对于大列表，可以使用二分搜索优化
	 */
	size_t current_pos = 0;
	for (size_t i = 0; i < pieces.size(); ++i) {
		const Piece &piece = pieces[i];

		if (current_pos + piece.length > pos) {
			piece_index = i;
			offset_in_piece = pos - current_pos;
			return true;
		}

		current_pos += piece.length;
	}

	/*
	 * 位置超出范围
	 */
	if (!pieces.empty()) {
		piece_index = pieces.size() - 1;
		offset_in_piece = pieces.back().length;
		return true;
	}

	return false;
}

/*
 * ============================================================================
 * 私有方法 - 行管理
 * ============================================================================
 */

/**
 * rebuild_line_cache - 重建行缓存
 *
 * 遍历所有 Piece，重建行信息缓存。
 */
bool TextBuffer::rebuild_line_cache(void)
{
	line_cache.clear();

	size_t current_pos = 0;
	size_t line_start = 0;
	size_t piece_index = 0;

	for (const auto &piece : pieces) {
		const char *buffer = (piece.type == PieceType::ORIGINAL) ?
					     mmap_data :
					     add_buffer;
		size_t buffer_offset = piece.offset;

		/*
		 * 遍历 Piece 中的字符
		 */
		for (size_t i = 0; i < piece.length; ++i) {
			char ch = buffer[buffer_offset + i];

			/*
			 * 遇到换行符，结束当前行
			 */
			if (ch == '\n') {
				LineInfo info;
				info.start_index = line_start;
				info.length = current_pos - line_start + 1;
				info.piece_index = piece_index;
				line_cache.push_back(info);

				/*
				 * 开始新行
				 */
				line_start = current_pos + 1;
			}

			current_pos++;
		}

		piece_index++;
	}

	/*
	 * 处理最后一行（如果没有换行符结尾）
	 */
	if (current_pos > line_start) {
		LineInfo info;
		info.start_index = line_start;
		info.length = current_pos - line_start;
		info.piece_index = piece_index - 1;
		line_cache.push_back(info);
	}

	line_cache_valid = true;

	return true;
}

/**
 * find_line_for_position - 查找包含指定位置的行
 *
 * 查找包含字符位置 pos 的行号。
 */
size_t TextBuffer::find_line_for_position(size_t pos)
{
	if (!line_cache_valid)
		return 0;

	/*
	 * 二分查找
	 */
	auto it = std::upper_bound(
		line_cache.begin(), line_cache.end(), pos,
		[](size_t pos, const LineInfo &info) {
			return pos < info.start_index;
		});

	if (it == line_cache.begin())
		return 0;

	it--;

	if (pos >= it->start_index + it->length)
		return line_cache.size();

	return std::distance(line_cache.begin(), it);
}

/**
 * get_piece_text - 获取 Piece 的文本内容
 *
 * 从指定 Piece 中提取文本内容。
 */
bool TextBuffer::get_piece_text(const Piece &piece, std::string &text)
{
	const char *buffer = (piece.type == PieceType::ORIGINAL) ?
				     mmap_data :
				     add_buffer;

	if (!buffer || piece.offset + piece.length >
			     ((piece.type == PieceType::ORIGINAL) ?
					    mmap_size :
					    add_buffer_size))
		return false;

	text.assign(buffer + piece.offset, piece.length);
	return true;
}

/**
 * get_char - 获取指定位置的字符
 *
 * 获取字符位置 pos 的字符。
 */
bool TextBuffer::get_char(size_t pos, char &ch)
{
	size_t piece_index, offset_in_piece;
	if (!find_piece_for_position(pos, piece_index, offset_in_piece))
		return false;

	if (piece_index >= pieces.size())
		return false;

	const Piece &piece = pieces[piece_index];
	const char *buffer = (piece.type == PieceType::ORIGINAL) ?
				     mmap_data :
				     add_buffer;

	if (!buffer || offset_in_piece >= piece.length)
		return false;

	ch = buffer[piece.offset + offset_in_piece];
	return true;
}
