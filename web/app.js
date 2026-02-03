/**
 * Mikufy v2.3(stable) - 代码编辑器前端JavaScript
 *
 * 本文件处理所有前端交互逻辑，通过C++后端API与系统交互
 *
 * 主要功能：
 *   - 文件系统操作：浏览、创建、删除、重命名文件和文件夹
 *   - 多标签页编辑：支持同时打开多个文件
 *   - 语法高亮：支持C/C++/JavaScript/TypeScript等语言
 *   - 媒体预览：支持图片、视频、音频文件预览
 *   - 快捷键支持：提供丰富的键盘快捷键
 *   - 状态管理：全局应用状态管理
 *
 * 架构设计：
 *   - AppState：全局状态管理对象
 *   - BackendAPI：后端API调用封装
 *   - DOM：DOM元素引用缓存
 *   - 函数模块：按功能划分的函数集合
 *
 * 作者：MikuTrive
 * 版本：v2.3(stable)
 * 许可证：详见 LICENSE 文件
 */

// ============================================================================
// 全局状态管理
// ============================================================================

/**
 * 全局应用状态对象
 *
 * 存储应用程序的所有状态信息，包括：
 *   - 当前工作目录和浏览路径
 *   - 打开的标签页和当前激活标签
 *   - 文件内容缓存
 *   - UI状态（加载中、对话框类型等）
 *   - 配置信息（图标映射、高亮缓存等）
 */
const AppState = {
    // 当前打开的工作目录（根目录）
    currentDirectory: null,
    // 当前显示的路径（可能包含子目录）
    currentPath: null,
    // 当前目录的路径历史，用于返回上级目录
    pathHistory: [],
    // 所有打开的标签页数组
    openTabs: [],
    // 当前激活的标签页索引（-1表示无激活标签）
    activeTab: -1,
    // 文件内容缓存（Map类型，用于存储未保存的内容）
    // 键：文件路径，值：文件内容
    contentCache: new Map(),
    // 文件树数据（当前目录下的文件列表）
    fileTree: [],
    // 右键菜单选中的项（文件或文件夹信息）
    contextSelected: null,
    // 新建对话框类型（'folder' 或 'file'）
    newDialogType: null,
    // 加载状态标志（防止重复加载）
    isLoading: false,
    // 语法高亮缓存（Map类型，用于性能优化）
    // 键：代码片段的前1000个字符，值：高亮后的HTML
    syntaxHighlightCache: new Map(),
    // 图标映射表（根据文件扩展名映射到对应的图标文件）
    // 键：文件扩展名，值：图标文件名
    iconMap: {
        '.ai': 'AI-24.svg',
        '.conf': 'Config-24.svg',
        '.config': 'Config-24.svg',
        '.theme': 'Config-24.svg',
        '.d': 'Config-24.svg',
        '.cpp': 'C++-24.svg',
        '.c++': 'C++-24.svg',
        '.CPP': 'C++-24.svg',
        '.C++': 'C++-24.svg',
        '.c': 'C-24.svg',
        '.C': 'C-24.svg',
        '.git': 'Git-24.svg',
        '.go': 'Go-24.svg',
        '.h': 'H-24.svg',
        '.H': 'H-24.svg',
        '.hpp': 'Hpp-24.svg',
        '.HPP': 'Hpp-24.svg',
        '.h++': 'Hpp-24.svg',
        '.H++': 'Hpp-24.svg',
        '.java': 'Java-24.svg',
        '.JAVA': 'Java-24.svg',
        '.js': 'JavaScript-24.svg',
        '.javascript': 'JavaScript-24.svg',
        '.jsx': 'JavaScript-24.svg',
        '.lua': 'Lua-24.svg',
        '.LUA': 'Lua-24.svg',
        '.md': 'MD-24.svg',
        '.MD': 'MD-24.svg',
        '.asm': 'Nasm-24.svg',
        '.s': 'Nasm-24.svg',
        '.o': 'Nasm-24.svg',
        '.txt': 'TXT-24.svg',
        '.TXT': 'TXT-24.svg',
        '.jpeg': 'Image-24.svg',
        '.jpg': 'Image-24.svg',
        '.png': 'Image-24.svg',
        '.svg': 'Image-24.svg',
        '.mp3': 'Music-24.svg',
        '.mp4': 'Video-24.svg',
        '.wav': 'Music-24.svg',
        '.php': 'PHP-24.svg',
        '.python': 'Python-24.svg',
        '.py': 'Python-24.svg',
        '.typescript': 'TypeScript-24.svg',
        '.ts': 'TypeScript-24.svg',
        '.html': 'HTML-24.svg',
        '.css': 'CSS-24.svg',
        '.json': 'JSON-24.svg',
        '.xml': 'XML-24.svg',
        '.yaml': 'XML-24.svg',
        '.yml': 'XML-24.svg',
        '.rust': 'Rust-24.svg',
        '.rs': 'Rust-24.svg',
        '.kotlin': 'KT-24.svg',
        '.kt': 'KT-24.svg',
        '.gitignore': 'Git-24.svg',
        '.gitattributes': 'Git-24.svg',
        '.ruby': 'Ruby-24.svg',
        '.rb': 'Ruby-24.svg',
        '.sh': 'Shell-24.svg',
        '.cmake': 'Cmake-24.svg',
        'CMakeLists.txt': 'Cmake-24.svg',
        'Makefile': 'MakeFile-24.svg',
        '.mk': 'MakeFile-24.svg',
        '.Cmake': 'Cmake-24.svg'
    }
};

// ============================================================================
// C++后端API调用封装
// ============================================================================

/**
 * 后端API调用封装对象
 *
 * 提供与C++后端交互的所有API方法，使用fetch进行HTTP请求
 * 所有方法均为异步函数，返回Promise对象
 *
 * API端点规范：
 *   - GET请求：查询类操作（打开文件夹、读取文件等）
 *   - POST请求：修改类操作（保存文件、创建文件等）
 *   - 响应格式：JSON格式，包含success、data、path等字段
 */
