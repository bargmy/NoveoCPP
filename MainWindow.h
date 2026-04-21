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
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QCloseEvent>
#include <QJsonObject>
#include <QPointer>
#include <QTimer>

#include "WebSocketClient.h"
#include "DataStructures.h"
#include "RestClient.h"
#include "SessionStore.h"
#include "UpdaterService.h"
#include "VoiceAudioBridge.h"
#include "ChatSettingsDialog.h"
#include "MediaViewerDialog.h"
#include "MessageItemWidget.h"
#include "SettingsDialog.h"

class QFrame;
class QGridLayout;
class QMediaPlayer;
class QScrollArea;
class QSlider;

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

    QString getReplyPreviewText(const QString& replyToId, const QString& chatId);
    void focusOnMessage(const QString& messageId);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void onConnected();
    void onDisconnected();
    void onLoginBtnClicked();
    void onRegisterBtnClicked();
    void onLoginSuccess(const User& user, const QString& token, qint64 expiresAt);
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
    void onPresenceUpdated(const QString& userId, bool online);
    void onTypingReceived(const QString& chatId, const QString& senderId);
    void onPasswordChanged(const QString& token, qint64 expiresAt, const QString& warning);
    void onUserUpdated(const User& user);

    void onScrollValueChanged(int value);
    void onChatListItemClicked(const QModelIndex& index);

    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void onTrayShowHide();
    void onTrayQuit();

private:
    void setupUi();
    void applyTheme();
    void applyLanguage(const QString& languageCode);
    QString uiText(const QString& key) const;
    void applyLayoutDirectionForLanguage(const QString& languageCode);
    bool shouldForceReloginForAuthError(const QString& message) const;
    void tryReconnectWithSavedSession();
    void updatePinnedMessageBar();
    void updateComposerStateForCurrentChat();
    void openStickerPicker();
    void performChatSettingsAction(const QString& action, const QString& chatId, const QJsonObject& extra = QJsonObject());
    void hideStickerPanel();
    void repositionStickerPanel();
    void updateSettingsOverlayGeometry();
    void updateContactsOverlayGeometry();
    void showSettingsDialog();
    void openChatSettingsDialog(const QString& chatId);
    void showSidebarMenu();
    void hideSidebarMenu();
    void updateSidebarMenuGeometry();
    void showContactsDialog();
    void applyContactsFilter(const QString& filterText);
    void showCreateOptionsMenu();
    void createGroupFromSidebar();
    void createChannelFromSidebar();
    QString resolveRecipientIdForChat(const QString& chatId) const;
    QString resolveFileUrl(const QString& rawUrl) const;
    QString displayTextForMessage(const Message& msg) const;
    void startReplyToMessage(const QString& messageId);
    void startEditMessage(const QString& messageId);
    void deleteMessageById(const QString& messageId);
    void forwardMessageById(const QString& messageId);
    void playAudioTrack(const QString& fileUrl, const QString& trackName, MessageItemWidget* sourceWidget = nullptr);
    void openMediaInViewer(const QString& mediaType, const QString& fileUrl);
    void refreshMessageWidgetSizes();
    void syncMessageWidgetSize(QListWidgetItem* item);
    void rebuildCurrentMessageCaches(const QString& chatId);
    void openAddMembersDialogForChat(const QString& chatId);

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

    void setupTrayIcon();
    void showNotificationForMessage(const Message& msg);

