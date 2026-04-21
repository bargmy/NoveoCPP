#ifndef MEDIAVIEWERDIALOG_H
#define MEDIAVIEWERDIALOG_H

#include <QDialog>
#include <QPixmap>
#include <QUrl>

class QLabel;
class QKeyEvent;
class QMediaPlayer;
class QMouseEvent;
class QNetworkAccessManager;
class QNetworkReply;
class QPushButton;
class QResizeEvent;
class QStackedWidget;
class QVideoWidget;
class QHideEvent;

class MediaViewerDialog : public QDialog
{
    Q_OBJECT
public:
    explicit MediaViewerDialog(QWidget* parent = nullptr);
    ~MediaViewerDialog() override;

    void showImage(const QUrl& url);
    void showVideo(const QUrl& url);
    void clearMedia();

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private:
    void positionCloseButton();
    void loadImageFromUrl(const QUrl& url);
    void updateImageDisplay();

    QStackedWidget* m_stack = nullptr;
    QLabel* m_imageLabel = nullptr;
    QVideoWidget* m_videoWidget = nullptr;
    QPushButton* m_closeButton = nullptr;
    QMediaPlayer* m_videoPlayer = nullptr;
    QNetworkAccessManager* m_nam = nullptr;
    QNetworkReply* m_pendingReply = nullptr;
    QPixmap m_currentImage;
};

#endif // MEDIAVIEWERDIALOG_H