const BackendAPI = {
    /**
     * 打开文件夹对话框
     *
     * 调用后端API打开系统的文件夹选择对话框
     * 用户选择文件夹后，返回文件夹的绝对路径
     *
     * @async
     * @returns {Promise<string|null>} 选中的文件夹路径，用户取消则返回null
     * @throws {Error} 网络请求失败时抛出错误
     *
     * @example
     * const folderPath = await BackendAPI.openFolderDialog();
     * if (folderPath) {
     *     console.log('选择的文件夹:', folderPath);
     * }
     */
    async openFolderDialog() {
        try {
            const response = await fetch('/api/open-folder');
            const data = await response.json();
            return data.path || null;
        } catch (error) {
            console.error('打开文件夹失败:', error);
            return null;
        }
    },

    /**
     * 获取指定目录下的文件列表
     *
     * 调用后端API获取指定目录下的所有文件和子目录
     * 返回的文件列表包含每个文件/目录的详细信息
     *
     * @async
     * @param {string} path 要查询的目录路径（绝对路径）
     * @returns {Promise<Array<Object>>} 文件列表数组，每个元素包含：
     *   - name {string}: 文件或目录名称
     *   - path {string}: 完整路径
     *   - isDirectory {boolean}: 是否为目录
     *   - size {number}: 文件大小（字节）
     *   - modifiedTime {string}: 修改时间
     * @throws {Error} 网络请求失败时抛出错误
     *
     * @example
     * const files = await BackendAPI.getDirectoryContents('/home/user/project');
     * files.forEach(file => {
     *     console.log(file.name, file.isDirectory ? '目录' : '文件');
     * });
     */
    async getDirectoryContents(path) {
        try {
            const response = await fetch(`/api/directory-contents?path=${encodeURIComponent(path)}`);
            const data = await response.json();
            return data.files || [];
        } catch (error) {
            console.error('获取目录内容失败:', error);
            return [];
        }
    },

    /**
     * 读取文件内容
     *
     * 调用后端API读取指定路径的文本文件内容
     * 注意：此方法仅适用于文本文件，二进制文件请使用readBinaryFile
     *
     * @async
     * @param {string} path 要读取的文件路径（绝对路径）
     * @returns {Promise<string>} 文件的文本内容，失败返回空字符串
     * @throws {Error} 网络请求失败时抛出错误
     *
     * @example
     * const content = await BackendAPI.readFile('/home/user/project/main.cpp');
     * console.log('文件内容:', content);
     */
    async readFile(path) {
        try {
            const response = await fetch(`/api/read-file?path=${encodeURIComponent(path)}`);
            const data = await response.json();
            return data.content || '';
        } catch (error) {
            console.error('读取文件失败:', error);
            return '';
        }
    },

    /**
     * 保存文件内容
     *
     * 调用后端API将内容保存到指定路径的文件
     * 如果文件不存在则创建，存在则覆盖
     *
     * @async
     * @param {string} path 要保存的文件路径（绝对路径）
     * @param {string} content 要保存的文件内容（字符串）
     * @returns {Promise<boolean>} 保存成功返回true，失败返回false
     * @throws {Error} 网络请求失败时抛出错误
     *
     * @example
     * const success = await BackendAPI.saveFile('/home/user/project/test.txt', 'Hello World');
     * if (success) {
     *     console.log('保存成功');
     * }
     */
    async saveFile(path, content) {
        try {
            const response = await fetch('/api/save-file', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({ path, content })
            });
            const data = await response.json();
            return data.success || false;
        } catch (error) {
            console.error('保存文件失败:', error);
            return false;
        }
    },

    /**
     * 新建文件夹
     *
     * 在指定父目录下创建一个新的文件夹
     *
     * @async
     * @param {string} parentPath 父目录的路径（绝对路径）
     * @param {string} name 要创建的文件夹名称
     * @returns {Promise<boolean>} 创建成功返回true，失败返回false
     * @throws {Error} 网络请求失败时抛出错误
     *
     * @example
     * const success = await BackendAPI.createFolder('/home/user/project', 'newFolder');
     * if (success) {
     *     console.log('文件夹创建成功');
     * }
     */
    async createFolder(parentPath, name) {
        try {
            const response = await fetch('/api/create-folder', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({ parentPath, name })
            });
            const data = await response.json();
            return data.success || false;
        } catch (error) {
            console.error('创建文件夹失败:', error);
            return false;
        }
    },

    /**
     * 新建文件
     *
     * 在指定父目录下创建一个新的空文件
     *
     * @async
     * @param {string} parentPath 父目录的路径（绝对路径）
     * @param {string} name 要创建的文件名称
     * @returns {Promise<boolean>} 创建成功返回true，失败返回false
     * @throws {Error} 网络请求失败时抛出错误
     *
     * @example
     * const success = await BackendAPI.createFile('/home/user/project', 'test.txt');
     * if (success) {
     *     console.log('文件创建成功');
     * }
     */
    async createFile(parentPath, name) {
        try {
            const response = await fetch('/api/create-file', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({ parentPath, name })
            });
            const data = await response.json();
            return data.success || false;
        } catch (error) {
            console.error('创建文件失败:', error);
            return false;
        }
    },

    /**
     * 删除文件或文件夹
     *
     * 删除指定路径的文件或文件夹
     * 如果是文件夹，将递归删除其所有内容
     *
     * @async
     * @param {string} path 要删除的文件或文件夹路径（绝对路径）
     * @returns {Promise<boolean>} 删除成功返回true，失败返回false
     * @throws {Error} 网络请求失败时抛出错误
     *
     * @example
     * const success = await BackendAPI.deleteItem('/home/user/project/test.txt');
     * if (success) {
     *     console.log('删除成功');
     * }
     */
    async deleteItem(path) {
        try {
            const response = await fetch('/api/delete', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({ path })
            });
            const data = await response.json();
            return data.success || false;
        } catch (error) {
            console.error('删除失败:', error);
            return false;
        }
    },

    /**
     * 重命名文件或文件夹
     *
     * 将文件或文件夹从旧路径移动到新路径（实现重命名功能）
     * 可以在同一目录下重命名，也可以移动到不同目录
     *
     * @async
     * @param {string} oldPath 原文件或文件夹的完整路径
     * @param {string} newPath 新的完整路径
     * @returns {Promise<boolean>} 重命名成功返回true，失败返回false
     * @throws {Error} 网络请求失败时抛出错误
     *
     * @example
     * const success = await BackendAPI.renameItem(
     *     '/home/user/project/old.txt',
     *     '/home/user/project/new.txt'
     * );
     * if (success) {
     *     console.log('重命名成功');
     * }
     */
    async renameItem(oldPath, newPath) {
        try {
            const response = await fetch('/api/rename', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({ oldPath, newPath })
            });
            const data = await response.json();
            return data.success || false;
        } catch (error) {
            console.error('重命名失败:', error);
            return false;
        }
    },

    /**
     * 获取文件类型信息
     *
     * 获取文件的详细信息，包括是否为二进制文件和MIME类型
     * 用于判断文件是否可以在编辑器中打开
     *
     * @async
     * @param {string} path 文件路径（绝对路径）
     * @returns {Promise<Object>} 文件信息对象，包含：
     *   - isBinary {boolean}: 是否为二进制文件
     *   - mimeType {string}: MIME类型字符串
     *   - size {number}: 文件大小（字节）
     * @throws {Error} 网络请求失败时抛出错误
     *
     * @example
     * const info = await BackendAPI.getFileInfo('/home/user/project/image.png');
     * if (info.isBinary) {
     *     console.log('这是二进制文件');
     * }
     */
    async getFileInfo(path) {
        try {
            const response = await fetch(`/api/file-info?path=${encodeURIComponent(path)}`);
            const data = await response.json();
            return data;
        } catch (error) {
            console.error('获取文件信息失败:', error);
            return { isBinary: false, mimeType: 'text/plain' };
        }
    },

/**
     * 保存所有打开的文件
     *
     * 该方法将指定的文件内容列表发送到后端进行保存。
     * 不再依赖contentCache，每次保存都接收完整的文件列表。
     *
     * @param {Array} files - 文件列表，每个元素是[路径, 内容]的数组
     * @returns {Promise<boolean>} 是否全部保存成功
     *
     * @example
     * const files = [['/path/to/file1.c', 'content1'], ['/path/to/file2.js', 'content2']];
     * if (await BackendAPI.saveAll(files)) {
     *     console.log('所有文件已保存');
     * }
     */
    async saveAll(files) {
        try {
            const response = await fetch('/api/save-all', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({
                    files: files
                })
            });
            const data = await response.json();
            return data.success || false;
        } catch (error) {
            console.error('保存所有文件失败:', error);
            return false;
        }
    },

    /**
     * 刷新文件树
     *
     * 重新加载当前目录的文件列表，用于同步外部对文件系统的修改
     *
     * @async
     * @returns {Promise<boolean>} 刷新成功返回true，失败返回false
     * @throws {Error} 网络请求失败时抛出错误
     *
     * @example
     * await BackendAPI.refresh();
     * console.log('文件树已刷新');
     */
    async refresh() {
        try {
            const response = await fetch('/api/refresh');
            const data = await response.json();
            return data.success || false;
        } catch (error) {
            console.error('刷新失败:', error);
            return false;
        }
    }
};

// ============================================================================
// DOM元素引用缓存
// ============================================================================

/**
 * DOM元素引用缓存对象
 *
 * 缓存所有需要频繁访问的DOM元素引用，避免重复的document.getElementById调用
 * 提高性能，代码更清晰
 *
 * 所有引用在initDOM()函数中初始化
 */
const DOM = {
    // 文件操作按钮
    btnOpenFolder: null,      // 打开文件夹按钮
    btnNewFolder: null,       // 新建文件夹按钮
    btnNewFile: null,         // 新建文件按钮
    btnSettings: null,        // 设置按钮

    // 路径显示区域
    currentPath: null,        // 当前路径显示文本元素
    btnBack: null,            // 返回上级目录按钮

    // 文件树区域
    fileTree: null,           // 文件树容器

    // 标签页区域
    tabsScroll: null,         // 标签页滚动容器
    tabsScrollbar: null,      // 标签页滚动条
    tabsScrollbarThumb: null, // 标签页滚动条滑块

    // 编辑器区域
    lineNumbers: null,        // 行号容器
    codeEditor: null,         // 代码编辑器容器
    verticalScrollbar: null,  // 垂直滚动条
    verticalScrollbarThumb: null, // 垂直滚动条滑块
    editorContainer: null,    // 编辑器主容器

    // 右键菜单
    contextMenu: null,        // 右键菜单容器
    contextRename: null,      // 重命名菜单项
    contextDelete: null,      // 删除菜单项

    // 新建对话框
    newDialog: null,          // 新建对话框容器
    newDialogTitle: null,     // 新建对话框标题
    newDialogInput: null,     // 新建对话框输入框
    newDialogConfirm: null,   // 新建对话框确认按钮
    newDialogCancel: null,    // 新建对话框取消按钮

    // 壁纸选择弹窗
    wallpaperDialog: null,    // 壁纸选择弹窗容器
    wallpaperGrid: null,      // 壁纸网格容器

    // 消息弹窗
    messageDialog: null,      // 消息弹窗容器
    messageDialogTitle: null, // 消息弹窗标题
    messageDialogMessage: null, // 消息弹窗消息内容
    messageDialogOk: null,    // 消息弹窗确定按钮
    messageDialogClose: null  // 消息弹窗关闭按钮
};

// ============================================================================
// 工具函数
// ============================================================================

/**
 * 获取文件图标
 *
 * 根据文件名和是否为目录返回对应的图标文件名
 * 使用预定义的图标映射表（AppState.iconMap）进行匹配
 *
 * @param {string} filename 文件名（包含扩展名）
 * @param {boolean} isDirectory 是否为目录
 * @returns {string} 图标文件名（相对于Icons目录）
 *
 * @example
 * const icon = getFileIcon('main.cpp', false);
 * console.log(icon); // 'C++-24.svg'
 */
function getFileIcon(filename, isDirectory) {
    if (isDirectory) {
        return 'Folder.png';
    }
    
    const ext = '.' + filename.split('.').pop().toLowerCase();
    return AppState.iconMap[ext] || 'Other-24.svg';
}

/**
 * 初始化DOM元素引用
 *
 * 在应用初始化时调用，获取所有需要的DOM元素引用并缓存到DOM对象中
 * 避免后续频繁调用document.getElementById，提高性能
 *
 * @function
 * @returns {void}
 *
 * @example
 * initDOM();
 * // 现在可以通过DOM.btnOpenFolder访问按钮元素
 */
