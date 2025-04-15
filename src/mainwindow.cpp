/*
 * @Author: Furdow wang22338014@gmail.com
 * @Date: 2025-04-14 17:37:03
 * @LastEditors: Furdow wang22338014@gmail.com
 * @LastEditTime: 2025-04-15 23:09:41
 * @FilePath: \IntelliMedia_Notes\src\mainwindow.cpp
 * @Description: 
 * 
 * Copyright (c) 2025 by Furdow, All Rights Reserved. 
 */
#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QToolButton>
#include <QIcon>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QLayout>
#include <Qt> // Need Qt namespace for WindowFlags
#include <QMouseEvent> // Include for mouse events
#include <QGuiApplication> // Include for screen information
#include <QScreen> // Include for screen information
#include <QSizePolicy> // Include for QSizePolicy
#include <QCursor> // 添加光标模块
#include <QDebug> // Include for qDebug

// 定义窗口大小调整敏感区域的大小（像素）
#define RESIZE_BORDER_SIZE 8 // 增加边缘敏感区域大小，使调整更容易

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // --- 1. 移除标准窗口框架和设置窗口属性 --- 
    setWindowFlags(Qt::FramelessWindowHint);
    
    // 必要的属性设置，确保窗口可以调整大小
    // setAttribute(Qt::WA_NoSystemBackground, false); // These might interfere with SizeGrip or styling
    // setAttribute(Qt::WA_TranslucentBackground, false);
    
    // 设置窗口最小尺寸，防止调整得太小
    setMinimumSize(800, 600);

    // 初始化拖动和调整大小变量
    m_mouse_pressed = false;
    m_resizing = false;
    m_resizeRegion = None;
    // -----------------------------

    // --- 2. 启用状态栏并在其上添加 QSizeGrip ---
    // QStatusBar *statusBar = new QStatusBar(this);
    // setStatusBar(statusBar);
    // statusBar->setSizeGripEnabled(true); // 在状态栏右下角启用 SizeGrip
    // 可选：隐藏状态栏本身，只留下 SizeGrip
    // statusBar->hide(); 
    // 或者给状态栏设置样式，使其融入背景
    // statusBar->setStyleSheet("QStatusBar { background: transparent; border: none; } QStatusBar::item { border: 0px; }");
    // ------------------------------------------

    // --- 3. 设置调试用背景色 ---
    ui->sidebarContainer->setStyleSheet("background-color: lightblue;");
    ui->mainContentContainer->setStyleSheet("background-color: lightyellow;");

    // --- 4. 创建并配置按钮 ---
    // --- 4a. 侧边栏切换按钮 ---
    toggleSidebarButton = new QToolButton(this); 
    toggleSidebarButton->setIcon(QIcon("://icons/round_left_fill.svg")); 
    toggleSidebarButton->setIconSize(QSize(20, 20)); 
    toggleSidebarButton->setFixedSize(28, 28); 
    toggleSidebarButton->setToolTip(ui->sidebarContainer->isVisible() ? "Hide Sidebar" : "Show Sidebar"); 
    toggleSidebarButton->setCursor(Qt::PointingHandCursor);
    toggleSidebarButton->setStyleSheet("QToolButton { border: none; background-color: transparent; border-radius: 4px; } QToolButton:hover { background-color: #555; }");
    connect(toggleSidebarButton, &QToolButton::clicked, this, &MainWindow::toggleSidebar);

    // --- 4b. 窗口控制按钮 ---
    minimizeButton = new QToolButton(this);
    minimizeButton->setIcon(QIcon("://icons/minis.svg"));
    minimizeButton->setFixedSize(28, 28);
    minimizeButton->setToolTip("Minimize");
    minimizeButton->setStyleSheet("QToolButton { border: none; background-color: transparent; } QToolButton:hover { background-color: #555; border-radius: 4px; }");
    connect(minimizeButton, &QToolButton::clicked, this, &MainWindow::showMinimized);

    maximizeButton = new QToolButton(this);
    maximizeButton->setIcon(QIcon("://icons/Maximize-filled.svg"));
    maximizeButton->setFixedSize(28, 28);
    maximizeButton->setToolTip("Maximize");
    maximizeButton->setStyleSheet("QToolButton { border: none; background-color: transparent; } QToolButton:hover { background-color: #555; border-radius: 4px; }");
    connect(maximizeButton, &QToolButton::clicked, this, &MainWindow::toggleMaximizeRestore);

    closeButton = new QToolButton(this);
    closeButton->setIcon(QIcon("://icons/round_close_fill.svg"));
    closeButton->setFixedSize(28, 28);
    closeButton->setToolTip("Close");
    closeButton->setStyleSheet("QToolButton { border: none; background-color: transparent; } QToolButton:hover { background-color: #E81123; border-radius: 4px; } QToolButton:pressed { background-color: #F1707A; }");
    connect(closeButton, &QToolButton::clicked, this, &MainWindow::close);

    // --- 3c. Theme and Settings buttons ---
    themeToggleButton = new QToolButton(this);
    // Assume starting with light theme, so show moon icon
    themeToggleButton->setIcon(QIcon("://icons/month.svg")); 
    themeToggleButton->setFixedSize(28, 28);
    themeToggleButton->setToolTip("Switch Theme");
    themeToggleButton->setStyleSheet("QToolButton { border: none; background-color: transparent; } QToolButton:hover { background-color: #555; border-radius: 4px; }");
    connect(themeToggleButton, &QToolButton::clicked, this, &MainWindow::toggleTheme);

    settingsButton = new QToolButton(this);
    settingsButton->setIcon(QIcon("://icons/setting.svg")); 
    settingsButton->setFixedSize(28, 28);
    settingsButton->setToolTip("Settings");
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
    
    // 设置buttonWidget的样式，添加淡灰色背景和底部边框
    buttonWidget->setStyleSheet("background-color: rgba(240, 240, 240, 180); border-bottom: 1px solid #ccc;");
    
    // 记住标题栏控件（用于窗口拖动）
    m_titleBarWidget = buttonWidget;

    // --- 6. 启用 Mouse Tracking 以接收无按钮按下的 move 事件 ---
    setMouseTracking(true); 
    // 对于 centralWidget 及其子控件也启用，确保事件能到达 MainWindow
    centralWidget()->setMouseTracking(true); 
    // 如果按钮栏 ui->widget 覆盖了边缘，也为它启用
    ui->widget->setMouseTracking(true);
    // 如果侧边栏或主内容区可能覆盖边缘，也为它们启用
    // ui->sidebarContainer->setMouseTracking(true);
    // ui->mainContentContainer->setMouseTracking(true);
}

