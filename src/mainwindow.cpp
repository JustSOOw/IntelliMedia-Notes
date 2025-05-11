/*
 * @Author: Furdow wang22338014@gmail.com
 * @Date: 2025-04-14 17:37:03
 * @LastEditors: Furdow wang22338014@gmail.com
 * @LastEditTime: 2025-05-11 18:01:28
 * @FilePath: \IntelliMedia_Notes\src\mainwindow.cpp
 * @Description: 
 * 
 * Copyright (c) 2025 by Furdow, All Rights Reserved. 
 */
#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "sidebarmanager.h" // 包含侧边栏管理器头文件
#include "searchmanager.h"  // 包含搜索管理器头文件
#include "texteditormanager.h" // 包含文本编辑器管理器头文件
#include "aiassistantdialog.h" // 包含AI助手对话框头文件
#include "IAiService.h" // 包含AI服务接口头文件
#include "settingsdialog.h" // 包含设置对话框头文件

#include <QToolButton>
#include <QIcon>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QLayout>
#include <Qt> // 需要Qt命名空间用于WindowFlags
#include <QMouseEvent> // 引入鼠标事件
#include <QGuiApplication> // 引入屏幕信息
#include <QScreen> // 引入屏幕信息
#include <QSizePolicy> // 引入QSizePolicy
#include <QCursor> // 添加光标模块
#include <QDebug> // 引入qDebug
#include <QFile> // 引入QFile
#include <QApplication> // 引入qApp
#include <QSvgRenderer> // 引入SVG渲染器
#include <QPainter> // 引入绘图器
#include <QStyleHints> // 引入QStyleHints
#include <QWindow> // 引入QWindow
#include <QTimer> // 引入QTimer
#include <QSettings> // 添加 QSettings 头文件
#include <QStyleFactory> // 确保包含
#include <QMessageBox> // 添加消息框头文件
#include <QPushButton> // 引入QPushButton
#include <QProcess> // 引入QProcess

// 定义窗口大小调整敏感区域的大小（像素）
#define RESIZE_BORDER_SIZE 8 // 调整区域大小

