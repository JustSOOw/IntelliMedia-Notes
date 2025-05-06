/*
 * @Author: cursor AI
 * @Date: 2025-05-13 09:40:00
 * @LastEditors: Furdow wang22338014@gmail.com
 * @LastEditTime: 2025-05-06 15:24:13
 * @FilePath: \IntelliMedia_Notes\src\DeepSeekService.cpp
 * @Description: DeepSeek API服务实现
 * 
 * Copyright (c) 2025, All Rights Reserved. 
 */
#include "DeepSeekService.h"
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

// DeepSeek API 常量
const QString API_ENDPOINT = "https://api.deepseek.com/chat/completions";
const QString MODEL_NAME = "deepseek-chat"; // DeepSeek-V3 模型
const QString DEFAULT_API_KEY = "sk-99853683dd524fda89a8fda9f0447e09"; // 请替换为实际的DeepSeek API密钥

// 构造函数
DeepSeekService::DeepSeekService(QObject *parent)
    : IAiService(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_apiKey(DEFAULT_API_KEY)
    , m_apiEndpoint(API_ENDPOINT)
    , m_modelName(MODEL_NAME)
{
    qDebug() << "DeepSeekService初始化开始";
    qDebug() << "API终端:" << m_apiEndpoint;
    qDebug() << "模型名称:" << m_modelName;
    
    // 连接网络管理器的finished信号到处理槽函数
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, &DeepSeekService::handleNetworkReply);
    
    qDebug() << "DeepSeekService初始化完成 (已移除启动时API测试)";
}

// 析构函数
DeepSeekService::~DeepSeekService()
{
    // QNetworkAccessManager会随对象删除而自动删除
}

// 设置API密钥
void DeepSeekService::setApiKey(const QString& apiKey)
{
    m_apiKey = apiKey;
}

// 设置API端点URL
void DeepSeekService::setApiEndpoint(const QString& apiEndpoint)
{
    // 验证API端点URL格式
    if (!apiEndpoint.isEmpty()) {
        m_apiEndpoint = apiEndpoint;
        qDebug() << "成功设置API端点URL:" << m_apiEndpoint;
    } else {
        qWarning() << "尝试设置空的API端点URL，将继续使用当前端点:" << m_apiEndpoint;
    }
}

// 润色文本
void DeepSeekService::rewriteText(const QString& textToRewrite)
{
    if (textToRewrite.isEmpty()) {
        emit aiError("润色", "文本内容为空，无法处理");
        return;
    }

    QString systemPrompt = "你是一位专业的文字编辑。请对用户提供的文本进行润色，提高其表达质量和专业度，但保持原意不变。使改进后的文本更加流畅、清晰且表达更准确。";
    sendRequest("rewrite", textToRewrite, systemPrompt);
}

// 总结文本
void DeepSeekService::summarizeText(const QString& textToSummarize)
{
    if (textToSummarize.isEmpty()) {
        emit aiError("总结", "文本内容为空，无法处理");
        return;
    }

    QString systemPrompt = "你是一位专业的文字编辑。请对用户提供的文本进行简明扼要的总结，保留文本的关键信息和核心观点。总结应该简洁明了，突出文本的主要内容。";
    sendRequest("summarize", textToSummarize, systemPrompt);
}

// 修复文本
void DeepSeekService::fixText(const QString& textToFix)
{
    if (textToFix.isEmpty()) {
        emit aiError("修复", "文本内容为空，无法处理");
        return;
    }

    QString systemPrompt = "你是一位专业的文字编辑和语法专家。请检查并修复用户提供文本中的语法错误、拼写错误和表达不当，使文本更加准确和符合规范。";
    sendRequest("fix", textToFix, systemPrompt);
}

// 生成通用文本
void DeepSeekService::generateGenericText(const QString& prompt)
{
    if (prompt.isEmpty()) {
        emit aiError("通用对话", "提示词为空，无法处理");
        return;
    }

    QString systemPrompt = "你是一位全能的AI助手，能够提供有用、准确、翔实的回答，帮助用户解决各种问题。";
    sendRequest("generic", prompt, systemPrompt);
}

