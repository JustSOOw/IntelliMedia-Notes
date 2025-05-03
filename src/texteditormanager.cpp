/*
 * @Author: cursor AI
 * @Date: 2023-05-05 10:00:00
 * @LastEditors: Furdow wang22338014@gmail.com
 * @LastEditTime: 2025-05-03 23:26:17
 * @FilePath: \IntelliMedia_Notes\src\texteditormanager.cpp
 * @Description: QTextEdit编辑器管理类实现
 * 
 * Copyright (c) 2023, All Rights Reserved. 
 */
#include "texteditormanager.h"
#include <QScrollBar>
#include <QTextLayout>
#include <QAbstractTextDocumentLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QTextBlock>
#include <QDebug>
#include <QMimeData>
#include <QApplication>
#include <QClipboard>
#include <QScreen>
#include <QStyleOption>
#include <QPainter>
#include <QSvgRenderer>
#include <QFileInfo>
#include <QToolBar>
#include <QAction>
#include <QFileDialog>
#include <QTextStream>
#include <QDir>
#include <QDateTime>
#include <QMenu>
#include <QContextMenuEvent>
#include <QImageReader>
#include <QInputDialog>
#include <QMessageBox>
#include <QRegularExpression>
#include <QTextDocument>
#include <QTextCursor>
#include <QTextList>
#include <QBuffer>
#include <QUuid>
#include <QStandardPaths>
#include <QUrl>
#include <QCheckBox>
#include <QGridLayout>
#include <QSlider>
#include <QRadioButton>
#include <QButtonGroup>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QGroupBox>
#include <QDrag>
#include <QTimer> // *** 添加 QTimer 头文件 ***
#include <QPixmap> // 添加 QPixmap 头文件
#include <QCursor> // 添加 QCursor 头文件
#include <QFontDatabase>
#include <QFontComboBox>
#include <QAbstractItemView> // <-- 包含 QAbstractItemView 头文件
#include <QAbstractScrollArea> // <-- 包含 QAbstractScrollArea 头文件，setSizeAdjustPolicy 需要

// 新增常量定义
const int HANDLE_SIZE = 8; // 手柄大小
const int HANDLE_HALF_SIZE = HANDLE_SIZE / 2;

//=======================================================================================
// NoteTextEdit 实现
//=======================================================================================
NoteTextEdit::NoteTextEdit(QWidget *parent)
    : QTextEdit(parent)
    , m_trackingMouse(false)
    , m_isResizing(false)
    , m_currentHandle(-1)
    , m_isMoving(false)
{
    setMouseTracking(true);
    installEventFilter(this);
    
    // 设置文档边距
    document()->setDocumentMargin(20);
    
    // 设置文本编辑器的属性
    setAcceptRichText(true);
    setAutoFormatting(QTextEdit::AutoAll);
    
    // 设置文本编辑器的外观
    setFrameStyle(QFrame::NoFrame);
    viewport()->setCursor(Qt::IBeamCursor);
    
    // 启用拖放功能
    setAcceptDrops(true);
    setContextMenuPolicy(Qt::DefaultContextMenu);
}

void NoteTextEdit::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_isMoving = false;
        m_isResizing = false; 
        m_currentHandle = -1;
        m_trackingMouse = true;
        emit editorClicked(event->pos());
        QPoint pressPosViewport = event->pos();
        qDebug() << "MousePressEvent: Viewport Pos =" << pressPosViewport;

        // 1. 检查是否点击在当前选中图片的手柄上
        int handleIndex = getHandleAtPos(pressPosViewport);
        if (handleIndex != -1 && !m_selectedImageCursor.isNull() && !m_selectedImageRect.isNull()) {
            qDebug() << "MousePressEvent: Clicked on handle" << handleIndex << "for selected image.";
            // ... (精确获取格式和尺寸的逻辑 - 保持不变) ...
            QTextCursor preciseCursor = cursorForPosition(m_selectedImageRect.center());
            int imagePos = -1;
            QTextImageFormat imageFormat;
            QTextBlock blockForHandle = preciseCursor.block();
             for (QTextBlock::iterator it = blockForHandle.begin(); !it.atEnd(); ++it) {
                 QTextFragment frag = it.fragment();
                 if (frag.isValid() && frag.charFormat().isImageFormat()) {
                     imageFormat = frag.charFormat().toImageFormat();
                     imagePos = frag.position(); 
                     if (qAbs(imageFormat.width() - m_selectedImageRect.width()) < 5 && qAbs(imageFormat.height() - m_selectedImageRect.height()) < 5) {
                          qDebug() << "MousePressEvent: Found matching image fragment at pos" << imagePos << "for handle click.";
                          break; 
                     } else {
                          imagePos = -1; 
                          imageFormat = QTextImageFormat();
                     }
                 }
             }
              if (imagePos != -1 && imageFormat.isValid()) {
                  m_selectedImageCursor.setPosition(imagePos); 
                  m_originalImageSize = QSize(imageFormat.width(), imageFormat.height());
                  qDebug() << "MousePressEvent: Precisely set cursor to" << imagePos << "Original size set to" << m_originalImageSize;
              } else {
                  qWarning() << "MousePressEvent: Clicked handle, but failed to precisely locate image format/position near rect center. Using potentially stale format.";
                  imageFormat = m_selectedImageCursor.charFormat().toImageFormat(); // Might be invalid!
                  m_originalImageSize = QSize(imageFormat.width(), imageFormat.height()); 
              }
            
            m_isResizing = true;
            m_currentHandle = handleIndex;
            m_resizeStartPos = pressPosViewport;
            event->accept();
            return; 
        }

        // 2. 通过几何区域查找点击的图片 (替代之前依赖光标的方法)
        bool clickedOnImage = false;
        QTextCursor hitImageCursor;
        QRect hitImageViewRect;
        
        // 将视口点击坐标转换为文档坐标
        QPointF pressPosDocument = pressPosViewport + QPointF(horizontalScrollBar()->value(), verticalScrollBar()->value());
        qDebug() << "MousePressEvent: Document Pos =" << pressPosDocument;

        // 查找点击位置对应的文本块
        // cursorForPosition 仍然可以用来快速定位到可能相关的块
        QTextCursor approxCursor = cursorForPosition(pressPosViewport);
        QTextBlock currentBlock = approxCursor.block();
        
        if (currentBlock.isValid()) {
            qDebug() << "MousePressEvent: Checking block" << currentBlock.blockNumber();
            const QTextLayout *layout = currentBlock.layout();
            // 遍历块中的所有片段
            for (QTextBlock::iterator it = currentBlock.begin(); !it.atEnd(); ++it) {
                QTextFragment frag = it.fragment();
                if (!frag.isValid() || !frag.charFormat().isImageFormat()) {
                    continue; // 只关心图片片段
                }
                
                // 计算图片片段在文档坐标系中的精确矩形
                QRectF fragDocRect;
                QTextImageFormat fragFormat = frag.charFormat().toImageFormat();
                int fragPosInBlock = frag.position() - currentBlock.position();
                bool lineFound = false;
                for (int i = 0; i < layout->lineCount(); ++i) {
                    QTextLine line = layout->lineAt(i);
                    if (fragPosInBlock >= line.textStart() && fragPosInBlock < line.textStart() + line.textLength()) {
                        QRectF blockBound = document()->documentLayout()->blockBoundingRect(currentBlock);
                        qreal x = line.cursorToX(fragPosInBlock); 
                        qreal y = line.y();                  
                        fragDocRect = QRectF(blockBound.left() + x, blockBound.top() + y, fragFormat.width(), fragFormat.height());
                        lineFound = true;
                        break;
                    }
                }
                
                if (!lineFound) {
                    qWarning() << "MousePressEvent: Could not find line for image fragment at pos" << frag.position();
                    continue; // 无法确定位置，跳过此片段
                }
                qDebug() << "MousePressEvent: Checking image fragment at" << frag.position() << "DocRect=" << fragDocRect;

                // 检查文档坐标点击点是否在图片文档矩形内
                if (fragDocRect.contains(pressPosDocument)) {
                    qDebug() << "MousePressEvent: Hit detected on image fragment at" << frag.position();
                    clickedOnImage = true;
                    hitImageCursor = QTextCursor(document());
                    hitImageCursor.setPosition(frag.position()); // 定位到图片起始位置
                    // 将文档矩形转回视口矩形存储
                    hitImageViewRect = fragDocRect.translated(-QPointF(horizontalScrollBar()->value(), verticalScrollBar()->value())).toRect();
                    break; // 找到第一个命中的图片即可
                }
            }
        } else {
             qDebug() << "MousePressEvent: Could not find valid block for click position.";
        }

        // 3. 根据命中结果处理
        if (clickedOnImage) {
             // 点击了图片
             bool isSameImageSelected = !m_selectedImageCursor.isNull() && 
                                        m_selectedImageCursor.position() == hitImageCursor.position();
                                        
             if (!isSameImageSelected) {
                  qDebug() << "MousePressEvent: Selecting image via geometry hit at pos" << hitImageCursor.position();
                  m_selectedImageCursor = hitImageCursor;
                  m_selectedImageRect = hitImageViewRect; // 使用几何计算出的 Rect
                  updateSelectionIndicator(); // 可能不需要，因为 Rect 已经设置
                  viewport()->update();
             } else {
                  qDebug() << "MousePressEvent: Clicked on already selected image (geometry hit).";
                  // 如果点击已选中的，确保 m_selectedImageRect 是最新的
                  if (m_selectedImageRect != hitImageViewRect) {
                      m_selectedImageRect = hitImageViewRect;
                      viewport()->update();
                  }
             }
             // 准备移动
             m_isMoving = true;
             m_moveStartPos = pressPosViewport;
             event->accept();
             return;
        } else {
            // 没有点击任何图片 (或手柄)
            qDebug() << "MousePressEvent: Click did not hit any image geometry or handle.";
            // 如果当前有图片选中，则取消选中
            if (!m_selectedImageCursor.isNull()) {
                qDebug() << "MousePressEvent: Deselecting image by clicking outside.";
                m_selectedImageCursor = QTextCursor();
                m_selectedImageRect = QRect();
                viewport()->update(); 
            }
            // 这里不 accept 事件，让 QTextEdit 处理文本光标定位等
            QTextEdit::mousePressEvent(event);
        }
        
        /* --- 旧的基于光标的点击判断逻辑 (移除或注释掉) ---
        QTextCursor clickCursor = cursorForPosition(pressPosViewport);
        QTextCharFormat formatAtCursor = clickCursor.charFormat();
        // ... (之前的 if/else 判断 formatAtCursor 和 nextCursor) ...
        */
    }
    // 处理非左键点击
    else {
    QTextEdit::mousePressEvent(event);
    }
}

