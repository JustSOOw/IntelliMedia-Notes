/*
 * @Author: cursor AI
 * @Date: 2023-05-05 10:00:00
 * @LastEditors: Furdow wang22338014@gmail.com
 * @LastEditTime: 2025-05-01 18:46:06
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
#include <QMenu>
#include <QContextMenuEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QFileDialog>
#include <QUrl>
#include <QImage>
#include <QUuid>
#include <QDir>
#include <QVBoxLayout>
#include <QPushButton>
#include <QDialog>
#include <QStringList>
#include <QList>
#include <QResizeEvent>
#include <QCheckBox>
#include <QGroupBox>
#include <QPainter>
#include <QPaintEvent>

class NoteTextEdit;
class FloatingToolBar;

// 自定义文本编辑器，用于扩展QTextEdit功能
class NoteTextEdit : public QTextEdit 
{
    Q_OBJECT

public:
    explicit NoteTextEdit(QWidget *parent = nullptr);
    
    // 重写鼠标事件，用于处理自定义上下文菜单和工具栏定位
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override; // 处理图片双击标注
    void mouseMoveEvent(QMouseEvent *event) override; // 添加 mouseMoveEvent
    
    // 处理拖放事件
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    
    // 重写事件过滤器
    bool eventFilter(QObject *watched, QEvent *event) override;
    
    // 图片插入和处理
    bool insertImageFromFile(const QString &filePath, int maxWidth = 600);
    QString saveImageToMediaFolder(const QString &sourceFilePath);
    QString getImageAtCursor(); // 获取光标处的图片路径
    
    // 调整图片大小和位置
    bool resizeImage(const QString &imagePath, int width, int height, Qt::Alignment alignment = Qt::AlignCenter);
    
protected:
    // 重写上下文菜单事件
    void contextMenuEvent(QContextMenuEvent *event) override;
    // 重写绘制事件，用于绘制选中框和手柄
    void paintEvent(QPaintEvent *event) override;
    // 重写滚动事件，用于同步选中框位置
    void scrollContentsBy(int dx, int dy) override;
    
signals:
    // 文本选择改变时发出信号，用于显示浮动工具栏
    void selectionChanged(const QPoint &pos, bool hasSelection);
    // 鼠标点击时发出信号
    void editorClicked(const QPoint &pos);
    // 图片调整大小完成时发出信号
    void imageResized();

private:
    bool m_trackingMouse;
    // 图片选中和调整大小相关成员
    QTextCursor m_selectedImageCursor; // 当前选中的图片光标
    QRect m_selectedImageRect;       // 选中图片在 viewport 中的矩形 (可能不再需要实时更新，主要用于判断是否有选中)
    bool m_isResizing;               // 是否正在调整大小
    int m_currentHandle;             // 当前拖拽的手柄索引 (0-3: TL, TR, BL, BR)
    QPoint m_resizeStartPos;         // 开始调整大小时的鼠标位置
    QSize m_originalImageSize;       // 开始调整大小时图片的原始尺寸
    bool m_isMoving;                 // 是否正在移动图片
    QPoint m_moveStartPos;           // 开始移动时的鼠标位置
    QTextImageFormat m_draggedImageFormat; // 用于内部拖动时临时存储图片格式
    int m_dragStartPosition = -1;      // 用于内部拖动时临时存储起始位置


    // 辅助函数
    void updateSelectionIndicator(); // 更新选中图片状态和矩形
    void drawSelectionIndicator(QPainter *painter); // 绘制选中框和手柄
    int getHandleAtPos(const QPoint &pos) const; // 获取鼠标位置处的手柄索引，-1表示没有
    void updateImageSize(const QPoint &mousePos); // 根据鼠标位置更新图片大小
    void setCursorForHandle(int handleIndex); // 根据手柄设置鼠标光标
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

// 文本编辑器管理器，负责管理编辑器和工具栏
class TextEditorManager : public QObject
{
    Q_OBJECT
    
public:
    explicit TextEditorManager(QWidget *parentWidget = nullptr);
    ~TextEditorManager();
    
    QWidget* getEditorWidget() const { return m_editorContainer; }
    NoteTextEdit* editor() const { return m_textEdit; }
    QTextEdit* textEdit() const { return m_textEdit; }
    
    // 笔记加载与保存
    void loadNote(const QString &notePath);
    void loadContent(const QString &content, const QString &path = "");
    void saveNote();
    QString saveContent() const;
    QString currentNotePath() const { return m_currentNotePath; }
    bool hasUnsavedChanges() const { return m_hasUnsavedChanges; }
    
    // 图片处理
    void showImageAnnotationDialog(const QString &imagePath);
    
    // 主题设置
    void setDarkTheme(bool dark);
    void setTheme(bool isDarkTheme); // 兼容性方法，内部调用setDarkTheme
    
    // 格式化操作
    void setAlignment(Qt::Alignment alignment);
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
    void setReadOnly(bool readOnly);
    
    // 工具栏访问
    QToolBar* topToolBar() const { return m_topToolBar; }
    
signals:
    // 通知笔记内容已修改
    void contentModified();
    
public slots:
    void insertImageFromButton(); // 添加从按钮插入图片的槽函数
    
private slots:
    void handleSelectionChanged(const QPoint &pos, bool hasSelection);
    void handleEditorClicked(const QPoint &pos);
    void handleTextEditClicked(const QPoint &pos);
    void documentModified();
    void updateToolBarForCurrentFormat();
    
    // 工具栏按钮槽函数
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
    void onSaveTriggered();
    void onUndoTriggered();
    void onRedoTriggered();
    void onCutTriggered();
    void onCopyTriggered();
    void onPasteTriggered();
    void onInsertImageTriggered();
    
private:
    // UI组件
    QWidget *m_editorContainer;
    NoteTextEdit *m_textEdit;  // 直接使用m_textEdit
    FloatingToolBar *m_floatingToolBar;
    QToolBar *m_topToolBar;
    QTimer *m_updateToolBarTimer;
    
    // 工具栏操作
    QAction *m_saveAction;
    QAction *m_undoAction;
    QAction *m_redoAction;
    QAction *m_cutAction;
    QAction *m_copyAction;
    QAction *m_pasteAction;
    QAction *m_insertImageAction;
    
    // 状态变量
    QString m_currentNotePath;
    QString m_currentFilePath;
    bool m_hasUnsavedChanges;
    bool m_isDarkTheme;
    QColor m_textColor;
    QColor m_highlightColor;
    
    // 帮助函数
    void setupUI();
    void setupTopToolBar();
    void connectSignals();
    void updateToolBarPosition(const QPoint &pos);
    void updateActionIcons();
    QIcon createColorIcon(const QString &path, const QColor &color, const QColor &disabledColor);
    QTextCharFormat currentCharFormat() const;
    void mergeFormatOnWordOrSelection(const QTextCharFormat &format);
};

#endif // TEXTEDITORMANAGER_H 