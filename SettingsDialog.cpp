#include "SettingsDialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMap>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace {
QString normalizeLanguageCode(const QString& code)
{
    const QString candidate = code.trimmed().toLower();
    if (candidate == QStringLiteral("fa") ||
        candidate == QStringLiteral("ar") ||
        candidate == QStringLiteral("ru") ||
        candidate == QStringLiteral("zh")) {
        return candidate;
    }
    return QStringLiteral("en");
}

QString settingsText(const QString& languageCode, const QString& key)
{
    static const QMap<QString, QMap<QString, QString>> kTexts = {
        {QStringLiteral("en"), {
            {QStringLiteral("settings_title"), QStringLiteral("Settings")},
            {QStringLiteral("profile_title"), QStringLiteral("Profile")},
            {QStringLiteral("account_title"), QStringLiteral("Account")},
            {QStringLiteral("preferences_title"), QStringLiteral("Preferences")},
            {QStringLiteral("menu_profile"), QStringLiteral("Profile")},
            {QStringLiteral("menu_account"), QStringLiteral("Account")},
            {QStringLiteral("menu_preferences"), QStringLiteral("Preferences")},
            {QStringLiteral("display_name_placeholder"), QStringLiteral("Display name")},
            {QStringLiteral("save"), QStringLiteral("Save")},
            {QStringLiteral("change_password"), QStringLiteral("Change Password")},
            {QStringLiteral("delete_account"), QStringLiteral("Delete Account")},
            {QStringLiteral("logout"), QStringLiteral("Logout")},
            {QStringLiteral("dark_mode"), QStringLiteral("Dark Mode")},
            {QStringLiteral("notifications"), QStringLiteral("Notifications")},
            {QStringLiteral("block_group_invites"), QStringLiteral("Block Group Invites")},
            {QStringLiteral("language"), QStringLiteral("Language")},
            {QStringLiteral("updater_title"), QStringLiteral("Application Update")},
            {QStringLiteral("check"), QStringLiteral("Check")},
            {QStringLiteral("download"), QStringLiteral("Download")},
            {QStringLiteral("install"), QStringLiteral("Install")},
            {QStringLiteral("id_prefix"), QStringLiteral("ID: %1")},
        }},
        {QStringLiteral("fa"), {
            {QStringLiteral("settings_title"), QStringLiteral("تنظیمات")},
            {QStringLiteral("profile_title"), QStringLiteral("پروفایل")},
            {QStringLiteral("account_title"), QStringLiteral("حساب")},
            {QStringLiteral("preferences_title"), QStringLiteral("ترجیحات")},
            {QStringLiteral("menu_profile"), QStringLiteral("پروفایل")},
            {QStringLiteral("menu_account"), QStringLiteral("حساب")},
            {QStringLiteral("menu_preferences"), QStringLiteral("ترجیحات")},
            {QStringLiteral("display_name_placeholder"), QStringLiteral("نام نمایشی")},
            {QStringLiteral("save"), QStringLiteral("ذخیره")},
            {QStringLiteral("change_password"), QStringLiteral("تغییر رمز عبور")},
            {QStringLiteral("delete_account"), QStringLiteral("حذف حساب")},
            {QStringLiteral("logout"), QStringLiteral("خروج")},
            {QStringLiteral("dark_mode"), QStringLiteral("حالت تیره")},
            {QStringLiteral("notifications"), QStringLiteral("اعلان‌ها")},
            {QStringLiteral("block_group_invites"), QStringLiteral("مسدود کردن دعوت گروه")},
            {QStringLiteral("language"), QStringLiteral("زبان")},
            {QStringLiteral("updater_title"), QStringLiteral("به‌روزرسانی برنامه")},
            {QStringLiteral("check"), QStringLiteral("بررسی")},
            {QStringLiteral("download"), QStringLiteral("دانلود")},
            {QStringLiteral("install"), QStringLiteral("نصب")},
            {QStringLiteral("id_prefix"), QStringLiteral("شناسه: %1")},
        }},
        {QStringLiteral("ar"), {
            {QStringLiteral("settings_title"), QStringLiteral("الإعدادات")},
            {QStringLiteral("profile_title"), QStringLiteral("الملف الشخصي")},
            {QStringLiteral("account_title"), QStringLiteral("الحساب")},
            {QStringLiteral("preferences_title"), QStringLiteral("التفضيلات")},
            {QStringLiteral("menu_profile"), QStringLiteral("الملف الشخصي")},
            {QStringLiteral("menu_account"), QStringLiteral("الحساب")},
            {QStringLiteral("menu_preferences"), QStringLiteral("التفضيلات")},
            {QStringLiteral("display_name_placeholder"), QStringLiteral("الاسم المعروض")},
            {QStringLiteral("save"), QStringLiteral("حفظ")},
            {QStringLiteral("change_password"), QStringLiteral("تغيير كلمة المرور")},
            {QStringLiteral("delete_account"), QStringLiteral("حذف الحساب")},
            {QStringLiteral("logout"), QStringLiteral("تسجيل الخروج")},
            {QStringLiteral("dark_mode"), QStringLiteral("الوضع الداكن")},
            {QStringLiteral("notifications"), QStringLiteral("الإشعارات")},
            {QStringLiteral("block_group_invites"), QStringLiteral("حظر دعوات المجموعات")},
            {QStringLiteral("language"), QStringLiteral("اللغة")},
            {QStringLiteral("updater_title"), QStringLiteral("تحديث التطبيق")},
            {QStringLiteral("check"), QStringLiteral("فحص")},
            {QStringLiteral("download"), QStringLiteral("تنزيل")},
            {QStringLiteral("install"), QStringLiteral("تثبيت")},
            {QStringLiteral("id_prefix"), QStringLiteral("المعرف: %1")},
        }},
        {QStringLiteral("ru"), {
            {QStringLiteral("settings_title"), QStringLiteral("Настройки")},
            {QStringLiteral("profile_title"), QStringLiteral("Профиль")},
            {QStringLiteral("account_title"), QStringLiteral("Аккаунт")},
            {QStringLiteral("preferences_title"), QStringLiteral("Предпочтения")},
            {QStringLiteral("menu_profile"), QStringLiteral("Профиль")},
            {QStringLiteral("menu_account"), QStringLiteral("Аккаунт")},
            {QStringLiteral("menu_preferences"), QStringLiteral("Предпочтения")},
            {QStringLiteral("display_name_placeholder"), QStringLiteral("Отображаемое имя")},
            {QStringLiteral("save"), QStringLiteral("Сохранить")},
            {QStringLiteral("change_password"), QStringLiteral("Изменить пароль")},
            {QStringLiteral("delete_account"), QStringLiteral("Удалить аккаунт")},
            {QStringLiteral("logout"), QStringLiteral("Выйти")},
            {QStringLiteral("dark_mode"), QStringLiteral("Темная тема")},
            {QStringLiteral("notifications"), QStringLiteral("Уведомления")},
            {QStringLiteral("block_group_invites"), QStringLiteral("Блокировать приглашения в группы")},
            {QStringLiteral("language"), QStringLiteral("Язык")},
            {QStringLiteral("updater_title"), QStringLiteral("Обновление приложения")},
            {QStringLiteral("check"), QStringLiteral("Проверить")},
            {QStringLiteral("download"), QStringLiteral("Скачать")},
            {QStringLiteral("install"), QStringLiteral("Установить")},
            {QStringLiteral("id_prefix"), QStringLiteral("ID: %1")},
        }},
        {QStringLiteral("zh"), {
            {QStringLiteral("settings_title"), QStringLiteral("设置")},
            {QStringLiteral("profile_title"), QStringLiteral("个人资料")},
            {QStringLiteral("account_title"), QStringLiteral("账户")},
            {QStringLiteral("preferences_title"), QStringLiteral("偏好设置")},
            {QStringLiteral("menu_profile"), QStringLiteral("个人资料")},
            {QStringLiteral("menu_account"), QStringLiteral("账户")},
            {QStringLiteral("menu_preferences"), QStringLiteral("偏好设置")},
            {QStringLiteral("display_name_placeholder"), QStringLiteral("显示名称")},
            {QStringLiteral("save"), QStringLiteral("保存")},
            {QStringLiteral("change_password"), QStringLiteral("修改密码")},
            {QStringLiteral("delete_account"), QStringLiteral("删除账户")},
            {QStringLiteral("logout"), QStringLiteral("退出登录")},
            {QStringLiteral("dark_mode"), QStringLiteral("深色模式")},
            {QStringLiteral("notifications"), QStringLiteral("通知")},
            {QStringLiteral("block_group_invites"), QStringLiteral("阻止群组邀请")},
            {QStringLiteral("language"), QStringLiteral("语言")},
            {QStringLiteral("updater_title"), QStringLiteral("应用更新")},
            {QStringLiteral("check"), QStringLiteral("检查")},
            {QStringLiteral("download"), QStringLiteral("下载")},
            {QStringLiteral("install"), QStringLiteral("安装")},
            {QStringLiteral("id_prefix"), QStringLiteral("ID: %1")},
        }},
    };

    const QString normalized = normalizeLanguageCode(languageCode);
    const QMap<QString, QString>& localized = kTexts.value(normalized);
    if (localized.contains(key)) {
        return localized.value(key);
    }
    return kTexts.value(QStringLiteral("en")).value(key, key);
}
}

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Settings"));
    setModal(false);
    setWindowModality(Qt::NonModal);
    resize(440, 560);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* header = new QWidget(this);
    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(10, 8, 10, 8);
    headerLayout->setSpacing(8);

    m_backButton = new QPushButton(QStringLiteral("<"), header);
    m_backButton->setObjectName(QStringLiteral("settingsBackButton"));
    m_backButton->setFixedWidth(34);
    m_backButton->setCursor(Qt::PointingHandCursor);
    m_backButton->setVisible(false);
    headerLayout->addWidget(m_backButton, 0, Qt::AlignLeft);

    m_titleLabel = new QLabel(QStringLiteral("Settings"), header);
    m_titleLabel->setObjectName(QStringLiteral("settingsTitleLabel"));
    m_titleLabel->setAlignment(Qt::AlignCenter);
    headerLayout->addWidget(m_titleLabel, 1);

    m_closeButton = new QPushButton(QStringLiteral("x"), header);
    m_closeButton->setObjectName(QStringLiteral("settingsCloseButton"));
    m_closeButton->setFixedWidth(34);
    m_closeButton->setCursor(Qt::PointingHandCursor);
    headerLayout->addWidget(m_closeButton, 0, Qt::AlignRight);

    root->addWidget(header);

    auto* line = new QFrame(this);
    line->setObjectName(QStringLiteral("settingsDivider"));
    line->setFrameShape(QFrame::HLine);
    root->addWidget(line);

    m_sections = new QStackedWidget(this);
    root->addWidget(m_sections, 1);

    auto* menuPage = new QWidget(m_sections);
    auto* menuLayout = new QVBoxLayout(menuPage);
    menuLayout->setContentsMargins(14, 14, 14, 14);
    menuLayout->setSpacing(6);
    m_menuProfileButton = new QPushButton(QStringLiteral("Profile"), menuPage);
    m_menuAccountButton = new QPushButton(QStringLiteral("Account"), menuPage);
    m_menuPreferencesButton = new QPushButton(QStringLiteral("Preferences"), menuPage);
    m_menuProfileButton->setObjectName(QStringLiteral("menuOption"));
    m_menuAccountButton->setObjectName(QStringLiteral("menuOption"));
    m_menuPreferencesButton->setObjectName(QStringLiteral("menuOption"));
    m_menuProfileButton->setCursor(Qt::PointingHandCursor);
    m_menuAccountButton->setCursor(Qt::PointingHandCursor);
    m_menuPreferencesButton->setCursor(Qt::PointingHandCursor);
    menuLayout->addWidget(m_menuProfileButton);
    menuLayout->addWidget(m_menuAccountButton);
    menuLayout->addWidget(m_menuPreferencesButton);
    menuLayout->addStretch();
    m_sections->addWidget(menuPage);

    auto* profilePage = new QWidget(m_sections);
    auto* profileLayout = new QVBoxLayout(profilePage);
    profileLayout->setContentsMargins(14, 14, 14, 14);
    profileLayout->setSpacing(10);

    m_profileUsernameLabel = new QLabel(QStringLiteral("Username"), profilePage);
    m_profileUsernameLabel->setObjectName(QStringLiteral("settingsProfileUsernameLabel"));
    m_profileUserIdLabel = new QLabel(QStringLiteral("ID"), profilePage);
    m_profileUserIdLabel->setObjectName(QStringLiteral("settingsProfileUserIdLabel"));
    m_editDisplayNameInput = new QLineEdit(profilePage);
    m_editDisplayNameInput->setPlaceholderText(QStringLiteral("Display name"));
    m_saveProfileButton = new QPushButton(QStringLiteral("Save"), profilePage);
    m_saveProfileButton->setObjectName(QStringLiteral("primaryAction"));
    m_saveProfileButton->setCursor(Qt::PointingHandCursor);

    profileLayout->addWidget(m_profileUsernameLabel);
    profileLayout->addWidget(m_profileUserIdLabel);
    profileLayout->addSpacing(6);
    profileLayout->addWidget(m_editDisplayNameInput);
    profileLayout->addWidget(m_saveProfileButton);
    profileLayout->addStretch();
    m_sections->addWidget(profilePage);

    auto* accountPage = new QWidget(m_sections);
    auto* accountLayout = new QVBoxLayout(accountPage);
    accountLayout->setContentsMargins(14, 14, 14, 14);
    accountLayout->setSpacing(6);

    m_changePasswordButton = new QPushButton(QStringLiteral("Change Password"), accountPage);
    m_deleteAccountButton = new QPushButton(QStringLiteral("Delete Account"), accountPage);
    m_logoutButton = new QPushButton(QStringLiteral("Logout"), accountPage);
    m_changePasswordButton->setObjectName(QStringLiteral("sectionAction"));
    m_deleteAccountButton->setObjectName(QStringLiteral("dangerAction"));
    m_logoutButton->setObjectName(QStringLiteral("sectionAction"));
    m_changePasswordButton->setCursor(Qt::PointingHandCursor);
    m_deleteAccountButton->setCursor(Qt::PointingHandCursor);
    m_logoutButton->setCursor(Qt::PointingHandCursor);

    accountLayout->addWidget(m_changePasswordButton);
    accountLayout->addWidget(m_deleteAccountButton);
    accountLayout->addWidget(m_logoutButton);
    accountLayout->addStretch();
    m_sections->addWidget(accountPage);

    auto* prefPage = new QWidget(m_sections);
    auto* prefLayout = new QVBoxLayout(prefPage);
    prefLayout->setContentsMargins(14, 14, 14, 14);
    prefLayout->setSpacing(8);

    m_darkModeCheck = new QCheckBox(QStringLiteral("Dark Mode"), prefPage);
    m_notificationsCheck = new QCheckBox(QStringLiteral("Notifications"), prefPage);
    m_blockInvitesCheck = new QCheckBox(QStringLiteral("Block Group Invites"), prefPage);
    prefLayout->addWidget(m_darkModeCheck);
    prefLayout->addWidget(m_notificationsCheck);
    prefLayout->addWidget(m_blockInvitesCheck);

    auto* languageRow = new QHBoxLayout();
    m_languageLabel = new QLabel(QStringLiteral("Language"), prefPage);
    m_languageCombo = new QComboBox(prefPage);
    m_languageCombo->addItem(QStringLiteral("English"), QStringLiteral("en"));
    m_languageCombo->addItem(QStringLiteral("فارسی"), QStringLiteral("fa"));
    m_languageCombo->addItem(QStringLiteral("العربية"), QStringLiteral("ar"));
    m_languageCombo->addItem(QStringLiteral("Русский"), QStringLiteral("ru"));
    m_languageCombo->addItem(QStringLiteral("中文"), QStringLiteral("zh"));
    languageRow->addWidget(m_languageLabel);
    languageRow->addStretch();
    languageRow->addWidget(m_languageCombo);
    prefLayout->addLayout(languageRow);
    prefLayout->addSpacing(6);

    m_updaterTitleLabel = new QLabel(QStringLiteral("Application Update"), prefPage);
    m_updaterTitleLabel->setStyleSheet("font-weight: 700;");
    m_updaterStatusLabel = new QLabel(QStringLiteral("Updater idle."), prefPage);
    m_updaterStatusLabel->setWordWrap(true);
    auto* updaterButtons = new QHBoxLayout();
    m_checkUpdateButton = new QPushButton(QStringLiteral("Check"), prefPage);
    m_downloadUpdateButton = new QPushButton(QStringLiteral("Download"), prefPage);
    m_installUpdateButton = new QPushButton(QStringLiteral("Install"), prefPage);
    m_checkUpdateButton->setObjectName(QStringLiteral("secondaryAction"));
    m_downloadUpdateButton->setObjectName(QStringLiteral("primaryAction"));
    m_installUpdateButton->setObjectName(QStringLiteral("successAction"));
    m_checkUpdateButton->setCursor(Qt::PointingHandCursor);
    m_downloadUpdateButton->setCursor(Qt::PointingHandCursor);
    m_installUpdateButton->setCursor(Qt::PointingHandCursor);
    updaterButtons->addWidget(m_checkUpdateButton);
    updaterButtons->addWidget(m_downloadUpdateButton);
    updaterButtons->addWidget(m_installUpdateButton);

    prefLayout->addWidget(m_updaterTitleLabel);
    prefLayout->addWidget(m_updaterStatusLabel);
    prefLayout->addLayout(updaterButtons);
    prefLayout->addStretch();
    m_sections->addWidget(prefPage);

    connect(m_backButton, &QPushButton::clicked, this, &SettingsDialog::showMenu);
    connect(m_closeButton, &QPushButton::clicked, this, &SettingsDialog::hide);

    connect(m_menuProfileButton, &QPushButton::clicked, this, [this]() { switchSection(Section::Profile); });
    connect(m_menuAccountButton, &QPushButton::clicked, this, [this]() { switchSection(Section::Account); });
    connect(m_menuPreferencesButton, &QPushButton::clicked, this, [this]() { switchSection(Section::Preferences); });

    connect(m_saveProfileButton, &QPushButton::clicked, this, [this]() {
        emit saveDisplayNameRequested(m_editDisplayNameInput->text().trimmed());
    });
    connect(m_changePasswordButton, &QPushButton::clicked, this, &SettingsDialog::changePasswordRequested);
    connect(m_deleteAccountButton, &QPushButton::clicked, this, &SettingsDialog::deleteAccountRequested);
    connect(m_logoutButton, &QPushButton::clicked, this, &SettingsDialog::logoutRequested);

    connect(m_darkModeCheck, &QCheckBox::toggled, this, &SettingsDialog::darkModeToggled);
    connect(m_notificationsCheck, &QCheckBox::toggled, this, &SettingsDialog::notificationsToggled);
    connect(m_blockInvitesCheck, &QCheckBox::toggled, this, &SettingsDialog::blockGroupInvitesToggled);
    connect(m_checkUpdateButton, &QPushButton::clicked, this, &SettingsDialog::checkForUpdatesRequested);
    connect(m_downloadUpdateButton, &QPushButton::clicked, this, &SettingsDialog::downloadUpdateRequested);
    connect(m_installUpdateButton, &QPushButton::clicked, this, &SettingsDialog::installUpdateRequested);
    connect(m_languageCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this]() {
        const QString code = m_languageCombo ? m_languageCombo->currentData().toString() : QStringLiteral("en");
        setLanguage(code);
        emit languageChanged(m_languageCode);
    });

    showMenu();
    setLanguage(QStringLiteral("en"));
    applyTheme();
}

