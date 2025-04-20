#include "mainwindow.h"

#include <QApplication>
#include <QFile>
#include <QDebug>
#include <QQuickStyle>

// 加载并应用样式表的辅助函数
void loadStyleSheet(const QString &sheetName)
{
    QFile file(sheetName);
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        QString styleSheet = QLatin1String(file.readAll());
        qApp->setStyleSheet(styleSheet);
        file.close();
    } else {
        qWarning() << "无法打开样式表文件:" << sheetName;
    }
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    // 设置 QML 控件样式为 Fusion (必须在 MainWindow 创建之前)
    QQuickStyle::setStyle("Fusion");

    // 启动时加载默认浅色主题
    loadStyleSheet(":/styles/light_theme.qss"); // 从资源加载

    MainWindow w;
    w.show();
    return a.exec();
}