private:
    WebSocketClient* m_client = nullptr;
    QNetworkAccessManager* m_nam = nullptr;
    RestClient* m_restClient = nullptr;
    UpdaterService* m_updaterService = nullptr;
    VoiceAudioBridge* m_voiceAudio = nullptr;

    QStackedWidget* m_stackedWidget = nullptr;
    QWidget* m_loginPage = nullptr;
    QWidget* m_appPage = nullptr;
    QStackedWidget* m_authFormsStack = nullptr;
    QLabel* m_brandTitleLabel = nullptr;
    QLabel* m_loginTitleLabel = nullptr;
    QLabel* m_loginSubtitleLabel = nullptr;
    QLabel* m_registerTitleLabel = nullptr;
    QLabel* m_registerSubtitleLabel = nullptr;
    QLabel* m_contactsTitleLabel = nullptr;
    QLabel* m_menuTitleLabel = nullptr;
    QLabel* m_joinInfoLabel = nullptr;
    QLabel* m_replyTitleLabel = nullptr;
    QLabel* m_stickerPanelTitleLabel = nullptr;
    QLabel* m_stickerLoadingLabel = nullptr;
    QLabel* m_sidebarNowPlayingLabel = nullptr;

    QLineEdit* m_usernameInput = nullptr;
    QLineEdit* m_passwordInput = nullptr;
    QLineEdit* m_registerUsernameInput = nullptr;
    QLineEdit* m_registerPasswordInput = nullptr;
    QPushButton* m_loginBtn = nullptr;
    QPushButton* m_registerBtn = nullptr;
    QPushButton* m_showRegisterSwitchBtn = nullptr;
    QPushButton* m_showLoginSwitchBtn = nullptr;
    QLabel* m_statusLabel = nullptr;

    QListWidget* m_chatListWidget = nullptr;
    QListWidget* m_contactListWidget = nullptr;
    QWidget* m_sidebarContainer = nullptr;
    QPushButton* m_sidebarSettingsBtn = nullptr;
    QPushButton* m_sidebarCreateBtn = nullptr;
    QLabel* m_sidebarTitleLabel = nullptr;
    QWidget* m_settingsOverlay = nullptr;
    QWidget* m_sidebarMenuOverlay = nullptr;
    QFrame* m_sidebarMenuPanel = nullptr;
    QPushButton* m_menuContactsBtn = nullptr;
    QPushButton* m_menuSettingsBtn = nullptr;
    QWidget* m_contactsOverlay = nullptr;
    QFrame* m_contactsPanel = nullptr;
    QLineEdit* m_contactsSearchInput = nullptr;
    QWidget* m_sidebarAudioPlayer = nullptr;
    QPushButton* m_sidebarPlayBtn = nullptr;
    QPushButton* m_sidebarCloseBtn = nullptr;
    QLabel* m_sidebarTrackName = nullptr;
    QLabel* m_sidebarCurrentTime = nullptr;
    QLabel* m_sidebarDuration = nullptr;
    QSlider* m_sidebarSeekBar = nullptr;
    QPushButton* m_sidebarVolumeToggleBtn = nullptr;
    QWidget* m_sidebarVolumeControl = nullptr;
    QSlider* m_sidebarVolumeBar = nullptr;

    QWidget* m_chatAreaWidget = nullptr;
    QLabel* m_chatTitle = nullptr;
    QPushButton* m_chatSettingsBtn = nullptr;
    QPushButton* m_voiceCallBtn = nullptr;
    QListWidget* m_chatList = nullptr;
    QWidget* m_pinnedBar = nullptr;
    QLabel* m_pinnedLabel = nullptr;
    QPushButton* m_openPinnedBtn = nullptr;
    QPushButton* m_unpinPinnedBtn = nullptr;
    QWidget* m_joinChannelBar = nullptr;
    QPushButton* m_joinChannelBtn = nullptr;

    QLineEdit* m_messageInput = nullptr;
    QPushButton* m_attachBtn = nullptr;
    QPushButton* m_stickerBtn = nullptr;
    QPushButton* m_sendBtn = nullptr;
    QWidget* m_inputArea = nullptr;
    QFrame* m_stickerPanel = nullptr;
    QScrollArea* m_stickerScrollArea = nullptr;
    QWidget* m_stickerGridHost = nullptr;
    QGridLayout* m_stickerGridLayout = nullptr;
    bool m_stickerPanelVisible = false;

    QWidget* m_editBar = nullptr;
    QLabel* m_editLabel = nullptr;
    QPushButton* m_cancelEditBtn = nullptr;

    QWidget* m_replyBar = nullptr;
    QLabel* m_replyLabel = nullptr;
    QPushButton* m_cancelReplyBtn = nullptr;

    QString m_updaterStatusText = QStringLiteral("Updater idle.");
    bool m_canDownloadUpdate = false;
    bool m_canInstallUpdate = false;

    QMap<QString, User> m_users;
    QMap<QString, Chat> m_chats;
    QString m_currentChatId;
    QString m_currentVoiceChatId;
    QString m_authToken;
    qint64 m_authExpiresAt = 0;
    QString m_languageCode = QStringLiteral("en");
    QStringList m_stickerCache;
    int m_reconnectAttempts = 0;
    bool m_manualDisconnect = false;
    QTimer* m_reconnectTimer = nullptr;
    bool m_waitingForSessionReconnectResult = false;
    bool m_hasAuthenticatedSession = false;

    bool m_isDarkMode = false;
    bool m_notificationsEnabled = true;
    QString m_highlightedMessageId;

    QMap<QString, Message> m_pendingMessages;
    QMap<QString, QListWidgetItem*> m_messageItemsById;
    QMap<QString, MessageItemWidget*> m_messageWidgetsById;
    QMap<QString, QString> m_currentMessagePreviewById;
    QMap<QString, QString> m_currentMessageSenderById;
    QTimer* m_messageResizeDebounceTimer = nullptr;
    int m_lastMessageViewportWidth = -1;

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

    QMediaPlayer* m_sidebarAudioPlayback = nullptr;
    QString m_currentAudioUrl;
    QPointer<MessageItemWidget> m_currentAudioSourceWidget;
    MediaViewerDialog* m_mediaViewerDialog = nullptr;
    SettingsDialog* m_settingsDialog = nullptr;
    ChatSettingsDialog* m_chatSettingsDialog = nullptr;
};

#endif // MAINWINDOW_H
