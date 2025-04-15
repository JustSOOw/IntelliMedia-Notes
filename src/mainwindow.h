/*
 * @Author: Furdow wang22338014@gmail.com
 * @Date: 2025-04-14 17:37:03
 * @LastEditors: Furdow wang22338014@gmail.com
 * @LastEditTime: 2025-04-15 21:40:10
 * @FilePath: \IntelliMedia_Notes\src\mainwindow.h
 * @Description: 
 * 
 * Copyright (c) 2025 by Furdow, All Rights Reserved. 
 */
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPoint> // Include QPoint for storing mouse position

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class QToolButton; // Forward declaration
class QMouseEvent; // Forward declaration for event handlers

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    // Override mouse event handlers for window dragging
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    
    // Function to update cursor based on position over edges
    void updateCursorShape(const QPoint &pos); 

private slots:
    void toggleSidebar();
    void toggleMaximizeRestore(); // Slot for maximize/restore button
    void toggleTheme();           // Slot for theme toggle button
    void openSettings();          // Slot for settings button

private:
    Ui::MainWindow *ui;

    // Window control buttons
    QToolButton *toggleSidebarButton;
    QToolButton *minimizeButton;
    QToolButton *maximizeButton;
    QToolButton *closeButton;
    QToolButton *themeToggleButton;   // Declare theme toggle button
    QToolButton *settingsButton;      // Declare settings button

    // For window dragging
    bool m_mouse_pressed = false;
    QPoint m_mouse_pos; // Position relative to window top-left
    QWidget* m_titleBarWidget = nullptr; // Reference to the title bar area for dragging (now ui->widget)
    
    // --- Restore Resizing Variables and Enum ---
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
    
    bool m_resizing = false;            
    ResizeRegion m_resizeRegion = None; 
    QPoint m_resizeStartPosGlobal; // Store global start position
    QRect m_resizeStartGeometry; // Store original window geometry
    
    // Helper function to determine resize region
    ResizeRegion getResizeRegion(const QPoint &clientPos);
    // -----------------------------------------
};
#endif // MAINWINDOW_H