// 发送请求到DeepSeek API
void DeepSeekService::sendRequest(const QString& operation, const QString& originalText, const QString& systemPrompt)
{
    // 详细记录操作
    qDebug() << "发送" << operationToDescription(operation) << "请求，文本长度:" << originalText.length();
    
    // 参数验证
    if (operation.isEmpty()) {
        emit aiError(operationToDescription(operation), "操作类型为空");
        return;
    }
    if (originalText.isEmpty()) {
        emit aiError(operationToDescription(operation), "原始文本为空");
        return;
    }
    if (systemPrompt.isEmpty()) {
        emit aiError(operationToDescription(operation), "系统提示词为空");
        return;
    }

    // 检查API密钥
    if (m_apiKey.isEmpty()) {
        emit aiError(operationToDescription(operation), "API密钥未设置，请在设置中配置有效的DeepSeek API密钥");
        return;
    }
    
    // 检查是否使用了默认API密钥
    if (m_apiKey == DEFAULT_API_KEY) {
        // 如果默认密钥不是一个有效的密钥格式，发出警告
        if (!m_apiKey.startsWith("sk-") || m_apiKey.length() < 20) {
            emit aiError(operationToDescription(operation), "使用了默认API密钥，请在设置中配置有效的DeepSeek API密钥");
            return;
        } else {
            qWarning() << "使用默认API密钥！建议更换为您自己的API密钥";
        }
    }
    
    // 检查API密钥格式
    if (!m_apiKey.startsWith("sk-") || m_apiKey.length() < 20) {
        emit aiError(operationToDescription(operation), "API密钥格式不正确，有效的DeepSeek API密钥应以'sk-'开头");
        return;
    }

    // 检查API端点
    if (m_apiEndpoint.isEmpty() || !m_apiEndpoint.startsWith("http")) {
        emit aiError(operationToDescription(operation), "API端点URL无效");
        return;
    }

    // 创建请求URL
    QUrl url(m_apiEndpoint);
    QNetworkRequest request(url);

    // 设置请求头
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_apiKey).toUtf8());

    // 创建请求体 (Payload)
    QJsonObject payload;
    payload["model"] = m_modelName;

    QJsonArray messages;
    // 添加系统提示
    QJsonObject systemMessage;
    systemMessage["role"] = "system";
    systemMessage["content"] = systemPrompt;
    messages.append(systemMessage);

    // 添加用户请求
    QJsonObject userMessage;
    userMessage["role"] = "user";
    userMessage["content"] = originalText;
    messages.append(userMessage);

    payload["messages"] = messages;
    
    // 根据需要设置其他参数 (例如 max_tokens, temperature)
    // payload["max_tokens"] = 4096; // 示例：可以根据需要调整最大令牌数
    // payload["temperature"] = 0.7; // 示例：可以调整创造性

    QJsonDocument doc(payload);
    QByteArray data = doc.toJson();

    qDebug() << "请求URL:" << url.toString();
    qDebug() << "请求负载:" << QString::fromUtf8(data).left(200) << "..."; // 打印部分负载用于调试

    // 发送POST请求
    QNetworkReply *reply = m_networkManager->post(request, data);
    
    // **增加超时设置**
    // 设置一个较长的超时时间，例如 120 秒 (120000 毫秒)
    QTimer::singleShot(120000, reply, [this, reply, operation]() { 
        if (reply && reply->isRunning()) {
             qWarning() << operationToDescription(operation) << "请求超时 (120s)，中止请求";
             reply->abort(); // 中止请求
        }
    });

    // 存储操作类型，用于后续处理响应
    m_activeReplies.insert(reply, operation);
    qDebug() << "请求已发送到DeepSeek API";
}

