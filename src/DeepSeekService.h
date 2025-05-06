/*
 * @Author: cursor AI
 * @Date: 2025-05-13 09:35:00
 * @LastEditors: cursor AI
 * @LastEditTime: 2025-05-13 09:35:00
 * @FilePath: \IntelliMedia_Notes\src\DeepSeekService.h
 * @Description: DeepSeek API服务实现
 * 
 * Copyright (c) 2025, All Rights Reserved. 
 */
#ifndef DEEPSEEKSERVICE_H
#define DEEPSEEKSERVICE_H

#include "IAiService.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <QTimer>
#include <QMap>

/**
 * @brief DeepSeek API服务实现类
 * 封装与DeepSeek API交互的所有细节
 */
class DeepSeekService : public IAiService
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit DeepSeekService(QObject *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~DeepSeekService() override;

    /**
     * @brief 设置API密钥
     * @param apiKey DeepSeek API密钥
     */
    void setApiKey(const QString& apiKey) override;

    /**
     * @brief 设置API端点URL
     * @param apiEndpoint DeepSeek API端点URL
     */
    void setApiEndpoint(const QString& apiEndpoint);

    /**
     * @brief 润色文本
     * @param textToRewrite 需要润色的文本
     */
    void rewriteText(const QString& textToRewrite) override;

    /**
     * @brief 总结文本
     * @param textToSummarize 需要总结的文本
     */
    void summarizeText(const QString& textToSummarize) override;

    /**
     * @brief 修复文本(语法纠错等)
     * @param textToFix 需要修复的文本
     */
    void fixText(const QString& textToFix) override;

    /**
     * @brief 生成通用文本(用于侧边栏通用对话)
     * @param prompt 用户输入的提示词
     */
    void generateGenericText(const QString& prompt) override;

private slots:
    /**
     * @brief 处理网络请求完成的响应
     * @param reply 网络响应对象
     */
    void handleNetworkReply(QNetworkReply *reply);

private:
    /**
     * @brief 发送请求到DeepSeek API
     * @param operation 操作类型标识符(如 "rewrite", "summarize", "fix", "generic")
     * @param originalText 原始文本
     * @param systemPrompt 系统提示词
     */
    void sendRequest(const QString& operation, const QString& originalText, const QString& systemPrompt);

    /**
     * @brief 构建请求体JSON
     * @param systemPrompt 系统提示词
     * @param userPrompt 用户提示词
     * @return 请求体JSON对象
     */
    QJsonObject buildRequestPayload(const QString& systemPrompt, const QString& userPrompt);

    /**
     * @brief 解析API响应
     * @param jsonResponse API响应的JSON数据
     * @return 生成的文本内容
     */
    QString parseResponse(const QByteArray& jsonResponse);

    /**
     * @brief 将操作类型转换为描述性文字
     * @param operation 操作类型
     * @return 对应的描述性文字
     */
    QString operationToDescription(const QString& operation);

    QNetworkAccessManager *m_networkManager; // 网络请求管理器
    QString m_apiKey;                       // DeepSeek API密钥
    QString m_apiEndpoint;                  // API端点URL
    QString m_modelName;                    // 模型名称
    QMap<QNetworkReply*, QString> m_activeReplies; // 声明 activeReplies
};

#endif // DEEPSEEKSERVICE_H 