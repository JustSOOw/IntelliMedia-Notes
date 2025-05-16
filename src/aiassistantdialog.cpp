/*
 * @Author: cursor AI
 * @Date: 2025-05-12 10:00:00
 * @LastEditors: Furdow wang22338014@gmail.com
 * @LastEditTime: 2025-05-06 21:31:55
 * @FilePath: \IntelliMedia_Notes\src\aiassistantdialog.cpp
 * @Description: AI助手对话框实现
 * 
 * Copyright (c) 2025, All Rights Reserved. 
 */
#include "aiassistantdialog.h"
#include "IAiService.h"
#include "DeepSeekService.h"
#include <QShowEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QDebug>
#include <QApplication>
#include <QScreen>
#include <QStyleOption>
#include <QPainter>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <QFile>
#include <QMessageBox>

// 构造函数
AiAssistantDialog::AiAssistantDialog(QWidget *parent)
    : QDialog(parent),
      m_inputLineEdit(nullptr),
      m_responseTextEdit(nullptr),
      m_functionButton(nullptr),
      m_functionMenu(nullptr),
      m_toneSubMenu(nullptr),
      m_sendButton(nullptr),
      m_insertButton(nullptr),
      m_retryButton(nullptr),
      m_cancelButton(nullptr),
      m_titleLabel(nullptr),
      m_selectedText(""),
      m_generatedContent(""),
      m_isDarkTheme(false),
      m_dragging(false),
      m_currentFunction("自由对话"), // 默认功能改为自由对话
      m_currentTone("专业"), // 默认语气
      m_aiService(nullptr), // 初始无AI服务
      m_lastErrorType(""),
      m_animationTimer(nullptr), // 初始化定时器指针
      m_animationDotIndex(0),    // 初始化点索引
      m_floatingToolBar(nullptr) // 初始化悬浮工具栏
{
    // 设置窗口属性
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setModal(true); // 模态对话框，阻止与其他窗口交互
    
    // 设置固定大小
    setFixedSize(500, 400);
    
    // 加载AI功能配置
    loadConfigurations();
    
    // 设置UI
    setupUI();
    
    // 应用初始样式
    updateStyles();
}

// 析构函数
AiAssistantDialog::~AiAssistantDialog()
{
    qDebug() << "AiAssistantDialog(" << this << ") is being destroyed.";
    // 确保定时器被停止和删除 (如果它是动态创建的且没有父对象)
    // 如果定时器有父对象(this)，理论上不需要手动删除，但停止是好的实践
    if (m_animationTimer && m_animationTimer->isActive()) {
        m_animationTimer->stop();
    }
    // 如果定时器没有父对象或需要立即清理，则：delete m_animationTimer;
    // Qt会自动删除子对象，不需要手动删除UI组件
}

// 设置主题
void AiAssistantDialog::setDarkTheme(bool dark)
{
    if (m_isDarkTheme != dark) {
        m_isDarkTheme = dark;
        updateStyles();
    }
}

// 显示事件处理
void AiAssistantDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    
    // 注释掉以下代码，使用外部设置的位置
    /*
    // 居中显示在父窗口
    if (parentWidget()) {
        QRect parentGeometry = parentWidget()->geometry();
        move(parentGeometry.center() - rect().center());
    } else {
        // 如果没有父窗口，则居中显示在屏幕上
        QScreen *screen = QGuiApplication::primaryScreen();
        if (screen) {
            QRect screenGeometry = screen->availableGeometry();
            move(screenGeometry.center() - rect().center());
        }
    }
    */
    
    // 清空输入框并设置焦点
    if (m_inputLineEdit) {
        m_inputLineEdit->clear();
        m_inputLineEdit->setFocus();
    }
    
    // 清空响应区域
    if (m_responseTextEdit) {
        m_responseTextEdit->clear();
    }
    
    // 隐藏悬浮工具栏，避免遮挡
    if (m_floatingToolBar && m_floatingToolBar->isVisible()) {
        m_floatingToolBar->hide();
    }
    
    // 禁用重试按钮
    if(m_retryButton) {
        m_retryButton->setEnabled(false);
    }
}

// 按键事件处理
void AiAssistantDialog::keyPressEvent(QKeyEvent *event)
{
    // 处理Esc键关闭对话框
    if (event->key() == Qt::Key_Escape) {
        close();
        event->accept();
    } else {
        QDialog::keyPressEvent(event);
    }
}

// 鼠标按下事件处理（用于拖动无边框窗口）
void AiAssistantDialog::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
    }
    QDialog::mousePressEvent(event);
}

// 鼠标移动事件处理（用于拖动无边框窗口）
void AiAssistantDialog::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton && m_dragging) {
        move(event->globalPosition().toPoint() - m_dragPosition);
        event->accept();
    }
    QDialog::mouseMoveEvent(event);
}

// 鼠标释放事件处理（用于拖动无边框窗口）
void AiAssistantDialog::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        event->accept();
    }
    QDialog::mouseReleaseEvent(event);
}

