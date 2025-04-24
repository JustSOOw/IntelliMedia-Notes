/*
 * @Author: cursor AI
 * @Date: 2023-05-05 10:00:00
 * @LastEditors: cursor AI
 * @LastEditTime: 2023-05-05 10:00:00
 * @FilePath: \IntelliMedia_Notes\src\texteditormanager.cpp
 * @Description: QTextEdit编辑器管理类实现
 * 
 * Copyright (c) 2023, All Rights Reserved. 
 */
#include "texteditormanager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QTextBlock>
#include <QDebug>
#include <QMimeData>
#include <QApplication>
#include <QClipboard>
#include <QScreen>
#include <QStyleOption>
#include <QPainter>
#include <QSvgRenderer>
#include <QFileInfo>
#include <QToolBar>
#include <QAction>
#include <QFileDialog>
#include <QTextStream>
#include <QDir>
#include <QDateTime>

//=======================================================================================
// NoteTextEdit 实现
//=======================================================================================
NoteTextEdit::NoteTextEdit(QWidget *parent)
    : QTextEdit(parent)
    , m_trackingMouse(false)
{
    setMouseTracking(true);
    installEventFilter(this);
    
    // 设置文档边距
    document()->setDocumentMargin(20);
    
    // 设置文本编辑器的属性
    setAcceptRichText(true);
    setAutoFormatting(QTextEdit::AutoAll);
    
    // 设置文本编辑器的外观
    setFrameStyle(QFrame::NoFrame);
    viewport()->setCursor(Qt::IBeamCursor);
}

void NoteTextEdit::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_trackingMouse = true;
        emit editorClicked(event->pos());
    }
    QTextEdit::mousePressEvent(event);
}

void NoteTextEdit::mouseReleaseEvent(QMouseEvent *event)
{
    m_trackingMouse = false;
    
    // 如果有文本选中，发送选中信号并传递位置
    bool hasSelection = textCursor().hasSelection();
    emit selectionChanged(event->pos(), hasSelection);
    
    QTextEdit::mouseReleaseEvent(event);
}

bool NoteTextEdit::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == this && event->type() == QEvent::MouseMove) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        
        if (m_trackingMouse && mouseEvent && mouseEvent->buttons() & Qt::LeftButton) {
            bool hasSelection = textCursor().hasSelection();
            if (hasSelection) {
                emit selectionChanged(mouseEvent->pos(), true);
            }
        }
    }
    
    return QTextEdit::eventFilter(watched, event);
}

//=======================================================================================
// FloatingToolBar 实现
//=======================================================================================
FloatingToolBar::FloatingToolBar(QWidget *parent)
    : QWidget(parent, Qt::ToolTip | Qt::FramelessWindowHint)
    , m_isDarkTheme(false)
    , m_textColor(Qt::black)
    , m_highlightColor(Qt::yellow)
{
    setAttribute(Qt::WA_ShowWithoutActivating);
    setMouseTracking(true);
    
    // 设置工具栏样式
    setStyleSheet("FloatingToolBar { border: 1px solid #cccccc; border-radius: 4px; background-color: #f5f5f5; }");
    
    setupUI();
    
    // 安装事件过滤器
    qApp->installEventFilter(this);
}