void NoteTextEdit::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_trackingMouse = false;

        if (m_isResizing) {
            m_isResizing = false;
            m_currentHandle = -1;
            updateImageSize(event->pos());
            setCursor(Qt::ArrowCursor);
            updateSelectionIndicator();
            qDebug() << "Resize finished.";
            viewport()->update();
            emit imageResized();
            event->accept();
            return;
        }

        // 检查是否有选中内容，并发送信号 (非调整大小时)
        bool hasSelection = textCursor().hasSelection();
        emit selectionChanged(event->pos(), hasSelection);

        // === 手动拖拽结束逻辑 ===
        if (m_manualDragging) {
            qDebug() << "mouseReleaseEvent: Manual dragging finished.";
            setCursor(Qt::ArrowCursor);
            // 销毁拖拽预览标签
            if (m_dragPreviewLabel) {
                m_dragPreviewLabel->hide();
                m_dragPreviewLabel->deleteLater();
                m_dragPreviewLabel = nullptr;
                qDebug() << "Drag preview label destroyed.";
            }
            
            QTextCursor dropCursor = cursorForPosition(event->pos());
            int newPos = dropCursor.position();
            qDebug() << "Drop position calculated:" << newPos;

            // 只有当位置真正改变时才执行移动操作
            if (newPos != m_dragStartPosition && newPos != m_dragStartPosition + 1) {
                qDebug() << "Position changed. Moving image from" << m_dragStartPosition << "to" << newPos;
                
                // 使用 beginEditBlock/endEditBlock 保证原子性
                QTextCursor editCursor = textCursor(); // 获取一个光标用于编辑块
                editCursor.beginEditBlock(); 

                QTextCursor deleteCursor(document());
                bool deleteFirst = (newPos < m_dragStartPosition);
                int originalDeletePos = m_dragStartPosition;

                qDebug() << "Original image format to move:" << m_draggedImageFormat.name() << "isValid:" << m_draggedImageFormat.isValid();
                qDebug() << "Delete first:" << deleteFirst;

                if (deleteFirst) {
                     // 先删除旧图片
                     deleteCursor.setPosition(originalDeletePos);
                     // 检查删除前光标位置的格式
                     QTextCharFormat formatBeforeDelete = deleteCursor.charFormat();
                     qDebug() << "Before delete (delete first) - cursor at:" << deleteCursor.position() << "Format is image:" << formatBeforeDelete.isImageFormat() << "Name:" << formatBeforeDelete.property(QTextFormat::UserProperty + 1).toString();
                     
                     deleteCursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, 1);
                     if (deleteCursor.charFormat().isImageFormat()) {
                        deleteCursor.removeSelectedText();
                        qDebug() << "Original image deleted (delete first). Position was:" << originalDeletePos;
                        // 删除后，如果新位置在旧位置之后，需要调整新位置
                        if (newPos > originalDeletePos) {
                            newPos--;
                            qDebug() << "Adjusted newPos after delete:" << newPos;
                        }
                     } else {
                          qDebug() << "Error: Could not select image for deletion (delete first) at pos:" << originalDeletePos;
                     }
                     
                     // 再插入新图片
                     dropCursor.setPosition(newPos);
                     dropCursor.insertImage(m_draggedImageFormat);
                     qDebug() << "New image inserted at:" << newPos;
                } else {
                    // 先插入新图片
                    // 如果插入位置在原图之后，直接插入
                    // 如果插入位置在原图之前，插入后需要调整删除位置
                    int insertPos = newPos;
                    bool adjustDeletePos = (insertPos <= originalDeletePos);
                    dropCursor.setPosition(insertPos);
                    dropCursor.insertImage(m_draggedImageFormat);
                    qDebug() << "New image inserted at:" << insertPos;

                    // 再删除旧图片
                    int adjustedDeletePos = originalDeletePos;
                    if (adjustDeletePos) {
                        adjustedDeletePos++; // 因为前面插入了一个字符
                        qDebug() << "Adjusted delete position due to prior insert:" << adjustedDeletePos;
                    }
                    deleteCursor.setPosition(adjustedDeletePos);
                    // 检查删除前光标位置的格式
                    QTextCharFormat formatBeforeDelete = deleteCursor.charFormat();
                    qDebug() << "Before delete (insert first) - cursor at:" << deleteCursor.position() << "Format is image:" << formatBeforeDelete.isImageFormat() << "Name:" << formatBeforeDelete.property(QTextFormat::UserProperty + 1).toString();
                    
                    deleteCursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, 1);
                    if (deleteCursor.charFormat().isImageFormat()) {
                        deleteCursor.removeSelectedText();
                        qDebug() << "Original image deleted (insert first). Position was:" << adjustedDeletePos;
                    } else {
                        qDebug() << "Error: Could not select image for deletion (insert first) at pos:" << adjustedDeletePos;
                    }
                }
                
                editCursor.endEditBlock(); // 结束编辑块
                qDebug() << "Edit block finished.";
                emit contentChangedByInteraction(); // 发出信号通知管理器内容已变
                // 更新光标到新插入图片的位置
                setTextCursor(dropCursor); 
                updateSelectionIndicator(); // 更新选中指示器到新位置
            } else {
                 qDebug() << "Position not changed significantly. Move cancelled.";
            }
            
            // 重置拖拽状态
            m_manualDragging = false;
            m_isMoving = false;
            m_dragStartPosition = -1;
            m_draggedImageFormat = QTextImageFormat();
            event->accept();
            qDebug() << "Manual drag state reset.";
            return;
        }
        // === 手动拖拽结束 ===

        // === 调整大小结束逻辑 ===
        if (m_isResizing) {
            qDebug() << "mouseReleaseEvent: Resizing finished.";
            m_isResizing = false;
            m_currentHandle = -1;
            setCursor(Qt::ArrowCursor);
            emit imageResized();
            emit contentChangedByInteraction();
            viewport()->update();
            event->accept();
            qDebug() << "Resize state reset.";
            return;
        }
        // === 调整大小结束 ===

        // 重置移动状态（如果只是点击或移动距离不够）
        if (m_isMoving) {
            qDebug() << "mouseReleaseEvent: Resetting 'isMoving' state (click or short move).";
            m_isMoving = false;
        }

        // 如果事件未被拖拽或调整大小处理，则调用基类实现
        QTextEdit::mouseReleaseEvent(event);
        qDebug() << "mouseReleaseEvent: Base class called.";

        // 处理常规文本选择后的浮动工具栏显示
        QTextCursor cursor = textCursor();
        if (cursor.hasSelection()) {
            QRect selectionRect = cursorRect(cursor);
            // 调整位置，使其出现在选区下方
            QPoint toolbarPos = mapToGlobal(selectionRect.bottomLeft() + QPoint(0, 5)); 
            emit selectionChanged(toolbarPos, true);
            qDebug() << "mouseReleaseEvent: Emitted selectionChanged for text selection.";
        } else {
            emit selectionChanged(QPoint(), false); // 隐藏工具栏
            qDebug() << "mouseReleaseEvent: Emitted selectionChanged to hide toolbar.";
            emit editorClicked(mapToGlobal(event->pos()));
        }
    } else {
        m_trackingMouse = false; // 重置 m_trackingMouse
    }
    QTextEdit::mouseReleaseEvent(event);
}

void NoteTextEdit::mouseDoubleClickEvent(QMouseEvent *event)
{
    // 检查双击位置是否在图片上
    QTextCursor cursor = cursorForPosition(event->pos());
    if (cursor.charFormat().isImageFormat()) {
        // 如果是图片，阻止默认的双击事件（如选择）
        qDebug() << "Image double-clicked, accepting event and doing nothing else.";
        event->accept();
        return;
    } else {
        // 如果不是图片，执行默认的双击行为（如选词）
        QTextEdit::mouseDoubleClickEvent(event);
        updateImageSize(event->pos());
        updateSelectionIndicator();
        update(); 
        setCursorForHandle(m_currentHandle);
        event->accept();
        return;
    }
}

void NoteTextEdit::mouseMoveEvent(QMouseEvent *event)
{
    

    // 1. 如果有图片选中，提前更新指示器矩形
    if (!m_selectedImageCursor.isNull()) {
        // 检查光标后的字符是否真的是图片 (同 paintEvent 逻辑)
        QTextCursor checkCursor = m_selectedImageCursor;
        if (checkCursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor) && 
            checkCursor.charFormat().isImageFormat()) 
        {
            updateSelectionIndicator(); // 确保 m_selectedImageRect 是最新的
        }
    }

    // 2. 调用基类处理标准事件（文本选择、滚动等），这可能会改变布局
    QTextEdit::mouseMoveEvent(event);

    // 3. 处理图片调整大小 (基于更新后的状态)
    if (m_isResizing) {
        qDebug() << "mouseMoveEvent: Resizing active.";
        updateImageSize(event->pos());
        // updateSelectionIndicator(); // updateImageSize 内部应该会间接更新, 或者在 paintEvent 更新
        viewport()->update(); // 强制重绘以显示大小变化
        setCursorForHandle(m_currentHandle); // 保持调整时光标
        event->accept();
        return; // 正在调整大小，不进行后续处理
    }

    // 4. 处理图片移动 (基于更新后的状态)
    if (m_isMoving) {
        qDebug() << "mouseMoveEvent: Moving active. Delta:" << (event->pos() - m_moveStartPos).manhattanLength();
        // 检查是否达到拖拽阈值
        if ((event->pos() - m_moveStartPos).manhattanLength() >= QApplication::startDragDistance()) {
            qDebug() << "mouseMoveEvent: StartDragDistance met, initiating manual drag operation.";
            
            // *** 完全替换使用QDrag的方案 ***
            // 获取当前选中图片的最新信息
            QTextImageFormat currentFormat;
            QTextCursor currentImgCursor = m_selectedImageCursor;
            if (currentImgCursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor) && 
                currentImgCursor.charFormat().isImageFormat()) {
                currentFormat = currentImgCursor.charFormat().toImageFormat();
            } else {
                qDebug() << "mouseMoveEvent: Drag failed - Could not re-verify selected image.";
                m_isMoving = false;
                event->ignore();
            return;
        }

            QString imgPath = currentFormat.name();
            if (imgPath.isEmpty()) {
                qDebug() << "mouseMoveEvent: Drag failed - Image path is empty.";
        m_isMoving = false;
                event->ignore();
                return;
            }
            
            // 存储拖拽图片的信息，备用
            m_dragStartPosition = m_selectedImageCursor.position();
            m_draggedImageFormat = currentFormat;
            qDebug() << "mouseMoveEvent: Stored startPos:" << m_dragStartPosition << "and format for manual drag.";
            
            // 创建和显示拖拽预览
            QImage image(imgPath);
            if (!image.isNull()) {
                QPixmap previewPixmap = QPixmap::fromImage(image).scaled(100, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                if (!m_dragPreviewLabel) {
                    m_dragPreviewLabel = new QLabel(nullptr, Qt::ToolTip | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
                    m_dragPreviewLabel->setAttribute(Qt::WA_TranslucentBackground);
                    // 设置明显的样式：半透明浅蓝色背景 + 蓝色虚线边框
                    m_dragPreviewLabel->setStyleSheet("border: 2px dashed blue; background-color: rgba(173, 216, 230, 100);");
                }
                m_dragPreviewLabel->setPixmap(previewPixmap);
                m_dragPreviewLabel->adjustSize();
                m_dragPreviewLabel->move(QCursor::pos() + QPoint(15, 15));
                m_dragPreviewLabel->show();
                m_dragPreviewLabel->raise();
            } else {
                 qDebug() << "mouseMoveEvent: Failed to load image for preview:" << imgPath;
            }

            // 启用手动拖拽模式
            m_manualDragging = true;

            // 设置鼠标形状为拖拽状态
            setCursor(Qt::DragMoveCursor);

            // 创建预览图标（可选） - 已通过QLabel实现
            // 这里我们用其他方式显示拖拽预览

            // 在此拦截事件，不要调用drag->exec()
            event->accept();
            return;
        } else {
            // 未达到拖拽阈值，仅接受事件阻止文本选择
            qDebug() << "mouseMoveEvent: Moving but below drag distance.";
        event->accept();
        return;
    }
    }
    
    // 手动拖拽模式的处理
    if (m_manualDragging) {
        // 计算当前放置位置（直接使用 QTextEdit 已有的视口到文档的转换逻辑）
        QPointF docPos = event->pos() + QPointF(horizontalScrollBar()->value(), verticalScrollBar()->value());

        // 创建临时光标并移动到最接近的位置
        QTextCursor dropCursor = cursorForPosition(event->pos());

        // 可选：在此显示拖拽预览
        // 更新预览标签的位置
        if (m_dragPreviewLabel) {
             m_dragPreviewLabel->move(QCursor::pos() + QPoint(15, 15));
        }


        event->accept();
        return;
    }

    // 5. 处理手柄悬停检测 (基于更新后的状态)
    int handleIndex = getHandleAtPos(event->pos()); // *** 恢复手柄悬停检测 ***
    setCursorForHandle(handleIndex);                // *** 恢复光标形状设置 ***
    
    // 如果事件没有被上面的逻辑处理掉，就让它继续传递 (虽然大部分情况已被处理)
    // event->ignore(); // 通常不需要，因为基类调用已发生
}

void NoteTextEdit::paintEvent(QPaintEvent *event)
{
    QTextEdit::paintEvent(event); // 先调用基类绘制文本和图片
    // 滚动或内容移动时更新选中框位置，确保手柄与图片同步 (调用时机可能需要斟酌)
    // updateSelectionIndicator(); // 暂时注释掉，避免在paintEvent中意外修改状态

     
    bool shouldDrawHandles = false;
    if (!m_selectedImageCursor.isNull()) {
        QTextCursor checkCursor = m_selectedImageCursor;
        // 尝试选中光标后的一个字符
        if (checkCursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor)) {
            // 检查选中的字符是否是图片
            if (checkCursor.charFormat().isImageFormat()) {
                shouldDrawHandles = true;
            }
        }
    }

    // 如果光标后的字符是图片，并且我们有有效的选中矩形，则绘制
    if (shouldDrawHandles && !m_selectedImageRect.isNull() && m_selectedImageRect.isValid()) { 
        QPainter painter(viewport());
        drawSelectionIndicator(&painter);
    }
}

bool NoteTextEdit::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == viewport()) {
        if (event->type() == QEvent::MouseMove && m_trackingMouse) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            bool hasSelection = textCursor().hasSelection();
            emit selectionChanged(mouseEvent->pos(), hasSelection);
        }
    }
    return QTextEdit::eventFilter(watched, event);
}

void NoteTextEdit::contextMenuEvent(QContextMenuEvent *event)
{
    // 创建标准上下文菜单（包含复制、粘贴、剪切等标准操作）
    QMenu *menu = createStandardContextMenu();
    
    menu->addSeparator();
    
    // 添加"纯文本粘贴"操作 (Ctrl+Shift+V)
    QAction *plainTextPasteAction = menu->addAction("纯文本粘贴");
    plainTextPasteAction->setShortcut(QKeySequence("Ctrl+Shift+V"));
    connect(plainTextPasteAction, &QAction::triggered, [this]() {
        // 获取剪贴板内容作为纯文本
        QClipboard *clipboard = QApplication::clipboard();
        QString text = clipboard->text();
        if (!text.isEmpty()) {
            // 插入纯文本（不保留格式）
            textCursor().insertText(text);
        }
    });
    
    // 添加"全选"操作 (Ctrl+A)
    QAction *selectAllAction = menu->addAction("全选");
    selectAllAction->setShortcut(QKeySequence("Ctrl+A"));
    connect(selectAllAction, &QAction::triggered, this, &QTextEdit::selectAll);
    
    // 添加"插入当前时间"操作 (Alt+Shift+D)
    QAction *insertTimeAction = menu->addAction("插入当前时间");
    insertTimeAction->setShortcut(QKeySequence("Alt+Shift+D"));
    connect(insertTimeAction, &QAction::triggered, [this]() {
        // 获取当前时间并格式化
        QString currentTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        // 在光标位置插入时间文本
        textCursor().insertText(currentTime);
    });
    
    // 添加"字数统计"操作
    QAction *wordCountAction = menu->addAction("字数统计");
    connect(wordCountAction, &QAction::triggered, [this]() {
        QString text = toPlainText();
        // 移除所有HTML标签和特殊字符
        text = text.simplified();
        
        int charCount = text.length();
        // 简单的中文字数统计方式（按空格分割）
        int wordCount = text.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts).count();
        
        QMessageBox::information(this, "字数统计",
                                QString("字符数：%1\n单词数：%2").arg(charCount).arg(wordCount));
    });
    
    // 添加插入图片操作
    menu->addSeparator();
    QAction *insertImageAction = menu->addAction("插入图片");
    connect(insertImageAction, &QAction::triggered, [this]() {
        QString filePath = QFileDialog::getOpenFileName(this, 
            "选择图片", 
            QStandardPaths::writableLocation(QStandardPaths::PicturesLocation),
            "图片文件 (*.png *.jpg *.jpeg *.bmp *.gif)");
            
        if (!filePath.isEmpty()) {
            insertImageFromFile(filePath);
        }
    });
    
    // 如果光标处有图片，添加图片相关操作
    QString imagePath = getImageAtCursor();
    if (!imagePath.isEmpty()) {
        menu->addSeparator();
        
        QAction *copyImageAction = menu->addAction("复制图片");
        connect(copyImageAction, &QAction::triggered, [this, imagePath]() {
            QClipboard *clipboard = QApplication::clipboard();
            QImage image(imagePath);
            if (!image.isNull()) {
                clipboard->setImage(image);
            }
        });
        
        QAction *saveImageAsAction = menu->addAction("图片另存为...");
        connect(saveImageAsAction, &QAction::triggered, [this, imagePath]() {
            QString saveFilePath = QFileDialog::getSaveFileName(this,
                "保存图片",
                QStandardPaths::writableLocation(QStandardPaths::PicturesLocation),
                "图片文件 (*.png *.jpg *.jpeg *.bmp)");
                
            if (!saveFilePath.isEmpty()) {
                QImage image(imagePath);
                image.save(saveFilePath);
            }
        });
    }
    
    menu->exec(event->globalPos());
    delete menu;
}