// 加载AI功能配置
void AiAssistantDialog::loadConfigurations()
{
    // 这里将来会从配置文件加载，现在先硬编码一些默认配置
    m_aiConfig = QJsonObject();
    
    // 功能列表
    QJsonArray functions = QJsonArray();
    
    // 添加"继续写作"功能
    QJsonObject continueWriting;
    continueWriting["name"] = "继续写作";
    continueWriting["prompt"] = "请继续完成下面的文本，保持风格一致：";
    functions.append(continueWriting);
    
    // 添加"内容润色"功能
    QJsonObject polish;
    polish["name"] = "内容润色";
    polish["prompt"] = "请对以下文本进行润色，提高其表达质量和专业度：";
    functions.append(polish);
    
    // 添加"语法检查"功能
    QJsonObject grammarCheck;
    grammarCheck["name"] = "语法检查";
    grammarCheck["prompt"] = "请检查以下文本中的语法错误并改正：";
    functions.append(grammarCheck);
    
    // 添加"重新组织"功能
    QJsonObject reorganize;
    reorganize["name"] = "重新组织";
    reorganize["prompt"] = "请重新组织以下文本，使其结构更加清晰：";
    functions.append(reorganize);
    
    // 将功能列表添加到配置中
    m_aiConfig["functions"] = functions;
    
    // 语气列表
    QJsonArray tones = QJsonArray();
    
    // 添加各种语气
    QJsonObject professional;
    professional["name"] = "专业";
    professional["prompt"] = "请使用专业、正式的语气";
    tones.append(professional);
    
    QJsonObject casual;
    casual["name"] = "日常";
    casual["prompt"] = "请使用轻松、日常的语气";
    tones.append(casual);
    
    QJsonObject creative;
    creative["name"] = "创意";
    creative["prompt"] = "请使用富有创意和想象力的语气";
    tones.append(creative);
    
    QJsonObject concise;
    concise["name"] = "简洁";
    concise["prompt"] = "请使用简洁、直接的语气";
    tones.append(concise);
    
    // 将语气列表添加到配置中
    m_aiConfig["tones"] = tones;
}

// 根据主题获取背景颜色
QColor AiAssistantDialog::getBackgroundColor() const
{
    return m_isDarkTheme ? QColor(40, 40, 40) : QColor(245, 245, 245);
}

// 根据主题获取文本颜色
QColor AiAssistantDialog::getTextColor() const
{
    return m_isDarkTheme ? QColor(230, 230, 230) : QColor(30, 30, 30);
}

// 根据主题获取边框颜色
QColor AiAssistantDialog::getBorderColor() const
{
    return m_isDarkTheme ? QColor(70, 70, 70) : QColor(200, 200, 200);
}

// 根据主题获取输入框背景颜色
QColor AiAssistantDialog::getInputBackgroundColor() const
{
    return m_isDarkTheme ? QColor(60, 60, 60) : QColor(255, 255, 255);
}

// 根据主题获取按钮颜色
QColor AiAssistantDialog::getButtonColor() const
{
    return m_isDarkTheme ? QColor(70, 130, 180) : QColor(66, 133, 244);
}

