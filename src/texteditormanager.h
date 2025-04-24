/*
 * @Author: cursor AI
 * @Date: 2023-05-05 10:00:00
 * @LastEditors: cursor AI
 * @LastEditTime: 2023-05-05 10:00:00
 * @FilePath: \IntelliMedia_Notes\src\texteditormanager.h
 * @Description: QTextEdit编辑器管理类，实现富文本编辑功能
 * 
 * Copyright (c) 2023, All Rights Reserved. 
 */
#ifndef TEXTEDITORMANAGER_H
#define TEXTEDITORMANAGER_H

#include <QObject>
#include <QTextEdit>
#include <QWidget>
#include <QToolBar>
#include <QToolButton>
#include <QAction>
#include <QComboBox>
#include <QSpinBox>
#include <QColorDialog>
#include <QFontComboBox>
#include <QTextCharFormat>
#include <QTimer>
#include <QPoint>
#include <QPair>
#include <QColor>
#include <QMap>
#include <QLabel>
#include <QToolTip>
#include <QTextDocument>
#include <QTextCursor>
#include <QTextList>
#include <QTextBlockFormat>
#include <QTextFrame>

// 自定义文本编辑器，用于扩展QTextEdit功能
class NoteTextEdit : public QTextEdit 
{
    Q_OBJECT

public:
    explicit NoteTextEdit(QWidget *parent = nullptr);
    
    // 重写鼠标事件，用于处理自定义上下文菜单和工具栏定位
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    
    // 重写事件过滤器
    bool eventFilter(QObject *watched, QEvent *event) override;

signals:
    // 文本选择改变时发出信号，用于显示浮动工具栏
    void selectionChanged(const QPoint &pos, bool hasSelection);
    // 鼠标点击时发出信号
    void editorClicked(const QPoint &pos);

private:
    bool m_trackingMouse;
};

// 工具栏管理类
class FloatingToolBar : public QWidget
{
    Q_OBJECT

public:
    explicit FloatingToolBar(QWidget *parent = nullptr);
    
    void setTheme(bool isDarkTheme);
    void updatePosition(const QPoint &pos);
    void setFormatActions(const QTextCharFormat &format);
    
    // 获取工具栏中的各种按钮和控件
    QToolButton* boldButton() const { return m_boldButton; }
    QToolButton* italicButton() const { return m_italicButton; }
    QToolButton* underlineButton() const { return m_underlineButton; }
    QToolButton* strikeOutButton() const { return m_strikeOutButton; }
    QToolButton* textColorButton() const { return m_textColorButton; }
    QToolButton* highlightButton() const { return m_highlightButton; }
    QToolButton* alignLeftButton() const { return m_alignLeftButton; }
    QToolButton* alignCenterButton() const { return m_alignCenterButton; }
    QToolButton* alignRightButton() const { return m_alignRightButton; }
    QToolButton* alignJustifyButton() const { return m_alignJustifyButton; }
    
protected:
    void showEvent(QShowEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    
private:
    void setupUI();
    QIcon createColorIcon(const QColor &color);
    QIcon createColorToolButtonIcon(const QString &iconPath, const QColor &color);

    // 工具栏按钮
    QToolButton *m_boldButton;
    QToolButton *m_italicButton;
    QToolButton *m_underlineButton;
    QToolButton *m_strikeOutButton;
    QToolButton *m_textColorButton;
    QToolButton *m_highlightButton;
    QToolButton *m_alignLeftButton;
    QToolButton *m_alignCenterButton;
    QToolButton *m_alignRightButton;
    QToolButton *m_alignJustifyButton;

    QColor m_textColor;
    QColor m_highlightColor;
    bool m_isDarkTheme;
};

// 主编辑器管理类
class TextEditorManager : public QObject
{
    Q_OBJECT
    
public:
    explicit TextEditorManager(QWidget *parent = nullptr);
    ~TextEditorManager();
    
    // 获取编辑器控件，用于在主窗口中嵌入
    NoteTextEdit* textEdit() const { return m_textEdit; }
    
    // 获取顶部工具栏，用于在主窗口中嵌入
    QToolBar* topToolBar() const { return m_topToolBar; }
    
    // 主题切换
    void setTheme(bool isDarkTheme);
    
    // 内容管理
    void loadContent(const QString &content, const QString &path = QString());
    QString saveContent() const;
    
    // 格式化操作
    void bold();
    void italic();
    void underline();
    void strikeOut();
    void setTextColor(const QColor &color);
    void setHighlightColor(const QColor &color);
    void alignLeft();
    void alignCenter();
    void alignRight();
    void alignJustify();
    
    // 设置是否可编辑
    void setReadOnly(bool readOnly);
    
signals:
    // 内容修改信号
    void contentModified();
    
private slots:
    // 编辑器相关槽函数
    void handleSelectionChanged(const QPoint &pos, bool hasSelection);
    void updateToolBarForCurrentFormat();
    void handleTextEditClicked(const QPoint &pos);
    
    // 格式化操作槽函数
    void onBoldTriggered();
    void onItalicTriggered();
    void onUnderlineTriggered();
    void onStrikeOutTriggered();
    void onTextColorTriggered();
    void onHighlightTriggered();
    void onAlignLeftTriggered();
    void onAlignCenterTriggered();
    void onAlignRightTriggered();
    void onAlignJustifyTriggered();
    
    // 顶部工具栏槽函数
    void onSaveTriggered();
    void onUndoTriggered();
    void onRedoTriggered();
    void onCutTriggered();
    void onCopyTriggered();
    void onPasteTriggered();
    void onInsertImageTriggered();
    
private:
    // 获取当前光标的字符格式
    QTextCharFormat currentCharFormat() const;
    
    // 应用块格式（用于对齐等）
    void mergeFormatOnWordOrSelection(const QTextCharFormat &format);
    void setAlignment(Qt::Alignment alignment);
    
    // 创建顶部工具栏
    void setupTopToolBar();
    
    // 更新操作图标
    void updateActionIcons();
    
    // 创建颜色图标的辅助函数
    QIcon createColorIcon(const QString &path, const QColor &color, const QColor &disabledColor);
    
    // UI组件
    NoteTextEdit *m_textEdit;
    FloatingToolBar *m_floatingToolBar;
    QToolBar *m_topToolBar;
    QTimer *m_updateToolBarTimer;
    
    // 顶部工具栏操作
    QAction *m_saveAction;
    QAction *m_undoAction;
    QAction *m_redoAction;
    QAction *m_cutAction;
    QAction *m_copyAction;
    QAction *m_pasteAction;
    QAction *m_insertImageAction;
    
    // 状态变量
    bool m_isDarkTheme;
    QString m_currentFilePath;
    
    // 颜色配置
    QColor m_textColor;
    QColor m_highlightColor;
};

#endif // TEXTEDITORMANAGER_H 