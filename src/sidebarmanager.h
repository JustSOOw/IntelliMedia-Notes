/*
 * @Author: Furdow wang22338014@gmail.com
 * @Date: 2025-04-17 12:00:00
 * @LastEditors: Furdow wang22338014@gmail.com
 * @LastEditTime: 2025-04-17 12:00:00
 * @FilePath: \IntelliMedia_Notes\src\sidebarmanager.h
 * @Description: 侧边栏管理器
 * 
 * Copyright (c) 2025 by Furdow, All Rights Reserved. 
 */
#ifndef SIDEBARMANAGER_H
#define SIDEBARMANAGER_H

#include <QObject>
#include <QtQuick/QQuickItem>
#include <QtQml/QQmlContext>
#include <QtQuickWidgets/QQuickWidget>
#include <QDir>
#include <QStandardPaths>
#include <QDateTime>
#include <QDebug>

class SidebarManager : public QObject
{
    Q_OBJECT
    
public:
    explicit SidebarManager(QQuickWidget *quickWidget, QObject *parent = nullptr);
    ~SidebarManager();
    
    // 初始化侧边栏
    void initialize();
    
    // 获取用户名称
    Q_INVOKABLE QString getUserName() const;
    
    // 获取用户状态
    Q_INVOKABLE QString getUserStatus() const;
    
    // 获取根目录路径
    Q_INVOKABLE QString getRootPath() const;
    
public slots:
    // 处理QML中的信号
    void onNoteSelected(const QString &path, const QString &type);
    void onCreateNote(const QString &parentPath);
    void onCreateFolder(const QString &parentPath);
    void onRenameItem(const QString &path, const QString &newName);
    void onDeleteItem(const QString &path);
    void onSendAIMessage(const QString &message);
    
signals:
    // 向QML发送信号
    void noteOpened(const QString &path, const QString &content);
    void aiMessageReceived(const QString &message);
    
private:
    QQuickWidget *m_quickWidget; // QQuickWidget实例的引用
    QQuickItem *m_rootObject;    // QML根对象
    QString m_rootPath;          // 笔记根目录路径
    
    // 初始化笔记存储目录
    void initializeNoteStorage();
    
    // 测试用的AI回复消息
    QString getTestAIResponse(const QString &userMessage);
};

#endif // SIDEBARMANAGER_H 