// 设置UI界面
void AiAssistantDialog::setupUI()
{
    // 创建主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(8);
    
    // 创建标题和关闭按钮布局
    QHBoxLayout *titleLayout = new QHBoxLayout();
    titleLayout->setContentsMargins(5, 5, 5, 5);
    titleLayout->setSpacing(0);
    
    // 创建标题标签
    m_titleLabel = new QLabel("AI 助手 - 编辑选中内容", this);
    QFont titleFont = m_titleLabel->font();
    titleFont.setBold(true);
    titleFont.setPointSize(10);
    m_titleLabel->setFont(titleFont);
    
    // 添加标题到布局
    titleLayout->addWidget(m_titleLabel, 1);
    
    // 创建关闭按钮
    QPushButton *closeButton = new QPushButton("×", this);
    closeButton->setFixedSize(24, 24);
    closeButton->setFlat(true);
    closeButton->setCursor(Qt::PointingHandCursor);
    connect(closeButton, &QPushButton::clicked, this, &AiAssistantDialog::close);
    
    // 添加关闭按钮到布局
    titleLayout->addWidget(closeButton);
    
    // 添加标题布局到主布局
    mainLayout->addLayout(titleLayout);
    
    // 创建水平分割线
    QFrame *line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(line);
    
    // 创建功能菜单布局
    QHBoxLayout *comboLayout = new QHBoxLayout();
    comboLayout->setContentsMargins(0, 5, 0, 5);
    comboLayout->setSpacing(8);
    
    // 创建功能按钮和菜单
    m_functionButton = new QToolButton(this);
    m_functionButton->setText("功能");
    m_functionButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_functionButton->setPopupMode(QToolButton::InstantPopup);
    m_functionButton->setArrowType(Qt::NoArrow);
    m_functionButton->setFixedHeight(28);
    m_functionButton->setCursor(Qt::PointingHandCursor);
    
    // 创建功能菜单
    m_functionMenu = new QMenu(this);
    m_functionButton->setMenu(m_functionMenu);
    
    // 设置功能菜单
    setupFunctionMenu();
    
    // 添加功能按钮到布局
    QLabel *functionLabel = new QLabel("功能:", this);
    comboLayout->addWidget(functionLabel);
    comboLayout->addWidget(m_functionButton, 1);
    comboLayout->addStretch();
    
    // 添加菜单布局到主布局
    mainLayout->addLayout(comboLayout);
    
    // 创建AI响应区域
    m_responseTextEdit = new QTextEdit(this);
    m_responseTextEdit->setReadOnly(true);
    m_responseTextEdit->setPlaceholderText("AI 助手将在这里显示回复...");
    
    // 添加AI响应区域到主布局
    mainLayout->addWidget(m_responseTextEdit, 1);
    
    // 创建输入区域布局
    QHBoxLayout *inputLayout = new QHBoxLayout();
    inputLayout->setContentsMargins(0, 5, 0, 5);
    inputLayout->setSpacing(8);
    
    // 创建输入框
    m_inputLineEdit = new QLineEdit(this);
    m_inputLineEdit->setFixedHeight(32);
    m_inputLineEdit->setPlaceholderText("在此输入消息...");
    m_inputLineEdit->setMaxLength(200); // 限制输入长度
    
    // 回车键发送消息
    connect(m_inputLineEdit, &QLineEdit::returnPressed, this, &AiAssistantDialog::sendMessage);
    
    // 创建发送按钮
    m_sendButton = new QPushButton("发送", this);
    m_sendButton->setFixedSize(60, 32);
    m_sendButton->setCursor(Qt::PointingHandCursor);
    connect(m_sendButton, &QPushButton::clicked, this, &AiAssistantDialog::sendMessage);
    
    // 添加输入框和发送按钮到输入布局
    inputLayout->addWidget(m_inputLineEdit, 1);
    inputLayout->addWidget(m_sendButton);
    
    // 添加输入布局到主布局
    mainLayout->addLayout(inputLayout);
    
    // 创建底部按钮布局
    QHBoxLayout *bottomButtonLayout = new QHBoxLayout();
    bottomButtonLayout->setContentsMargins(0, 5, 0, 0);
    bottomButtonLayout->setSpacing(8);
    
    // 创建底部按钮
    m_insertButton = new QPushButton("插入到文档", this);
    m_insertButton->setFixedHeight(32);
    m_insertButton->setCursor(Qt::PointingHandCursor);
    connect(m_insertButton, &QPushButton::clicked, this, &AiAssistantDialog::onInsertButtonClicked);
    
    m_retryButton = new QPushButton("重新尝试", this);
    m_retryButton->setFixedHeight(32);
    m_retryButton->setCursor(Qt::PointingHandCursor);
    connect(m_retryButton, &QPushButton::clicked, this, &AiAssistantDialog::onRetryButtonClicked);
    
    m_cancelButton = new QPushButton("取消", this);
    m_cancelButton->setObjectName("m_cancelButton"); // 设置对象名以便在QSS中选择
    m_cancelButton->setFixedHeight(32);
    m_cancelButton->setCursor(Qt::PointingHandCursor);
    connect(m_cancelButton, &QPushButton::clicked, this, &AiAssistantDialog::onCancelButtonClicked);
    
    // 添加按钮到布局
    bottomButtonLayout->addWidget(m_insertButton);
    bottomButtonLayout->addWidget(m_retryButton);
    bottomButtonLayout->addWidget(m_cancelButton);
    
    // 添加底部按钮布局到主布局
    mainLayout->addLayout(bottomButtonLayout);
    
    // 设置初始状态
    m_insertButton->setEnabled(false); // 初始禁用，直到有生成内容
    m_retryButton->setEnabled(false);  // 初始禁用，直到有生成内容
}

// 更新样式
void AiAssistantDialog::updateStyles()
{
    // 获取主题颜色
    QColor bgColor = getBackgroundColor();
    QColor textColor = getTextColor();
    QColor borderColor = getBorderColor();
    QColor inputBgColor = getInputBackgroundColor();
    QColor buttonColor = getButtonColor();
    
    // 加载QSS文件
    QFile qssFile(":/styles/ai_assistant_dialog.qss");
    if (qssFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString styleSheet = qssFile.readAll();
        qssFile.close();
        
        // 替换颜色变量
        styleSheet.replace("%bg-color%", bgColor.name());
        styleSheet.replace("%text-color%", textColor.name());
        styleSheet.replace("%border-color%", borderColor.name());
        styleSheet.replace("%input-bg-color%", inputBgColor.name());
        styleSheet.replace("%button-color%", buttonColor.name());
        styleSheet.replace("%button-hover-color%", buttonColor.lighter(110).name());
        styleSheet.replace("%button-pressed-color%", buttonColor.darker(110).name());
        styleSheet.replace("%button-disabled-color%", m_isDarkTheme ? "#555555" : "#cccccc");
        styleSheet.replace("%cancel-button-bg%", m_isDarkTheme ? "#505050" : "#f0f0f0");
        styleSheet.replace("%cancel-button-hover%", m_isDarkTheme ? "#606060" : "#e0e0e0");
        
        // 替换RGB值用于透明效果
        int r, g, b;
        buttonColor.getRgb(&r, &g, &b);
        styleSheet.replace("%button-color-rgb%", QString("%1, %2, %3").arg(r).arg(g).arg(b));
        
        // 应用样式表
        setStyleSheet(styleSheet);
    } else {
        qDebug() << "无法加载AI助手对话框样式文件";
    }
}