void SettingsDialog::setUserInfo(const QString& username, const QString& userId)
{
    m_profileUsernameLabel->setText(username);
    m_currentUserId = userId;
    m_profileUserIdLabel->setText(settingsText(m_languageCode, QStringLiteral("id_prefix")).arg(userId));
    m_editDisplayNameInput->setText(username);
}

void SettingsDialog::setDarkMode(bool enabled)
{
    m_darkMode = enabled;
    m_darkModeCheck->blockSignals(true);
    m_darkModeCheck->setChecked(enabled);
    m_darkModeCheck->blockSignals(false);
    applyTheme();
}

void SettingsDialog::setNotifications(bool enabled)
{
    m_notificationsCheck->blockSignals(true);
    m_notificationsCheck->setChecked(enabled);
    m_notificationsCheck->blockSignals(false);
}

void SettingsDialog::setBlockGroupInvites(bool enabled)
{
    m_blockInvitesCheck->blockSignals(true);
    m_blockInvitesCheck->setChecked(enabled);
    m_blockInvitesCheck->blockSignals(false);
}

void SettingsDialog::setUpdaterState(const QString& statusText, bool canDownload, bool canInstall)
{
    m_updaterStatusLabel->setText(statusText);
    m_downloadUpdateButton->setEnabled(canDownload);
    m_installUpdateButton->setEnabled(canInstall);
}