void NoteTextEdit::dragEnterEvent(QDragEnterEvent *event)
{
    // 检查是否是图片文件
    if (event->mimeData()->hasUrls()) {
        QList<QUrl> urls = event->mimeData()->urls();
        for (const QUrl &url : urls) {
            QString filePath = url.toLocalFile();
            QImageReader reader(filePath);
            if (reader.canRead()) {
                event->acceptProposedAction();
                return;
            }
        }
    }
    
    // 如果不是图片，使用默认处理
    QTextEdit::dragEnterEvent(event);
}

void NoteTextEdit::dropEvent(QDropEvent *event)
{
    // 处理图片文件的放置
    if (event->mimeData()->hasUrls()) {
        QList<QUrl> urls = event->mimeData()->urls();
        for (const QUrl &url : urls) {
            QString filePath = url.toLocalFile();
            QImageReader reader(filePath);
            if (reader.canRead()) {
                if (event->source() == this && event->dropAction() == Qt::MoveAction) {
                    qDebug() << "DropEvent: Internal move started."; // *** 添加内部移动开始日志 ***
                    // 内部移动：使用存储的位置和格式完成移动
                    if (m_dragStartPosition == -1 || !m_draggedImageFormat.isValid()) { 
                         qWarning() << "DropEvent: Internal move started but no valid start position or format was stored! StartPos=" << m_dragStartPosition;
                         event->ignore(); 
                         m_draggedImageFormat = QTextImageFormat(); // 清理
                         m_dragStartPosition = -1;
                         return;
                    }
                    
                    // 1. 删除原始位置的图片
                    QTextCursor deleteCursor(document());
                    deleteCursor.setPosition(m_dragStartPosition);
                    qDebug() << "DropEvent (Internal Move): Attempting to delete original image at pos:" << m_dragStartPosition;
                    deleteCursor.beginEditBlock();
                    if (deleteCursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor) && 
                        deleteCursor.charFormat().isImageFormat() && 
                        deleteCursor.charFormat().toImageFormat().name() == m_draggedImageFormat.name()) // 确认是同一张图
                    {
                        deleteCursor.removeSelectedText();
                        qDebug() << "DropEvent: Deleted original image at pos:" << m_dragStartPosition;
                    } else {
                        qWarning() << "DropEvent: Failed to find/verify original image at stored pos:" << m_dragStartPosition;
                        // 即使删除失败，也继续尝试插入
                    }
                    deleteCursor.endEditBlock();

                    // 2. 在新位置插入存储的图片格式
                    QTextCursor dropCursor = cursorForPosition(event->position().toPoint());
                    dropCursor.beginEditBlock();
                    dropCursor.insertImage(m_draggedImageFormat);
                    dropCursor.endEditBlock();

                    // ***关键：更新选中光标到新插入图片的位置***
                    // 需要重新定位光标到刚插入的图片前
                    m_selectedImageCursor = QTextCursor(document());
                    m_selectedImageCursor.setPosition(dropCursor.position() - 1); 
                    setTextCursor(m_selectedImageCursor); 
                    qDebug() << "DropEvent (Internal Move): After setTextCursor. Cursor pos:" << textCursor().position();
                    
                    // *** 获取当前光标并清除选择状态 ***
                    QTextCursor currentCursor = textCursor();
                    currentCursor.clearSelection();
                    setTextCursor(currentCursor); // 可能需要重新设置一下光标
                    qDebug() << "DropEvent (Internal Move): After clearSelection.";
                    
                     
                    // *** 尝试强制更新包含新图片的块 ***
                    QTextBlock newBlock = m_selectedImageCursor.block(); // 获取新光标所在的块
                    if (newBlock.isValid()) {
                        qDebug() << "DropEvent (Internal Move): Forcing update for block:" << newBlock.blockNumber();
                        document()->documentLayout()->updateBlock(newBlock);
                    } else {
                        qWarning() << "DropEvent (Internal Move): Could not get valid block for new cursor position!";
                    }
                    // *** 结束块更新尝试 ***

                    updateSelectionIndicator();
                    viewport()->update();
                    
                    event->acceptProposedAction();
                    m_draggedImageFormat = QTextImageFormat(); // 清理存储的格式
                    m_dragStartPosition = -1;                // 清理存储的位置
                    qDebug() << "DropEvent: Internal move finished using stored format and pos.";
                    return;
                } else {
                     qDebug() << "DropEvent: External drop detected.";
                // 外部拖放：插入图片文件
                     QTextCursor dropCursor = cursorForPosition(event->position().toPoint());
                dropCursor.clearSelection();
                setTextCursor(dropCursor);
                     // 插入图片后，新的图片应该被选中
                     if (insertImageFromFile(filePath)) { // insertImageFromFile 现在返回bool
                        // 插入成功后，将光标定位到图片前并选中
                        m_selectedImageCursor = textCursor(); // 获取插入后的光标
                        m_selectedImageCursor.movePosition(QTextCursor::PreviousCharacter); // 移动到图片前
                        qDebug() << "DropEvent (External): Selected image cursor at:" << m_selectedImageCursor.position(); // 保留关键放置日志
                        updateSelectionIndicator(); // 更新选中框
                        viewport()->update(); // 强制重绘
                event->acceptProposedAction();
                        // qDebug() << "DropEvent: External drop finished."; // 减少日志
                     } else {
                        qDebug() << "DropEvent: External drop failed to insert image.";
                        event->ignore();
                     }
                     return; // 处理完图片拖放
                }
            }
        }
    }
    
    // 如果不是图片，使用默认处理
    // qDebug() << "DropEvent: Not an image URL, falling back to QTextEdit::dropEvent."; // 减少日志
    QTextEdit::dropEvent(event);
}

bool NoteTextEdit::insertImageFromFile(const QString &filePath, int maxWidth)
{
    QImageReader reader(filePath);
    QImage image = reader.read();
    
    if (image.isNull()) {
        qDebug() << "无法加载图片:" << filePath << reader.errorString();
        return false;
    }
    
    // 调整图片大小，如果超过最大宽度
    QSize originalSize = image.size();
    if (maxWidth > 0 && image.width() > maxWidth) {
        qDebug() << "Resizing image from" << originalSize << "to max width" << maxWidth;
        image = image.scaledToWidth(maxWidth, Qt::SmoothTransformation);
        qDebug() << " -> New size:" << image.size();
    }
    
    // 保存图片到媒体文件夹
    QString savedImagePath = saveImageToMediaFolder(filePath);
    if (savedImagePath.isEmpty()) {
        qDebug() << "无法保存图片到媒体文件夹";
        return false;
    }
    
    // 插入图片到文档
    QTextCursor cursor = textCursor();
    QTextDocument *doc = document();
    
    QTextImageFormat imageFormat;
    imageFormat.setName(savedImagePath);
    imageFormat.setWidth(image.width());
    imageFormat.setHeight(image.height());
    // imageFormat.setProperty(QTextFormat::FullWidthSelection, true); // 暂时移除，观察效果
    
    cursor.insertImage(imageFormat);
    // cursor.insertBlock(); // 暂时移除，让图片更像行内元素
    setTextCursor(cursor); // 更新编辑器光标
    qDebug() << "insertImageFromFile: Inserted image" << savedImagePath << "New cursor pos:" << textCursor().position(); // 添加日志
    
    return true;
}

QString NoteTextEdit::saveImageToMediaFolder(const QString &sourceFilePath)
{
    // 确保媒体文件夹存在
    QString mediaFolderPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/IntelliMedia_Notes/media";
    QDir mediaDir(mediaFolderPath);
    
    if (!mediaDir.exists()) {
        if (!mediaDir.mkpath(".")) {
            qDebug() << "无法创建媒体文件夹:" << mediaFolderPath;
            return QString();
        }
    }
    
    // 为图片创建唯一的文件名
    QFileInfo fileInfo(sourceFilePath);
    QString extension = fileInfo.suffix().toLower();
    QString uniqueFileName = QUuid::createUuid().toString(QUuid::WithoutBraces) + "." + extension;
    QString targetFilePath = mediaFolderPath + "/" + uniqueFileName;
    
    // 复制原始图片到媒体文件夹
    if (!QFile::copy(sourceFilePath, targetFilePath)) {
        qDebug() << "无法复制图片到:" << targetFilePath;
        return QString();
    }
    
    return targetFilePath;
}

QString NoteTextEdit::getImageAtCursor()
{
    QTextCursor cursor = textCursor();
    QTextCharFormat format = cursor.charFormat();
    
    // 检查光标处是否有图片
    if (format.isImageFormat()) {
        QTextImageFormat imageFormat = format.toImageFormat();
        return imageFormat.name();
    }
    
    return QString();
}

// 更新选中图片的状态和矩形
void NoteTextEdit::updateSelectionIndicator()
{
    // 1. 检查光标有效性
    if (m_selectedImageCursor.isNull()) {
        if (!m_selectedImageRect.isNull()) { 
        m_selectedImageRect = QRect();
             viewport()->update(); 
         }
        return;
    }
    
    // 2. 尝试获取光标后字符的图片格式
    QTextImageFormat format; 
    QTextCursor formatCursor = m_selectedImageCursor;
    bool formatObtained = false;
    if (formatCursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor)) {
        if (formatCursor.charFormat().isImageFormat()) {
             format = formatCursor.charFormat().toImageFormat();
             if (format.isValid()) { 
                 formatObtained = true;
             }
        }
    }
    
    // 3. 如果未能获取有效格式，则清除状态并返回
    if (!formatObtained) { 
       if (!m_selectedImageRect.isNull()) { 
           qDebug() << "updateSelectionIndicator: Clearing rect because no valid format obtained.";
        m_selectedImageRect = QRect();
           viewport()->update(); 
       }
        return;
    }
    
    // 4. 计算文档矩形 (此时 format 肯定是有效的)
    QRectF docRect;
    QTextBlock block = m_selectedImageCursor.block();
    const QTextLayout *layout = block.layout();
    int posInBlock = m_selectedImageCursor.position() - block.position();
    bool foundLine = false;
    for (int i = 0; i < layout->lineCount(); ++i) {
        QTextLine line = layout->lineAt(i);
        if (posInBlock >= line.textStart() && posInBlock < line.textStart() + line.textLength()) {
            QRectF blockBound = document()->documentLayout()->blockBoundingRect(block);
            qreal x = line.cursorToX(posInBlock);
            qreal y = line.y();
            docRect = QRectF(blockBound.left() + x, blockBound.top() + y, format.width(), format.height());
            foundLine = true;
            break;
        }
    }
    if (!foundLine) {
        qWarning() << "updateSelectionIndicator: Could not find line for image at pos" << m_selectedImageCursor.position();
        // 如果找不到，也清除旧矩形避免手柄留在错误位置
         if (!m_selectedImageRect.isNull()) { 
             m_selectedImageRect = QRect();
             viewport()->update(); 
         }
        return; 
    }

    QPointF offset(horizontalScrollBar()->value(), verticalScrollBar()->value());
    QRectF viewRect = docRect.translated(-offset);
    QRect newRect = viewRect.toRect();

    if (m_selectedImageRect != newRect) {
        m_selectedImageRect = newRect;
        viewport()->update(); 
    } 
}

// 绘制选中框和手柄
void NoteTextEdit::drawSelectionIndicator(QPainter *painter)
{
    // 直接使用 updateSelectionIndicator 计算好的矩形
    if (m_selectedImageRect.isNull() || !m_selectedImageRect.isValid()) {
        return;
    }

    QRect currentImageRect = m_selectedImageRect;

    painter->save();

    // 绘制淡蓝色边框
    painter->setPen(QPen(QColor(173, 216, 230, 180), 1, Qt::SolidLine)); 
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(currentImageRect.adjusted(0, 0, -1, -1)); 

    // 绘制手柄
    painter->setPen(Qt::black);
    painter->setBrush(QColor(173, 216, 230)); 

    QPoint tl = currentImageRect.topLeft();
    QPoint tr = currentImageRect.topRight();
    QPoint bl = currentImageRect.bottomLeft();
    QPoint br = currentImageRect.bottomRight();
    QRect handle_tl(tl.x() - HANDLE_HALF_SIZE, tl.y() - HANDLE_HALF_SIZE, HANDLE_SIZE, HANDLE_SIZE);
    QRect handle_tr(tr.x() - HANDLE_HALF_SIZE + 1, tr.y() - HANDLE_HALF_SIZE, HANDLE_SIZE, HANDLE_SIZE); 
    QRect handle_bl(bl.x() - HANDLE_HALF_SIZE, bl.y() - HANDLE_HALF_SIZE + 1, HANDLE_SIZE, HANDLE_SIZE); 
    QRect handle_br(br.x() - HANDLE_HALF_SIZE + 1, br.y() - HANDLE_HALF_SIZE + 1, HANDLE_SIZE, HANDLE_SIZE);

    painter->drawRect(handle_tl);
    painter->drawRect(handle_tr);
    painter->drawRect(handle_bl);
    painter->drawRect(handle_br);

    painter->restore();
}

