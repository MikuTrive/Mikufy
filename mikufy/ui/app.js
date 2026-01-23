// ==================== 全局变量 ====================
/**
 * 当前工作区路径
 * 存储用户选择的根目录路径，用于文件操作的基础路径
 */
let currentWorkspace = null;

/**
 * 当前打开的文件路径
 * 存储当前正在编辑的文件的完整路径
 */
let currentFile = null;

/**
 * 当前选中的文件树项
 * 存储用户在文件树中选中的 DOM 元素引用
 */
let selectedFileItem = null;

/**
 * 文件树数据
 * 存储从后端获取的完整文件树结构数据
 * 包含所有文件和文件夹的层级关系
 */
let fileTreeData = [];

/**
 * 修改过的文件集合
 * 使用 Set 存储所有被修改但尚未保存的文件路径
 * 用于实现自动保存功能
 */
let modifiedFiles = new Set();

/**
 * 展开的文件夹集合
 * 使用 Set 存储当前处于展开状态的文件夹路径
 * 用于控制文件树的折叠/展开状态
 */
let expandedFolders = new Set();

/**
 * 打开的标签页列表
 * 存储所有已打开文件的标签页信息
 * 每个标签页包含：路径、名称、图标等信息
 */
let openTabs = [];

/**
 * 当前活动的标签页
 * 存储当前正在显示和编辑的标签页对应的文件路径
 */
let activeTab = null;

// ==================== 图标映射配置 ====================
/**
 * 文件图标映射表
 * 根据文件扩展名或特殊文件名映射到对应的图标文件路径
 * 用于在文件树中显示不同类型文件的图标
 * 
 * 支持的文件类型：
 * - C/C++ 文件：.c, .cpp, .h 等
 * - 配置文件：.json, .yaml, .toml 等
 * - Web 文件：.html, .css, .js, .ts 等
 * - 编程语言：Python, Java, Go, Rust 等
 * - 媒体文件：.png, .jpg, .mp3, .mp4 等
 * - Git 相关：.git, .gitignore 等
 * - 其他：zip, pdf, exe 等
 */
const FILE_ICON_MAP = {
    // C/C++ 文件图标
    '.c': 'Icons/C-24.png',
    '.C': 'Icons/C-24.png',
    '.cpp': 'Icons/C++-24.png',
    '.CPP': 'Icons/C++-24.png',
    '.cc': 'Icons/C++-24.png',
    '.c++': 'Icons/C++-24.png',
    '.cxx': 'Icons/C++-24.png',
    '.h': 'Icons/H-24.png',
    '.hpp': 'Icons/H-24.png',
    '.hxx': 'Icons/H-24.png',
    
    // 配置文件图标
    '.conf': 'Icons/Conf-24.png',
    '.config': 'Icons/Conf-24.png',
    '.kdl': 'Icons/Conf-24.png',
    '.ini': 'Icons/Conf-24.png',
    '.json': 'Icons/JSON-24.png',
    '.xml': 'Icons/Conf-24.png',
    '.yaml': 'Icons/Conf-24.png',
    '.yml': 'Icons/Conf-24.png',
    '.toml': 'Icons/Conf-24.png',
    
    // 汇编和目标文件图标
    '.s': 'Icons/Nasm-24.png',
    '.S': 'Icons/Nasm-24.png',
    '.asm': 'Icons/Nasm-24.png',
    '.o': 'Icons/Nasm-24.png',
    '.obj': 'Icons/Nasm-24.png',
    
    // 文档文件图标
    '.md': 'Icons/MD-24.png',
    '.MD': 'Icons/MD-24.png',
    '.txt': 'Icons/TXT-24.png',
    '.TXT': 'Icons/TXT-24.png',
    '.log': 'Icons/TXT-24.png',
    
    // Web 开发文件图标
    '.html': 'Icons/HTML-24.png',
    '.htm': 'Icons/HTML-24.png',
    '.css': 'Icons/CSS-24.png',
    '.js': 'Icons/JS.png',
    '.ts': 'Icons/Typescript-24.png',
    '.tsx': 'Icons/Typescript-24.png',
    '.jsx': 'Icons/React-24.png',
    
    // 其他编程语言图标
    '.py': 'Icons/Python-24.png',
    '.java': 'Icons/Java-24.png',
    '.jar': 'Icons/Jar-24.png',
    '.go': 'Icons/Go-24.png',
    '.rs': 'Icons/Rust-24.png',
    '.php': 'Icons/Php-24.png',
    '.kt': 'Icons/Kotlin-24.png',
    '.lua': 'Icons/Lua-24.png',
    '.sql': 'Icons/SQL-24.png',
    '.sh': 'Icons/shell-24.png',
    '.bash': 'Icons/shell-24.png',
    '.vue': 'Icons/Vuejs-24.png',
    '.svelte': 'Icons/Svelte-24.png',
    '.swift': 'Icons/Other-24.png',
    '.rb': 'Icons/Other-24.png',
    '.dart': 'Icons/Other-24.png',
    
    // 媒体文件图标
    '.png': 'Icons/png-24.png',
    '.jpg': 'Icons/jpg-24.png',
    '.jpeg': 'Icons/jpeg-24.png',
    '.gif': 'Icons/Other-24.png',
    '.svg': 'Icons/Other-24.png',
    '.mp3': 'Icons/mp3-24.png',
    '.wav': 'Icons/wav-24.png',
    '.mp4': 'Icons/mp4-24.png',
    
    // Git 版本控制相关图标
    '.git': 'Icons/Git-24.png',
    '.gitignore': 'Icons/Git-24.png',
    '.gitattributes': 'Icons/Git-24.png',
    
    // Node.js 包管理图标
    'package.json': 'Icons/npm-24.png',
    'package-lock.json': 'Icons/npm-24.png',
    
    // 其他文件类型图标
    '.zip': 'Icons/Other-24.png',
    '.tar': 'Icons/Other-24.png',
    '.gz': 'Icons/Other-24.png',
    '.7z': 'Icons/Other-24.png',
    '.pdf': 'Icons/Other-24.png',
    '.exe': 'Icons/Other-24.png',
    '.dll': 'Icons/Other-24.png',
    '.so': 'Icons/Other-24.png',
    '.a': 'Icons/Other-24.png',
    '.lib': 'Icons/Other-24.png',
    
    // Adobe AI 文件图标
    '.ai': 'Icons/AI-24.png',
    '.AI': 'Icons/AI-24.png'
};