// 定义所有按钮一致的图标大小
const QSize BUTTON_ICON_SIZE(20, 20);

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_sidebarManager(nullptr) // 初始化 m_sidebarManager
    , m_textEditorManager(nullptr) // 初始化 m_textEditorManager
    , m_aiAssistantDialog(nullptr) // 初始化 m_aiAssistantDialog
{
    ui->setupUi(this);

    // 输出QSettings信息
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, QApplication::organizationName(), QApplication::applicationName());
    qDebug() << "MainWindow构造函数 - QSettings路径:" << settings.fileName();
    qDebug() << "MainWindow构造函数 - 当前主题设置:" << settings.value("General/Theme", "Light").toString();
    qDebug() << "MainWindow构造函数 - 组织名:" << QApplication::organizationName();
    qDebug() << "MainWindow构造函数 - 应用名:" << QApplication::applicationName();

    // 检查是否是重启后的实例
    bool isRestarted = QApplication::arguments().contains("--restarted");
    if (isRestarted) {
        qDebug() << "检测到应用程序重启标志";
    }

    // --- 1. 移除标准窗口框架和设置窗口属性 --- 
    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);  // 允许使用透明背景
    setAttribute(Qt::WA_Hover);  // 让Qt生成悬停事件
    
    // 设置窗口最小尺寸，防止调整得太小
    setMinimumSize(800, 600);

    // 初始化拖动和调整大小变量
    m_dragging = false;
    m_resizing = false;
    m_resizeRegion = None;
    
    // --- 7. 初始化主题 ---
    // 使用之前创建的settings对象
    QString currentTheme = settings.value("General/Theme", "Light").toString(); // 读取主题设置，默认为Light
    m_isDarkTheme = (currentTheme == "Dark"); // 设置 m_isDarkTheme

    // 在设置任何依赖主题的组件之前，先加载初始样式表
    loadAndApplyStyleSheet(m_isDarkTheme ? ":/styles/dark_theme.qss" : ":/styles/light_theme.qss");
    // updateButtonIcons(); // 在主题加载后立即更新图标，确保初始状态正确
    // themeToggleButton->setToolTip(m_isDarkTheme ? "切换到浅色主题" : "切换到深色主题"); // 更新初始提示

    // --- 8. 初始化其他组件 ---
    // 初始化侧边栏
    setupSidebar();
    
    // 初始化搜索功能
    setupSearch();
    
    // 初始化文本编辑器
    setupTextEditor();
    
    // 初始化设置
    setupSettings();
    
    // 检查是否需要恢复上次会话状态
    if (isRestarted) {
        // 延迟执行恢复操作，确保UI完全初始化
        QTimer::singleShot(100, this, &MainWindow::restoreLastSession);
    }
    
    // --- 4. 创建并配置按钮 ---
    // --- 4a. 侧边栏切换按钮 ---
    toggleSidebarButton = new QToolButton(this); 
    toggleSidebarButton->setIcon(QIcon("://icons/round_left_fill.svg")); 
    toggleSidebarButton->setIconSize(QSize(20, 20)); 
    toggleSidebarButton->setFixedSize(28, 28); 
    toggleSidebarButton->setToolTip(ui->sidebarContainer->isVisible() ? "隐藏侧边栏" : "显示侧边栏"); 
    toggleSidebarButton->setCursor(Qt::PointingHandCursor);
    toggleSidebarButton->setStyleSheet("QToolButton { border: none; background-color: transparent; border-radius: 4px; } QToolButton:hover { background-color: #555; }");
    connect(toggleSidebarButton, &QToolButton::clicked, this, &MainWindow::toggleSidebar);

    // --- 4b. 窗口控制按钮 ---
    minimizeButton = new QToolButton(this);
    minimizeButton->setIcon(QIcon("://icons/minis.svg"));
    minimizeButton->setFixedSize(28, 28);
    minimizeButton->setToolTip("最小化");
    minimizeButton->setStyleSheet("QToolButton { border: none; background-color: transparent; } QToolButton:hover { background-color: #555; border-radius: 4px; }");
    connect(minimizeButton, &QToolButton::clicked, this, &MainWindow::showMinimized);

    maximizeButton = new QToolButton(this);
    maximizeButton->setIcon(QIcon("://icons/Maximize-filled.svg"));
    maximizeButton->setFixedSize(28, 28);
    maximizeButton->setToolTip("最大化");
    maximizeButton->setStyleSheet("QToolButton { border: none; background-color: transparent; } QToolButton:hover { background-color: #555; border-radius: 4px; }");
    connect(maximizeButton, &QToolButton::clicked, this, &MainWindow::toggleMaximizeRestore);

    closeButton = new QToolButton(this);
    closeButton->setIcon(QIcon("://icons/round_close_fill.svg"));
    closeButton->setFixedSize(28, 28);
    closeButton->setToolTip("关闭");
    closeButton->setStyleSheet("QToolButton { border: none; background-color: transparent; } QToolButton:hover { background-color: #E81123; border-radius: 4px; } QToolButton:pressed { background-color: #F1707A; }");
    connect(closeButton, &QToolButton::clicked, this, &MainWindow::close);

    // --- 3c. 主题和设置按钮 ---
    themeToggleButton = new QToolButton(this);
    // 以浅色主题开始，显示月亮图标
    themeToggleButton->setIcon(QIcon("://icons/month.svg")); 
    themeToggleButton->setFixedSize(28, 28);
    themeToggleButton->setToolTip("切换主题");
    themeToggleButton->setStyleSheet("QToolButton { border: none; background-color: transparent; } QToolButton:hover { background-color: #555; border-radius: 4px; }");
    connect(themeToggleButton, &QToolButton::clicked, this, &MainWindow::toggleTheme);

    settingsButton = new QToolButton(this);
    settingsButton->setIcon(QIcon("://icons/setting.svg")); 
    settingsButton->setFixedSize(28, 28);
    settingsButton->setToolTip("设置");
    settingsButton->setStyleSheet("QToolButton { border: none; background-color: transparent; } QToolButton:hover { background-color: #555; border-radius: 4px; }");
    connect(settingsButton, &QToolButton::clicked, this, &MainWindow::openSettings);
    
    // --- 5. 使用Designer中创建的widget作为按钮容器 ---
    // 获取Designer中添加的widget控件
    QWidget* buttonWidget = ui->widget;  // 确保widget是您在Designer中添加的控件名称
    
    // 为widget添加水平布局
    QHBoxLayout* buttonLayout = new QHBoxLayout(buttonWidget);
    buttonLayout->setContentsMargins(5, 5, 5, 5);
    buttonLayout->setSpacing(2);
    
    // 在左侧添加侧边栏按钮
    buttonLayout->addWidget(toggleSidebarButton);
    
    // 添加拉伸项，使右侧按钮靠右
    buttonLayout->addStretch(1);
    
    // 在右侧添加主题和设置按钮 (在窗口控制按钮之前)
    buttonLayout->addWidget(themeToggleButton);
    buttonLayout->addWidget(settingsButton);
    // 添加一点间距
    buttonLayout->addSpacing(10); 
    
    // 在右侧添加窗口控制按钮
    buttonLayout->addWidget(minimizeButton);
    buttonLayout->addWidget(maximizeButton);
    buttonLayout->addWidget(closeButton);
    
    // 记住标题栏控件（用于窗口拖动）
    m_titleBarWidget = buttonWidget;

    // --- 6. 启用鼠标跟踪以接收无按钮按下的移动事件 ---
    setMouseTracking(true); 
    // 确保所有子控件都启用鼠标跟踪，以便能将事件传递给窗口边框
    centralWidget()->setMouseTracking(true); 
    ui->widget->setMouseTracking(true);
    ui->sidebarContainer->setMouseTracking(true);
    ui->mainContentContainer->setMouseTracking(true);

    // 根据默认浅色主题设置初始按钮图标颜色
    updateButtonIcons(); 
    themeToggleButton->setToolTip(m_isDarkTheme ? "切换到浅色主题" : "切换到深色主题");

    // --- 8. 安装事件过滤器处理窗口边缘和角落拖动事件 ---
    installEventFilter(this);
}

MainWindow::~MainWindow()
{
    // 保存当前打开的笔记
    saveCurrentNote();
    
    delete ui;
    delete m_sidebarManager; // 释放侧边栏管理器
    delete m_searchManager;
    // m_textEditorManager将通过其父对象自动删除
}

