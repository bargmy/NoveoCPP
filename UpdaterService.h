#ifndef UPDATERSERVICE_H
#define UPDATERSERVICE_H

#include <QObject>
#include <QString>
#include <QMetaType>

class UpdaterService : public QObject
{
    Q_OBJECT
public:
    enum class Status {
        Idle,
        Checking,
        UpdateAvailable,
        Downloading,
        Downloaded,
        UpToDate,
        Error
    };
    Q_ENUM(Status)

    struct UpdateInfo {
        QString version;
        QString notes;
        QString downloadUrl;
    };

    explicit UpdaterService(QObject* parent = nullptr);

    void setUpdaterExecutable(const QString& updaterExecutablePath);
    void setFeedUrl(const QString& feedUrl);

    Status status() const;
    QString statusText() const;
    UpdateInfo availableUpdate() const;

public slots:
    void checkForUpdates();
    void downloadUpdate();
    void restartAndInstall();

signals:
    void statusChanged(UpdaterService::Status status, const QString& message);
    void updateAvailable(const UpdaterService::UpdateInfo& info);
    void downloadProgress(int percent);
    void readyToInstall();

private:
    bool ensureUpdaterBinary(QString* errorMessage = nullptr) const;
    void setStatus(Status status, const QString& message);
    void parseCheckResponse(const QByteArray& output);
    void parseDownloadResponse(const QByteArray& output);
    QString runUpdaterCommand(const QStringList& args, QByteArray* stdOut, QByteArray* stdErr) const;

    QString m_updaterExecutable;
    QString m_feedUrl;
    Status m_status = Status::Idle;
    QString m_statusText = QStringLiteral("Idle");
    UpdateInfo m_availableUpdate;
};

Q_DECLARE_METATYPE(UpdaterService::UpdateInfo)

#endif // UPDATERSERVICE_H