void FloatingToolBar::setupUI()
{
    // 创建布局
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(5, 5, 5, 5);
    layout->setSpacing(3);
    
    // 创建按钮
    m_boldButton = new QToolButton(this);
    m_italicButton = new QToolButton(this);
    m_underlineButton = new QToolButton(this);
    m_strikeOutButton = new QToolButton(this);
    m_textColorButton = new QToolButton(this);
    m_highlightButton = new QToolButton(this);
    m_alignLeftButton = new QToolButton(this);
    m_alignCenterButton = new QToolButton(this);
    m_alignRightButton = new QToolButton(this);
    m_alignJustifyButton = new QToolButton(this);
    
    // 设置图标
    m_boldButton->setIcon(QIcon(":/icons/editor/bold.svg"));
    m_italicButton->setIcon(QIcon(":/icons/editor/italic.svg"));
    m_underlineButton->setIcon(QIcon(":/icons/editor/underline.svg"));
    m_strikeOutButton->setIcon(QIcon(":/icons/editor/strikeout.svg"));
    m_textColorButton->setIcon(QIcon(":/icons/editor/text-color.svg"));
    m_highlightButton->setIcon(QIcon(":/icons/editor/highlight.svg"));
    m_alignLeftButton->setIcon(QIcon(":/icons/editor/align-left.svg"));
    m_alignCenterButton->setIcon(QIcon(":/icons/editor/align-center.svg"));
    m_alignRightButton->setIcon(QIcon(":/icons/editor/align-right.svg"));
    m_alignJustifyButton->setIcon(QIcon(":/icons/editor/align-justify.svg"));
    
    // 设置工具提示
    m_boldButton->setToolTip("粗体");
    m_italicButton->setToolTip("斜体");
    m_underlineButton->setToolTip("下划线");
    m_strikeOutButton->setToolTip("删除线");
    m_textColorButton->setToolTip("文本颜色");
    m_highlightButton->setToolTip("文本高亮");
    m_alignLeftButton->setToolTip("左对齐");
    m_alignCenterButton->setToolTip("居中对齐");
    m_alignRightButton->setToolTip("右对齐");
    m_alignJustifyButton->setToolTip("两端对齐");
    
    // 设置按钮可选中
    m_boldButton->setCheckable(true);
    m_italicButton->setCheckable(true);
    m_underlineButton->setCheckable(true);
    m_strikeOutButton->setCheckable(true);
    m_alignLeftButton->setCheckable(true);
    m_alignCenterButton->setCheckable(true);
    m_alignRightButton->setCheckable(true);
    m_alignJustifyButton->setCheckable(true);
    
    // 设置按钮大小
    QSize buttonSize(24, 24);
    m_boldButton->setIconSize(buttonSize);
    m_italicButton->setIconSize(buttonSize);
    m_underlineButton->setIconSize(buttonSize);
    m_strikeOutButton->setIconSize(buttonSize);
    m_textColorButton->setIconSize(buttonSize);
    m_highlightButton->setIconSize(buttonSize);
    m_alignLeftButton->setIconSize(buttonSize);
    m_alignCenterButton->setIconSize(buttonSize);
    m_alignRightButton->setIconSize(buttonSize);
    m_alignJustifyButton->setIconSize(buttonSize);
    
    // 设置按钮样式
    QString buttonStyle = "QToolButton { border: none; padding: 2px; border-radius: 4px; background-color: transparent; } "
                          "QToolButton:hover { background-color: #e0e0e0; } "
                          "QToolButton:checked { background-color: #c0c0c0; } "
                          "QToolButton:pressed { background-color: #b0b0b0; }";
    
    m_boldButton->setStyleSheet(buttonStyle);
    m_italicButton->setStyleSheet(buttonStyle);
    m_underlineButton->setStyleSheet(buttonStyle);
    m_strikeOutButton->setStyleSheet(buttonStyle);
    m_textColorButton->setStyleSheet(buttonStyle);
    m_highlightButton->setStyleSheet(buttonStyle);
    m_alignLeftButton->setStyleSheet(buttonStyle);
    m_alignCenterButton->setStyleSheet(buttonStyle);
    m_alignRightButton->setStyleSheet(buttonStyle);
    m_alignJustifyButton->setStyleSheet(buttonStyle);
    
    // 添加按钮到布局
    layout->addWidget(m_boldButton);
    layout->addWidget(m_italicButton);
    layout->addWidget(m_underlineButton);
    layout->addWidget(m_strikeOutButton);
    
    // 添加分隔符
    QFrame *separator1 = new QFrame(this);
    separator1->setFrameShape(QFrame::VLine);
    separator1->setFrameShadow(QFrame::Sunken);
    layout->addWidget(separator1);
    
    layout->addWidget(m_textColorButton);
    layout->addWidget(m_highlightButton);
    
    // 添加分隔符
    QFrame *separator2 = new QFrame(this);
    separator2->setFrameShape(QFrame::VLine);
    separator2->setFrameShadow(QFrame::Sunken);
    layout->addWidget(separator2);
    
    layout->addWidget(m_alignLeftButton);
    layout->addWidget(m_alignCenterButton);
    layout->addWidget(m_alignRightButton);
    layout->addWidget(m_alignJustifyButton);
    
    setLayout(layout);
}

