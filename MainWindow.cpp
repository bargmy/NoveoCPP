#include "MainWindow.h"
#include "UserListDelegate.h"
#include "WebSocketClient.h"

// --- FIX: ADD ALL MISSING INCLUDES ---
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QSettings>
#include <QDateTime>
#include <QScrollBar>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenu>
#include <QClipboard>
#include <QGuiApplication>
#include <QStyle>
#include <QRegularExpression>
#include <QPainter>
#include <QAbstractItemView>
#include <QLineEdit>        
#include <QLabel>           
#include <QPushButton>      
#include <QStackedWidget>   
#include <QCheckBox>        
#include <QTextDocument>    
#include <QListWidget>      
#include <QTabWidget>       

// ==========================================
// MESSAGE DELEGATE
// ==========================================
class MessageDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void getBubbleLayout(const QString &text, const QString &sender, bool isMe, int viewWidth,
                         QRect &bubbleRect, QRect &textRect, QRect &nameRect, QRect &avatarRect, int &neededHeight) const {
        int maxBubbleWidth = viewWidth * 0.75;
        int nameHeight = isMe ? 0 : 18;
        int timeHeight = 12;
        int bubblePadding = 16;
        int avatarSize = 38;
        int avatarGap = 10;
        int sideMargin = 10;

        static QRegularExpression regex("^(.*)\\s\\[#([a-fA-F0-9]{6}),\\s*\"(.*)\"\\]$");
        QRegularExpressionMatch match = regex.match(sender);
        QString displayName = sender;
        QString tagText;
        bool hasTag = false;

        if (match.hasMatch()) {
            displayName = match.captured(1).trimmed();
            tagText = match.captured(3);
            hasTag = true;
        }

        int totalNameWidth = 0;
        if (!isMe) {
            QFont nameFont("Segoe UI", 11);
            nameFont.setBold(true);
            QFontMetrics fm(nameFont);
            int nameTextW = fm.horizontalAdvance(displayName);
            
            int tagW = 0;
            if (hasTag) {
                QFont tagFont("Segoe UI", 9);
                tagFont.setBold(true);
                QFontMetrics tagFm(tagFont);
                tagW = tagFm.horizontalAdvance(tagText) + 12; 
            }
            totalNameWidth = nameTextW + (hasTag ? (tagW + 8) : 0);
        }

        QTextDocument doc;
        doc.setDefaultFont(QFont("Segoe UI", 10));
        doc.setPlainText(text);
        doc.setTextWidth(maxBubbleWidth - 20);
        int textWidth = doc.idealWidth();
        int textHeight = doc.size().height();

        int bubbleW = qMax(textWidth + 20, qMax(totalNameWidth + 30, 60));
        int bubbleH = textHeight + nameHeight + timeHeight + bubblePadding;
        neededHeight = bubbleH + 10;

        int x;
        if (isMe) {
            x = viewWidth - bubbleW - sideMargin;
            avatarRect = QRect();
        } else {
            x = sideMargin + avatarSize + avatarGap;
            avatarRect = QRect(sideMargin, 5 + bubbleH - avatarSize, avatarSize, avatarSize);
        }

        bubbleRect = QRect(x, 5, bubbleW, bubbleH);
        
        if (!isMe) {
            nameRect = QRect(x + 10, 5 + 5, bubbleW - 20, 15);
            textRect = QRect(x + 10, 5 + 23, bubbleW - 20, textHeight);
        } else {
            textRect = QRect(x + 10, 5 + 8, bubbleW - 20, textHeight);
        }
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);

        QString text = index.data(Qt::UserRole + 1).toString();
        QString sender = index.data(Qt::UserRole + 2).toString();
        qint64 timestamp = index.data(Qt::UserRole + 3).toLongLong();
        bool isMe = index.data(Qt::UserRole + 4).toBool();
        QIcon avatar = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
        bool isDarkMode = option.widget->property("darkMode").toBool();

        QRect bubbleRect, textRect, nameRect, avatarRect;
        int neededHeight;
        getBubbleLayout(text, sender, isMe, option.rect.width(), bubbleRect, textRect, nameRect, avatarRect, neededHeight);

        bubbleRect.translate(0, option.rect.top());
        textRect.translate(0, option.rect.top());
        nameRect.translate(0, option.rect.top());
        avatarRect.translate(0, option.rect.top());

        if (!isMe && !avatar.isNull()) {
            avatar.paint(painter, avatarRect);
        }

        QColor bubbleColor;
        if (isDarkMode) {
            bubbleColor = isMe ? QColor("#2b5278") : QColor("#2b2b2b");
        } else {
            bubbleColor = isMe ? QColor("#EEFFDE") : Qt::white;
        }

        painter->setBrush(bubbleColor);
        if (!isDarkMode && !isMe) painter->setPen(QColor("#d0d0d0"));
        else painter->setPen(Qt::NoPen);
        
        painter->drawRoundedRect(bubbleRect, 12, 12);

        if (!isMe) {
             static QRegularExpression regex("^(.*)\\s\\[#([a-fA-F0-9]{6}),\\s*\"(.*)\"\\]$");
             QRegularExpressionMatch match = regex.match(sender);
             QString name = sender;
             QString tagText;
             QColor tagColor;
             bool hasTag = false;

             if (match.hasMatch()) {
                 name = match.captured(1).trimmed();
                 tagColor = QColor("#" + match.captured(2));
                 tagText = match.captured(3);
                 hasTag = true;
             }

             painter->setPen(QColor("#E35967"));
             QFont nameFont = option.font;
             nameFont.setPixelSize(11);
             nameFont.setBold(true);
             painter->setFont(nameFont);
             painter->drawText(nameRect.left(), nameRect.top() + 10, name);
             
             if(hasTag) {
                 QFontMetrics fm(nameFont);
                 int nw = fm.horizontalAdvance(name);
                 int tx = nameRect.left() + nw + 6;
                 
                 painter->setBrush(tagColor);
                 painter->setPen(Qt::NoPen);
                 painter->drawRoundedRect(tx, nameRect.top(), fm.horizontalAdvance(tagText)+10, 14, 3, 3);
                 
                 painter->setPen(Qt::white);
                 QFont tf = nameFont; tf.setPixelSize(9); painter->setFont(tf);
                 painter->drawText(tx + 5, nameRect.top() + 10, tagText);
             }
        }

        painter->setPen(isDarkMode ? Qt::white : Qt::black);
        QTextDocument doc;
        QFont textFont("Segoe UI", 10);
        doc.setDefaultFont(textFont);
        doc.setPlainText(text);
        doc.setTextWidth(textRect.width());
        
        painter->translate(textRect.topLeft());
        doc.drawContents(painter);
        painter->translate(-textRect.topLeft());

        QDateTime dt; dt.setSecsSinceEpoch(timestamp);
        painter->setPen(QColor("#888888"));
        QFont timeFont = option.font; timeFont.setPixelSize(9); painter->setFont(timeFont);
        painter->drawText(bubbleRect.adjusted(0,0,-8,-5), Qt::AlignBottom | Qt::AlignRight, dt.toString("hh:mm AP"));

        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        QString text = index.data(Qt::UserRole + 1).toString();
        QString sender = index.data(Qt::UserRole + 2).toString();
        bool isMe = index.data(Qt::UserRole + 4).toBool();
        QRect b, t, n, a; int h;
        getBubbleLayout(text, sender, isMe, option.rect.width(), b, t, n, a, h);
        return QSize(option.rect.width(), h);
    }
};

