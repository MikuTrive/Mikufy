/**
 * Mikufy v2.11-nova - 代码编辑器前端JavaScript
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
 * 版本：v2.11-nova
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
    // 终端相关状态
    terminalVisible: false,       // 终端是否可见
    terminalPath: null,           // 终端当前路径
    terminalUser: null,           // 终端用户名
    terminalHostname: null,       // 终端主机名
    terminalIsRoot: false,        // 是否为root用户
    terminalCommandHistory: [],   // 命令历史
    terminalHistoryIndex: -1,     // 命令历史索引
    terminalHeight: 126,          // 终端高度（默认为6个行数高度）
    terminalCurrentPid: null,     // 当前活动进程的ID
    terminalPollInterval: null,   // 轮询进程输出的定时器
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
        '.jpeg': 'Image-24.png',
        '.jpg': 'Image-24.png',
        '.png': 'Image-24.png',
        '.webp': 'Image-24.png',
        '.svg': 'Image-24.png',
        '.mp3': 'Music-24.png',
        '.mp4': 'Video-24.png',
        '.webm': 'Video-24.png',
        '.wav': 'Music-24.png',
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
        '.mk': 'MakeFile-24.svg',
        '.Cmake': 'Cmake-24.svg',
        '.nix': 'Nix-24.svg',
        '.zig': 'Zig-24.svg',
        '.zip': 'Zip-24.svg',
        '.7z': '7z-24.svg',
        '.gz': 'Tar-24.svg',
        '.bz2': 'Tar-24.svg',
        '.tgz': 'Tar-24.svg',
        '.cab': 'Cab-24.svg',
        '.jar': 'Jar-24.svg',
        '.war': 'Jar-24.svg',
        '.xz': 'Tar-24.svg',
        '.rar': 'Rar-24.svg',
        '.iso': 'ISO-24.png',
        '.jsx': 'React-24.svg',
        '.rpm': 'RPM-24.svg',
        '.deb': 'DEB-24.svg',
        '.img': 'Kernel-24.svg'
    },
    // 渲染结果缓存（Map类型，用于存储已渲染的 HTML）
    // 键：文件路径，值：{ lines: Array<string>, totalLines: number }
    renderCache: new Map(),
    // 缓存大小限制（MB）
    cacheSizeLimit: 50,
    // 当前缓存大小（MB）
    currentCacheSize: 0
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
    },
    
    /**
     * 语法高亮
     *
     * 对代码进行语法高亮处理，返回高亮后的 HTML。
     *
     * @async
     * @param {string} code 要高亮的代码内容
     * @param {string} language 编程语言（如 "cpp", "javascript", "python" 等）
     * @param {string} filename 文件名（用于语言检测）
     * @returns {Promise<Object>} 高亮结果对象，包含：
     *   - success {boolean}: 是否成功
     *   - html {string}: 高亮后的 HTML 内容
     *   - language {string}: 使用的语言
     * @throws {Error} 网络请求失败时抛出错误
     *
     * @example
     * const result = await BackendAPI.highlightCode(code, 'cpp', 'main.cpp');
     * if (result.success) {
     *     console.log(result.html);
     * }
     */

    /**
     * 获取终端信息
     *
     * 获取当前用户名、主机名和当前路径信息
     *
     * @async
     * @param {string} path 终端路径（可选）
     * @returns {Promise<Object>} 终端信息对象，包含：
     *   - success {boolean}: 是否成功
     *   - user {string}: 用户名
     *   - hostname {string}: 主机名
     *   - path {string}: 当前路径
     *   - isRoot {boolean}: 是否为root用户
     * @throws {Error} 网络请求失败时抛出错误
     */
    async getTerminalInfo(path) {
        try {
            const response = await fetch('/api/terminal-info', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({ path: path || AppState.terminalPath })
            });
            const data = await response.json();
            return data;
        } catch (error) {
            console.error('获取终端信息失败:', error);
            return { success: false, user: '', hostname: '', path: '', isRoot: false };
        }
    },

    /**
     * 执行终端命令
     *
     * 在指定路径下执行shell命令
     *
     * @async
     * @param {string} command 要执行的命令
     * @param {string} path 执行路径
     * @returns {Promise<Object>} 执行结果对象，包含：
     *   - success {boolean}: 是否成功
     *   - output {string}: 命令输出
     *   - error {string}: 错误信息
     * @throws {Error} 网络请求失败时抛出错误
     */
    async executeCommand(command, path, interrupt = false) {
        try {
            const response = await fetch('/api/terminal-execute', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({ command, path: path || AppState.terminalPath, interrupt })
            });
            const data = await response.json();
            return data;
        } catch (error) {
            console.error('执行命令失败:', error);
            return { success: false, output: '', error: error.message };
        }
    },

    /**
     * 获取交互式进程的输出
     *
     * @async
     * @param {number} pid 进程ID
     * @returns {Promise<Object>} 包含 success, output, is_running, pid 的对象
     */
    async getProcessOutput(pid) {
        try {
            const response = await fetch('/api/terminal-get-output', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({ pid })
            });
            const data = await response.json();
            return data;
        } catch (error) {
            console.error('获取进程输出失败:', error);
            return { success: false, output: '', is_running: false, pid };
        }
    },

    /**
     * 向交互式进程发送输入
     *
     * @async
     * @param {number} pid 进程ID
     * @param {string} input 输入内容
     * @returns {Promise<Object>} 包含 success 的对象
     */
    async sendProcessInput(pid, input) {
        try {
            const response = await fetch('/api/terminal-send-input', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({ pid, input })
            });
            const data = await response.json();
            return data;
        } catch (error) {
            console.error('发送进程输入失败:', error);
            return { success: false };
        }
    },

    /**
     * 终止交互式进程
     *
     * @async
     * @param {number} pid 进程ID
     * @returns {Promise<Object>} 包含 success 的对象
     */
    async killProcess(pid) {
        try {
            const response = await fetch('/api/terminal-kill-process', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({ pid })
            });
            const data = await response.json();
            return data;
        } catch (error) {
            console.error('终止进程失败:', error);
            return { success: false };
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
    btnTerminal: null,        // 打开终端按钮

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

    // 终端区域
    terminalView: null,       // 终端视图容器
    terminalContent: null,    // 终端内容区域
    terminalResizeHandle: null, // 终端高度调整横条

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
    messageDialogClose: null,  // 消息弹窗关闭按钮

    // 通知栏
    notification: null,       // 通知栏容器
    notificationText: null    // 通知栏文本元素
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
    DOM.btnTerminal = document.getElementById('btn-terminal');
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
    DOM.terminalView = document.getElementById('terminal-view');
    DOM.terminalContent = document.getElementById('terminal-content');
    DOM.terminalResizeHandle = document.getElementById('terminal-resize-handle');

    console.log('[initDOM] DOM.lineNumbers 元素:', DOM.lineNumbers);
    console.log('[initDOM] DOM.codeEditor 元素:', DOM.codeEditor);
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

    // 通知栏元素
    DOM.notification = document.getElementById('notification');
    if (DOM.notification) {
        DOM.notificationText = DOM.notification.querySelector('.notification-text');
    } else {
        DOM.notificationText = null;
    }

    // 默认禁用终端按钮
    if (DOM.btnTerminal) {
        DOM.btnTerminal.disabled = true;
    }
}

/**
 * 显示通知（从头顶下滑）
 *
 * 显示一个从头顶下滑的通知条，用于显示操作成功等信息
 * 通知会自动在3秒后消失
 *
 * @param {string} message 通知消息内容
 * @returns {void}
 *
 * @example
 * showNotification('所有文件已保存成功!');
 */
