#ifndef MESSAGEBUBBLE_H
#define MESSAGEBUBBLE_H

#include <QWidget>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QDateTime>

class MessageBubble : public QWidget {
    Q_OBJECT
public:
    MessageBubble(const QString &text, const QString &senderName, qint64 timestamp, bool isMe, QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    bool m_isMe;
    QColor m_backgroundColor;
    QColor m_textColor;
};

#endif // MESSAGEBUBBLE_H
