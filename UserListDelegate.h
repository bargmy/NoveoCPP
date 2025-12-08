#ifndef USERLISTDELEGATE_H
#define USERLISTDELEGATE_H

#include <QStyledItemDelegate>
#include <QPainter>
#include <QRegularExpression>

class UserListDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        painter->save();

        // 1. Draw Background (handles hover/selection automatically via style)
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);
        option.widget->style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, option.widget);

        // 2. Extract Data
        QString fullText = index.data(Qt::DisplayRole).toString();
        QIcon icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
        bool isOnline = index.data(Qt::UserRole + 1).toBool(); // Retrieved from UserRole+1

        QRect rect = opt.rect;
        const int padding = 10;
        const int iconSize = 40;

        // 3. Draw Icon (PFP)
        if (!icon.isNull()) {
            int iconY = rect.top() + (rect.height() - iconSize) / 2;
            icon.paint(painter, rect.left() + padding, iconY, iconSize, iconSize);

            // --- ONLINE STATUS INDICATOR ---
            if (isOnline) {
                int dotSize = 10;
                // Position bottom-right of avatar
                int dotX = rect.left() + padding + iconSize - dotSize + 2;
                int dotY = iconY + iconSize - dotSize + 2;
                
                painter->setPen(Qt::NoPen);
                
                // Draw white border (or background color) to separate dot from avatar
                painter->setBrush(opt.palette.window()); 
                painter->drawEllipse(dotX - 1, dotY - 1, dotSize + 2, dotSize + 2);

                // Draw Green Dot
                painter->setBrush(QColor("#4CE668")); // Bright Green
                painter->drawEllipse(dotX, dotY, dotSize, dotSize);
            }
        }

        // 4. Parse Nametag: Name [#color, "Tag"]
        static QRegularExpression regex(R"(^(.*)\s\[#([a-fA-F0-9]{6}),\s*"(.*)"\]$)");
        QRegularExpressionMatch match = regex.match(fullText);
        
        QString name;
        QString tagText;
        QColor tagColor;
        bool hasTag = false;

        if (match.hasMatch()) {
            name = match.captured(1).trimmed();
            tagColor = QColor("#" + match.captured(2));
            tagText = match.captured(3);
            hasTag = true;
        } else {
            name = fullText;
        }

        // 5. Draw Name
        QColor textColor = (opt.state & QStyle::State_Selected) ? Qt::white : opt.palette.text().color();
        painter->setPen(textColor);
        QFont nameFont = opt.font;
        nameFont.setPixelSize(14);
        painter->setFont(nameFont);

        int textX = rect.left() + padding + iconSize + padding;
        QFontMetrics fm(nameFont);
        int nameWidth = fm.horizontalAdvance(name);

        QRect nameRect(textX, rect.top(), nameWidth, rect.height());
        painter->drawText(nameRect, Qt::AlignVCenter | Qt::AlignLeft, name);

        // 6. Draw Nametag (if present)
        if (hasTag) {
            int tagX = textX + nameWidth + 8;
            int tagH = 18;
            int tagY = rect.center().y() - tagH / 2;
            
            QFont tagFont = nameFont;
            tagFont.setPixelSize(10);
            tagFont.setBold(true);
            QFontMetrics tagFm(tagFont);
            int tagW = tagFm.horizontalAdvance(tagText) + 12;

            // Tag Background
            painter->setBrush(tagColor);
            painter->setPen(Qt::NoPen);
            painter->drawRoundedRect(tagX, tagY, tagW, tagH, 4, 4);

            // Tag Text
            painter->setPen(Qt::white);
            painter->setFont(tagFont);
            painter->drawText(QRect(tagX, tagY, tagW, tagH), Qt::AlignCenter, tagText);
        }

        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        Q_UNUSED(index);
        return QSize(option.rect.width(), 60); // Enforce consistent row height
    }
};

#endif // USERLISTDELEGATE_H
