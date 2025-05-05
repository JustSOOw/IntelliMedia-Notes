/*
 * @Author: cursor AI
 * @Date: 2025-05-12 10:00:00
 * @LastEditors: Furdow wang22338014@gmail.com
 * @LastEditTime: 2025-05-05 22:43:07
 * @FilePath: \IntelliMedia_Notes\src\aiassistantdialog.cpp
 * @Description: AI助手对话框实现
 * 
 * Copyright (c) 2025, All Rights Reserved. 
 */
#include "aiassistantdialog.h"
#include "IAiService.h"
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
    : QDialog(parent)
    , m_inputLineEdit(nullptr)
    , m_responseTextEdit(nullptr)
    , m_functionButton(nullptr)
    , m_functionMenu(nullptr)
    , m_toneSubMenu(nullptr)
    , m_sendButton(nullptr)
    , m_insertButton(nullptr)
    , m_retryButton(nullptr)
    , m_cancelButton(nullptr)
    , m_titleLabel(nullptr)
    , m_selectedText("")
    , m_generatedContent("")
    , m_isDarkTheme(false)
    , m_dragging(false)
    , m_currentFunction("继续写作") // 默认功能
    , m_currentTone("专业") // 默认语气
    , m_aiService(nullptr) // 初始无AI服务
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
    
    // 添加所有功能选项到功能菜单
    m_functionMenu->addAction(m_continueWritingAction);
    m_functionMenu->addAction(m_polishContentAction);
    m_functionMenu->addAction(m_expandContentAction);
    m_functionMenu->addAction(m_simplifyContentAction);
    m_functionMenu->addAction(m_grammarCheckAction);
    m_functionMenu->addAction(m_changeToneAction);
    
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
    
    // 创建并显示加载提示
    m_responseTextEdit->clear();
    m_responseTextEdit->setAlignment(Qt::AlignCenter);
    
    // 添加加载动画文本
    QString loadingText = "<div style='text-align:center; margin-top:30px;'>";
    
    // 添加加载图标 (使用Unicode字符作为简单动画)
    loadingText += "<div style='font-size:24px; margin-bottom:15px;'>⏳</div>";
    
    // 主要文本
    loadingText += "<div style='font-size:16px; margin-bottom:10px;'><b>正在与DeepSeek AI交流...</b></div>";
    
    // 添加提示
    loadingText += QString("<div style='font-size:12px; color:%1; margin-top:20px;'>")
        .arg(m_isDarkTheme ? "#aaa" : "#666");
    loadingText += "请耐心等待，这可能需要几秒钟时间";
    loadingText += "</div>";
    
    // 添加DeepSeek API可能不稳定的提示
    loadingText += QString("<div style='font-size:11px; color:%1; margin-top:10px; font-style:italic;'>")
        .arg(m_isDarkTheme ? "#999" : "#999");
    loadingText += "提示: 如果长时间没有响应，可能是DeepSeek API暂时不可用<br>";
    loadingText += "请尝试稍后再试";
    loadingText += "</div>";
    
    loadingText += "</div>";
    
    m_responseTextEdit->setHtml(loadingText);
    
    // 添加处理中...动画效果
    QTimer *animationTimer = new QTimer(this);
    QStringList dots = {" ", ".", "..", "..."};
    int dotIndex = 0;
    
    connect(animationTimer, &QTimer::timeout, this, [this, dots, animationTimer, &dotIndex]() {
        // 更新标题添加动画效果
        m_titleLabel->setText("AI助手 - 处理中" + dots.at(static_cast<int>(dotIndex)));
        dotIndex = (dotIndex + 1) % static_cast<int>(dots.size());
    });
    
    // 350毫秒更新一次动画
    animationTimer->start(350);
    
    // 在请求完成或超时时停止动画
    QTimer::singleShot(30000, this, [animationTimer]() {
        // 30秒后如果仍未收到响应，停止动画
        if (animationTimer->isActive()) {
            animationTimer->stop();
            animationTimer->deleteLater();
        }
    });
    
    // 设置对话框标题
    m_titleLabel->setText("AI助手 - 处理中...");
}

// 发送消息到AI
void AiAssistantDialog::sendMessage()
{
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
        handleAiError("AI服务", "AI服务未初始化");
        return;
    }
    
    // 处理AI请求
    processAiRequest();
}