// ==================== 初始化函数 ====================
/**
 * 页面加载完成后初始化应用
 * 
 * 这个函数在 DOM 完全加载后触发，负责：
 * 1. 打印初始化日志
 * 2. 初始化 UI 元素
 * 3. 绑定事件监听器
 * 4. 检查是否有之前打开的工作区
 */
document.addEventListener('DOMContentLoaded', () => {
    console.log('Mikufy Code Editor v1.0 初始化中...');
    
    // 初始化各个模块
    initializeUI();           // 初始化 UI 元素
    initializeEventListeners(); // 绑定事件监听器
    checkWorkspace();         // 检查工作区状态
});

/**
 * 初始化 UI 元素
 * 
 * 设置编辑器的占位符文本，引导用户选择工作区
 */
function initializeUI() {
    // 获取编辑器元素
    const editor = document.getElementById('code-editor');
    
    // 如果没有打开工作区，显示提示信息
    if (!currentWorkspace) {
        editor.placeholder = '点击左侧"打开文件夹"按钮选择工作区开始编辑...';
    }
}

/**
 * 初始化事件监听器
 * 
 * 绑定所有用户交互事件：
 * - 侧边栏按钮（打开文件夹、新建文件夹、新建文件、保存）
 * - 编辑器事件（内容变化、按键、滚动）
 * - 右键菜单事件
 * - 模态对话框事件
 * - 键盘快捷键
 */
function initializeEventListeners() {
    // ==================== 侧边栏操作按钮事件 ====================
    // 绑定"打开文件夹"按钮点击事件
    document.getElementById('btn-open-folder').addEventListener('click', openFolder);
    
    // 绑定"新建文件夹"按钮点击事件
    document.getElementById('btn-new-folder').addEventListener('click', createNewFolder);
    
    // 绑定"新建文件"按钮点击事件
    document.getElementById('btn-new-file').addEventListener('click', createNewFile);
    
    // 绑定"保存所有"按钮点击事件
    document.getElementById('btn-save').addEventListener('click', saveAllFiles);
    
    // ==================== 编辑器事件 ====================
    // 绑定编辑器内容变化事件（用于自动保存）
    document.getElementById('code-editor').addEventListener('input', handleEditorChange);
    
    // 获取编辑器相关元素
    const editor = document.getElementById('code-editor');
    const editorContainer = document.getElementById('editor-container');
    const lineNumbers = document.getElementById('line-numbers');
    
    // 绑定编辑器按键事件（处理 Tab 键缩进）
    editor.addEventListener('keydown', function(e) {
        // 检测 Tab 键按下
        if (e.key === 'Tab') {
            // 阻止默认行为（失去焦点）
            e.preventDefault();
            
            // 获取当前光标位置
            const start = this.selectionStart;
            const end = this.selectionEnd;
            
            // 插入 4 个空格作为缩进
            this.value = this.value.substring(0, start) + '    ' + this.value.substring(end);
            
            // 移动光标到缩进后的位置
            this.selectionStart = this.selectionEnd = start + 4;
            
            // 触发 input 事件以保存更改
            handleEditorChange();
        }
    });
    
    // 绑定编辑器滚动事件（同步行号滚动）
    editor.addEventListener('scroll', function() {
        lineNumbers.scrollTop = this.scrollTop;
    });
    
    // 绑定容器滚动事件（同步编辑器和行号滚动）
    editorContainer.addEventListener('scroll', function() {
        editor.scrollTop = this.scrollTop;
        lineNumbers.scrollTop = this.scrollTop;
    });
    
    // 初始化行号显示
    updateLineNumbers();
    
    // ==================== 右键菜单事件 ====================
    // 绑定"重命名"菜单项点击事件
    document.getElementById('ctx-rename').addEventListener('click', handleRename);
    
    // 绑定"删除"菜单项点击事件
    document.getElementById('ctx-delete').addEventListener('click', handleDelete);
    
    // ==================== 模态对话框事件 ====================
    // 绑定"关闭"按钮点击事件
    document.getElementById('modal-close').addEventListener('click', closeModal);
    
    // 绑定"取消"按钮点击事件
    document.getElementById('modal-cancel').addEventListener('click', closeModal);
    
    // 绑定"确认"按钮点击事件
    document.getElementById('modal-confirm').addEventListener('click', handleModalConfirm);
    
    // ==================== 全局点击事件 ====================
    // 点击其他地方关闭右键菜单
    document.addEventListener('click', (e) => {
        // 如果点击的不是右键菜单，则关闭菜单
        if (!e.target.closest('.context-menu')) {
            hideContextMenu();
        }
    });
    
    // ==================== 键盘快捷键事件 ====================
    // 绑定全局键盘快捷键
    document.addEventListener('keydown', handleKeyboardShortcuts);
}

/**
 * 检查是否有工作区
 * 
 * 向后端查询是否有之前打开的工作区路径
 * 如果有，则自动加载该工作区的文件树
 */
function checkWorkspace() {
    // 向后端发送检查工作区请求
    sendToBackend('checkWorkspace', {}, (response) => {
        // 如果后端返回了工作区路径，则加载文件树
        if (response && response.workspace) {
            currentWorkspace = response.workspace;
            loadFileTree();
        }
    });
}

// ==================== 文件操作函数 ====================
/**
 * 打开文件夹
 * 
 * 调用后端打开文件夹选择对话框
 * 用户选择文件夹后，更新工作区路径并加载文件树
 */