void SettingsDialog::setCurrentLanguage(const QString& languageCode)
{
    const QString normalized = normalizeLanguageCode(languageCode);
    if (m_languageCombo) {
        const QSignalBlocker blocker(m_languageCombo);
        int idx = m_languageCombo->findData(normalized);
        if (idx < 0) {
            idx = 0;
        }
        m_languageCombo->setCurrentIndex(idx);
    }
    setLanguage(normalized);
}

void SettingsDialog::setLanguage(const QString& languageCode)
{
    m_languageCode = normalizeLanguageCode(languageCode);

    if (m_menuProfileButton) {
        m_menuProfileButton->setText(settingsText(m_languageCode, QStringLiteral("menu_profile")));
    }
    if (m_menuAccountButton) {
        m_menuAccountButton->setText(settingsText(m_languageCode, QStringLiteral("menu_account")));
    }
    if (m_menuPreferencesButton) {
        m_menuPreferencesButton->setText(settingsText(m_languageCode, QStringLiteral("menu_preferences")));
    }
    if (m_editDisplayNameInput) {
        m_editDisplayNameInput->setPlaceholderText(settingsText(m_languageCode, QStringLiteral("display_name_placeholder")));
    }
    if (m_saveProfileButton) {
        m_saveProfileButton->setText(settingsText(m_languageCode, QStringLiteral("save")));
    }
    if (m_changePasswordButton) {
        m_changePasswordButton->setText(settingsText(m_languageCode, QStringLiteral("change_password")));
    }
    if (m_deleteAccountButton) {
        m_deleteAccountButton->setText(settingsText(m_languageCode, QStringLiteral("delete_account")));
    }
    if (m_logoutButton) {
        m_logoutButton->setText(settingsText(m_languageCode, QStringLiteral("logout")));
    }
    if (m_darkModeCheck) {
        m_darkModeCheck->setText(settingsText(m_languageCode, QStringLiteral("dark_mode")));
    }
    if (m_notificationsCheck) {
        m_notificationsCheck->setText(settingsText(m_languageCode, QStringLiteral("notifications")));
    }
    if (m_blockInvitesCheck) {
        m_blockInvitesCheck->setText(settingsText(m_languageCode, QStringLiteral("block_group_invites")));
    }
    if (m_languageLabel) {
        m_languageLabel->setText(settingsText(m_languageCode, QStringLiteral("language")));
    }
    if (m_updaterTitleLabel) {
        m_updaterTitleLabel->setText(settingsText(m_languageCode, QStringLiteral("updater_title")));
    }
    if (m_checkUpdateButton) {
        m_checkUpdateButton->setText(settingsText(m_languageCode, QStringLiteral("check")));
    }
    if (m_downloadUpdateButton) {
        m_downloadUpdateButton->setText(settingsText(m_languageCode, QStringLiteral("download")));
    }
    if (m_installUpdateButton) {
        m_installUpdateButton->setText(settingsText(m_languageCode, QStringLiteral("install")));
    }
    if (m_profileUserIdLabel) {
        m_profileUserIdLabel->setText(settingsText(m_languageCode, QStringLiteral("id_prefix")).arg(m_currentUserId));
    }

    syncHeaderForSection(static_cast<Section>(m_sections->currentIndex()));
}