void FloatingToolBar::setTheme(bool isDarkTheme)
{
    m_isDarkTheme = isDarkTheme;
    
    // 根据主题设置工具栏样式
    if (isDarkTheme) {
        setStyleSheet("FloatingToolBar { border: 1px solid #444444; border-radius: 4px; background-color: #333333; }");
        
        QString buttonStyle = "QToolButton { border: none; padding: 2px; border-radius: 4px; background-color: transparent; color: white; } "
                              "QToolButton:hover { background-color: #444444; } "
                              "QToolButton:checked { background-color: #555555; } "
                              "QToolButton:pressed { background-color: #666666; }";
        
        m_boldButton->setStyleSheet(buttonStyle);
        m_italicButton->setStyleSheet(buttonStyle);
        m_underlineButton->setStyleSheet(buttonStyle);
        m_strikeOutButton->setStyleSheet(buttonStyle);
        m_textColorButton->setStyleSheet(buttonStyle);
        m_highlightButton->setStyleSheet(buttonStyle);
        m_alignLeftButton->setStyleSheet(buttonStyle);
        m_alignCenterButton->setStyleSheet(buttonStyle);
        m_alignRightButton->setStyleSheet(buttonStyle);
        m_alignJustifyButton->setStyleSheet(buttonStyle);
    } else {
        setStyleSheet("FloatingToolBar { border: 1px solid #cccccc; border-radius: 4px; background-color: #f5f5f5; }");
        
        QString buttonStyle = "QToolButton { border: none; padding: 2px; border-radius: 4px; background-color: transparent; color: black; } "
                              "QToolButton:hover { background-color: #e0e0e0; } "
                              "QToolButton:checked { background-color: #c0c0c0; } "
                              "QToolButton:pressed { background-color: #b0b0b0; }";
        
        m_boldButton->setStyleSheet(buttonStyle);
        m_italicButton->setStyleSheet(buttonStyle);
        m_underlineButton->setStyleSheet(buttonStyle);
        m_strikeOutButton->setStyleSheet(buttonStyle);
        m_textColorButton->setStyleSheet(buttonStyle);
        m_highlightButton->setStyleSheet(buttonStyle);
        m_alignLeftButton->setStyleSheet(buttonStyle);
        m_alignCenterButton->setStyleSheet(buttonStyle);
        m_alignRightButton->setStyleSheet(buttonStyle);
        m_alignJustifyButton->setStyleSheet(buttonStyle);
    }
}

void FloatingToolBar::updatePosition(const QPoint &pos)
{
    // 获取编辑器中的选择位置，使工具栏显示在选择文本的上方
    QWidget *parentWidget = qobject_cast<QWidget*>(parent());
    if (!parentWidget) return;
    
    // 计算工具栏位置，使其显示在文本上方约20像素处
    QPoint globalPos = parentWidget->mapToGlobal(pos);
    globalPos.setY(globalPos.y() - height() - 5); // 在文本上方5个像素
    
    // 确保工具栏不会超出屏幕边界
    QRect screenGeometry = QGuiApplication::screenAt(globalPos)->geometry();
    
    // 调整X坐标，确保工具栏不会超出屏幕右侧
    if (globalPos.x() + width() > screenGeometry.right()) {
        globalPos.setX(screenGeometry.right() - width());
    }
    
    // 调整X坐标，确保工具栏不会超出屏幕左侧
    if (globalPos.x() < screenGeometry.left()) {
        globalPos.setX(screenGeometry.left());
    }
    
    // 如果工具栏会超出屏幕顶部，则显示在文本下方
    if (globalPos.y() < screenGeometry.top()) {
        globalPos.setY(pos.y() + 20); // 在文本下方20个像素
    }
    
    move(globalPos);
}