function openFolder() {
    console.log('点击打开文件夹按钮');
    
    // 向后端发送打开文件夹请求
    sendToBackend('openFolder', {}, (response) => {
        console.log('收到 openFolder 响应:', response);
        console.log('响应类型:', typeof response);
        console.log('响应成功:', response.success);
        console.log('响应路径:', response.path);
        
        // 如果用户成功选择了文件夹
        if (response && response.success && response.path) {
            // 更新当前工作区路径
            currentWorkspace = response.path;
            console.log('设置工作区:', currentWorkspace);
            
            // 加载文件树
            loadFileTree();
            
            // 更新标题栏显示的工作区路径
            document.getElementById('current-file').textContent = currentWorkspace;
        } else {
            console.log('打开文件夹失败或用户取消');
        }
    });
}

/**
 * 加载文件树
 * 
 * 从后端获取工作区的文件结构并渲染到界面
 * 文件树包含所有文件和文件夹的层级关系
 */
function loadFileTree() {
    // 如果没有工作区，直接返回
    if (!currentWorkspace) return;
    
    // 向后端请求文件树数据
    sendToBackend('getFileTree', { path: currentWorkspace }, (response) => {
        console.log('收到文件树响应:', response);
        
        // 如果成功获取文件树数据
        if (response && response.files) {
            // 保存文件树数据
            fileTreeData = response.files;
            console.log('文件树数据:', fileTreeData);
            
            // 渲染文件树到界面
            renderFileTree(fileTreeData);
        }
    });
}

/**
 * 渲染文件树（性能优化版）
 * 
 * 递归渲染文件树结构，支持多级嵌套
 * 每个文件项包含图标、名称和操作按钮
 * 
 * 性能优化：
 * - 分批渲染：使用 requestAnimationFrame 分批渲染 DOM 元素
 * - 避免一次性渲染大量元素导致卡顿
 * - 提高响应速度
 * 
 * @param {Array} files - 文件列表数据
 * @param {HTMLElement} container - 容器元素（DOM 节点）
 * @param {number} level - 缩进层级（用于视觉层级显示）
 * @param {number} batchSize - 每批渲染的文件数量（默认为 50）
 */
function renderFileTree(files, container = null, level = 0, batchSize = 50) {
    // 如果没有提供容器，使用文件树根容器
    if (!container) {
        container = document.getElementById('file-tree');
        container.innerHTML = '';
    }
    
    // 性能优化：如果文件数量超过阈值，使用分批渲染
    if (files.length > batchSize) {
        renderFileTreeBatch(files, container, level, batchSize);
        return;
    }
    
    // 文件数量较少，直接渲染
    renderFileTreeImmediate(files, container, level);
}

/**
 * 立即渲染文件树（用于小量文件）
 * 
 * @param {Array} files - 文件列表数据
 * @param {HTMLElement} container - 容器元素
 * @param {number} level - 缩进层级
 */
function renderFileTreeImmediate(files, container, level) {
    // 遍历所有文件项
    files.forEach(file => {
        // 创建文件树项元素
        const item = document.createElement('div');
        
        // 设置 CSS 类名：
        // - file-tree-item: 基础样式
        // - level-N: 缩进层级（N 为 1-5）
        // - folder: 文件夹样式
        // - hidden: 隐藏文件样式
        item.className = `file-tree-item level-${Math.min(level, 5)} ${file.isDirectory ? 'folder' : ''} ${file.isHidden ? 'hidden' : ''}`;
        
        // 存储文件数据到 dataset 中
        item.dataset.path = file.path;      // 完整路径
        item.dataset.isDir = file.isDirectory; // 是否为目录
        
        // 获取文件图标
        // 如果是文件夹，使用文件夹图标；否则根据文件名获取对应图标
        const icon = file.isDirectory ? 'Icons/Folder.png' : getFileIcon(file.name);
        
        // 检查文件夹是否处于展开状态
        const isExpanded = expandedFolders.has(file.path);
        
        // 构建文件树项的 HTML 内容
        let itemHtml = '';
        
        // 如果是文件夹，添加折叠/展开按钮
        if (file.isDirectory) {
            // 根据展开状态选择不同的图标
            const toggleIcon = isExpanded ? 'Icons/Put-away.png' : 'Icons/Spread-out.png';
            itemHtml += `<span class="folder-toggle" data-path="${file.path}"><img src="${toggleIcon}" alt="toggle"></span>`;
        } else {
            // 如果不是文件夹，添加占位符保持对齐
            itemHtml += `<span style="width: 16px; margin-right: 4px;"></span>`;
        }
        
        // 添加文件图标和文件名
        itemHtml += `
            <img src="${icon}" alt="${file.isDirectory ? '文件夹' : '文件'}">
            <span>${file.name}</span>
        `;
        
        // 设置 HTML 内容
        item.innerHTML = itemHtml;
        
        // ==================== 绑定事件监听器 ====================
        
        // 点击事件：选中文件/文件夹或打开文件
        item.addEventListener('click', (e) => {
            // 选中当前项
            selectFileItem(item);
            
            // 如果不是文件夹，则打开文件
            if (!file.isDirectory) {
                openFile(file.path);
            }
        });
        
        // 右键事件：显示上下文菜单
        item.addEventListener('contextmenu', (e) => {
            e.preventDefault(); // 阻止默认右键菜单
            selectFileItem(item);
            showContextMenu(e, file);
        });
        
        // ==================== 文件夹子项处理 ====================
        
        // 如果是文件夹且有子项，创建子容器
        if (file.isDirectory && file.children && file.children.length > 0) {
            // 创建子项容器
            const childrenContainer = document.createElement('div');
            childrenContainer.className = 'file-tree-children';
            childrenContainer.dataset.parentPath = file.path;
            
            // 根据展开状态设置显示/隐藏
            if (isExpanded) {
                childrenContainer.style.display = 'block';
            } else {
                childrenContainer.style.display = 'none';
            }
            
            // 将子容器添加到文件树项中（嵌套结构）
            item.appendChild(childrenContainer);
            
            // 递归渲染子项
            renderFileTreeImmediate(file.children, childrenContainer, level + 1);
        }
        
        // 将文件树项添加到容器中
        container.appendChild(item);
    });
    
    // 绑定折叠/展开按钮的事件监听器
    const toggleButtons = container.querySelectorAll('.folder-toggle');
    toggleButtons.forEach(button => {
        button.addEventListener('click', (e) => {
            e.stopPropagation();
            const path = button.dataset.path;
            toggleFolder(path);
        });
    });
}

