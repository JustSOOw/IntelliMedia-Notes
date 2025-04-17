import QtQuick 2.15

// 可复用的动画集合
QtObject {
    // 按钮缩放动画
    readonly property Component buttonScaleAnimation: Component {
        ParallelAnimation {
            PropertyAnimation {
                target: parent
                property: "scale"
                from: 1.0
                to: 0.9
                duration: 100
                easing.type: Easing.OutQuad
            }
            PropertyAnimation {
                target: parent
                property: "opacity"
                from: 1.0
                to: 0.8
                duration: 100
                easing.type: Easing.OutQuad
            }
        }
    }
    
    // 按钮恢复动画
    readonly property Component buttonRestoreAnimation: Component {
        ParallelAnimation {
            PropertyAnimation {
                target: parent
                property: "scale"
                from: 0.9
                to: 1.0
                duration: 100
                easing.type: Easing.OutQuad
            }
            PropertyAnimation {
                target: parent
                property: "opacity"
                from: 0.8
                to: 1.0
                duration: 100
                easing.type: Easing.OutQuad
            }
        }
    }
    
    // 菜单弹出动画
    readonly property Component menuPopupAnimation: Component {
        ParallelAnimation {
            PropertyAnimation {
                target: parent
                property: "scale"
                from: 0.8
                to: 1.0
                duration: 150
                easing.type: Easing.OutBack
            }
            PropertyAnimation {
                target: parent
                property: "opacity"
                from: 0.0
                to: 1.0
                duration: 150
                easing.type: Easing.OutCubic
            }
        }
    }
    
    // 淡入动画
    readonly property Component fadeInAnimation: Component {
        PropertyAnimation {
            target: parent
            property: "opacity"
            from: 0.0
            to: 1.0
            duration: 200
            easing.type: Easing.OutCubic
        }
    }
    
    // 淡出动画
    readonly property Component fadeOutAnimation: Component {
        PropertyAnimation {
            target: parent
            property: "opacity"
            from: 1.0
            to: 0.0
            duration: 150
            easing.type: Easing.InCubic
        }
    }
} 