function initDOM() {
    DOM.btnOpenFolder = document.getElementById('btn-open-folder');
    DOM.btnNewFolder = document.getElementById('btn-new-folder');
    DOM.btnNewFile = document.getElementById('btn-new-file');
    DOM.btnSettings = document.getElementById('btn-settings');
    DOM.currentPath = document.getElementById('current-path');
    DOM.btnBack = document.getElementById('btn-back');
    DOM.fileTree = document.getElementById('file-tree');
    DOM.tabsScroll = document.getElementById('tabs-scroll');
    DOM.tabsScrollbar = document.getElementById('tabs-scrollbar');
    DOM.tabsScrollbarThumb = document.getElementById('tabs-scrollbar-thumb');
    DOM.lineNumbers = document.getElementById('line-numbers');
    DOM.codeEditor = document.getElementById('code-editor');
    DOM.verticalScrollbar = document.getElementById('vertical-scrollbar');
    DOM.verticalScrollbarThumb = document.getElementById('vertical-scrollbar-thumb');
    DOM.contextMenu = document.getElementById('context-menu');
    DOM.contextRename = document.getElementById('context-rename');
    DOM.contextDelete = document.getElementById('context-delete');
    DOM.newDialog = document.getElementById('new-dialog');
    DOM.newDialogTitle = document.getElementById('new-dialog-title');
    DOM.newDialogInput = document.getElementById('new-dialog-input');
    DOM.newDialogConfirm = document.getElementById('new-dialog-confirm');
    DOM.newDialogCancel = document.getElementById('new-dialog-cancel');
    DOM.editorContainer = document.getElementById('editor-container');
    DOM.wallpaperDialog = document.getElementById('wallpaper-dialog');
    DOM.wallpaperGrid = document.getElementById('wallpaper-grid');
    DOM.messageDialog = document.getElementById('message-dialog');
    DOM.messageDialogTitle = document.getElementById('message-dialog-title');
    DOM.messageDialogMessage = document.getElementById('message-dialog-message');
    DOM.messageDialogOk = document.getElementById('message-dialog-ok');
    DOM.messageDialogClose = document.getElementById('message-dialog-close');
}

/**
 * 显示自定义消息弹窗
 *
 * 在屏幕中央显示一个模态消息弹窗，用于向用户显示提示、警告或错误信息
 * 弹窗具有淡入和滑入动画效果
 *
 * @param {string} message 要显示的消息内容
 * @param {string} [title='提示'] 弹窗标题，默认为"提示"
 * @returns {void}
 *
 * @example
 * showMessage('保存成功', '提示');
 * showMessage('文件不存在', '错误');
 */
function showMessage(message, title = '提示') {
    DOM.messageDialogTitle.textContent = title;
    DOM.messageDialogMessage.textContent = message;
    DOM.messageDialog.style.display = 'flex';
}

/**
 * 隐藏自定义消息弹窗
 *
 * 隐藏当前显示的消息弹窗
 * 通常在用户点击确定按钮或关闭按钮后调用
 *
 * @returns {void}
 *
 * @example
 * hideMessageDialog();
 */
function hideMessageDialog() {
    DOM.messageDialog.style.display = 'none';
}

/**
 * 渲染文件树
 *
 * 在文件树容器中渲染文件和文件夹列表
 * 使用DocumentFragment进行批量DOM操作，提高性能
 *
 * @param {Array<Object>} files 文件列表数组，每个元素包含：
 *   - name {string}: 文件或目录名称
 *   - path {string}: 完整路径
 *   - isDirectory {boolean}: 是否为目录
 * @returns {void}
 *
 * @example
 * const files = [
 *     { name: 'src', path: '/project/src', isDirectory: true },
 *     { name: 'main.cpp', path: '/project/main.cpp', isDirectory: false }
 * ];
 * renderFileTree(files);
 */
function renderFileTree(files) {
    console.log('开始渲染文件树，文件数量:', files.length);

    // 清空文件树容器
    DOM.fileTree.innerHTML = '';

    // 排序规则：目录在前，文件在后，同类型按名称排序（字母顺序）
    // 这样可以提高文件树的可读性，方便用户查找
    const sortedFiles = files.sort((a, b) => {
        if (a.isDirectory !== b.isDirectory) {
            return a.isDirectory ? -1 : 1;
        }
        return a.name.localeCompare(b.name);
    });

    // 使用DocumentFragment进行批量DOM操作
    // DocumentFragment是一个轻量级的DOM节点，可以作为临时容器
    // 一次性添加到实际DOM中，减少重绘和重排次数，提高性能
    const fragment = document.createDocumentFragment();

    // 遍历排序后的文件列表，为每个文件/目录创建DOM元素
    sortedFiles.forEach(file => {
        // 创建文件树项容器
        const item = document.createElement('div');
        // 设置CSS类名：基础类名 + 目录类名（如果是目录）
        item.className = 'file-tree-item' + (file.isDirectory ? ' directory' : '');
        // 将文件信息存储在dataset中，方便后续访问
        item.dataset.path = file.path;
        item.dataset.name = file.name;
        item.dataset.isDirectory = file.isDirectory;

        // 如果是目录，添加展开按钮
        // 用户点击展开按钮可以进入子目录
        if (file.isDirectory) {
            const expandBtn = document.createElement('button');
            expandBtn.className = 'file-tree-expand-btn';
            expandBtn.innerHTML = `<img src="../Icons/Spread-out.png" alt="展开">`;
            // 绑定点击事件：进入子目录
            // 使用stopPropagation防止事件冒泡，避免触发文件树项的点击事件
            expandBtn.onclick = (e) => {
                e.stopPropagation();
                enterDirectory(file.path, file.name);
            };
            item.appendChild(expandBtn);
        }

        // LICENSE文件特殊处理：添加许可证图标
        // 这是项目的许可证文件，用特殊图标标识
        if (!file.isDirectory && file.name === 'LICENSE') {
            const licenseBadge = document.createElement('img');
            licenseBadge.className = 'license-badge';
            licenseBadge.src = '../Icons/LICENSE-24.svg';
            licenseBadge.alt = 'LICENSE';
            item.appendChild(licenseBadge);
        }

        // 根据文件类型获取对应的图标
        const icon = document.createElement('img');
        icon.className = 'file-tree-icon';
        icon.src = `../Icons/${getFileIcon(file.name, file.isDirectory)}`;
        icon.alt = file.name;
        item.appendChild(icon);

        // 创建文件名显示元素
        const name = document.createElement('span');
        name.className = 'file-tree-name';
        name.textContent = file.name;
        item.appendChild(name);

        // 绑定点击事件：处理文件或目录的点击
        item.onclick = () => handleFileItemClick(file);

        // 绑定右键菜单事件：显示重命名和删除菜单
        item.oncontextmenu = (e) => {
            e.preventDefault(); // 阻止浏览器默认的右键菜单
            showContextMenu(e, file);
        };

        // 将当前文件树项添加到DocumentFragment
        fragment.appendChild(item);
    });

    // 一次性将所有文件树项添加到DOM中
    // 这样只触发一次重排，性能更好
    DOM.fileTree.appendChild(fragment);

    console.log('文件树渲染完成');
}

/**
 * 处理文件树项点击
 * @param {Object} file 文件信息
 */
async function handleFileItemClick(file) {
    // 如果是目录，不做操作（由展开按钮处理）
    if (file.isDirectory) {
        return;
    }
    
    // 检查文件扩展名，判断是否为媒体文件
    const ext = '.' + file.name.split('.').pop().toLowerCase();
    const mediaExtensions = ['.jpeg', '.jpg', '.png', '.svg', '.mp3', '.mp4', '.wav'];

    // 如果是媒体文件，允许打开
    if (mediaExtensions.includes(ext)) {
        openFileTab(file.path, file.name);
        return;
    }
    
    // 对于其他文件，检查是否为二进制文件
    const fileInfo = await BackendAPI.getFileInfo(file.path);
    if (fileInfo.isBinary) {
        showMessage('无法打开二进制文件', '错误');
        return;
    }
    
    // 打开文件标签页
    openFileTab(file.path, file.name);
}

/**
 * 打开文件标签页
 * @param {string} path 文件路径
 * @param {string} name 文件名
 */
async function openFileTab(path, name) {
    // 检查是否已存在该标签页
    const existingIndex = AppState.openTabs.findIndex(tab => tab.path === path);
    
    if (existingIndex !== -1) {
        // 已存在，激活该标签页
        activateTab(existingIndex);
        return;
    }
    
    // 获取文件内容
    let content = '';
    if (AppState.contentCache.has(path)) {
        content = AppState.contentCache.get(path);
    } else {
        content = await BackendAPI.readFile(path);
    }
    
    // 添加新标签页
    AppState.openTabs.push({ path, name, content });
    activateTab(AppState.openTabs.length - 1);
    
    // 渲染标签页
    renderTabs();
}

/**
 * 激活指定标签页
 * @param {number} index 标签页索引
 */
function activateTab(index) {
    if (index < 0 || index >= AppState.openTabs.length) {
        return;
    }
    
    AppState.activeTab = index;
    const tab = AppState.openTabs[index];
    
    // 调试：打印切换前的内容
    console.log('=== 切换到标签页 ===');
    console.log('标签页:', tab.name);
    console.log('内容长度:', tab.content.length);
    console.log('内容预览:', tab.content.substring(0, 200));
    console.log('换行符数量:', (tab.content.match(/\n/g) || []).length);
    
    // 渲染标签页高亮
    document.querySelectorAll('.tab-item').forEach((item, i) => {
        item.classList.toggle('active', i === index);
    });
    
    // 检查标签页类型并显示相应内容
    if (tab.isAbout) {
        showAboutPage();
    } else if (tab.isSettings) {
        showSettingsContent();
    } else {
        // 检查文件类型
        const ext = '.' + tab.name.split('.').pop().toLowerCase();
        const mediaExtensions = ['.jpeg', '.jpg', '.png', '.svg', '.mp3', '.mp4', '.wav'];

        if (mediaExtensions.includes(ext)) {
            showMediaContent(tab.path, ext);
        } else {
            showCodeContent(tab.content);
        }
    }
}