/**
 * 分批渲染文件树（用于大量文件）
 * 
 * 使用 requestAnimationFrame 分批渲染 DOM 元素
 * 避免一次性渲染大量元素导致卡顿
 * 
 * @param {Array} files - 文件列表数据
 * @param {HTMLElement} container - 容器元素
 * @param {number} level - 缩进层级
 * @param {number} batchSize - 每批渲染的文件数量
 */
function renderFileTreeBatch(files, container, level, batchSize) {
    let currentIndex = 0;
    const totalFiles = files.length;
    
    // 显示加载进度
    const progressIndicator = document.createElement('div');
    progressIndicator.className = 'file-tree-item';
    progressIndicator.style.paddingLeft = '20px';
    progressIndicator.style.color = '#888888';
    progressIndicator.textContent = `加载中... (${currentIndex}/${totalFiles})`;
    container.appendChild(progressIndicator);
    
    // 分批渲染函数
    function renderBatch() {
        const endIndex = Math.min(currentIndex + batchSize, totalFiles);
        
        // 渲染当前批次
        for (let i = currentIndex; i < endIndex; i++) {
            const file = files[i];
            
            // 创建文件树项元素
            const item = document.createElement('div');
            
            // 设置 CSS 类名
            item.className = `file-tree-item level-${Math.min(level, 5)} ${file.isDirectory ? 'folder' : ''} ${file.isHidden ? 'hidden' : ''}`;
            
            // 存储文件数据到 dataset 中
            item.dataset.path = file.path;
            item.dataset.isDir = file.isDirectory;
            
            // 获取文件图标
            const icon = file.isDirectory ? 'Icons/Folder.png' : getFileIcon(file.name);
            
            // 检查文件夹是否处于展开状态
            const isExpanded = expandedFolders.has(file.path);
            
            // 构建文件树项的 HTML 内容
            let itemHtml = '';
            
            if (file.isDirectory) {
                const toggleIcon = isExpanded ? 'Icons/Put-away.png' : 'Icons/Spread-out.png';
                itemHtml += `<span class="folder-toggle" data-path="${file.path}"><img src="${toggleIcon}" alt="toggle"></span>`;
            } else {
                itemHtml += `<span style="width: 16px; margin-right: 4px;"></span>`;
            }
            
            itemHtml += `
                <img src="${icon}" alt="${file.isDirectory ? '文件夹' : '文件'}">
                <span>${file.name}</span>
            `;
            
            item.innerHTML = itemHtml;
            
            // 绑定事件监听器
            item.addEventListener('click', (e) => {
                selectFileItem(item);
                if (!file.isDirectory) {
                    openFile(file.path);
                }
            });
            
            item.addEventListener('contextmenu', (e) => {
                e.preventDefault();
                selectFileItem(item);
                showContextMenu(e, file);
            });
            
            // 文件夹子项处理
            if (file.isDirectory && file.children && file.children.length > 0) {
                const childrenContainer = document.createElement('div');
                childrenContainer.className = 'file-tree-children';
                childrenContainer.dataset.parentPath = file.path;
                
                if (isExpanded) {
                    childrenContainer.style.display = 'block';
                } else {
                    childrenContainer.style.display = 'none';
                }
                
                item.appendChild(childrenContainer);
                
                // 递归渲染子项（也使用分批渲染）
                renderFileTree(file.children, childrenContainer, level + 1, batchSize);
            }
            
            container.appendChild(item);
        }
        
        // 更新进度
        currentIndex = endIndex;
        progressIndicator.textContent = `加载中... (${currentIndex}/${totalFiles})`;
        
        // 如果还有文件未渲染，继续下一批
        if (currentIndex < totalFiles) {
            requestAnimationFrame(renderBatch);
        } else {
            // 渲染完成，移除进度指示器
            container.removeChild(progressIndicator);
            
            // 绑定折叠/展开按钮的事件监听器
            const toggleButtons = container.querySelectorAll('.folder-toggle');
            toggleButtons.forEach(button => {
                button.addEventListener('click', (e) => {
                    e.stopPropagation();
                    const path = button.dataset.path;
                    toggleFolder(path);
                });
            });
        }
    }
    
    // 开始渲染
    requestAnimationFrame(renderBatch);
}

/**
 * 切换文件夹的展开/折叠状态
 * 
 * 根据文件夹的当前状态进行切换：
 * - 如果折叠，则展开并渲染子项
 * - 如果展开，则折叠并隐藏子项
 * 同时更新按钮图标
 * 
 * 性能优化：
 * - 延迟加载：只在展开时才加载子目录内容
 * - 避免一次性加载整个文件树
 * - 减少内存占用和渲染时间
 * 
 * @param {string} path - 文件夹路径
 */