// 加载并应用样式表
void MainWindow::loadAndApplyStyleSheet(const QString &themeStyleSheetPath)
{
    // 首先清除现有样式表
    qApp->setStyleSheet("");
    
    // 加载全局样式表
    QFile globalStyleFile(":/styles/style.qss");
    QString globalStyle;
    if (globalStyleFile.open(QFile::ReadOnly | QFile::Text)) {
        globalStyle = QLatin1String(globalStyleFile.readAll());
        globalStyleFile.close();
    } else {
        qWarning() << "无法加载全局样式表:" << globalStyleFile.errorString();
    }
    
    // 加载主题特定样式表
    QFile themeStyleFile(themeStyleSheetPath);
    QString themeStyle;
    if (themeStyleFile.open(QFile::ReadOnly | QFile::Text)) {
        themeStyle = QLatin1String(themeStyleFile.readAll());
        themeStyleFile.close();
    } else {
        qWarning() << "无法加载主题样式表:" << themeStyleSheetPath << themeStyleFile.errorString();
    }
    
    // 合并样式表并应用
    QString combinedStyle = globalStyle + "\n" + themeStyle;
    qApp->setStyleSheet(combinedStyle);
    
    // 设置dark属性，供样式表使用
    setProperty("dark", m_isDarkTheme);
    
    qDebug() << "应用了全局样式表:" << themeStyleSheetPath;
}

// --- 给SVG图标上色的辅助函数 ---
QIcon MainWindow::colorizeSvgIcon(const QString &path, const QColor &color)
{
    // 加载SVG
    QSvgRenderer renderer(path);
    if (!renderer.isValid()) {
        qWarning() << "无效的SVG文件:" << path;
        return QIcon(); // 出错时返回空图标
    }

    // 获取默认大小
    QSize svgSize = renderer.defaultSize();
    if (svgSize.isEmpty()) {
        qWarning() << "SVG文件没有默认大小:" << path;
        // 回退或固定大小
        svgSize = BUTTON_ICON_SIZE; 
    }

    // 创建像素图以绘制SVG
    QPixmap pixmap(svgSize); // 使用SVG的默认大小进行初始渲染
    pixmap.fill(Qt::transparent); // 确保透明背景

    // 将SVG绘制到像素图上
    QPainter painter(&pixmap);
    renderer.render(&painter);
    painter.end(); // 结束原始SVG绘制

    // 从渲染的像素图创建掩码（alpha通道定义形状）
    QPixmap coloredPixmap(svgSize); // 创建相同大小的最终像素图
    coloredPixmap.fill(color); // 用目标颜色填充
    coloredPixmap.setMask(pixmap.createMaskFromColor(Qt::transparent)); // 使用原始形状作为掩码
    
    return QIcon(coloredPixmap);
}

// 使用着色更新按钮图标的辅助函数
void MainWindow::updateButtonIcons()
{
    // 定义浅色和深色主题的颜色
    QColor iconColor = m_isDarkTheme ? Qt::white : QColor(50, 50, 50); // 浅色主题用深灰色，深色主题用白色
    // QColor closeIconHoverColor = QColor(255, 255, 255); // 红色悬停背景上的白色图标 // 此变量未使用，考虑移除

    // **重要: 使用基础图标路径 (例如 "base")**
    QString basePath = "://icons/";

    // 侧边栏切换
    if (ui->sidebarContainer->isVisible()) {
         toggleSidebarButton->setIcon(colorizeSvgIcon(basePath + "round_left_fill.svg", iconColor));
    } else {
         toggleSidebarButton->setIcon(colorizeSvgIcon(basePath + "round_right_fill.svg", iconColor));
    }
    
    // 窗口控制
    minimizeButton->setIcon(colorizeSvgIcon(basePath + "minis.svg", iconColor));
    if (isMaximized()) {
        maximizeButton->setIcon(colorizeSvgIcon(basePath + "Minimize-filled.svg", iconColor)); // 假设Minimize-filled是还原
    } else {
        maximizeButton->setIcon(colorizeSvgIcon(basePath + "Maximize-filled.svg", iconColor));
    }
    // 关闭按钮需要特殊处理悬停颜色（如果图标颜色改变）
    // 目前使用标准颜色。悬停效果仅为背景。
    closeButton->setIcon(colorizeSvgIcon(basePath + "round_close_fill.svg", iconColor)); 
    
    // 主题切换按钮（使用特定图标但为其着色）
    QString themeIconPath = m_isDarkTheme ? (basePath + "Sun.svg") : (basePath + "month.svg");
    themeToggleButton->setIcon(colorizeSvgIcon(themeIconPath, iconColor)); 
    
    // 设置按钮
    settingsButton->setIcon(colorizeSvgIcon(basePath + "setting.svg", iconColor));
}

// --- 槽函数实现 ---
void MainWindow::toggleSidebar()
{
    bool isVisible = ui->sidebarContainer->isVisible();
    ui->sidebarContainer->setVisible(!isVisible);
    updateButtonIcons(); // 根据新状态和主题更新图标
}

void MainWindow::toggleMaximizeRestore()
{
    if (isMaximized()) {
        showNormal();
    } else {
        showMaximized();
    }
    updateButtonIcons(); // 根据新状态和主题更新图标
}

