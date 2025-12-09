#include "MainWindow.h"
#include "UserListDelegate.h"
#include <QStyleOption>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation>
#include <QScrollBar>
#include <QStandardPaths>
#include <QDir>
#include <QCryptographicHash>
#include <QFile>
#include <QPainterPath>
#include <QTextDocument> // FIXED: Added missing include
#include <QCheckBox>     // FIXED: Added missing include
#include <QApplication>  // FIXED: Added missing include

// Custom role to store the avatar URL in items for updating later
const int AvatarUrlRole = Qt::UserRole + 10;
const QString API_BASE_URL = "https://api.pcpapc172.ir:8443";

// ==========================================
// 2. MESSAGE DELEGATE (Chat Area)
// ==========================================
// Removed Q_OBJECT to avoid moc issues in .cpp file
class MessageDelegate : public QStyledItemDelegate {
    bool m_isDarkMode = false;
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void setTheme(bool isDarkMode) {
        m_isDarkMode = isDarkMode;
    }

    void getBubbleLayout(const QString &text, const QString &sender, bool isMe, int viewWidth,
                         QRect &bubbleRect, QRect &textRect, QRect &nameRect, QRect &avatarRect, int &neededHeight) const {
        
        int maxBubbleWidth = viewWidth * 0.75;
        int nameHeight = isMe ? 0 : 18;
        int timeHeight = 12;
        int bubblePadding = 16;
        int avatarSize = 38; 
        int avatarGap = 10;
        int sideMargin = 10;

        // Parse name/color tags
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

        QTextDocument doc; // This now works because of <QTextDocument> include
        doc.setDefaultFont(QFont("Segoe UI", 10));
        doc.setPlainText(text);
        doc.setTextWidth(maxBubbleWidth - 20);
        
        int textWidth = doc.idealWidth();
        int textHeight = doc.size().height();
        
        int contentWidth = isMe ? (textWidth + 20) : qMax(textWidth + 20, totalNameWidth + 30);
        int bubbleW = qMax(contentWidth, 60);
        int bubbleH = textHeight + nameHeight + timeHeight + bubblePadding;

        neededHeight = bubbleH + 10;
        int x;

        if (isMe) {
            x = viewWidth - bubbleW - sideMargin;
            avatarRect = QRect(); // No avatar for self
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
        // FIXED: Added template argument <QIcon>
        QIcon avatar = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));

        QRect bubbleRect, textRect, nameRect, avatarRect;
        int neededHeight;
        getBubbleLayout(text, sender, isMe, option.rect.width(), bubbleRect, textRect, nameRect, avatarRect, neededHeight);

        bubbleRect.translate(0, option.rect.top());
        textRect.translate(0, option.rect.top());
        nameRect.translate(0, option.rect.top());
        avatarRect.translate(0, option.rect.top());

        // Draw Avatar
        if (!isMe && !avatar.isNull()) {
            avatar.paint(painter, avatarRect);
        }

        QColor bubbleColor;
        QColor borderColor;
        QColor textColor;
        QColor timeColor;

        if (m_isDarkMode) {
            bubbleColor = isMe ? QColor("#2b5278") : QColor("#2d2d2d");
            borderColor = Qt::transparent;
            textColor = Qt::white;
            timeColor = QColor("#a0a0a0");
        } else {
            bubbleColor = isMe ? QColor("#EEFFDE") : Qt::white;
            borderColor = QColor("#d0d0d0");
            textColor = Qt::black;
            timeColor = QColor("#888888");
        }

        painter->setBrush(bubbleColor);
        painter->setPen(borderColor == Qt::transparent ? Qt::NoPen : QPen(borderColor, 1));
        painter->drawRoundedRect(bubbleRect, 12, 12);