/**
 * 语法高亮（类似Visual Studio Code）
 * @param {string} code 代码内容
 * @returns {string} 高亮后的HTML
 */
function syntaxHighlight(code) {
    // 检查缓存
    const cacheKey = code.substring(0, 1000); // 使用前1000个字符作为缓存键
    if (AppState.syntaxHighlightCache.has(cacheKey)) {
        return AppState.syntaxHighlightCache.get(cacheKey);
    }

    // 转义HTML特殊字符
    let html = code
        .replace(/&/g, '&amp;')
        .replace(/</g, '&lt;')
        .replace(/>/g, '&gt;');

    // 先匹配字符串（单引号和双引号），避免在字符串内高亮其他内容
    html = html.replace(/("(?:[^"\\]|\\.)*"|'(?:[^'\\]|\\.)*'|`(?:[^`\\]|\\.)*`)/g, (match) => {
        return `<span class="syntax-string">${match}</span>`;
    });

    // 匹配单行注释（但要避免匹配URL中的//）
    html = html.replace(/\/\/.*$/gm, (match) => {
        return `<span class="syntax-comment">${match}</span>`;
    });

    // 匹配多行注释
    html = html.replace(/\/\*[\s\S]*?\*\//g, (match) => {
        return `<span class="syntax-comment">${match}</span>`;
    });

    // 匹配预处理指令（#include, #define等）
    html = html.replace(/^#\s*(include|define|ifdef|ifndef|endif|pragma|undef)/gm, (match) => {
        return `<span class="syntax-preprocessor">${match}</span>`;
    });

    // 匹配数字
    html = html.replace(/\b\d+(\.\d+)?([eE][+-]?\d+)?\b/g, (match) => {
        return `<span class="syntax-number">${match}</span>`;
    });

    // 匹配关键字（C/C++/JavaScript/TypeScript等）
    const keywords = [
        'int', 'float', 'double', 'char', 'void', 'return', 'if', 'else', 'for', 'while',
        'do', 'switch', 'case', 'break', 'continue', 'struct', 'class', 'enum', 'union',
        'typedef', 'const', 'static', 'extern', 'volatile', 'register', 'auto', 'signed',
        'unsigned', 'long', 'short', 'sizeof', 'bool', 'true', 'false', 'null', 'undefined',
        'let', 'const', 'var', 'function', 'async', 'await', 'try', 'catch', 'throw', 'new',
        'this', 'super', 'extends', 'import', 'export', 'default', 'from', 'interface',
        'type', 'public', 'private', 'protected', 'readonly', 'abstract', 'get', 'set',
        'goto', 'sizeof', 'typeof', 'instanceof', 'in', 'of'
    ];

    const keywordPattern = new RegExp(`\\b(${keywords.join('|')})\\b`, 'g');
    html = html.replace(keywordPattern, (match) => {
        return `<span class="syntax-keyword">${match}</span>`;
    });

    // 匹配函数定义和调用（但不要匹配关键字）
    html = html.replace(/\b([a-zA-Z_]\w*)\s*\(/g, (match, funcName) => {
        // 检查是否是关键字
        if (keywords.includes(funcName)) {
            return match;
        }
        return `<span class="syntax-function">${funcName}</span>(`;
    });

    // 匹配宏定义
    html = html.replace(/\b[A-Z_][A-Z0-9_]*\b/g, (match) => {
        // 检查是否是预处理指令
        if (match.startsWith('#')) {
            return match;
        }
        return `<span class="syntax-macro">${match}</span>`;
    });

    // 缓存结果（限制缓存大小）
    if (AppState.syntaxHighlightCache.size > 100) {
        // 如果缓存超过100个，清除最早的缓存
        const firstKey = AppState.syntaxHighlightCache.keys().next().value;
        AppState.syntaxHighlightCache.delete(firstKey);
    }
    AppState.syntaxHighlightCache.set(cacheKey, html);

    return html;
}

/**
 * 显示代码内容
 * @param {string} content 代码内容
 */
function showCodeContent(content) {
    DOM.editorContainer.classList.remove('media-mode');
    DOM.lineNumbers.style.opacity = '0.5';

    // 设置代码内容为可编辑
    DOM.codeEditor.contentEditable = 'true';

    // 清空编辑器
    DOM.codeEditor.innerHTML = '';

    if (content) {
        // 调试：打印显示前的内容
        console.log('=== showCodeContent ===');
        console.log('内容长度:', content.length);
        console.log('换行符数量:', (content.match(/\n/g) || []).length);
        console.log('内容预览:', content.substring(0, 300));

        // 按行分割内容，为每一行创建一个 <div> 元素
        // 这样可以确保空行也被正确保存
        // 注意：<div> 是块级元素，自动换行，不需要额外的 <br>
        const lines = content.split('\n');

        console.log('分割后的行数:', lines.length);
        console.log('前5行:', lines.slice(0, 5).map(l => JSON.stringify(l)));

        lines.forEach((line, index) => {
            const lineDiv = document.createElement('div');
            // 设置行内容，即使为空也保留这个 div（代表空行）
            // 使用 innerHTML 可以避免 contenteditable 的自动规范化
            lineDiv.innerHTML = '';
            lineDiv.appendChild(document.createTextNode(line));

            DOM.codeEditor.appendChild(lineDiv);
        });

        console.log('创建的div数量:', DOM.codeEditor.childElementCount);
    }

    // 更新行号
    updateLineNumbers(DOM.codeEditor.textContent);

    // 重置滚动条
    DOM.codeEditor.scrollTop = 0;
}

/**
 * 显示媒体内容
 * @param {string} path 文件路径
 * @param {string} ext 文件扩展名
 */
function showMediaContent(path, ext) {
    DOM.editorContainer.classList.add('media-mode');
    DOM.lineNumbers.style.opacity = '0';
    
    let mediaHtml = '';
    
    if (ext === '.jpeg' || ext === '.jpg' || ext === '.png' || ext === '.svg') {
        // 图片 - 使用后端API获取图片数据
        const imageUrl = `/api/read-binary-file?path=${encodeURIComponent(path)}`;
        mediaHtml = `<div class="media-container"><img src="${imageUrl}" class="media-image" alt="${path}" style="max-width: 100%; max-height: 100%; object-fit: contain;"></div>`;
    } else if (ext === '.mp4') {
        // 视频
        const videoUrl = `/api/read-binary-file?path=${encodeURIComponent(path)}`;
        mediaHtml = `
            <div class="media-video">
                <video controls autoplay style="max-width: 100%; max-height: 100%; border-radius: 8px;">
                    <source src="${videoUrl}" type="video/mp4">
                    您的浏览器不支持视频播放
                </video>
            </div>`;
    } else if (ext === '.mp3' || ext === '.wav') {
        // 音频
        const audioUrl = `/api/read-binary-file?path=${encodeURIComponent(path)}`;
        mediaHtml = `
            <div class="media-container">
                <div class="audio-player">
                    <audio controls autoplay>
                        <source src="${audioUrl}" type="audio/mpeg">
                        您的浏览器不支持音频播放
                    </audio>
                </div>
            </div>`;
    }
    
    DOM.codeEditor.innerHTML = mediaHtml;
    DOM.lineNumbers.innerHTML = '';
}

/**
 * 播放视频
 * @param {string} path 视频路径
 */
window.playVideo = function(path) {
    const videoUrl = `/api/read-binary-file?path=${encodeURIComponent(path)}`;
    const videoHtml = `
        <div class="media-video">
            <video controls autoplay style="max-width: 100%; max-height: 100%; border-radius: 8px;">
                <source src="${videoUrl}" type="video/mp4">
                您的浏览器不支持视频播放
            </video>
        </div>`;
    DOM.codeEditor.innerHTML = videoHtml;
};

/**
 * 播放音频
 * @param {string} path 音频路径
 */
window.playAudio = function(path) {
    const audioUrl = `/api/read-binary-file?path=${encodeURIComponent(path)}`;
    const audioHtml = `
        <div class="media-container">
            <div class="audio-player">
                <audio controls autoplay style="width: 80%; margin-top: 20px;">
                    <source src="${audioUrl}" type="audio/mpeg">
                    您的浏览器不支持音频播放
                </audio>
            </div>
        </div>`;
    DOM.codeEditor.innerHTML = audioHtml;
};

/**
 * updateLineNumbers - 更新行号
 *
 * 计算编辑器中的行数并更新行号显示。
 * 通过分析codeEditor的直接子节点来统计行数，不递归。
 *
 * @content: 代码内容（参数保留但不使用，直接分析DOM结构）
 *
 * 注意: 由于showCodeContent为每行创建一个div元素，
 *       我们只需统计codeEditor的直接子div/p/br元素数量。
 */
