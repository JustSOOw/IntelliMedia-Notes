/*
 * @Author: cursor AI
 * @Date: 2025-05-12 10:00:00
 * @LastEditors: cursor AI
 * @LastEditTime: 2025-05-12 10:00:00
 * @FilePath: \IntelliMedia_Notes\src\aiassistantdialog.h
 * @Description: AI助手对话框，用于处理文本编辑时的AI交互
 * 
 * Copyright (c) 2025, All Rights Reserved. 
 */
#ifndef AIASSISTANTDIALOG_H
#define AIASSISTANTDIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QJsonDocument>
#include <QPoint>
#include <QMenu>
#include <QAction>
#include <QToolButton>

// AI助手对话框，用于实现与AI的交互
class AiAssistantDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AiAssistantDialog(QWidget *parent = nullptr);
    ~AiAssistantDialog();

    // 设置当前选中的文本，用于发送给AI处理
    void setSelectedText(const QString &text) { m_selectedText = text; }
    
    // 获取AI生成的内容
    QString getGeneratedContent() const { return m_generatedContent; }
    
    // 设置对话框主题（深色/浅色）
    void setDarkTheme(bool dark);

signals:
    // 插入AI生成的内容到文档信号
    void insertContentToDocument(const QString &content);
    
    // 对话框关闭信号
    void dialogClosed();

protected:
    // 重写事件处理
    void showEvent(QShowEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    // 发送消息到AI
    void sendMessage();
    
    // 功能选择处理
    void onFunctionSelected(QAction* action);
    
    // 语气选择处理
    void onToneSelected(QAction* action);
    
    // 底部按钮处理
    void onInsertButtonClicked();
    void onRetryButtonClicked();
    void onCancelButtonClicked();
    
    // 处理AI响应（模拟阶段）
    void processAiResponse();

private:
    // UI组件
    QLineEdit *m_inputLineEdit;      // 单行输入框
    QTextEdit *m_responseTextEdit;   // AI响应显示区域
    QToolButton *m_functionButton;   // 功能按钮
    QMenu *m_functionMenu;           // 功能菜单
    QMenu *m_toneSubMenu;            // 语气子菜单
    QPushButton *m_sendButton;       // 发送按钮
    QPushButton *m_insertButton;     // 插入到文档按钮
    QPushButton *m_retryButton;      // 重新尝试按钮
    QPushButton *m_cancelButton;     // 取消按钮
    QLabel *m_titleLabel;            // 标题标签
    
    // 功能和语气选项
    QAction *m_continueWritingAction;  // 继续写作
    QAction *m_polishContentAction;    // 内容润色
    QAction *m_expandContentAction;    // 内容扩写
    QAction *m_simplifyContentAction;  // 内容精简
    QAction *m_grammarCheckAction;     // 语法纠错
    QAction *m_changeToneAction;       // 语气转变
    
    // 语气子菜单项
    QAction *m_professionalToneAction; // 专业语气
    QAction *m_casualToneAction;       // 日常语气
    QAction *m_honestToneAction;       // 坦诚语气
    QAction *m_confidentToneAction;    // 自信语气
    QAction *m_humorousToneAction;     // 幽默语气
    
    // 数据成员
    QString m_selectedText;          // 编辑器中选中的文本
    QString m_generatedContent;      // AI生成的内容
    bool m_isDarkTheme;              // 当前主题状态
    QString m_currentFunction;       // 当前选中的功能
    QString m_currentTone;           // 当前选中的语气
    
    // 拖拽相关
    bool m_dragging;                 // 是否正在拖拽
    QPoint m_dragPosition;           // 拖拽起始位置
    
    // 配置相关
    QJsonObject m_aiConfig;          // AI配置（功能、提示词等）
    
    // UI辅助函数
    void setupUI();                  // 设置UI
    void setupFunctionMenu();        // 设置功能菜单
    void updateFunctionButtonText(); // 更新功能按钮文本
    void loadConfigurations();       // 加载配置
    void updateStyles();             // 更新样式
    
    // 根据主题获取颜色
    QColor getBackgroundColor() const;
    QColor getTextColor() const;
    QColor getBorderColor() const;
    QColor getInputBackgroundColor() const;
    QColor getButtonColor() const;
    
    // 模拟AI响应（临时实现）
    QString simulateAiResponse(const QString &userInput, const QString &function, const QString &tone);
};

#endif // AIASSISTANTDIALOG_H 