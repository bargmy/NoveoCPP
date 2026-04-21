#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QStackedWidget;

class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget* parent = nullptr);

    void setUserInfo(const QString& username, const QString& userId);
    void setDarkMode(bool enabled);
    void setNotifications(bool enabled);
    void setBlockGroupInvites(bool enabled);
    void setUpdaterState(const QString& statusText, bool canDownload, bool canInstall);
    void setCurrentLanguage(const QString& languageCode);
    void setLanguage(const QString& languageCode);
    void showMenu();

signals:
    void saveDisplayNameRequested(const QString& newName);
    void changePasswordRequested();
    void deleteAccountRequested();
    void logoutRequested();
    void darkModeToggled(bool enabled);
    void notificationsToggled(bool enabled);
    void blockGroupInvitesToggled(bool enabled);
    void checkForUpdatesRequested();
    void downloadUpdateRequested();
    void installUpdateRequested();
    void languageChanged(const QString& languageCode);

private:
    enum class Section {
        Menu = 0,
        Profile,
        Account,
        Preferences
    };

    void applyTheme();
    void switchSection(Section section);
    void syncHeaderForSection(Section section);
    bool m_darkMode = false;
    QString m_languageCode = QStringLiteral("en");
    QString m_currentUserId;

    QLabel* m_titleLabel = nullptr;
    QPushButton* m_backButton = nullptr;
    QPushButton* m_closeButton = nullptr;
    QStackedWidget* m_sections = nullptr;

    QPushButton* m_menuProfileButton = nullptr;
    QPushButton* m_menuAccountButton = nullptr;
    QPushButton* m_menuPreferencesButton = nullptr;
    QLabel* m_profileUsernameLabel = nullptr;
    QLabel* m_profileUserIdLabel = nullptr;
    QLineEdit* m_editDisplayNameInput = nullptr;
    QPushButton* m_saveProfileButton = nullptr;

    QPushButton* m_changePasswordButton = nullptr;
    QPushButton* m_deleteAccountButton = nullptr;
    QPushButton* m_logoutButton = nullptr;

    QCheckBox* m_darkModeCheck = nullptr;
    QCheckBox* m_notificationsCheck = nullptr;
    QCheckBox* m_blockInvitesCheck = nullptr;
    QLabel* m_languageLabel = nullptr;
    QComboBox* m_languageCombo = nullptr;
    QLabel* m_updaterTitleLabel = nullptr;
    QLabel* m_updaterStatusLabel = nullptr;
    QPushButton* m_checkUpdateButton = nullptr;
    QPushButton* m_downloadUpdateButton = nullptr;
    QPushButton* m_installUpdateButton = nullptr;
};

#endif // SETTINGSDIALOG_H