function showNotification(message) {
    if (!DOM.notification || !DOM.notificationText) {
        console.error('[showNotification] 通知元素未初始化');
        return;
    }
    
    // 设置通知文本
    DOM.notificationText.textContent = message;
    
    // 显示通知栏（添加show类触发动画）
    DOM.notification.classList.add('show');
    
    // 3秒后自动隐藏
    setTimeout(() => {
        DOM.notification.classList.remove('show');
    }, 3000);
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
    // 清空文件树容器
    DOM.fileTree.innerHTML = '';

    // 限制显示的文件数量，防止一次性渲染太多文件
    const MAX_FILES_DISPLAY = 500;
    const displayFiles = files.slice(0, MAX_FILES_DISPLAY);

    // 如果文件数量超过限制，显示提示信息
    if (files.length > MAX_FILES_DISPLAY) {
        const warningDiv = document.createElement('div');
        warningDiv.className = 'file-tree-warning';
        warningDiv.textContent = `文件过多，仅显示前 ${MAX_FILES_DISPLAY} 个文件（共 ${files.length} 个）`;
        warningDiv.style.cssText = 'padding: 10px; background: rgba(255, 152, 0, 0.2); color: #ff9800; font-size: 12px; text-align: center;';
        DOM.fileTree.appendChild(warningDiv);
    }

    // 排序规则：目录在前，文件在后，同类型按名称排序
    const sortedFiles = displayFiles.sort((a, b) => {
        if (a.isDirectory !== b.isDirectory) {
            return a.isDirectory ? -1 : 1;
        }
        return a.name.localeCompare(b.name);
    });

    // 使用DocumentFragment进行批量DOM操作
    const fragment = document.createDocumentFragment();

    // 遍历排序后的文件列表
    sortedFiles.forEach(file => {
        const item = document.createElement('div');
        item.className = 'file-tree-item' + (file.isDirectory ? ' directory' : '');
        item.dataset.path = file.path;
        item.dataset.name = file.name;
        item.dataset.isDirectory = file.isDirectory;

        // 如果是目录，添加展开按钮
        if (file.isDirectory) {
            const expandBtn = document.createElement('button');
            expandBtn.className = 'file-tree-expand-btn';
            expandBtn.innerHTML = `<img src="../Icons/Spread-out.png" alt="展开">`;
            expandBtn.onclick = (e) => {
                e.stopPropagation();
                enterDirectory(file.path, file.name);
            };
            item.appendChild(expandBtn);
        }

        // LICENSE文件特殊处理
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

        // 绑定点击事件
        item.onclick = () => handleFileItemClick(file);

        // 绑定右键菜单事件
        item.oncontextmenu = (e) => {
            e.preventDefault();
            showContextMenu(e, file);
        };

        fragment.appendChild(item);
    });

    // 一次性将所有文件树项添加到DOM中
    DOM.fileTree.appendChild(fragment);
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

    // 纯文本模式，不进行语法高亮
    // 根据文件扩展名简单判断语言（仅用于显示，不影响渲染）
    let language = '';
    const ext = name.split('.').pop().toLowerCase();
    const langMap = {
        'c': 'c', 'cpp': 'cpp', 'h': 'cpp', 'hpp': 'cpp',
        'js': 'javascript', 'jsx': 'javascript', 'ts': 'typescript', 'tsx': 'typescript',
        'py': 'python', 'java': 'java', 'go': 'go', 'rs': 'rust',
        'sh': 'shell', 'html': 'html', 'css': 'css', 'json': 'json'
    };
    language = langMap[ext] || '';

    // 添加新标签页
    AppState.openTabs.push({ path, name, content, language });
    activateTab(AppState.openTabs.length - 1);

    // 渲染标签页
    renderTabs();

    // 启用终端按钮
    if (DOM.btnTerminal) {
        DOM.btnTerminal.disabled = false;
    }
}

/**
 * 激活指定标签页
 * @param {number} index 标签页索引
 */
async function activateTab(index) {
    if (index < 0 || index >= AppState.openTabs.length) {
        return;
    }

    // 获取当前激活的标签页索引
    const previousIndex = AppState.activeTab;
    AppState.activeTab = index;
    const tab = AppState.openTabs[index];

    // 添加过渡动画
    const tabItems = document.querySelectorAll('.tab-item');
    tabItems.forEach((item, i) => {
        if (i === index) {
            // 激活新标签页
            item.classList.add('active');
            item.style.transition = 'background-color 0.15s ease, transform 0.15s ease';
        } else if (i === previousIndex) {
            // 取消之前的标签页
            item.classList.remove('active');
        }
    });

    // 自动滚动到当前标签页
    ensureTabVisible(index);

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
            // 设置行号可见性
            DOM.editorContainer.classList.remove('media-mode');
            DOM.lineNumbers.style.opacity = '0.5';
            DOM.codeEditor.contentEditable = 'true';

            // 清理之前的虚拟滚动状态和事件监听器
            if (DOM.codeEditor._virtualScrollHandler) {
                DOM.codeEditor.removeEventListener('scroll', DOM.codeEditor._virtualScrollHandler);
                DOM.codeEditor._virtualScrollHandler = null;
            }
            if (DOM.codeEditor._lastStartLine !== undefined) {
                delete DOM.codeEditor._lastStartLine;
            }
            if (DOM.codeEditor._lastEndLine !== undefined) {
                delete DOM.codeEditor._lastEndLine;
            }

            // 清理防抖定时器
            if (DOM.codeEditor._scrollDebounceTimer) {
                clearTimeout(DOM.codeEditor._scrollDebounceTimer);
                DOM.codeEditor._scrollDebounceTimer = null;
            }
            if (DOM.codeEditor._inputDebounceTimer) {
                clearTimeout(DOM.codeEditor._inputDebounceTimer);
                DOM.codeEditor._inputDebounceTimer = null;
            }

            // 使用语言信息和原始内容
            const language = tab.language || '';
            const content = tab.rawContent || tab.content || '';
            
            if (content) {
                // 重新渲染内容（根据文件大小选择渲染方式）
                const lines = content.split('\n');
                const totalLines = lines.length;
                const USE_VIRTUAL_SCROLL_THRESHOLD = 5000;
                const useVirtualScroll = totalLines > USE_VIRTUAL_SCROLL_THRESHOLD;

                if (useVirtualScroll) {
                    // 使用虚拟滚动
                    console.log('[activateTab] 使用虚拟滚动，行数:', totalLines);
                    tab.isVirtualScroll = true;
                    renderVirtualScroll(lines, totalLines, language, tab);
                } else {
                    // 使用完全渲染
                    console.log('[activateTab] 使用完全渲染，行数:', totalLines);
                    tab.isVirtualScroll = false;
                    renderFullContent(lines, totalLines, language, tab);
                }

                // 恢复滚动位置
                if (tab.scrollTop !== undefined) {
                    DOM.codeEditor.scrollTop = tab.scrollTop;
                }

                // 确保行号正确显示（重要：解决切换标签页后行号不显示的问题）
                // 使用 requestAnimationFrame 确保在 DOM 更新后执行
                requestAnimationFrame(() => {
                    requestAnimationFrame(() => {
                        updateLineNumbers(content);
                        console.log('[activateTab] 已确保行号更新');
                    });
                });

                // 调试：检查渲染后的 opacity 值
                console.log('[activateTab] 渲染后 DOM.lineNumbers.style.opacity:', DOM.lineNumbers.style.opacity);
                console.log('[activateTab] 渲染后 DOM.lineNumbers computedStyle.opacity:', window.getComputedStyle(DOM.lineNumbers).opacity);
            } else {
                // 空内容 - 清空编辑器并显示一个空行
                DOM.codeEditor.innerHTML = '';
                DOM.lineNumbers.innerHTML = '<div class="line-number">1</div>';
            }
        }
    }
}

/**
 * 确保标签页可见（自动滚动）
 * @param {number} index 标签页索引
 */
function ensureTabVisible(index) {
    const tabItems = document.querySelectorAll('.tab-item');
    if (index < 0 || index >= tabItems.length) {
        return;
    }

    const targetTab = tabItems[index];
    const container = DOM.tabsScroll;

    if (!targetTab || !container) {
        return;
    }

    // 获取标签页和容器的位置信息（统一使用 getBoundingClientRect）
    const tabRect = targetTab.getBoundingClientRect();
    const containerRect = container.getBoundingClientRect();

    const containerLeft = containerRect.left;
    const containerRight = containerRect.right;
    const tabLeft = tabRect.left;
    const tabRight = tabRect.right;

    // 如果标签页在左侧不可见区域
    if (tabLeft < containerLeft) {
        // 滚动到标签页左边缘，在容器左侧留 20px 空间
        const scrollOffset = tabLeft - containerLeft - 20;
        container.scrollLeft += scrollOffset;
    }
    // 如果标签页在右侧不可见区域
    else if (tabRight > containerRight) {
        // 滚动到标签页右边缘，在容器右侧留 20px 空间
        const scrollOffset = tabRight - containerRight + 20;
        container.scrollLeft += scrollOffset;
    }
}

/**
 * 显示代码内容
 * @param {string} content 代码内容
 * @param {string} language 编程语言（可选）
 */
