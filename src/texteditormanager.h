/*
 * @Author: cursor AI
 * @Date: 2023-05-05 10:00:00
 * @LastEditors: Furdow wang22338014@gmail.com
 * @LastEditTime: 2025-05-13 16:58:12
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
#include <QDrag>
#include "fixedwidthfontcombo.h"
#include "aiassistantdialog.h"

class NoteTextEdit;
class FloatingToolBar;
class AiAssistantDialog; // 前向声明，因为 TextEditorManager 不再拥有它
class DatabaseManager;

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
    // 新增：重写键盘按下事件，用于处理删除配对括号的特殊情况
    void keyPressEvent(QKeyEvent *event) override;
    
signals:
    // 文本选择改变时发出信号，用于显示浮动工具栏
    void selectionChanged(const QPoint &pos, bool hasSelection);
    // 鼠标点击时发出信号
    void editorClicked(const QPoint &pos);
    // 图片调整大小完成时发出信号
    void imageResized();
    // 内容因为拖拽或调整大小被修改时发出信号
    void contentChangedByInteraction();

private:
    bool m_trackingMouse;
    // 图片选中和调整大小相关成员
    QTextCursor m_selectedImageCursor; // 当前选中的图片光标
    QRect m_selectedImageRect;       // 选中图片在 viewport 中的矩形 (可能不再需要实时更新，主要用于判断是否有选中)
    bool m_isResizing;               // 是否正在调整大小
    int m_currentHandle;             // 当前拖拽的手柄索引 (0-3: TL, TR, BL, BR)
    QPoint m_resizeStartPos;         // 开始调整大小时的鼠标位置
    QSize m_originalImageSize;       // 开始调整大小时图片的原始尺寸
    bool m_isMoving = false;               // 是否正在移动图片
    QPoint m_moveStartPos;           // 开始移动时的鼠标位置
    QTextImageFormat m_draggedImageFormat; // 拖拽的图片格式
    int m_dragStartPosition = -1;          // 拖拽开始位置
    bool m_manualDragging = false;         // 手动拖拽模式标志
    QLabel *m_dragPreviewLabel = nullptr;  // 拖拽预览标签
    bool m_justDeletedClosingPair = false; // 新增：标记是否刚刚删除了一个配对的右括号

    // 辅助函数
    void updateSelectionIndicator(); // 更新选中图片状态和矩形
    void drawSelectionIndicator(QPainter *painter); // 绘制选中框和手柄
    int getHandleAtPos(const QPoint &pos) const; // 获取鼠标位置处的手柄索引，-1表示没有
    void updateImageSize(const QPoint &mousePos); // 根据鼠标位置更新图片大小
    void setCursorForHandle(int handleIndex); // 根据手柄设置鼠标光标

    // 新增：静态配对表及其初始化函数
    static QMap<QString, QString> initPairMap();
    static const QMap<QString, QString> s_pairMap;

private slots:
    void handleTextChangedForAutoPair(); 
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
    QToolButton* aiAssistantButton() const { return m_aiAssistantButton; }
    FixedWidthFontCombo* fontComboBox() const { return m_fontComboBox; }
    QComboBox* fontSizeComboBox() const { return m_fontSizeComboBox; }
    QComboBox* headingComboBox() const { return m_headingComboBox; }
    
signals:
    // 添加字体、字号和标题选择的信号
    void fontFamilyChanged(const QString &family);
    void fontSizeChanged(const QString &size);
    void headingChanged(int level);
    
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
    QToolButton *m_aiAssistantButton;

    // 新增：字体、字号和标题选择控件
    FixedWidthFontCombo *m_fontComboBox;
    QComboBox *m_fontSizeComboBox;
    QComboBox *m_headingComboBox;

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
    
    // 标记内容已修改（用于外部调用）
    void markContentModified() { m_hasUnsavedChanges = true; m_saveAction->setEnabled(true); }
    
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

    // 新增：获取选中文本和插入文本方法
    QString getSelectedText() const; // 获取当前选中的文本
    void insertText(const QString &text); // 插入文本到当前光标位置
    
    // 工具栏访问
    QToolBar* topToolBar() const { return m_topToolBar; }
    FloatingToolBar* getFloatingToolBar() const { return m_floatingToolBar; }
    
    // AI助手相关方法
    void triggerAiAssistant(); // 主动触发AI助手对话框
    
    // 设置数据库管理器
    void setDatabaseManager(DatabaseManager *dbManager) { m_dbManager = dbManager; }
    DatabaseManager* getDatabaseManager() const { return m_dbManager; }
    
    // 应用编辑器设置
    void applyEditorSettings();
    void updateFont(const QString &family, int size);
    void updateTabWidth(int spaces);
    void updateAutoPairEnabled(bool enabled);

signals:
    void contentModified();  // 内容被修改信号（字符级别的修改）
    void contentChangedByInteraction(); // 内容被用户交互修改信号（如图片拖拽、调整大小等）
    void requestShowAiAssistant(const QString &selectedText); // 请求显示AI助手信号
    
public slots:
    void insertImageFromButton(); // 添加从按钮插入图片的槽函数
    
    // 设置相关槽函数 - 接收从设置对话框传来的设置变更
    void onEditorFontSettingChanged(const QString &fontFamily, int fontSize);
    void onTabWidthSettingChanged(int tabWidth);
    void onAutoPairSettingChanged(bool enabled);
    
private slots:
    void handleSelectionChanged(const QPoint &pos, bool hasSelection);
    void handleEditorClicked(const QPoint &pos);
    void handleTextEditClicked(const QPoint &pos);
    void handleCursorPositionChanged(); // 新增：处理光标位置变化的槽函数
    void documentModified();
    void updateToolBarForCurrentFormat();
    void setupFontComboBoxes();
    
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
    
    // 字体、字号和标题选择槽函数
    void onFontFamilyChanged(const QString &family);
    void onFontSizeChanged(const QString &size);
    void onHeadingChanged(int index);
    void applyHeading(int level);
    
    // AI助手槽函数
    void onShowAiAssistantActionTriggered();
    void insertAiContent(const QString &content);
    
private:
    // UI组件
    QWidget *m_editorContainer;
    NoteTextEdit *m_textEdit;
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
    
    // 新增：字体、字号和标题选择操作
    QAction *m_fontAction;
    FixedWidthFontCombo *m_fontComboBox;
    QComboBox *m_fontSizeComboBox;
    QComboBox *m_headingComboBox;
    
    // 状态变量
    QString m_currentNotePath;
    QString m_currentFilePath;
    bool m_hasUnsavedChanges;
    bool m_isDarkTheme;
    QColor m_textColor;
    QColor m_highlightColor;
    
    // AI助手相关成员
    QAction *m_showAiAssistantAction; // 显示AI助手的动作
    
    // 帮助函数
    void setupUI();
    void setupTopToolBar();
    void connectSignals();
    void updateToolBarPosition(const QPoint &pos);
    void updateActionIcons();
    QIcon createColorIcon(const QString &path, const QColor &color, const QColor &disabledColor);
    QTextCharFormat currentCharFormat() const;
    void mergeFormatOnWordOrSelection(const QTextCharFormat &format);
    void setEllipsisDisplayText(QComboBox *comboBox, const QString &fullText, int maxLength, int keepLength);

    // 数据库管理器指针
    DatabaseManager *m_dbManager = nullptr;
};

#endif // TEXTEDITORMANAGER_H 