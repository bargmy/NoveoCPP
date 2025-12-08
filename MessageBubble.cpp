#include "MessageBubble.h"

// NOTE: This class is technically obsolete. 
// We now use the high-performance 'MessageDelegate' inside MainWindow.cpp 
// to render messages without lag.
//
// This file is kept here solely to prevent linker errors if 'MessageBubble.cpp' 
// is still listed in your CMakeLists.txt.

MessageBubble::MessageBubble(const QString &text, const QString &senderName, qint64 timestamp, bool isMe, QWidget *parent)
    : QWidget(parent)
{
    // Mark parameters as unused to prevent compiler warnings
    Q_UNUSED(text);
    Q_UNUSED(senderName);
    Q_UNUSED(timestamp);
    Q_UNUSED(isMe);
    
    // Hide this widget since it shouldn't be seen if accidentally created
    this->hide();
}

void MessageBubble::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    // Do nothing. The rendering is now handled by MessageDelegate in MainWindow.cpp
}