// ==========================================
// MAIN WINDOW IMPLEMENTATION
// ==========================================

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_client(new WebSocketClient(this)), m_nam(new QNetworkAccessManager(this))
{
    QSettings settings("Noveo", "MessengerClient");
    m_isDarkMode = settings.value("darkMode", false).toBool();

    m_typingTimer = new QTimer(this);
    m_typingTimer->setInterval(2000);
    m_typingTimer->setSingleShot(true);

    m_typingClearTimer = new QTimer(this);
    m_typingClearTimer->setInterval(3000);
    m_typingClearTimer->setSingleShot(true);
    connect(m_typingClearTimer, &QTimer::timeout, this, [this](){
        if(m_typingLabel) m_typingLabel->clear();
    });

    setupUi();
    applyTheme();

    if(settings.contains("username")) {
        m_usernameInput->setText(settings.value("username").toString());
        m_passwordInput->setText(settings.value("password").toString());
    }

    connect(m_client, &WebSocketClient::connected, this, &MainWindow::onConnected);
    connect(m_client, &WebSocketClient::loginSuccess, this, &MainWindow::onLoginSuccess);
    connect(m_client, &WebSocketClient::authFailed, this, &MainWindow::onAuthFailed);
    connect(m_client, &WebSocketClient::chatHistoryReceived, this, &MainWindow::onChatHistoryReceived);
    connect(m_client, &WebSocketClient::messageReceived, this, &MainWindow::onMessageReceived);
    connect(m_client, &WebSocketClient::userListUpdated, this, &MainWindow::onUserListUpdated);
    connect(m_client, &WebSocketClient::newChatCreated, this, &MainWindow::onNewChatCreated);
    connect(m_client, &WebSocketClient::userTyping, this, &MainWindow::onUserTyping);
    connect(m_client, &WebSocketClient::userPresenceChanged, this, &MainWindow::onUserPresenceChanged);
    connect(m_client, &WebSocketClient::messageDeleted, this, &MainWindow::onMessageDeleted);
    connect(m_nam, &QNetworkAccessManager::finished, this, &MainWindow::onAvatarDownloaded);

    m_statusLabel->setText("Connecting to server...");
    m_client->connectToServer();
}