// 设置功能菜单
void AiAssistantDialog::setupFunctionMenu()
{
    // 清空现有菜单项
    m_functionMenu->clear();
    
    // 创建自由对话功能
    m_freeChatAction = new QAction(QIcon(":/icons/editor/free-chat.svg"), "自由对话", this);
    m_freeChatAction->setCheckable(true);
    m_freeChatAction->setChecked(m_currentFunction == "自由对话");
    
    // 创建功能选项并添加图标
    m_continueWritingAction = new QAction(QIcon(":/icons/editor/continue-writing.svg"), "继续写作", this);
    m_continueWritingAction->setCheckable(true);
    m_continueWritingAction->setChecked(m_currentFunction == "继续写作");
    
    m_polishContentAction = new QAction(QIcon(":/icons/editor/polish-content.svg"), "内容润色", this);
    m_polishContentAction->setCheckable(true);
    m_polishContentAction->setChecked(m_currentFunction == "内容润色");
    
    m_expandContentAction = new QAction(QIcon(":/icons/editor/expand-content.svg"), "内容扩写", this);
    m_expandContentAction->setCheckable(true);
    m_expandContentAction->setChecked(m_currentFunction == "内容扩写");
    
    m_simplifyContentAction = new QAction(QIcon(":/icons/editor/simplify-content.svg"), "内容精简", this);
    m_simplifyContentAction->setCheckable(true);
    m_simplifyContentAction->setChecked(m_currentFunction == "内容精简");
    
    m_grammarCheckAction = new QAction(QIcon(":/icons/editor/grammar-check.svg"), "语法纠错", this);
    m_grammarCheckAction->setCheckable(true);
    m_grammarCheckAction->setChecked(m_currentFunction == "语法纠错");
    
    // 创建语气转变选项和子菜单
    m_changeToneAction = new QAction(QIcon(":/icons/editor/change-tone.svg"), "语气转变", this);
    m_changeToneAction->setCheckable(true);
    m_changeToneAction->setChecked(m_currentFunction == "语气转变");
    
    // 创建语气子菜单
    m_toneSubMenu = new QMenu("选择语气", this);
    
    // 创建语气选项
    m_professionalToneAction = new QAction("专业", this);
    m_professionalToneAction->setCheckable(true);
    m_professionalToneAction->setChecked(m_currentTone == "专业");
    
    m_casualToneAction = new QAction("日常", this);
    m_casualToneAction->setCheckable(true);
    m_casualToneAction->setChecked(m_currentTone == "日常");
    
    m_honestToneAction = new QAction("坦诚", this);
    m_honestToneAction->setCheckable(true);
    m_honestToneAction->setChecked(m_currentTone == "坦诚");
    
    m_confidentToneAction = new QAction("自信", this);
    m_confidentToneAction->setCheckable(true);
    m_confidentToneAction->setChecked(m_currentTone == "自信");
    
    m_humorousToneAction = new QAction("幽默", this);
    m_humorousToneAction->setCheckable(true);
    m_humorousToneAction->setChecked(m_currentTone == "幽默");
    
    // 将语气选项添加到语气子菜单
    m_toneSubMenu->addAction(m_professionalToneAction);
    m_toneSubMenu->addAction(m_casualToneAction);
    m_toneSubMenu->addAction(m_honestToneAction);
    m_toneSubMenu->addAction(m_confidentToneAction);
    m_toneSubMenu->addAction(m_humorousToneAction);
    
    // 将语气子菜单关联到语气转变选项
    m_changeToneAction->setMenu(m_toneSubMenu);
    
    // 添加所有功能选项到功能菜单，将自由对话放在最前面
    m_functionMenu->addAction(m_freeChatAction);
    m_functionMenu->addAction(m_continueWritingAction);
    m_functionMenu->addAction(m_polishContentAction);
    m_functionMenu->addAction(m_expandContentAction);
    m_functionMenu->addAction(m_simplifyContentAction);
    m_functionMenu->addAction(m_grammarCheckAction);
    m_functionMenu->addAction(m_changeToneAction);
    
    // 连接自由对话选项的信号
    connect(m_freeChatAction, &QAction::triggered, this, [this]() { 
        onFunctionSelected(m_freeChatAction); 
    });
    
    // 连接功能选项的信号
    connect(m_continueWritingAction, &QAction::triggered, this, [this]() { 
        onFunctionSelected(m_continueWritingAction); 
    });
    connect(m_polishContentAction, &QAction::triggered, this, [this]() { 
        onFunctionSelected(m_polishContentAction); 
    });
    connect(m_expandContentAction, &QAction::triggered, this, [this]() { 
        onFunctionSelected(m_expandContentAction); 
    });
    connect(m_simplifyContentAction, &QAction::triggered, this, [this]() { 
        onFunctionSelected(m_simplifyContentAction); 
    });
    connect(m_grammarCheckAction, &QAction::triggered, this, [this]() { 
        onFunctionSelected(m_grammarCheckAction); 
    });
    connect(m_changeToneAction, &QAction::triggered, this, [this]() { 
        onFunctionSelected(m_changeToneAction); 
    });
    
    // 连接语气选项的信号
    connect(m_professionalToneAction, &QAction::triggered, this, [this]() { 
        onToneSelected(m_professionalToneAction); 
    });
    connect(m_casualToneAction, &QAction::triggered, this, [this]() { 
        onToneSelected(m_casualToneAction); 
    });
    connect(m_honestToneAction, &QAction::triggered, this, [this]() { 
        onToneSelected(m_honestToneAction); 
    });
    connect(m_confidentToneAction, &QAction::triggered, this, [this]() { 
        onToneSelected(m_confidentToneAction); 
    });
    connect(m_humorousToneAction, &QAction::triggered, this, [this]() { 
        onToneSelected(m_humorousToneAction); 
    });
    
    // 设置功能按钮的当前显示文本
    updateFunctionButtonText();
}

