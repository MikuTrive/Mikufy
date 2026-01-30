/**
 * Mikufy v2.5 - 窗口管理器头文件
 * 负责GTK窗口和WebKitWebView的创建与管理
 */

#ifndef MIKUFY_WINDOW_MANAGER_H
#define MIKUFY_WINDOW_MANAGER_H

#include "main.h"
#include "web_server.h"

/**
 * 窗口管理器类
 * 管理GTK窗口和WebKit WebView
 */
class WindowManager {
public:
    WindowManager(WebServer* webServer);
    ~WindowManager();
    
    // 禁止拷贝和移动
    WindowManager(const WindowManager&) = delete;
    WindowManager& operator=(const WindowManager&) = delete;
    WindowManager(WindowManager&&) = delete;
    WindowManager& operator=(WindowManager&&) = delete;
    
    /**
     * 初始化窗口
     * @return 是否成功
     */
    bool init();
    
    /**
     * 显示窗口
     */
    void show();
    
    /**
     * 运行GTK主循环
     */
    void run();
    
    /**
     * 关闭窗口
     */
    void close();
    
    /**
     * 获取当前工作目录（通过文件对话框选择）
     * @return 工作目录路径，取消返回空字符串
     */
    std::string getCurrentWorkingDirectory();

    /**
     * 加载前端页面
     */
    void loadFrontendPage();

    /**
     * 异步打开文件夹对话框
     * @return 工作目录路径
     */
    std::string openFolderDialogAsync();

private:
    WebServer* webServer;
    GtkWidget* window;
    WebKitWebView* webView;
    std::string currentWorkingDirectory;
    std::mutex mutex;
    std::string dialogResult;
    bool dialogWaiting;
    GCond dialogCond;
    GCond* dialogReady;
    GMutex dialogMutex;
    
    /**
     * 创建GTK窗口
     */
    bool createWindow();
    
    /**
     * 创建WebKit WebView
     */
    bool createWebView();
    
    /**
     * 处理窗口关闭事件
     */
    static gboolean onWindowClose(GtkWidget* widget, GdkEvent* event, gpointer user_data);
    
    /**
     * 处理窗口销毁事件
     */
    static void onWindowDestroy(GtkWidget* widget, gpointer user_data);
    
    /**
     * 处理WebView加载完成事件
     */
    static void onWebViewLoadChanged(WebKitWebView* webView, WebKitLoadEvent loadEvent, gpointer user_data);
};

#endif // MIKUFY_WINDOW_MANAGER_H