MainWindow::~MainWindow() {}

void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
    if(m_chatList) m_chatList->doItemsLayout(); 
}

void MainWindow::setupUi() {
    resize(1000, 700);
    setWindowTitle("Noveo Desktop");

    m_stackedWidget = new QStackedWidget(this);
    setCentralWidget(m_stackedWidget);

    // --- Login Page ---
    m_loginPage = new QWidget();
    QVBoxLayout *loginLayout = new QVBoxLayout(m_loginPage);
    
    QLabel *title = new QLabel("Noveo");
    title->setStyleSheet("font-size: 32px; font-weight: bold; margin-bottom: 20px;");
    title->setAlignment(Qt::AlignCenter);
    
    m_usernameInput = new QLineEdit(); 
    m_usernameInput->setPlaceholderText("Username");
    
    m_passwordInput = new QLineEdit(); 
    m_passwordInput->setPlaceholderText("Password");
    m_passwordInput->setEchoMode(QLineEdit::Password);
    
    m_loginBtn = new QPushButton("Log In"); 
    m_loginBtn->setCursor(Qt::PointingHandCursor);
    m_loginBtn->setMinimumHeight(40);
    
    m_statusLabel = new QLabel("Waiting for connection..."); 
    m_statusLabel->setAlignment(Qt::AlignCenter);
    
    loginLayout->addStretch(); 
    loginLayout->addWidget(title); 
    loginLayout->addWidget(m_usernameInput);
    loginLayout->addWidget(m_passwordInput); 
    loginLayout->addWidget(m_loginBtn);
    loginLayout->addWidget(m_statusLabel); 
    loginLayout->addStretch();
    loginLayout->setContentsMargins(300, 50, 300, 50);
    
    m_stackedWidget->addWidget(m_loginPage);

    // --- App Page ---
    m_appPage = new QWidget();
    QHBoxLayout *appLayout = new QHBoxLayout(m_appPage);
    appLayout->setContentsMargins(0,0,0,0); 
    appLayout->setSpacing(0);

    // Sidebar
    m_sidebarTabs = new QTabWidget(); 
    m_sidebarTabs->setFixedWidth(300);
    m_sidebarTabs->setTabPosition(QTabWidget::South);
    
    m_chatListWidget = new QListWidget();
    m_chatListWidget->setIconSize(QSize(42, 42));
    m_chatListWidget->setItemDelegate(new UserListDelegate(this));
    m_chatListWidget->setFrameShape(QFrame::NoFrame);
    m_chatListWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_contactListWidget = new QListWidget();
    m_contactListWidget->setIconSize(QSize(42, 42));
    m_contactListWidget->setItemDelegate(new UserListDelegate(this));
    m_contactListWidget->setFrameShape(QFrame::NoFrame);
    m_contactListWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // Settings Tab
    m_settingsTab = new QWidget();
    QVBoxLayout *settingsLayout = new QVBoxLayout(m_settingsTab);
    QCheckBox *darkModeCheck = new QCheckBox("Dark Mode");
    darkModeCheck->setChecked(m_isDarkMode);
    connect(darkModeCheck, &QCheckBox::toggled, this, &MainWindow::onDarkModeToggled);
    QPushButton *logoutBtn = new QPushButton("Log Out");
    connect(logoutBtn, &QPushButton::clicked, this, &MainWindow::onLogoutClicked);
    
    settingsLayout->addWidget(new QLabel("Settings"));
    settingsLayout->addWidget(darkModeCheck);
    settingsLayout->addStretch(); 
    settingsLayout->addWidget(logoutBtn);
    settingsLayout->addSpacing(20);

    m_sidebarTabs->addTab(m_chatListWidget, "Chats");
    m_sidebarTabs->addTab(m_contactListWidget, "Contacts");
    m_sidebarTabs->addTab(m_settingsTab, "Settings");

    // Chat Area
    m_chatAreaWidget = new QWidget();
    QVBoxLayout *chatAreaLayout = new QVBoxLayout(m_chatAreaWidget);
    chatAreaLayout->setContentsMargins(0,0,0,0); 
    chatAreaLayout->setSpacing(0);

    QWidget *header = new QWidget(); 
    header->setFixedHeight(60); 
    header->setObjectName("chatHeader");
    QVBoxLayout *headerVLayout = new QVBoxLayout(header);
    headerVLayout->setContentsMargins(20, 10, 20, 5);
    headerVLayout->setSpacing(2);
    
    m_chatTitle = new QLabel("Select a chat");
    m_chatTitle->setStyleSheet("font-size: 16px; font-weight: bold;");
    
    m_typingLabel = new QLabel(""); 
    m_typingLabel->setStyleSheet("font-size: 11px; color: #888;");
    
    headerVLayout->addWidget(m_chatTitle);
    headerVLayout->addWidget(m_typingLabel);

    m_chatList = new QListWidget();
    m_chatList->setObjectName("chatList");
    m_chatList->setFrameShape(QFrame::NoFrame);
    m_chatList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_chatList->setSelectionMode(QAbstractItemView::SingleSelection); 
    m_chatList->setUniformItemSizes(false); 
    m_chatList->setResizeMode(QListView::Adjust);
    m_chatList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); 
    m_chatList->setItemDelegate(new MessageDelegate(this));
    
    m_chatList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_chatList, &QListWidget::customContextMenuRequested, this, &MainWindow::onMessageContextMenu);

    m_replyContainer = new QWidget();
    m_replyContainer->setFixedHeight(40);
    m_replyContainer->setStyleSheet("background: #f0f0f0; border-top: 1px solid #ccc; border-left: 4px solid #E35967; padding-left: 10px;");
    m_replyContainer->hide();
    
    QHBoxLayout *replyLayout = new QHBoxLayout(m_replyContainer);
    replyLayout->setContentsMargins(10, 0, 10, 0);
    
    m_replyLabel = new QLabel("Replying...");
    m_cancelReplyBtn = new QPushButton("×"); 
    m_cancelReplyBtn->setFixedWidth(30);
    m_cancelReplyBtn->setFlat(true);
    m_cancelReplyBtn->setCursor(Qt::PointingHandCursor);
    m_cancelReplyBtn->setStyleSheet("font-size: 16px; font-weight: bold;");
    
    connect(m_cancelReplyBtn, &QPushButton::clicked, this, [this](){ 
        m_replyToId.clear(); 
        m_replyContainer->hide(); 
    });
    
    replyLayout->addWidget(m_replyLabel);
    replyLayout->addStretch();
    replyLayout->addWidget(m_cancelReplyBtn);

    QWidget *inputArea = new QWidget(); 
    inputArea->setFixedHeight(60);
    inputArea->setObjectName("inputArea");
    
    QHBoxLayout *inputLayout = new QHBoxLayout(inputArea);
    inputLayout->setContentsMargins(20, 10, 20, 10);
    
    m_messageInput = new QLineEdit(); 
    m_messageInput->setPlaceholderText("Write a message...");
    
    m_sendBtn = new QPushButton("Send");
    m_sendBtn->setCursor(Qt::PointingHandCursor);
    
    inputLayout->addWidget(m_messageInput); 
    inputLayout->addWidget(m_sendBtn);

    chatAreaLayout->addWidget(header);
    chatAreaLayout->addWidget(m_chatList);
    chatAreaLayout->addWidget(m_replyContainer);
    chatAreaLayout->addWidget(inputArea);

    appLayout->addWidget(m_sidebarTabs);
    appLayout->addWidget(m_chatAreaWidget);
    m_stackedWidget->addWidget(m_appPage);

    connect(m_loginBtn, &QPushButton::clicked, this, &MainWindow::onLoginBtnClicked);
    connect(m_chatListWidget, &QListWidget::itemClicked, this, &MainWindow::onChatSelected);
    connect(m_contactListWidget, &QListWidget::itemClicked, this, &MainWindow::onContactSelected);
    connect(m_sendBtn, &QPushButton::clicked, this, &MainWindow::onSendBtnClicked);
    connect(m_messageInput, &QLineEdit::returnPressed, this, &MainWindow::onSendBtnClicked);
    connect(m_messageInput, &QLineEdit::textChanged, this, &MainWindow::onInputTextChanged);
}

