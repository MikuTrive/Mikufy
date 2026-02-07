/**
 * Mikufy v2.7-nova - 高性能虚拟滚动编辑器
 *
 * 本文件实现了基于 Canvas 的高性能虚拟滚动编辑器，支持大文件的流畅加载和编辑。
 *
 * 核心架构：
 *   - 虚拟滚动：只渲染可见区域 + 预加载缓冲行
 *   - Canvas 渲染：使用 requestAnimationFrame 驱动重绘
 *   - 批量数据传输：从 C++ 后端批量获取行数据
 *
 * 性能目标：
 *   - 支持 10万~50万行文件的流畅滚动（60 FPS）
 *   - 内存使用稳定，无指数增长
 *   - 滚动无掉帧
 *
 * 作者：MikuTrive
 * 版本：v2.7-nova
 */

// ============================================================================
// 高性能虚拟编辑器类
// ============================================================================

/**
 * VirtualEditor - 高性能虚拟滚动编辑器
 *
 * 基于 Canvas 实现的虚拟滚动编辑器，支持大文件的高效渲染和编辑。
 */
class VirtualEditor {
	/**
	 * 构造函数
	 * @containerId - 编辑器容器的 DOM ID
	 */
	constructor(containerId) {
		this.container = document.getElementById(containerId);
		if (!this.container) {
			throw new Error(`Container ${containerId} not found`);
		}

		// 配置参数
		this.config = {
			lineHeight: 21,           // 行高（像素）
			charWidth: 8.5,           // 字符宽度（像素，等宽字体）
			visibleLines: 40,         // 可见行数
			bufferLines: 20,          // 预加载缓冲行数
			fontFamily: 'Cascadia Mono, Consolas, Monaco, Courier New, monospace',
			fontSize: 14,
			lineNumberWidth: 50       // 行号区域宽度
		};

		// 状态
		this.currentFile = null;     // 当前文件路径
		this.totalLines = 0;         // 总行数
		this.scrollTop = 0;           // 当前滚动位置
		this.startLine = 0;           // 可见区域起始行
		this.endLine = 0;             // 可见区域结束行
		this.visibleLines = [];       // 可见行内容
		this.isScrolling = false;     // 是否正在滚动
		this.renderScheduled = false; // 是否已调度渲染

		// 创建 DOM 结构
		this.createDOM();

		// 初始化 Canvas
		this.initCanvas();

		// 绑定事件
		this.bindEvents();

		// 启动渲染循环
		this.startRenderLoop();
	}

	/**
	 * 创建 DOM 结构
	 */
	createDOM() {
		// 清空容器
		this.container.innerHTML = '';

		// 编辑器容器
		this.editor = document.createElement('div');
		this.editor.className = 'virtual-editor';

		// Spacer（撑开滚动高度）
		this.spacer = document.createElement('div');
		this.spacer.className = 'editor-spacer';

		// 文本层 Canvas
		this.textCanvas = document.createElement('canvas');
		this.textCanvas.id = 'text-layer';
		this.textCanvas.className = 'editor-canvas';

		// 行号层 Canvas
		this.lineNumberCanvas = document.createElement('canvas');
		this.lineNumberCanvas.id = 'line-number-layer';
		this.lineNumberCanvas.className = 'editor-canvas editor-line-numbers';

		// 滚动容器
		this.scrollContainer = document.createElement('div');
		this.scrollContainer.className = 'editor-scroll-container';
		this.scrollContainer.appendChild(this.spacer);
		this.scrollContainer.appendChild(this.lineNumberCanvas);
		this.scrollContainer.appendChild(this.textCanvas);

		this.editor.appendChild(this.scrollContainer);
		this.container.appendChild(this.editor);
	}

