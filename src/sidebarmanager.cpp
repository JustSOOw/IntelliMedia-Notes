/*
 * @Author: Furdow wang22338014@gmail.com
 * @Date: 2025-04-17 12:00:00
 * @LastEditors: Furdow wang22338014@gmail.com
 * @LastEditTime: 2025-04-17 12:00:00
 * @FilePath: \IntelliMedia_Notes\src\sidebarmanager.cpp
 * @Description: 侧边栏管理器实现
 * 
 * Copyright (c) 2025 by Furdow, All Rights Reserved. 
 */
#include "sidebarmanager.h"
#include <QFileInfo>
#include <QFile>
#include <QTextStream>
#include <QInputDialog>
#include <QMessageBox>

// 构造函数
SidebarManager::SidebarManager(QQuickWidget *quickWidget, QObject *parent)
    : QObject(parent)
    , m_quickWidget(quickWidget)
    , m_rootObject(nullptr)
{
    // 初始化笔记存储
    initializeNoteStorage();
}

// 析构函数
SidebarManager::~SidebarManager()
{
}

// 初始化侧边栏
void SidebarManager::initialize()
{
    // 设置QML上下文属性
    QQmlContext *context = m_quickWidget->rootContext();
    context->setContextProperty("sidebarManager", this);
    
    // 设置QML源文件
    m_quickWidget->setSource(QUrl("qrc:/qml/Sidebar.qml"));
    
    // 获取QML根对象
    m_rootObject = m_quickWidget->rootObject();
    
    if (m_rootObject) {
        // 连接QML信号到C++槽
        QObject::connect(m_rootObject, SIGNAL(noteSelected(QString,QString)),
                        this, SLOT(onNoteSelected(QString,QString)));
        QObject::connect(m_rootObject, SIGNAL(createNote(QString)),
                        this, SLOT(onCreateNote(QString)));
        QObject::connect(m_rootObject, SIGNAL(createFolder(QString)),
                        this, SLOT(onCreateFolder(QString)));
        QObject::connect(m_rootObject, SIGNAL(renameItem(QString,QString)),
                        this, SLOT(onRenameItem(QString,QString)));
        QObject::connect(m_rootObject, SIGNAL(deleteItem(QString)),
                        this, SLOT(onDeleteItem(QString)));
        QObject::connect(m_rootObject, SIGNAL(sendAIMessage(QString)),
                        this, SLOT(onSendAIMessage(QString)));
    } else {
        qWarning() << "无法获取QML根对象!";
    }
}

// 获取用户名称
QString SidebarManager::getUserName() const
{
    return "15039630768"; // 固定用户名
}

// 获取用户状态
QString SidebarManager::getUserStatus() const
{
    return "在线"; // 固定状态
}

// 获取根目录路径
QString SidebarManager::getRootPath() const
{
    return m_rootPath;
}

// 笔记被选中
void SidebarManager::onNoteSelected(const QString &path, const QString &type)
{
    qDebug() << "笔记选中:" << path << "类型:" << type;
    
    // 如果是普通笔记文件，读取内容并发送信号
    if (type == "note") {
        QFile file(path);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            QString content = in.readAll();
            file.close();
            
            emit noteOpened(path, content);
        } else {
            qWarning() << "无法打开笔记文件:" << path;
        }
    }
}

// 创建新笔记
void SidebarManager::onCreateNote(const QString &parentPath)
{
    qDebug() << "创建笔记在:" << parentPath;
    
    // 实际实现时，弹出对话框请求用户输入笔记名称
    // 然后在指定路径创建文件
    bool ok;
    QString noteName = QInputDialog::getText(nullptr, 
                                           "创建新笔记", 
                                           "请输入笔记名称:", 
                                           QLineEdit::Normal,
                                           "新笔记",
                                           &ok);
    
    if (ok && !noteName.isEmpty()) {
        // 创建笔记文件
        // 此处省略具体实现
        qDebug() << "创建笔记:" << noteName << "在:" << parentPath;
        // TODO: 实现笔记文件的创建
    }
}

// 创建新文件夹
void SidebarManager::onCreateFolder(const QString &parentPath)
{
    qDebug() << "创建文件夹在:" << parentPath;
    
    // 实际实现时，弹出对话框请求用户输入文件夹名称
    // 然后在指定路径创建文件夹
    bool ok;
    QString folderName = QInputDialog::getText(nullptr,
                                             "创建新文件夹",
                                             "请输入文件夹名称:",
                                             QLineEdit::Normal,
                                             "新文件夹",
                                             &ok);
    
    if (ok && !folderName.isEmpty()) {
        // 创建文件夹
        // 此处省略具体实现
        qDebug() << "创建文件夹:" << folderName << "在:" << parentPath;
        // TODO: 实现文件夹的创建
    }
}

// 重命名项目
void SidebarManager::onRenameItem(const QString &path, const QString &newName)
{
    qDebug() << "重命名:" << path << "为:" << newName;
    
    // 实际实现时，重命名文件或文件夹
    // TODO: 实现重命名操作
}

// 删除项目
void SidebarManager::onDeleteItem(const QString &path)
{
    qDebug() << "删除:" << path;
    
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(nullptr, "确认删除", 
                                 "确定要删除该项目吗？此操作不可恢复。",
                                 QMessageBox::Yes|QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        // 实际删除文件或文件夹
        // TODO: 实现删除操作
        qDebug() << "已确认删除:" << path;
    }
}

// 发送AI消息
void SidebarManager::onSendAIMessage(const QString &message)
{
    qDebug() << "发送AI消息:" << message;
    
    // 简单模拟AI响应
    QString response = getTestAIResponse(message);
    
    // 返回AI回复
    emit aiMessageReceived(response);
}

// 初始化笔记存储目录
void SidebarManager::initializeNoteStorage()
{
    // 使用应用程序数据目录作为笔记的根目录
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    m_rootPath = appDataPath + "/notes";
    
    // 确保目录存在
    QDir dir;
    if (!dir.exists(m_rootPath)) {
        dir.mkpath(m_rootPath);
    }
    
    qDebug() << "笔记存储目录:" << m_rootPath;
}

// 测试用的AI回复消息
QString SidebarManager::getTestAIResponse(const QString &userMessage)
{
    // 简单的回复逻辑，实际应用中可以接入真正的AI服务
    if (userMessage.contains("你好") || userMessage.contains("您好")) {
        return "你好！我是您的AI助手，有什么可以帮助您的？";
    } else if (userMessage.contains("天气")) {
        return "抱歉，我没有联网功能，无法查询天气信息。";
    } else if (userMessage.contains("帮助") || userMessage.contains("能做什么")) {
        return "我可以帮助您管理笔记、回答简单问题，或者提供一些建议。";
    } else if (userMessage.length() < 5) {
        return "请提供更详细的信息，这样我才能更好地帮助您。";
    } else {
        return "我收到了您的消息：\"" + userMessage + "\"。我正在学习中，希望能更好地为您服务。";
    }
} 