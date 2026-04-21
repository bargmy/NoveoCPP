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
#include <QTabWidget>
#include <QCheckBox>
#include <QMap>
#include <QSet>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QCloseEvent>
#include <QStringList>

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

struct ContactEntry {
    QString userId;
    QString username;
    QString displayName;
    QString avatarUrl;
    bool isMutual = false;
};

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    QString getReplyPreviewText(const QString& replyToId, const QString& chatId);
    void focusOnMessage(const QString& messageId);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

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
    void onNotificationsToggled(bool checked);

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

    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void onTrayShowHide();
    void onTrayQuit();

private:
    void setupUi();
    void applyTheme();

    void renderMessages(const QString& chatId);
    void addMessageBubble(const Message& msg, bool appendStretch, bool animate);
    void prependMessageBubble(const Message& msg);

    QString resolveChatName(const Chat& chat);
    QString displayNameForUserId(const QString& userId) const;
    QString normalizeAvatarUrl(const QString& url) const;
    QColor getColorForName(const QString& name);

    QIcon getAvatar(const QString& name, const QString& url);
    QIcon generateGenericAvatar(const QString& name);
    void updateAvatarOnItems(const QString& url, const QPixmap& pixmap);

    void scrollToBottom();
    void smoothScrollToBottom();
    bool isScrolledToBottom() const;

    void updateMessageStatus(const QString& messageId, MessageStatus newStatus);
    MessageStatus calculateMessageStatus(const Message& msg, const Chat& chat);

    void setupTrayIcon();
    void showNotificationForMessage(const Message& msg);

    void fetchContacts();
    void fetchProfileSummary();
    void populateContactList();
    void updateSidebarProfile();
    void refreshProfileTab();
    void refreshStarsTab();

private:
    WebSocketClient* m_client = nullptr;
    QNetworkAccessManager* m_nam = nullptr;

    QStackedWidget* m_stackedWidget = nullptr;
    QWidget* m_loginPage = nullptr;
    QWidget* m_appPage = nullptr;

    QLineEdit* m_usernameInput = nullptr;
    QLineEdit* m_passwordInput = nullptr;
    QPushButton* m_loginBtn = nullptr;
    QLabel* m_statusLabel = nullptr;

    QWidget* m_sidebarShell = nullptr;
    QWidget* m_sidebarHeader = nullptr;
    QLabel* m_sidebarDisplayNameLabel = nullptr;
    QLabel* m_sidebarHandleLabel = nullptr;

    QListWidget* m_chatListWidget = nullptr;
    QListWidget* m_contactListWidget = nullptr;
    QTabWidget* m_sidebarTabs = nullptr;
    QWidget* m_settingsTab = nullptr;
    QWidget* m_starsTab = nullptr;
    QWidget* m_profileTab = nullptr;

    QLabel* m_profileBioLabel = nullptr;
    QLabel* m_profileStatsLabel = nullptr;
    QListWidget* m_profileGiftsList = nullptr;
    QListWidget* m_profileGroupsList = nullptr;

    QLabel* m_starsBalanceLabel = nullptr;
    QLabel* m_starsHintLabel = nullptr;

    QWidget* m_chatAreaWidget = nullptr;
    QLabel* m_chatTitle = nullptr;
    QListWidget* m_chatList = nullptr;

    QLineEdit* m_messageInput = nullptr;
    QPushButton* m_sendBtn = nullptr;

    QWidget* m_editBar = nullptr;
    QLabel* m_editLabel = nullptr;
    QPushButton* m_cancelEditBtn = nullptr;

    QWidget* m_replyBar = nullptr;
    QLabel* m_replyLabel = nullptr;
    QPushButton* m_cancelReplyBtn = nullptr;

    QCheckBox* m_notificationsCheck = nullptr;

    QMap<QString, User> m_users;
    QMap<QString, ContactEntry> m_contacts;
    QMap<QString, Chat> m_chats;
    QString m_currentChatId;

    bool m_isDarkMode = false;
    bool m_notificationsEnabled = true;
    bool m_contactsLoaded = false;

    QString m_authToken;
    User m_currentAccount;
    QString m_profileBio;
    QStringList m_profileGifts;
    QStringList m_profileGroups;
    double m_starsBalance = 0.0;

    QString m_highlightedMessageId;
    QMap<QString, Message> m_pendingMessages;

    QString m_editingMessageId;
    QString m_editingOriginalText;

    QString m_replyingToMessageId;
    QString m_replyingToText;
    QString m_replyingToSender;

    bool m_isLoadingHistory = false;

    QMap<QString, QPixmap> m_avatarCache;
    QSet<QString> m_pendingDownloads;

    QSystemTrayIcon* m_trayIcon = nullptr;
    QMenu* m_trayMenu = nullptr;
    QAction* m_trayShowHideAction = nullptr;
    QAction* m_trayQuitAction = nullptr;
};

#endif // MAINWINDOW_H