// 更新功能按钮显示文本
void AiAssistantDialog::updateFunctionButtonText()
{
    if (m_currentFunction == "语气转变") {
        // 如果是语气转变，显示当前选择的语气
        m_functionButton->setText("语气转变: " + m_currentTone);
    } else {
        // 否则显示当前选择的功能
        m_functionButton->setText(m_currentFunction);
    }
}

// 功能选择处理
void AiAssistantDialog::onFunctionSelected(QAction* action)
{
    // 取消选中其他功能选项
    for (QAction* functionAction : {m_continueWritingAction, m_polishContentAction, 
                                      m_expandContentAction, m_simplifyContentAction, 
                                      m_grammarCheckAction, m_changeToneAction}) {
        if (functionAction != action) {
            functionAction->setChecked(false);
        }
    }
    
    // 更新当前功能
    m_currentFunction = action->text();
    
    // 更新功能按钮显示文本
    updateFunctionButtonText();
}

// 语气选择处理
void AiAssistantDialog::onToneSelected(QAction* action)
{
    // 取消选中其他语气选项
    for (QAction* toneAction : {m_professionalToneAction, m_casualToneAction, 
                                  m_honestToneAction, m_confidentToneAction, 
                                  m_humorousToneAction}) {
        if (toneAction != action) {
            toneAction->setChecked(false);
        }
    }
    
    // 更新当前语气
    m_currentTone = action->text();
    
    // 选中语气转变功能
    m_changeToneAction->setChecked(true);
    
    // 取消选中其他功能选项
    for (QAction* functionAction : {m_continueWritingAction, m_polishContentAction, 
                                      m_expandContentAction, m_simplifyContentAction, 
                                      m_grammarCheckAction}) {
        functionAction->setChecked(false);
    }
    
    // 更新当前功能为语气转变
    m_currentFunction = "语气转变";
    
    // 更新功能按钮显示文本
    updateFunctionButtonText();
}

// 显示加载状态
void AiAssistantDialog::showLoadingState()
{
    // 禁用输入和相关按钮
    m_inputLineEdit->setEnabled(false);
    m_sendButton->setEnabled(false);
    m_functionButton->setEnabled(false);
    m_responseTextEdit->setReadOnly(true);
    
    // 停止并删除旧的定时器（如果存在）
    if (m_animationTimer) {
        m_animationTimer->stop();
        delete m_animationTimer; // 删除旧实例
        m_animationTimer = nullptr;
    }
    
    // 创建新的定时器实例，并将其作为对话框的子对象
    m_animationTimer = new QTimer(this); 
    QStringList dots = {" ", ".", "..", "..."};
    m_animationDotIndex = 0; // 重置索引
    
    connect(m_animationTimer, &QTimer::timeout, this, [this, dots]() { // 不再捕获 animationTimer 和 dotIndex 引用
        // 更新标题添加动画效果
        if (m_titleLabel && !dots.isEmpty()) { // 增加列表非空检查
             int currentSize = static_cast<int>(dots.size());
             if (m_animationDotIndex >= 0 && m_animationDotIndex < currentSize) {
                 m_titleLabel->setText("AI助手 - 处理中" + dots.at(m_animationDotIndex)); 
             } else {
                 qWarning() << "m_animationDotIndex is out of range:" << m_animationDotIndex << "size:" << currentSize;
                 m_titleLabel->setText("AI助手 - 处理中..."); // Fallback text
                 m_animationDotIndex = 0; // Reset index on error
             }
             m_animationDotIndex = (m_animationDotIndex + 1) % currentSize;
        } else if (m_titleLabel) {
             m_titleLabel->setText("AI助手 - 处理中..."); // Fallback if dots is empty
        }
       
    });
    
    m_animationTimer->start(350);
    
    if (m_titleLabel) {
       m_titleLabel->setText("AI助手 - 处理中...");
    }
}

// --- 辅助函数：停止加载动画 ---
void AiAssistantDialog::stopLoadingAnimation()
{
    if (m_animationTimer && m_animationTimer->isActive()) {
        m_animationTimer->stop();
    }
    // 恢复标题 (如果需要，可以在调用处单独恢复)
    // if(m_titleLabel) m_titleLabel->setText("AI 助手 - 编辑选中内容");
}

// 发送消息到AI
void AiAssistantDialog::sendMessage()
{
    qDebug() << "AiAssistantDialog(" << this << ")::sendMessage - Checking m_aiService:" << m_aiService;
    QString userInput = m_inputLineEdit->text().trimmed();
    if (userInput.isEmpty() && m_selectedText.isEmpty()) {
        // 如果没有输入内容且没有选中文本，不做任何处理
        return;
    }
    
    // 清空输入框
    m_inputLineEdit->clear();
    
    // 显示加载状态
    showLoadingState();
    
    // 检查AI服务是否可用
    if (!m_aiService) {
        handleAiError("AI服务", "AI服务未初始化 - triggered in sendMessage"); // 添加来源信息
        return;
    }
    

    
    // 处理AI请求
    processAiRequest();
}

