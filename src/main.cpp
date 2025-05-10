#include "mainwindow.h"
#include "DeepSeekService.h"

#include <QApplication>
#include <QFile>
#include <QDebug>
#include <QQuickStyle>
#include <QTranslator>
#include <QSettings>
#include <QLocale>
#include <QDir>

// 加载并应用样式表的辅助函数
QString loadStyleSheet(const QString &sheetName)
{
    QFile file(sheetName);
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        QString styleSheet = QLatin1String(file.readAll());
        file.close();
        return styleSheet;
    } else {
        qWarning() << "无法打开样式表文件:" << sheetName;
        return QString();
    }
}

// 加载翻译文件的辅助函数
void loadTranslation(QApplication &app)
{
    // 读取设置中的语言选项
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, 
                      QApplication::organizationName(), QApplication::applicationName());
    QString language = settings.value("General/Language", "System").toString();
    
    // 创建翻译器实例
    static QTranslator translator;
    
    // 根据设置选择语言
    QString locale;
    if (language == "System") {
        locale = QLocale::system().name(); // 使用系统语言
    } else {
        locale = language; // 使用用户选择的语言
    }
    
    // 加载翻译文件
    bool loaded = false;
    
    // 首先尝试从资源文件加载
    if (translator.load(":/translations/intellimedia_notes_" + locale)) {
        loaded = true;
        qDebug() << "从资源文件加载语言:" << locale;
    }
    // 然后尝试从应用程序目录加载
    else if (translator.load("intellimedia_notes_" + locale, QApplication::applicationDirPath() + "/translations")) {
        loaded = true;
        qDebug() << "从应用程序目录加载语言:" << locale;
    }
    // 如果失败，尝试从当前工作目录的translations子目录加载
    else if (translator.load("intellimedia_notes_" + locale, "translations")) {
        loaded = true;
        qDebug() << "从translations目录加载语言:" << locale;
    }
    
    if (loaded) {
        app.installTranslator(&translator);
        qDebug() << "已加载语言:" << locale;
    } else {
        qWarning() << "无法加载语言文件:" << locale;
    }
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    // 设置组织名和应用名，用于QSettings和QStandardPaths
    QApplication::setOrganizationName("IntelliMedia");
    QApplication::setApplicationName("IntelliMedia_Notes");
    
    // 加载翻译文件
    loadTranslation(a);
    
    // 设置 QML 控件样式为 Fusion (必须在 MainWindow 创建之前)
    QQuickStyle::setStyle("Fusion");

    // 加载全局样式和浅色主题样式
    QString globalStyle = loadStyleSheet(":/styles/style.qss");
    QString lightThemeStyle = loadStyleSheet(":/styles/light_theme.qss");
    
    // 应用组合样式
    a.setStyleSheet(globalStyle + lightThemeStyle);

    // 创建DeepSeek AI服务
    DeepSeekService *aiService = new DeepSeekService();
    
    // 显式验证API端点URL - 使用不含 /v1/ 的版本
    QString correctApiEndpoint = "https://api.deepseek.com/chat/completions";
    aiService->setApiEndpoint(correctApiEndpoint);
    qDebug() << "主程序确认API端点:" << correctApiEndpoint;
    
    // 如果有需要，这里可以从配置文件或环境变量中读取API密钥
    // 目前使用的是DeepSeekService中默认硬编码的密钥

    MainWindow w;
    
    // 将AI服务传递给主窗口
    w.setAiService(aiService);
    
    w.show();
    
    // 设置应用程序退出时自动删除aiService
    QObject::connect(&a, &QApplication::aboutToQuit, [aiService]() {
        delete aiService;
    });
    
    return a.exec();
}