void MainWindow::applyTheme() {
    QString scrollStyle = R"(
        QScrollBar:vertical { background: %1; width: 10px; margin: 0; }
        QScrollBar:handle:vertical { background: %2; min-height: 20px; border-radius: 5px; margin: 2px; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }
    )";
    
    QString bg = m_isDarkMode ? "#1e1e1e" : "#f5f5f5";
    QString fg = m_isDarkMode ? "#ffffff" : "#000000";
    QString handle = m_isDarkMode ? "#555555" : "#cccccc";
    QString listBg = m_isDarkMode ? "#121212" : "#ffffff";
    QString inputBg = m_isDarkMode ? "#333333" : "#ffffff";
    QString headerBg = m_isDarkMode ? "#252525" : "#eeeeee";
    QString replyBg = m_isDarkMode ? "#2d2d2d" : "#f0f0f0";
    QString replyBorder = m_isDarkMode ? "#444444" : "#cccccc";
    
    QString style = QString("QWidget { background-color: %1; color: %2; font-family: 'Segoe UI'; }").arg(bg, fg);
    
    style += QString("QListWidget { background-color: %1; border: none; outline: none; }").arg(listBg);
    style += QString("QListWidget::item:selected { background-color: %1; }").arg(m_isDarkMode ? "#2c3e50" : "#d0e1f5");
    style += QString("QListWidget::item:hover { background-color: %1; }").arg(m_isDarkMode ? "#1f2a36" : "#e5f1fb");

    style += QString("QLineEdit { padding: 8px; border-radius: 6px; background: %1; color: %2; border: 1px solid #555; }")
                 .arg(inputBg, fg);
    
    style += QString("QWidget#chatHeader { background-color: %1; border-bottom: 1px solid #444; }").arg(headerBg);
    style += QString("QWidget#inputArea { background-color: %1; border-top: 1px solid #444; }").arg(headerBg);
    
    m_replyContainer->setStyleSheet(QString("background: %1; border-top: 1px solid %2; border-left: 4px solid #E35967; padding-left: 10px;")
                                    .arg(replyBg, replyBorder));
    
    style += scrollStyle.arg(bg, handle);

    m_appPage->setStyleSheet(style);
    
    m_chatList->setProperty("darkMode", m_isDarkMode);
    m_chatList->viewport()->update(); 
}