async function showCodeContent(content, language = '') {
    DOM.editorContainer.classList.remove('media-mode');
    DOM.lineNumbers.style.opacity = '0.5';

    // 设置代码内容为可编辑
    DOM.codeEditor.contentEditable = 'true';

    // 清空编辑器
    DOM.codeEditor.innerHTML = '';

    // 移除之前的虚拟滚动事件监听器和防抖定时器（如果有）
    if (DOM.codeEditor._virtualScrollHandler) {
        DOM.codeEditor.removeEventListener('scroll', DOM.codeEditor._virtualScrollHandler);
        DOM.codeEditor._virtualScrollHandler = null;
    }
    if (DOM.codeEditor._scrollDebounceTimer) {
        clearTimeout(DOM.codeEditor._scrollDebounceTimer);
        DOM.codeEditor._scrollDebounceTimer = null;
    }

    // 移除之前的输入防抖定时器
    if (DOM.codeEditor._inputDebounceTimer) {
        clearTimeout(DOM.codeEditor._inputDebounceTimer);
        DOM.codeEditor._inputDebounceTimer = null;
    }

    if (content) {
        // 获取当前标签页，存储原始内容和语言
        const currentTab = AppState.openTabs[AppState.activeTab];
        if (currentTab) {
            currentTab.rawContent = content; // 存储原始未高亮内容
            currentTab.content = content;    // 存储完整内容（用于保存）
            currentTab.language = language;   // 存储语言
            currentTab.lines = content.split('\n'); // 存储所有行
            currentTab.isVirtualScroll = false; // 标记不使用虚拟滚动
        }

        // 按行分割内容
        const lines = content.split('\n');
        const totalLines = lines.length;

        // 判断是否需要使用虚拟滚动（超过5000行）
        const USE_VIRTUAL_SCROLL_THRESHOLD = 5000;
        const useVirtualScroll = totalLines > USE_VIRTUAL_SCROLL_THRESHOLD;

        if (useVirtualScroll) {
            // 使用虚拟滚动渲染大文件
            currentTab.isVirtualScroll = true;
            renderVirtualScroll(lines, totalLines, language, currentTab);
        } else {
            // 使用完全渲染（分批）处理中小文件
            renderFullContent(lines, totalLines, language, currentTab);
        }
    } else {
        // 空内容
        DOM.lineNumbers.innerHTML = '<div class="line-number">1</div>';
    }
}

/**
 * 完全渲染内容（用于中小文件）
 * @param {Array<string>} lines 所有行
 * @param {number} totalLines 总行数
 * @param {string} language 编程语言
 * @param {Object} tab 当前标签页
 */
function renderFullContent(lines, totalLines, language, tab) {
    // 清空编辑器和行号内容（重要：防止旧内容残留）
    DOM.codeEditor.innerHTML = '';
    DOM.lineNumbers.innerHTML = '';
    
    // 重置滚动位置
    DOM.codeEditor.scrollTop = 0;
    DOM.codeEditor._lastStartLine = 0;
    DOM.codeEditor._lastEndLine = 0;

    // 使用 DocumentFragment 批量添加 DOM 元素
    const fragment = document.createDocumentFragment();

    // 分批渲染，避免一次性渲染太多行导致卡顿
    const BATCH_SIZE = 500;
    const firstBatchSize = Math.min(BATCH_SIZE, totalLines);

    // 渲染第一批（前 500 行）
    for (let i = 0; i < firstBatchSize; i++) {
        const lineDiv = document.createElement('div');
        lineDiv.dataset.lineIndex = i;
        lineDiv.style.minHeight = '21px';
        lineDiv.textContent = lines[i] || '';
        fragment.appendChild(lineDiv);
    }

    // 添加第一批内容
    DOM.codeEditor.appendChild(fragment);

    // 立即更新行号（使用统一的行号更新函数）
    updateLineNumbersOptimized(totalLines);

    // 分批渲染剩余的行
    if (totalLines > BATCH_SIZE) {
        let currentIndex = BATCH_SIZE;

        function renderNextBatch() {
            const endIndex = Math.min(currentIndex + BATCH_SIZE, totalLines);

            if (currentIndex >= totalLines) {
                // 渲染完成，确保行号正确
                requestAnimationFrame(() => {
                    updateLineNumbers(lines.join('\n'));
                });
                return;
            }

            const batchFragment = document.createDocumentFragment();
            for (let i = currentIndex; i < endIndex; i++) {
                const lineDiv = document.createElement('div');
                lineDiv.dataset.lineIndex = i;
                lineDiv.style.minHeight = '21px';
                lineDiv.textContent = lines[i] || '';
                batchFragment.appendChild(lineDiv);
            }

            DOM.codeEditor.appendChild(batchFragment);

            currentIndex = endIndex;

            // 继续下一批
            if (currentIndex < totalLines) {
                setTimeout(renderNextBatch, 0);
            }
        }

        setTimeout(renderNextBatch, 0);
    } else {
        // 没有分批渲染，确保行号正确
        requestAnimationFrame(() => {
            updateLineNumbers(lines.join('\n'));
        });
    }

    // 纯文本模式，不进行语法高亮
    // 已移除 applySyntaxHighlightingFull 调用
}

/**
 * 虚拟滚动渲染（用于大文件，支持增量高亮）
 * @param {Array<string>} lines 所有行
 * @param {number} totalLines 总行数
 * @param {string} language 编程语言
 * @param {Object} tab 当前标签页
 */
function renderVirtualScroll(lines, totalLines, language, tab) {
    // 移除旧的事件监听器
    if (DOM.codeEditor._virtualScrollHandler) {
        DOM.codeEditor.removeEventListener('scroll', DOM.codeEditor._virtualScrollHandler);
        DOM.codeEditor._virtualScrollHandler = null;
    }

    // 清空编辑器和行号内容（重要：防止旧内容残留）
    DOM.codeEditor.innerHTML = '';
    DOM.lineNumbers.innerHTML = '';
    
    // 重置滚动位置
    DOM.codeEditor.scrollTop = 0;
    DOM.codeEditor._lastStartLine = 0;
    DOM.codeEditor._lastEndLine = 0;

    const lineHeight = 21;
    const bufferLines = 10;
    const containerHeight = DOM.codeEditor.clientHeight;
    const visibleLines = Math.ceil(containerHeight / lineHeight) + bufferLines;
    const startLine = 0;
    const endLine = Math.min(visibleLines, totalLines);

    tab.isVirtualScroll = true;

    renderVisibleRangeWithHighlight(lines, startLine, endLine, totalLines, lineHeight, language, tab);
    updateLineNumbersForVirtualScroll(startLine, endLine, totalLines);

    DOM.codeEditor._virtualScrollHandler = () => {
        requestAnimationFrame(() => {
            const scrollTop = DOM.codeEditor.scrollTop;
            const newStartLine = Math.max(0, Math.floor(scrollTop / lineHeight) - bufferLines);
            const newEndLine = Math.min(totalLines, newStartLine + visibleLines);

            if (newStartLine !== DOM.codeEditor._lastStartLine || newEndLine !== DOM.codeEditor._lastEndLine) {
                renderVisibleRangeWithHighlight(lines, newStartLine, newEndLine, totalLines, lineHeight, language, tab);
                updateLineNumbersForVirtualScroll(newStartLine, newEndLine, totalLines);
                DOM.codeEditor._lastStartLine = newStartLine;
                DOM.codeEditor._lastEndLine = newEndLine;
            }
        });
    };

    DOM.codeEditor.addEventListener('scroll', DOM.codeEditor._virtualScrollHandler);
    DOM.codeEditor._lastStartLine = startLine;
    DOM.codeEditor._lastEndLine = endLine;

    console.log(`使用虚拟滚动渲染大文件（纯文本模式），共 ${totalLines} 行`);
}

/**
 * 渲染可见范围的行（支持增量高亮）
 * @param {Array<string>} lines 所有行
 * @param {number} startLine 起始行号
 * @param {number} endLine 结束行号
 * @param {number} totalLines 总行数
 * @param {number} lineHeight 行高
 * @param {string} language 编程语言
 * @param {Object} tab 当前标签页
 */
function renderVisibleRangeWithHighlight(lines, startLine, endLine, totalLines, lineHeight, language, tab) {
    // 清空编辑器内容（重要：防止旧内容残留）
    DOM.codeEditor.innerHTML = '';
    
    const fragment = document.createDocumentFragment();

    if (startLine > 0) {
        const topPlaceholder = document.createElement('div');
        topPlaceholder.style.height = (startLine * lineHeight) + 'px';
        topPlaceholder.style.visibility = 'hidden';
        fragment.appendChild(topPlaceholder);
    }

    renderLinesWithHighlight(lines, startLine, endLine, fragment, language, tab);

    if (endLine < totalLines) {
        const bottomPlaceholder = document.createElement('div');
        bottomPlaceholder.style.height = ((totalLines - endLine) * lineHeight) + 'px';
        bottomPlaceholder.style.visibility = 'hidden';
        fragment.appendChild(bottomPlaceholder);
    }

    DOM.codeEditor.appendChild(fragment);
}

