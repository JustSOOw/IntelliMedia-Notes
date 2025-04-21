/*
 * @Author: Furdow wang22338014@gmail.com
 * @Date: 2025-04-14 17:37:03
 * @LastEditors: Furdow wang22338014@gmail.com
 * @LastEditTime: 2025-04-20 18:35:22
 * @FilePath: \IntelliMedia_Notes\src\mainwindow.cpp
 * @Description: 
 * 
 * Copyright (c) 2025 by Furdow, All Rights Reserved. 
 */
#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "sidebarmanager.h" // 包含侧边栏管理器头文件
#include "searchmanager.h"  // 包含搜索管理器头文件

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

// 定义窗口大小调整敏感区域的大小（像素）
#define RESIZE_BORDER_SIZE 8 // 调整区域大小

// 定义所有按钮一致的图标大小
const QSize BUTTON_ICON_SIZE(20, 20);

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

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
    
    // 初始化侧边栏
    setupSidebar();
    
    // 初始化搜索功能
    setupSearch();
    
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
    // 假设以浅色主题开始，显示月亮图标
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

    // --- 7. 初始化主题
    m_isDarkTheme = false; // 以浅色主题开始
    updateButtonIcons(); // 根据默认浅色主题设置初始图标
    
    // --- 8. 安装事件过滤器处理窗口边缘和角落拖动事件 ---
    installEventFilter(this);
}

MainWindow::~MainWindow()
{
    delete ui;
    delete m_sidebarManager; // 释放侧边栏管理器
}

// 加载并应用样式表的辅助函数
void MainWindow::loadAndApplyStyleSheet(const QString &sheetName)
{
    QFile file(sheetName);
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        QString styleSheet = QLatin1String(file.readAll());
        qApp->setStyleSheet(styleSheet); // 应用于整个应用程序
        file.close();
        qDebug() << "应用了样式表:" << sheetName;
    } else {
        qWarning() << "无法打开样式表文件:" << sheetName;
    }
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
    QColor closeIconHoverColor = QColor(255, 255, 255); // 红色悬停背景上的白色图标

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
    
    if (m_isDarkTheme) {
        loadAndApplyStyleSheet("://styles/dark_theme.qss");
    } else {
        loadAndApplyStyleSheet("://styles/light_theme.qss");
    }
    
    updateButtonIcons(); // 为新主题更新图标
}

void MainWindow::openSettings()
{
    // TODO: 实现打开设置对话框/面板
    qDebug() << "设置按钮被点击!";
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
    
    // 创建搜索管理器
    m_searchManager = new SearchManager(m_sidebarManager->getDatabaseManager(), this);
    
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
    qDebug() << "主窗口处理笔记选择:" << path << type;
    // TODO: 在主内容区域显示选中的笔记
}

// 处理搜索对话框关闭
void MainWindow::onSearchClosed()
{
    if (m_sidebarManager) {
        m_sidebarManager->resetToDefaultView();
    }
}