void SettingsDialog::showMenu()
{
    switchSection(Section::Menu);
}

void SettingsDialog::applyTheme()
{
    const QString bg = m_darkMode ? QStringLiteral("#111827") : QStringLiteral("#ffffff");
    const QString text = m_darkMode ? QStringLiteral("#e5e7eb") : QStringLiteral("#111827");
    const QString muted = m_darkMode ? QStringLiteral("#9ca3af") : QStringLiteral("#6b7280");
    const QString divider = m_darkMode ? QStringLiteral("#374151") : QStringLiteral("#e5e7eb");
    const QString hover = m_darkMode ? QStringLiteral("#1f2937") : QStringLiteral("#f3f4f6");
    const QString dangerHover = m_darkMode ? QStringLiteral("#3f1d21") : QStringLiteral("#fef2f2");
    const QString inputBg = m_darkMode ? QStringLiteral("#1f2937") : QStringLiteral("#ffffff");
    const QString inputBorder = m_darkMode ? QStringLiteral("#4b5563") : QStringLiteral("#d1d5db");

    setStyleSheet(QString(
        "QDialog { background: %1; color: %2; border: 1px solid %4; border-radius: 12px; }"
        "#settingsDivider { color: %4; background: %4; }"
        "#settingsTitleLabel { font-size: 18px; font-weight: 700; color: %2; }"
        "#settingsProfileUsernameLabel { font-size: 16px; font-weight: 700; color: %2; }"
        "#settingsProfileUserIdLabel { font-size: 12px; color: %3; }"
        "QPushButton#settingsBackButton, QPushButton#settingsCloseButton {"
        " border: none; color: %3; font-weight: 700; }"
        "QPushButton#settingsBackButton:hover, QPushButton#settingsCloseButton:hover { color: %2; }"
        "QPushButton#menuOption, QPushButton#sectionAction {"
        " text-align: left; padding: 10px 12px; border: none; border-radius: 8px; background: transparent; color: %2; }"
        "QPushButton#menuOption:hover, QPushButton#sectionAction:hover { background: %5; }"
        "QPushButton#dangerAction {"
        " text-align: left; padding: 10px 12px; border: none; border-radius: 8px; background: transparent; color: #dc2626; }"
        "QPushButton#dangerAction:hover { background: %6; }"
        "QLineEdit, QComboBox {"
        " padding: 8px 10px; border: 1px solid %8; border-radius: 8px; background: %7; color: %2; }"
        "QCheckBox { color: %2; }"
        "QPushButton#primaryAction {"
        " background: #2563eb; color: #ffffff; border: none; border-radius: 8px; padding: 8px 12px; }"
        "QPushButton#primaryAction:hover { background: #1d4ed8; }"
        "QPushButton#secondaryAction {"
        " background: #6b7280; color: #ffffff; border: none; border-radius: 8px; padding: 8px 12px; }"
        "QPushButton#secondaryAction:hover { background: #4b5563; }"
        "QPushButton#successAction {"
        " background: #16a34a; color: #ffffff; border: none; border-radius: 8px; padding: 8px 12px; }"
        "QPushButton#successAction:hover { background: #15803d; }")
                      .arg(bg, text, muted, divider, hover, dangerHover, inputBg, inputBorder));
}

void SettingsDialog::switchSection(Section section)
{
    m_sections->setCurrentIndex(static_cast<int>(section));
    syncHeaderForSection(section);
}

void SettingsDialog::syncHeaderForSection(Section section)
{
    switch (section) {
    case Section::Menu:
        m_titleLabel->setText(settingsText(m_languageCode, QStringLiteral("settings_title")));
        m_backButton->setVisible(false);
        break;
    case Section::Profile:
        m_titleLabel->setText(settingsText(m_languageCode, QStringLiteral("profile_title")));
        m_backButton->setVisible(true);
        break;
    case Section::Account:
        m_titleLabel->setText(settingsText(m_languageCode, QStringLiteral("account_title")));
        m_backButton->setVisible(true);
        break;
    case Section::Preferences:
        m_titleLabel->setText(settingsText(m_languageCode, QStringLiteral("preferences_title")));
        m_backButton->setVisible(true);
        break;
    }
}