void FloatingToolBar::setFormatActions(const QTextCharFormat &format)
{
    // 根据当前字符格式更新按钮状态
    m_boldButton->setChecked(format.fontWeight() >= QFont::Bold);
    m_italicButton->setChecked(format.fontItalic());
    m_underlineButton->setChecked(format.fontUnderline());
    m_strikeOutButton->setChecked(format.fontStrikeOut());
    
    // 更新文本颜色和高亮颜色
    if (format.hasProperty(QTextFormat::ForegroundBrush)) {
        m_textColor = format.foreground().color();
    }
    
    if (format.hasProperty(QTextFormat::BackgroundBrush)) {
        m_highlightColor = format.background().color();
    }
}

QIcon FloatingToolBar::createColorIcon(const QColor &color)
{
    QPixmap pixmap(16, 16);
    pixmap.fill(color);
    return QIcon(pixmap);
}

QIcon FloatingToolBar::createColorToolButtonIcon(const QString &iconPath, const QColor &color)
{
    // 加载SVG
    QSvgRenderer renderer(iconPath);
    if (!renderer.isValid()) {
        qWarning() << "无效的SVG文件:" << iconPath;
        return QIcon();
    }
    
    // 创建像素图
    QSize iconSize(24, 24);
    QPixmap pixmap(iconSize);
    pixmap.fill(Qt::transparent);
    
    // 在像素图上绘制SVG
    QPainter painter(&pixmap);
    
    // 将SVG渲染到像素图
    renderer.render(&painter);
    
    // 根据颜色创建彩色小方块
    QRect colorRect(12, 12, 8, 8);
    painter.fillRect(colorRect, color);
    painter.setPen(m_isDarkTheme ? Qt::white : Qt::black);
    painter.drawRect(colorRect);
    
    painter.end();
    
    return QIcon(pixmap);
}

void FloatingToolBar::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    
    // 确保工具栏有焦点
    setFocus();
    raise();
}

bool FloatingToolBar::eventFilter(QObject *watched, QEvent *event)
{
    // 监听鼠标点击事件，当点击工具栏外部区域时隐藏工具栏
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        if (!geometry().contains(mouseEvent->globalPos()) && isVisible()) {
            // 如果点击事件不在工具栏内且工具栏可见，隐藏工具栏
            hide();
        }
    }
    
    return QWidget::eventFilter(watched, event);
}

//=======================================================================================
// TextEditorManager 实现
//=======================================================================================
TextEditorManager::TextEditorManager(QWidget *parent)
    : QObject(parent)
    , m_isDarkTheme(false)  // 默认使用亮色主题
    , m_textColor(Qt::black)
    , m_highlightColor(Qt::yellow)
{
    // 创建文本编辑器
    m_textEdit = new NoteTextEdit(qobject_cast<QWidget*>(parent));
    
    // 创建浮动工具栏
    m_floatingToolBar = new FloatingToolBar(m_textEdit->viewport());
    m_floatingToolBar->hide();
    
    // 创建顶部工具栏
    m_topToolBar = new QToolBar(qobject_cast<QWidget*>(parent));
    setupTopToolBar();
    
    // 初始化更新工具栏计时器
    m_updateToolBarTimer = new QTimer(this);
    m_updateToolBarTimer->setSingleShot(true);
    m_updateToolBarTimer->setInterval(50);
    
    // 连接信号和槽
    connect(m_textEdit, &NoteTextEdit::selectionChanged, this, &TextEditorManager::handleSelectionChanged);
    connect(m_textEdit, &NoteTextEdit::editorClicked, this, &TextEditorManager::handleTextEditClicked);
    connect(m_textEdit, &QTextEdit::textChanged, this, &TextEditorManager::contentModified);
    connect(m_updateToolBarTimer, &QTimer::timeout, this, &TextEditorManager::updateToolBarForCurrentFormat);
    
    // 连接工具栏按钮的信号
    connect(m_floatingToolBar->boldButton(), &QToolButton::clicked, this, &TextEditorManager::onBoldTriggered);
    connect(m_floatingToolBar->italicButton(), &QToolButton::clicked, this, &TextEditorManager::onItalicTriggered);
    connect(m_floatingToolBar->underlineButton(), &QToolButton::clicked, this, &TextEditorManager::onUnderlineTriggered);
    connect(m_floatingToolBar->strikeOutButton(), &QToolButton::clicked, this, &TextEditorManager::onStrikeOutTriggered);
    connect(m_floatingToolBar->textColorButton(), &QToolButton::clicked, this, &TextEditorManager::onTextColorTriggered);
    connect(m_floatingToolBar->highlightButton(), &QToolButton::clicked, this, &TextEditorManager::onHighlightTriggered);
    connect(m_floatingToolBar->alignLeftButton(), &QToolButton::clicked, this, &TextEditorManager::onAlignLeftTriggered);
    connect(m_floatingToolBar->alignCenterButton(), &QToolButton::clicked, this, &TextEditorManager::onAlignCenterTriggered);
    connect(m_floatingToolBar->alignRightButton(), &QToolButton::clicked, this, &TextEditorManager::onAlignRightTriggered);
    connect(m_floatingToolBar->alignJustifyButton(), &QToolButton::clicked, this, &TextEditorManager::onAlignJustifyTriggered);
    
    // 设置默认亮色主题
    setTheme(m_isDarkTheme);
}