// 获取鼠标位置处的手柄索引
int NoteTextEdit::getHandleAtPos(const QPoint &pos) const
{
    // 直接使用 updateSelectionIndicator 计算好的矩形
    if (m_selectedImageRect.isNull()) return -1;

    QRect currentImageRect = m_selectedImageRect;

    // 计算手柄矩形，中心对齐角点
    QPoint tl = currentImageRect.topLeft();
    QPoint tr = currentImageRect.topRight();
    QPoint bl = currentImageRect.bottomLeft();
    QPoint br = currentImageRect.bottomRight();
    QRect handle_tl(tl.x() - HANDLE_HALF_SIZE, tl.y() - HANDLE_HALF_SIZE, HANDLE_SIZE, HANDLE_SIZE);
    QRect handle_tr(tr.x() - HANDLE_HALF_SIZE + 1, tr.y() - HANDLE_HALF_SIZE, HANDLE_SIZE, HANDLE_SIZE); 
    QRect handle_bl(bl.x() - HANDLE_HALF_SIZE, bl.y() - HANDLE_HALF_SIZE + 1, HANDLE_SIZE, HANDLE_SIZE); 
    QRect handle_br(br.x() - HANDLE_HALF_SIZE + 1, br.y() - HANDLE_HALF_SIZE + 1, HANDLE_SIZE, HANDLE_SIZE);

    // 增加容差进行点击判断
    if (handle_tl.adjusted(-2, -2, 2, 2).contains(pos)) {
        return 0; // Top-Left
    }
    if (handle_tr.adjusted(-2, -2, 2, 2).contains(pos)) {
        return 1; // Top-Right
    }
    if (handle_bl.adjusted(-2, -2, 2, 2).contains(pos)) {
        return 2; // Bottom-Left
    }
    if (handle_br.adjusted(-2, -2, 2, 2).contains(pos)) {
        return 3; // Bottom-Right
    }
    return -1;
}

// 根据鼠标位置更新图片大小
void NoteTextEdit::updateImageSize(const QPoint &mousePos)
{
    if (!m_isResizing || m_selectedImageCursor.isNull()) return;

    // 获取图片格式
    QTextImageFormat format;
    QTextCursor formatCursor = m_selectedImageCursor;
    if (formatCursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor)) {
        format = formatCursor.charFormat().toImageFormat();
    } else {
        return; // 无法获取格式，直接返回
    }

    // 计算鼠标相对于起始点的偏移
    int dx = mousePos.x() - m_resizeStartPos.x();
    int dy = mousePos.y() - m_resizeStartPos.y();

    // 计算新尺寸 (保持宽高比)
    QSize newSize = m_originalImageSize;
    double aspectRatio = static_cast<double>(m_originalImageSize.width()) / m_originalImageSize.height();
    if (aspectRatio <= 0) aspectRatio = 1.0;

    switch (m_currentHandle) {
        case 0: // Top-Left
            newSize.setWidth(qMax(10, m_originalImageSize.width() - dx));
            newSize.setHeight(qMax(10, static_cast<int>(newSize.width() / aspectRatio)));
            if (qAbs((m_originalImageSize.height() - dy) - newSize.height()) > qAbs((m_originalImageSize.width() - dx) - newSize.width())) {
                 newSize.setHeight(qMax(10, m_originalImageSize.height() - dy));
                 newSize.setWidth(qMax(10, static_cast<int>(newSize.height() * aspectRatio)));
            }
            break;
        case 1: // Top-Right
            newSize.setWidth(qMax(10, m_originalImageSize.width() + dx));
            newSize.setHeight(qMax(10, static_cast<int>(newSize.width() / aspectRatio)));
            if (qAbs((m_originalImageSize.height() - dy) - newSize.height()) > qAbs((m_originalImageSize.width() + dx) - newSize.width())) {
                 newSize.setHeight(qMax(10, m_originalImageSize.height() - dy));
                 newSize.setWidth(qMax(10, static_cast<int>(newSize.height() * aspectRatio)));
            }
            break;
        case 2: // Bottom-Left
            newSize.setWidth(qMax(10, m_originalImageSize.width() - dx));
            newSize.setHeight(qMax(10, static_cast<int>(newSize.width() / aspectRatio)));
             if (qAbs((m_originalImageSize.height() + dy) - newSize.height()) > qAbs((m_originalImageSize.width() - dx) - newSize.width())) {
                 newSize.setHeight(qMax(10, m_originalImageSize.height() + dy));
                 newSize.setWidth(qMax(10, static_cast<int>(newSize.height() * aspectRatio)));
            }
            break;
        case 3: // Bottom-Right
            newSize.setWidth(qMax(10, m_originalImageSize.width() + dx));
            newSize.setHeight(qMax(10, static_cast<int>(newSize.width() / aspectRatio)));
            if (qAbs((m_originalImageSize.height() + dy) - newSize.height()) > qAbs((m_originalImageSize.width() + dx) - newSize.width())) {
                 newSize.setHeight(qMax(10, m_originalImageSize.height() + dy));
                 newSize.setWidth(qMax(10, static_cast<int>(newSize.height() * aspectRatio)));
            }
            break;
        default: 
            return;
    }

    // 更新文档中的图片格式
    // 检查新尺寸是否有效
    if (newSize.width() < 10 || newSize.height() < 10) {
        return;
    }
    format.setWidth(newSize.width());
    format.setHeight(newSize.height());

    QTextCursor tempCursor = m_selectedImageCursor; // 使用临时光标修改
    tempCursor.beginEditBlock(); // 开始编辑块
    // 选中图片字符以便应用格式
    tempCursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
    tempCursor.setCharFormat(format);
    tempCursor.endEditBlock(); // 结束编辑块

    // 强制文档布局更新
    document()->documentLayout()->updateBlock(tempCursor.block());
    document()->adjustSize();
}

// 根据手柄设置鼠标光标
void NoteTextEdit::setCursorForHandle(int handleIndex)
{
    switch (handleIndex) {
        case 0: // Top-Left
        case 3: // Bottom-Right
            viewport()->setCursor(Qt::SizeFDiagCursor);
            break;
        case 1: // Top-Right
        case 2: // Bottom-Left
            viewport()->setCursor(Qt::SizeBDiagCursor);
            break;
        default: // 不在手柄上
            // 恢复文本编辑光标
            viewport()->setCursor(Qt::IBeamCursor);
            break;
    }
}

// 重写滚动以同步图片选中框位置，确保手柄与图片同步
void NoteTextEdit::scrollContentsBy(int dx, int dy)
{
    QTextEdit::scrollContentsBy(dx, dy);
    updateSelectionIndicator();
    viewport()->update();
}

//=======================================================================================
// FloatingToolBar 实现
//=======================================================================================
FloatingToolBar::FloatingToolBar(QWidget *parent)
    : QWidget(parent, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
    , m_isDarkTheme(false)
    , m_textColor(Qt::black)
    , m_highlightColor(Qt::yellow)
{
    setAttribute(Qt::WA_ShowWithoutActivating);
    setMouseTracking(true);
    
    // 设置工具栏样式
    setStyleSheet("FloatingToolBar { border: 1px solid #cccccc; border-radius: 4px; background-color: #f5f5f5; }");
    
    setupUI();
    
    // 安装事件过滤器
    qApp->installEventFilter(this);
    
    // 连接字体、字号和标题选择的信号
    connect(m_fontComboBox, &QComboBox::currentTextChanged, this, [this](const QString &text) {
        emit fontFamilyChanged(text);
    });
    
    connect(m_fontSizeComboBox, &QComboBox::currentTextChanged, this, [this](const QString &text) {
        emit fontSizeChanged(text);
    });
    
    connect(m_headingComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        emit headingChanged(index);
    });
}

void FloatingToolBar::setupUI()
{
    // 创建布局
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(5, 5, 5, 5);
    layout->setSpacing(3);
    
    // 创建按钮
    m_boldButton = new QToolButton(this);
    m_italicButton = new QToolButton(this);
    m_underlineButton = new QToolButton(this);
    m_strikeOutButton = new QToolButton(this);
    m_textColorButton = new QToolButton(this);
    m_highlightButton = new QToolButton(this);
    m_alignLeftButton = new QToolButton(this);
    m_alignCenterButton = new QToolButton(this);
    m_alignRightButton = new QToolButton(this);
    m_alignJustifyButton = new QToolButton(this);
    
    // 创建字体、字号和标题选择控件
    m_fontComboBox = new QFontComboBox(this);
    m_fontComboBox->setFixedWidth(80); // 设置固定宽度，使按钮有统一的大小
    m_fontComboBox->setToolTip("选择字体");
    m_fontComboBox->view()->setMinimumWidth(180); // 下拉列表较宽
    m_fontComboBox->view()->setSizeAdjustPolicy(QAbstractItemView::AdjustToContents);
    
    // 恢复正常样式并修复空白问题
    m_fontComboBox->setStyleSheet(
        "QFontComboBox { border: 1px solid #cccccc; border-radius: 4px; text-overflow: ellipsis; }"
        "QFontComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: right; width: 15px; border-left-width: 1px; border-left-color: #cccccc; border-left-style: solid; border-top-right-radius: 4px; border-bottom-right-radius: 4px; }"
        "QFontComboBox::down-arrow { image: url(:/icons/editor/dropdown.svg); width: 8px; height: 8px; }"
        "QFontComboBox QLineEdit { padding-right: 5px; }" // 减少右侧填充
        "QFontComboBox QAbstractItemView { border: 1px solid #cccccc; selection-background-color: #e0e0e0; }"
    );
    
    // 添加字号下拉框
    m_fontSizeComboBox = new QComboBox(this);
    m_fontSizeComboBox->setFixedWidth(35); // 字号按钮可以更窄
    m_fontSizeComboBox->view()->setMinimumWidth(60);
    m_fontSizeComboBox->setToolTip("选择字号");
    
    // 恢复正常样式并修复空白问题
    m_fontSizeComboBox->setStyleSheet(
        "QComboBox { border: 1px solid #cccccc; border-radius: 4px; qproperty-alignment: AlignCenter; }"
        "QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: right; width: 15px; border-left-width: 1px; border-left-color: #cccccc; border-left-style: solid; border-top-right-radius: 4px; border-bottom-right-radius: 4px; }"
        "QComboBox::down-arrow { image: url(:/icons/editor/dropdown.svg); width: 8px; height: 8px; }"
        "QComboBox QLineEdit { padding-right: 5px; }" // 减少右侧填充
        "QComboBox QAbstractItemView { border: 1px solid #cccccc; selection-background-color: #e0e0e0; }"
    );
    
    QStringList fontSizes = {"8", "9", "10", "11", "12", "14", "16", "18", "20", "22", "24", "26", "28", "36", "48", "72"};
    m_fontSizeComboBox->addItems(fontSizes);
    m_fontSizeComboBox->setCurrentText(QString::number(12)); // 默认字号
    
    // 添加标题级别下拉框
    m_headingComboBox = new QComboBox(this);
    m_headingComboBox->setFixedWidth(70); // 设置固定宽度
    m_headingComboBox->view()->setMinimumWidth(150); // 下拉列表较宽
    m_headingComboBox->setToolTip("选择标题级别");
    
    // 恢复正常样式并修复空白问题
    m_headingComboBox->setStyleSheet(
        "QComboBox { border: 1px solid #cccccc; border-radius: 4px; qproperty-alignment: AlignCenter; }"
        "QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: right; width: 15px; border-left-width: 1px; border-left-color: #cccccc; border-left-style: solid; border-top-right-radius: 4px; border-bottom-right-radius: 4px; }"
        "QComboBox::down-arrow { image: url(:/icons/editor/dropdown.svg); width: 8px; height: 8px; }"
        "QComboBox QLineEdit { padding-right: 5px; }" // 减少右侧填充
        "QComboBox QAbstractItemView { border: 1px solid #cccccc; selection-background-color: #e0e0e0; }"
    );
    
    // 设置图标
    m_boldButton->setIcon(QIcon(":/icons/editor/bold.svg"));
    m_italicButton->setIcon(QIcon(":/icons/editor/italic.svg"));
    m_underlineButton->setIcon(QIcon(":/icons/editor/underline.svg"));
    m_strikeOutButton->setIcon(QIcon(":/icons/editor/strikeout.svg"));
    m_textColorButton->setIcon(QIcon(":/icons/editor/text-color.svg"));
    m_highlightButton->setIcon(QIcon(":/icons/editor/highlight.svg"));
    m_alignLeftButton->setIcon(QIcon(":/icons/editor/align-left.svg"));
    m_alignCenterButton->setIcon(QIcon(":/icons/editor/align-center.svg"));
    m_alignRightButton->setIcon(QIcon(":/icons/editor/align-right.svg"));
    m_alignJustifyButton->setIcon(QIcon(":/icons/editor/align-justify.svg"));
    
    // 设置按钮可选中
    m_boldButton->setCheckable(true);
    m_italicButton->setCheckable(true);
    m_underlineButton->setCheckable(true);
    m_strikeOutButton->setCheckable(true);
    m_alignLeftButton->setCheckable(true);
    m_alignCenterButton->setCheckable(true);
    m_alignRightButton->setCheckable(true);
    m_alignJustifyButton->setCheckable(true);
    
    // 设置按钮大小
    QSize buttonSize(20, 20); // <-- 缩小按钮尺寸
    m_boldButton->setIconSize(buttonSize);
    m_italicButton->setIconSize(buttonSize);
    m_underlineButton->setIconSize(buttonSize);
    m_strikeOutButton->setIconSize(buttonSize);
    m_textColorButton->setIconSize(buttonSize);
    m_highlightButton->setIconSize(buttonSize);
    m_alignLeftButton->setIconSize(buttonSize);
    m_alignCenterButton->setIconSize(buttonSize);
    m_alignRightButton->setIconSize(buttonSize);
    m_alignJustifyButton->setIconSize(buttonSize);
    
    // 设置按钮样式
    QString buttonStyle = "QToolButton { border: none; padding: 2px; border-radius: 4px; background-color: transparent; } " // <-- 减小内边距
                          "QToolButton:hover { background-color: #e0e0e0; } "
                          "QToolButton:checked { background-color: #c0c0c0; } "
                          "QToolButton:pressed { background-color: #b0b0b0; }";
    
    m_boldButton->setStyleSheet(buttonStyle);
    m_italicButton->setStyleSheet(buttonStyle);
    m_underlineButton->setStyleSheet(buttonStyle);
    m_strikeOutButton->setStyleSheet(buttonStyle);
    m_textColorButton->setStyleSheet(buttonStyle);
    m_highlightButton->setStyleSheet(buttonStyle);
    m_alignLeftButton->setStyleSheet(buttonStyle);
    m_alignCenterButton->setStyleSheet(buttonStyle);
    m_alignRightButton->setStyleSheet(buttonStyle);
    m_alignJustifyButton->setStyleSheet(buttonStyle);
    
    // 设置下拉框样式 (检查下拉箭头图标路径)
    QString comboBoxStyle = "QComboBox { border: 1px solid #cccccc; border-radius: 3px; padding: 1px 18px 1px 3px; min-height: 20px; } " // <-- 减小最小高度
                            "QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: top right; width: 15px; border-left-width: 1px; border-left-color: #cccccc; border-left-style: solid; border-top-right-radius: 3px; border-bottom-right-radius: 3px; } "
                            "QComboBox::down-arrow { image: url(:/icons/editor/dropdown.svg); width: 12px; height: 12px; } " // <-- 确认此图标资源有效
                            "QComboBox QAbstractItemView { border: 1px solid #cccccc; selection-background-color: #e0e0e0; }";
    
    m_fontComboBox->setStyleSheet(comboBoxStyle);
    m_fontSizeComboBox->setStyleSheet(comboBoxStyle);
    m_headingComboBox->setStyleSheet(comboBoxStyle);
    
    // 添加字体、字号和标题选择到布局
    layout->addWidget(m_fontComboBox);
    layout->addWidget(m_fontSizeComboBox);
    layout->addWidget(m_headingComboBox);
    
    // 添加分隔符
    QFrame *separator0 = new QFrame(this);
    separator0->setFrameShape(QFrame::VLine);
    separator0->setFrameShadow(QFrame::Sunken);
    layout->addWidget(separator0);
    
    // 添加按钮到布局
    layout->addWidget(m_boldButton);
    layout->addWidget(m_italicButton);
    layout->addWidget(m_underlineButton);
    layout->addWidget(m_strikeOutButton);
    
    // 添加分隔符
    QFrame *separator1 = new QFrame(this);
    separator1->setFrameShape(QFrame::VLine);
    separator1->setFrameShadow(QFrame::Sunken);
    layout->addWidget(separator1);
    
    layout->addWidget(m_textColorButton);
    layout->addWidget(m_highlightButton);
    
    // 添加分隔符
    QFrame *separator2 = new QFrame(this);
    separator2->setFrameShape(QFrame::VLine);
    separator2->setFrameShadow(QFrame::Sunken);
    layout->addWidget(separator2);
    
    layout->addWidget(m_alignLeftButton);
    layout->addWidget(m_alignCenterButton);
    layout->addWidget(m_alignRightButton);
    layout->addWidget(m_alignJustifyButton);
    
    setLayout(layout);
    
    // 添加标题级别下拉框的选项
    m_headingComboBox->addItem("正文");
    m_headingComboBox->addItem("一级标题");
    m_headingComboBox->addItem("二级标题");
    m_headingComboBox->addItem("三级标题");
    m_headingComboBox->addItem("四级标题");
    m_headingComboBox->addItem("五级标题");
    m_headingComboBox->addItem("六级标题");
    
    // 设置工具提示
    m_boldButton->setToolTip("粗体");
    m_italicButton->setToolTip("斜体");
    m_underlineButton->setToolTip("下划线");
    m_strikeOutButton->setToolTip("删除线");
    m_textColorButton->setToolTip("文本颜色");
    m_highlightButton->setToolTip("文本高亮");
    m_alignLeftButton->setToolTip("左对齐");
    m_alignCenterButton->setToolTip("居中对齐");
    m_alignRightButton->setToolTip("右对齐");
    m_alignJustifyButton->setToolTip("两端对齐");
}