        if (!isMe) {
            static QRegularExpression regex("^(.*)\\s\\[#([a-fA-F0-9]{6}),\\s*\"(.*)\"\\]$");
            QRegularExpressionMatch match = regex.match(sender);
            QString displayName = sender;
            QString tagText;
            QColor tagColor;
            bool hasTag = false;

            if (match.hasMatch()) {
                displayName = match.captured(1).trimmed();
                tagColor = QColor("#" + match.captured(2));
                tagText = match.captured(3);
                hasTag = true;
            }

            painter->setPen(QColor("#E35967"));
            QFont nameFont = option.font;
            nameFont.setPixelSize(11);
            nameFont.setBold(true);
            painter->setFont(nameFont);
            
            QFontMetrics fm(nameFont);
            int nameW = fm.horizontalAdvance(displayName);
            painter->drawText(nameRect.left(), nameRect.top() + fm.ascent(), displayName);

            if (hasTag) {
                int tagX = nameRect.left() + nameW + 6;
                int tagY = nameRect.top();
                int tagW = fm.horizontalAdvance(tagText) + 10;
                int tagH = 14;

                painter->setBrush(tagColor);
                painter->setPen(Qt::NoPen);
                painter->drawRoundedRect(tagX, tagY, tagW, tagH, 3, 3);

                painter->setPen(Qt::white);
                QFont tagFont = nameFont;
                tagFont.setPixelSize(9);
                painter->setFont(tagFont);
                painter->drawText(QRect(tagX, tagY, tagW, tagH), Qt::AlignCenter, tagText);
            }
        }

        painter->setPen(textColor);
        QTextDocument doc;
        QFont textFont("Segoe UI", 10);
        doc.setDefaultFont(textFont);
        
        QString html = QString("<span style='color:%1'>%2</span>")
                       .arg(textColor.name())
                       .arg(text.toHtmlEscaped().replace("\n", "<br>"));
        doc.setHtml(html);
        doc.setTextWidth(textRect.width());
        
        painter->translate(textRect.topLeft());
        doc.drawContents(painter);
        painter->translate(-textRect.topLeft());

        QDateTime dt;
        dt.setSecsSinceEpoch(timestamp);
        QString timeStr = dt.toString("hh:mm AP");
        
        painter->setPen(timeColor);
        QFont timeFont = option.font;
        timeFont.setPixelSize(9);
        painter->setFont(timeFont);
        painter->drawText(bubbleRect.adjusted(0,0,-8,-5), Qt::AlignBottom | Qt::AlignRight, timeStr);

        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        QString text = index.data(Qt::UserRole + 1).toString();
        QString sender = index.data(Qt::UserRole + 2).toString();
        bool isMe = index.data(Qt::UserRole + 4).toBool();
        QRect b, t, n, a;
        int h;
        getBubbleLayout(text, sender, isMe, option.rect.width(), b, t, n, a, h);
        return QSize(option.rect.width(), h);
    }
};

// ==========================================
// 3. MAIN WINDOW IMPL
// ==========================================

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_client(new WebSocketClient(this)), m_nam(new QNetworkAccessManager(this))
{
    // Initialize Cache Directory
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/avatars";
    QDir dir;
    if (!dir.exists(cacheDir)) {
        dir.mkpath(cacheDir);
    }

    QSettings settings("Noveo", "MessengerClient");
    m_isDarkMode = settings.value("darkMode", false).toBool();
    
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

    m_statusLabel->setText("Connecting to server...");
    m_client->connectToServer();
}

MainWindow::~MainWindow() {}

void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
}