TextEditorManager::~TextEditorManager()
{
    // 父对象将负责删除m_textEdit和m_floatingToolBar
}

void TextEditorManager::setupTopToolBar()
{
    // 设置工具栏属性
    m_topToolBar->setMovable(false);
    m_topToolBar->setFloatable(false);
    m_topToolBar->setIconSize(QSize(20, 20));
    
    // 创建操作，使用自定义图标
    m_saveAction = m_topToolBar->addAction(QIcon(":/icons/editor/save.svg"), "保存");
    m_topToolBar->addSeparator();
    
    m_undoAction = m_topToolBar->addAction(QIcon(":/icons/editor/undo.svg"), "撤销");
    m_redoAction = m_topToolBar->addAction(QIcon(":/icons/editor/redo.svg"), "重做");
    m_topToolBar->addSeparator();
    
    m_cutAction = m_topToolBar->addAction(QIcon(":/icons/editor/cut.svg"), "剪切");
    m_copyAction = m_topToolBar->addAction(QIcon(":/icons/editor/copy.svg"), "复制");
    m_pasteAction = m_topToolBar->addAction(QIcon(":/icons/editor/paste.svg"), "粘贴");
    m_topToolBar->addSeparator();
    
    m_insertImageAction = m_topToolBar->addAction(QIcon(":/icons/editor/image.svg"), "插入图片");
    
    // 设置工具提示
    m_saveAction->setToolTip("保存笔记 (Ctrl+S)");
    m_undoAction->setToolTip("撤销上一步操作 (Ctrl+Z)");
    m_redoAction->setToolTip("重做上一步操作 (Ctrl+Y)");
    m_cutAction->setToolTip("剪切选中文本 (Ctrl+X)");
    m_copyAction->setToolTip("复制选中文本 (Ctrl+C)");
    m_pasteAction->setToolTip("粘贴文本 (Ctrl+V)");
    m_insertImageAction->setToolTip("从文件插入图片");
    
    // 连接信号
    connect(m_saveAction, &QAction::triggered, this, &TextEditorManager::onSaveTriggered);
    connect(m_undoAction, &QAction::triggered, this, &TextEditorManager::onUndoTriggered);
    connect(m_redoAction, &QAction::triggered, this, &TextEditorManager::onRedoTriggered);
    connect(m_cutAction, &QAction::triggered, this, &TextEditorManager::onCutTriggered);
    connect(m_copyAction, &QAction::triggered, this, &TextEditorManager::onCopyTriggered);
    connect(m_pasteAction, &QAction::triggered, this, &TextEditorManager::onPasteTriggered);
    connect(m_insertImageAction, &QAction::triggered, this, &TextEditorManager::onInsertImageTriggered);
    
    // 绑定编辑器的撤销/重做状态
    connect(m_textEdit->document(), &QTextDocument::undoAvailable, m_undoAction, &QAction::setEnabled);
    connect(m_textEdit->document(), &QTextDocument::redoAvailable, m_redoAction, &QAction::setEnabled);
    
    // 初始状态设置
    m_undoAction->setEnabled(false);
    m_redoAction->setEnabled(false);
    
    // 设置工具栏的外观
    m_topToolBar->setStyleSheet("QToolBar { border: none; background-color: transparent; }");
}

