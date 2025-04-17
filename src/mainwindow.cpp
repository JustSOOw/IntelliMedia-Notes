/*
 * @Author: Furdow wang22338014@gmail.com
 * @Date: 2025-04-14 17:37:03
 * @LastEditors: Furdow wang22338014@gmail.com
 * @LastEditTime: 2025-04-17 14:50:41
 * @FilePath: \IntelliMedia_Notes\src\mainwindow.cpp
 * @Description: 
 * 
 * Copyright (c) 2025 by Furdow, All Rights Reserved. 
 */
#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "sidebarmanager.h" // 包含侧边栏管理器头文件

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

// 定义窗口大小调整敏感区域的大小（像素）
#define RESIZE_BORDER_SIZE 8 // 增加边缘敏感区域大小，使调整更容易

// 定义所有按钮一致的图标大小
const QSize BUTTON_ICON_SIZE(20, 20);

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // --- 1. 移除标准窗口框架和设置窗口属性 --- 
    setWindowFlags(Qt::FramelessWindowHint);
    

    
    // 设置窗口最小尺寸，防止调整得太小
    setMinimumSize(800, 600);

    // 初始化拖动和调整大小变量
    m_dragging = false;
    m_resizing = false;
    m_resizeRegion = None;
    
    // 初始化侧边栏
    setupSidebar();
    
    // -----------------------------



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
    // --------------------------------------

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
    // 对于centralWidget及其子控件也启用，确保事件能到达MainWindow
    centralWidget()->setMouseTracking(true); 
    // 如果按钮栏ui->widget覆盖了边缘，也为它启用
    ui->widget->setMouseTracking(true);
    // 如果侧边栏或主内容区可能覆盖边缘，也为它们启用
    // ui->sidebarContainer->setMouseTracking(true);
    // ui->mainContentContainer->setMouseTracking(true);

    // --- 7. 初始化主题（现在在main.cpp加载初始样式表） ---
    m_isDarkTheme = false; // 以浅色主题开始
    updateButtonIcons(); // 根据默认浅色主题设置初始图标
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
        qDebug() << "Applied stylesheet:" << sheetName;
    } else {
        qWarning() << "Could not open stylesheet file:" << sheetName;
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

// --- 鼠标事件处理实现 ---

// 确定调整大小区域的辅助函数
MainWindow::ResizeRegion MainWindow::getResizeRegion(const QPoint &clientPos) {
    int width = this->width();
    int height = this->height();
    
    bool top = clientPos.y() <= RESIZE_BORDER_SIZE;
    bool bottom = clientPos.y() >= height - RESIZE_BORDER_SIZE;
    bool left = clientPos.x() <= RESIZE_BORDER_SIZE;
    bool right = clientPos.x() >= width - RESIZE_BORDER_SIZE;

    // 优先处理角落
    if (top && left) return TopLeft;
    if (top && right) return TopRight;
    if (bottom && left) return BottomLeft;
    if (bottom && right) return BottomRight;
    // 然后是边缘
    if (top) return Top;
    if (bottom) return Bottom;
    if (left) return Left;
    if (right) return Right;
    
    return None;
}

// 根据位置更新光标形状
void MainWindow::updateCursorShape(const QPoint &pos) {
    if (isMaximized() || m_resizing) { // 如果已最大化或正在调整大小，不改变光标
        return;
    }

    ResizeRegion region = getResizeRegion(pos);
    
    switch (region) {
        case TopLeft:     setCursor(Qt::SizeFDiagCursor); break;
        case TopRight:    setCursor(Qt::SizeBDiagCursor); break;
        case BottomLeft:  setCursor(Qt::SizeBDiagCursor); break;
        case BottomRight: setCursor(Qt::SizeFDiagCursor); break;
        case Top:         setCursor(Qt::SizeVerCursor);   break;
        case Bottom:      setCursor(Qt::SizeVerCursor);   break;
        case Left:        setCursor(Qt::SizeHorCursor);   break;
        case Right:       setCursor(Qt::SizeHorCursor);   break;
        default:          unsetCursor(); break; // 使用默认光标
    }
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    // --- 窗口大小调整 --- 
    // 在按下时计算调整区域
    ResizeRegion region = getResizeRegion(event->pos());
    
    // 检查是否在调整区域内按下左键，并且窗口未最大化
    if (region != None && event->button() == Qt::LeftButton && !isMaximized()) {
        m_resizing = true;
        m_resizeRegion = region; // 存储当前区域
        m_resizeStartPosGlobal = event->globalPosition().toPoint();
        m_resizeStartGeometry = geometry();
        event->accept(); // 接受事件，防止传递给父控件
        return; // 如果开始调整大小，则不处理拖动
    }

    // --- 窗口拖动 --- 
    if (event->button() == Qt::LeftButton) {
         // 检查是否在标题栏区域内按下
         if (m_titleBarWidget && m_titleBarWidget->rect().contains(event->pos())) {
             m_mouse_pos = event->pos(); // 存储相对位置
             m_dragging = true; // 设置拖动标志
             event->accept(); // 接受事件
             return;
         }
    }

    QMainWindow::mousePressEvent(event); // 调用基类实现处理其他情况
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    // --- 窗口大小调整 ---
    if (m_resizing) {
        QPoint delta = event->globalPosition().toPoint() - m_resizeStartPosGlobal;
        QRect newGeometry = m_resizeStartGeometry;
        
        // 根据调整区域修改几何信息
        if (m_resizeRegion & Top)    newGeometry.setTop(newGeometry.top() + delta.y());
        if (m_resizeRegion & Bottom) newGeometry.setBottom(newGeometry.bottom() + delta.y());
        if (m_resizeRegion & Left)   newGeometry.setLeft(newGeometry.left() + delta.x());
        if (m_resizeRegion & Right)  newGeometry.setRight(newGeometry.right() + delta.x());

        // 限制最小尺寸 (例如 200x100)
        if (newGeometry.width() < 200) {
            if (m_resizeRegion & Left) newGeometry.setLeft(newGeometry.right() - 200);
            else newGeometry.setWidth(200);
        }
        if (newGeometry.height() < 100) {
            if (m_resizeRegion & Top) newGeometry.setTop(newGeometry.bottom() - 100);
            else newGeometry.setHeight(100);
        }

        setGeometry(newGeometry);
        event->accept();
        return;
    }

    // --- 窗口拖动 ---
    else if (m_dragging && (event->buttons() & Qt::LeftButton)) { // 使用 m_dragging
        // 检查移动是否在标题栏区域内开始（理论上m_dragging为true时已满足）
        // if (m_titleBarWidget && m_titleBarWidget->rect().contains(m_mouse_pos)) { // 这个检查可以省略，因为m_dragging已保证
             move(event->globalPosition().toPoint() - m_mouse_pos);
             event->accept();
             return;
        // }
    }

    // 仅当不调整大小且不拖动时才更新光标形状
    updateCursorShape(event->pos()); 
    QMainWindow::mouseMoveEvent(event); // 调用基类处理其他移动事件
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    // --- 窗口大小调整 ---
    if (m_resizing && event->button() == Qt::LeftButton) {
        m_resizing = false; // 重置调整大小状态
        m_resizeRegion = None; // 重置调整区域
        unsetCursor(); // 恢复默认光标
        event->accept();
        return;
    }
    
    // --- 窗口拖动 ---
    if (m_dragging && event->button() == Qt::LeftButton) {
        m_dragging = false; // 重置拖动状态
        event->accept();
        return;
    }

    QMainWindow::mouseReleaseEvent(event); // 调用基类实现
    unsetCursor(); // 确保在释放时恢复光标
}
// ------------------------------------

// --- 窗口大小调整事件处理 ---
void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event); // 调用基类实现是良好的实践
    
    // 窗口大小改变后，更新最大化/还原按钮的图标
    updateButtonIcons(); 
    
    // 在调整大小后，可能需要重新计算或更新光标形状逻辑（如果光标依赖于窗口尺寸）
    // 例如，如果鼠标恰好在新的边缘上
    // updateCursorShape(mapFromGlobal(QCursor::pos())); 
    // 注意: mapFromGlobal可能需要QWidget*上下文，或者直接使用event中的信息（如果适用）
}

// 设置侧边栏
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
}

// 处理笔记选择
void MainWindow::handleNoteSelected(const QString &path, const QString &type)
{
    qDebug() << "主窗口处理笔记选择:" << path << type;
    // TODO: 在主内容区域显示选中的笔记
}