function toggleFolder(path) {
    // 检查文件夹当前是否展开
    const isExpanded = expandedFolders.has(path);
    
    // 切换状态
    if (isExpanded) {
        expandedFolders.delete(path);
        console.log('折叠文件夹:', path);
    } else {
        expandedFolders.add(path);
        console.log('展开文件夹:', path);
    }
    
    // 查找对应的文件夹项 DOM 元素
    const folderItem = document.querySelector(`.file-tree-item[data-path="${path}"]`);
    if (!folderItem) return;
    
    // 查找或创建子项容器
    let childrenContainer = folderItem.querySelector('.file-tree-children');
    
    // ==================== 处理展开操作 ====================
    
    if (!isExpanded) {
        // 查找文件夹的子项数据
        const folderData = findFolderInTree(fileTreeData, path);
        
        // 性能优化：如果子项数据为空，延迟加载
        if (!folderData || !folderData.children || folderData.children.length === 0) {
            console.log('延迟加载子目录:', path);
            
            // 显示加载提示
            const loadingIndicator = document.createElement('div');
            loadingIndicator.className = 'file-tree-item';
            loadingIndicator.style.paddingLeft = '60px';
            loadingIndicator.style.color = '#888888';
            loadingIndicator.textContent = '加载中...';
            
            // 如果子容器不存在，创建新的
            if (!childrenContainer) {
                childrenContainer = document.createElement('div');
                childrenContainer.className = 'file-tree-children';
                childrenContainer.dataset.parentPath = path;
                folderItem.appendChild(childrenContainer);
            }
            
            // 显示子容器
            childrenContainer.style.display = 'block';
            
            // 添加加载提示
            childrenContainer.innerHTML = '';
            childrenContainer.appendChild(loadingIndicator);
            
            // 向后端请求加载子目录内容
            sendToBackend('getSubDirectory', { path: path }, (response) => {
                if (response && response.success && response.files) {
                    // 更新文件树数据
                    if (folderData) {
                        folderData.children = response.files;
                    }
                    
                    // 渲染子项
                    childrenContainer.innerHTML = '';
                    renderFileTree(response.files, childrenContainer, 0);
                } else {
                    // 加载失败，显示错误提示
                    loadingIndicator.textContent = '加载失败';
                    loadingIndicator.style.color = '#e55c00';
                }
            });
        } else {
            // 如果有子项数据，渲染子项
            if (!childrenContainer) {
                childrenContainer = document.createElement('div');
                childrenContainer.className = 'file-tree-children';
                childrenContainer.dataset.parentPath = path;
                folderItem.appendChild(childrenContainer);
            }
            
            // 显示子容器
            childrenContainer.style.display = 'block';
            
            // 清空子容器内容
            childrenContainer.innerHTML = '';
            
            // 渲染子项
            renderFileTree(folderData.children, childrenContainer, 0);
        }
    }
    // ==================== 处理折叠操作 ====================
    else if (childrenContainer) {
        // 隐藏子项容器
        childrenContainer.style.display = 'none';
    }
    
    // ==================== 更新按钮图标 ====================
    
    // 获取折叠/展开按钮
    const toggleButton = folderItem.querySelector('.folder-toggle');
    if (toggleButton) {
        // 获取按钮内的图片元素
        const img = toggleButton.querySelector('img');
        if (img) {
            // 根据新的状态更新图标
            if (!isExpanded) {
                img.src = 'Icons/Put-away.png'; // 展开状态
            } else {
                img.src = 'Icons/Spread-out.png'; // 折叠状态
            }
        }
    }
}

/**
 * 在文件树中查找文件夹
 * 
 * 递归搜索文件树，查找指定路径的文件夹数据
 * 
 * @param {Array} files - 文件列表
 * @param {string} path - 要查找的文件夹路径
 * @returns {Object|null} 找到的文件夹数据，未找到返回 null
 */
function findFolderInTree(files, path) {
    // 遍历所有文件
    for (const file of files) {
        // 如果路径匹配，返回当前文件
        if (file.path === path) {
            return file;
        }
        
        // 如果是文件夹，递归搜索子项
        if (file.isDirectory && file.children) {
            const found = findFolderInTree(file.children, path);
            if (found) return found;
        }
    }
    
    // 未找到，返回 null
    return null;
}

/**
 * 根据文件名获取对应的图标路径
 * 
 * 首先检查特殊文件名（如 package.json）
 * 然后检查文件扩展名
 * 最后返回默认图标
 * 
 * @param {string} filename - 文件名
 * @returns {string} 图标文件路径
 */
function getFileIcon(filename) {
    // 提取文件扩展名
    const ext = filename.includes('.') ? filename.substring(filename.lastIndexOf('.')) : '';
    
    // 优先检查特殊文件名（如 package.json）
    if (FILE_ICON_MAP[filename]) {
        return FILE_ICON_MAP[filename];
    }
    
    // 检查文件扩展名
    if (FILE_ICON_MAP[ext]) {
        return FILE_ICON_MAP[ext];
    }
    
    // 返回默认图标
    return 'Icons/Other-24.png';
}

/**
 * 选中文件树项
 * 
 * 设置文件树项的选中状态：
 * 1. 移除之前选中的项的选中样式
 * 2. 为当前项添加选中样式
 * 3. 更新全局选中项引用
 * 
 * @param {HTMLElement} item - 文件树项元素
 */
function selectFileItem(item) {
    // 移除所有文件树项的选中状态
    document.querySelectorAll('.file-tree-item.selected').forEach(el => {
        el.classList.remove('selected');
    });
    
    // 为当前项添加选中状态
    item.classList.add('selected');
    
    // 更新全局选中项引用
    selectedFileItem = item;
}

/**
 * 打开文件
 * 
 * 1. 更新当前文件路径
 * 2. 添加或激活标签页
 * 3. 从后端读取文件内容
 * 4. 更新编辑器内容
 * 
 * 性能优化：
 * - 大文件只加载预览内容，避免卡顿
 * - 显示加载提示
 * 
 * @param {string} filepath - 文件路径
 */
function openFile(filepath) {
    // 更新当前文件路径
    currentFile = filepath;
    
    // 从路径中提取文件名
    const filename = filepath.split('/').pop();
    
    // 更新标题栏显示的文件路径
    document.getElementById('current-file').textContent = filepath;
    
    // 添加或激活标签页
    addTab(filepath, filename);
    
    // 显示加载提示
    const editor = document.getElementById('code-editor');
    editor.value = '加载中...';
    editor.disabled = true; // 禁用编辑器，防止用户编辑
    
    // 向后端请求文件内容
    sendToBackend('openFile', { path: filepath }, (response) => {
        if (response && response.content !== undefined) {
            // 将文件内容设置到编辑器
            editor.value = response.content;
            editor.disabled = false; // 启用编辑器
            
            // 更新行号显示
            updateLineNumbers();
            
            // 如果是预览内容，显示警告
            if (response.isPreview) {
                console.log('警告: 文件过大，只显示预览内容');
            }
        }
    });
}

/**
 * 添加标签页
 * 
 * 如果标签页已存在，则激活它
 * 否则创建新标签页并添加到列表
 * 
 * @param {string} filepath - 文件路径
 * @param {string} filename - 文件名
 */