void TextEditorManager::setTheme(bool isDarkTheme)
{
    m_isDarkTheme = isDarkTheme;
    
    // 设置浮动工具栏主题
    m_floatingToolBar->setTheme(isDarkTheme);
    
    // 设置文本编辑器主题 - 移除样式表设置，让主窗口的QSS处理
    // 注意：这里只保留最基本的设置，其他样式将由全局QSS处理
    m_textEdit->setStyleSheet("");
    
    // 更新按钮图标颜色
    updateActionIcons();
}

void TextEditorManager::updateActionIcons()
{
    QColor iconColor = m_isDarkTheme ? Qt::white : QColor(50, 50, 50);
    QColor disabledColor = m_isDarkTheme ? QColor(120, 120, 120) : QColor(180, 180, 180);
    
    // 更新保存按钮图标
    QIcon saveIcon = createColorIcon(":/icons/editor/save.svg", iconColor, disabledColor);
    m_saveAction->setIcon(saveIcon);
    
    // 更新撤销按钮图标
    QIcon undoIcon = createColorIcon(":/icons/editor/undo.svg", iconColor, disabledColor);
    m_undoAction->setIcon(undoIcon);
    
    // 更新重做按钮图标
    QIcon redoIcon = createColorIcon(":/icons/editor/redo.svg", iconColor, disabledColor);
    m_redoAction->setIcon(redoIcon);
    
    // 更新剪切按钮图标
    QIcon cutIcon = createColorIcon(":/icons/editor/cut.svg", iconColor, disabledColor);
    m_cutAction->setIcon(cutIcon);
    
    // 更新复制按钮图标
    QIcon copyIcon = createColorIcon(":/icons/editor/copy.svg", iconColor, disabledColor);
    m_copyAction->setIcon(copyIcon);
    
    // 更新粘贴按钮图标
    QIcon pasteIcon = createColorIcon(":/icons/editor/paste.svg", iconColor, disabledColor);
    m_pasteAction->setIcon(pasteIcon);
    
    // 更新插入图片按钮图标
    QIcon imageIcon = createColorIcon(":/icons/editor/image.svg", iconColor, disabledColor);
    m_insertImageAction->setIcon(imageIcon);
}

QIcon TextEditorManager::createColorIcon(const QString &path, const QColor &color, const QColor &disabledColor)
{
    QIcon icon;
    
    // 加载SVG
    QSvgRenderer renderer(path);
    if (!renderer.isValid()) {
        qWarning() << "无效的SVG文件:" << path;
        return QIcon();
    }
    
    // 创建正常状态的图标
    QSize iconSize(24, 24);
    QPixmap normalPixmap(iconSize);
    normalPixmap.fill(Qt::transparent);
    
    QPainter normalPainter(&normalPixmap);
    
    // 在像素图上绘制SVG，替换颜色
    QSvgRenderer normalRenderer(path);
    
    // 创建与SVG文件内容相同的副本，但将颜色替换为当前主题颜色
    QByteArray svgContent;
    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) {
        svgContent = file.readAll();
        svgContent.replace("currentColor", color.name().toLatin1());
        file.close();
    }
    
    QSvgRenderer coloredRenderer(svgContent);
    if (coloredRenderer.isValid()) {
        coloredRenderer.render(&normalPainter);
    } else {
        // 如果颜色替换失败，使用原始SVG
        normalRenderer.render(&normalPainter);
    }
    normalPainter.end();
    
    // 创建禁用状态的图标
    QPixmap disabledPixmap(iconSize);
    disabledPixmap.fill(Qt::transparent);
    
    QPainter disabledPainter(&disabledPixmap);
    
    // 创建禁用状态的SVG内容
    QByteArray disabledSvgContent = svgContent;
    disabledSvgContent.replace(color.name().toLatin1(), disabledColor.name().toLatin1());
    
    QSvgRenderer disabledRenderer(disabledSvgContent);
    if (disabledRenderer.isValid()) {
        disabledRenderer.render(&disabledPainter);
    } else {
        // 如果颜色替换失败，使用原始SVG
        normalRenderer.render(&disabledPainter);
    }
    disabledPainter.end();
    
    // 添加到图标
    icon.addPixmap(normalPixmap, QIcon::Normal, QIcon::Off);
    icon.addPixmap(disabledPixmap, QIcon::Disabled, QIcon::Off);
    
    return icon;
}