	/**
	 * 初始化 Canvas
	 */
	initCanvas() {
		// 获取容器尺寸
		const rect = this.container.getBoundingClientRect();

		// 设置 Canvas 尺寸
		this.width = rect.width;
		this.height = rect.height;

		this.textCanvas.width = this.width - this.config.lineNumberWidth;
		this.textCanvas.height = this.height;
		this.lineNumberCanvas.width = this.config.lineNumberWidth;
		this.lineNumberCanvas.height = this.height;

		// 获取 Context
		this.textCtx = this.textCanvas.getContext('2d', { alpha: true });
		this.lineNumberCtx = this.lineNumberCanvas.getContext('2d', { alpha: true });

		// 配置字体
		const font = `${this.config.fontSize}px ${this.config.fontFamily}`;
		this.textCtx.font = font;
		this.lineNumberCtx.font = font;

		// 计算可见行数
		this.config.visibleLines = Math.ceil(this.height / this.config.lineHeight);
	}

	/**
	 * 绑定事件
	 */
	bindEvents() {
		// 滚动事件
		this.scrollContainer.addEventListener('scroll', () => {
			this.handleScroll();
		});

		// 窗口大小改变
		window.addEventListener('resize', () => {
			this.handleResize();
		});

		// 滚轮事件（优化滚动性能）
		this.scrollContainer.addEventListener('wheel', (e) => {
			e.preventDefault();
			this.scrollContainer.scrollTop += e.deltaY;
		}, { passive: false });
	}

	/**
	 * 处理滚动事件
	 */
	handleScroll() {
		this.scrollTop = this.scrollContainer.scrollTop;
		this.isScrolling = true;

		// 计算可见行范围
		this.updateVisibleRange();

		// 调度渲染
		this.scheduleRender();

		// 重置滚动状态
		setTimeout(() => {
			this.isScrolling = false;
		}, 100);
	}

	/**
	 * 处理窗口大小改变
	 */
	handleResize() {
		// 重新初始化 Canvas
		this.initCanvas();

		// 更新可见行范围
		this.updateVisibleRange();

		// 调度渲染
		this.scheduleRender();
	}

	/**
	 * 更新可见行范围
	 */
	updateVisibleRange() {
		// 计算起始行
		this.startLine = Math.floor(this.scrollTop / this.config.lineHeight);

		// 计算结束行（包含缓冲区）
		this.endLine = Math.min(
			this.startLine + this.config.visibleLines + this.config.bufferLines * 2,
			this.totalLines
		);

		// 修正起始行
		this.startLine = Math.max(0, this.startLine - this.config.bufferLines);
	}

	/**
	 * 调度渲染
	 */
	scheduleRender() {
		if (!this.renderScheduled) {
			this.renderScheduled = true;
			requestAnimationFrame(() => this.render());
		}
	}

	/**
	 * 启动渲染循环
	 */
	startRenderLoop() {
		const loop = () => {
			if (this.renderScheduled) {
				this.render();
				this.renderScheduled = false;
			}
			requestAnimationFrame(loop);
		};
		requestAnimationFrame(loop);
	}

	/**
	 * 渲染
	 */
	async render() {
		if (!this.currentFile || this.startLine >= this.endLine) {
			return;
		}

		try {
			// 从后端获取行数据
			const response = await fetch('/api/get-lines', {
				method: 'POST',
				headers: {
					'Content-Type': 'application/json'
				},
				body: JSON.stringify({
					path: this.currentFile,
					start_line: this.startLine,
					end_line: this.endLine
				})
			});

			const data = await response.json();

			if (!data.success) {
				console.error('Failed to get lines:', data.error);
				return;
			}

			this.visibleLines = data.lines;
			this.language = data.language;

			// 更新 spacer 高度
			this.spacer.style.height = `${this.totalLines * this.config.lineHeight}px`;

			// 清空 Canvas
			this.clearCanvas();

			// 绘制行号
			this.drawLineNumbers();

			// 绘制文本
			this.drawText();

		} catch (error) {
			console.error('Render error:', error);
		}
	}

	/**
	 * 清空 Canvas
	 */
	clearCanvas() {
		this.textCtx.clearRect(0, 0, this.textCanvas.width, this.textCanvas.height);
		this.lineNumberCtx.clearRect(0, 0, this.lineNumberCanvas.width, this.lineNumberCanvas.height);
	}