// 处理AI请求
void AiAssistantDialog::processAiRequest()
{
    // 获取用户输入或选中文本
    QString text = !m_selectedText.isEmpty() ? m_selectedText : m_inputLineEdit->text().trimmed();
    
    // 根据当前功能调用不同的AI服务方法
    if (m_currentFunction == "继续写作") {
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
    
    // 连接各种完成信号
    connect(m_aiService, &IAiService::rewriteFinished, 
            this, &AiAssistantDialog::handleRewriteFinished);
    
    connect(m_aiService, &IAiService::summaryFinished, 
            this, &AiAssistantDialog::handleSummaryFinished);
    
    connect(m_aiService, &IAiService::fixFinished, 
            this, &AiAssistantDialog::handleFixFinished);
    
    connect(m_aiService, &IAiService::genericTextFinished, 
            this, &AiAssistantDialog::handleGenericTextFinished);
    
    // 连接错误信号
    connect(m_aiService, &IAiService::aiError, 
            this, &AiAssistantDialog::handleAiError);
}

// 处理润色完成
void AiAssistantDialog::handleRewriteFinished(const QString& originalText, const QString& rewrittenText)
{
    // 显示润色结果
    m_responseTextEdit->clear();
    m_responseTextEdit->append("<b>原文：</b>");
    m_responseTextEdit->append(originalText);
    m_responseTextEdit->append("\n<b>润色后：</b>");
    m_responseTextEdit->append(rewrittenText);
    
    // 保存生成的内容
    m_generatedContent = rewrittenText;
    
    // 启用按钮
    m_sendButton->setEnabled(true);
    m_functionButton->setEnabled(true);
    m_insertButton->setEnabled(true);
    m_retryButton->setEnabled(true);
}

// 处理总结完成
void AiAssistantDialog::handleSummaryFinished(const QString& originalText, const QString& summaryText)
{
    // 显示总结结果
    m_responseTextEdit->clear();
    m_responseTextEdit->append("<b>原文：</b>");
    m_responseTextEdit->append(originalText);
    m_responseTextEdit->append("\n<b>总结：</b>");
    m_responseTextEdit->append(summaryText);
    
    // 保存生成的内容
    m_generatedContent = summaryText;
    
    // 启用按钮
    m_sendButton->setEnabled(true);
    m_functionButton->setEnabled(true);
    m_insertButton->setEnabled(true);
    m_retryButton->setEnabled(true);
}

// 处理修复完成
void AiAssistantDialog::handleFixFinished(const QString& originalText, const QString& fixedText)
{
    // 显示修复结果
    m_responseTextEdit->clear();
    m_responseTextEdit->append("<b>原文：</b>");
    m_responseTextEdit->append(originalText);
    m_responseTextEdit->append("\n<b>修复后：</b>");
    m_responseTextEdit->append(fixedText);
    
    // 保存生成的内容
    m_generatedContent = fixedText;
    
    // 启用按钮
    m_sendButton->setEnabled(true);
    m_functionButton->setEnabled(true);
    m_insertButton->setEnabled(true);
    m_retryButton->setEnabled(true);
}

// 处理通用文本生成完成
void AiAssistantDialog::handleGenericTextFinished(const QString& prompt, const QString& generatedText)
{
    // 显示生成结果
    m_responseTextEdit->clear();
    
    // 根据当前功能调整显示方式
    if (m_currentFunction == "继续写作") {
        m_responseTextEdit->setText(generatedText);
    } else if (m_currentFunction == "内容扩写") {
        m_responseTextEdit->append("<b>原文：</b>");
        m_responseTextEdit->append(m_selectedText);
        m_responseTextEdit->append("\n<b>扩写后：</b>");
        m_responseTextEdit->append(generatedText);
    } else if (m_currentFunction == "内容精简") {
        m_responseTextEdit->append("<b>原文：</b>");
        m_responseTextEdit->append(m_selectedText);
        m_responseTextEdit->append("\n<b>精简后：</b>");
        m_responseTextEdit->append(generatedText);
    } else if (m_currentFunction == "语气转变") {
        m_responseTextEdit->append("<b>原文：</b>");
        m_responseTextEdit->append(m_selectedText);
        m_responseTextEdit->append(QString("\n<b>使用%1语气后：</b>").arg(m_currentTone));
        m_responseTextEdit->append(generatedText);
    } else {
        m_responseTextEdit->setText(generatedText);
    }
    
    // 保存生成的内容
    m_generatedContent = generatedText;
    
    // 启用按钮
    m_sendButton->setEnabled(true);
    m_functionButton->setEnabled(true);
    m_insertButton->setEnabled(true);
    m_retryButton->setEnabled(true);
}

// AI错误处理
void AiAssistantDialog::handleAiError(const QString& operationDescription, const QString& errorMessage)
{
    qWarning() << "AI错误:" << operationDescription << "-" << errorMessage;
    
    // 隐藏加载状态
    m_responseTextEdit->setReadOnly(false);
    
    // 构建更友好的错误消息
    QString friendlyError;
    QString detailedError;
    
    // 根据错误消息提供具体建议
    if (errorMessage.contains("API密钥无效") || errorMessage.contains("认证失败")) {
        friendlyError = "API密钥问题";
        detailedError = "您的DeepSeek API密钥可能无效或已过期。请检查设置中的API密钥配置。";
    } 
    else if (errorMessage.contains("超过API速率限制") || errorMessage.contains("超过API配额")) {
        friendlyError = "API使用限制";
        detailedError = "您已达到DeepSeek API的使用限制。请稍后再试或检查您的账户状态。";
    }
    else if (errorMessage.contains("请求超时") || errorMessage.contains("连接被拒绝") || 
             errorMessage.contains("网络") || errorMessage.contains("找不到服务器")) {
        friendlyError = "网络连接问题";
        detailedError = "无法连接到DeepSeek服务器。请检查您的网络连接或稍后再试。";
    }
    else if (errorMessage.contains("服务器内部错误") || errorMessage.contains("繁忙")) {
        friendlyError = "DeepSeek服务器问题";
        detailedError = "DeepSeek服务器当前可能存在问题。根据最近的报告，DeepSeek API可能正在经历服务中断。请稍后再试。";
    }
    else if (errorMessage.contains("服务未初始化")) {
        friendlyError = "AI服务未初始化";
        detailedError = "AI服务未能正确初始化。这可能是因为API密钥问题或DeepSeek服务当前不可用。";
    }
    else {
        friendlyError = "AI服务错误";
        detailedError = errorMessage;
    }
    
    // 记录错误
    qWarning() << "友好错误消息:" << friendlyError;
    qWarning() << "详细错误消息:" << detailedError;
    
    // 显示错误信息对话框
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
        errorBox.setStyleSheet("QMessageBox { background-color: #333; color: #fff; }"
                               "QLabel { color: #fff; }"
                               "QPushButton { background-color: #555; color: #fff; padding: 6px 12px; border-radius: 4px; }"
                               "QPushButton:hover { background-color: #666; }"
                               "QTextEdit { background-color: #444; color: #ddd; border: 1px solid #555; }");
    }
    
    errorBox.exec();
    
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