void TextEditorManager::loadContent(const QString &content, const QString &path)
{
    m_currentFilePath = path;
    
    // 加载HTML内容到编辑器
    m_textEdit->setHtml(content);
    
    // 重置编辑器的修改状态
    m_textEdit->document()->setModified(false);
}

QString TextEditorManager::saveContent() const
{
    // 返回编辑器的HTML内容
    return m_textEdit->toHtml();
}

void TextEditorManager::handleSelectionChanged(const QPoint &pos, bool hasSelection)
{
    if (hasSelection) {
        // 如果有文本选中，更新工具栏位置并显示
        m_floatingToolBar->updatePosition(pos);
        m_floatingToolBar->show();
        
        // 启动更新计时器，避免频繁更新
        m_updateToolBarTimer->start();
    } else {
        // 如果没有文本选中，隐藏工具栏
        m_floatingToolBar->hide();
    }
}

void TextEditorManager::handleTextEditClicked(const QPoint &pos)
{
    // 检查是否有文本选中
    bool hasSelection = m_textEdit->textCursor().hasSelection();
    
    if (!hasSelection) {
        // 如果没有文本选中，隐藏工具栏
        m_floatingToolBar->hide();
    }
}

void TextEditorManager::updateToolBarForCurrentFormat()
{
    // 获取当前光标的字符格式
    QTextCharFormat format = currentCharFormat();
    
    // 更新工具栏按钮状态
    m_floatingToolBar->setFormatActions(format);
    
    // 设置对齐按钮状态
    QTextBlockFormat blockFormat = m_textEdit->textCursor().blockFormat();
    Qt::Alignment alignment = blockFormat.alignment();
    
    m_floatingToolBar->alignLeftButton()->setChecked(alignment == Qt::AlignLeft);
    m_floatingToolBar->alignCenterButton()->setChecked(alignment == Qt::AlignCenter);
    m_floatingToolBar->alignRightButton()->setChecked(alignment == Qt::AlignRight);
    m_floatingToolBar->alignJustifyButton()->setChecked(alignment == Qt::AlignJustify);
}

QTextCharFormat TextEditorManager::currentCharFormat() const
{
    return m_textEdit->textCursor().charFormat();
}

void TextEditorManager::mergeFormatOnWordOrSelection(const QTextCharFormat &format)
{
    QTextCursor cursor = m_textEdit->textCursor();
    if (!cursor.hasSelection()) {
        cursor.select(QTextCursor::WordUnderCursor);
    }
    cursor.mergeCharFormat(format);
    m_textEdit->mergeCurrentCharFormat(format);
}

void TextEditorManager::setAlignment(Qt::Alignment alignment)
{
    m_textEdit->setAlignment(alignment);
}

// 格式化操作
void TextEditorManager::bold()
{
    QTextCharFormat fmt;
    fmt.setFontWeight(m_floatingToolBar->boldButton()->isChecked() ? QFont::Bold : QFont::Normal);
    mergeFormatOnWordOrSelection(fmt);
}

void TextEditorManager::italic()
{
    QTextCharFormat fmt;
    fmt.setFontItalic(m_floatingToolBar->italicButton()->isChecked());
    mergeFormatOnWordOrSelection(fmt);
}

void TextEditorManager::underline()
{
    QTextCharFormat fmt;
    fmt.setFontUnderline(m_floatingToolBar->underlineButton()->isChecked());
    mergeFormatOnWordOrSelection(fmt);
}

void TextEditorManager::strikeOut()
{
    QTextCharFormat fmt;
    fmt.setFontStrikeOut(m_floatingToolBar->strikeOutButton()->isChecked());
    mergeFormatOnWordOrSelection(fmt);
}

void TextEditorManager::setTextColor(const QColor &color)
{
    m_textColor = color;
    QTextCharFormat fmt;
    fmt.setForeground(QBrush(color));
    mergeFormatOnWordOrSelection(fmt);
}