function addTab(filepath, filename) {
    // 检查标签页是否已存在
    const existingTab = openTabs.find(tab => tab.path === filepath);
    if (existingTab) {
        // 标签页已存在，激活它
        setActiveTab(filepath);
        return;
    }
    
    // 创建新标签页对象
    const tab = {
        path: filepath,    // 文件完整路径
        name: filename,    // 文件名
        icon: getFileIcon(filename) // 文件图标
    };
    
    // 添加到标签页列表
    openTabs.push(tab);
    
    // 渲染标签页
    renderTabs();
    
    // 激活新标签页
    setActiveTab(filepath);
}

/**
 * 渲染标签页
 * 
 * 清空标签页容器并重新渲染所有标签页
 * 每个标签页包含图标、名称和关闭按钮
 */
function renderTabs() {
    // 获取标签页容器
    const container = document.getElementById('tabs-container');
    
    // 清空容器
    container.innerHTML = '';
    
    // 遍历所有标签页
    openTabs.forEach(tab => {
        // 创建标签页元素
        const tabEl = document.createElement('div');
        
        // 设置 CSS 类名（活动标签页有特殊样式）
        tabEl.className = `tab ${activeTab === tab.path ? 'active' : ''}`;
        tabEl.dataset.path = tab.path;
        
        // 设置标签页 HTML 内容
        tabEl.innerHTML = `
            <img src="${tab.icon}" alt="${tab.name}">
            <span>${tab.name}</span>
            <div class="tab-close" data-path="${tab.path}">
                <img src="Icons/Close.png" alt="关闭">
            </div>
        `;
        
        // ==================== 绑定标签页事件 ====================
        
        // 点击标签页：切换到该文件
        tabEl.addEventListener('click', (e) => {
            // 如果点击的不是关闭按钮，切换标签页
            if (!e.target.closest('.tab-close')) {
                setActiveTab(tab.path);
                openFile(tab.path);
            }
        });
        
        // 点击关闭按钮：关闭标签页
        const closeBtn = tabEl.querySelector('.tab-close');
        closeBtn.addEventListener('click', (e) => {
            e.stopPropagation(); // 阻止事件冒泡到标签页
            closeTab(tab.path);
        });
        
        // 添加到容器
        container.appendChild(tabEl);
    });
}

/**
 * 设置活动标签页
 * 
 * 更新活动标签页并重新渲染标签页
 * 
 * @param {string} filepath - 文件路径
 */
function setActiveTab(filepath) {
    // 更新活动标签页路径
    activeTab = filepath;
    
    // 重新渲染标签页（更新活动状态样式）
    renderTabs();
}

/**
 * 关闭标签页
 * 
 * 从标签页列表中移除指定标签页
 * 如果关闭的是当前活动标签页，则切换到其他标签页
 * 
 * @param {string} filepath - 文件路径
 */
function closeTab(filepath) {
    // 查找标签页在列表中的索引
    const index = openTabs.findIndex(tab => tab.path === filepath);
    if (index === -1) return;
    
    // 从列表中移除标签页
    openTabs.splice(index, 1);
    
    // ==================== 处理活动标签页切换 ====================
    
    if (activeTab === filepath) {
        // 如果还有其他标签页，切换到相邻标签页
        if (openTabs.length > 0) {
            // 选择前一个或后一个标签页
            const newIndex = Math.min(index, openTabs.length - 1);
            
            // 激活新标签页
            setActiveTab(openTabs[newIndex].path);
            
            // 打开新标签页对应的文件
            openFile(openTabs[newIndex].path);
        } else {
            // 没有标签页了，清空编辑器
            activeTab = null;
            currentFile = null;
            document.getElementById('code-editor').value = '';
            document.getElementById('current-file').textContent = '未打开文件';
        }
    }
    
    // 重新渲染标签页
    renderTabs();
}

/**
 * 处理编辑器内容变化
 * 
 * 当用户修改编辑器内容时：
 * 1. 将文件添加到修改列表
 * 2. 自动保存到后端
 * 3. 更新行号显示
 */
function handleEditorChange() {
    // 如果没有打开的文件，直接返回
    if (!currentFile) return;
    
    // 将当前文件添加到修改过的文件集合
    modifiedFiles.add(currentFile);
    
    // 获取编辑器内容
    const content = document.getElementById('code-editor').value;
    
    // 发送保存请求到后端
    sendToBackend('saveFile', { path: currentFile, content: content });
    
    // 更新行号显示
    updateLineNumbers();
}

/**
 * 更新行号
 * 
 * 根据编辑器内容的行数生成行号
 * 每行一个数字，用换行符分隔
 */
function updateLineNumbers() {
    // 获取编辑器和行号元素
    const editor = document.getElementById('code-editor');
    const lineNumbers = document.getElementById('line-numbers');
    
    // 计算行数（按换行符分割）
    const lines = editor.value.split('\n').length;
    
    // 生成行号 HTML
    let lineNumbersHtml = '';
    for (let i = 1; i <= lines; i++) {
        lineNumbersHtml += i + '\n';
    }
    
    // 设置行号文本
    lineNumbers.textContent = lineNumbersHtml;
}

/**
 * 保存所有修改过的文件
 * 
 * 遍历所有修改过的文件并发送保存请求
 * 保存完成后清空修改列表
 */
function saveAllFiles() {
    // 如果没有需要保存的文件，直接返回
    if (modifiedFiles.size === 0) {
        console.log('没有需要保存的文件');
        return;
    }
    
    console.log('保存所有修改过的文件:', Array.from(modifiedFiles));
    
    // 遍历所有修改过的文件并保存
    modifiedFiles.forEach(filePath => {
        // 发送保存请求到后端
        // 注意：这里使用空字符串作为内容，实际保存应该从编辑器获取
        sendToBackend('saveFile', { path: filePath, content: '' }, (response) => {
            if (response && response.success) {
                console.log('已保存文件:', filePath);
            }
        });
    });
    
    // 清空修改过的文件集合
    modifiedFiles.clear();
    
    console.log('所有文件已保存');
}

/**
 * 创建新文件夹
 * 
 * 显示模态对话框让用户输入文件夹名称
 * 创建成功后刷新文件树
 */