void MainWindow::setupUi() {
    resize(1000, 700);
    setWindowTitle("Noveo Desktop");

    m_stackedWidget = new QStackedWidget(this);
    setCentralWidget(m_stackedWidget);

    // --- Login Page -----
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

    // --- App Page -----
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
    m_chatListWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_contactListWidget = new QListWidget();
    m_contactListWidget->setIconSize(QSize(42, 42));
    m_contactListWidget->setItemDelegate(new UserListDelegate(this));
    m_contactListWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // Settings
    m_settingsTab = new QWidget();
    QVBoxLayout *settingsLayout = new QVBoxLayout(m_settingsTab);
    QCheckBox *darkModeCheck = new QCheckBox("Dark Mode"); // Works now with include
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

    // Header
    QWidget *header = new QWidget();
    header->setObjectName("chatHeader");
    header->setFixedHeight(60);
    QHBoxLayout *headerLayout = new QHBoxLayout(header);
    m_chatTitle = new QLabel("Select a chat");
    m_chatTitle->setStyleSheet("font-size: 16px; font-weight: bold; margin-left: 10px;");
    headerLayout->addWidget(m_chatTitle);

    // Chat List Widget
    m_chatList = new QListWidget();
    m_chatList->setObjectName("chatList");
    m_chatList->setFrameShape(QFrame::NoFrame);
    m_chatList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_chatList->setUniformItemSizes(false);
    m_chatList->setResizeMode(QListView::Adjust);
    m_chatList->setSelectionMode(QAbstractItemView::NoSelection);
    m_chatList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
    MessageDelegate* msgDelegate = new MessageDelegate(this);
    msgDelegate->setTheme(m_isDarkMode);
    m_chatList->setItemDelegate(msgDelegate);

    connect(m_chatList->verticalScrollBar(), &QScrollBar::valueChanged, this, &MainWindow::onScrollValueChanged);

    // Input Area
    QWidget *inputArea = new QWidget();
    inputArea->setObjectName("inputArea");
    inputArea->setFixedHeight(60);
    QHBoxLayout *inputLayout = new QHBoxLayout(inputArea);
    
    m_messageInput = new QLineEdit();
    m_messageInput->setPlaceholderText("Write a message...");
    
    m_sendBtn = new QPushButton("Send");
    m_sendBtn->setCursor(Qt::PointingHandCursor);

    inputLayout->addWidget(m_messageInput);
    inputLayout->addWidget(m_sendBtn);

    chatAreaLayout->addWidget(header);
    chatAreaLayout->addWidget(m_chatList);
    chatAreaLayout->addWidget(inputArea);

    appLayout->addWidget(m_sidebarTabs);
    appLayout->addWidget(m_chatAreaWidget);

    m_stackedWidget->addWidget(m_appPage);

    connect(m_loginBtn, &QPushButton::clicked, this, &MainWindow::onLoginBtnClicked);
    connect(m_chatListWidget, &QListWidget::itemClicked, this, &MainWindow::onChatSelected);
    connect(m_contactListWidget, &QListWidget::itemClicked, this, &MainWindow::onContactSelected);
    connect(m_sendBtn, &QPushButton::clicked, this, &MainWindow::onSendBtnClicked);
    connect(m_messageInput, &QLineEdit::returnPressed, this, &MainWindow::onSendBtnClicked);
}

// ... (Keep existing login/connection handlers) ...

void MainWindow::onConnected() {
    m_statusLabel->setText("Connected. Please log in.");
}

void MainWindow::onLoginBtnClicked() {
    QString user = m_usernameInput->text().trimmed();
    QString pass = m_passwordInput->text().trimmed();
    if (user.isEmpty() || pass.isEmpty()) return;
    
    m_statusLabel->setText("Logging in...");
    m_client->login(user, pass);
}

void MainWindow::onLoginSuccess(const User &user, const QString &token) {
    QSettings settings("Noveo", "MessengerClient");
    settings.setValue("username", user.username);
    settings.setValue("password", m_passwordInput->text());
    
    m_stackedWidget->setCurrentWidget(m_appPage);
}

void MainWindow::onAuthFailed(const QString &msg) {
    m_statusLabel->setText("Login Failed: " + msg);
}

void MainWindow::onChatHistoryReceived(const std::vector<Chat> &chats) {
    m_chats.clear();
    m_chatListWidget->clear();
    
    for (const auto &chat : chats) {
        m_chats[chat.chatId] = chat;
        
        QListWidgetItem *item = new QListWidgetItem(m_chatListWidget);
        item->setText(resolveChatName(chat));
        item->setData(Qt::UserRole, chat.chatId);
        
        // PFP Handling Logic
        QString url = chat.avatarUrl;
        if (url.startsWith("/")) url = API_BASE_URL + url;
        
        item->setData(AvatarUrlRole, url); // IMPORTANT: Store for update
        item->setIcon(getAvatar(chat.chatName, chat.avatarUrl));
    }
}

void MainWindow::onUserListUpdated(const std::vector<User> &users) {
    m_users.clear();
    m_contactListWidget->clear();
    
    for (const auto &user : users) {
        m_users[user.userId] = user;
        if (user.userId == m_client->currentUserId()) continue;

        QListWidgetItem *item = new QListWidgetItem(m_contactListWidget);
        item->setText(user.username);
        item->setData(Qt::UserRole, user.userId);
        
        QString url = user.avatarUrl;
        if (url.startsWith("/")) url = API_BASE_URL + url;
        
        item->setData(AvatarUrlRole, url);
        item->setIcon(getAvatar(user.username, user.avatarUrl));
    }
}

void MainWindow::onNewChatCreated(const Chat &chat) {
    m_chats[chat.chatId] = chat;
    QListWidgetItem *item = new QListWidgetItem(m_chatListWidget);
    item->setText(resolveChatName(chat));
    item->setData(Qt::UserRole, chat.chatId);
    
    QString url = chat.avatarUrl;
    if (url.startsWith("/")) url = API_BASE_URL + url;
    item->setData(AvatarUrlRole, url);
    item->setIcon(getAvatar(chat.chatName, chat.avatarUrl));
}