void MainWindow::onLoginBtnClicked() {
    m_statusLabel->setText("Logging in...");
    m_client->login(m_usernameInput->text(), m_passwordInput->text());
}

void MainWindow::onConnected() { 
    m_statusLabel->setText("Connected. Please log in."); 
}

void MainWindow::onAuthFailed(const QString &msg) { 
    m_statusLabel->setText("Error: " + msg); 
    QMessageBox::warning(this, "Login Failed", msg);
}

void MainWindow::onLoginSuccess(const User &user, const QString &token) {
    Q_UNUSED(user);
    Q_UNUSED(token);
    QSettings settings("Noveo", "MessengerClient");
    settings.setValue("username", m_usernameInput->text());
    settings.setValue("password", m_passwordInput->text());
    settings.setValue("darkMode", m_isDarkMode);
    
    m_stackedWidget->setCurrentWidget(m_appPage);
}

void MainWindow::onChatHistoryReceived(const std::vector<Chat> &chats) {
    m_chats.clear();
    m_chatListWidget->clear();
    for (const auto &chat : chats) {
        m_chats[chat.chatId] = chat;
        
        QListWidgetItem *item = new QListWidgetItem(resolveChatName(chat));
        item->setData(Qt::UserRole, chat.chatId);
        item->setIcon(getAvatar(chat.chatId)); 
        
        if (chat.avatarUrl.startsWith("http")) {
            loadAvatar(chat.avatarUrl, chat.chatId, true);
        }
        
        m_chatListWidget->addItem(item);
    }
}