function updateLineNumbers(content)
{
	let lines = 0;
	const childNodes = DOM.codeEditor.childNodes;

	/*
	 * 遍历codeEditor的直接子节点
	 * 统计表示行的元素数量
	 */
	for (let i = 0; i < childNodes.length; i++) {
		const node = childNodes[i];

		if (node.nodeType === Node.ELEMENT_NODE) {
			const tagName = node.tagName?.toLowerCase();

			/*
			 * div或p元素各占一行（即使为空）
			 */
			if (tagName === 'div' || tagName === 'p') {
				lines++;
			}
			/*
			 * br元素表示换行（占一行）
			 */
			else if (tagName === 'br') {
				lines++;
			}
		} else if (node.nodeType === Node.TEXT_NODE) {
			/*
			 * 文本节点：计算其中的换行符数量
			 * 注意：由于我们使用div包裹每行，通常不会有直接的文本节点
			 * 但为了兼容性，这里保留文本节点的处理
			 */
			const textContent = node.textContent || '';
			if (textContent.length > 0) {
				/*
				 * 如果文本节点包含换行符，计算换行符数量
				 * 否则算作一行
				 */
				const newLines = (textContent.match(/\n/g) || []).length;
				lines += newLines + 1;
			}
		}
	}

	/*
	 * 如果没有找到任何行，至少显示1行
	 */
	if (lines === 0)
		lines = 1;

	/*
	 * 生成行号HTML
	 */
	let lineNumbersHtml = '';
	for (let i = 1; i <= lines; i++) {
		lineNumbersHtml += `<div class="line-number">${i}</div>`;
	}

	DOM.lineNumbers.innerHTML = lineNumbersHtml;
}

/**
 * 渲染标签页
 */
function renderTabs() {
    DOM.tabsScroll.innerHTML = '';

    // 使用 DocumentFragment 减少 DOM 操作
    const fragment = document.createDocumentFragment();

    AppState.openTabs.forEach((tab, index) => {
        const tabItem = document.createElement('div');
        tabItem.className = 'tab-item' + (index === AppState.activeTab ? ' active' : '');
        tabItem.dataset.index = index;

        // 图标
        const icon = document.createElement('img');
        icon.className = 'tab-icon';
        icon.src = `../Icons/${getFileIcon(tab.name, false)}`;
        tabItem.appendChild(icon);

        // 名称
        const name = document.createElement('span');
        name.className = 'tab-name';
        name.textContent = tab.name;
        tabItem.appendChild(name);

        // 关闭按钮
        const closeBtn = document.createElement('button');
        closeBtn.className = 'tab-close';
        closeBtn.innerHTML = `<img src="../Icons/Close.png" alt="关闭">`;
        closeBtn.onclick = (e) => {
            e.stopPropagation();
            closeTab(index);
        };
        tabItem.appendChild(closeBtn);

        // 点击事件
        tabItem.onclick = () => activateTab(index);

        fragment.appendChild(tabItem);
    });

    // 一次性添加到 DOM
    DOM.tabsScroll.appendChild(fragment);

    // 更新标签页滚动条
    updateTabsScrollbar();
}

/**
 * 关闭标签页
 * @param {number} index 标签页索引
 */
function closeTab(index) {
    if (index < 0 || index >= AppState.openTabs.length) {
        return;
    }
    
    const tab = AppState.openTabs[index];
    
    // 如果有未保存的内容，保存到缓存
    // 使用 getEditorContent() 正确获取带格式的编辑器内容
    if (index === AppState.activeTab) {
        const currentContent = getEditorContent();
        if (currentContent !== tab.content) {
            tab.content = currentContent;
            AppState.contentCache.set(tab.path, currentContent);
        }
    }
    
    // 移除标签页
    AppState.openTabs.splice(index, 1);
    
    // 调整激活标签页
    if (AppState.activeTab >= AppState.openTabs.length) {
        AppState.activeTab = AppState.openTabs.length - 1;
    }
    
    // 重新渲染
    renderTabs();
    
    // 如果还有标签页，激活当前标签页
    if (AppState.activeTab >= 0) {
        activateTab(AppState.activeTab);
    } else {
        // 没有标签页，清空编辑器
        DOM.codeEditor.textContent = '';
        DOM.lineNumbers.innerHTML = '';
    }
}

/**
 * 切换到左边的标签页
 */
function switchTabLeft() {
    if (AppState.openTabs.length <= 1) {
        return;
    }
    
    // 如果当前不是第一个标签页，切换到左边
    if (AppState.activeTab > 0) {
        activateTab(AppState.activeTab - 1);
    }
}

/**
 * 切换到右边的标签页
 */
function switchTabRight() {
    if (AppState.openTabs.length <= 1) {
        return;
    }
    
    // 如果当前不是最后一个标签页，切换到右边
    if (AppState.activeTab < AppState.openTabs.length - 1) {
        activateTab(AppState.activeTab + 1);
    }
}

/**
 * 删除当前标签页
 */
function deleteCurrentTab() {
    if (AppState.openTabs.length === 0) {
        return;
    }
    
    // 获取当前标签页索引
    const currentIndex = AppState.activeTab;
    
    // 关闭当前标签页
    closeTab(currentIndex);
}

/**
 * 进入子目录
 * @param {string} path 目录路径
 * @param {string} name 目录名称
 */
async function enterDirectory(path, name) {
    // 保存当前路径到历史
    if (AppState.currentPath) {
        AppState.pathHistory.push(AppState.currentPath);
    }
    
    AppState.currentPath = path;
    
    // 更新路径显示
    const pathParts = path.split('/');
    const relativePath = pathParts.slice(pathParts.indexOf(AppState.currentDirectory.split('/').pop())).join('/');
    DOM.currentPath.textContent = relativePath || name;
    
    // 启用返回按钮
    DOM.btnBack.disabled = false;
    
    // 加载目录内容
    await loadDirectoryContents(path);
}

/**
 * 返回上级目录
 */
async function goBack() {
    if (AppState.pathHistory.length === 0) {
        return;
    }
    
    const previousPath = AppState.pathHistory.pop();
    AppState.currentPath = previousPath;
    
    // 更新路径显示
    const pathParts = previousPath.split('/');
    const relativePath = pathParts.slice(pathParts.indexOf(AppState.currentDirectory.split('/').pop())).join('/');
    DOM.currentPath.textContent = relativePath;
    
    // 如果回到根目录，禁用返回按钮
    if (AppState.pathHistory.length === 0) {
        DOM.btnBack.disabled = true;
    }
    
    // 加载目录内容
    await loadDirectoryContents(previousPath);
}

/**
 * 加载目录内容
 * @param {string} path 目录路径
 */
async function loadDirectoryContents(path) {
    // 防止重复加载
    if (AppState.isLoading) {
        console.log('正在加载中，跳过重复请求');
        return;
    }

    AppState.isLoading = true;
    console.log('正在加载目录内容:', path);

    try {
        const files = await BackendAPI.getDirectoryContents(path);
        console.log('获取到的文件列表:', files);
        renderFileTree(files);
    } catch (error) {
        console.error('加载目录内容失败:', error);
    } finally {
        AppState.isLoading = false;
    }
}

/**
 * 显示右键菜单
 * @param {Event} event 事件对象
 * @param {Object} file 文件信息
 */
function showContextMenu(event, file) {
    AppState.contextSelected = file;
    
    DOM.contextMenu.style.display = 'block';
    DOM.contextMenu.style.left = event.pageX + 'px';
    DOM.contextMenu.style.top = event.pageY + 'px';
}

/**
 * 隐藏右键菜单
 */
function hideContextMenu() {
    DOM.contextMenu.style.display = 'none';
    AppState.contextSelected = null;
}

/**
 * 显示新建对话框
 * @param {string} type 类型 'folder' 或 'file'
 */
function showNewDialog(type) {
    AppState.newDialogType = type;
    DOM.newDialogTitle.textContent = type === 'folder' ? '新建文件夹' : '新建文件';
    DOM.newDialogInput.value = '';
    DOM.newDialog.style.display = 'flex';
    // 使用 setTimeout 确保对话框完全渲染后再设置焦点
    setTimeout(() => {
        DOM.newDialogInput.focus();
        DOM.newDialogInput.select();
    }, 10);
}

/**
 * 隐藏新建对话框
 */
function hideNewDialog() {
    DOM.newDialog.style.display = 'none';
    AppState.newDialogType = null;
}

/**
 * 处理新建确认
 */
async function handleNewConfirm() {
    const name = DOM.newDialogInput.value.trim();
    if (!name) {
        return;
    }
    
    const parentPath = AppState.currentPath || AppState.currentDirectory;
    if (!parentPath) {
        alert('请先打开一个文件夹');
        return;
    }
    
    let success = false;
    if (AppState.newDialogType === 'folder') {
        success = await BackendAPI.createFolder(parentPath, name);
    } else {
        success = await BackendAPI.createFile(parentPath, name);
    }
    
    if (success) {
        hideNewDialog();
        await loadDirectoryContents(parentPath);
    } else {
        showMessage('创建失败', '错误');
    }
}

/**
 * 显示了解页面
 */