// 处理网络响应
void DeepSeekService::handleNetworkReply(QNetworkReply *reply)
{
    // 确保reply被删除
    reply->deleteLater(); 
    
    // 从 m_activeReplies 中获取操作类型并移除该条目
    if (!m_activeReplies.contains(reply)) {
        qWarning() << "收到未追踪的网络响应，忽略。URL:" << (reply ? reply->url().toString() : "N/A");
        return;
    }
    QString operation = m_activeReplies.take(reply);
    QString operationDesc = operationToDescription(operation);
    QString requestUrl = reply->url().toString();

    // 记录响应信息
    qDebug() << "收到API响应，操作:" << operationDesc 
             << "请求URL:" << requestUrl
             << "HTTP状态码:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    // 检查网络错误
    if (reply->error() != QNetworkReply::NoError) {
        QString errorMsg;
        
        // 记录详细错误信息
        qWarning() << "API请求错误:" 
                  << "请求URL:" << requestUrl
                  << "错误代码:" << reply->error()
                  << "错误字符串:" << reply->errorString()
                  << "HTTP状态码:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        
        // 根据错误类型提供更具体的错误信息
        switch (reply->error()) {
            case QNetworkReply::ConnectionRefusedError:
                errorMsg = "连接被拒绝，请检查网络连接";
                break;
            case QNetworkReply::RemoteHostClosedError:
                errorMsg = "远程服务器关闭了连接";
                break;
            case QNetworkReply::HostNotFoundError:
                errorMsg = "找不到服务器地址，请检查API端点配置";
                break;
            case QNetworkReply::TimeoutError:
                errorMsg = "请求超时，DeepSeek服务器可能繁忙或无法访问";
                break;
            case QNetworkReply::OperationCanceledError:
                // 区分是用户取消还是超时取消
                if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).isNull()) {
                    // 如果没有HTTP状态码，很可能是定时器触发的abort
                    errorMsg = "请求超时 (120s)"; 
                } else {
                    errorMsg = "请求被取消"; // 其他原因的取消
                }
                break;
            case QNetworkReply::SslHandshakeFailedError:
                errorMsg = "SSL握手失败，可能是网络安全问题";
                break;
            case QNetworkReply::TemporaryNetworkFailureError:
                errorMsg = "临时网络故障，请稍后重试";
                break;
            case QNetworkReply::NetworkSessionFailedError:
                errorMsg = "网络会话失败，请检查网络连接";
                break;
            case QNetworkReply::BackgroundRequestNotAllowedError:
                errorMsg = "后台请求不被允许";
                break;
            case QNetworkReply::TooManyRedirectsError:
                errorMsg = "重定向次数过多";
                break;
            case QNetworkReply::InsecureRedirectError:
                errorMsg = "不安全的重定向";
                break;
            case QNetworkReply::ProxyConnectionRefusedError:
            case QNetworkReply::ProxyConnectionClosedError:
            case QNetworkReply::ProxyNotFoundError:
            case QNetworkReply::ProxyTimeoutError:
            case QNetworkReply::ProxyAuthenticationRequiredError:
                errorMsg = "代理服务器错误，请检查网络代理设置";
                break;
            case QNetworkReply::ContentAccessDenied:
            case QNetworkReply::ContentOperationNotPermittedError:
            case QNetworkReply::ContentNotFoundError:
                errorMsg = "内容访问错误，API服务可能有问题";
                break;
            case QNetworkReply::AuthenticationRequiredError:
                errorMsg = "认证失败，API密钥可能无效或已过期";
                break;
            case QNetworkReply::ContentReSendError:
                errorMsg = "内容重发错误";
                break;
            case QNetworkReply::ContentConflictError:
            case QNetworkReply::ContentGoneError:
                errorMsg = "内容冲突或不可用";
                break;
            case QNetworkReply::InternalServerError:
                errorMsg = "DeepSeek服务器内部错误，请稍后重试";
                break;
            case QNetworkReply::UnknownNetworkError:
            default:
                errorMsg = QString("未知网络错误: %1").arg(reply->errorString());
                break;
        }
        
        qWarning() << "AI请求错误(" << operationDesc << ") :" << errorMsg;
        qWarning() << "HTTP状态码:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        qWarning() << "错误字符串:" << reply->errorString();
        
        emit aiError(operationDesc, errorMsg);
        return;
    }

    // 读取响应数据
    QByteArray responseData = reply->readAll();

    // 检查响应数据是否为空
    if (responseData.isEmpty()) {
        qWarning() << "DeepSeek服务器返回空响应";
        emit aiError(operationDesc, "服务器返回的数据为空");
        return;
    }

    try {
        qDebug() << "收到API响应，开始解析... (" << operationDesc << ")";
        
        // 输出响应头信息，帮助调试
        QList<QByteArray> headerList = reply->rawHeaderList();
        for (const QByteArray &header : headerList) {
            qDebug() << "响应头:" << header << ":" << reply->rawHeader(header);
        }
        
        // 解析响应数据并提取生成的文本
        QString generatedText = parseResponse(responseData);
        qDebug() << "解析成功，生成文本长度:" << generatedText.length() << "(" << operationDesc << ")";

        // 根据操作类型发出对应的完成信号
        if (operation == "rewrite") {
            // emit rewriteFinished(originalText, generatedText); // 需要 originalText
            emit genericTextFinished("", generatedText); // 暂时用通用信号替代，或者修改信号
        } else if (operation == "summarize") {
            // emit summaryFinished(originalText, generatedText); // 需要 originalText
            emit genericTextFinished("", generatedText); // 暂时用通用信号替代
        } else if (operation == "fix") {
            // emit fixFinished(originalText, generatedText); // 需要 originalText
            emit genericTextFinished("", generatedText); // 暂时用通用信号替代
        } else if (operation == "generic") {
            // emit genericTextFinished(originalText, generatedText); // 需要 originalText
            emit genericTextFinished("", generatedText); // 暂时用通用信号替代，第二个参数是生成的文本
        } else {
             qWarning() << "未知的操作类型，无法发出完成信号:" << operation;
        }
    } catch (const std::exception& e) {
        // 捕获解析过程中的异常
        QString errorMessage = QString("处理API响应时出错 (%1): %2").arg(operationDesc).arg(e.what());
        qWarning() << errorMessage;
        qWarning() << "原始响应:" << responseData;
        emit aiError(operationDesc, errorMessage);
    }
}