void FloatingToolBar::setTheme(bool isDarkTheme)
{
    m_isDarkTheme = isDarkTheme;
    
    // 根据主题设置工具栏样式
    if (isDarkTheme) {
        setStyleSheet("FloatingToolBar { border: 1px solid #444444; border-radius: 4px; background-color: #333333; }");
        
        QString buttonStyle = "QToolButton { border: none; padding: 1px; border-radius: 3px; background-color: transparent; color: white; } " // <-- 减小内边距
                              "QToolButton:hover { background-color: #444444; } "
                              "QToolButton:checked { background-color: #555555; } "
                              "QToolButton:pressed { background-color: #666666; }";
        
        QString comboBoxStyle = "QComboBox { border: 1px solid #555555; border-radius: 3px; padding: 1px; min-height: 20px; background-color: #444444; color: white; } " // <-- 减小最小高度
                                "QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: right; width: 15px; border-left-width: 1px; border-left-color: #555555; border-left-style: solid; border-top-right-radius: 3px; border-bottom-right-radius: 3px; } "
                                "QComboBox::down-arrow { image: url(:/icons/editor/dropdown.svg); width: 8px; height: 8px; } "
                                "QComboBox QLineEdit { padding-right: 5px; background-color: #444444; color: white; }" // 减少右侧填充
                                "QComboBox QAbstractItemView { border: 1px solid #555555; background-color: #333333; color: white; selection-background-color: #555555; }";
        
        m_boldButton->setStyleSheet(buttonStyle);
        m_italicButton->setStyleSheet(buttonStyle);
        m_underlineButton->setStyleSheet(buttonStyle);
        m_strikeOutButton->setStyleSheet(buttonStyle);
        m_textColorButton->setStyleSheet(buttonStyle);
        m_highlightButton->setStyleSheet(buttonStyle);
        m_alignLeftButton->setStyleSheet(buttonStyle);
        m_alignCenterButton->setStyleSheet(buttonStyle);
        m_alignRightButton->setStyleSheet(buttonStyle);
        m_alignJustifyButton->setStyleSheet(buttonStyle);
        
        m_fontComboBox->setStyleSheet(comboBoxStyle);
        m_fontSizeComboBox->setStyleSheet(comboBoxStyle);
        m_headingComboBox->setStyleSheet(comboBoxStyle);
        
        // 设置深色主题下的lineEdit样式，确保居中效果和省略号显示
        if (m_headingComboBox->lineEdit()) {
            m_headingComboBox->lineEdit()->setStyleSheet("QLineEdit { text-align: center; qproperty-alignment: AlignCenter; padding: 0 4px 0 4px; text-overflow: ellipsis; min-width: 0; background-color: #444444; color: white; }");
        }
        if (m_fontSizeComboBox->lineEdit()) {
            m_fontSizeComboBox->lineEdit()->setStyleSheet("QLineEdit { text-align: center; qproperty-alignment: AlignCenter; padding: 0 4px 0 4px; text-overflow: ellipsis; min-width: 0; background-color: #444444; color: white; }");
        }
    } else {
        setStyleSheet("FloatingToolBar { border: 1px solid #cccccc; border-radius: 4px; background-color: #f5f5f5; }");
        
        QString buttonStyle = "QToolButton { border: none; padding: 1px; border-radius: 3px; background-color: transparent; color: black; } " // <-- 减小内边距
                              "QToolButton:hover { background-color: #e0e0e0; } "
                              "QToolButton:checked { background-color: #c0c0c0; } "
                              "QToolButton:pressed { background-color: #b0b0b0; }";
        
        QString comboBoxStyle = "QComboBox { border: 1px solid #cccccc; border-radius: 3px; padding: 1px; min-height: 20px; background-color: white; color: black; } " // <-- 减小最小高度
                                "QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: right; width: 15px; border-left-width: 1px; border-left-color: #cccccc; border-left-style: solid; border-top-right-radius: 3px; border-bottom-right-radius: 3px; } "
                                "QComboBox::down-arrow { image: url(:/icons/editor/dropdown.svg); width: 8px; height: 8px; } "
                                "QComboBox QLineEdit { padding-right: 5px; background-color: white; color: black; }" // 减少右侧填充
                                "QComboBox QAbstractItemView { border: 1px solid #cccccc; background-color: white; color: black; selection-background-color: #e0e0e0; }";
        
        m_boldButton->setStyleSheet(buttonStyle);
        m_italicButton->setStyleSheet(buttonStyle);
        m_underlineButton->setStyleSheet(buttonStyle);
        m_strikeOutButton->setStyleSheet(buttonStyle);
        m_textColorButton->setStyleSheet(buttonStyle);
        m_highlightButton->setStyleSheet(buttonStyle);
        m_alignLeftButton->setStyleSheet(buttonStyle);
        m_alignCenterButton->setStyleSheet(buttonStyle);
        m_alignRightButton->setStyleSheet(buttonStyle);
        m_alignJustifyButton->setStyleSheet(buttonStyle);
        
        m_fontComboBox->setStyleSheet(comboBoxStyle);
        m_fontSizeComboBox->setStyleSheet(comboBoxStyle);
        m_headingComboBox->setStyleSheet(comboBoxStyle);
        
        // 设置亮色主题下的lineEdit样式，确保居中效果和省略号显示
        if (m_headingComboBox->lineEdit()) {
            m_headingComboBox->lineEdit()->setStyleSheet("QLineEdit { text-align: center; qproperty-alignment: AlignCenter; padding: 0 4px 0 4px; text-overflow: ellipsis; min-width: 0; background-color: white; color: black; }");
        }
        if (m_fontSizeComboBox->lineEdit()) {
            m_fontSizeComboBox->lineEdit()->setStyleSheet("QLineEdit { text-align: center; qproperty-alignment: AlignCenter; padding: 0 4px 0 4px; text-overflow: ellipsis; min-width: 0; background-color: white; color: black; }");
        }
    }
}

void FloatingToolBar::updatePosition(const QPoint &)
{
    // QTextEdit *editor = qobject_cast<QTextEdit*>(parent()); // 错误：父对象是 viewport
    NoteTextEdit *editor = qobject_cast<NoteTextEdit*>(parentWidget()->parentWidget()); // 获取 NoteTextEdit 实例
    if (!editor) {
        qWarning() << "FloatingToolBar::updatePosition - Could not get NoteTextEdit parent.";
        return;
    }

    QTextCursor cursor = editor->textCursor();
    if (!cursor.hasSelection()) {
        // 如果没有选择，可以考虑隐藏或不移动
        // hide(); // 或者 return;
        return; // 暂时返回，避免在没有选择时移动
    }
    
    // 获取选区的起始和结束位置的矩形 (相对于 viewport)
    int startPos = cursor.selectionStart();
    int endPos = cursor.selectionEnd();
    QTextCursor startCursor = cursor;
    QTextCursor endCursor = cursor;
    startCursor.setPosition(startPos);
    endCursor.setPosition(endPos);
    
    // 使用 viewport()->cursorRect() 获取相对于 viewport 的坐标
    QRect startRectViewport = editor->cursorRect(startCursor);
    QRect endRectViewport = editor->cursorRect(endCursor);

    // 合并起始和结束矩形，获得选区的外接矩形 (相对于 viewport)
    QRect selectionRectViewport = startRectViewport.united(endRectViewport);

    // 将选区矩形的视口坐标转换为全局坐标，用于屏幕边界检查
    QPoint globalTopLeft = editor->viewport()->mapToGlobal(selectionRectViewport.topLeft());
    QPoint globalBottomLeft = editor->viewport()->mapToGlobal(selectionRectViewport.bottomLeft());

    // 获取屏幕尺寸信息
    QScreen *screen = QGuiApplication::screenAt(globalBottomLeft);
    if (!screen) screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->availableGeometry();

    // 计算目标全局坐标
    // 默认全局位置：在选区下方且水平居中
    int globalX = globalBottomLeft.x() + (selectionRectViewport.width() - width()) / 2;
    int globalY = globalBottomLeft.y() + 5; // 稍微增加与选区的距离

    // 如果工具栏超出了屏幕底部，则改为显示在选区上方
    if (globalY + height() > screenGeometry.bottom()) {
        globalY = globalTopLeft.y() - height() - 5; // 稍微增加与选区的距离
    }

    // 确保工具栏水平不超出屏幕
    if (globalX < screenGeometry.left()) {
        globalX = screenGeometry.left() + 5;
    } else if (globalX + width() > screenGeometry.right()) {
        globalX = screenGeometry.right() - width() - 5;
    }

    // 获取编辑器 viewport 的全局几何区域
    QRect viewportRect = editor->viewport()->rect();
    QPoint viewportTopLeftGlobal = editor->viewport()->mapToGlobal(viewportRect.topLeft());
    QPoint viewportBottomRightGlobal = editor->viewport()->mapToGlobal(viewportRect.bottomRight());
    QRect viewportGlobalRect(viewportTopLeftGlobal, viewportBottomRightGlobal);
    
    // 根据 viewport 边界钳制全局坐标
    // 钳制 X 轴
    if (globalX < viewportGlobalRect.left()) {
        globalX = viewportGlobalRect.left();
    } else if (globalX + width() > viewportGlobalRect.right()) {
        globalX = viewportGlobalRect.right() - width();
    }
    
    // 钳制 Y 轴
    if (globalY < viewportGlobalRect.top()) {
        globalY = viewportGlobalRect.top();
    } else if (globalY + height() > viewportGlobalRect.bottom()) {
        globalY = viewportGlobalRect.bottom() - height();
    }

    // FloatingToolBar 是顶级窗口 (Qt::Tool)，move 需要全局坐标
    move(globalX, globalY);

    // 确保显示
    if (!isVisible()) {
         show();
    }
    raise(); // 确保在顶层
}