void MainWindow::toggleTheme()
{
    m_isDarkTheme = !m_isDarkTheme;

    // 保存新的主题设置
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, QApplication::organizationName(), QApplication::applicationName());
    settings.setValue("General/Theme", m_isDarkTheme ? "Dark" : "Light");
    settings.sync(); // 确保设置被同步保存
    
    // 更新图标 (应该在样式表加载之前或之后，确保颜色正确，当前在之前)
    updateButtonIcons(); 
    
    // 加载相应的样式表
    QString styleSheet = m_isDarkTheme ? ":/styles/dark_theme.qss" : ":/styles/light_theme.qss";
    loadAndApplyStyleSheet(styleSheet);
    
    // 更新工具提示
    themeToggleButton->setToolTip(m_isDarkTheme ? "切换到浅色主题" : "切换到深色主题");

    // 更新侧边栏管理器的主题状态
    if (m_sidebarManager) {
        m_sidebarManager->updateTheme(m_isDarkTheme);
    }
    
    // 更新文本编辑器主题 - 这里需确保在样式表加载后调用
    if (m_textEditorManager) {
        m_textEditorManager->setDarkTheme(m_isDarkTheme);
    }

    // 通知AI助手对话框主题已更改
    if (m_aiAssistantDialog) {
        m_aiAssistantDialog->setDarkTheme(m_isDarkTheme);
    }
}

void MainWindow::openSettings()
{
    showSettingsDialog(); // 调用我们集成了主题加载逻辑的 showSettingsDialog
}

// 处理设置对话框关闭事件
void MainWindow::onSettingsClosed()
{
    qDebug() << "MainWindow::onSettingsClosed - 设置对话框已关闭";
    
    // 应用除主题外的其他设置
    applySettings();
    
    // 不再调用applyThemeFromSettings()，因为主题已经通过信号直接应用
}

// 应用设置
void MainWindow::applySettings()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, QApplication::organizationName(), QApplication::applicationName());
    
    // 应用自动保存设置
    bool autoSaveEnabled = settings.value("General/AutoSaveEnabled", false).toBool();
    int autoSaveInterval = settings.value("General/AutoSaveInterval", 5).toInt();
    
    // 停止现有的定时器
    if (m_autoSaveTimer->isActive()) {
        m_autoSaveTimer->stop();
    }
    
    // 如果启用了自动保存，则启动定时器
    if (autoSaveEnabled) {
        m_autoSaveTimer->start(autoSaveInterval * 60 * 1000); // 转换为毫秒
        qDebug() << "自动保存已更新，间隔：" << autoSaveInterval << "分钟";
    }
    
    // 应用自定义背景（如果有）
    QString customBgPath = settings.value("General/CustomBackground", "").toString();
    if (!customBgPath.isEmpty() && QFile::exists(customBgPath)) {
        // 这里可以添加应用自定义背景的代码
        qDebug() << "应用自定义背景：" << customBgPath;
    }
    
    // 应用语言设置（需要重启应用才能生效）
    QString language = settings.value("General/Language", "zh_CN").toString();
    qDebug() << "语言设置已更改为：" << language << "（需要重启应用才能生效）";
}

// 事件过滤器 - 用于处理窗口拖动和调整大小
bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    // 处理窗口移动和调整大小
    if (watched == this) {
        switch (event->type()) {
            case QEvent::MouseButtonPress: {
                auto *mouseEvent = static_cast<QMouseEvent*>(event);
                if (mouseEvent->button() == Qt::LeftButton) {
                    if (m_titleBarWidget && m_titleBarWidget->geometry().contains(mouseEvent->pos())) {
                        // 点击标题栏，准备拖动窗口
                        m_dragging = true;
                        m_mouse_pos = mouseEvent->pos();
                    } else {
                        // 检查是否在调整区域内点击
                        m_resizeRegion = getResizeRegion(mouseEvent->pos());
                        if (m_resizeRegion != None) {
                            m_resizing = true;
                            m_resizeStartPosGlobal = mouseEvent->globalPosition().toPoint();
                            m_resizeStartGeometry = geometry();
                            return true;
                        }
                    }
                }
                break;
            }
            
            case QEvent::MouseMove: {
                auto *mouseEvent = static_cast<QMouseEvent*>(event);
                
                if (m_dragging && (mouseEvent->buttons() & Qt::LeftButton)) {
                    // 移动窗口
                    move(mouseEvent->globalPosition().toPoint() - m_mouse_pos);
                    return true;
                } else if (m_resizing && (mouseEvent->buttons() & Qt::LeftButton)) {
                    // 调整窗口大小
                    handleResize(mouseEvent->globalPosition().toPoint());
                    return true;
                } else {
                    // 根据鼠标位置更新光标
                    updateCursorForResize(mouseEvent->pos());
                }
                
                break;
            }
            
            case QEvent::MouseButtonRelease: {
                if (static_cast<QMouseEvent*>(event)->button() == Qt::LeftButton) {
                    m_dragging = false;
                    m_resizing = false;
                    m_resizeRegion = None;
                }
                break;
            }
            
            case QEvent::Leave: {
                // 鼠标离开窗口时恢复光标
                if (!m_resizing && !m_dragging) {
                    setCursor(Qt::ArrowCursor);
                }
                break;
            }
            
            default:
                break;
        }
    }
    
    return QMainWindow::eventFilter(watched, event);
}

// 获取调整大小的区域
MainWindow::ResizeRegion MainWindow::getResizeRegion(const QPoint &pos)
{
    if (isMaximized()) {
        return None; // 最大化时不允许调整大小
    }
    
    int x = pos.x();
    int y = pos.y();
    int width = this->width();
    int height = this->height();
    
    bool left = x <= RESIZE_BORDER_SIZE;
    bool right = x >= width - RESIZE_BORDER_SIZE;
    bool top = y <= RESIZE_BORDER_SIZE;
    bool bottom = y >= height - RESIZE_BORDER_SIZE;
    
    if (top && left) return TopLeft;
    if (top && right) return TopRight;
    if (bottom && left) return BottomLeft;
    if (bottom && right) return BottomRight;
    if (left) return Left;
    if (right) return Right;
    if (top) return Top;
    if (bottom) return Bottom;
    
    return None;
}

