#include "ChatSettingsDialog.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayoutItem>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

ChatSettingsDialog::ChatSettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Chat Settings"));
    setModal(true);
    resize(480, 560);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* header = new QWidget(this);
    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(12, 10, 12, 10);
    headerLayout->setSpacing(8);
    m_titleLabel = new QLabel(QStringLiteral("Chat Settings"), header);
    m_titleLabel->setObjectName(QStringLiteral("chatSettingsTitleLabel"));
    auto* closeBtn = new QPushButton(QStringLiteral("x"), header);
    closeBtn->setObjectName(QStringLiteral("closeBtn"));
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setFixedWidth(30);
    headerLayout->addWidget(m_titleLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(closeBtn);
    root->addWidget(header);

    auto* separator = new QFrame(this);
    separator->setObjectName(QStringLiteral("chatSettingsDivider"));
    separator->setFrameShape(QFrame::HLine);
    root->addWidget(separator);

    auto* body = new QWidget(this);
    auto* bodyLayout = new QVBoxLayout(body);
    bodyLayout->setContentsMargins(14, 12, 14, 12);
    bodyLayout->setSpacing(10);

    auto* chatLabel = new QLabel(QStringLiteral("Chat"), body);
    chatLabel->setObjectName(QStringLiteral("chatSettingsSectionLabel"));
    m_chatNameLabel = new QLabel(body);
    m_chatNameLabel->setObjectName(QStringLiteral("chatSettingsChatNameLabel"));
    bodyLayout->addWidget(chatLabel);
    bodyLayout->addWidget(m_chatNameLabel);

    auto* membersLabel = new QLabel(QStringLiteral("Members"), body);
    membersLabel->setObjectName(QStringLiteral("chatSettingsSectionLabel"));
    bodyLayout->addWidget(membersLabel);

    auto* scroll = new QScrollArea(body);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    m_membersHost = new QWidget(scroll);
    m_membersLayout = new QVBoxLayout(m_membersHost);
    m_membersLayout->setContentsMargins(0, 0, 0, 0);
    m_membersLayout->setSpacing(6);
    m_membersLayout->addStretch();
    scroll->setWidget(m_membersHost);
    bodyLayout->addWidget(scroll, 1);

    m_addMembersButton = new QPushButton(QStringLiteral("Add Members"), body);
    m_addMembersButton->setObjectName(QStringLiteral("actionBtn"));
    m_addMembersButton->setCursor(Qt::PointingHandCursor);
    bodyLayout->addWidget(m_addMembersButton);

    auto* actions = new QHBoxLayout();
    m_removeHandleButton = new QPushButton(QStringLiteral("Remove Handle"), body);
    m_removeHandleButton->setObjectName(QStringLiteral("warningBtn"));
    m_removeHandleButton->setCursor(Qt::PointingHandCursor);
    m_deleteChatButton = new QPushButton(QStringLiteral("Delete Chat"), body);
    m_deleteChatButton->setObjectName(QStringLiteral("dangerBtn"));
    m_deleteChatButton->setCursor(Qt::PointingHandCursor);
    actions->addWidget(m_removeHandleButton);
    actions->addStretch();
    actions->addWidget(m_deleteChatButton);
    bodyLayout->addLayout(actions);

    root->addWidget(body, 1);

    connect(closeBtn, &QPushButton::clicked, this, &ChatSettingsDialog::hide);
    connect(m_addMembersButton, &QPushButton::clicked, this, [this]() {
        if (!m_chatId.isEmpty()) {
            emit addMembersRequested(m_chatId);
        }
    });
    connect(m_removeHandleButton, &QPushButton::clicked, this, [this]() {
        if (!m_chatId.isEmpty()) {
            emit removeHandleRequested(m_chatId);
        }
    });
    connect(m_deleteChatButton, &QPushButton::clicked, this, [this]() {
        if (!m_chatId.isEmpty()) {
            emit deleteChatRequested(m_chatId);
        }
    });
    applyTheme();
}

void ChatSettingsDialog::setChatData(const Chat& chat, const QMap<QString, User>& users, const QString& currentUserId)
{
    m_chatId = chat.chatId;
    const QString resolvedName = chat.chatName.isEmpty() ? chat.chatId : chat.chatName;
    m_titleLabel->setText(QStringLiteral("Settings - %1").arg(resolvedName));
    m_chatNameLabel->setText(resolvedName);

    clearMemberRows();

    const bool isOwner = (!chat.ownerId.isEmpty() && chat.ownerId == currentUserId);
    for (const QString& memberId : chat.members) {
        const QString username = users.contains(memberId) ? users[memberId].username : memberId;
        const bool memberIsOwner = (memberId == chat.ownerId);
        QWidget* row = createMemberRow(memberId, username, memberIsOwner, isOwner && !memberIsOwner);
        m_membersLayout->insertWidget(m_membersLayout->count() - 1, row);
    }

    m_addMembersButton->setVisible(isOwner && chat.chatType == QStringLiteral("group"));
    m_removeHandleButton->setVisible(isOwner);
    m_deleteChatButton->setVisible(isOwner);
}