void FloatingToolBar::setFormatActions(const QTextCharFormat &format)
{
    // 根据当前字符格式更新按钮状态
    m_boldButton->setChecked(format.fontWeight() >= QFont::Bold);
    m_italicButton->setChecked(format.fontItalic());
    m_underlineButton->setChecked(format.fontUnderline());
    m_strikeOutButton->setChecked(format.fontStrikeOut());
    
    // 更新文本颜色和高亮颜色
    if (format.hasProperty(QTextFormat::ForegroundBrush)) {
        m_textColor = format.foreground().color();
    }
    
    if (format.hasProperty(QTextFormat::BackgroundBrush)) {
        m_highlightColor = format.background().color();
    }
}

QIcon FloatingToolBar::createColorIcon(const QColor &color)
{
    QPixmap pixmap(16, 16);
    pixmap.fill(color);
    return QIcon(pixmap);
}

QIcon FloatingToolBar::createColorToolButtonIcon(const QString &iconPath, const QColor &color)
{
    // 加载SVG
    QSvgRenderer renderer(iconPath);
    if (!renderer.isValid()) {
        qWarning() << "无效的SVG文件:" << iconPath;
        return QIcon();
    }
    
    // 创建像素图
    QSize iconSize(24, 24);
    QPixmap pixmap(iconSize);
    pixmap.fill(Qt::transparent);
    
    // 在像素图上绘制SVG
    QPainter painter(&pixmap);
    
    // 将SVG渲染到像素图
    renderer.render(&painter);
    
    // 根据颜色创建彩色小方块
    QRect colorRect(12, 12, 8, 8);
    painter.fillRect(colorRect, color);
    painter.setPen(m_isDarkTheme ? Qt::white : Qt::black);
    painter.drawRect(colorRect);
    
    painter.end();
    
    return QIcon(pixmap);
}

void FloatingToolBar::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    
    // 确保工具栏有焦点
    setFocus();
    raise();
}

bool FloatingToolBar::eventFilter(QObject *watched, QEvent *event)
{
    // 监听鼠标点击事件，当点击工具栏外部区域时隐藏工具栏
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        QPoint globalClickPos = mouseEvent->globalPosition().toPoint();

        // 检查点击是否在工具栏自身内部
        bool clickInsideToolbar = geometry().contains(globalClickPos);

        // 检查点击是否在任何可见的下拉框视图内部
        bool clickInsideDropdown = false;
        QList<QComboBox*> comboBoxes = {m_fontComboBox, m_fontSizeComboBox, m_headingComboBox};
        for (QComboBox* comboBox : comboBoxes) {
            if (comboBox && comboBox->view() && comboBox->view()->isVisible()) { // 添加对 view() 指针的检查
                // 获取视图的全局几何区域
                // QRect viewGeometry = comboBox->view()->mapToGlobal(comboBox->view()->rect()); // 错误用法
                QPoint viewTopLeftGlobal = comboBox->view()->mapToGlobal(QPoint(0, 0));
                QRect viewGeometry(viewTopLeftGlobal, comboBox->view()->size()); // 使用全局左上角和尺寸构造全局矩形
                if (viewGeometry.contains(globalClickPos)) {
                    clickInsideDropdown = true;
                    break; // 点击在一个下拉框内，无需再检查其他下拉框
                }
            }
        }

        // 如果点击既不在工具栏内，也不在可见的下拉框内，并且工具栏当前可见，则隐藏
        if (!clickInsideToolbar && !clickInsideDropdown && isVisible()) {
            hide();
            // 返回 true 表示事件已被处理，防止可能的进一步传递导致意外行为
            return true; 
        }
    }
    
    // 对于其他事件，调用基类的 eventFilter
    return QWidget::eventFilter(watched, event);
}

//=======================================================================================
// TextEditorManager 实现
//=======================================================================================
TextEditorManager::TextEditorManager(QWidget *parent)
    : QObject(parent)
    , m_isDarkTheme(false)  // 默认使用亮色主题
    , m_textColor(Qt::black)
    , m_highlightColor(Qt::yellow)
    , m_hasUnsavedChanges(false)  // 初始化m_hasUnsavedChanges
    , m_currentNotePath("")       // 初始化路径为空字符串
    , m_currentFilePath("")       // 初始化路径为空字符串
{
    // 创建编辑器容器
    m_editorContainer = new QWidget(qobject_cast<QWidget*>(parent));
    QVBoxLayout *containerLayout = new QVBoxLayout(m_editorContainer);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setSpacing(0);

    // 创建文本编辑器
    m_textEdit = new NoteTextEdit(m_editorContainer);
    
    // 创建浮动工具栏
    m_floatingToolBar = new FloatingToolBar(m_textEdit->viewport());
    m_floatingToolBar->hide();
    
    // 创建顶部工具栏
    m_topToolBar = new QToolBar(m_editorContainer);
    setupTopToolBar();
    
    // 添加组件到容器
    containerLayout->addWidget(m_topToolBar);
    containerLayout->addWidget(m_textEdit);
    
    // 初始化更新工具栏计时器
    m_updateToolBarTimer = new QTimer(this);
    m_updateToolBarTimer->setSingleShot(true);
    m_updateToolBarTimer->setInterval(50);
    
    // 连接信号和槽
    connect(m_textEdit, &NoteTextEdit::selectionChanged, this, &TextEditorManager::handleSelectionChanged);
    connect(m_textEdit, &NoteTextEdit::editorClicked, this, &TextEditorManager::handleTextEditClicked);
    connect(m_textEdit, &QTextEdit::textChanged, this, &TextEditorManager::contentModified);
    connect(m_textEdit, &NoteTextEdit::imageResized, this, &TextEditorManager::documentModified);
    connect(m_updateToolBarTimer, &QTimer::timeout, this, &TextEditorManager::updateToolBarForCurrentFormat);
    
    // 设置字体下拉框处理
    setupFontComboBoxes();
    
    // 连接工具栏按钮的信号
    connect(m_floatingToolBar->boldButton(), &QToolButton::clicked, this, &TextEditorManager::onBoldTriggered);
    connect(m_floatingToolBar->italicButton(), &QToolButton::clicked, this, &TextEditorManager::onItalicTriggered);
    connect(m_floatingToolBar->underlineButton(), &QToolButton::clicked, this, &TextEditorManager::onUnderlineTriggered);
    connect(m_floatingToolBar->strikeOutButton(), &QToolButton::clicked, this, &TextEditorManager::onStrikeOutTriggered);
    connect(m_floatingToolBar->textColorButton(), &QToolButton::clicked, this, &TextEditorManager::onTextColorTriggered);
    connect(m_floatingToolBar->highlightButton(), &QToolButton::clicked, this, &TextEditorManager::onHighlightTriggered);
    connect(m_floatingToolBar->alignLeftButton(), &QToolButton::clicked, this, &TextEditorManager::onAlignLeftTriggered);
    connect(m_floatingToolBar->alignCenterButton(), &QToolButton::clicked, this, &TextEditorManager::onAlignCenterTriggered);
    connect(m_floatingToolBar->alignRightButton(), &QToolButton::clicked, this, &TextEditorManager::onAlignRightTriggered);
    connect(m_floatingToolBar->alignJustifyButton(), &QToolButton::clicked, this, &TextEditorManager::onAlignJustifyTriggered);
    
    // 连接浮动工具栏字体、字号和标题选择控件的信号
    connect(m_floatingToolBar, &FloatingToolBar::fontFamilyChanged, this, &TextEditorManager::onFontFamilyChanged);
    connect(m_floatingToolBar, &FloatingToolBar::fontSizeChanged, this, &TextEditorManager::onFontSizeChanged);
    connect(m_floatingToolBar, &FloatingToolBar::headingChanged, this, &TextEditorManager::onHeadingChanged);
    
    // 设置默认亮色主题
    setTheme(m_isDarkTheme);
}

TextEditorManager::~TextEditorManager()
{
    // 父对象将负责删除m_textEdit和m_floatingToolBar
}

void TextEditorManager::setupTopToolBar()
{
    // 设置工具栏属性
    m_topToolBar->setMovable(false);
    m_topToolBar->setFloatable(false);
    m_topToolBar->setIconSize(QSize(20, 20));
    
    // 创建操作，使用自定义图标
    m_saveAction = m_topToolBar->addAction(QIcon(":/icons/editor/save.svg"), "保存");
    m_topToolBar->addSeparator();
    
    m_undoAction = m_topToolBar->addAction(QIcon(":/icons/editor/undo.svg"), "撤销");
    m_redoAction = m_topToolBar->addAction(QIcon(":/icons/editor/redo.svg"), "重做");
    m_topToolBar->addSeparator();
    
    m_cutAction = m_topToolBar->addAction(QIcon(":/icons/editor/cut.svg"), "剪切");
    m_copyAction = m_topToolBar->addAction(QIcon(":/icons/editor/copy.svg"), "复制");
    m_pasteAction = m_topToolBar->addAction(QIcon(":/icons/editor/paste.svg"), "粘贴");
    m_topToolBar->addSeparator();
    
    // 添加字体、字号和标题选择控件
    QWidget *fontWidget = new QWidget(m_topToolBar);
    QHBoxLayout *fontLayout = new QHBoxLayout(fontWidget);
    fontLayout->setContentsMargins(5, 0, 5, 0);
    fontLayout->setSpacing(5);
    
    m_fontComboBox = new QFontComboBox(fontWidget);
    m_fontComboBox->setMinimumWidth(80); // 保持最小宽度
    m_fontComboBox->setMaximumWidth(110); // 稍微增加最大宽度以容纳更多文本
    m_fontComboBox->setToolTip("选择字体");
    m_fontComboBox->setEditable(true);
    m_fontComboBox->lineEdit()->setReadOnly(true);
    m_fontComboBox->lineEdit()->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    
    // 恢复正常样式并修复空白问题
    m_fontComboBox->setStyleSheet(
        "QFontComboBox { border: 1px solid #cccccc; border-radius: 4px; text-overflow: ellipsis; }"
        "QFontComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: right; width: 15px; border-left-width: 1px; border-left-color: #cccccc; border-left-style: solid; border-top-right-radius: 4px; border-bottom-right-radius: 4px; }"
        "QFontComboBox::down-arrow { image: url(:/icons/editor/dropdown.svg); width: 8px; height: 8px; }"
        "QFontComboBox QLineEdit { padding-right: 5px; }" // 减少右侧填充
        "QFontComboBox QAbstractItemView { border: 1px solid #cccccc; selection-background-color: #e0e0e0; }"
    );
    
    fontLayout->addWidget(m_fontComboBox);
    m_topToolBar->addWidget(fontWidget);
    
    // 添加字号下拉框
    QWidget *fontSizeWidget = new QWidget(m_topToolBar);
    QHBoxLayout *fontSizeLayout = new QHBoxLayout(fontSizeWidget);
    fontSizeLayout->setContentsMargins(0, 0, 5, 0);
    fontSizeLayout->setSpacing(5);
    
    m_fontSizeComboBox = new QComboBox(fontSizeWidget);
    m_fontSizeComboBox->setMinimumWidth(40); // <-- 调整宽度
    m_fontSizeComboBox->setMaximumWidth(60); // <-- 调整宽度
    m_fontSizeComboBox->setToolTip("选择字号");
    m_fontSizeComboBox->setEditable(true);
    m_fontSizeComboBox->lineEdit()->setReadOnly(true);
    m_fontSizeComboBox->lineEdit()->setAlignment(Qt::AlignCenter);
    
    // 恢复正常样式并修复空白问题
    m_fontSizeComboBox->setStyleSheet(
        "QComboBox { border: 1px solid #cccccc; border-radius: 4px; qproperty-alignment: AlignCenter; }"
        "QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: right; width: 15px; border-left-width: 1px; border-left-color: #cccccc; border-left-style: solid; border-top-right-radius: 4px; border-bottom-right-radius: 4px; }"
        "QComboBox::down-arrow { image: url(:/icons/editor/dropdown.svg); width: 8px; height: 8px; }"
        "QComboBox QLineEdit { padding-right: 5px; }" // 减少右侧填充
        "QComboBox QAbstractItemView { border: 1px solid #cccccc; selection-background-color: #e0e0e0; }"
    );
    
    QStringList fontSizes = {"8", "9", "10", "11", "12", "14", "16", "18", "20", "22", "24", "26", "28", "36", "48", "72"};
    m_fontSizeComboBox->addItems(fontSizes);
    m_fontSizeComboBox->setCurrentText("12");
    fontSizeLayout->addWidget(m_fontSizeComboBox);
    m_topToolBar->addWidget(fontSizeWidget);
    
    // 添加标题级别下拉框
    QWidget *headingWidget = new QWidget(m_topToolBar);
    QHBoxLayout *headingLayout = new QHBoxLayout(headingWidget);
    headingLayout->setContentsMargins(0, 0, 5, 0);
    headingLayout->setSpacing(5);
    
    m_headingComboBox = new QComboBox(headingWidget);
    m_headingComboBox->setFixedWidth(80); // 设置固定宽度
    m_headingComboBox->view()->setMinimumWidth(150); // 下拉列表较宽
    m_headingComboBox->setToolTip("选择标题级别");
    
    // 修改：设置为可编辑并强制居中显示
    m_headingComboBox->setEditable(true);
    m_headingComboBox->lineEdit()->setReadOnly(true);
    m_headingComboBox->lineEdit()->setAlignment(Qt::AlignCenter);
    
    // 恢复正常样式并修复空白问题
    m_headingComboBox->setStyleSheet(
        "QComboBox { border: 1px solid #cccccc; border-radius: 4px; qproperty-alignment: AlignCenter; }"
        "QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: right; width: 15px; border-left-width: 1px; border-left-color: #cccccc; border-left-style: solid; border-top-right-radius: 4px; border-bottom-right-radius: 4px; }"
        "QComboBox::down-arrow { image: url(:/icons/editor/dropdown.svg); width: 8px; height: 8px; }"
        "QComboBox QLineEdit { padding-right: 5px; }" // 减少右侧填充
        "QComboBox QAbstractItemView { border: 1px solid #cccccc; selection-background-color: #e0e0e0; }"
    );
    
    // 设置图标
    m_headingComboBox->addItem("正文");
    m_headingComboBox->addItem("一级标题");
    m_headingComboBox->addItem("二级标题");
    m_headingComboBox->addItem("三级标题");
    m_headingComboBox->addItem("四级标题");
    m_headingComboBox->addItem("五级标题");
    m_headingComboBox->addItem("六级标题");
    headingLayout->addWidget(m_headingComboBox);
    m_topToolBar->addWidget(headingWidget);
    m_topToolBar->addSeparator();
    
    m_insertImageAction = m_topToolBar->addAction(QIcon(":/icons/editor/image.svg"), "插入图片");
    
    // 设置工具提示
    m_saveAction->setToolTip("保存笔记 (Ctrl+S)");
    m_undoAction->setToolTip("撤销上一步操作 (Ctrl+Z)");
    m_redoAction->setToolTip("重做上一步操作 (Ctrl+Y)");
    m_cutAction->setToolTip("剪切选中文本 (Ctrl+X)");
    m_copyAction->setToolTip("复制选中文本 (Ctrl+C)");
    m_pasteAction->setToolTip("粘贴文本 (Ctrl+V)");
    m_insertImageAction->setToolTip("从文件插入图片");
    
    // 连接信号
    connect(m_saveAction, &QAction::triggered, this, &TextEditorManager::onSaveTriggered);
    connect(m_undoAction, &QAction::triggered, this, &TextEditorManager::onUndoTriggered);
    connect(m_redoAction, &QAction::triggered, this, &TextEditorManager::onRedoTriggered);
    connect(m_cutAction, &QAction::triggered, this, &TextEditorManager::onCutTriggered);
    connect(m_copyAction, &QAction::triggered, this, &TextEditorManager::onCopyTriggered);
    connect(m_pasteAction, &QAction::triggered, this, &TextEditorManager::onPasteTriggered);
    connect(m_insertImageAction, &QAction::triggered, this, &TextEditorManager::onInsertImageTriggered);
    
    // 连接字体、字号和标题选择的信号
    connect(m_fontComboBox, &QFontComboBox::currentFontChanged, this, [this](const QFont &font) {
        onFontFamilyChanged(font.family());
    });
    connect(m_fontSizeComboBox, &QComboBox::currentTextChanged, this, &TextEditorManager::onFontSizeChanged);
    connect(m_headingComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &TextEditorManager::onHeadingChanged);
    
    // 绑定编辑器的撤销/重做状态
    connect(m_textEdit->document(), &QTextDocument::undoAvailable, m_undoAction, &QAction::setEnabled);
    connect(m_textEdit->document(), &QTextDocument::redoAvailable, m_redoAction, &QAction::setEnabled);
    
    // 初始状态设置
    m_undoAction->setEnabled(false);
    m_redoAction->setEnabled(false);
    
        // 设置工具栏的外观
    m_topToolBar->setStyleSheet("QToolBar { border: none; background-color: transparent; }");
}