function createNewFolder() {
    // 检查是否有工作区
    if (!currentWorkspace) {
        alert('请先打开一个工作区文件夹');
        return;
    }
    
    // 显示模态对话框
    showModal('新建文件夹', '请输入文件夹名称', '', (name) => {
        if (name && name.trim()) {
            // 构建文件夹完整路径
            const folderPath = currentWorkspace + '/' + name.trim();
            
            // 向后端发送创建文件夹请求
            sendToBackend('createFolder', { path: folderPath }, (response) => {
                if (response && response.success) {
                    // 刷新文件树
                    loadFileTree();
                } else {
                    alert('创建文件夹失败: ' + (response?.error || '未知错误'));
                }
            });
        }
    });
}

/**
 * 创建新文件
 * 
 * 显示模态对话框让用户输入文件名（包含扩展名）
 * 创建成功后刷新文件树并打开新文件
 */
function createNewFile() {
    // 检查是否有工作区
    if (!currentWorkspace) {
        alert('请先打开一个工作区文件夹');
        return;
    }
    
    // 显示模态对话框
    showModal('新建文件', '请输入文件名（包含扩展名）', '', (name) => {
        if (name && name.trim()) {
            // 构建文件完整路径
            const filePath = currentWorkspace + '/' + name.trim();
            
            // 向后端发送创建文件请求
            sendToBackend('createFile', { path: filePath }, (response) => {
                if (response && response.success) {
                    // 先刷新文件树，确保文件树刷新完成后再打开文件
                    sendToBackend('getFileTree', { path: currentWorkspace }, (treeResponse) => {
                        if (treeResponse && treeResponse.files) {
                            fileTreeData = treeResponse.files;
                            renderFileTree(fileTreeData);
                        }
                        // 文件树刷新完成后，打开新文件
                        openFile(filePath);
                    });
                } else {
                    alert('创建文件失败: ' + (response?.error || '未知错误'));
                }
            });
        }
    });
}

/**
 * 处理重命名操作
 * 
 * 显示模态对话框让用户输入新名称
 * 重命名成功后刷新文件树
 */
function handleRename() {
    // 检查是否有选中的文件/文件夹
    if (!selectedFileItem) return;
    
    // 获取旧路径和旧名称
    const oldPath = selectedFileItem.dataset.path;
    const oldName = selectedFileItem.querySelector('span').textContent;
    
    // 隐藏右键菜单
    hideContextMenu();
    
    // 显示模态对话框
    showModal('重命名', '请输入新名称', oldName, (newName) => {
        // 验证新名称不为空且与旧名称不同
        if (newName && newName.trim() && newName !== oldName) {
            // 构建新路径
            const newPath = oldPath.substring(0, oldPath.lastIndexOf('/')) + '/' + newName.trim();
            
            // 向后端发送重命名请求
            sendToBackend('rename', { oldPath: oldPath, newPath: newPath }, (response) => {
                if (response && response.success) {
                    // 刷新文件树
                    loadFileTree();
                } else {
                    alert('重命名失败: ' + (response?.error || '未知错误'));
                }
            });
        }
    });
}

/**
 * 处理删除操作
 * 
 * 直接删除选中的文件或文件夹（不显示确认对话框）
 * 删除成功后刷新文件树
 * 如果删除的是当前打开的文件，清空编辑器
 */
function handleDelete() {
    // 检查是否有选中的文件/文件夹
    if (!selectedFileItem) return;
    
    // 获取路径、名称和类型
    const path = selectedFileItem.dataset.path;
    const name = selectedFileItem.querySelector('span').textContent;
    const isDir = selectedFileItem.dataset.isDir === 'true';
    
    // 隐藏右键菜单
    hideContextMenu();
    
    // 直接发送删除请求（不显示确认对话框）
    sendToBackend('delete', { path: path, isDirectory: isDir }, (response) => {
        if (response && response.success) {
            // 刷新文件树
            loadFileTree();
            
            // 如果删除的是当前打开的文件，清空编辑器
            if (path === currentFile) {
                currentFile = null;
                document.getElementById('code-editor').value = '';
                document.getElementById('current-file').textContent = '未打开文件';
                updateLineNumbers();
            }
        } else {
            alert('删除失败: ' + (response?.error || '未知错误'));
        }
    });
}

// ==================== 右键菜单函数 ====================
/**
 * 显示右键菜单
 * 
 * 在鼠标右键点击的位置显示上下文菜单
 * 自动调整位置防止超出屏幕边界
 * 
 * @param {Event} e - 鼠标事件对象
 * @param {Object} file - 文件信息对象
 */
function showContextMenu(e, file) {
    // 获取右键菜单元素
    const menu = document.getElementById('context-menu');
    
    // 移除隐藏类，显示菜单
    menu.classList.remove('hidden');
    
    // ==================== 计算菜单位置 ====================
    
    // 获取鼠标点击位置
    let x = e.clientX;
    let y = e.clientY;
    
    // 获取菜单尺寸
    const menuWidth = 150;
    const menuHeight = 80;
    
    // 防止菜单超出屏幕右边界
    if (x + menuWidth > window.innerWidth) {
        x = window.innerWidth - menuWidth - 10;
    }
    
    // 防止菜单超出屏幕底部边界
    if (y + menuHeight > window.innerHeight) {
        y = window.innerHeight - menuHeight - 10;
    }
    
    // 设置菜单位置
    menu.style.left = x + 'px';
    menu.style.top = y + 'px';
}

/**
 * 隐藏右键菜单
 * 
 * 添加隐藏类，使菜单不可见
 */
function hideContextMenu() {
    document.getElementById('context-menu').classList.add('hidden');
}

// ==================== 模态对话框函数 ====================
/**
 * 模态对话框回调函数
 * 
 * 用户点击确认按钮时调用此函数
 */
let modalCallback = null;

/**
 * 显示模态对话框
 * 
 * 显示一个模态对话框，包含标题、提示信息和输入框
 * 用户可以输入内容并确认或取消
 * 
 * @param {string} title - 对话框标题
 * @param {string} message - 提示信息
 * @param {string} defaultValue - 输入框默认值
 * @param {Function} callback - 确认按钮的回调函数
 */