// 处理AI请求
void AiAssistantDialog::processAiRequest()
{
    qDebug() << "AiAssistantDialog::processAiRequest - Checking m_aiService:" << m_aiService;
    // 检查AI服务是否可用 (再次检查以防万一)
    if (!m_aiService) {
        handleAiError("AI服务", "AI服务未初始化 - triggered in processAiRequest"); // 添加来源信息
        return;
    }

    // 获取用户输入或选中文本
    QString text = !m_selectedText.isEmpty() ? m_selectedText : m_inputLineEdit->text().trimmed();
    
    // 根据当前功能调用不同的AI服务方法
    if (m_currentFunction == "自由对话") {
        // 自由对话不添加任何额外提示词，直接发送用户输入
        m_aiService->generateGenericText(text);
    } else if (m_currentFunction == "继续写作") {
        m_aiService->generateGenericText("请继续完成下面的文本，保持风格一致：\n\n" + text);
    } else if (m_currentFunction == "内容润色") {
        m_aiService->rewriteText(text);
    } else if (m_currentFunction == "语法纠错") {
        m_aiService->fixText(text);
    } else if (m_currentFunction == "内容精简") {
        m_aiService->generateGenericText("请将以下文本精简，保留核心信息但使表达更加简洁：\n\n" + text);
    } else if (m_currentFunction == "内容扩写") {
        m_aiService->generateGenericText("请扩展以下内容，增加细节和相关信息，使文本更加丰富：\n\n" + text);
    } else if (m_currentFunction == "语气转变") {
        QString tonePrompt = "请将以下文本转变为" + m_currentTone + "的语气，但保持原意不变：\n\n";
        m_aiService->generateGenericText(tonePrompt + text);
    } else {
        // 默认使用通用文本生成
        m_aiService->generateGenericText(text);
    }
}

// 插入按钮点击处理
void AiAssistantDialog::onInsertButtonClicked()
{
    if (!m_generatedContent.isEmpty()) {
        // 发出插入内容信号
        emit insertContentToDocument(m_generatedContent);
        
        // 禁用插入按钮
        m_insertButton->setEnabled(false);
        
        // 关闭对话框
        accept();
    }
}

// 重试按钮点击处理
void AiAssistantDialog::onRetryButtonClicked()
{
    // 显示加载状态
    showLoadingState();
    
    // 检查AI服务是否可用
    if (!m_aiService) {
        handleAiError("AI服务", "AI服务未初始化");
        return;
    }
    
    // 针对特定错误进行恢复处理
    if (m_lastErrorType == "API端点URL格式错误") {
        // 这个错误类型是我们添加的，用于标识URL格式问题
        // 使用DeepSeekService的专用方法尝试重置API端点
        // 注意：这里直接调用可能不是最佳实践，更好的方式是通过信号/槽或接口
        IAiService *aiServicePtr = m_aiService; // 临时指针以调用
        if (auto deepSeekService = qobject_cast<DeepSeekService*>(aiServicePtr)) {
             deepSeekService->setApiEndpoint("https://api.deepseek.com/v1/chat/completions");
             qDebug() << "已尝试重置API端点URL并重试";
        } else {
            qWarning() << "无法将IAiService转换为DeepSeekService以重置端点";
            handleAiError("重试", "无法执行端点修复");
            return;
        }
       
    }
    
    // 重新处理AI请求
    processAiRequest();
}

// 取消按钮点击处理
void AiAssistantDialog::onCancelButtonClicked()
{
    // 关闭对话框
    reject();
}

// 设置AI服务
void AiAssistantDialog::setAiService(IAiService *service)
{
    qDebug() << "AiAssistantDialog(" << this << ")::setAiService called with service:" << service;
    // 如果已有服务，先断开信号连接
    if (m_aiService) {
        disconnect(m_aiService, nullptr, this, nullptr);
    }
    
    m_aiService = service;
    
    if (m_aiService) {
        // 连接AI服务信号
        connectAiServiceSignals();
        

    }
}

// 连接AI服务信号
void AiAssistantDialog::connectAiServiceSignals()
{
    if (!m_aiService) {
        return;
    }
    
    // 使用 Qt::UniqueConnection 防止重复连接
    disconnect(m_aiService, nullptr, this, nullptr); // 先断开所有旧连接
    
    // 连接各种完成信号
    connect(m_aiService, &IAiService::rewriteFinished, 
            this, &AiAssistantDialog::handleRewriteFinished, Qt::UniqueConnection);
    
    connect(m_aiService, &IAiService::summaryFinished, 
            this, &AiAssistantDialog::handleSummaryFinished, Qt::UniqueConnection);
    
    connect(m_aiService, &IAiService::fixFinished, 
            this, &AiAssistantDialog::handleFixFinished, Qt::UniqueConnection);
    
    connect(m_aiService, &IAiService::genericTextFinished, 
            this, &AiAssistantDialog::handleGenericTextFinished, Qt::UniqueConnection);
    
    // 连接错误信号
    connect(m_aiService, &IAiService::aiError, 
            this, &AiAssistantDialog::handleAiError, Qt::UniqueConnection);
}

// 处理润色完成
void AiAssistantDialog::handleRewriteFinished(const QString& originalText, const QString& rewrittenText)
{
    stopLoadingAnimation(); // 停止动画
     // 恢复UI状态
    m_inputLineEdit->setEnabled(true);
    m_sendButton->setEnabled(true);
    m_functionButton->setEnabled(true);
    m_responseTextEdit->setReadOnly(false);
    m_responseTextEdit->setAlignment(Qt::AlignLeft);
    if(m_titleLabel) m_titleLabel->setText("AI 助手 - 编辑选中内容");

    // 显示润色结果
    m_responseTextEdit->clear();
    m_responseTextEdit->append("<b>原文：</b>");
    m_responseTextEdit->append(originalText);
    m_responseTextEdit->append("\n<b>润色后：</b>");
    m_responseTextEdit->append(rewrittenText);
    
    // 保存生成的内容
    m_generatedContent = rewrittenText;
    
    // 启用按钮
    m_insertButton->setEnabled(true);
    m_retryButton->setEnabled(true);
}

