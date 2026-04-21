#ifndef MESSAGEBUBBLE_H
#define MESSAGEBUBBLE_H

#include <QWidget>

class MessageBubble : public QWidget {
    Q_OBJECT
public:
    MessageBubble(const QString& text, const QString& senderName, qint64 timestamp, bool isMe, QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;
};

#endif // MESSAGEBUBBLE_H