QString ChatSettingsDialog::chatId() const
{
    return m_chatId;
}

void ChatSettingsDialog::setDarkMode(bool enabled)
{
    m_darkMode = enabled;
    applyTheme();
}

void ChatSettingsDialog::applyTheme()
{
    const QString bg = m_darkMode ? QStringLiteral("#111827") : QStringLiteral("#ffffff");
    const QString text = m_darkMode ? QStringLiteral("#e5e7eb") : QStringLiteral("#111827");
    const QString muted = m_darkMode ? QStringLiteral("#9ca3af") : QStringLiteral("#6b7280");
    const QString divider = m_darkMode ? QStringLiteral("#374151") : QStringLiteral("#e5e7eb");
    const QString memberRowBg = m_darkMode ? QStringLiteral("#1f2937") : QStringLiteral("#f9fafb");

    setStyleSheet(QString(
        "QDialog { background: %1; color: %2; }"
        "#chatSettingsTitleLabel { font-size: 18px; font-weight: 700; color: %2; }"
        "#chatSettingsDivider { color: %4; background: %4; }"
        "#chatSettingsSectionLabel { font-size: 12px; color: %3; }"
        "#chatSettingsChatNameLabel { font-size: 15px; font-weight: 700; color: %2; }"
        "#chatSettingsMemberName { font-weight: 700; color: %2; }"
        "#chatSettingsMemberRole { font-size: 11px; color: %3; }"
        "QPushButton#closeBtn { border: none; color: %3; font-weight: 700; }"
        "QPushButton#closeBtn:hover { color: %2; }"
        "QFrame#memberRow { border-radius: 8px; background: %5; }"
        "QPushButton#removeMemberBtn {"
        " border: none; border-radius: 6px; padding: 5px 8px; background: #ef4444; color: #ffffff; }"
        "QPushButton#removeMemberBtn:hover { background: #dc2626; }"
        "QPushButton#actionBtn {"
        " border: none; border-radius: 8px; padding: 8px 12px; color: #ffffff; background: #2563eb; }"
        "QPushButton#actionBtn:hover { background: #1d4ed8; }"
        "QPushButton#dangerBtn {"
        " border: none; border-radius: 8px; padding: 8px 12px; color: #ffffff; background: #dc2626; }"
        "QPushButton#dangerBtn:hover { background: #b91c1c; }"
        "QPushButton#warningBtn {"
        " border: none; border-radius: 8px; padding: 8px 12px; color: #ffffff; background: #d97706; }"
        "QPushButton#warningBtn:hover { background: #b45309; }")
                      .arg(bg, text, muted, divider, memberRowBg));
}

void ChatSettingsDialog::clearMemberRows()
{
    if (!m_membersLayout) {
        return;
    }
    while (m_membersLayout->count() > 1) {
        QLayoutItem* item = m_membersLayout->takeAt(0);
        if (!item) {
            continue;
        }
        if (QWidget* widget = item->widget()) {
            widget->deleteLater();
        }
        delete item;
    }
}

QWidget* ChatSettingsDialog::createMemberRow(const QString& userId, const QString& username, bool isOwner, bool canRemove)
{
    auto* row = new QFrame(m_membersHost);
    row->setObjectName(QStringLiteral("memberRow"));
    auto* rowLayout = new QHBoxLayout(row);
    rowLayout->setContentsMargins(10, 8, 10, 8);
    rowLayout->setSpacing(8);

    auto* info = new QVBoxLayout();
    auto* name = new QLabel(username, row);
    name->setObjectName(QStringLiteral("chatSettingsMemberName"));
    auto* role = new QLabel(isOwner ? QStringLiteral("Owner") : QStringLiteral("Member"), row);
    role->setObjectName(QStringLiteral("chatSettingsMemberRole"));
    info->addWidget(name);
    info->addWidget(role);
    rowLayout->addLayout(info, 1);

    if (canRemove) {
        auto* removeBtn = new QPushButton(QStringLiteral("Remove"), row);
        removeBtn->setObjectName(QStringLiteral("removeMemberBtn"));
        removeBtn->setCursor(Qt::PointingHandCursor);
        rowLayout->addWidget(removeBtn, 0, Qt::AlignRight);
        connect(removeBtn, &QPushButton::clicked, this, [this, userId]() {
            if (!m_chatId.isEmpty()) {
                emit removeMemberRequested(m_chatId, userId);
            }
        });
    }
    return row;
}
