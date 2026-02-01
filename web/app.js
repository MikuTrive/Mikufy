/**
 * Mikufy v2.2(stable) - 代码编辑器前端JavaScript
 * 处理所有前端交互逻辑，通过C++后端API与系统交互
 */

// 全局状态管理
const AppState = {
    // 当前打开的工作目录
    currentDirectory: null,
    // 当前显示的路径（可能包含子目录）
    currentPath: null,
    // 当前目录的路径历史，用于返回上级
    pathHistory: [],
    // 所有打开的标签页
    openTabs: [],
    // 当前激活的标签页索引
    activeTab: -1,
    // 文件内容缓存（用于未保存的内容）
    contentCache: new Map(),
    // 文件树数据
    fileTree: [],
    // 右键菜单选中的项
    contextSelected: null,
    // 新建对话框类型
    newDialogType: null, // 'folder' or 'file'
    // 加载状态标志
    isLoading: false,
    // 图标映射表
    iconMap: {
        '.ai': 'AI-24.png',
        '.conf': 'Conf-24.png',
        '.config': 'Conf-24.png',
        '.theme': 'Conf-24.png',
        '.d': 'Conf-24.png',
        '.cpp': 'C++-24.png',
        '.c++': 'C++-24.png',
        '.CPP': 'C++-24.png',
        '.C++': 'C++-24.png',
        '.c': 'C-24.png',
        '.C': 'C-24.png',
        '.git': 'Git-24.png',
        '.go': 'Go-24.png',
        '.h': 'H-24.png',
        '.H': 'H-24.png',
        '.hpp': 'H-24.png',
        '.HPP': 'H-24.png',
        '.jar': 'Jar-24.png',
        '.java': 'Java-24.png',
        '.JAVA': 'Java-24.png',
        '.js': 'JS-24.png',
        '.jsx': 'JS-24.png',
        '.lua': 'Lua-24.png',
        '.LUA': 'Lua-24.png',
        '.md': 'MD-24.png',
        '.MD': 'MD-24.png',
        '.asm': 'Nasm-24.png',
        '.s': 'Nasm-24.png',
        '.o': 'Nasm-24.png',
        '.txt': 'TXT-24.png',
        '.TXT': 'TXT-24.png',
        '.jpeg': 'jpeg-24.png',
        '.jpg': 'jpg-24.png',
        '.png': 'png-24.png',
        '.mp3': 'mp3-24.png',
        '.mp4': 'mp4-24.png',
        '.wav': 'wav-24.png',
        '.php': 'Php-24.png',
        '.PHP': 'Php-24.png',
        '.python': 'Python-24.png',
        '.py': 'Python-24.png',
        '.PY': 'Python-24.png',
        '.typescript': 'Typescript-24.png',
        '.ts': 'Typescript-24.png',
        '.TS': 'Typescript-24.png',
        '.vue': 'Vuejs-24.png',
        '.svelte': 'Svelte-24.png',
        '.html': 'HTML-24.png',
        '.css': 'CSS-24.png',
        '.json': 'JSON-24.png',
        '.xml': 'Other-24.png',
        '.yaml': 'Other-24.png',
        '.yml': 'Other-24.png',
        '.rust': 'Rust-24.png',
        '.rs': 'Rust-24.png',
        '.kotlin': 'Kotlin-24.png',
        '.kt': 'Kotlin-24.png',
        '.npm': 'npm-24.png',
        '.gitignore': 'Git-24.png',
        '.angularjs': 'angularjs-24.png',
        '.gitattributes': 'Git-24.png'
    }
};