function showAboutPage() {
    const aboutHtml = `
        <div class="about-content">
            <div class="about-logo">
                <img src="../Mikufy.png" alt="Mikufy Logo">
            </div>
            <div class="about-info">
                <div class="about-shortcuts">
                    <h2>快捷键</h2>
                    <ul>
                        <li><kbd>Ctrl+S</kbd> - 保存所有文件</li>
                        <li><kbd>F5</kbd> - 刷新</li>
                        <li><kbd>F11</kbd> - 全屏切换</li>
                        <li><kbd>Ctrl+O</kbd> - 打开文件夹</li>
                        <li><kbd>Ctrl+F</kbd> - 新建文件夹</li>
                        <li><kbd>Ctrl+N</kbd> - 新建文件</li>
                        <li><kbd>Ctrl+←</kbd> - 切换到左边的标签页</li>
                        <li><kbd>Ctrl+→</kbd> - 切换到右边的标签页</li>
                        <li><kbd>Ctrl+M</kbd> - 删除当前标签页</li>
                        <li><kbd>Tab</kbd> - 自动缩进（4格）</li>
                    </ul>
                </div>
                <div class="about-features">
                    <h2>功能</h2>
                    <ul>
                        <li>文件树导航</li>
                        <li>多标签页编辑</li>
                        <li>图片、视频、音频预览</li>
                        <li>右键菜单（重命名、删除）</li>
                        <li>更多等你来提供</li>
                    </ul>
                </div>
            </div>
        </div>`;

    DOM.codeEditor.innerHTML = aboutHtml;
    DOM.lineNumbers.innerHTML = '';
    DOM.lineNumbers.style.opacity = '0';

    // 了解页面不可编辑
    DOM.codeEditor.contentEditable = 'false';
}

/**
 * 显示设置页面
 */
function showSettingsPage() {
    // 检查是否已存在设置标签页
    const existingIndex = AppState.openTabs.findIndex(tab => tab.isSettings);
    
    if (existingIndex !== -1) {
        // 已存在，激活该标签页
        activateTab(existingIndex);
        return;
    }
    
    // 添加设置标签页
    AppState.openTabs.push({
        path: 'settings',
        name: '设置',
        content: '',
        isSettings: true
    });
    
    activateTab(AppState.openTabs.length - 1);
    
    // 渲染标签页
    renderTabs();
}

/**
 * 显示设置页面内容
 */
function showSettingsContent() {
    const settingsHtml = `<div class="settings-line">    <button id="btn-change-wallpaper" class="settings-button">更换编辑器壁纸</button></div>`;

    DOM.codeEditor.innerHTML = settingsHtml;
    DOM.lineNumbers.innerHTML = '';
    DOM.lineNumbers.style.opacity = '0';

    // 设置页面不可编辑
    DOM.codeEditor.contentEditable = 'false';

    // 绑定更换壁纸按钮事件
    const changeWallpaperBtn = document.getElementById('btn-change-wallpaper');
    if (changeWallpaperBtn) {
        changeWallpaperBtn.onclick = showWallpaperDialog;
        // 阻止按钮获取焦点，避免光标问题
        changeWallpaperBtn.onfocus = function(e) {
            e.preventDefault();
            this.blur();
        };
    }
}

/**
 * 显示壁纸选择弹窗
 */
function showWallpaperDialog() {
    DOM.wallpaperDialog.style.display = 'flex';
    loadWallpapers();
}

/**
 * 隐藏壁纸选择弹窗
 */
function hideWallpaperDialog() {
    DOM.wallpaperDialog.style.display = 'none';
    DOM.wallpaperGrid.innerHTML = '';
}

/**
 * 加载壁纸列表
 */
function loadWallpapers() {
    DOM.wallpaperGrid.innerHTML = '';

    // 壁纸文件列表（从web/Background目录）
    const wallpaperFiles = [
        'index-1.png',
        'index-2.jpg',
        'index-3.png',
        'index-4.png',
        'index-5.png',
        'index-6.png'
    ];

    wallpaperFiles.forEach(filename => {
        const wallpaperItem = document.createElement('div');
        wallpaperItem.className = 'wallpaper-item';
        wallpaperItem.dataset.filename = filename;

        const img = document.createElement('img');
        img.className = 'wallpaper-preview';
        img.src = `Background/${filename}`;
        img.alt = filename;

        wallpaperItem.appendChild(img);
        DOM.wallpaperGrid.appendChild(wallpaperItem);

        // 点击事件
        wallpaperItem.onclick = () => {
            // 移除所有壁纸项的选中状态
            document.querySelectorAll('.wallpaper-item').forEach(item => {
                item.classList.remove('selected');
            });
            // 选中当前壁纸项
            wallpaperItem.classList.add('selected');
        };

        // 双击事件
        wallpaperItem.ondblclick = () => {
            selectWallpaper(filename);
        };
    });

    // 绑定ESC键退出
    document.addEventListener('keydown', handleWallpaperDialogKeydown);
}

/**
 * 处理壁纸弹窗的键盘事件
 */
function handleWallpaperDialogKeydown(e) {
    if (e.key === 'Escape' && DOM.wallpaperDialog.style.display === 'flex') {
        hideWallpaperDialog();
        document.removeEventListener('keydown', handleWallpaperDialogKeydown);
    } else if (e.key === 'Enter' && DOM.wallpaperDialog.style.display === 'flex') {
        const selectedItem = document.querySelector('.wallpaper-item.selected');
        if (selectedItem) {
            const filename = selectedItem.dataset.filename;
            selectWallpaper(filename);
        }
    }
}

/**
 * 选择壁纸
 */
async function selectWallpaper(filename) {
    try {
        const response = await fetch('/api/change-wallpaper', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({ filename })
        });
        const data = await response.json();
        
        if (data.success) {
            hideWallpaperDialog();
            document.removeEventListener('keydown', handleWallpaperDialogKeydown);
            // 立即应用新壁纸
            document.body.style.backgroundImage = `url('Background/${filename}')`;
        } else {
            showMessage('更换壁纸失败', '错误');
        }
    } catch (error) {
        console.error('更换壁纸失败:', error);
        showMessage('更换壁纸失败', '错误');
    }
}

/**
 * 更新标签页滚动条
 */
function updateTabsScrollbar() {
    const scrollWidth = DOM.tabsScroll.scrollWidth;
    const clientWidth = DOM.tabsScroll.clientWidth;
    
    if (scrollWidth > clientWidth) {
        const scrollRatio = clientWidth / scrollWidth;
        DOM.tabsScrollbarThumb.style.width = (clientWidth * scrollRatio) + 'px';
        DOM.tabsScrollbar.style.display = 'block';
    } else {
        DOM.tabsScrollbar.style.display = 'none';
    }
}

/**
 * 同步代码编辑器和行号滚动
 */
function syncScroll() {
    // 使用 requestAnimationFrame 进行性能优化
    requestAnimationFrame(() => {
        const scrollTop = DOM.codeEditor.scrollTop;
        const scrollHeight = DOM.codeEditor.scrollHeight - DOM.codeEditor.clientHeight;

        // 同步行号滚动
        DOM.lineNumbers.scrollTop = scrollTop;

        // 同步自定义滚动条位置
        if (scrollHeight > 0) {
            const thumbHeight = DOM.verticalScrollbarThumb.clientHeight;
            const trackHeight = DOM.verticalScrollbar.clientHeight;
            const thumbTop = (scrollTop / scrollHeight) * (trackHeight - thumbHeight);
            DOM.verticalScrollbarThumb.style.top = Math.max(0, Math.min(thumbTop, trackHeight - thumbHeight)) + 'px';
        }
    });
}

/**
 * 同步标签页滚动
 */
function syncTabsScroll() {
    // 标签页滚动条已隐藏，此函数不再需要更新滚动条位置
}

/**
 * 初始化事件监听器
 */