// --- AVATAR & CACHING LOGIC ---

QString MainWindow::getCachePath(const QString &url) const {
    // Generate a safe filename from the URL
    QString hash = QCryptographicHash::hash(url.toUtf8(), QCryptographicHash::Md5).toHex();
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/avatars/" + hash + ".png";
}

void MainWindow::saveToCache(const QString &url, const QByteArray &data) {
    QString path = getCachePath(url);
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(data);
        file.close();
    }
}

QIcon MainWindow::getAvatar(const QString &name, const QString &urlIn) {
    if (urlIn.isEmpty() || urlIn.contains("default.png")) {
        return generateGenericAvatar(name);
    }

    QString url = urlIn;
    if (url.startsWith("/")) {
        url = API_BASE_URL + url;
    }

    // 1. Check Memory Cache
    if (m_avatarCache.contains(url)) {
        return QIcon(m_avatarCache[url]);
    }

    // 2. Check Disk Cache
    QString cachePath = getCachePath(url);
    if (QFile::exists(cachePath)) {
        QPixmap pix;
        if (pix.load(cachePath)) {
            // Found on disk! Load immediately for speed
            // Make circular
            QPixmap circular(42, 42);
            circular.fill(Qt::transparent);
            QPainter p(&circular);
            p.setRenderHint(QPainter::Antialiasing);
            QPainterPath path;
            path.addEllipse(0, 0, 42, 42);
            p.setClipPath(path);
            p.drawPixmap(0, 0, 42, 42, pix.scaled(42, 42, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
            
            m_avatarCache.insert(url, circular);
            
            // ALSO: Trigger a background check to see if it changed on server
            downloadAvatar(url);
            
            return QIcon(circular);
        }
    }

    // 3. Not in cache -> Download
    downloadAvatar(url);
    
    // Return placeholder while loading
    return generateGenericAvatar(name);
}

void MainWindow::downloadAvatar(const QString &url) {
    // Optimization: Don't check the same URL twice in one session to save bandwidth
    // OR if it's already downloading
    if (m_checkedUrls.contains(url) || m_pendingDownloads.contains(url)) {
        return;
    }

    m_pendingDownloads.insert(url);
    m_checkedUrls.insert(url); // Mark as checked for this session

    QNetworkRequest request((QUrl(url)));
    // Add SSL config if needed (ignore errors for self-signed)
    QSslConfiguration conf = request.sslConfiguration();
    conf.setPeerVerifyMode(QSslSocket::VerifyNone);
    request.setSslConfiguration(conf);

    QNetworkReply *reply = m_nam->get(request);
    
    connect(reply, &QNetworkReply::finished, this, [this, reply, url]() {
        reply->deleteLater();
        m_pendingDownloads.remove(url);
        
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray data = reply->readAll();
            QPixmap pixmap;
            if (pixmap.loadFromData(data)) {
                // Save original to disk
                saveToCache(url, data);

                // Create circular version for memory cache/display
                QPixmap circular(42, 42);
                circular.fill(Qt::transparent);
                QPainter p(&circular);
                p.setRenderHint(QPainter::Antialiasing);
                QPainterPath path;
                path.addEllipse(0, 0, 42, 42);
                p.setClipPath(path);
                p.drawPixmap(0, 0, 42, 42, pixmap.scaled(42, 42, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
                
                // Update Memory Cache
                m_avatarCache.insert(url, circular);
                
                // Update UI
                updateAvatarOnItems(url, circular); 
            }
        } else {
            // On error, maybe allow retry later? 
            // For now, removing from checkedUrls allows retry next time getAvatar is called
            // m_checkedUrls.remove(url); 
            qDebug() << "Avatar download failed:" << url << reply->errorString();
        }
    });
}

void MainWindow::updateAvatarOnItems(const QString &url, const QPixmap &pixmap) {
    QIcon icon(pixmap);
    
    // Helper to update standard lists
    auto updateList = [&](QListWidget *list) {
        for(int i=0; i < list->count(); ++i) {
             QListWidgetItem *item = list->item(i);
             if (item->data(AvatarUrlRole).toString() == url) {
                 item->setIcon(icon);
             }
        }
    };
    
    updateList(m_chatListWidget);
    updateList(m_contactListWidget);

    // Chat Message Area works differently (MessageDelegate uses DecorationRole)
    // We need to iterate and trigger repaint
    for(int i=0; i < m_chatList->count(); ++i) {
        QListWidgetItem *item = m_chatList->item(i);
        if (item->data(AvatarUrlRole).toString() == url) {
            item->setData(Qt::DecorationRole, icon); // This triggers the delegate repaint
        }
    }
}

QIcon MainWindow::generateGenericAvatar(const QString &name) {
    QPixmap pixmap(42, 42);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    
    painter.setBrush(getColorForName(name));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(0, 0, 42, 42);
    
    painter.setPen(Qt::white);
    QFont font("Arial", 16, QFont::Bold);
    painter.setFont(font);
    
    QString initials = name.left(1).toUpper();
    painter.drawText(QRect(0, 0, 42, 42), Qt::AlignCenter, initials);
    
    return QIcon(pixmap);
}

// --- REST OF UI LOGIC ---

void MainWindow::onChatSelected(QListWidgetItem *item) {
    QString chatId = item->data(Qt::UserRole).toString();
    if (chatId == m_currentChatId) return;
    
    m_currentChatId = chatId;
    m_chatTitle->setText(item->text());
    m_chatList->clear(); // Clear UI
    m_isLoadingHistory = false; // Reset lock
    
    renderMessages(chatId);
    
    // Smooth scroll to bottom after rendering
    QTimer::singleShot(100, this, &MainWindow::scrollToBottom);
}

void MainWindow::renderMessages(const QString &chatId) {
    if (!m_chats.contains(chatId)) return;
    
    const auto &messages = m_chats[chatId].messages;
    for (const auto &msg : messages) {
        addMessageBubble(msg, false, false);
    }
}

void MainWindow::addMessageBubble(const Message &msg, bool appendStretch, bool animate) {
    QListWidgetItem *item = new QListWidgetItem(m_chatList);
    
    // Store data for the Delegate
    item->setData(Qt::UserRole + 1, msg.text);
    
    bool isMe = (msg.senderId == m_client->currentUserId());
    QString senderName = "Unknown";
    QString avatarUrl = "";
    
    if (isMe) {
        senderName = "Me";
    } else {
        if (m_users.contains(msg.senderId)) {
            senderName = m_users[msg.senderId].username;
            avatarUrl = m_users[msg.senderId].avatarUrl;
        } else {
            // Try to find in chat members...
            senderName = msg.senderId;
        }
    }
    
    item->setData(Qt::UserRole + 2, senderName);
    item->setData(Qt::UserRole + 3, msg.timestamp);
    item->setData(Qt::UserRole + 4, isMe);

    // Set Avatar Data correctly for updates
    if (!isMe) {
        QString url = avatarUrl;
        if (url.startsWith("/")) url = API_BASE_URL + url;
        
        item->setData(AvatarUrlRole, url);
        item->setData(Qt::DecorationRole, getAvatar(senderName, avatarUrl));
    }

    m_chatList->addItem(item);
    
    if (animate) {
        // Optional: Simple fade in or slide
    }
}

void MainWindow::onMessageReceived(const Message &msg) {
    // Update data model
    if (m_chats.contains(msg.chatId)) {
        m_chats[msg.chatId].messages.push_back(msg);
        
        // Move chat to top of sidebar
        for(int i=0; i<m_chatListWidget->count(); i++) {
            if (m_chatListWidget->item(i)->data(Qt::UserRole).toString() == msg.chatId) {
                QListWidgetItem *item = m_chatListWidget->takeItem(i);
                m_chatListWidget->insertItem(0, item);
                m_chatListWidget->setCurrentItem(item);
                break;
            }
        }
    }
    
    if (m_currentChatId == msg.chatId) {
        bool wasAtBottom = isScrolledToBottom();
        addMessageBubble(msg, false, true);
        if (wasAtBottom || msg.senderId == m_client->currentUserId()) {
            smoothScrollToBottom();
        }
    }
}

void MainWindow::onSendBtnClicked() {
    QString text = m_messageInput->text().trimmed();
    if (text.isEmpty() || m_currentChatId.isEmpty()) return;
    
    m_client->sendMessage(m_currentChatId, text);
    m_messageInput->clear();
}

void MainWindow::onContactSelected(QListWidgetItem *item) {
    QString userId = item->data(Qt::UserRole).toString();
    // Logic to start/open chat with user...
}

// ... (Helpers: Color, Name resolution) ...
QString MainWindow::resolveChatName(const Chat &chat) {
    if (chat.chatType == "private") {
        for (const QString &uid : chat.members) {
            if (uid != m_client->currentUserId()) {
                if (m_users.contains(uid)) return m_users[uid].username;
            }
        }
    }
    return chat.chatName;
}

QColor MainWindow::getColorForName(const QString &name) {
    int hash = 0;
    for (QChar c : name) hash = c.unicode() + ((hash << 5) - hash);
    int h = qAbs(hash) % 360;
    return QColor::fromHsl(h, 200, 150);
}

void MainWindow::applyTheme() {
    // FIXED: Removed qApp dependency by using QApplication::instance() or just qApp macro (which works with #include <QApplication>)
    if (m_isDarkMode) {
        qApp->setStyleSheet(
            "QMainWindow { background-color: #1e1e1e; }"
            "QWidget { color: #ffffff; }"
            "QLineEdit { background-color: #2d2d2d; border: 1px solid #3d3d3d; padding: 8px; border-radius: 4px; color: white; }"
            "QListWidget { background-color: #252526; border: none; }"
            "QListWidget::item { padding: 10px; border-bottom: 1px solid #2d2d2d; }"
            "QListWidget::item:selected { background-color: #37373d; }"
            "QPushButton { background-color: #007acc; color: white; border: none; padding: 8px; border-radius: 4px; }"
            "QPushButton:hover { background-color: #0098ff; }"
            "QTabWidget::pane { border: 1px solid #3d3d3d; }"
            "QTabBar::tab { background: #2d2d2d; color: #ccc; padding: 8px 20px; }"
            "QTabBar::tab:selected { background: #3d3d3d; color: white; }"
        );
        // FIXED: Use dynamic_cast instead of qobject_cast since we removed Q_OBJECT from the internal class
        MessageDelegate* d = dynamic_cast<MessageDelegate*>(m_chatList->itemDelegate());
        if(d) d->setTheme(true);
    } else {
        qApp->setStyleSheet(
            "QMainWindow { background-color: #f0f2f5; }"
            "QWidget { color: #000000; }"
            "QLineEdit { background-color: white; border: 1px solid #ccc; padding: 8px; border-radius: 4px; }"
            "QListWidget { background-color: white; border: none; }"
            "QListWidget::item { padding: 10px; border-bottom: 1px solid #f0f0f0; }"
            "QListWidget::item:selected { background-color: #e6f3ff; }"
            "QPushButton { background-color: #0084ff; color: white; border: none; padding: 8px; border-radius: 4px; }"
            "QPushButton:hover { background-color: #0096ff; }"
            "QTabWidget::pane { border: 1px solid #ddd; }"
            "QTabBar::tab { background: #e0e0e0; color: #333; padding: 8px 20px; }"
            "QTabBar::tab:selected { background: white; color: black; }"
        );
        MessageDelegate* d = dynamic_cast<MessageDelegate*>(m_chatList->itemDelegate());
        if(d) d->setTheme(false);
    }
}

void MainWindow::onDarkModeToggled(bool checked) {
    m_isDarkMode = checked;
    QSettings settings("Noveo", "MessengerClient");
    settings.setValue("darkMode", m_isDarkMode);
    applyTheme();
    // Refresh chat list to apply new colors
    m_chatList->viewport()->update();
}

void MainWindow::onLogoutClicked() {
    // Clear settings and reset
    QSettings settings("Noveo", "MessengerClient");
    settings.remove("username");
    settings.remove("password");
    m_stackedWidget->setCurrentWidget(m_loginPage);
    // FIXED: Removed unknown method 'disconnectFromServer'. Reconnecting will reset state anyway.
    m_client->connectToServer();
}

void MainWindow::scrollToBottom() {
    if (m_chatList->count() > 0)
        m_chatList->scrollToBottom();
}

void MainWindow::smoothScrollToBottom() {
    scrollToBottom();
}

bool MainWindow::isScrolledToBottom() const {
    QScrollBar *sb = m_chatList->verticalScrollBar();
    return sb->value() >= (sb->maximum() - 50);
}

void MainWindow::onScrollValueChanged(int value) {
    // History loading logic stub
    if (value < 50 && !m_isLoadingHistory && m_chatList->count() > 0) {
        // m_isLoadingHistory = true;
        // m_client->requestHistory(...);
    }
}

void MainWindow::prependMessageBubble(const Message &msg) {
    // Logic to insert item at index 0 and maintain scroll position
}