// C++后端API调用函数
const BackendAPI = {
    /**
     * 打开文件夹对话框
     * @returns {Promise<string|null>} 选中的文件夹路径或null
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
     * @param {string} path 目录路径
     * @returns {Promise<Array>} 文件列表
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
     * @param {string} path 文件路径
     * @returns {Promise<string>} 文件内容
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
     * @param {string} path 文件路径
     * @param {string} content 文件内容
     * @returns {Promise<boolean>} 是否成功
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
     * @param {string} parentPath 父目录路径
     * @param {string} name 文件夹名称
     * @returns {Promise<boolean>} 是否成功
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
     * @param {string} parentPath 父目录路径
     * @param {string} name 文件名称
     * @returns {Promise<boolean>} 是否成功
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
     * @param {string} path 路径
     * @returns {Promise<boolean>} 是否成功
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
     * @param {string} oldPath 原路径
     * @param {string} newPath 新路径
     * @returns {Promise<boolean>} 是否成功
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
     * @param {string} path 文件路径
     * @returns {Promise<Object>} 文件类型信息
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
     * @returns {Promise<boolean>} 是否成功
     */
    async saveAll() {
        try {
            const response = await fetch('/api/save-all', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({
                    files: Array.from(AppState.contentCache.entries())
                })
            });
            const data = await response.json();
            if (data.success) {
                AppState.contentCache.clear();
            }
            return data.success || false;
        } catch (error) {
            console.error('保存所有文件失败:', error);
            return false;
        }
    },

    /**
     * 刷新文件树
     * @returns {Promise<boolean>} 是否成功
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

// DOM元素引用
const DOM = {
    btnOpenFolder: null,
    btnNewFolder: null,
    btnNewFile: null,
    currentPath: null,
    btnBack: null,
    fileTree: null,
    tabsScroll: null,
    tabsScrollbar: null,
    tabsScrollbarThumb: null,
    lineNumbers: null,
    codeEditor: null,
    verticalScrollbar: null,
    verticalScrollbarThumb: null,
    contextMenu: null,
    contextRename: null,
    contextDelete: null,
    newDialog: null,
    newDialogTitle: null,
    newDialogInput: null,
    newDialogConfirm: null,
    newDialogCancel: null,
    editorContainer: null
};

/**
 * 获取文件图标
 * @param {string} filename 文件名
 * @param {boolean} isDirectory 是否为目录
 * @returns {string} 图标文件名
 */
function getFileIcon(filename, isDirectory) {
    if (isDirectory) {
        return 'Folder.png';
    }
    
    const ext = '.' + filename.split('.').pop().toLowerCase();
    return AppState.iconMap[ext] || 'Other-24.png';
}

/**
 * 初始化DOM元素引用
 */
function initDOM() {
    DOM.btnOpenFolder = document.getElementById('btn-open-folder');
    DOM.btnNewFolder = document.getElementById('btn-new-folder');
    DOM.btnNewFile = document.getElementById('btn-new-file');
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
}

/**
 * 渲染文件树
 * @param {Array} files 文件列表
 */