// 处理总结完成
void AiAssistantDialog::handleSummaryFinished(const QString& originalText, const QString& summaryText)
{
    stopLoadingAnimation(); // 停止动画
     // 恢复UI状态
    m_inputLineEdit->setEnabled(true);
    m_sendButton->setEnabled(true);
    m_functionButton->setEnabled(true);
    m_responseTextEdit->setReadOnly(false);
    m_responseTextEdit->setAlignment(Qt::AlignLeft);
    if(m_titleLabel) m_titleLabel->setText("AI 助手 - 编辑选中内容");

    // 显示总结结果
    m_responseTextEdit->clear();
    m_responseTextEdit->append("<b>原文：</b>");
    m_responseTextEdit->append(originalText);
    m_responseTextEdit->append("\n<b>总结：</b>");
    m_responseTextEdit->append(summaryText);
    
    // 保存生成的内容
    m_generatedContent = summaryText;
    
    // 启用按钮
    m_insertButton->setEnabled(true);
    m_retryButton->setEnabled(true);
}

// 处理修复完成
void AiAssistantDialog::handleFixFinished(const QString& originalText, const QString& fixedText)
{
    stopLoadingAnimation(); // 停止动画
    // 恢复UI状态
    m_inputLineEdit->setEnabled(true);
    m_sendButton->setEnabled(true);
    m_functionButton->setEnabled(true);
    m_responseTextEdit->setReadOnly(false);
    m_responseTextEdit->setAlignment(Qt::AlignLeft);
    if(m_titleLabel) m_titleLabel->setText("AI 助手 - 编辑选中内容");

    // 显示修复结果
    m_responseTextEdit->clear();
    m_responseTextEdit->append("<b>原文：</b>");
    m_responseTextEdit->append(originalText);
    m_responseTextEdit->append("\n<b>修复后：</b>");
    m_responseTextEdit->append(fixedText);
    
    // 保存生成的内容
    m_generatedContent = fixedText;
    
    // 启用按钮
    m_insertButton->setEnabled(true);
    m_retryButton->setEnabled(true);
}

// 处理通用文本生成完成
void AiAssistantDialog::handleGenericTextFinished(const QString& prompt, const QString& generatedText)
{
    stopLoadingAnimation(); // 停止动画
     // 恢复UI状态
    m_inputLineEdit->setEnabled(true);
    m_sendButton->setEnabled(true);
    m_functionButton->setEnabled(true);
    m_responseTextEdit->setReadOnly(false);
    m_responseTextEdit->setAlignment(Qt::AlignLeft);
    if(m_titleLabel) m_titleLabel->setText("AI 助手 - 编辑选中内容");

    // 显示生成结果
    m_responseTextEdit->clear();
    
    // 根据当前功能调整显示方式
    if (m_currentFunction == "继续写作") {
        m_responseTextEdit->setText(generatedText);
    } else if (m_currentFunction == "内容扩写") {
        m_responseTextEdit->append("<b>原文：</b>");
        m_responseTextEdit->append(m_selectedText); // 使用成员变量m_selectedText
        m_responseTextEdit->append("\n<b>扩写后：</b>");
        m_responseTextEdit->append(generatedText);
    } else if (m_currentFunction == "内容精简") {
        m_responseTextEdit->append("<b>原文：</b>");
        m_responseTextEdit->append(m_selectedText); // 使用成员变量m_selectedText
        m_responseTextEdit->append("\n<b>精简后：</b>");
        m_responseTextEdit->append(generatedText);
    } else if (m_currentFunction == "语气转变") {
        m_responseTextEdit->append("<b>原文：</b>");
        m_responseTextEdit->append(m_selectedText); // 使用成员变量m_selectedText
        m_responseTextEdit->append(QString("\n<b>使用%1语气后：</b>").arg(m_currentTone));
        m_responseTextEdit->append(generatedText);
    } else {
        // 如果是直接发送消息（没有选中文本），prompt就是用户输入
        // 如果是基于选中文本的其他功能，prompt是原始请求（包含指令）
        // 为了更清晰，可以考虑显示原始请求或选中的文本
        if (!m_selectedText.isEmpty()) {
             m_responseTextEdit->append("<b>原始请求/文本：</b>");
             m_responseTextEdit->append(m_selectedText); // 或 prompt
             m_responseTextEdit->append("\n<b>生成结果：</b>");
        }
        m_responseTextEdit->append(generatedText);
    }
    
    // 保存生成的内容
    m_generatedContent = generatedText;
    
    // 启用按钮
    m_insertButton->setEnabled(true);
    m_retryButton->setEnabled(true);
}