void TextEditorManager::setTheme(bool isDarkTheme)
{
    m_isDarkTheme = isDarkTheme;
    
    // 设置浮动工具栏主题
    m_floatingToolBar->setTheme(isDarkTheme);
    
    // 设置文本编辑器主题 
    m_textEdit->setStyleSheet("");
    
    // 更新按钮图标颜色
    updateActionIcons();
}

void TextEditorManager::updateActionIcons()
{
    QColor iconColor = m_isDarkTheme ? Qt::white : QColor(50, 50, 50);
    QColor disabledColor = m_isDarkTheme ? QColor(120, 120, 120) : QColor(180, 180, 180);
    
    // 更新保存按钮图标
    QIcon saveIcon = createColorIcon(":/icons/editor/save.svg", iconColor, disabledColor);
    m_saveAction->setIcon(saveIcon);
    
    // 更新撤销按钮图标
    QIcon undoIcon = createColorIcon(":/icons/editor/undo.svg", iconColor, disabledColor);
    m_undoAction->setIcon(undoIcon);
    
    // 更新重做按钮图标
    QIcon redoIcon = createColorIcon(":/icons/editor/redo.svg", iconColor, disabledColor);
    m_redoAction->setIcon(redoIcon);
    
    // 更新剪切按钮图标
    QIcon cutIcon = createColorIcon(":/icons/editor/cut.svg", iconColor, disabledColor);
    m_cutAction->setIcon(cutIcon);
    
    // 更新复制按钮图标
    QIcon copyIcon = createColorIcon(":/icons/editor/copy.svg", iconColor, disabledColor);
    m_copyAction->setIcon(copyIcon);
    
    // 更新粘贴按钮图标
    QIcon pasteIcon = createColorIcon(":/icons/editor/paste.svg", iconColor, disabledColor);
    m_pasteAction->setIcon(pasteIcon);
    
    // 更新插入图片按钮图标
    QIcon imageIcon = createColorIcon(":/icons/editor/image.svg", iconColor, disabledColor);
    m_insertImageAction->setIcon(imageIcon);
}

QIcon TextEditorManager::createColorIcon(const QString &path, const QColor &color, const QColor &disabledColor)
{
    QIcon icon;
    
    // 加载SVG
    QSvgRenderer renderer(path);
    if (!renderer.isValid()) {
        qWarning() << "无效的SVG文件:" << path;
        return QIcon();
    }
    
    // 创建正常状态的图标
    QSize iconSize(24, 24);
    QPixmap normalPixmap(iconSize);
    normalPixmap.fill(Qt::transparent);
    
    QPainter normalPainter(&normalPixmap);
    
    // 在像素图上绘制SVG，替换颜色
    QSvgRenderer normalRenderer(path);
    
    // 创建与SVG文件内容相同的副本，但将颜色替换为当前主题颜色
    QByteArray svgContent;
    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) {
        svgContent = file.readAll();
        svgContent.replace("currentColor", color.name().toLatin1());
        file.close();
    }
    
    QSvgRenderer coloredRenderer(svgContent);
    if (coloredRenderer.isValid()) {
        coloredRenderer.render(&normalPainter);
    } else {
        // 如果颜色替换失败，使用原始SVG
        normalRenderer.render(&normalPainter);
    }
    normalPainter.end();
    
    // 创建禁用状态的图标
    QPixmap disabledPixmap(iconSize);
    disabledPixmap.fill(Qt::transparent);
    
    QPainter disabledPainter(&disabledPixmap);
    
    // 创建禁用状态的SVG内容
    QByteArray disabledSvgContent = svgContent;
    disabledSvgContent.replace(color.name().toLatin1(), disabledColor.name().toLatin1());
    
    QSvgRenderer disabledRenderer(disabledSvgContent);
    if (disabledRenderer.isValid()) {
        disabledRenderer.render(&disabledPainter);
    } else {
        // 如果颜色替换失败，使用原始SVG
        normalRenderer.render(&disabledPainter);
    }
    disabledPainter.end();
    
    // 添加到图标
    icon.addPixmap(normalPixmap, QIcon::Normal, QIcon::Off);
    icon.addPixmap(disabledPixmap, QIcon::Disabled, QIcon::Off);
    
    return icon;
}

void TextEditorManager::loadContent(const QString &content, const QString &path)
{
    m_currentFilePath = path;
    
    // 加载HTML内容到编辑器
    m_textEdit->setHtml(content);
    
    // 重置编辑器的修改状态
    m_textEdit->document()->setModified(false);
}

QString TextEditorManager::saveContent() const
{
    // 返回编辑器的HTML内容
    return m_textEdit->toHtml();
}

void TextEditorManager::handleSelectionChanged(const QPoint &pos, bool hasSelection)
{
    if (hasSelection) {
        // 如果有文本选中，更新工具栏位置并显示
        // 不使用传入的pos参数，让updatePosition自己确定位置
        m_floatingToolBar->updatePosition(QPoint());
        m_floatingToolBar->show();
        
        // 启动更新计时器，避免频繁更新
        m_updateToolBarTimer->start();
    } else {
        // 如果没有文本选中，隐藏工具栏
        m_floatingToolBar->hide();
    }
}

void TextEditorManager::handleTextEditClicked(const QPoint &pos)
{
    // 检查是否有文本选中
    bool hasSelection = m_textEdit->textCursor().hasSelection();
    
    if (!hasSelection) {
        // 如果没有文本选中，隐藏工具栏
        m_floatingToolBar->hide();
    }
}

void TextEditorManager::updateToolBarForCurrentFormat()
{
    // 获取当前光标的字符格式
    QTextCharFormat format = currentCharFormat();
    
    // 更新浮动工具栏按钮状态
    m_floatingToolBar->setFormatActions(format);
    
    // 获取当前块格式
    QTextBlockFormat blockFormat = m_textEdit->textCursor().blockFormat();
    Qt::Alignment alignment = blockFormat.alignment();
    
    // 更新对齐按钮状态
    m_floatingToolBar->alignLeftButton()->setChecked(alignment == Qt::AlignLeft);
    m_floatingToolBar->alignCenterButton()->setChecked(alignment == Qt::AlignCenter);
    m_floatingToolBar->alignRightButton()->setChecked(alignment == Qt::AlignRight);
    m_floatingToolBar->alignJustifyButton()->setChecked(alignment == Qt::AlignJustify);
    
    // 更新顶部工具栏的字体、字号选择器
    if (format.hasProperty(QTextFormat::FontFamilies)) { // <-- 使用 FontFamilies
        // 更新字体下拉框，防止信号循环
        QStringList families = format.fontFamilies().toStringList(); // <-- 使用 fontFamilies().toStringList()
        if (!families.isEmpty()) {
            QSignalBlocker blocker(m_fontComboBox);
            
            // 原始字体名称
            QString fontFamily = families.first();
            
            // 设置实际字体（用于内部存储）
            m_fontComboBox->setCurrentText(fontFamily);
            
            // 设置显示用的省略文本
            QString displayText = fontFamily;
            if (displayText.length() > 15) {
                displayText = displayText.left(12) + "...";
            }
            m_fontComboBox->lineEdit()->setText(displayText);
        }
    }
    
    if (format.hasProperty(QTextFormat::FontPointSize)) {
        // 更新字号下拉框，防止信号循环
        QSignalBlocker blocker(m_fontSizeComboBox);
        m_fontSizeComboBox->setCurrentText(QString::number(static_cast<int>(format.fontPointSize())));
    }
    
    // 更新标题级别下拉框
    // 这里需要根据当前块的字体大小和粗细来判断标题级别
    QSignalBlocker blocker(m_headingComboBox);
    
    int fontSize = static_cast<int>(format.fontPointSize());
    bool isBold = (format.fontWeight() >= QFont::Bold);
    
    if (!isBold || fontSize <= 12) {
        m_headingComboBox->setCurrentIndex(0); // 正文
    } else {
        // 根据字体大小判断标题级别
        if (fontSize >= 24) {
            m_headingComboBox->setCurrentIndex(1); // 一级标题
        } else if (fontSize >= 20) {
            m_headingComboBox->setCurrentIndex(2); // 二级标题
        } else if (fontSize >= 18) {
            m_headingComboBox->setCurrentIndex(3); // 三级标题
        } else if (fontSize >= 16) {
            m_headingComboBox->setCurrentIndex(4); // 四级标题
        } else if (fontSize >= 14) {
            m_headingComboBox->setCurrentIndex(5); // 五级标题
        } else {
            m_headingComboBox->setCurrentIndex(6); // 六级标题
        }
    }
    
    // 也更新浮动工具栏的下拉框（如果有）
    if (m_floatingToolBar->fontComboBox()) {
        if (format.hasProperty(QTextFormat::FontFamilies)) { // <-- 使用 FontFamilies
             QStringList families = format.fontFamilies().toStringList(); // <-- 使用 fontFamilies().toStringList()
             if (!families.isEmpty()) {
                QSignalBlocker ftblocker1(m_floatingToolBar->fontComboBox());
                
                // 原始字体名称
                QString fontFamily = families.first();
                
                // 设置实际字体（用于内部存储）
                m_floatingToolBar->fontComboBox()->setCurrentText(fontFamily);
                
                // 设置显示用的省略文本
                QString displayText = fontFamily;
                if (displayText.length() > 15) {
                    displayText = displayText.left(12) + "...";
                }
                m_floatingToolBar->fontComboBox()->lineEdit()->setText(displayText);
             }
        }
    }
    
    if (m_floatingToolBar->fontSizeComboBox()) {
        QSignalBlocker ftblocker2(m_floatingToolBar->fontSizeComboBox());
        m_floatingToolBar->fontSizeComboBox()->setCurrentText(QString::number(static_cast<int>(format.fontPointSize())));
    }
    
    if (m_floatingToolBar->headingComboBox()) {
        QSignalBlocker ftblocker3(m_floatingToolBar->headingComboBox());
        m_floatingToolBar->headingComboBox()->setCurrentIndex(m_headingComboBox->currentIndex());
    }
}

QTextCharFormat TextEditorManager::currentCharFormat() const
{
    return m_textEdit->textCursor().charFormat();
}

void TextEditorManager::mergeFormatOnWordOrSelection(const QTextCharFormat &format)
{
    QTextCursor cursor = m_textEdit->textCursor();
    if (!cursor.hasSelection()) {
        cursor.select(QTextCursor::WordUnderCursor);
    }
    cursor.mergeCharFormat(format);
    m_textEdit->mergeCurrentCharFormat(format);
}

