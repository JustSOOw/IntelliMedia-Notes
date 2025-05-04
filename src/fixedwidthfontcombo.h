#ifndef FIXEDWIDTHFONTCOMBO_H
#define FIXEDWIDTHFONTCOMBO_H

#include <QFontComboBox>
#include <QListView>
#include <QTimer>

// 固定下拉菜单宽度的字体下拉框
class FixedWidthFontCombo : public QFontComboBox
{
    Q_OBJECT
    
public:
    explicit FixedWidthFontCombo(QWidget *parent = nullptr) : QFontComboBox(parent), m_menuWidth(250) {}
    
    // 设置下拉菜单宽度
    void setMenuWidth(int width) {
        m_menuWidth = width;
    }
    
    // 获取下拉菜单宽度
    int menuWidth() const {
        return m_menuWidth;
    }

protected:
    // 重写showPopup方法来设置下拉菜单宽度
    void showPopup() override {
        // 先调用基类方法显示弹出菜单
        QFontComboBox::showPopup();
        
        // 获取下拉视图并设置宽度
        if (auto *listView = qobject_cast<QListView*>(view())) {
            // 延迟执行以确保视图已完全创建
            QTimer::singleShot(0, this, [this, listView]() {
                // 手动计算弹出窗口的宽度
                int width = m_menuWidth; // 使用设置的菜单宽度
                
                // 调整菜单宽度
                if (auto *popup = listView->parentWidget()) {
                    // 有时popup是QComboBoxPrivateContainer或其他类型
                    popup->setFixedWidth(width);
                    
                    // 更新滚动区域
                    listView->setFixedWidth(width);
                    
                    // 重新定位popup
                    QPoint globalPos = mapToGlobal(QPoint(0, height()));
                    popup->move(globalPos.x(), popup->y());
                }
            });
        }
    }
    
private:
    int m_menuWidth; // 下拉菜单的宽度
};

#endif // FIXEDWIDTHFONTCOMBO_H 