	/**
	 * 绘制行号
	 */
	drawLineNumbers() {
		const ctx = this.lineNumberCtx;
		const lineHeight = this.config.lineHeight;

		// 设置样式
		ctx.fillStyle = '#808080';
		ctx.font = `${this.config.fontSize}px ${this.config.fontFamily}`;
		ctx.textAlign = 'right';
		ctx.textBaseline = 'top';

		// 绘制每一行的行号
		for (let i = 0; i < this.visibleLines.length; i++) {
			const lineNum = this.startLine + i + 1;
			const y = (i * lineHeight) - (this.startLine * lineHeight) + this.scrollTop;

			if (y >= -lineHeight && y <= this.height) {
				ctx.fillText(lineNum.toString(), this.config.lineNumberWidth - 10, y);
			}
		}
	}

	/**
	 * 绘制文本
	 */
	drawText() {
		const ctx = this.textCtx;
		const lineHeight = this.config.lineHeight;
		const charWidth = this.config.charWidth;

		// 设置样式
		ctx.fillStyle = '#ffffff';
		ctx.font = `${this.config.fontSize}px ${this.config.fontFamily}`;
		ctx.textAlign = 'left';
		ctx.textBaseline = 'top';

		// 绘制每一行的文本
		for (let i = 0; i < this.visibleLines.length; i++) {
			const line = this.visibleLines[i];
			const y = (i * lineHeight) - (this.startLine * lineHeight) + this.scrollTop;

			if (y >= -lineHeight && y <= this.height) {
				// 转义 HTML 特殊字符
				const escapedLine = this.escapeHTML(line);
				ctx.fillText(escapedLine, 0, y);
			}
		}
	}

	/**
	 * 转义 HTML 特殊字符
	 */
	escapeHTML(str) {
		return str.replace(/&/g, '&amp;')
			.replace(/</g, '&lt;')
			.replace(/>/g, '&gt;')
			.replace(/"/g, '&quot;')
			.replace(/'/g, '&#039;');
	}

	/**
	 * 打开文件
	 */
	async openFile(filePath) {
		try {
			// 调用后端 API 打开文件
			const response = await fetch('/api/open-file-virtual', {
				method: 'POST',
				headers: {
					'Content-Type': 'application/json'
				},
				body: JSON.stringify({
					path: filePath
				})
			});

			const data = await response.json();

			if (!data.success) {
				console.error('Failed to open file:', data.error);
				return false;
			}

			// 更新状态
			this.currentFile = filePath;
			this.totalLines = data.totalLines;
			this.totalChars = data.totalChars;
			this.language = data.language;

			// 重置滚动位置
			this.scrollTop = 0;
			this.scrollContainer.scrollTop = 0;

			// 更新可见行范围
			this.updateVisibleRange();

			// 调度渲染
			this.scheduleRender();

			console.log(`File opened: ${filePath}, Lines: ${this.totalLines}`);
			return true;

		} catch (error) {
			console.error('Open file error:', error);
			return false;
		}
	}

	/**
	 * 关闭文件
	 */
	async closeFile() {
		if (!this.currentFile) {
			return;
		}

		try {
			// 调用后端 API 关闭文件
			await fetch('/api/close-file-virtual', {
				method: 'POST',
				headers: {
					'Content-Type': 'application/json'
				},
				body: JSON.stringify({
					path: this.currentFile
				})
			});

			// 清空状态
			this.currentFile = null;
			this.totalLines = 0;
			this.visibleLines = [];

			// 清空 Canvas
			this.clearCanvas();

		} catch (error) {
			console.error('Close file error:', error);
		}
	}

	/**
	 * 获取总行数
	 */
	getTotalLines() {
		return this.totalLines;
	}

	/**
	 * 获取当前文件路径
	 */
	getCurrentFile() {
		return this.currentFile;
	}
}

// ============================================================================
// 导出
// ============================================================================

// 如果在浏览器环境中，将 VirtualEditor 挂载到全局
if (typeof window !== 'undefined') {
	window.VirtualEditor = VirtualEditor;
}
