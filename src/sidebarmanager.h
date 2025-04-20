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
#include <QJsonArray>
#include <QJsonObject>
#include <QVariantList>
#include <QVariantMap>
#include "databasemanager.h" // 添加数据库管理器头文件

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
    
    // 获取文件夹结构（供QML模型使用）
    Q_INVOKABLE QVariantList getFolderStructure();
    
    // 获取文件夹内容（供QML模型使用）
    Q_INVOKABLE QVariantList getFolderContents(int folder_id, int parentLevel);
    
    // 供QML直接调用的方法
    Q_INVOKABLE bool deleteItem(const QString &path);
    Q_INVOKABLE bool renameItem(const QString &path, const QString &newName);
    Q_INVOKABLE int createNote(const QString &parentPath, const QString &noteName);
    Q_INVOKABLE int createFolder(const QString &parentPath, const QString &folderName);
    
public slots:
    // 处理QML中的信号
    void onNoteSelected(const QString &path, const QString &type);
    void onCreateNote(const QString &parentPath, const QString &noteName = "新笔记");
    void onCreateFolder(const QString &parentPath, const QString &folderName = "新文件夹");
    void onRenameItem(const QString &path, const QString &newName);
    void onDeleteItem(const QString &path);
    void onSendAIMessage(const QString &message);
    
    // 刷新文件列表
    void refreshNotesList();
    
signals:
    // 向QML发送信号
    void noteOpened(const QString &path, const QString &content);
    void aiMessageReceived(const QString &message);
    void folderStructureChanged(); // 文件夹结构变化信号
    
private:
    QQuickWidget *m_quickWidget; // QQuickWidget实例的引用
    QQuickItem *m_rootObject;    // QML根对象
    QString m_rootPath;          // 笔记根目录路径
    DatabaseManager *m_dbManager; // 数据库管理器
    
    // 初始化笔记存储目录
    void initializeNoteStorage();
    
    // 辅助函数：递归获取文件夹内容并添加到列表
    void appendFolderChildren(int parentFolderId, int currentLevel, QVariantList &list);
    
    // 将数据库FolderInfo转换为QML可用的格式
    QVariantMap folderToQML(const FolderInfo &folder, int level, bool expanded);
    
    // 将数据库NoteInfo转换为QML可用的格式
    QVariantMap noteToQML(const NoteInfo &note, int level);
    
    // 从路径中提取ID（例如"/folder_1/note_2"提取为2）
    int extractIdFromPath(const QString &path);
    
    // 测试用的AI回复消息
    QString getTestAIResponse(const QString &userMessage);
};

#endif // SIDEBARMANAGER_H 