/**
 * 渲染行（纯文本模式，无语法高亮）
 * @param {Array<string>} lines 所有行
 * @param {number} startLine 起始行号
 * @param {number} endLine 结束行号
 * @param {DocumentFragment} fragment DOM片段
 * @param {string} language 编程语言（已弃用参数，保留兼容性）
 * @param {Object} tab 当前标签页（已弃用参数，保留兼容性）
 */
async function renderLinesWithHighlight(lines, startLine, endLine, fragment, language, tab) {
    // 纯文本模式，不进行语法高亮
    // 使用textContent设置纯文本，避免HTML标签污染
    for (let i = startLine; i < endLine; i++) {
        const lineDiv = document.createElement('div');
        lineDiv.dataset.lineIndex = i;
        lineDiv.style.minHeight = '21px';
        lineDiv.textContent = lines[i] || '';
        fragment.appendChild(lineDiv);
    }
}

/**
 * 优化的行号更新函数
 * @param {number} totalLines 总行数
 */
function updateLineNumbersOptimized(totalLines) {
    console.log('[updateLineNumbersOptimized] 更新行号，总行数:', totalLines);

    // 分批渲染行号，避免一次性生成太多HTML导致浏览器崩溃
    const BATCH_SIZE = 500;
    const firstBatchSize = Math.min(BATCH_SIZE, totalLines);

    // 先清空行号区域
    DOM.lineNumbers.innerHTML = '';

    // 渲染第一批行号
    const fragment = document.createDocumentFragment();
    for (let i = 1; i <= firstBatchSize; i++) {
        const lineDiv = document.createElement('div');
        lineDiv.className = 'line-number';
        lineDiv.textContent = i;
        fragment.appendChild(lineDiv);
    }
    DOM.lineNumbers.appendChild(fragment);

    // 分批渲染剩余行号
    if (totalLines > BATCH_SIZE) {
        let currentIndex = BATCH_SIZE + 1;

        function renderNextBatch() {
            const endIndex = Math.min(currentIndex + BATCH_SIZE, totalLines);

            if (currentIndex > totalLines) {
                return; // 渲染完成
            }

            const batchFragment = document.createDocumentFragment();
            for (let i = currentIndex; i <= endIndex; i++) {
                const lineDiv = document.createElement('div');
                lineDiv.className = 'line-number';
                lineDiv.textContent = i;
                batchFragment.appendChild(lineDiv);
            }
            DOM.lineNumbers.appendChild(batchFragment);

            currentIndex = endIndex + 1;

            // 继续渲染下一批
            requestAnimationFrame(renderNextBatch);
        }

        // 开始渲染剩余批次
        requestAnimationFrame(renderNextBatch);
    }
}

/**
 * 添加到渲染缓存（优化版）
 * @param {string} path 文件路径
 * @param {Object} data 渲染数据
 */
function addToRenderCache(path, data) {
    try {
        const size = estimateCacheSize(data);

        if (AppState.renderCache.has(path)) {
            const oldData = AppState.renderCache.get(path);
            const oldSize = estimateCacheSize(oldData);
            AppState.currentCacheSize -= oldSize;
        }

        if (AppState.currentCacheSize + size > AppState.cacheSizeLimit) {
            evictOldestCacheEntries(size);
        }

        AppState.renderCache.set(path, data);
        AppState.currentCacheSize += size;
    } catch (error) {
        console.error('缓存管理失败:', error);
    }
}

/**
 * 估算缓存大小（优化版，不使用Blob）
 * @param {Object} data 缓存数据
 * @returns {number} 大小（MB）
 */
function estimateCacheSize(data) {
    let size = 0;
    if (data && data.lines) {
        for (let i = 0; i < data.lines.length; i++) {
            size += data.lines[i].length;
        }
    }
    return size / (1024 * 1024);
}

/**
 * 清理最旧的缓存条目
 * @param {number} requiredSize 需要释放的空间（MB）
 */
function evictOldestCacheEntries(requiredSize) {
    const entries = Array.from(AppState.renderCache.entries());
    let cleared = 0;
    for (let i = 0; i < entries.length; i++) {
        const [key, value] = entries[i];
        const entrySize = estimateCacheSize(value);
        AppState.renderCache.delete(key);
        AppState.currentCacheSize -= entrySize;
        cleared += entrySize;
        if (AppState.currentCacheSize + requiredSize <= AppState.cacheSizeLimit) {
            break;
        }
    }
    console.log(`清理了 ${cleared.toFixed(2)} MB 缓存`);
}

/**
 * 从缓存获取渲染结果
 * @param {string} path 文件路径
 * @returns {Object|null} 渲染数据或 null
 */
function getFromRenderCache(path) {
    return AppState.renderCache.get(path) || null;
}

/**
 * 清除指定文件的缓存
 * @param {string} path 文件路径
 */
function clearRenderCache(path) {
    if (AppState.renderCache.has(path)) {
        const data = AppState.renderCache.get(path);
        const size = new Blob([JSON.stringify(data)]).size / (1024 * 1024);
        AppState.renderCache.delete(path);
        AppState.currentCacheSize -= size;
    }
}

/**
 * 在后台应用完整的语法高亮（已废弃，纯文本模式）
 * @param {Array<string>} lines 所有行
 * @param {string} language 编程语言（已弃用参数，保留兼容性）
 * @param {Object} tab 当前标签页（已弃用参数，保留兼容性）
 */
async function applySyntaxHighlightingFull(lines, language, tab) {
    // 纯文本模式，不进行语法高亮
    // 此函数已废弃，保留用于兼容性
    // 直接返回，不进行任何操作
}

/**
 * 渲染可见区域的行（虚拟滚动，纯文本模式）
 * @param {number} startLine 起始行号
 * @param {number} endLine 结束行号（不包含）
 * @param {Array<string>} lines 所有行
 * @param {Object} tab 当前标签页（已弃用参数，保留兼容性）
 */
async function renderVisibleLines(startLine, endLine, lines, tab) {
    // 使用 DocumentFragment 批量添加 DOM 元素
    const fragment = document.createDocumentFragment();

    // 创建占位符以保持滚动位置
    if (startLine > 0) {
        const placeholder = document.createElement('div');
        placeholder.style.height = (startLine * 21) + 'px';
        placeholder.style.visibility = 'hidden';
        fragment.appendChild(placeholder);
    }

    // 纯文本模式，不进行语法高亮
    // 使用textContent设置纯文本，避免HTML标签污染
    for (let i = startLine; i < endLine; i++) {
        const lineDiv = document.createElement('div');
        lineDiv.dataset.lineIndex = i;
        lineDiv.style.minHeight = '21px';
        lineDiv.textContent = lines[i] || '';
        fragment.appendChild(lineDiv);
    }

    // 创建占位符以保持滚动位置
    if (endLine < lines.length) {
        const placeholder = document.createElement('div');
        placeholder.style.height = ((lines.length - endLine) * 21) + 'px';
        placeholder.style.visibility = 'hidden';
        fragment.appendChild(placeholder);
    }

    // 清空编辑器并添加新内容
    DOM.codeEditor.innerHTML = '';
    DOM.codeEditor.appendChild(fragment);
}

/**
 * 更新行号（虚拟滚动版本）
 * @param {number} startLine 起始行号
 * @param {number} endLine 结束行号
 * @param {number} totalLines 总行数
 */
function updateLineNumbersForVirtualScroll(startLine, endLine, totalLines) {
    let lineNumbersHtml = '';
    for (let i = startLine; i < endLine; i++) {
        lineNumbersHtml += `<div class="line-number">${i + 1}</div>`;
    }

    // 为行号容器添加占位符以保持对齐
    const lineHeight = 21;
    let placeholderTop = '';
    let placeholderBottom = '';

    if (startLine > 0) {
        placeholderTop = `<div style="height: ${startLine * lineHeight}px; visibility: hidden;"></div>`;
    }

    if (endLine < totalLines) {
        placeholderBottom = `<div style="height: ${(totalLines - endLine) * lineHeight}px; visibility: hidden;"></div>`;
    }

    const finalHtml = placeholderTop + lineNumbersHtml + placeholderBottom;
    console.log('[updateLineNumbersForVirtualScroll] startLine:', startLine, 'endLine:', endLine, 'totalLines:', totalLines);
    console.log('[updateLineNumbersForVirtualScroll] lineNumbersHtml 长度:', lineNumbersHtml.length);
    console.log('[updateLineNumbersForVirtualScroll] finalHtml 长度:', finalHtml.length);
    console.log('[updateLineNumbersForVirtualScroll] placeholderTop 高度:', startLine * lineHeight);
    console.log('[updateLineNumbersForVirtualScroll] placeholderBottom 高度:', (totalLines - endLine) * lineHeight);

    DOM.lineNumbers.innerHTML = finalHtml;

    console.log('[updateLineNumbersForVirtualScroll] DOM.lineNumbers.innerHTML 长度:', DOM.lineNumbers.innerHTML.length);
    console.log('[updateLineNumbersForVirtualScroll] DOM.lineNumbers.children.length:', DOM.lineNumbers.children.length);
}