void TextEditorManager::setHighlightColor(const QColor &color)
{
    m_highlightColor = color;
    QTextCharFormat fmt;
    fmt.setBackground(QBrush(color));
    mergeFormatOnWordOrSelection(fmt);
}

void TextEditorManager::alignLeft()
{
    setAlignment(Qt::AlignLeft);
}

void TextEditorManager::alignCenter()
{
    setAlignment(Qt::AlignCenter);
}

void TextEditorManager::alignRight()
{
    setAlignment(Qt::AlignRight);
}

void TextEditorManager::alignJustify()
{
    setAlignment(Qt::AlignJustify);
}

// 工具栏按钮点击槽函数
void TextEditorManager::onBoldTriggered()
{
    bold();
}

void TextEditorManager::onItalicTriggered()
{
    italic();
}

void TextEditorManager::onUnderlineTriggered()
{
    underline();
}

void TextEditorManager::onStrikeOutTriggered()
{
    strikeOut();
}

void TextEditorManager::onTextColorTriggered()
{
    QColor color = QColorDialog::getColor(m_textColor, m_textEdit, "选择文本颜色");
    if (color.isValid()) {
        setTextColor(color);
    }
}

void TextEditorManager::onHighlightTriggered()
{
    QColor color = QColorDialog::getColor(m_highlightColor, m_textEdit, "选择高亮颜色");
    if (color.isValid()) {
        setHighlightColor(color);
    }
}

void TextEditorManager::onAlignLeftTriggered()
{
    alignLeft();
    updateToolBarForCurrentFormat();
}

void TextEditorManager::onAlignCenterTriggered()
{
    alignCenter();
    updateToolBarForCurrentFormat();
}

void TextEditorManager::onAlignRightTriggered()
{
    alignRight();
    updateToolBarForCurrentFormat();
}

void TextEditorManager::onAlignJustifyTriggered()
{
    alignJustify();
    updateToolBarForCurrentFormat();
}

// 顶部工具栏槽函数
void TextEditorManager::onSaveTriggered()
{
    QString content = saveContent();
    emit contentModified();
    qDebug() << "保存笔记：" << m_currentFilePath;
    
    // 以下是一个简单的文件保存测试，用于开发阶段
    if (!m_currentFilePath.isEmpty()) {
        QFile file(m_currentFilePath + ".html");
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream stream(&file);
            stream << content;
            file.close();
            qDebug() << "文件已保存到：" << m_currentFilePath + ".html";
        } else {
            qDebug() << "无法保存文件：" << file.errorString();
        }
    } else {
        // 如果没有当前文件路径，创建一个临时文件
        QString tempPath = QDir::tempPath() + "/temp_note_" + 
                          QString::number(QDateTime::currentMSecsSinceEpoch()) + ".html";
        QFile file(tempPath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream stream(&file);
            stream << content;
            file.close();
            qDebug() << "文件已保存到临时位置：" << tempPath;
        }
    }
}

void TextEditorManager::onUndoTriggered()
{
    m_textEdit->undo();
}

void TextEditorManager::onRedoTriggered()
{
    m_textEdit->redo();
}

void TextEditorManager::onCutTriggered()
{
    m_textEdit->cut();
}

void TextEditorManager::onCopyTriggered()
{
    m_textEdit->copy();
}

void TextEditorManager::onPasteTriggered()
{
    m_textEdit->paste();
}

void TextEditorManager::onInsertImageTriggered()
{
    QString filePath = QFileDialog::getOpenFileName(
        m_textEdit, "选择图片",
        QString(), "图像文件 (*.png *.jpg *.jpeg *.bmp *.gif)");
    
    if (!filePath.isEmpty()) {
        QImage image(filePath);
        if (!image.isNull()) {
            QTextCursor cursor = m_textEdit->textCursor();
            cursor.insertImage(image);
        }
    }
}

void TextEditorManager::setReadOnly(bool readOnly)
{
    m_textEdit->setReadOnly(readOnly);
    
    // 禁用或启用相关操作
    m_saveAction->setEnabled(!readOnly);
    m_cutAction->setEnabled(!readOnly);
    m_pasteAction->setEnabled(!readOnly);
    m_insertImageAction->setEnabled(!readOnly);
} 