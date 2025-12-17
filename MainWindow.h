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

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    // Helper to get clean reply preview text (public for MessageDelegate)
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

    // Context menu slots
    void onChatListContextMenu(const QPoint& pos);
    void onEditMessage();
    void onDeleteMessage();
    void onReplyToMessage();
    void onForwardMessage();
    void onCancelEdit();
    void onCancelReply();

    void onMessageUpdated(const QString& chatId, const QString& messageId, const QString& newContent, qint64 editedAt);
    void onMessageDeleted(const QString& chatId, const QString& messageId);

    // Scroll detection
    void onScrollValueChanged(int value);

    // NEW: Click on replied message => focus original
    void onChatMessageClicked(QListWidgetItem* item);

private:
    void setupUi();
    void applyTheme();

    void renderMessages(const QString& chatId);
    void addMessageBubble(const Message& msg, bool appendStretch, bool animate);
    void prependMessageBubble(const Message& msg);

    QString resolveChatName(const Chat& chat);
    QColor getColorForName(const QString& name);

    // Avatar handling
    QIcon getAvatar(const QString& name, const QString& url);
    QIcon generateGenericAvatar(const QString& name);
    void updateAvatarOnItems(const QString& url, const QPixmap& pixmap);

    void scrollToBottom();
    void smoothScrollToBottom();
    bool isScrolledToBottom() const;

    void updateMessageStatus(const QString& messageId, MessageStatus newStatus);
    MessageStatus calculateMessageStatus(const Message& msg, const Chat& chat);

    // NEW: Focus + highlight helpers
    void focusMessageById(const QString& messageId);
    void clearAllMessageHighlights();

private:
    WebSocketClient* m_client = nullptr;
    QNetworkAccessManager* m_nam = nullptr;

    // UI Elements
    QStackedWidget* m_stackedWidget = nullptr;
    QWidget* m_loginPage = nullptr;
    QWidget* m_appPage = nullptr;

    QLineEdit* m_usernameInput = nullptr;
    QLineEdit* m_passwordInput = nullptr;
    QPushButton* m_loginBtn = nullptr;
    QLabel* m_statusLabel = nullptr;

    QListWidget* m_chatListWidget = nullptr;
    QListWidget* m_contactListWidget = nullptr;
    QTabWidget* m_sidebarTabs = nullptr;
    QWidget* m_settingsTab = nullptr;

    QWidget* m_chatAreaWidget = nullptr;
    QLabel* m_chatTitle = nullptr;
    QListWidget* m_chatList = nullptr;

    QLineEdit* m_messageInput = nullptr;
    QPushButton* m_sendBtn = nullptr;

    // Edit mode UI
    QWidget* m_editBar = nullptr;
    QLabel* m_editLabel = nullptr;
    QPushButton* m_cancelEditBtn = nullptr;

    // Reply mode UI
    QWidget* m_replyBar = nullptr;
    QLabel* m_replyLabel = nullptr;
    QPushButton* m_cancelReplyBtn = nullptr;

    // Data
    QMap<QString, User> m_users;
    QMap<QString, Chat> m_chats;
    QString m_currentChatId;
    bool m_isDarkMode = false;

    // Pending message tracking for optimistic sends
    QMap<QString, Message> m_pendingMessages;

    // Edit mode state
    QString m_editingMessageId;
    QString m_editingOriginalText;

    // Reply mode state
    QString m_replyingToMessageId;
    QString m_replyingToText;
    QString m_replyingToSender;

    bool m_isLoadingHistory = false;

    // Avatar Cache
    QMap<QString, QPixmap> m_avatarCache;
    QSet<QString> m_pendingDownloads;
};

#endif // MAINWINDOW_H
