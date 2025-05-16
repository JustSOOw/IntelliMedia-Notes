/*
 * @Author: Furdow wang22338014@gmail.com
 * @Date: 2025-04-14 17:37:03
 * @LastEditors: Furdow wang22338014@gmail.com
 * @LastEditTime: 2025-05-13 17:31:07
 * @FilePath: \IntelliMedia_Notes\src\mainwindow.h
 * @Description: 
 * 
 * Copyright (c) 2025 by Furdow, All Rights Reserved. 
 */
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QToolButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QStatusBar>
#include <QSizeGrip>
#include <QResizeEvent>
#include <QGraphicsDropShadowEffect>
#include <QPixmap>
#include <QBitmap>
#include <QPoint> // 引入QPoint用于存储鼠标位置
#include <QColor> // 引入QColor
#include <QSvgRenderer> // 引入SVG渲染
#include <QPainter> // 引入QPainter
#include <QIcon> // 引入QIcon
#include <QtQuickWidgets/QQuickWidget> // 添加 QQuickWidget
#include <QTimer>
#include <QSystemTrayIcon> // 添加系统托盘图标
#include <QMenu> // 添加菜单

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class QToolButton; // 前向声明
class QMouseEvent; // 事件处理的前向声明

// 前向声明侧边栏管理器和搜索管理器
class SidebarManager;
class SearchManager;
class TextEditorManager; // 添加文本编辑器管理器前向声明
class AiAssistantDialog; // 添加AI助手对话框前向声明
class IAiService; // 添加AI服务接口前向声明
class SettingsDialog; // 添加设置对话框前向声明

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    enum ResizeRegion {
        None = 0,
        Top = 1,
        Bottom = 2,
        Left = 4,
        Right = 8,
        TopLeft = Top | Left,
        TopRight = Top | Right,
        BottomLeft = Bottom | Left,
        BottomRight = Bottom | Right
    };
    
    // 设置AI服务
    void setAiService(IAiService *service);
    
    // 获取AI服务
    IAiService* getAiService() const { return m_aiService; }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    
    // 重写鼠标事件处理用于窗口拖动
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    
    // 重写大小调整事件
    void resizeEvent(QResizeEvent *event) override;
    
    // 重写关闭事件，用于实现最小化到托盘而不是直接关闭
    void closeEvent(QCloseEvent *event) override;

private slots:
    void toggleMaximizeRestore(); // 最大化/还原按钮槽函数
    void toggleTheme();           // 主题切换按钮槽函数
    void openSettings();          // 设置按钮槽函数
    void toggleSidebar();         // 侧边栏切换按钮槽函数
    void handleNoteSelected(const QString &path, const QString &type); // 处理笔记选择
    void openSearchDialog();      // 打开搜索对话框
    void onSearchClosed();        // 处理搜索对话框关闭
    void showAiAssistant();       // 显示AI助手对话框
    void showAiAssistantWithText(const QString &selectedText); // 显示AI助手对话框并设置选中文本
    void saveCurrentNote();       // 保存当前笔记
    void handleContentModified(); // 处理编辑器内容修改
    void insertAiContent(const QString &content); // 插入AI生成的内容
    
    // 设置相关槽函数
    void onSettingsClosed();     // 处理设置对话框关闭事件
    void applySettings();        // 应用设置
    void applyThemeFromSettings(); // 从设置应用主题
    void applyTheme(const QString &theme); // 直接应用指定的主题
    void applyLanguage(const QString &language); // 应用语言设置
    void applyAutoSaveInterval(int interval); // 应用自动保存间隔设置
    void restartApplication(); // 重启应用程序
    void restoreLastSession(); // 恢复上次会话状态
    
    // 系统托盘相关槽函数
    void trayIconActivated(QSystemTrayIcon::ActivationReason reason); // 处理托盘图标激活
    void showHideWindow(); // 显示/隐藏主窗口
    void quitApplication(); // 退出应用程序

private:
    void showSettingsDialog();   // 新增：声明用于显示和管理设置对话框的函数
    Ui::MainWindow *ui;
    
    // 窗口控制按钮
    QToolButton *toggleSidebarButton;
    QToolButton *minimizeButton;
    QToolButton *maximizeButton;
    QToolButton *closeButton;
    QToolButton *themeToggleButton;   // 主题切换按钮
    QToolButton *settingsButton;      // 设置按钮
    
    // 用于窗口拖动
    bool m_isResizing = false;
    QPoint m_mouse_pos; // 相对于窗口左上角的位置
    QWidget* m_titleBarWidget = nullptr; // 标题栏区域引用，用于拖动（现在是ui->widget）
    bool m_dragging = false; // 新增：用于跟踪窗口是否正在被拖动
    
    // --- 窗口大小调整变量和枚举 ---
    bool m_resizing = false; 
    ResizeRegion m_resizeRegion = None;
    QPoint m_resizeStartPosGlobal; // 存储全局起始位置
    QRect m_resizeStartGeometry; // 存储原始窗口几何信息
    
    // 确定调整区域的辅助函数
    ResizeRegion getResizeRegion(const QPoint &pos);
    
    // 处理窗口大小调整的函数
    void handleResize(const QPoint &globalPos);
    
    // 根据鼠标位置更新调整大小时光标的函数
    void updateCursorForResize(const QPoint &pos);
    
    bool m_isDarkTheme; // 当前主题状态
    
    // 加载样式表的辅助函数
    void loadAndApplyStyleSheet(const QString &sheetName);
    
    // 根据主题更新按钮图标的辅助函数
    void updateButtonIcons();
    
    // 给SVG图标上色的辅助函数
    QIcon colorizeSvgIcon(const QString &path, const QColor &color);
    
    // 侧边栏相关
    QQuickWidget *m_sidebarWidget;   // 侧边栏QML容器
    SidebarManager *m_sidebarManager = nullptr; // 侧边栏管理器
    SearchManager *m_searchManager = nullptr;   // 搜索管理器
    
    // 文本编辑器相关
    TextEditorManager *m_textEditorManager = nullptr; // 文本编辑器管理器
    
    // AI相关
    AiAssistantDialog *m_aiAssistantDialog = nullptr; // AI助手对话框
    IAiService *m_aiService = nullptr;  // AI服务接口
    
    // 设置相关
    SettingsDialog *m_settingsDialog = nullptr; // 设置对话框
    QTimer *m_autoSaveTimer = nullptr;         // 自动保存定时器
    QTimer *m_backupCheckTimer = nullptr;      // 自动备份检查定时器
    
    // 系统托盘相关
    QSystemTrayIcon *m_trayIcon = nullptr; // 系统托盘图标
    QMenu *m_trayMenu = nullptr; // 系统托盘菜单
    
    // 初始化系统托盘
    bool setupTrayIcon();
    
    // 初始化侧边栏
    void setupSidebar();
    
    // 初始化搜索功能
    void setupSearch();
    
    // 初始化文本编辑器
    void setupTextEditor();
    
    // 初始化AI助手
    void setupAiAssistant();
    
    // 初始化设置
    void setupSettings();
    
    // 当前打开的笔记路径
    QString m_currentNotePath;
    
    // -----------------------------------------
};

#endif // MAINWINDOW_H
