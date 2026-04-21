#include "MessageBubble.h"

MessageBubble::MessageBubble(const QString& text, const QString& senderName, qint64 timestamp, bool isMe, QWidget* parent)
    : QWidget(parent)
{
    Q_UNUSED(text);
    Q_UNUSED(senderName);
    Q_UNUSED(timestamp);
    Q_UNUSED(isMe);
    hide();
}

void MessageBubble::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
}