// 根据鼠标位置更新光标形状
void MainWindow::updateCursorForResize(const QPoint &pos)
{
    ResizeRegion region = getResizeRegion(pos);
    
    switch (region) {
        case Left:
        case Right:
            setCursor(Qt::SizeHorCursor);
            break;
        case Top:
        case Bottom:
            setCursor(Qt::SizeVerCursor);
            break;
        case TopLeft:
        case BottomRight:
            setCursor(Qt::SizeFDiagCursor);
            break;
        case TopRight:
        case BottomLeft:
            setCursor(Qt::SizeBDiagCursor);
            break;
        default:
            setCursor(Qt::ArrowCursor);
            break;
    }
}

// 处理窗口大小调整逻辑
void MainWindow::handleResize(const QPoint &globalPos)
{
    if (!m_resizing || isMaximized()) {
        return;
    }
    
    QPoint delta = globalPos - m_resizeStartPosGlobal;
    QRect newGeometry = m_resizeStartGeometry;
    
    // 根据调整区域修改几何形状
    if (m_resizeRegion & Left) {
        newGeometry.setLeft(m_resizeStartGeometry.left() + delta.x());
    }
    if (m_resizeRegion & Right) {
        newGeometry.setRight(m_resizeStartGeometry.right() + delta.x());
    }
    if (m_resizeRegion & Top) {
        newGeometry.setTop(m_resizeStartGeometry.top() + delta.y());
    }
    if (m_resizeRegion & Bottom) {
        newGeometry.setBottom(m_resizeStartGeometry.bottom() + delta.y());
    }
    
    // 应用最小尺寸约束
    if (newGeometry.width() < minimumWidth()) {
        if (m_resizeRegion & Left) {
            newGeometry.setLeft(newGeometry.right() - minimumWidth());
        } else {
            newGeometry.setRight(newGeometry.left() + minimumWidth());
        }
    }
    
    if (newGeometry.height() < minimumHeight()) {
        if (m_resizeRegion & Top) {
            newGeometry.setTop(newGeometry.bottom() - minimumHeight());
        } else {
            newGeometry.setBottom(newGeometry.top() + minimumHeight());
        }
    }
    
    // 设置新的几何形状
    setGeometry(newGeometry);
}

// 以下是为兼容性保留的鼠标事件处理函数
// 大部分逻辑已经移至eventFilter中
void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        // 这里我们保留一些旧的逻辑，以防有些情况未被eventFilter捕获
        m_mouse_pos = event->pos();
    }
    QMainWindow::mousePressEvent(event);
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    // 为兼容性保留
    QMainWindow::mouseMoveEvent(event);
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    // 为兼容性保留
    QMainWindow::mouseReleaseEvent(event);
}

// 处理窗口大小变化事件
void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    
    // 如果窗口状态改变，更新最大化/还原按钮图标
    if (isMaximized() != (maximizeButton->toolTip() == "还原")) {
        if (isMaximized()) {
            maximizeButton->setToolTip("还原");
        } else {
            maximizeButton->setToolTip("最大化");
        }
        updateButtonIcons();
    }
}

// --- 设置侧边栏 ---
void MainWindow::setupSidebar()
{
    // 创建QQuickWidget来显示QML
    m_sidebarWidget = new QQuickWidget(ui->sidebarContainer);
    m_sidebarWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    
    // 创建格局管理器
    QVBoxLayout *layout = new QVBoxLayout(ui->sidebarContainer);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_sidebarWidget);
    ui->sidebarContainer->setLayout(layout);
    
    // 创建侧边栏管理器并初始化
    m_sidebarManager = new SidebarManager(m_sidebarWidget, this);
    m_sidebarManager->initialize();
    
    // 在 SidebarManager 初始化后，立即使用 MainWindow 的当前主题状态更新它
    m_sidebarManager->updateTheme(m_isDarkTheme);
    
    // 连接信号和槽
    connect(m_sidebarManager, &SidebarManager::noteOpened,
            this, &MainWindow::handleNoteSelected);
    
    // 连接搜索按钮点击信号
    connect(m_sidebarManager, &SidebarManager::searchButtonClicked,
            this, &MainWindow::openSearchDialog);
}

// 初始化搜索功能
void MainWindow::setupSearch()
{
    // 需要SidebarManager已初始化，因为使用它的数据库管理器
    if (!m_sidebarManager) {
        qWarning() << "错误: 在初始化搜索管理器之前, 需要先初始化侧边栏管理器";
        return;
    }
    
    // 创建搜索管理器，传入SidebarManager指针
    m_searchManager = new SearchManager(m_sidebarManager->getDatabaseManager(), m_sidebarManager, this);
    
    // 连接搜索管理器和主窗口
    connect(m_searchManager, &SearchManager::noteSelected, this, &MainWindow::handleNoteSelected);
    connect(m_searchManager, &SearchManager::searchClosed, this, &MainWindow::onSearchClosed);
}

// 打开搜索对话框
void MainWindow::openSearchDialog()
{
    if (m_searchManager) {
        m_searchManager->showSearchDialog();
    } else {
        qWarning() << "错误: 搜索管理器尚未初始化";
    }
}