function initEventListeners() {
    // 消息弹窗事件
    DOM.messageDialogOk.onclick = hideMessageDialog;
    DOM.messageDialogClose.onclick = hideMessageDialog;

    // 打开文件夹按钮
    DOM.btnOpenFolder.onclick = async () => {
        const folderPath = await BackendAPI.openFolderDialog();
        console.log('选中的文件夹路径:', folderPath);
        if (folderPath) {
            AppState.currentDirectory = folderPath;
            AppState.currentPath = folderPath;
            AppState.pathHistory = [];

            // 更新路径显示
            const folderName = folderPath.split('/').pop();
            DOM.currentPath.textContent = folderName;
            DOM.btnBack.disabled = true;

            // 加载目录内容
            await loadDirectoryContents(folderPath);
        } else {
            console.log('未选择文件夹或选择取消');
        }
    };
    
    // 新建文件夹按钮
    DOM.btnNewFolder.onclick = () => {
        if (!AppState.currentDirectory) {
            showMessage('请先打开一个文件夹', '提示');
            return;
        }
        showNewDialog('folder');
    };

    // 新建文件按钮
    DOM.btnNewFile.onclick = () => {
        if (!AppState.currentDirectory) {
            showMessage('请先打开一个文件夹', '提示');
            return;
        }
        showNewDialog('file');
    };
    
    // 设置按钮
    DOM.btnSettings.onclick = () => {
        showSettingsPage();
    };
    
    // 返回按钮
    DOM.btnBack.onclick = goBack;
    
    // 右键菜单事件
    DOM.contextRename.onclick = async () => {
        if (!AppState.contextSelected) {
            return;
        }
        
        const newName = prompt('请输入新名称:', AppState.contextSelected.name);
        if (newName && newName !== AppState.contextSelected.name) {
            const oldPath = AppState.contextSelected.path;
            const pathParts = oldPath.split('/');
            pathParts[pathParts.length - 1] = newName;
            const newPath = pathParts.join('/');
            
            const success = await BackendAPI.renameItem(oldPath, newPath);
            if (success) {
                hideContextMenu();
                await loadDirectoryContents(AppState.currentPath);
            } else {
                showMessage('重命名失败', '错误');
            }
        }
    };
    
    DOM.contextDelete.onclick = async () => {
        if (!AppState.contextSelected) {
            return;
        }
        
        const success = await BackendAPI.deleteItem(AppState.contextSelected.path);
        if (success) {
            hideContextMenu();
            await loadDirectoryContents(AppState.currentPath);
        } else {
            showMessage('删除失败', '错误');
        }
    };
    
    // 新建对话框事件
    DOM.newDialogConfirm.onclick = handleNewConfirm;
    DOM.newDialogCancel.onclick = hideNewDialog;
    
    DOM.newDialogInput.onkeydown = (e) => {
        if (e.key === 'Enter') {
            e.preventDefault();
            handleNewConfirm();
        } else if (e.key === 'Escape') {
            e.preventDefault();
            hideNewDialog();
        }
    };
    
    // 代码编辑器滚动事件
    DOM.codeEditor.onscroll = syncScroll;
    DOM.tabsScroll.onscroll = syncTabsScroll;
    
    // 垂直滚动条拖动
    let isVerticalDragging = false;
    DOM.verticalScrollbarThumb.onmousedown = (e) => {
        isVerticalDragging = true;
        e.preventDefault();
    };
    
    // 标签页滚动条拖动
    let isTabsDragging = false;
    DOM.tabsScrollbarThumb.onmousedown = (e) => {
        isTabsDragging = true;
        e.preventDefault();
    };
    
    // 合并的鼠标移动事件处理
    document.onmousemove = (e) => {
        // 处理垂直滚动条拖动
        if (isVerticalDragging) {
            const rect = DOM.verticalScrollbar.getBoundingClientRect();
            const thumbHeight = DOM.verticalScrollbarThumb.clientHeight;
            const maxTop = DOM.verticalScrollbar.clientHeight - thumbHeight;
            let newTop = e.clientY - rect.top - thumbHeight / 2;
            
            newTop = Math.max(0, Math.min(newTop, maxTop));
            DOM.verticalScrollbarThumb.style.top = newTop + 'px';
            
            // 同步代码编辑器滚动
            const scrollRatio = newTop / maxTop;
            const maxScroll = DOM.codeEditor.scrollHeight - DOM.codeEditor.clientHeight;
            DOM.codeEditor.scrollTop = scrollRatio * maxScroll;
        }
        
        // 处理标签页滚动条拖动
        if (isTabsDragging) {
            const rect = DOM.tabsScrollbar.getBoundingClientRect();
            const thumbWidth = DOM.tabsScrollbarThumb.clientWidth;
            const maxLeft = DOM.tabsScrollbar.clientWidth - thumbWidth;
            let newLeft = e.clientX - rect.left - thumbWidth / 2;
            
            newLeft = Math.max(0, Math.min(newLeft, maxLeft));
            DOM.tabsScrollbarThumb.style.left = newLeft + 'px';
            
            // 同步标签页滚动
            const scrollRatio = newLeft / maxLeft;
            const maxScroll = DOM.tabsScroll.scrollWidth - DOM.tabsScroll.clientWidth;
            DOM.tabsScroll.scrollLeft = scrollRatio * maxScroll;
        }
    };
    
    // 合并的鼠标释放事件处理
    document.onmouseup = () => {
        isVerticalDragging = false;
        isTabsDragging = false;
    };
    
    // 垂直滚动条点击跳转
    DOM.verticalScrollbar.onclick = (e) => {
        if (e.target === DOM.verticalScrollbarThumb) return;
        
        const rect = DOM.verticalScrollbar.getBoundingClientRect();
        const thumbHeight = DOM.verticalScrollbarThumb.clientHeight;
        const clickY = e.clientY - rect.top;
        
        const maxTop = DOM.verticalScrollbar.clientHeight - thumbHeight;
        let newTop = clickY - thumbHeight / 2;
        newTop = Math.max(0, Math.min(newTop, maxTop));
        
        const scrollRatio = newTop / maxTop;
        const maxScroll = DOM.codeEditor.scrollHeight - DOM.codeEditor.clientHeight;
        DOM.codeEditor.scrollTop = scrollRatio * maxScroll;
    };
    
    // 鼠标滚轮支持标签页滚动
    DOM.tabsContainer = document.getElementById('tabs-container');
    DOM.tabsContainer.addEventListener('wheel', (e) => {
        // 支持横向和纵向滚轮都可以横向滚动标签页
        if (e.deltaX !== 0) {
            e.preventDefault();
            DOM.tabsScroll.scrollLeft += e.deltaX;
        } else if (e.deltaY !== 0) {
            e.preventDefault();
            DOM.tabsScroll.scrollLeft += e.deltaY;
        }
    });
    
    // 键盘快捷键
    document.onkeydown = (e) => {
        // 如果对话框可见，不处理快捷键（除了 Escape）
        if (DOM.newDialog.style.display === 'flex') {
            // 只处理 Escape 键关闭对话框
            if (e.key === 'Escape') {
                hideNewDialog();
                e.preventDefault();
            }
            return;
        }

        // 检查是否在设置页面或了解页面中
        const currentTab = AppState.openTabs[AppState.activeTab];
        const isInNonEditableTab = currentTab && (currentTab.isAbout || currentTab.isSettings);

        // 如果在设置页面或了解页面中，阻止所有输入（除了功能键）
        if (isInNonEditableTab) {
            // 允许的功能键
            const allowedKeys = ['F5', 'F11', 'Escape', 'Tab', 'ArrowLeft', 'ArrowRight', 'ArrowUp', 'ArrowDown'];
            // 允许的组合键
            const isAllowedShortcut = e.ctrlKey || e.altKey || e.metaKey;
            const isAllowedKey = allowedKeys.includes(e.key);

            if (!isAllowedShortcut && !isAllowedKey && e.key.length === 1) {
                // 普通字符输入，阻止
                e.preventDefault();
                return false;
            }
        }

        // Ctrl+S - 保存
        if (e.ctrlKey && e.key === 's') {
            e.preventDefault();
            handleSaveAll();
        }
        
        // F5 - 刷新
        if (e.key === 'F5') {
            e.preventDefault();
            handleRefresh();
        }
        
        // F11 - 全屏切换
        if (e.key === 'F11') {
            e.preventDefault();
            handleToggleFullscreen();
        }

        // ESC - 在全屏状态下阻止退出全屏
        if (e.key === 'Escape' && document.fullscreenElement) {
            e.preventDefault();
        }

        // Ctrl+O - 打开文件夹
        if (e.ctrlKey && e.key === 'o') {
            e.preventDefault();
            DOM.btnOpenFolder.click();
        }
        
        // Ctrl+F - 新建文件夹
        if (e.ctrlKey && e.key === 'f') {
            e.preventDefault();
            DOM.btnNewFolder.click();
        }
        
        // Ctrl+N - 新建文件
        if (e.ctrlKey && e.key === 'n') {
            e.preventDefault();
            DOM.btnNewFile.click();
        }
        
        // Ctrl+ArrowLeft - 切换到左边的标签页
        if (e.ctrlKey && e.key === 'ArrowLeft') {
            e.preventDefault();
            switchTabLeft();
        }
        
        // Ctrl+ArrowRight - 切换到右边的标签页
        if (e.ctrlKey && e.key === 'ArrowRight') {
            e.preventDefault();
            switchTabRight();
        }
        
        // Ctrl+M - 删除当前标签页
        if (e.ctrlKey && e.key === 'm') {
            e.preventDefault();
            deleteCurrentTab();
        }
        
        // Tab - 自动缩进（只在代码编辑器中）
        if (e.key === 'Tab' && document.activeElement === DOM.codeEditor && !isInNonEditableTab) {
            e.preventDefault();
            handleTabIndent();
        }
    };
    
    // 点击其他地方隐藏右键菜单
    document.onclick = (e) => {
        if (!DOM.contextMenu.contains(e.target)) {
            hideContextMenu();
        }
    };
    
    // 鼠标滚轮支持标签页滚动
    DOM.tabsContainer = document.getElementById('tabs-container');
    DOM.tabsContainer.addEventListener('wheel', (e) => {
        // 支持横向和纵向滚轮都可以横向滚动标签页
        if (e.deltaX !== 0) {
            e.preventDefault();
            DOM.tabsScroll.scrollLeft += e.deltaX;
        } else if (e.deltaY !== 0) {
            e.preventDefault();
            DOM.tabsScroll.scrollLeft += e.deltaY;
        }
    });
    
    // 垂直滚动条点击跳转
    DOM.verticalScrollbar.onclick = (e) => {
        if (e.target === DOM.verticalScrollbarThumb) return;
        
        const rect = DOM.verticalScrollbar.getBoundingClientRect();
        const thumbHeight = DOM.verticalScrollbarThumb.clientHeight;
        const clickY = e.clientY - rect.top;
        
        const maxTop = DOM.verticalScrollbar.clientHeight - thumbHeight;
        let newTop = clickY - thumbHeight / 2;
        newTop = Math.max(0, Math.min(newTop, maxTop));
        
        const scrollRatio = newTop / maxTop;
        const maxScroll = DOM.codeEditor.scrollHeight - DOM.codeEditor.clientHeight;
        DOM.codeEditor.scrollTop = scrollRatio * maxScroll;
    };
    
    // 代码编辑器输入事件
    DOM.codeEditor.oninput = () => {
        // 检查是否在设置页面或了解页面中
        const currentTab = AppState.openTabs[AppState.activeTab];
        if (currentTab && (currentTab.isAbout || currentTab.isSettings)) {
            // 在非可编辑页面中，清除任何输入内容
            return;
        }
        
        // 更新行号
        updateLineNumbers(DOM.codeEditor.textContent);
        
        // 保存修改的内容到缓存（仅对普通文件标签页）
        if (AppState.activeTab >= 0 && AppState.activeTab < AppState.openTabs.length) {
            const tab = AppState.openTabs[AppState.activeTab];
            if (!tab.isAbout && !tab.isSettings) {
                tab.content = getEditorContent();
                AppState.contentCache.set(tab.path, tab.content);
            }
        }
    };
    
    // 阻止在非可编辑页面中的粘贴操作
    DOM.codeEditor.addEventListener('paste', (e) => {
        const currentTab = AppState.openTabs[AppState.activeTab];
        if (currentTab && (currentTab.isAbout || currentTab.isSettings)) {
            e.preventDefault();
            e.stopPropagation();
        }
    });
    
    // 阻止在非可编辑页面中的拖放操作
    DOM.codeEditor.addEventListener('drop', (e) => {
        const currentTab = AppState.openTabs[AppState.activeTab];
        if (currentTab && (currentTab.isAbout || currentTab.isSettings)) {
            e.preventDefault();
            e.stopPropagation();
        }
    });
}