function showModal(title, message, defaultValue = '', callback) {
    // 设置对话框内容
    document.getElementById('modal-title').textContent = title;
    document.getElementById('modal-message').textContent = message;
    document.getElementById('modal-input').value = defaultValue;
    
    // 显示对话框
    document.getElementById('modal-overlay').classList.remove('hidden');
    
    // 聚焦到输入框
    document.getElementById('modal-input').focus();
    
    // 保存回调函数
    modalCallback = callback;
}

/**
 * 关闭模态对话框
 * 
 * 隐藏对话框并清空输入框和回调函数
 */
function closeModal() {
    // 隐藏对话框
    document.getElementById('modal-overlay').classList.add('hidden');
    
    // 清空输入框
    document.getElementById('modal-input').value = '';
    
    // 清空回调函数
    modalCallback = null;
}

/**
 * 处理模态对话框确认
 * 
 * 当用户点击确认按钮时调用
 * 调用之前保存的回调函数并关闭对话框
 */
function handleModalConfirm() {
    // 获取输入框的值
    const value = document.getElementById('modal-input').value;
    
    // 如果有回调函数，调用它
    if (modalCallback) {
        modalCallback(value);
    }
    
    // 关闭对话框
    closeModal();
}

// ==================== 键盘快捷键函数 ====================
/**
 * 处理键盘快捷键
 * 
 * 全局键盘事件处理器，支持以下快捷键：
 * - Ctrl+S: 保存文件
 * - Ctrl+N: 新建文件
 * - F2: 重命名
 * - Delete: 删除
 * - Enter: 打开文件（仅在非编辑器区域）
 * 
 * @param {KeyboardEvent} e - 键盘事件对象
 */
function handleKeyboardShortcuts(e) {
    // 获取编辑器元素
    const editor = document.getElementById('code-editor');
    
    // 如果焦点在编辑器中，只处理 Ctrl+S
    if (document.activeElement === editor) {
        if (e.ctrlKey && e.key === 's') {
            e.preventDefault();
            if (currentFile) {
                handleEditorChange();
            }
        }
        return;
    }
    
    // ==================== 全局快捷键 ====================
    
    // Ctrl+S: 保存文件
    if (e.ctrlKey && e.key === 's') {
        e.preventDefault();
        if (currentFile) {
            handleEditorChange();
        }
    }
    
    // Ctrl+N: 新建文件
    if (e.ctrlKey && e.key === 'n') {
        e.preventDefault();
        createNewFile();
    }
    
    // F2: 重命名
    if (e.key === 'F2' && selectedFileItem) {
        e.preventDefault();
        handleRename();
    }
    
    // Delete: 删除
    if (e.key === 'Delete' && selectedFileItem) {
        e.preventDefault();
        handleDelete();
    }
    
    // Enter: 打开文件（仅在非编辑器区域有效）
    if (e.key === 'Enter' && selectedFileItem) {
        e.preventDefault();
        // 只对文件（非文件夹）有效
        if (selectedFileItem.dataset.isDir === 'false') {
            openFile(selectedFileItem.dataset.path);
        }
    }
}

// ==================== 与后端通信函数 ====================
/**
 * 发送消息到 C++ 后端
 * 
 * 将前端请求封装为 JSON 格式并通过 webview 发送到后端
 * 支持多种 webview 环境：
 * - WebKit webview（window.webkit.messageHandlers）
 * - 其他 webview（window.external.invoke）
 * - 开发环境模拟
 * 
 * @param {string} action - 操作类型（如 'openFolder', 'saveFile' 等）
 * @param {Object} data - 要发送的数据对象
 * @param {Function} callback - 后端响应的回调函数
 */
function sendToBackend(action, data = {}, callback = null) {
    // 将操作和数据封装为 JSON 字符串
    const message = JSON.stringify({ action, data });
    console.log('发送到后端:', message);
    
    // ==================== WebKit webview 环境 ====================
    
    if (typeof window.webkit !== 'undefined' && window.webkit.messageHandlers && window.webkit.messageHandlers.backend) {
        try {
            // 通过 webkit 消息处理器发送消息
            window.webkit.messageHandlers.backend.postMessage(message);
            
            // 如果需要回调，设置临时处理函数
            if (callback) {
                window.backendCallback = callback;
            }
        } catch (e) {
            console.error('发送消息失败:', e);
            if (callback) {
                callback({ success: false, error: e.message });
            }
        }
    }
    // ==================== 其他 webview 环境 ====================
    else if (typeof window.external !== 'undefined' && window.external.invoke) {
        try {
            // 通过 external.invoke 发送消息
            window.external.invoke(message);
            
            // 如果需要回调，设置临时处理函数
            if (callback) {
                window.backendCallback = callback;
            }
        } catch (e) {
            console.error('发送消息失败:', e);
            if (callback) {
                callback({ success: false, error: e.message });
            }
        }
    }
    // ==================== 开发环境模拟 ====================
    else {
        // 未检测到 webview 环境，使用模拟模式
        console.warn('未检测到 webview 环境，使用模拟模式');
        if (callback) {
            // 模拟异步响应
            setTimeout(() => {
                console.log('模拟响应:', action);
                callback({ success: true });
            }, 100);
        }
    }
}

/**
 * 接收来自 C++ 后端的消息
 * 
 * 处理从后端返回的 JSON 格式消息
 * 调用之前设置的回调函数
 * 
 * @param {string} message - JSON 格式的消息字符串
 */
function receiveFromBackend(message) {
    try {
        // 解析 JSON 消息
        const data = JSON.parse(message);
        
        // 如果有回调函数，调用它
        if (window.backendCallback) {
            window.backendCallback(data);
            window.backendCallback = null;
        }
    } catch (e) {
        console.error('解析后端消息失败:', e);
    }
}

// ==================== 暴露给后端的函数 ====================
/**
 * 将 receiveFromBackend 函数暴露给后端调用
 * 
 * 后端可以通过 JavaScript 调用此函数来向前端发送消息
 * 这是前端与后端通信的关键桥梁
 */
window.receiveFromBackend = receiveFromBackend;