// 处理笔记选择
void MainWindow::handleNoteSelected(const QString &path, const QString &type)
{
    qDebug() << "选中笔记:" << path << "类型:" << type;
    
    // 在加载新笔记前，保存当前笔记
    if (!m_currentNotePath.isEmpty() && m_textEditorManager) {
        saveCurrentNote();
    }
    
    // 更新当前笔记路径
    m_currentNotePath = path;
    
    // 从数据库加载笔记内容并显示
    if (m_textEditorManager) {
        // 这里只是示例，实际应该从数据库管理器获取内容
        // QString content = "<html><body><h1>笔记标题</h1><p>这是一个示例笔记内容</p></body></html>";
        
        // 如果接入了数据库，应该从数据库获取内容
        // content = m_databaseManager->getNoteContent(path);
        
        // 传递笔记路径，由TextEditorManager内部处理加载具体内容
        m_textEditorManager->loadNote(path);
    }
}

// 保存当前笔记
void MainWindow::saveCurrentNote()
{
    if (m_textEditorManager && !m_currentNotePath.isEmpty()) {
        m_textEditorManager->saveNote();
        
        // 这里只是示例，实际应该保存到数据库管理器
        qDebug() << "保存笔记:" << m_currentNotePath;
        // 如果接入了数据库，应该保存到数据库
        // m_databaseManager->saveNoteContent(m_currentNotePath, content);
    }
}

// 处理编辑器内容修改
void MainWindow::handleContentModified()
{
    // 可以在这里实现自动保存功能，或者更新UI显示修改状态
    // qDebug() << "笔记内容已修改";
}

// 初始化文本编辑器
void MainWindow::setupTextEditor()
{
    m_textEditorManager = new TextEditorManager(this);
    connect(m_textEditorManager, SIGNAL(contentModified()), this, SLOT(handleContentModified()));
    
    // 连接 AI 助手请求信号到新的带参数的槽
    connect(m_textEditorManager, &TextEditorManager::requestShowAiAssistant, 
            this, &MainWindow::showAiAssistantWithText); // <--- 修改这里的连接

    QVBoxLayout *mainLayout = new QVBoxLayout(ui->mainContentContainer);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(m_textEditorManager->getEditorWidget());
    m_textEditorManager->setDarkTheme(m_isDarkTheme);
    m_textEditorManager->loadNote("");
}

// 处理搜索对话框关闭
void MainWindow::onSearchClosed()
{
    if (m_sidebarManager) {
        m_sidebarManager->resetToDefaultView();
    }
}

// 设置AI服务
void MainWindow::setAiService(IAiService *service)
{
    m_aiService = service;
    qDebug() << "MainWindow::setAiService called with service:" << service;
    
    // 确保在设置了有效的服务后才初始化AI助手
    if (m_aiService) {
        setupAiAssistant();
    } else {
        qWarning() << "AI Service provided to MainWindow is null!";
    }
}

// 初始化AI助手
void MainWindow::setupAiAssistant()
{
    qDebug() << "MainWindow::setupAiAssistant called.";
    // 检查服务是否有效
    if (!m_aiService) {
        qWarning() << "setupAiAssistant called but m_aiService is null!";
        return;
    }
    
    // 只在对话框未创建时创建它
    if (!m_aiAssistantDialog) {
        m_aiAssistantDialog = new AiAssistantDialog(this);
        m_aiAssistantDialog->setDarkTheme(m_isDarkTheme); // 应用当前主题
        m_aiAssistantDialog->setAiService(m_aiService);
        qDebug() << "AI Service passed to AiAssistantDialog (" << m_aiAssistantDialog << ") :" << m_aiService;
        
        // 连接插入内容的信号
        connect(m_aiAssistantDialog, &AiAssistantDialog::insertContentToDocument, 
                this, &MainWindow::insertAiContent);
                
        // 其他连接...
    } else {
         // 如果对话框已存在，理论上服务应该已经被设置，但可以再次确认
         qDebug() << "AiAssistantDialog (" << m_aiAssistantDialog << ") already exists, ensuring service is set.";
         m_aiAssistantDialog->setAiService(m_aiService);
    }
}

// 显示AI助手对话框 (无参版本，由快捷键等直接调用)
void MainWindow::showAiAssistant()
{
    showAiAssistantWithText("");
}