function renderFileTree(files) {
    console.log('开始渲染文件树，文件数量:', files.length);
    DOM.fileTree.innerHTML = '';

    // 排序：目录在前，文件在后，同类型按名称排序
    const sortedFiles = files.sort((a, b) => {
        if (a.isDirectory !== b.isDirectory) {
            return a.isDirectory ? -1 : 1;
        }
        return a.name.localeCompare(b.name);
    });
    
    sortedFiles.forEach(file => {
        const item = document.createElement('div');
        item.className = 'file-tree-item' + (file.isDirectory ? ' directory' : '');
        item.dataset.path = file.path;
        item.dataset.name = file.name;
        item.dataset.isDirectory = file.isDirectory;
        
        // 展开按钮（仅目录）
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
        
        // 文件图标
        const icon = document.createElement('img');
        icon.className = 'file-tree-icon';
        icon.src = `../Icons/${getFileIcon(file.name, file.isDirectory)}`;
        icon.alt = file.name;
        item.appendChild(icon);
        
        // 文件名
        const name = document.createElement('span');
        name.className = 'file-tree-name';
        name.textContent = file.name;
        item.appendChild(name);
        
        // 点击事件
        item.onclick = () => handleFileItemClick(file);

        // 右键菜单事件
        item.oncontextmenu = (e) => {
            e.preventDefault();
            showContextMenu(e, file);
        };

        DOM.fileTree.appendChild(item);
    });

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
    const mediaExtensions = ['.jpeg', '.jpg', '.png', '.mp3', '.mp4', '.wav'];
    
    // 如果是媒体文件，允许打开
    if (mediaExtensions.includes(ext)) {
        openFileTab(file.path, file.name);
        return;
    }
    
    // 对于其他文件，检查是否为二进制文件
    const fileInfo = await BackendAPI.getFileInfo(file.path);
    if (fileInfo.isBinary) {
        alert('无法打开二进制文件');
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
    
    // 检查文件类型并显示相应内容
    const ext = '.' + tab.name.split('.').pop().toLowerCase();
    const mediaExtensions = ['.jpeg', '.jpg', '.png', '.mp3', '.mp4', '.wav'];
    
    if (mediaExtensions.includes(ext)) {
        showMediaContent(tab.path, ext);
    } else {
        showCodeContent(tab.content);
    }
}

/**
 * 语法高亮（类似Visual Studio Code）
 * @param {string} code 代码内容
 * @returns {string} 高亮后的HTML
 */
function syntaxHighlight(code) {
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
    
    if (ext === '.jpeg' || ext === '.jpg' || ext === '.png') {
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
                    <audio controls autoplay style="width: 80%; margin-top: 20px;">
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
 * 更新行号
 * @param {string} content 代码内容
 */
function updateLineNumbers(content) {
    // 获取编辑器中所有子节点
    const childNodes = DOM.codeEditor.childNodes;
    let lines = 0;

    for (let i = 0; i < childNodes.length; i++) {
        const node = childNodes[i];

        if (node.nodeType === Node.TEXT_NODE) {
            // 文本节点，计算其中的换行符数量
            const textContent = node.textContent || '';
            const newLines = (textContent.match(/\n/g) || []).length;
            lines += newLines + 1;
        } else if (node.nodeType === Node.ELEMENT_NODE) {
            const tagName = node.tagName?.toLowerCase();
            // <br> 标签表示换行（一行）
            if (tagName === 'br') {
                lines++;
            }
            // <div> 或 <p> 等块级元素表示新的一行（即使是空的也算一行）
            else if (tagName === 'div' || tagName === 'p') {
                lines++;
            }
        }
    }

    // 如果没有找到任何行，至少显示1行
    if (lines === 0) {
        lines = 1;
    }

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
        
        DOM.tabsScroll.appendChild(tabItem);
    });
    
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
    if (index === AppState.activeTab && DOM.codeEditor.textContent !== tab.content) {
        AppState.contentCache.set(tab.path, DOM.codeEditor.textContent);
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
    DOM.newDialogInput.focus();
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
        alert('创建失败');
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
                        <li><kbd>Tab</kbd> - 自动缩进（4格）</li>
                    </ul>
                </div>
                <div class="about-features">
                    <h2>功能</h2>
                    <ul>
                        <li>支持多种编程语言语法高亮</li>
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
            alert('请先打开一个文件夹');
            return;
        }
        showNewDialog('folder');
    };
    
    // 新建文件按钮
    DOM.btnNewFile.onclick = () => {
        if (!AppState.currentDirectory) {
            alert('请先打开一个文件夹');
            return;
        }
        showNewDialog('file');
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
                alert('重命名失败');
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
            alert('删除失败');
        }
    };
    
    // 新建对话框事件
    DOM.newDialogConfirm.onclick = handleNewConfirm;
    DOM.newDialogCancel.onclick = hideNewDialog;
    
    DOM.newDialogInput.onkeypress = (e) => {
        if (e.key === 'Enter') {
            handleNewConfirm();
        } else if (e.key === 'Escape') {
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
        
        // Tab - 自动缩进
        if (e.key === 'Tab' && document.activeElement === DOM.codeEditor) {
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
    };
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
 */
async function handleSaveAll() {
    // 保存当前标签页内容
    if (AppState.activeTab >= 0) {
        const tab = AppState.openTabs[AppState.activeTab];
        const currentContent = getEditorContent();
        if (currentContent !== tab.content) {
            tab.content = currentContent;
            AppState.contentCache.set(tab.path, currentContent);
        }
    }
    
    // 调用后端保存所有文件
    const success = await BackendAPI.saveAll();
    
    if (success) {
        console.log('所有文件已保存');
    } else {
        alert('保存失败');
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
    
    console.log('Mikufy v2.2(stable) 初始化完成');
}

// 页面加载完成后初始化
window.onload = init;

// 页面关闭前保存
window.onbeforeunload = async () => {
    await handleSaveAll();
};