/**
 * 渲染剩余行（延迟渲染）
 * @param {Array<string>} lines 所有行
 * @param {number} startIndex 起始索引
 * @param {string} language 编程语言
 * @param {Object} tab 当前标签页
 */
function renderRemainingLines(lines, startIndex, language, tab) {
    const BATCH_SIZE = 100; // 每批渲染 100 行
    let currentIndex = startIndex;

    function renderBatch() {
        const endIndex = Math.min(currentIndex + BATCH_SIZE, lines.length);

        if (currentIndex >= lines.length) {
            return; // 渲染完成
        }

        const fragment = document.createDocumentFragment();

        for (let i = currentIndex; i < endIndex; i++) {
            const lineDiv = document.createElement('div');
            lineDiv.dataset.lineIndex = i;
            lineDiv.innerHTML = '';
            lineDiv.appendChild(document.createTextNode(lines[i]));
            fragment.appendChild(lineDiv);
        }

        DOM.codeEditor.appendChild(fragment);

        currentIndex = endIndex;

        // 更新行号
        updateLineNumbers(DOM.codeEditor.textContent);

        // 继续下一批渲染
        if (currentIndex < lines.length) {
            requestAnimationFrame(renderBatch);
        }
    }

    renderBatch();
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
 * 如果使用虚拟滚动，从标签页数据获取总行数；否则通过分析DOM结构统计。
 *
 * @content: 代码内容（可选参数，如果提供则直接使用其计算行数）
 *
 * 注意: 只有当 tab.isVirtualScroll 为 true 时，才使用虚拟滚动行号显示；
 *       否则我们优先使用传入的内容参数来计算行数，如果没有传入则从DOM统计。
 */
function updateLineNumbers(content)
{
	// 检查是否使用虚拟滚动（必须同时满足有lines数组且isVirtualScroll标志为true）
	const currentTab = AppState.openTabs[AppState.activeTab];
	if (currentTab && currentTab.isVirtualScroll && currentTab.lines && Array.isArray(currentTab.lines)) {
		// 使用虚拟滚动：使用 updateLineNumbersForVirtualScroll 来显示行号
		// 获取当前可见区域的行号
		const scrollTop = DOM.codeEditor.scrollTop;
		const lineHeight = 21;
		const startLine = Math.max(0, Math.floor(scrollTop / lineHeight));
		const endLine = Math.min(currentTab.lines.length, startLine + 50);

		// 确保至少显示一行
		if (startLine >= currentTab.lines.length) {
			startLine = Math.max(0, currentTab.lines.length - 1);
		}

		updateLineNumbersForVirtualScroll(startLine, Math.max(startLine + 1, endLine), currentTab.lines.length);
		return;
	}

	// 非虚拟滚动模式：优先使用传入的内容参数计算行数
	let lines = 0;
	let editorContent = '';

	// 如果传入了内容参数，优先使用它
	if (content !== undefined && content !== null) {
		editorContent = content;
	} else {
		// 否则从 DOM 获取内容
		editorContent = getEditorContent();
	}

	// 从内容计算行数（这是最准确的方法）
	if (editorContent) {
		// 移除末尾的空换行符（如果末尾只有换行符）
		let trimmedContent = editorContent;
		while (trimmedContent.endsWith('\n') && trimmedContent.length > 0) {
			trimmedContent = trimmedContent.slice(0, -1);
		}
		
		// 计算换行符数量，行数 = 换行符数量 + 1
		const newLineCount = (trimmedContent.match(/\n/g) || []).length;
		lines = newLineCount + 1;
	} else {
		// 空内容，至少显示1行
		lines = 1;
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

	// 同时更新标签页的行数
	if (currentTab) {
		const newLines = editorContent.split('\n');
		currentTab.lines = newLines;
		currentTab.content = editorContent;
		currentTab.rawContent = editorContent;
	}
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
            tab.rawContent = currentContent;
            AppState.contentCache.set(tab.path, currentContent);
        }
        // 保存滚动位置
        tab.scrollTop = DOM.codeEditor.scrollTop;
    }

    // 如果使用虚拟滚动，移除事件监听器
    if (tab.isVirtualScroll && DOM.codeEditor._virtualScrollHandler) {
        DOM.codeEditor.removeEventListener('scroll', DOM.codeEditor._virtualScrollHandler);
        DOM.codeEditor._virtualScrollHandler = null;
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
                        <li><kbd>Ctrl+方向左键</kbd> - 切换到左边的标签页</li>
                        <li><kbd>Ctrl+方向右键</kbd> - 切换到右边的标签页</li>
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
                        <li>右键菜单(重命名、删除)</li>
                        <li>完整编辑器壁纸</li>
                        <li>更高效的性能处理</li>
                        <li>单文件万行内容流畅</li>
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

    // 调用后端API获取壁纸列表
    BackendAPI.getWallpapers = async function() {
        try {
            const response = await fetch('/api/get-wallpapers');
            const data = await response.json();
            return data.wallpapers || [];
        } catch (error) {
            console.error('获取壁纸列表失败:', error);
            return [];
        }
    };

    // 异步加载壁纸
    async function loadWallpapersAsync() {
        const wallpapers = await BackendAPI.getWallpapers();

        // 渲染壁纸列表
        wallpapers.forEach(wallpaper => {
            const filename = wallpaper.filename;
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

    // 开始异步加载
    loadWallpapersAsync();
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

        // 检查是否使用虚拟滚动
        const currentTab = AppState.openTabs[AppState.activeTab];
        const isVirtualScroll = currentTab && currentTab.isVirtualScroll;

        // 只在非虚拟滚动模式下同步行号滚动
        if (!isVirtualScroll) {
            DOM.lineNumbers.scrollTop = scrollTop;
        }

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

    // 终端按钮
    DOM.btnTerminal.onclick = () => {
        toggleTerminal();
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

        // 立即更新行数，不使用防抖
        if (AppState.activeTab >= 0 && AppState.activeTab < AppState.openTabs.length) {
            const tab = AppState.openTabs[AppState.activeTab];
            if (!tab.isAbout && !tab.isSettings) {
                // 立即获取内容并更新行数
                const content = getEditorContent();
                const newLines = content.split('\n');

                // 更新标签页内容和行数
                tab.content = content;
                tab.rawContent = content;
                tab.lines = newLines;
                AppState.contentCache.set(tab.path, content);

                // 立即更新行号，确保行号和内容同步
                updateLineNumbers(content);
            }
        }
    };

/**
 * 更新虚拟滚动模式下的内容
 * @param {Object} tab 当前标签页
 */
function updateVirtualScrollContent(tab) {
    // 获取当前可见区域
    const scrollTop = DOM.codeEditor.scrollTop;
    const lineHeight = 21;
    const bufferLines = 10;
    const containerHeight = DOM.codeEditor.clientHeight;
    const visibleLines = Math.ceil(containerHeight / lineHeight) + bufferLines;

    const startLine = Math.max(0, Math.floor(scrollTop / lineHeight) - bufferLines);
    const endLine = Math.min(tab.lines.length, startLine + visibleLines);

    // 从DOM获取可见区域的内容
    const visibleLinesContent = [];
    const childNodes = DOM.codeEditor.childNodes;

    // 检测是否有换行符插入（行数变化）
    let hasNewLines = false;
    let totalNewLines = 0;

    for (let i = 0; i < childNodes.length; i++) {
        const node = childNodes[i];
        if (node.nodeType === Node.ELEMENT_NODE && node.dataset.lineIndex !== undefined) {
            const lineIndex = parseInt(node.dataset.lineIndex);
            const content = node.textContent;
            
            // 检测该行是否包含换行符（用户按Enter）
            if (content.includes('\n')) {
                hasNewLines = true;
                const newLineCount = (content.match(/\n/g) || []).length;
                totalNewLines += newLineCount;
            }

            if (lineIndex >= startLine && lineIndex < endLine) {
                visibleLinesContent[lineIndex] = content;
            }
        }
    }

    // 如果检测到换行符，需要重新计算行数
    if (hasNewLines && totalNewLines > 0) {
        // 从DOM重新获取所有内容（包括新插入的行）
        const allContent = getEditorContent();
        const newLines = allContent.split('\n');
        
        // 更新标签页的行数
        tab.lines = newLines;
        tab.content = allContent;
        tab.rawContent = allContent;
        AppState.contentCache.set(tab.path, allContent);
        
        // 重新渲染行号（使用新的行数）
        updateLineNumbersOptimized(newLines.length);
        
        console.log('[updateVirtualScrollContent] 检测到换行符，行数从', tab.lines.length - totalNewLines, '更新为', newLines.length);
        return;
    }

    // 如果没有换行符，只更新已修改的行
    let contentChanged = false;
    for (let i = startLine; i < endLine; i++) {
        if (visibleLinesContent[i] !== undefined && visibleLinesContent[i] !== tab.lines[i]) {
            tab.lines[i] = visibleLinesContent[i];
            contentChanged = true;
        }
    }

    // 如果内容有变化，更新缓存
    if (contentChanged) {
        tab.content = tab.lines.join('\n');
        tab.rawContent = tab.content;
        AppState.contentCache.set(tab.path, tab.content);
    }

    // 更新行号（虚拟滚动模式）
    updateLineNumbersForVirtualScroll(startLine, endLine, tab.lines.length);
}
    
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
 * 获取编辑器内容
 *
 * 从编辑器的 DOM 结构中提取文本内容，保留原始格式。
 * 每个行 div 元素对应一行代码，按顺序拼接所有行的内容。
 *
 * 返回: 完整的编辑器内容字符串
 */
function getEditorContent() {
    // 递归获取节点的完整内容，保留所有空格和缩进
    function getNodeContent(node) {
        let content = '';

        if (node.nodeType === Node.TEXT_NODE) {
            // 使用 nodeValue 获取文本节点的原始内容，保留所有空格
            content = node.nodeValue || '';
        } else if (node.nodeType === Node.ELEMENT_NODE) {
            const tagName = node.tagName?.toLowerCase();

            // 忽略占位符元素（visibility: hidden）
            if (node.style && node.style.visibility === 'hidden') {
                return '';
            }

            if (tagName === 'br') {
                // <br> 标签表示换行
                content = '\n';
            } else if (tagName === 'div' || tagName === 'p') {
                // <div> 或 <p> 表示新的一行
                // 递归处理所有子节点
                const childNodes = node.childNodes;
                let lineContent = '';
                for (let i = 0; i < childNodes.length; i++) {
                    lineContent += getNodeContent(childNodes[i]);
                }
                // 总是添加换行符，即使 lineContent 为空
                // 这样可以保留空行（包括只包含空格的行）
                content = lineContent + '\n';
            } else {
                // 其他元素，递归处理所有子节点
                const childNodes = node.childNodes;
                for (let i = 0; i < childNodes.length; i++) {
                    content += getNodeContent(childNodes[i]);
                }
            }
        }

        return content;
    }

    // 遍历编辑器的所有子节点
    let content = '';
    const childNodes = DOM.codeEditor.childNodes;

    for (let i = 0; i < childNodes.length; i++) {
        const node = childNodes[i];
        content += getNodeContent(node);
    }

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
     * 关键修复：总是从编辑器获取最新内容，确保用户编辑的内容被正确保存
     */
    const filesToSave = [];

    /*
     * 遍历所有打开的标签页
     */
    for (let i = 0; i < AppState.openTabs.length; i++) {
        const tab = AppState.openTabs[i];

        // 跳过特殊标签页（了解、设置等）
        if (tab.isAbout || tab.isSettings) {
            continue;
        }

        /*
         * 总是从编辑器获取最新内容，确保用户编辑的内容被正确保存
         * 如果是当前激活标签页，直接从DOM获取
         * 如果不是当前标签页，使用tab.content缓存
         */
        let content;

        if (i === AppState.activeTab) {
            // 当前激活标签页，从DOM获取最新内容
            content = getEditorContent();
            // 更新缓存
            tab.content = content;
            AppState.contentCache.set(tab.path, content);
            console.log(`[保存] 从DOM获取当前标签页 ${tab.path}: 内容长度 ${content.length}`);
        } else if (tab.content) {
            // 非当前标签页，使用缓存内容
            content = tab.content;
            console.log(`[保存] 使用缓存内容保存 ${tab.path}: 内容长度 ${content.length}`);
        } else {
            // 没有内容，跳过
            console.warn(`[保存] 文件 ${tab.path} 没有内容，跳过`);
            continue;
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
        console.log('[保存] 没有需要保存的文件');
        return;
    }

    console.log(`[保存] 准备保存 ${filesToSave.length} 个文件`);

    /*
     * 调用后端保存所有文件
     */
    const success = await BackendAPI.saveAll(filesToSave);

    /*
     * 显示保存结果通知（从头顶下滑）
     */
    if (success) {
        showNotification('所以文件均已保存成功!');
    } else {
        showMessage('保存失败', '错误');
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
 * 解析命令影响的目录
 * @param {string} command - 执行的命令
 * @param {string} basePath - 基准路径（终端当前路径）
 * @returns {string[]} 受影响的目录列表
 */
function parseAffectedDirectories(command, basePath) {
    const dirs = new Set();

    // 解析命令中的路径参数
    const pathRegex = /(?:mkdir|rm|touch|mv|cp|ln)\s+([^\s]+)/g;
    let match;
    while ((match = pathRegex.exec(command)) !== null) {
        let path = match[1];
        if (!path.startsWith('/')) {
            path = basePath + '/' + path;
        }
        // 获取父目录
        const parentPath = path.substring(0, path.lastIndexOf('/'));
        dirs.add(parentPath);
        dirs.add(path);
    }

    // 编译命令通常影响当前目录
    if (command.includes(' build') || command.includes(' compile') || command.includes(' install')) {
        dirs.add(basePath);
    }

    return Array.from(dirs);
}

/**
 * 判断两个目录是否相关
 * @param {string} dir1 - 目录1
 * @param {string} dir2 - 目录2
 * @returns {boolean} 是否相关
 */
function isRelatedDirectory(dir1, dir2) {
    if (!dir1 || !dir2) return false;
    // 规范化路径（去除末尾斜杠）
    const d1 = dir1.endsWith('/') ? dir1.slice(0, -1) : dir1;
    const d2 = dir2.endsWith('/') ? dir2.slice(0, -1) : dir2;
    // 检查是否是相同目录或父子目录
    return d1 === d2 || d1.startsWith(d2 + '/') || d2.startsWith(d1 + '/');
}

/**
 * 智能刷新目录
 * @param {string} command - 执行的命令
 * @param {string} terminalPath - 终端路径
 */
async function smartRefresh(command, terminalPath) {
    // 解析命令影响的目录
    const affectedDirs = parseAffectedDirectories(command, terminalPath);

    // 获取当前文件树路径
    const currentTreePath = AppState.currentPath || AppState.currentDirectory;

    // 判断是否需要刷新
    for (const dir of affectedDirs) {
        // 检查是否是当前目录或父/子目录
        if (isRelatedDirectory(dir, currentTreePath)) {
            // 只刷新当前显示的目录
            if (currentTreePath) {
                await loadDirectoryContents(currentTreePath);
            }
            break;
        }
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

    // 获取完整内容来更新行号（使用 getEditorContent 而不是 textContent）
    const content = getEditorContent();
    updateLineNumbers(content);

    // 保存修改的内容到缓存
    if (AppState.activeTab >= 0 && AppState.activeTab < AppState.openTabs.length) {
        const tab = AppState.openTabs[AppState.activeTab];
        if (!tab.isAbout) {
            tab.content = content;
            tab.rawContent = content;
            tab.lines = content.split('\n');
            AppState.contentCache.set(tab.path, tab.content);
        }
    }
}

/**
 * 应用程序初始化
 */
function init() {
    console.log('[init] 开始初始化...');
    initDOM();
    console.log('[init] DOM初始化完成');
    initEventListeners();
    console.log('[init] 事件监听器初始化完成');

    // 显示了解页面
    showAboutPage();
    console.log('[init] 显示了解页面');

    // 添加了解标签页
    AppState.openTabs.push({
        path: 'about',
        name: '了解',
        content: '',
        isAbout: true
    });
    AppState.activeTab = 0;
    renderTabs();
    console.log('[init] 标签页渲染完成');

    console.log('Mikufy v2.11-nova 初始化完成');
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

// ============================================================================
// 终端功能相关函数
// ============================================================================

/**
 * 打开终端
 *
 * 显示终端视图，初始化终端信息
 */
async function openTerminal() {
    if (AppState.terminalVisible) {
        return;
    }

    // 根据当前文件路径设置终端路径
    if (AppState.currentPath && AppState.openTabs[AppState.activeTab]) {
        const currentTab = AppState.openTabs[AppState.activeTab];
        if (currentTab.path) {
            // 获取文件所在目录
            const pathParts = currentTab.path.split('/');
            pathParts.pop(); // 移除文件名
            AppState.terminalPath = pathParts.join('/') || '/';
        }
    } else if (AppState.currentDirectory) {
        AppState.terminalPath = AppState.currentDirectory;
    }

    if (!AppState.terminalPath) {
        AppState.terminalPath = '/';
    }

    // 获取终端信息
    const terminalInfo = await BackendAPI.getTerminalInfo(AppState.terminalPath);
    if (terminalInfo.success) {
        AppState.terminalUser = terminalInfo.user;
        AppState.terminalHostname = terminalInfo.hostname;
        AppState.terminalPath = terminalInfo.path;
        AppState.terminalIsRoot = terminalInfo.isRoot;
    }

    // 初始化终端高度（使用保存的高度或默认值）
    const terminalHeight = AppState.terminalHeight || 126;
    DOM.terminalView.style.height = `${terminalHeight}px`;

    // 显示终端视图
    DOM.terminalView.classList.add('visible');
    DOM.btnTerminal.classList.add('active');
    AppState.terminalVisible = true;

    // 清空并初始化终端内容
    DOM.terminalContent.innerHTML = '';
    displayTerminalPrompt();

    // 添加终端高度调整功能
    initTerminalResize();
}

/**
 * 关闭终端
 *
 * 隐藏终端视图，清理资源
 */
function closeTerminal() {
    if (!AppState.terminalVisible) {
        return;
    }

    // 停止轮询
    stopPollingProcess();

    // 终止活动进程
    if (AppState.terminalCurrentPid) {
        BackendAPI.killProcess(AppState.terminalCurrentPid);
        AppState.terminalCurrentPid = null;
    }

    // 隐藏终端视图
    DOM.terminalView.classList.remove('visible');
    DOM.terminalView.style.height = ''; // 清除内联样式
    DOM.btnTerminal.classList.remove('active');
    AppState.terminalVisible = false;

    // 清空终端内容
    DOM.terminalContent.innerHTML = '';
}

/**
 * 切换终端显示状态
 *
 * 如果终端可见则关闭，否则打开
 */
async function toggleTerminal() {
    if (AppState.terminalVisible) {
        closeTerminal();
    } else {
        await openTerminal();
    }
}

/**
 * 更新终端提示符
 *
 * 更新现有提示符的内容（用户名、主机名、路径等）
 */
function updateTerminalPrompt() {
    const inputLine = document.querySelector('.terminal-input-line');
    if (!inputLine) {
        displayTerminalPrompt();
        return;
    }

    // 更新root状态
    if (AppState.terminalIsRoot) {
        inputLine.classList.add('root');
    } else {
        inputLine.classList.remove('root');
    }

    // 更新各个span的内容
    const userSpan = inputLine.querySelector('.user');
    const hostnameSpan = inputLine.querySelector('.hostname');
    const pathSpan = inputLine.querySelector('.path');
    const promptCharSpan = inputLine.querySelector('.prompt-char');

    if (userSpan) userSpan.textContent = AppState.terminalUser;
    if (hostnameSpan) hostnameSpan.textContent = AppState.terminalHostname;
    if (pathSpan) {
        // 将绝对路径转换为相对路径显示（如 /home/user -> ~/user）
        const homeDir = `/home/${AppState.terminalUser}`;
        let displayPath = AppState.terminalPath;
        if (displayPath.startsWith(homeDir)) {
            displayPath = '~' + displayPath.substring(homeDir.length);
        }
        pathSpan.textContent = displayPath;
    }
    if (promptCharSpan) promptCharSpan.textContent = AppState.terminalIsRoot ? '#' : '$';
}

/**
 * 显示新的终端提示符
 *
 * 总是创建新的提示符行（不检查是否已存在）
 */
function displayNewPrompt() {
    const promptDiv = document.createElement('div');
    promptDiv.className = `terminal-input-line ${AppState.terminalIsRoot ? 'root' : ''}`;

    const userSpan = document.createElement('span');
    userSpan.className = 'user';
    userSpan.textContent = AppState.terminalUser;

    const atSpan = document.createElement('span');
    atSpan.className = 'at';
    atSpan.textContent = '@';

    const hostnameSpan = document.createElement('span');
    hostnameSpan.className = 'hostname';
    hostnameSpan.textContent = AppState.terminalHostname;

    const colonSpan = document.createElement('span');
    colonSpan.className = 'colon';
    colonSpan.textContent = ':';

    const pathSpan = document.createElement('span');
    pathSpan.className = 'path';
    // 将绝对路径转换为相对路径显示（如 /home/user -> ~/user）
    const homeDir = `/home/${AppState.terminalUser}`;
    let displayPath = AppState.terminalPath;
    if (displayPath.startsWith(homeDir)) {
        displayPath = '~' + displayPath.substring(homeDir.length);
    }
    pathSpan.textContent = displayPath;

    const promptCharSpan = document.createElement('span');
    promptCharSpan.className = 'prompt-char';
    promptCharSpan.textContent = AppState.terminalIsRoot ? '#' : '$';

    // 添加空格
    const spaceSpan = document.createElement('span');
    spaceSpan.textContent = ' ';

    promptDiv.appendChild(userSpan);
    promptDiv.appendChild(atSpan);
    promptDiv.appendChild(hostnameSpan);
    promptDiv.appendChild(colonSpan);
    promptDiv.appendChild(pathSpan);
    promptDiv.appendChild(promptCharSpan);
    promptDiv.appendChild(spaceSpan);

    // 添加输入框
    const input = document.createElement('input');
    input.type = 'text';
    input.id = 'terminal-input';
    input.autocomplete = 'off';
    input.spellcheck = 'false';
    input.className = 'terminal-input';

    promptDiv.appendChild(input);

    // 将整个提示符+输入框添加到终端内容区域
    DOM.terminalContent.appendChild(promptDiv);

    // 获取输入框并添加事件监听
    if (input) {
        input.addEventListener('keydown', handleTerminalInput);
        input.focus();
    }

    // 滚动到底部
    DOM.terminalContent.scrollTop = DOM.terminalContent.scrollHeight;
}

/**
 * 显示终端提示符
 *
 * 在终端中显示用户名@主机名:路径$ 提示符
 */
function displayTerminalPrompt() {
    // 检查是否已存在提示符
    const existingInputLine = document.querySelector('.terminal-input-line');
    if (existingInputLine) {
        updateTerminalPrompt();
        return;
    }

    displayNewPrompt();
}

/**
 * 处理终端输入
 *
 * 处理用户在终端中输入的命令
 * @param {KeyboardEvent} event 键盘事件
 */
async function handleTerminalInput(event) {
    const input = event.target;

    // Ctrl+C - 发送中断信号
    if (event.ctrlKey && event.key === 'c') {
        event.preventDefault();
        
        // 如果有活动的进程，终止进程
        if (AppState.terminalCurrentPid) {
            await BackendAPI.killProcess(AppState.terminalCurrentPid);
            stopPollingProcess();
            AppState.terminalCurrentPid = null;
        }
        
        // 显示 ^C 符号
        const currentInputLine = input.closest('.terminal-input-line');
        if (currentInputLine) {
            // 移除输入框
            const inputField = currentInputLine.querySelector('input');
            if (inputField) {
                inputField.remove();
            }
            
            // 添加 ^C 符号
            const interruptSpan = document.createElement('span');
            interruptSpan.className = 'terminal-command';
            interruptSpan.style.color = '#f44336';
            interruptSpan.textContent = '^C';
            currentInputLine.appendChild(interruptSpan);
            
            // 添加换行
            currentInputLine.classList.add('terminal-output-line');
        }
        
        // 创建新的提示符行
        displayNewPrompt();
        
        // 清空输入框
        input.value = '';
        return;
    }

    const command = input.value;

    if (event.key === 'Enter') {
        // 如果有活动的进程，发送输入到进程
        if (AppState.terminalCurrentPid) {
            const inputContent = command + '\n';
            const result = await BackendAPI.sendProcessInput(AppState.terminalCurrentPid, inputContent);
            
            if (result.success) {
                // 清空输入框
                input.value = '';
            } else {
                // 显示错误
                const errorLine = document.createElement('div');
                errorLine.className = 'terminal-output-line';
                errorLine.style.color = '#f44336';
                errorLine.textContent = 'Failed to send input to process';
                DOM.terminalContent.appendChild(errorLine);
            }
            return;
        }

        // 没有活动的进程，执行新命令
        if (command) {
            // 处理clear命令
            if (command === 'clear') {
                // 清空终端内容
                DOM.terminalContent.innerHTML = '';
                // 重新显示提示符
                displayTerminalPrompt();
                return;
            }

            // 添加到命令历史
            AppState.terminalCommandHistory.push(command);
            AppState.terminalHistoryIndex = AppState.terminalCommandHistory.length;

            // 找到当前的输入行
            const currentInputLine = input.closest('.terminal-input-line');

            // 将当前的输入行转换为输出行（保留提示符和命令）
            if (currentInputLine) {
                // 移除输入框
                const inputField = currentInputLine.querySelector('input');
                if (inputField) {
                    inputField.remove();
                }
                
                // 添加命令文本到提示符后面
                const commandSpan = document.createElement('span');
                commandSpan.className = 'terminal-command';
                commandSpan.textContent = command;
                currentInputLine.appendChild(commandSpan);
                
                // 添加换行
                currentInputLine.classList.add('terminal-output-line');
            }

            // 执行命令
            const result = await BackendAPI.executeCommand(command, AppState.terminalPath);

            // 更新路径（如果cd命令改变了路径）
            if (command.startsWith('cd ') && result.success && result.newPath) {
                AppState.terminalPath = result.newPath;
            }

            // 更新是否为root用户
            if (result.isRoot !== undefined) {
                AppState.terminalIsRoot = result.isRoot;
            }

            // 显示输出（去除首尾空白）
            if (result.output && result.output.trim()) {
                const outputLine = document.createElement('div');
                outputLine.className = 'terminal-output-line';
                outputLine.textContent = result.output;
                DOM.terminalContent.appendChild(outputLine);
            }

            // 如果命令需要刷新文件树，使用智能刷新
            if (result.success && result.should_refresh) {
                // 使用后端返回的should_refresh标志
                await smartRefresh(command, AppState.terminalPath);
            }

            // 显示错误
            if (result.error && !result.success) {
                const errorLine = document.createElement('div');
                errorLine.className = 'terminal-output-line';
                
                // 如果是超时错误，使用黄色显示
                if (result.timedOut) {
                    errorLine.style.color = '#f39c12';
                } else {
                    errorLine.style.color = '#f44336';
                }
                
                errorLine.textContent = result.error;
                DOM.terminalContent.appendChild(errorLine);
            }

            // 如果是交互式进程，启动轮询
            if (result.interactive && result.pid) {
                AppState.terminalCurrentPid = result.pid;
                startPollingProcess(result.pid);
                
                // 滚动到底部
                DOM.terminalContent.scrollTop = DOM.terminalContent.scrollHeight;
            } else {
                // 重新获取终端信息并创建新的提示符
                const terminalInfo = await BackendAPI.getTerminalInfo(AppState.terminalPath);
                if (terminalInfo.success) {
                    AppState.terminalUser = terminalInfo.user;
                    AppState.terminalHostname = terminalInfo.hostname;
                    AppState.terminalPath = terminalInfo.path;
                    AppState.terminalIsRoot = terminalInfo.isRoot;
                }

                // 创建新的提示符行
                displayNewPrompt();
            }
        } else {
            // 空命令，只显示换行
            const newLine = document.createElement('div');
            newLine.className = 'terminal-output-line';
            newLine.textContent = '';
            DOM.terminalContent.appendChild(newLine);
            
            // 创建新的提示符行
            displayNewPrompt();
        }

        // 清空输入框
        input.value = '';
    } else if (event.key === 'ArrowUp') {
        // 上箭头：显示上一条命令
        event.preventDefault();
        if (AppState.terminalHistoryIndex > 0) {
            AppState.terminalHistoryIndex--;
            input.value = AppState.terminalCommandHistory[AppState.terminalHistoryIndex] || '';
        }
    } else if (event.key === 'ArrowDown') {
        // 下箭头：显示下一条命令
        event.preventDefault();
        if (AppState.terminalHistoryIndex < AppState.terminalCommandHistory.length - 1) {
            AppState.terminalHistoryIndex++;
            input.value = AppState.terminalCommandHistory[AppState.terminalHistoryIndex] || '';
        } else {
            AppState.terminalHistoryIndex = AppState.terminalCommandHistory.length;
            input.value = '';
        }
    } else if (event.key === 'Tab') {
        // Tab键：阻止默认行为（避免焦点移动）
        event.preventDefault();
    } else if (event.key === 'c' && event.ctrlKey) {
        // Ctrl+C：清空当前输入
        input.value = '';
    }
}

/**
 * 初始化终端高度调整功能
 *
 * 为终端视图添加拖动调整高度的功能
 * 使用requestAnimationFrame实现60FPS流畅动画
 */
function initTerminalResize() {
    const resizeHandle = DOM.terminalResizeHandle;
    if (!resizeHandle) {
        return;
    }

    let isResizing = false;
    let startY = 0;
    let startHeight = 0;
    let animationFrameId = null;

    resizeHandle.addEventListener('mousedown', (e) => {
        isResizing = true;
        startY = e.clientY;
        startHeight = DOM.terminalView.clientHeight;
        document.addEventListener('mousemove', onMouseMove);
        document.addEventListener('mouseup', onMouseUp);
        e.preventDefault();
    });

    function onMouseMove(e) {
        if (!isResizing) return;

        // 取消之前的动画帧请求
        if (animationFrameId) {
            cancelAnimationFrame(animationFrameId);
        }

        // 使用requestAnimationFrame实现60FPS
        animationFrameId = requestAnimationFrame(() => {
            const deltaY = startY - e.clientY;
            const minHeight = 63;  // 最小高度为3行（3 * 21px）
            const maxHeight = 504; // 最大高度为24行（24 * 21px）

            let newHeight = startHeight + deltaY;
            newHeight = Math.max(minHeight, Math.min(newHeight, maxHeight));

            DOM.terminalView.style.height = `${newHeight}px`;
            AppState.terminalHeight = newHeight;

            animationFrameId = null;
        });
    }

    function onMouseUp() {
        isResizing = false;
        if (animationFrameId) {
            cancelAnimationFrame(animationFrameId);
            animationFrameId = null;
        }
        document.removeEventListener('mousemove', onMouseMove);
        document.removeEventListener('mouseup', onMouseUp);
    }

    // 点击终端视图时聚焦到输入框
    DOM.terminalView.addEventListener('click', (e) => {
        const input = document.getElementById('terminal-input');
        if (input && e.target !== input && !input.contains(e.target)) {
            input.focus();
        }
    });
}

/**
 * 开始轮询交互式进程的输出
 *
 * @param {number} pid 进程ID
 */
async function startPollingProcess(pid) {
    // 清除旧的轮询定时器
    if (AppState.terminalPollInterval) {
        clearInterval(AppState.terminalPollInterval);
        AppState.terminalPollInterval = null;
    }

    // 每隔100ms轮询一次
    AppState.terminalPollInterval = setInterval(async () => {
        const result = await BackendAPI.getProcessOutput(pid);
        
        if (result.success && result.output && result.output.trim()) {
            // 显示输出
            const outputLine = document.createElement('div');
            outputLine.className = 'terminal-output-line';
            outputLine.textContent = result.output;
            DOM.terminalContent.appendChild(outputLine);
            
            // 滚动到底部
            DOM.terminalContent.scrollTop = DOM.terminalContent.scrollHeight;
        }

        // 如果进程已结束，停止轮询
        if (!result.is_running) {
            stopPollingProcess();
            
            // 不显示进程结束信息，直接创建新的提示符行
            // const endLine = document.createElement('div');
            // endLine.className = 'terminal-output-line';
            // endLine.style.color = '#888';
            // endLine.textContent = `Process ${pid} exited`;
            // DOM.terminalContent.appendChild(endLine);
            
            // 清空当前进程ID
            AppState.terminalCurrentPid = null;
            
            // 创建新的提示符行
            displayNewPrompt();
        }
    }, 100);
}

/**
 * 停止轮询交互式进程的输出
 */
function stopPollingProcess() {
    if (AppState.terminalPollInterval) {
        clearInterval(AppState.terminalPollInterval);
        AppState.terminalPollInterval = null;
    }
}
