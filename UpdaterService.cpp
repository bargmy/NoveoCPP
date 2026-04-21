#include "UpdaterService.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>

UpdaterService::UpdaterService(QObject* parent)
    : QObject(parent)
{
    qRegisterMetaType<UpdaterService::UpdateInfo>("UpdaterService::UpdateInfo");
}

void UpdaterService::setUpdaterExecutable(const QString& updaterExecutablePath)
{
    m_updaterExecutable = updaterExecutablePath;
}

void UpdaterService::setFeedUrl(const QString& feedUrl)
{
    m_feedUrl = feedUrl;
}

UpdaterService::Status UpdaterService::status() const
{
    return m_status;
}

QString UpdaterService::statusText() const
{
    return m_statusText;
}

UpdaterService::UpdateInfo UpdaterService::availableUpdate() const
{
    return m_availableUpdate;
}

void UpdaterService::checkForUpdates()
{
    setStatus(Status::Checking, QStringLiteral("Checking for updates..."));

    QString error;
    if (!ensureUpdaterBinary(&error)) {
        setStatus(Status::Error, error);
        return;
    }
    if (m_feedUrl.isEmpty()) {
        setStatus(Status::Error, QStringLiteral("Update feed URL is not configured."));
        return;
    }

    QByteArray stdOut;
    QByteArray stdErr;
    const QString processError = runUpdaterCommand(
        QStringList() << QStringLiteral("--check") << QStringLiteral("--feed") << m_feedUrl << QStringLiteral("--json"),
        &stdOut,
        &stdErr);

    if (!processError.isEmpty()) {
        setStatus(Status::Error, processError);
        return;
    }

    parseCheckResponse(stdOut);
}

void UpdaterService::downloadUpdate()
{
    if (m_status != Status::UpdateAvailable) {
        setStatus(Status::Error, QStringLiteral("No update is available to download."));
        return;
    }

    QString error;
    if (!ensureUpdaterBinary(&error)) {
        setStatus(Status::Error, error);
        return;
    }

    setStatus(Status::Downloading, QStringLiteral("Downloading update..."));
    emit downloadProgress(0);

    QByteArray stdOut;
    QByteArray stdErr;
    const QString processError = runUpdaterCommand(
        QStringList() << QStringLiteral("--download") << QStringLiteral("--feed") << m_feedUrl << QStringLiteral("--json"),
        &stdOut,
        &stdErr);

    if (!processError.isEmpty()) {
        setStatus(Status::Error, processError);
        return;
    }

    parseDownloadResponse(stdOut);
}

void UpdaterService::restartAndInstall()
{
    if (m_status != Status::Downloaded) {
        setStatus(Status::Error, QStringLiteral("Update has not been downloaded yet."));
        return;
    }

    QString error;
    if (!ensureUpdaterBinary(&error)) {
        setStatus(Status::Error, error);
        return;
    }

    QByteArray stdOut;
    QByteArray stdErr;
    const QString processError = runUpdaterCommand(
        QStringList() << QStringLiteral("--install") << QStringLiteral("--restart"),
        &stdOut,
        &stdErr);

    if (!processError.isEmpty()) {
        setStatus(Status::Error, processError);
        return;
    }
}

bool UpdaterService::ensureUpdaterBinary(QString* errorMessage) const
{
    QString executable = m_updaterExecutable;
    if (executable.isEmpty()) {
#ifdef Q_OS_WIN
        executable = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("noveo-updater.exe"));
#else
        executable = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("noveo-updater"));
#endif
    }

    QFileInfo fi(executable);
    if (!fi.exists() || !fi.isFile()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Updater binary not found: %1").arg(executable);
        }
        return false;
    }
    return true;
}

void UpdaterService::setStatus(Status status, const QString& message)
{
    m_status = status;
    m_statusText = message;
    emit statusChanged(status, message);
}

void UpdaterService::parseCheckResponse(const QByteArray& output)
{
    const QJsonDocument doc = QJsonDocument::fromJson(output);
    if (!doc.isObject()) {
        setStatus(Status::Error, QStringLiteral("Updater returned invalid JSON."));
        return;
    }

    const QJsonObject root = doc.object();
    const bool hasUpdate = root.value(QStringLiteral("available")).toBool(false);
    if (!hasUpdate) {
        setStatus(Status::UpToDate, QStringLiteral("You are on the latest version."));
        return;
    }

    m_availableUpdate.version = root.value(QStringLiteral("version")).toString();
    m_availableUpdate.notes = root.value(QStringLiteral("notes")).toString();
    m_availableUpdate.downloadUrl = root.value(QStringLiteral("downloadUrl")).toString();
    setStatus(Status::UpdateAvailable,
              QStringLiteral("Update available: %1").arg(m_availableUpdate.version.isEmpty()
                                                            ? QStringLiteral("new version")
                                                            : m_availableUpdate.version));
    emit updateAvailable(m_availableUpdate);
}

void UpdaterService::parseDownloadResponse(const QByteArray& output)
{
    const QJsonDocument doc = QJsonDocument::fromJson(output);
    if (!doc.isObject()) {
        setStatus(Status::Error, QStringLiteral("Updater returned invalid download JSON."));
        return;
    }

    const QJsonObject root = doc.object();
    const int percent = root.value(QStringLiteral("percent")).toInt(100);
    emit downloadProgress(percent);

    const bool downloaded = root.value(QStringLiteral("downloaded")).toBool(percent >= 100);
    if (!downloaded) {
        setStatus(Status::Error, QStringLiteral("Update download did not complete."));
        return;
    }

    setStatus(Status::Downloaded, QStringLiteral("Update downloaded. Ready to install."));
    emit readyToInstall();
}

QString UpdaterService::runUpdaterCommand(const QStringList& args, QByteArray* stdOut, QByteArray* stdErr) const
{
    QString executable = m_updaterExecutable;
    if (executable.isEmpty()) {
#ifdef Q_OS_WIN
        executable = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("noveo-updater.exe"));
#else
        executable = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("noveo-updater"));
#endif
    }

    QProcess process;
    process.start(executable, args);
    if (!process.waitForStarted(5000)) {
        return QStringLiteral("Failed to start updater process.");
    }
    if (!process.waitForFinished(120000)) {
        process.kill();
        return QStringLiteral("Updater process timed out.");
    }

    if (stdOut) {
        *stdOut = process.readAllStandardOutput();
    }
    const QByteArray stderrBytes = process.readAllStandardError();
    if (stdErr) {
        *stdErr = stderrBytes;
    }

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        const QString err = QString::fromUtf8(stderrBytes).trimmed();
        return err.isEmpty() ? QStringLiteral("Updater process failed.") : err;
    }
    return QString();
}
