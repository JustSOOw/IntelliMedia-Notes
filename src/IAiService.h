/*
 * @Author: cursor AI
 * @Date: 2025-05-13 09:30:00
 * @LastEditors: cursor AI
 * @LastEditTime: 2025-05-13 09:30:00
 * @FilePath: \IntelliMedia_Notes\src\IAiService.h
 * @Description: AI服务接口定义
 * 
 * Copyright (c) 2025, All Rights Reserved. 
 */
#ifndef IAISERVICE_H
#define IAISERVICE_H

#include <QObject>
#include <QString>

/**
 * @brief AI服务接口类
 * 定义了应用程序需要的所有AI操作和用于异步返回结果的信号
 */
class IAiService : public QObject
{
    Q_OBJECT

public:
    IAiService() = default;
    explicit IAiService(QObject *parent) : QObject(parent) {}
    virtual ~IAiService() = default;

    /**
     * @brief 设置API密钥
     * @param apiKey API密钥
     */
    virtual void setApiKey(const QString& apiKey) = 0;

    /**
     * @brief 润色文本
     * @param textToRewrite 需要润色的文本
     */
    virtual void rewriteText(const QString& textToRewrite) = 0;

    /**
     * @brief 总结文本
     * @param textToSummarize 需要总结的文本
     */
    virtual void summarizeText(const QString& textToSummarize) = 0;

    /**
     * @brief 修复文本(语法纠错等)
     * @param textToFix 需要修复的文本
     */
    virtual void fixText(const QString& textToFix) = 0;

    /**
     * @brief 生成通用文本(用于侧边栏通用对话)
     * @param prompt 用户输入的提示词
     */
    virtual void generateGenericText(const QString& prompt) = 0;

signals:
    /**
     * @brief 润色完成信号
     * @param originalText 原始文本
     * @param rewrittenText 润色后的文本
     */
    void rewriteFinished(const QString& originalText, const QString& rewrittenText);

    /**
     * @brief 总结完成信号
     * @param originalText 原始文本
     * @param summaryText 总结后的文本
     */
    void summaryFinished(const QString& originalText, const QString& summaryText);

    /**
     * @brief 修复完成信号
     * @param originalText 原始文本
     * @param fixedText 修复后的文本
     */
    void fixFinished(const QString& originalText, const QString& fixedText);

    /**
     * @brief 通用文本生成完成信号
     * @param prompt 用户输入的提示词
     * @param generatedText 生成的文本
     */
    void genericTextFinished(const QString& prompt, const QString& generatedText);

    /**
     * @brief AI错误信号
     * @param operationDescription 操作描述(如 "润色", "总结", "通用对话" 等)
     * @param errorMessage 错误信息
     */
    void aiError(const QString& operationDescription, const QString& errorMessage);
};

#endif // IAISERVICE_H 