void MainWindow::onUserListUpdated(const std::vector<User> &users) {
    m_users.clear();
    m_contactListWidget->clear();
    for (const auto &user : users) {
        if (user.userId == m_client->currentUserId()) continue;
        
        m_users[user.userId] = user;
        
        QListWidgetItem *item = new QListWidgetItem(user.username);
        item->setData(Qt::UserRole, user.userId);
        item->setData(Qt::UserRole + 1, user.online);
        item->setIcon(getAvatar(user.userId));
        
        if (user.avatarUrl.startsWith("http")) {
            loadAvatar(user.avatarUrl, user.userId, false);
        }

        m_contactListWidget->addItem(item);
    }
}

void MainWindow::onMessageReceived(const Message &msg) {
    if (m_chats.contains(msg.chatId)) {
        m_chats[msg.chatId].messages.push_back(msg);
        
        bool wasSignalsBlocked = m_chatListWidget->signalsBlocked();
        m_chatListWidget->blockSignals(true);
        
        for(int i=0; i<m_chatListWidget->count(); i++) {
            QListWidgetItem *it = m_chatListWidget->item(i);
            if(it->data(Qt::UserRole).toString() == msg.chatId) {
                m_chatListWidget->takeItem(i);
                m_chatListWidget->insertItem(0, it);
                
                if (m_currentChatId == msg.chatId) {
                    m_chatListWidget->setCurrentItem(it);
                }
                break;
            }
        }
        m_chatListWidget->blockSignals(wasSignalsBlocked);
    }

    if (msg.chatId == m_currentChatId) {
        addMessageBubble(msg);
        smoothScrollToBottom();
    }
}

void MainWindow::onChatSelected(QListWidgetItem *item) {
    if(!item) return;
    QString chatId = item->data(Qt::UserRole).toString();
    
    if(m_currentChatId == chatId && m_chatList->count() > 0) return;
    
    m_currentChatId = chatId;
    m_chatTitle->setText(resolveChatName(m_chats[chatId]));
    
    m_typingLabel->clear();
    m_replyToId.clear();
    m_replyContainer->hide();
    
    renderMessages(chatId);
}

void MainWindow::onContactSelected(QListWidgetItem *item) {
    QString userId = item->data(Qt::UserRole).toString();
    QMessageBox::information(this, "Contact", "Selected contact: " + m_users[userId].username);
}

void MainWindow::renderMessages(const QString &chatId) {
    m_chatList->clear();
    if (!m_chats.contains(chatId)) return;
    
    const auto &msgs = m_chats[chatId].messages;
    for (const auto &msg : msgs) {
        addMessageBubble(msg);
    }
    m_chatList->scrollToBottom();
}

void MainWindow::addMessageBubble(const Message &msg) {
    QListWidgetItem *item = new QListWidgetItem(m_chatList);
    item->setData(Qt::UserRole, msg.messageId); 
    item->setData(Qt::UserRole + 1, msg.text);
    item->setData(Qt::UserRole + 2, msg.senderId); 
    item->setData(Qt::UserRole + 3, msg.timestamp);
    
    bool isMe = (msg.senderId == m_client->currentUserId());
    item->setData(Qt::UserRole + 4, isMe);
    
    if (!isMe) {
        item->setIcon(getAvatar(msg.senderId));
    }
    
    m_chatList->addItem(item);
}

void MainWindow::loadAvatar(const QString &url, const QString &id, bool isChat) {
    if (m_avatarCache.contains(id)) return; 
    
    // --- FIX: USE UNIFORM INITIALIZATION TO AVOID AMBIGUITY ---
    QNetworkRequest req{QUrl(url)};
    QNetworkReply *reply = m_nam->get(req);
    reply->setProperty("targetId", id);
    reply->setProperty("isChat", isChat);
}

