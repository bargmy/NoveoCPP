#include "MessageBubble.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDateTime>
#include <QRegularExpression>

MessageBubble::MessageBubble(const QString &text, const QString &senderName, qint64 timestamp, bool isMe, QWidget *parent)
    : QWidget(parent), m_isMe(isMe)
{
    // Colors
    if (m_isMe) {
        m_backgroundColor = QColor("#EEFFDE"); // Light Green
        m_textColor = Qt::black;
    } else {
        m_backgroundColor = Qt::white; // White for others
        m_textColor = Qt::black;
    }

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setAlignment(m_isMe ? Qt::AlignRight : Qt::AlignLeft);
    mainLayout->setContentsMargins(10, 5, 10, 5);

    QWidget *bubble = new QWidget(this);
    bubble->setObjectName("bubble");
    // Max width ~70%
    bubble->setMaximumWidth(600);

    QVBoxLayout *bubbleLayout = new QVBoxLayout(bubble);
    bubbleLayout->setContentsMargins(12, 8, 12, 8);
    bubbleLayout->setSpacing(4);

    if (!m_isMe) {
        // --- NAMETAG PARSING START ---
        // Regex to find "Name [#color, "TAG"]"
        // CHANGED: Using standard escaped string instead of raw string
        static QRegularExpression regex("^(.*)\\s\\[#([a-fA-F0-9]{6}),\\s*\"(.*)\"\\]$");
        QRegularExpressionMatch match = regex.match(senderName);

        QString displayName = senderName;
        QString tagText;
        QColor tagColor;
        bool hasTag = false;

        if (match.hasMatch()) {
            displayName = match.captured(1).trimmed();
            tagColor = QColor("#" + match.captured(2));
            tagText = match.captured(3);
            hasTag = true;
        }
        // --- NAMETAG PARSING END ---

        // Name Header Layout
        QHBoxLayout *nameLayout = new QHBoxLayout();
        nameLayout->setContentsMargins(0, 0, 0, 0);
        nameLayout->setSpacing(5);

        // Name Label
        QLabel *nameLabel = new QLabel(displayName);
        nameLabel->setStyleSheet("color: #E35967; font-weight: bold; font-size: 11px; background: transparent; border: none;");
        nameLayout->addWidget(nameLabel);

        // Tag Label (if detected)
        if (hasTag) {
            QLabel *tagLabel = new QLabel(tagText);
            // Render the tag with the parsed color and rounded corners
            tagLabel->setStyleSheet(QString(
                "background-color: %1; color: white; font-weight: bold; font-size: 9px; "
                "padding: 2px 5px; border-radius: 4px;"
            ).arg(tagColor.name()));
            nameLayout->addWidget(tagLabel);
        }

        nameLayout->addStretch();
        bubbleLayout->addLayout(nameLayout);
    }

    QLabel *msgLabel = new QLabel(text);
    msgLabel->setWordWrap(true);
    msgLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    msgLabel->setStyleSheet(QString("color: %1; font-size: 13px; background: transparent; border: none;").arg(m_isMe ? "black" : "black"));
    bubbleLayout->addWidget(msgLabel);

    QDateTime dt;
    dt.setSecsSinceEpoch(timestamp);
    QLabel *timeLabel = new QLabel(dt.toString("hh:mm AP"));
    timeLabel->setStyleSheet("color: #888; font-size: 10px; background: transparent; border: none;");
    timeLabel->setAlignment(Qt::AlignRight);
    bubbleLayout->addWidget(timeLabel);

    mainLayout->addWidget(bubble);
}

void MessageBubble::paintEvent(QPaintEvent *event) {
    QWidget *bubble = findChild<QWidget*>("bubble");
    if (bubble) {
        // Enforce bubble styling here to override global stylesheet issues
        bubble->setStyleSheet(QString(
            "background-color: %1;"
            "border-radius: 10px;"
            "border-bottom-%2-radius: 0px;"
            "border: 1px solid #d0d0d0;"
        ).arg(m_backgroundColor.name()).arg(m_isMe ? "right" : "left"));
    }
    QWidget::paintEvent(event);
}