// 新增辅助函数，处理来自编辑器的请求和直接请求
void MainWindow::showAiAssistantWithText(const QString &selectedTextFromEditor)
{
    qDebug() << "MainWindow::showAiAssistantWithText called. Selected text from editor:" << selectedTextFromEditor.left(50) << "...";
    qDebug() << "MainWindow::showAiAssistantWithText - Current m_aiAssistantDialog instance:" << m_aiAssistantDialog;
    qDebug() << "MainWindow::showAiAssistantWithText - MainWindow's m_aiService:" << m_aiService;

    if (!m_aiAssistantDialog) {
        qWarning() << "CRITICAL ERROR: Attempting to show AI Assistant but m_aiAssistantDialog is null!";
        if (m_aiService) {
            qInfo() << "Attempting to re-initialize AiAssistantDialog as it was null.";
            setupAiAssistant(); 
            if (!m_aiAssistantDialog) { 
                 qCritical() << "CRITICAL ERROR: Failed to re-initialize AiAssistantDialog. Aborting show.";
                 return;
            }
        } else {
            qCritical() << "CRITICAL ERROR: m_aiService in MainWindow is also null. Cannot initialize AiAssistantDialog. Aborting show.";
            return;
        }
    }

    if (!m_aiService) {
         qWarning() << "CRITICAL ERROR: AI Service pointer (m_aiService) in MainWindow is null when trying to show AI Assistant!";
         return;
    }
    
    m_aiAssistantDialog->setAiService(m_aiService); 
    qDebug() << "MainWindow::showAiAssistantWithText - Re-ensured service for dialog instance:" << m_aiAssistantDialog << "MainWindow's m_aiService is:" << m_aiService;

    QString selectedTextToShow;
    if (!selectedTextFromEditor.isEmpty()) {
        selectedTextToShow = selectedTextFromEditor;
    } else if (m_textEditorManager) {
        selectedTextToShow = m_textEditorManager->getSelectedText();
    } else {
         selectedTextToShow = "";
         qWarning() << "TextEditorManager is null, cannot get selected text.";
    }
    m_aiAssistantDialog->setSelectedText(selectedTextToShow);
    m_aiAssistantDialog->setDarkTheme(m_isDarkTheme);
    m_aiAssistantDialog->setFloatingToolBar(m_textEditorManager->getFloatingToolBar());
    
    qDebug() << "MainWindow::showAiAssistantWithText - Showing dialog instance:" << m_aiAssistantDialog;

    if (this->isVisible()) {
        QRect parentGeometry = this->geometry();
        QPoint centerPos = parentGeometry.center() - m_aiAssistantDialog->rect().center();
        m_aiAssistantDialog->move(centerPos);
    } else {
         QScreen *screen = QGuiApplication::primaryScreen();
         if (screen) {
             QRect screenGeometry = screen->availableGeometry();
             m_aiAssistantDialog->move(screenGeometry.center() - m_aiAssistantDialog->rect().center());
         }
    }
    
    m_aiAssistantDialog->show();
    m_aiAssistantDialog->raise();
    m_aiAssistantDialog->activateWindow();
}

// 插入AI生成的内容到编辑器
void MainWindow::insertAiContent(const QString &content)
{
    // 这个函数现在由 AiAssistantDialog 的 insertContentToDocument 信号触发
    if (m_textEditorManager) {
        m_textEditorManager->insertText(content);
    }
}

// 初始化设置模块
void MainWindow::setupSettings()
{
    // 应用初始设置
    SettingsDialog::applySettings();
    
    // 初始化自动保存定时器
    m_autoSaveTimer = new QTimer(this);
    connect(m_autoSaveTimer, &QTimer::timeout, this, &MainWindow::saveCurrentNote);
    
    // 从设置中读取自动保存配置
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, QApplication::organizationName(), QApplication::applicationName());
    bool autoSaveEnabled = settings.value("General/AutoSaveEnabled", false).toBool();
    int autoSaveInterval = settings.value("General/AutoSaveInterval", 5).toInt();
    
    // 如果启用了自动保存，则启动定时器
    if (autoSaveEnabled) {
        m_autoSaveTimer->start(autoSaveInterval * 60 * 1000); // 转换为毫秒
        qDebug() << "自动保存已启用，间隔：" << autoSaveInterval << "分钟";
    }
}

// 显示设置对话框
void MainWindow::showSettingsDialog()
{
    // 确保只有一个设置对话框实例
    if (!m_settingsDialog) {
        m_settingsDialog = new SettingsDialog(this);
        // 连接主题改变信号到应用主题槽
        connect(m_settingsDialog, &SettingsDialog::themeChanged, this, &MainWindow::applyTheme);
        // 连接语言改变信号到应用语言槽
        connect(m_settingsDialog, &SettingsDialog::languageChanged, this, &MainWindow::applyLanguage);
        // 连接对话框关闭信号
        connect(m_settingsDialog, &QDialog::finished, this, &MainWindow::onSettingsClosed);
    }
    m_settingsDialog->loadSettings(); // 打开对话框前加载当前设置
    m_settingsDialog->exec(); // 使用 exec() 以模态方式显示
    
    // 不再在这里调用applyThemeFromSettings，因为主题已经通过信号直接应用
}

// 应用语言设置
void MainWindow::applyLanguage(const QString &language)
{
    qDebug() << "MainWindow::applyLanguage - 语言设置已更改为:" << language;
    
    // 保存语言设置到配置文件
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, QApplication::organizationName(), QApplication::applicationName());
    settings.setValue("General/Language", language);
    settings.sync();
    
    // 创建自定义消息框，提供立即重启选项
    QMessageBox msgBox(this);
    msgBox.setWindowTitle(tr("语言设置"));
    msgBox.setText(tr("语言设置已更改为 %1。\n需要重启应用程序以应用新的语言设置。").arg(
        language == "zh_CN" ? "简体中文" : 
        language == "en_US" ? "English" : 
        tr("跟随系统")));
    msgBox.setIcon(QMessageBox::Information);
    
    // 添加立即重启和稍后重启按钮
    QPushButton *restartNowButton = msgBox.addButton(tr("立即重启"), QMessageBox::AcceptRole);
    msgBox.addButton(tr("稍后重启"), QMessageBox::RejectRole);
    
    msgBox.exec();
    
    // 如果用户选择了立即重启
    if (msgBox.clickedButton() == restartNowButton) {
        restartApplication();
    }
}