// 构建请求体JSON
QJsonObject DeepSeekService::buildRequestPayload(const QString& systemPrompt, const QString& userPrompt)
{
    QJsonObject payload;
    payload["model"] = m_modelName;
    
    // 设置温度参数为0.7，保持一定的创造性但不会过于随机
    payload["temperature"] = 0.7;
    
    // 消息数组
    QJsonArray messages;
    
    // 系统消息
    QJsonObject systemMessage;
    systemMessage["role"] = "system";
    systemMessage["content"] = systemPrompt;
    messages.append(systemMessage);
    
    // 用户消息
    QJsonObject userMessage;
    userMessage["role"] = "user";
    userMessage["content"] = userPrompt;
    messages.append(userMessage);
    
    payload["messages"] = messages;
    
    return payload;
}

// 解析API响应
QString DeepSeekService::parseResponse(const QByteArray& jsonResponse)
{
    qDebug() << "开始解析API响应";
    
    // 输出响应的前200个字符（调试用）
    QString truncatedResponse = jsonResponse.size() > 200 
        ? QString(jsonResponse.left(200) + "...") 
        : QString(jsonResponse);
    qDebug() << "API响应预览: " << truncatedResponse;
    
    // 解析JSON响应
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonResponse, &parseError);
    
    // 检查JSON解析错误
    if (parseError.error != QJsonParseError::NoError) {
        QString errorMsg = QString("JSON解析错误: %1").arg(parseError.errorString());
        qWarning() << errorMsg;
        qWarning() << "错误位置: " << parseError.offset;
        throw std::runtime_error(errorMsg.toStdString());
    }
    
    // 检查响应是否为JSON对象
    if (!doc.isObject()) {
        QString errorMsg = "无效的JSON响应格式: 预期是JSON对象";
        qWarning() << errorMsg;
        throw std::runtime_error(errorMsg.toStdString());
    }
    
    QJsonObject responseObj = doc.object();
    
    // 检查是否包含错误信息
    if (responseObj.contains("error")) {
        QJsonObject errorObj = responseObj["error"].toObject();
        QString errorType = errorObj["type"].toString();
        QString errorMessage = errorObj["message"].toString();
        QString errorCode = errorObj["code"].toString();
        
        QString detailedError = QString("API错误: 类型=%1, 代码=%2, 消息=%3")
                               .arg(errorType)
                               .arg(errorCode)
                               .arg(errorMessage);
        qWarning() << detailedError;
        
        // 特定错误的处理
        if (errorType == "invalid_request_error") {
            if (errorMessage.contains("API key")) {
                throw std::runtime_error("API密钥无效或已过期，请检查您的API密钥设置");
            } else if (errorMessage.contains("rate limit")) {
                throw std::runtime_error("超过API速率限制，请稍后重试");
            }
        } else if (errorType == "server_error") {
            throw std::runtime_error("DeepSeek服务器内部错误，请稍后重试");
        } else if (errorType == "quota_exceeded") {
            throw std::runtime_error("已超过API配额限制，请检查您的账户余额");
        }
        
        // 通用错误处理
        throw std::runtime_error(detailedError.toStdString());
    }
    
    // 检查响应状态码（如果有）
    if (responseObj.contains("status")) {
        int statusCode = responseObj["status"].toInt();
        if (statusCode != 200) {
            QString message = responseObj.contains("message") 
                ? responseObj["message"].toString() 
                : "未知错误";
            
            QString statusError = QString("API返回非成功状态码: %1, 消息: %2")
                                .arg(statusCode)
                                .arg(message);
            qWarning() << statusError;
            throw std::runtime_error(statusError.toStdString());
        }
    }
    
    // 检查是否包含choices数组
    if (!responseObj.contains("choices") || !responseObj["choices"].isArray()) {
        QString errorMsg = "响应中缺少choices数组";
        qWarning() << errorMsg;
        qWarning() << "完整响应: " << QString(jsonResponse);
        throw std::runtime_error(errorMsg.toStdString());
    }
    
    QJsonArray choices = responseObj["choices"].toArray();
    if (choices.isEmpty()) {
        QString errorMsg = "choices数组为空";
        qWarning() << errorMsg;
        throw std::runtime_error(errorMsg.toStdString());
    }
    
    // 获取第一个choice
    QJsonObject choice = choices.first().toObject();
    
    // 检查是否包含message对象
    if (!choice.contains("message") || !choice["message"].isObject()) {
        QString errorMsg = "响应中缺少message对象";
        qWarning() << errorMsg;
        qWarning() << "Choice内容: " << QJsonDocument(choice).toJson();
        throw std::runtime_error(errorMsg.toStdString());
    }
    
    QJsonObject message = choice["message"].toObject();
    
    // 检查是否包含content字段
    if (!message.contains("content") || !message["content"].isString()) {
        QString errorMsg = "响应中缺少content字段";
        qWarning() << errorMsg;
        qWarning() << "Message内容: " << QJsonDocument(message).toJson();
        throw std::runtime_error(errorMsg.toStdString());
    }
    
    // 返回生成的文本内容
    QString content = message["content"].toString();
    qDebug() << "成功解析API响应，内容长度: " << content.length();
    return content;
}

// 将操作类型转换为描述性文字
QString DeepSeekService::operationToDescription(const QString& operation)
{
    if (operation == "rewrite") return "润色";
    if (operation == "summarize") return "总结";
    if (operation == "fix") return "修复";
    if (operation == "generic") return "通用对话";
    return operation;
} 