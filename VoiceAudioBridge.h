#ifndef VOICEAUDIOBRIDGE_H
#define VOICEAUDIOBRIDGE_H

#include <QAudioFormat>
#include <QAudioInput>
#include <QAudioOutput>
#include <QIODevice>
#include <QObject>
#include <QScopedPointer>

class VoiceAudioBridge : public QObject
{
    Q_OBJECT
public:
    explicit VoiceAudioBridge(QObject* parent = nullptr);
    ~VoiceAudioBridge() override;

    bool start();
    void stop();
    bool isRunning() const;

public slots:
    void playPcm(const QByteArray& pcm16le);

signals:
    void pcmCaptured(const QByteArray& pcm16le);
    void audioError(const QString& message);

private:
    QAudioFormat createPcmFormat() const;
    bool ensureOutputStarted();

    QScopedPointer<QAudioInput> m_audioInput;
    QScopedPointer<QAudioOutput> m_audioOutput;
    QIODevice* m_inputDevice = nullptr;
    QIODevice* m_outputDevice = nullptr;
};

#endif // VOICEAUDIOBRIDGE_H