void MainWindow::onAvatarDownloaded(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        QPixmap pixmap;
        if (pixmap.loadFromData(data)) {
            QIcon icon(pixmap);
            QString id = reply->property("targetId").toString();
            
            m_avatarCache[id] = icon;

            auto updateList = [&](QListWidget *list) {
                for(int i=0; i<list->count(); i++) {
                    if (list->item(i)->data(Qt::UserRole).toString() == id) {
                        list->item(i)->setIcon(icon);
                    }
                }
            };
            
            updateList(m_chatListWidget);
            updateList(m_contactListWidget);
            
            if (m_currentChatId == id || (m_chats.contains(m_currentChatId) && m_chats[m_currentChatId].members.contains(id))) {
                 m_chatList->viewport()->update();
            }
        }
    }
    reply->deleteLater();
}

QIcon MainWindow::getAvatar(const QString &id) {
    if (m_avatarCache.contains(id)) return m_avatarCache[id];
    return QIcon(); 
}

void MainWindow::onMessageContextMenu(const QPoint &pos) {
    QListWidgetItem *item = m_chatList->itemAt(pos);
    if (!item) return;

    QString msgId = item->data(Qt::UserRole).toString();
    QString text = item->data(Qt::UserRole + 1).toString();
    bool isMe = item->data(Qt::UserRole + 4).toBool();

    QMenu menu(this);
    QAction *replyAct = menu.addAction("Reply");
    QAction *copyAct = menu.addAction("Copy Text");
    QAction *deleteAct = nullptr;
    
    if (isMe) {
        menu.addSeparator();
        deleteAct = menu.addAction("Delete Message");
    }

    QAction *selected = menu.exec(m_chatList->mapToGlobal(pos));
    
    if (selected == replyAct) {
        m_replyToId = msgId;
        m_replyLabel->setText("Replying to: " + text.left(20) + (text.length()>20 ? "..." : ""));
        m_replyContainer->show();
        m_messageInput->setFocus();
    } else if (selected == copyAct) {
        QGuiApplication::clipboard()->setText(text);
    } else if (selected == deleteAct) {
        m_client->deleteMessage(m_currentChatId, msgId);
        delete m_chatList->takeItem(m_chatList->row(item));
    }
}

void MainWindow::onInputTextChanged(const QString &text) {
    Q_UNUSED(text);
    if (m_currentChatId.isEmpty()) return;
    
    if (!m_typingTimer->isActive()) {
        m_client->sendTyping(m_currentChatId);
        m_typingTimer->start();
    }
}

void MainWindow::onUserTyping(const QString &chatId, const QString &username) {
    if (chatId == m_currentChatId) {
        m_typingLabel->setText(username + " is typing...");
        m_typingClearTimer->start(); 
    }
}

void MainWindow::onUserPresenceChanged(const QString &userId, bool online) {
    if (m_users.contains(userId)) m_users[userId].online = online;
    
    for(int i=0; i<m_contactListWidget->count(); i++) {
        QListWidgetItem *it = m_contactListWidget->item(i);
        if (it->data(Qt::UserRole).toString() == userId) {
            it->setData(Qt::UserRole + 1, online);
            m_contactListWidget->update(m_contactListWidget->visualItemRect(it));
        }
    }
}

void MainWindow::onSendBtnClicked() {
    QString text = m_messageInput->text().trimmed();
    if (text.isEmpty() || m_currentChatId.isEmpty()) return;
    
    m_client->sendMessage(m_currentChatId, text, m_replyToId);
    
    m_messageInput->clear();
    m_replyToId.clear();
    m_replyContainer->hide();
}

void MainWindow::onMessageDeleted(const QString &chatId, const QString &messageId) {
    if (chatId == m_currentChatId) {
        for(int i=0; i<m_chatList->count(); i++) {
            if (m_chatList->item(i)->data(Qt::UserRole).toString() == messageId) {
                delete m_chatList->takeItem(i);
                break;
            }
        }
    }
}

void MainWindow::onNewChatCreated(const Chat &c) {
    onChatHistoryReceived({c});
}

void MainWindow::onDarkModeToggled(bool checked) {
    m_isDarkMode = checked;
    applyTheme();
}

void MainWindow::onLogoutClicked() {
    m_usernameInput->clear();
    m_passwordInput->clear();
    m_chats.clear();
    m_users.clear();
    m_avatarCache.clear();
    
    m_stackedWidget->setCurrentWidget(m_loginPage);
}

void MainWindow::smoothScrollToBottom() {
    m_chatList->scrollToBottom();
}

QString MainWindow::resolveChatName(const Chat &c) {
    if (!c.chatName.isEmpty()) return c.chatName;
    return "Chat"; 
}