MainWindow::~MainWindow()
{
    delete ui;
}

// --- 槽函数实现 ---
void MainWindow::toggleSidebar()
{
    bool isVisible = ui->sidebarContainer->isVisible();
    ui->sidebarContainer->setVisible(!isVisible);

    if (isVisible) { // isVisible现在表示隐藏前的状态
        toggleSidebarButton->setToolTip("Show Sidebar");
        // 侧边栏被隐藏，设置向右箭头图标
        toggleSidebarButton->setIcon(QIcon("://icons/round_right_fill.svg")); 
    } else { // 侧边栏将要显示
        toggleSidebarButton->setToolTip("Hide Sidebar");
        // 侧边栏已显示，设置向左箭头图标
        toggleSidebarButton->setIcon(QIcon("://icons/round_left_fill.svg")); 
    }
}

void MainWindow::toggleMaximizeRestore()
{
    if (isMaximized()) {
        showNormal();
        maximizeButton->setIcon(QIcon("://icons/Maximize-filled.svg")); // 恢复图标
        maximizeButton->setToolTip("Maximize");
    } else {
        showMaximized();
        maximizeButton->setIcon(QIcon("://icons/Minimize-filled.svg")); // 还原图标
        maximizeButton->setToolTip("Restore Down");
    }
}

// --- 新槽函数实现 (暂时为空) ---
void MainWindow::toggleTheme()
{
    // TODO: Implement theme switching logic
    // 1. Determine current theme
    // 2. Load the other theme's QSS file (e.g., dark.qss or light.qss)
    // 3. Apply the new QSS to the application (qApp->setStyleSheet())
    // 4. Update the themeToggleButton icon (to sun or moon)
    // 5. Notify QML side if needed (e.g., via context property update)
    qDebug() << "Toggle Theme button clicked!"; 
    // Example: Toggle icon
    static bool isDark = false; // Simple state tracking for example
    if (isDark) {
        themeToggleButton->setIcon(QIcon("://icons/month.svg"));
    } else {
        themeToggleButton->setIcon(QIcon("://icons/Sun.svg"));
    }
    isDark = !isDark;
}