// AI错误处理
void AiAssistantDialog::handleAiError(const QString& operationDescription, const QString& errorMessage)
{
    stopLoadingAnimation(); // 停止动画
    qWarning() << "AI错误:" << operationDescription << "-" << errorMessage;
    
    // 恢复UI状态
    m_inputLineEdit->setEnabled(true);
    m_sendButton->setEnabled(true);
    m_functionButton->setEnabled(true);
    m_responseTextEdit->setReadOnly(false);
    m_responseTextEdit->setAlignment(Qt::AlignLeft); // 恢复左对齐
    if(m_titleLabel) m_titleLabel->setText("AI 助手 - 编辑选中内容"); // 恢复标题
    
    // 构建更友好的错误消息
    QString friendlyError;
    QString detailedError;
    
    // 根据错误消息提供具体建议
    if (errorMessage.contains("API密钥无效") || errorMessage.contains("认证失败")) {
        friendlyError = "API密钥问题";
        detailedError = "您的DeepSeek API密钥可能无效或已过期。请检查设置中的API密钥配置。";
        m_lastErrorType = "API密钥无效";
    } 
    else if (errorMessage.contains("超过API速率限制") || errorMessage.contains("超过API配额")) {
        friendlyError = "API使用限制";
        detailedError = "您已达到DeepSeek API的使用限制。请稍后再试或检查您的账户状态。";
        m_lastErrorType = "API使用限制";
    }
    else if (errorMessage.contains("请求超时") || errorMessage.contains("连接被拒绝") || 
             errorMessage.contains("网络") || errorMessage.contains("找不到服务器") ||
             errorMessage.contains("Operation canceled")) { // 将Operation canceled也归类为网络问题
        friendlyError = "网络连接问题";
        detailedError = "无法连接到DeepSeek服务器或请求被取消。请检查您的网络连接或稍后再试。";
        m_lastErrorType = "网络连接问题";
    }
    else if (errorMessage.contains("服务器内部错误") || errorMessage.contains("繁忙")) {
        friendlyError = "DeepSeek服务器问题";
        detailedError = "DeepSeek服务器当前可能存在问题。根据最近的报告，DeepSeek API可能正在经历服务中断。请稍后再试。";
        m_lastErrorType = "服务器问题";
    }
    else if (errorMessage.contains("服务未初始化")) {
        friendlyError = "AI服务未初始化";
        detailedError = "AI服务未能正确初始化。这可能是因为之前的错误或程序逻辑问题。";
        m_lastErrorType = "服务未初始化";
    }
    else if (errorMessage.contains("Method Not Allowed") || errorMessage.contains("API端点URL格式错误")) {
        friendlyError = "API端点URL格式错误";
        detailedError = "DeepSeek API端点URL格式可能不正确。系统将尝试自动修复，请点击重试按钮。";
        m_lastErrorType = "API端点URL格式错误";
    }
    else if (errorMessage.contains("内容访问错误")) {
        friendlyError = "API访问错误";
        detailedError = "DeepSeek API访问出现问题。这可能是因为API端点URL格式不正确，系统将尝试自动修复，请点击重试按钮。";
        m_lastErrorType = "API端点URL格式错误"; // 归类为格式错误以便重试修复
    }
     else if (errorMessage.contains("JSON解析错误")) {
        friendlyError = "响应解析错误";
        detailedError = "无法解析来自DeepSeek服务器的响应。响应格式可能无效。";
        m_lastErrorType = "响应解析错误";
    } 
    else {
        friendlyError = "AI服务错误";
        detailedError = errorMessage;
        m_lastErrorType = "未知错误";
    }
    
    // 记录错误
    qWarning() << "友好错误消息:" << friendlyError;
    qWarning() << "详细错误消息:" << detailedError;
    
    // 显示错误信息对话框 (暂时移除条件，总是显示，方便调试)
    // if (m_lastErrorType != "服务未初始化") { 
    QMessageBox errorBox(QMessageBox::Warning, 
                         "在" + operationDescription + "操作中发生错误", 
                         friendlyError + "\n\n" + detailedError,
                         QMessageBox::Ok, 
                         this);
    errorBox.setDetailedText("原始错误: " + errorMessage + 
                             "\n\n提示: DeepSeek API近期可能存在稳定性问题，建议：\n" +
                             "1. 检查API密钥是否正确\n" +
                             "2. 确认网络连接正常\n" +
                             "3. 稍后再试\n" +
                             "4. 尝试使用其他AI服务提供商");
    
    // 设置对话框样式
    errorBox.setWindowFlags(errorBox.windowFlags() & ~Qt::WindowContextHelpButtonHint);
    if (m_isDarkTheme) {
        errorBox.setStyleSheet("QMessageBox { background-color: #333; color: #fff; }" \
                               "QLabel { color: #fff; }" \
                               "QPushButton { background-color: #555; color: #fff; padding: 6px 12px; border-radius: 4px; }" \
                               "QPushButton:hover { background-color: #666; }" \
                               "QTextEdit { background-color: #444; color: #ddd; border: 1px solid #555; }");
    }
    
    errorBox.exec();
    // }
    
    // 允许重试
    m_retryButton->setEnabled(true);
    
    // 在编辑框中显示错误
    m_responseTextEdit->clear();
    m_responseTextEdit->setTextColor(m_isDarkTheme ? QColor(255, 100, 100) : QColor(200, 0, 0));
    m_responseTextEdit->append("错误: " + friendlyError);
    m_responseTextEdit->append("\n" + detailedError);
    
    // 建议用户检查DeepSeek状态
    m_responseTextEdit->append("\n建议：");
    m_responseTextEdit->append("• 检查API密钥是否正确");
    m_responseTextEdit->append("• 确认网络连接正常");
    m_responseTextEdit->append("• 稍后再试");
    
    // 恢复默认文本颜色
    m_responseTextEdit->setTextColor(getTextColor());
} 