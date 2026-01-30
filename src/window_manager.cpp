/**
 * Mikufy v2.5 - 窗口管理器实现
 * 负责GTK窗口和WebKit WebView的创建与管理
 */

#include "../headers/window_manager.h"

// 构造函数
WindowManager::WindowManager(WebServer* webServer)
    : webServer(webServer), window(nullptr), webView(nullptr), dialogWaiting(false) {
    g_cond_init(&dialogCond);
}

// 析构函数
WindowManager::~WindowManager() {
    if (window) {
        gtk_widget_destroy(window);
    }
}

/**
 * 初始化窗口
 */
bool WindowManager::init() {
    // 初始化GTK
    if (!gtk_init_check(nullptr, nullptr)) {
        return false;
    }
    
    // 创建窗口
    if (!createWindow()) {
        return false;
    }
    
    // 创建WebView
    if (!createWebView()) {
        return false;
    }
    
    return true;
}

/**
 * 创建GTK窗口
 */
bool WindowManager::createWindow() {
    // 创建窗口
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    
    // 设置窗口标题
    gtk_window_set_title(GTK_WINDOW(window), WINDOW_TITLE);
    
    // 设置窗口大小
    gtk_window_set_default_size(GTK_WINDOW(window), WINDOW_WIDTH, WINDOW_HEIGHT);
    
    // 设置窗口位置居中
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    
    // 设置窗口可调整大小
    gtk_window_set_resizable(GTK_WINDOW(window), TRUE);
    
    // 连接关闭信号
    g_signal_connect(window, "delete-event", G_CALLBACK(onWindowClose), this);
    g_signal_connect(window, "destroy", G_CALLBACK(onWindowDestroy), this);
    
    return true;
}

/**
 * 创建WebKit WebView
 */
bool WindowManager::createWebView() {
    // 创建WebView
    webView = WEBKIT_WEB_VIEW(webkit_web_view_new());
    
    // 设置WebView属性
    WebKitSettings* settings = webkit_web_view_get_settings(webView);
    
    // 启用JavaScript
    webkit_settings_set_enable_javascript(settings, TRUE);
    
    // 禁用开发者工具
    webkit_settings_set_enable_developer_extras(settings, FALSE);
    
    // 设置不缓存
    webkit_settings_set_enable_write_console_messages_to_stdout(settings, TRUE);
    
    // 连接加载完成信号
    g_signal_connect(webView, "load-changed", G_CALLBACK(onWebViewLoadChanged), this);
    
    // 将WebView添加到窗口
    gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(webView));
    
    // 显示WebView
    gtk_widget_show_all(GTK_WIDGET(webView));
    
    return true;
}

/**
 * 加载前端页面
 */
void WindowManager::loadFrontendPage() {
    std::string url = "http://localhost:" + std::to_string(WEB_SERVER_PORT) + "/";
    webkit_web_view_load_uri(webView, url.c_str());
}

/**
 * 显示窗口
 */
void WindowManager::show() {
    if (window) {
        gtk_widget_show_all(window);
    }
}

/**
 * 运行GTK主循环
 */
void WindowManager::run() {
    if (window) {
        gtk_main();
    }
}

/**
 * 关闭窗口
 */
void WindowManager::close() {
    if (window && GTK_IS_WIDGET(window)) {
        gtk_widget_destroy(window);
        window = nullptr;
        webView = nullptr;
    }
}

/**
 * 获取当前工作目录（通过文件对话框选择）
 */
std::string WindowManager::getCurrentWorkingDirectory() {
    // 使用异步方式打开对话框，避免在HTTP请求处理中阻塞GTK主循环
    return openFolderDialogAsync();
}

/**
 * 异步打开文件夹对话框
 */
std::string WindowManager::openFolderDialogAsync() {
    // 检查窗口是否有效
    if (!window || !GTK_IS_WIDGET(window) || !gtk_widget_get_visible(window)) {
        return "";
    }

    dialogResult = "";
    dialogWaiting = true;

    // 获取主上下文
    GMainContext* context = g_main_context_default();

    // 使用g_main_context_invoke在主线程中执行对话框
    g_main_context_invoke(context, [](gpointer user_data) -> gboolean {
        WindowManager* manager = static_cast<WindowManager*>(user_data);

        // 创建文件选择对话框
        GtkWidget* dialog = gtk_file_chooser_dialog_new(
            "选择工作文件夹",
            GTK_WINDOW(manager->window),
            GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
            "取消", GTK_RESPONSE_CANCEL,
            "选择", GTK_RESPONSE_ACCEPT,
            nullptr
        );

        if (dialog && GTK_IS_WIDGET(dialog)) {
            // 设置对话框为模态
            gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);

            // 显示对话框
            gtk_widget_show_all(dialog);

            // 运行对话框
            gint response = gtk_dialog_run(GTK_DIALOG(dialog));

            if (response == GTK_RESPONSE_ACCEPT) {
                // 获取选择的文件夹路径
                char* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
                if (filename) {
                    manager->dialogResult = filename;
                    g_free(filename);
                }
            }

            // 销毁对话框
            if (GTK_IS_WIDGET(dialog)) {
                gtk_widget_destroy(dialog);
            }
        }

        // 发送完成信号
        manager->dialogWaiting = false;

        return G_SOURCE_REMOVE;
    }, this);

    // 等待对话框完成
    while (dialogWaiting) {
        // 等待100ms，避免CPU占用过高
        usleep(100000);
    }

    return dialogResult;
}

/**
 * 处理窗口关闭事件
 */
gboolean WindowManager::onWindowClose(GtkWidget* widget, GdkEvent* event, gpointer user_data) {
    (void)widget;
    (void)event;

    WindowManager* manager = static_cast<WindowManager*>(user_data);

    // 保存所有文件
    // 这里可以通过JavaScript调用前端的保存函数
    // 或者直接调用WebServer的API
    
    // 关闭窗口
    manager->close();
    
    return FALSE;
}

/**
 * 处理窗口销毁事件
 */
void WindowManager::onWindowDestroy(GtkWidget* widget, gpointer user_data) {
    (void)widget;

    WindowManager* manager = static_cast<WindowManager*>(user_data);
    manager->window = nullptr;
    manager->webView = nullptr;

    // 检查是否还有主循环在运行
    if (gtk_main_level() > 0) {
        gtk_main_quit();
    }
}

/**
 * 处理WebView加载完成事件
 */
void WindowManager::onWebViewLoadChanged(WebKitWebView* webView, WebKitLoadEvent loadEvent, gpointer user_data) {
    (void)webView;
    (void)user_data;

    if (loadEvent == WEBKIT_LOAD_FINISHED) {
        // 页面加载完成
        // 可以在这里执行一些初始化操作
    }
}