void MainWindow::openSettings()
{
    // TODO: Implement opening the settings dialog/panel
    qDebug() << "Settings button clicked!";
}

// --- 鼠标事件处理实现 (重新实现) ---

// Helper function to determine resize region
MainWindow::ResizeRegion MainWindow::getResizeRegion(const QPoint &clientPos) {
    int width = this->width();
    int height = this->height();
    
    bool top = clientPos.y() <= RESIZE_BORDER_SIZE;
    bool bottom = clientPos.y() >= height - RESIZE_BORDER_SIZE;
    bool left = clientPos.x() <= RESIZE_BORDER_SIZE;
    bool right = clientPos.x() >= width - RESIZE_BORDER_SIZE;

    // Prioritize corners
    if (top && left) return TopLeft;
    if (top && right) return TopRight;
    if (bottom && left) return BottomLeft;
    if (bottom && right) return BottomRight;
    // Then edges
    if (top) return Top;
    if (bottom) return Bottom;
    if (left) return Left;
    if (right) return Right;
    
    return None;
}

// Update cursor shape based on position
void MainWindow::updateCursorShape(const QPoint &pos) {
    if (isMaximized() || m_resizing) { // Don't change cursor if maximized or already resizing
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
        default:          unsetCursor(); break; // Use default cursor
    }
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        // Check if starting resize
        m_resizeRegion = getResizeRegion(event->pos());
        if (m_resizeRegion != None && !isMaximized()) { // Allow resize only if not maximized
            m_resizing = true;
            m_resizeStartPosGlobal = event->globalPosition().toPoint();
            m_resizeStartGeometry = this->geometry(); // Store full geometry
            event->accept();
            return; // Don't process drag if resizing
        }
        
        // Check if starting drag
        if (m_titleBarWidget && m_titleBarWidget->geometry().contains(event->pos())) {
            m_mouse_pressed = true;
            m_mouse_pos = event->globalPosition().toPoint() - this->frameGeometry().topLeft();
            event->accept();
            return; // Processed drag start
        }
    }
    // Call base class implementation if event is not handled
    QMainWindow::mousePressEvent(event);
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    // Handle resize
    if (m_resizing && (event->buttons() & Qt::LeftButton)) {
        QPoint delta = event->globalPosition().toPoint() - m_resizeStartPosGlobal;
        QRect newGeometry = m_resizeStartGeometry;

        if (m_resizeRegion & Left)   newGeometry.setLeft(m_resizeStartGeometry.left() + delta.x());
        if (m_resizeRegion & Right)  newGeometry.setRight(m_resizeStartGeometry.right() + delta.x());
        if (m_resizeRegion & Top)    newGeometry.setTop(m_resizeStartGeometry.top() + delta.y());
        if (m_resizeRegion & Bottom) newGeometry.setBottom(m_resizeStartGeometry.bottom() + delta.y());

        // Ensure minimum size constraints
        QSize minSize = this->minimumSize();
        if (newGeometry.width() < minSize.width()) {
            if (m_resizeRegion & Left) newGeometry.setLeft(newGeometry.right() - minSize.width());
            else newGeometry.setWidth(minSize.width());
        }
        if (newGeometry.height() < minSize.height()) {
            if (m_resizeRegion & Top) newGeometry.setTop(newGeometry.bottom() - minSize.height());
            else newGeometry.setHeight(minSize.height());
        }

        this->setGeometry(newGeometry);
        event->accept();
    }
    // Handle drag
    else if (m_mouse_pressed && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPosition().toPoint() - m_mouse_pos);
        event->accept();
    }
    // Update cursor if mouse tracking is enabled and not dragging/resizing
    else if (hasMouseTracking()) {
         updateCursorShape(event->pos());
         QMainWindow::mouseMoveEvent(event); // Call base class for other move event handling
    }
    else {
        QMainWindow::mouseMoveEvent(event); // Call base class if not handled
    }
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_mouse_pressed = false;
        if (m_resizing) {
            m_resizing = false;
            m_resizeRegion = None;
            unsetCursor(); // Reset cursor after resizing
        }
        event->accept();
    }
    else {
        QMainWindow::mouseReleaseEvent(event); // Call base class if not handled
    }
}
// ------------------------------------