/**
 * 获取编辑器内容（纯文本）
 */
function getEditorContent() {
    // 遍历编辑器的所有子节点，正确处理换行
    let content = '';
    const childNodes = DOM.codeEditor.childNodes;

    console.log('=== getEditorContent ===');
    console.log('子节点数量:', childNodes.length);

    for (let i = 0; i < childNodes.length; i++) {
        const node = childNodes[i];

        if (node.nodeType === Node.TEXT_NODE) {
            // 文本节点，直接添加内容
            // 即使是空字符串也要添加，因为可能包含空格
            const text = node.textContent || '';
            content += text;
            if (text) {
                console.log('文本节点:', JSON.stringify(text));
            }
        } else if (node.nodeType === Node.ELEMENT_NODE) {
            const tagName = node.tagName?.toLowerCase();

            if (tagName === 'br') {
                // <br> 标签表示换行
                content += '\n';
                console.log('BR元素');
            } else if (tagName === 'div' || tagName === 'p') {
                // <div> 或 <p> 表示新的一行
                // 先检查是否是空行（没有任何子节点或只有空白字符）
                const divContent = node.textContent || '';
                content += divContent;
                // 添加换行符，即使 div 是空的也添加
                content += '\n';
                console.log('DIV元素:', JSON.stringify(divContent));
            } else {
                // 其他元素（如果有），添加其文本内容
                content += node.textContent || '';
                console.log('其他元素:', tagName, JSON.stringify(node.textContent || ''));
            }
        }
    }

    console.log('最终内容长度:', content.length);
    console.log('换行符数量:', (content.match(/\n/g) || []).length);
    console.log('内容预览:', content.substring(0, 300));

    // 不再移除末尾的换行符，保留用户输入的所有空行

    return content;
}

/**
 * 处理保存所有文件
 *
 * 该函数收集所有打开标签页的当前内容并发送到后端保存。
 * 每次保存都会获取编辑器中的最新内容，不受contentCache限制。
 */
async function handleSaveAll() {
    /*
     * 收集所有打开标签页的当前内容
     * 每次都从编辑器获取最新内容，而不是依赖contentCache
     */
    const filesToSave = [];

    /*
     * 遍历所有打开的标签页
     */
    for (let i = 0; i < AppState.openTabs.length; i++) {
        const tab = AppState.openTabs[i];

        /*
         * 如果是当前激活的标签页，从编辑器获取最新内容
         * 否则使用tab.content中缓存的内容
         */
        let content;
        if (i === AppState.activeTab) {
            content = getEditorContent();
            /* 更新tab的缓存内容 */
            tab.content = content;
        } else {
            content = tab.content || '';
        }

        /*
         * 将文件路径和内容添加到保存列表
         */
        filesToSave.push([tab.path, content]);
    }

    /*
     * 如果没有打开的文件，直接返回
     */
    if (filesToSave.length === 0) {
        return;
    }

    /*
     * 调用后端保存所有文件
     */
    const success = await BackendAPI.saveAll(filesToSave);

    /*
     * 显示保存结果弹窗
     */
    if (success) {
        showSaveSuccessNotification();
    } else {
        showSaveFailureNotification();
    }
}

/**
 * 处理刷新
 */
async function handleRefresh() {
    await BackendAPI.refresh();
    
    if (AppState.currentPath) {
        await loadDirectoryContents(AppState.currentPath);
    }
}

/**
 * 处理全屏切换
 */
function handleToggleFullscreen() {
    if (!document.fullscreenElement) {
        document.documentElement.requestFullscreen().catch(err => {
            console.error('进入全屏失败:', err);
        });
    } else {
        document.exitFullscreen();
    }
}

/**
 * 处理Tab缩进
 */
function handleTabIndent() {
    const selection = window.getSelection();
    if (!selection.rangeCount) return;
    
    const range = selection.getRangeAt(0);
    
    // 创建包含4个空格的文本节点
    const spaces = document.createTextNode('    ');
    
    // 插入空格
    range.insertNode(spaces);
    
    // 将光标移动到插入的空格后面
    range.setStartAfter(spaces);
    range.setEndAfter(spaces);
    selection.removeAllRanges();
    selection.addRange(range);
    
    // 更新行号
    updateLineNumbers(DOM.codeEditor.textContent);
    
    // 保存修改的内容到缓存
    if (AppState.activeTab >= 0 && AppState.activeTab < AppState.openTabs.length) {
        const tab = AppState.openTabs[AppState.activeTab];
        if (!tab.isAbout) {
            tab.content = getEditorContent();
            AppState.contentCache.set(tab.path, tab.content);
        }
    }
}

/**
 * 应用程序初始化
 */
function init() {
    initDOM();
    initEventListeners();
    
    // 显示了解页面
    showAboutPage();
    
    // 添加了解标签页
    AppState.openTabs.push({
        path: 'about',
        name: '了解',
        content: '',
        isAbout: true
    });
    AppState.activeTab = 0;
    renderTabs();
    
    console.log('Mikufy v2.3(stable) 初始化完成');
}

/**
 * showSaveSuccessNotification - 显示保存成功通知
 *
 * 在窗口顶部显示保存成功的通知弹窗，包含成功图标和提示文字。
 * 弹窗会在3秒后自动消失。
 */
function showSaveSuccessNotification() {
	/*
	 * 检查是否已存在通知弹窗
	 */
	const existingNotification = document.getElementById('save-notification');
	if (existingNotification) {
		existingNotification.remove();
	}

	/*
	 * 创建通知弹窗元素
	 */
	const notification = document.createElement('div');
	notification.id = 'save-notification';
	notification.className = 'save-notification save-success';

	/*
	 * 创建图标元素
	 */
	const icon = document.createElement('img');
	icon.src = 'Icons/Hight.svg';
	icon.alt = '成功';
	icon.className = 'save-notification-icon';

	/*
	 * 创建文本元素
	 */
	const text = document.createElement('span');
	text.textContent = '保存成功';
	text.className = 'save-notification-text';

	/*
	 * 组装通知弹窗
	 */
	notification.appendChild(icon);
	notification.appendChild(text);

	/*
	 * 添加到页面
	 */
	document.body.appendChild(notification);

	/*
	 * 3秒后自动移除通知
	 */
	setTimeout(() => {
		notification.classList.add('fade-out');
		setTimeout(() => {
			if (notification && notification.parentNode) {
				notification.parentNode.removeChild(notification);
			}
		}, 300);
	}, 3000);
}

/**
 * showSaveFailureNotification - 显示保存失败通知
 *
 * 在窗口顶部显示保存失败的通知弹窗，包含失败图标和提示文字。
 * 弹窗会在3秒后自动消失。
 */
function showSaveFailureNotification() {
	/*
	 * 检查是否已存在通知弹窗
	 */
	const existingNotification = document.getElementById('save-notification');
	if (existingNotification) {
		existingNotification.remove();
	}

	/*
	 * 创建通知弹窗元素
	 */
	const notification = document.createElement('div');
	notification.id = 'save-notification';
	notification.className = 'save-notification save-failure';

	/*
	 * 创建图标元素
	 */
	const icon = document.createElement('img');
	icon.src = 'Icons/Low.svg';
	icon.alt = '失败';
	icon.className = 'save-notification-icon';

	/*
	 * 创建文本元素
	 */
	const text = document.createElement('span');
	text.textContent = '保存失败';
	text.className = 'save-notification-text';

	/*
	 * 组装通知弹窗
	 */
	notification.appendChild(icon);
	notification.appendChild(text);

	/*
	 * 添加到页面
	 */
	document.body.appendChild(notification);

	/*
	 * 3秒后自动移除通知
	 */
	setTimeout(() => {
		notification.classList.add('fade-out');
		setTimeout(() => {
			if (notification && notification.parentNode) {
				notification.parentNode.removeChild(notification);
			}
		}, 300);
	}, 3000);
}

// 页面加载完成后初始化
window.onload = init;

// 页面关闭前保存
window.onbeforeunload = async () => {
    await handleSaveAll();
};