// 重启应用程序
void MainWindow::restartApplication()
{
    qDebug() << "正在重启应用程序...";
    
    // 保存所有未保存的数据
    saveCurrentNote();
    
    // 保存当前状态（如打开的文件路径）
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, QApplication::organizationName(), QApplication::applicationName());
    settings.setValue("LastSession/CurrentNote", m_currentNotePath);
    
    // 保存当前窗口状态
    settings.setValue("LastSession/WindowGeometry", saveGeometry());
    settings.setValue("LastSession/WindowState", saveState());
    settings.setValue("LastSession/IsMaximized", isMaximized());
    
    settings.sync(); // 确保所有设置都被写入
    
    // 获取应用程序路径
    QString appPath = QApplication::applicationFilePath();
    QStringList arguments = QApplication::arguments();
    arguments.removeFirst(); // 移除程序名称
    
    // 添加重启标记参数
    arguments << "--restarted";
    
    qDebug() << "启动新进程:" << appPath << arguments;
    
    // 启动新进程
    bool success = QProcess::startDetached(appPath, arguments);
    
    if (success) {
        qDebug() << "新进程启动成功，即将关闭当前实例";
        // 关闭当前实例
        QApplication::quit();
    } else {
        qCritical() << "启动新进程失败";
        QMessageBox::critical(this, tr("重启失败"), 
            tr("应用程序重启失败。请手动关闭并重新启动应用程序。"));
    }
}

// 直接应用指定的主题
void MainWindow::applyTheme(const QString &theme)
{
    bool newIsDarkStatus = m_isDarkTheme; // 用于判断是否需要更新图标等
    QString styleSheetToLoad;

    if (theme == "Dark") {
        newIsDarkStatus = true;
        styleSheetToLoad = ":/styles/dark_theme.qss";
    } else if (theme == "Light") {
        newIsDarkStatus = false;
        styleSheetToLoad = ":/styles/light_theme.qss";
    } else if (theme == "System") {
        // 首先重置样式表，避免之前设置的样式影响
        qApp->setStyleSheet("");
        
        // 设置基础样式
        qApp->setStyle(QStyleFactory::create("Fusion"));
        
        // 获取系统调色板
        QPalette systemPalette = qApp->style()->standardPalette();
        qApp->setPalette(systemPalette);

        // 根据系统调色板的亮度判断是亮色还是暗色，用于统一内部状态和图标
        newIsDarkStatus = (systemPalette.color(QPalette::Window).lightness() < 128);
        
        // 根据系统主题的亮暗选择相应的样式表
        styleSheetToLoad = newIsDarkStatus ? ":/styles/dark_theme.qss" : ":/styles/light_theme.qss";
    } else {
        qWarning() << "MainWindow::applyTheme - 未知主题名称:" << theme;
        return;
    }

    // 更新状态和组件
    m_isDarkTheme = newIsDarkStatus;

    if (!styleSheetToLoad.isEmpty()) {
        // 加载样式表
        loadAndApplyStyleSheet(styleSheetToLoad); 
    }

    updateButtonIcons(); // 更新所有按钮图标
    themeToggleButton->setToolTip(m_isDarkTheme ? "切换到浅色主题" : "切换到深色主题");

    if (m_sidebarManager) {
        m_sidebarManager->updateTheme(m_isDarkTheme);
    }
    if (m_textEditorManager) {
        m_textEditorManager->setDarkTheme(m_isDarkTheme);
    }
    if (m_aiAssistantDialog) {
        m_aiAssistantDialog->setDarkTheme(m_isDarkTheme);
    }
    
    // 保存主题设置
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, QApplication::organizationName(), QApplication::applicationName());
    settings.setValue("General/Theme", theme);
    settings.sync();
}

// 新增一个公共槽函数，用于从设置对话框应用主题
void MainWindow::applyThemeFromSettings()
{
    // 使用完整的QSettings构造函数，确保与SettingsDialog使用相同的设置文件
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, QApplication::organizationName(), QApplication::applicationName());
    // 确保读取最新的设置
    settings.clear(); // 清除缓存
    settings.sync();
    QString themeName = settings.value("General/Theme", "Light").toString();
    
    // 直接调用applyTheme方法
    applyTheme(themeName);
}

// 恢复上次会话状态
void MainWindow::restoreLastSession()
{
    qDebug() << "正在恢复上次会话状态...";
    
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, QApplication::organizationName(), QApplication::applicationName());
    
    // 恢复窗口几何形状和状态
    if (settings.contains("LastSession/WindowGeometry")) {
        restoreGeometry(settings.value("LastSession/WindowGeometry").toByteArray());
    }
    
    if (settings.contains("LastSession/WindowState")) {
        restoreState(settings.value("LastSession/WindowState").toByteArray());
    }
    
    // 如果之前是最大化状态，则恢复最大化
    if (settings.value("LastSession/IsMaximized", false).toBool()) {
        showMaximized();
    }
    
    // 恢复上次打开的笔记
    QString lastNotePath = settings.value("LastSession/CurrentNote", "").toString();
    if (!lastNotePath.isEmpty() && m_sidebarManager) {
        // 延迟加载笔记，确保侧边栏已经完全初始化
        QTimer::singleShot(200, this, [this, lastNotePath]() {
            qDebug() << "恢复打开上次的笔记:" << lastNotePath;
            m_sidebarManager->openNoteByPath(lastNotePath);
        });
    }
    
    // 更新按钮图标
    updateButtonIcons();
}

// --- 其他槽函数和方法 ... ---
