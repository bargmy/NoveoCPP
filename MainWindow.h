#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QIcon>
#include <QStyleOptionViewItem>
#include <QModelIndex>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QStackedWidget>
#include <QMap>
#include <QSet>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include "WebSocketClient.h"
#include "DataStructures.h"


class UserListDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    ~UserListDelegate() override = default;

    void paint(QPainter* painter,
        const QStyleOptionViewItem& option,
        const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option,
        const QModelIndex& index) const override;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    QString getReplyPreviewText(const QString& replyToId, const QString& chatId);

protected:
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onConnected();
    void onLoginBtnClicked();
    void onLoginSuccess(const User& user, const QString& token);
    void onAuthFailed(const QString& msg);
    void onChatHistoryReceived(const std::vector<Chat>& chats);
    void onMessageReceived(const Message& msg);
    void onUserListUpdated(const std::vector<User>& users);
    void onSendBtnClicked();
    void onNewChatCreated(const Chat& chat);
    void onMessageSeenUpdate(const QString& chatId, const QString& messageId, const QString& userId);

    void onChatSelected(QListWidgetItem* item);
    void onContactSelected(QListWidgetItem* item);
    void onDarkModeToggled(bool checked);
    void onLogoutClicked();

    void onChatListContextMenu(const QPoint& pos);
    void onEditMessage();
    void onDeleteMessage();
    void onReplyToMessage();
    void onForwardMessage();
    void onCancelEdit();
    void onCancelReply();
    void onMessageUpdated(const QString& chatId, const QString& messageId, const QString& newContent, qint64 editedAt);
    void onMessageDeleted(const QString& chatId, const QString& messageId);

    void onScrollValueChanged(int value);
    void onChatListItemClicked(const QModelIndex& index);

private:
    void setupUi();
    void applyTheme();

    void renderMessages(const QString& chatId);
    void addMessageBubble(const Message& msg, bool appendStretch, bool animate);
    void prependMessageBubble(const Message& msg);

    QString resolveChatName(const Chat& chat);
    QColor getColorForName(const QString& name);

    QIcon getAvatar(const QString& name, const QString& url);
    QIcon generateGenericAvatar(const QString& name);
    void updateAvatarOnItems(const QString& url, const QPixmap& pixmap);

    void scrollToBottom();
    void smoothScrollToBottom();
    bool isScrolledToBottom() const;

    void updateMessageStatus(const QString& messageId, MessageStatus newStatus);
    MessageStatus calculateMessageStatus(const Message& msg, const Chat& chat);
    void focusOnMessage(const QString& messageId);

    WebSocketClient* m_client;
    QNetworkAccessManager* m_nam;

    QStackedWidget* m_stackedWidget;
    QWidget* m_loginPage;
    QWidget* m_appPage;
    QLineEdit* m_usernameInput;
    QLineEdit* m_passwordInput;
    QPushButton* m_loginBtn;
    QLabel* m_statusLabel;

    QListWidget* m_chatListWidget;
    QListWidget* m_contactListWidget;
    QTabWidget* m_sidebarTabs;
    QWidget* m_settingsTab;
    QWidget* m_chatAreaWidget;
    QLabel* m_chatTitle;
    QListWidget* m_chatList;
    QLineEdit* m_messageInput;
    QPushButton* m_sendBtn;

    QWidget* m_editBar;
    QLabel* m_editLabel;
    QPushButton* m_cancelEditBtn;

    QWidget* m_replyBar;
    QLabel* m_replyLabel;
    QPushButton* m_cancelReplyBtn;

    QMap<QString, User> m_users;
    QMap<QString, Chat> m_chats;
    QString m_currentChatId;
    bool m_isDarkMode;

    QMap<QString, Message> m_pendingMessages;

    QString m_editingMessageId;
    QString m_editingOriginalText;

    QString m_replyingToMessageId;
    QString m_replyingToText;
    QString m_replyingToSender;

    bool m_isLoadingHistory = false;

    QMap<QString, QPixmap> m_avatarCache;
    QSet<QString> m_pendingDownloads;
    
    QString m_highlightedMessageId;
};

#endif // MAINWINDOW_H