void TextEditorManager::setAlignment(Qt::Alignment alignment)
{
    m_textEdit->setAlignment(alignment);
}

// 格式化操作
void TextEditorManager::bold()
{
    QTextCharFormat fmt;
    fmt.setFontWeight(m_floatingToolBar->boldButton()->isChecked() ? QFont::Bold : QFont::Normal);
    mergeFormatOnWordOrSelection(fmt);
}

void TextEditorManager::italic()
{
    QTextCharFormat fmt;
    fmt.setFontItalic(m_floatingToolBar->italicButton()->isChecked());
    mergeFormatOnWordOrSelection(fmt);
}

void TextEditorManager::underline()
{
    QTextCharFormat fmt;
    fmt.setFontUnderline(m_floatingToolBar->underlineButton()->isChecked());
    mergeFormatOnWordOrSelection(fmt);
}

void TextEditorManager::strikeOut()
{
    QTextCharFormat fmt;
    fmt.setFontStrikeOut(m_floatingToolBar->strikeOutButton()->isChecked());
    mergeFormatOnWordOrSelection(fmt);
}

void TextEditorManager::setTextColor(const QColor &color)
{
    m_textColor = color;
    QTextCharFormat fmt;
    fmt.setForeground(QBrush(color));
    mergeFormatOnWordOrSelection(fmt);
}

void TextEditorManager::setHighlightColor(const QColor &color)
{
    m_highlightColor = color;
    QTextCharFormat fmt;
    fmt.setBackground(QBrush(color));
    mergeFormatOnWordOrSelection(fmt);
}

void TextEditorManager::alignLeft()
{
    setAlignment(Qt::AlignLeft);
}

void TextEditorManager::alignCenter()
{
    setAlignment(Qt::AlignCenter);
}

void TextEditorManager::alignRight()
{
    setAlignment(Qt::AlignRight);
}

void TextEditorManager::alignJustify()
{
    setAlignment(Qt::AlignJustify);
}

// 工具栏按钮点击槽函数
void TextEditorManager::onBoldTriggered()
{
    bold();
}

void TextEditorManager::onItalicTriggered()
{
    italic();
}

void TextEditorManager::onUnderlineTriggered()
{
    underline();
}

void TextEditorManager::onStrikeOutTriggered()
{
    strikeOut();
}

void TextEditorManager::onTextColorTriggered()
{
    QColor color = QColorDialog::getColor(m_textColor, m_textEdit, "选择文本颜色");
    if (color.isValid()) {
        setTextColor(color);
    }
}

void TextEditorManager::onHighlightTriggered()
{
    QColor color = QColorDialog::getColor(m_highlightColor, m_textEdit, "选择高亮颜色");
    if (color.isValid()) {
        setHighlightColor(color);
    }
}

void TextEditorManager::onAlignLeftTriggered()
{
    alignLeft();
    updateToolBarForCurrentFormat();
}

void TextEditorManager::onAlignCenterTriggered()
{
    alignCenter();
    updateToolBarForCurrentFormat();
}

void TextEditorManager::onAlignRightTriggered()
{
    alignRight();
    updateToolBarForCurrentFormat();
}

void TextEditorManager::onAlignJustifyTriggered()
{
    alignJustify();
    updateToolBarForCurrentFormat();
}

// 顶部工具栏槽函数
void TextEditorManager::onSaveTriggered()
{
    QString content = saveContent();
    emit contentModified();
    qDebug() << "保存笔记：" << m_currentFilePath;
    
    // 以下是一个简单的文件保存测试，用于开发阶段
    if (!m_currentFilePath.isEmpty()) {
        QFile file(m_currentFilePath + ".html");
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream stream(&file);
            stream << content;
            file.close();
            qDebug() << "文件已保存到：" << m_currentFilePath + ".html";
        } else {
            qDebug() << "无法保存文件：" << file.errorString();
        }
    } else {
        // 如果没有当前文件路径，创建一个临时文件
        QString tempPath = QDir::tempPath() + "/temp_note_" + 
                          QString::number(QDateTime::currentMSecsSinceEpoch()) + ".html";
        QFile file(tempPath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream stream(&file);
            stream << content;
            file.close();
            qDebug() << "文件已保存到临时位置：" << tempPath;
        }
    }
}

void TextEditorManager::onUndoTriggered()
{
    m_textEdit->undo();
}

void TextEditorManager::onRedoTriggered()
{
    m_textEdit->redo();
}

void TextEditorManager::onCutTriggered()
{
    m_textEdit->cut();
}

void TextEditorManager::onCopyTriggered()
{
    m_textEdit->copy();
}

void TextEditorManager::onPasteTriggered()
{
    m_textEdit->paste();
}

void TextEditorManager::onInsertImageTriggered()
{
    QString filePath = QFileDialog::getOpenFileName(
        m_textEdit, "选择图片",
        QString(), "图像文件 (*.png *.jpg *.jpeg *.bmp *.gif)");
    
    if (!filePath.isEmpty()) {
        m_textEdit->insertImageFromFile(filePath);
    }
}

void TextEditorManager::setReadOnly(bool readOnly)
{
    m_textEdit->setReadOnly(readOnly);
    
    // 禁用或启用相关操作
    m_saveAction->setEnabled(!readOnly);
    m_cutAction->setEnabled(!readOnly);
    m_pasteAction->setEnabled(!readOnly);
    m_insertImageAction->setEnabled(!readOnly);
}

void TextEditorManager::loadNote(const QString &notePath)
{
    m_currentNotePath = notePath;
    
    // 如果路径为空，加载默认内容
    if (notePath.isEmpty()) {
        m_textEdit->setHtml("<html><body><p>欢迎使用IntelliMedia Notes！</p><p>请从侧边栏选择一个笔记或创建新笔记开始。</p></body></html>");
        return;
    }
    
    m_textEdit->setHtml("<html><body><h1>笔记：" + notePath + "</h1><p>这是来自路径" + notePath + "的笔记内容</p></body></html>");
    
    // 重置修改状态
    m_textEdit->document()->setModified(false);
    m_hasUnsavedChanges = false;
}

void TextEditorManager::saveNote()
{
    if (m_currentNotePath.isEmpty()) {
        return;
    }
    
    QString content = m_textEdit->toHtml();
    
    // 打印日志
    qDebug() << "保存笔记内容:" << m_currentNotePath;
    
    // 重置修改状态
    m_textEdit->document()->setModified(false);
    m_hasUnsavedChanges = false;
}

void TextEditorManager::insertImageFromButton()
{
    // 调用onInsertImageTriggered方法
    onInsertImageTriggered();
}

void TextEditorManager::handleEditorClicked(const QPoint &pos)
{
    // 检查是否有文本选中
    bool hasSelection = m_textEdit->textCursor().hasSelection();
    
    if (!hasSelection) {
        // 如果没有文本选中，隐藏工具栏
        m_floatingToolBar->hide();
    }
}

void TextEditorManager::documentModified()
{
    // 标记有未保存的更改
    m_hasUnsavedChanges = true;
    
    // 发出内容修改信号
    emit contentModified();
}

void TextEditorManager::setDarkTheme(bool dark)
{
    // 调用setTheme方法实现功能
    setTheme(dark);
}

void TextEditorManager::onFontFamilyChanged(const QString &family)
{
    QTextCharFormat fmt;
    fmt.setFontFamilies({family}); // <-- 使用 setFontFamilies
    mergeFormatOnWordOrSelection(fmt);
    documentModified();
}

void TextEditorManager::onFontSizeChanged(const QString &size)
{
    bool ok;
    int fontSize = size.toInt(&ok);
    if (ok) {
        QTextCharFormat fmt;
        fmt.setFontPointSize(fontSize);
        mergeFormatOnWordOrSelection(fmt);
        documentModified();
    }
}

void TextEditorManager::onHeadingChanged(int index)
{
    applyHeading(index);
    documentModified();
}

void TextEditorManager::applyHeading(int level)
{
    QTextCursor cursor = m_textEdit->textCursor();
    QTextBlockFormat blockFormat = cursor.blockFormat();
    QTextCharFormat charFormat = cursor.charFormat();
    
    // 保存当前块的对齐方式
    Qt::Alignment alignment = blockFormat.alignment();
    
    // 首先清除任何现有的字体大小和粗体设置
    charFormat.setFontWeight(QFont::Normal);
    
    if (level == 0) { // 正文
        // 正文样式
        charFormat.setFontPointSize(12); // 正文使用12号字体
    } else {
        // 标题样式 (标题级别从1开始，对应h1到h6)
        charFormat.setFontWeight(QFont::Bold); // 所有标题都使用粗体
        
        // 根据标题级别设置字体大小
        switch (level) {
            case 1: // 一级标题
                charFormat.setFontPointSize(24);
                break;
            case 2: // 二级标题
                charFormat.setFontPointSize(20);
                break;
            case 3: // 三级标题
                charFormat.setFontPointSize(18);
                break;
            case 4: // 四级标题
                charFormat.setFontPointSize(16);
                break;
            case 5: // 五级标题
                charFormat.setFontPointSize(14);
                break;
            case 6: // 六级标题
                charFormat.setFontPointSize(13);
                break;
        }
    }
    
    // 开始修改块和字符格式
    cursor.beginEditBlock();
    
    // 应用块格式（保持原有的对齐方式）
    blockFormat.setAlignment(alignment);
    cursor.setBlockFormat(blockFormat);
    
    // 应用字符格式（字体粗细和大小）
    cursor.select(QTextCursor::BlockUnderCursor);
    cursor.mergeCharFormat(charFormat);
    
    cursor.endEditBlock();
    
    // 不需要调用 m_textEdit->mergeCurrentCharFormat(charFormat)，因为我们已经直接设置了整个块的格式
    
    // 更新焦点回到编辑器
    m_textEdit->setFocus();
}

void TextEditorManager::setupFontComboBoxes()
{
    const int topToolbarMaxLength = 15;
    const int topToolbarKeepLength = 12;
    const int floatToolbarMaxLength = 10; // 悬浮工具栏使用更短的阈值
    const int floatToolbarKeepLength = 7;  // 悬浮工具栏保留更少的字符
    
    // 主工具栏字体下拉框设置
    m_fontComboBox->setEditable(true);
    m_fontComboBox->lineEdit()->setReadOnly(true);
    
    // 处理字体名称显示
    connect(m_fontComboBox, &QFontComboBox::currentTextChanged, this, [this, topToolbarMaxLength, topToolbarKeepLength, floatToolbarMaxLength, floatToolbarKeepLength](const QString &text) {
        // 仅当不是从代码设置时触发，避免信号循环
        if (m_fontComboBox->signalsBlocked())
            return;
            
        // 应用字体，但不更改显示文本
        QTextCharFormat fmt;
        fmt.setFontFamilies({text});
        mergeFormatOnWordOrSelection(fmt);
        
        // 设置主工具栏省略显示文本
        setEllipsisDisplayText(m_fontComboBox, text, topToolbarMaxLength, topToolbarKeepLength);
        
        // 同步浮动工具栏的字体和显示文本
        if (m_floatingToolBar && m_floatingToolBar->fontComboBox()) {
             QSignalBlocker ftblocker(m_floatingToolBar->fontComboBox());
             m_floatingToolBar->fontComboBox()->setCurrentText(text); // 设置完整字体名称
             setEllipsisDisplayText(m_floatingToolBar->fontComboBox(), text, floatToolbarMaxLength, floatToolbarKeepLength); // 设置省略显示
        }
    });
    
    // 处理初始状态
    setEllipsisDisplayText(m_fontComboBox, m_fontComboBox->currentText(), topToolbarMaxLength, topToolbarKeepLength);
    
    // 浮动工具栏字体下拉框设置
    if (m_floatingToolBar && m_floatingToolBar->fontComboBox()) {
        m_floatingToolBar->fontComboBox()->setEditable(true);
        m_floatingToolBar->fontComboBox()->lineEdit()->setReadOnly(true);
        
        // 处理浮动工具栏字体名称显示
        connect(m_floatingToolBar->fontComboBox(), &QFontComboBox::currentTextChanged, this, [this, topToolbarMaxLength, topToolbarKeepLength, floatToolbarMaxLength, floatToolbarKeepLength](const QString &text) {
            // 仅当不是从代码设置时触发，避免信号循环
            if (m_floatingToolBar->fontComboBox()->signalsBlocked())
                return;
                
            // 应用字体，但不更改显示文本
            QTextCharFormat fmt;
            fmt.setFontFamilies({text});
            mergeFormatOnWordOrSelection(fmt);
            
            // 设置浮动工具栏省略显示文本
            setEllipsisDisplayText(m_floatingToolBar->fontComboBox(), text, floatToolbarMaxLength, floatToolbarKeepLength);
            
            // 同步主工具栏的字体和显示文本
             QSignalBlocker mblocker(m_fontComboBox);
             m_fontComboBox->setCurrentText(text); // 设置完整字体名称
             setEllipsisDisplayText(m_fontComboBox, text, topToolbarMaxLength, topToolbarKeepLength); // 设置省略显示
        });
        
        // 处理初始状态
        setEllipsisDisplayText(m_floatingToolBar->fontComboBox(), m_floatingToolBar->fontComboBox()->currentText(), floatToolbarMaxLength, floatToolbarKeepLength);
    }
}

// 将 setEllipsisDisplayText 的实现放在这里
void TextEditorManager::setEllipsisDisplayText(QComboBox *comboBox, const QString &fullText, int maxLength, int keepLength)
{
    if (!comboBox || !comboBox->lineEdit()) return;
    
    QString displayText = fullText;
    if (displayText.length() > maxLength) {
        displayText = displayText.left(keepLength) + "...";
    }
    
    // 更新显示文本，但不触发信号
    QSignalBlocker blocker(comboBox);
    comboBox->lineEdit()->setText(displayText);
}

// 确保只有一个 updateToolBarForCurrentFormat 实现 