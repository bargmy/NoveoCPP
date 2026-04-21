#include "VoiceAudioBridge.h"

#include <QAudioDeviceInfo>

VoiceAudioBridge::VoiceAudioBridge(QObject* parent)
    : QObject(parent)
{
}

VoiceAudioBridge::~VoiceAudioBridge()
{
    stop();
}

bool VoiceAudioBridge::start()
{
    if (isRunning()) {
        return true;
    }

    const QAudioFormat format = createPcmFormat();
    const QAudioDeviceInfo inputDeviceInfo = QAudioDeviceInfo::defaultInputDevice();
    const QAudioDeviceInfo outputDeviceInfo = QAudioDeviceInfo::defaultOutputDevice();

    if (!inputDeviceInfo.isFormatSupported(format)) {
        emit audioError("Input audio format is not supported.");
        return false;
    }
    if (!outputDeviceInfo.isFormatSupported(format)) {
        emit audioError("Output audio format is not supported.");
        return false;
    }

    m_audioInput.reset(new QAudioInput(inputDeviceInfo, format, this));
    m_audioOutput.reset(new QAudioOutput(outputDeviceInfo, format, this));

    m_inputDevice = m_audioInput->start();
    if (!m_inputDevice) {
        emit audioError("Failed to start microphone capture.");
        stop();
        return false;
    }

    m_outputDevice = m_audioOutput->start();
    if (!m_outputDevice) {
        emit audioError("Failed to start audio output.");
        stop();
        return false;
    }

    connect(m_inputDevice, &QIODevice::readyRead, this, [this]() {
        if (!m_inputDevice) {
            return;
        }
        const QByteArray frame = m_inputDevice->readAll();
        if (!frame.isEmpty()) {
            emit pcmCaptured(frame);
        }
    });

    return true;
}

void VoiceAudioBridge::stop()
{
    if (m_audioInput) {
        m_audioInput->stop();
    }
    if (m_audioOutput) {
        m_audioOutput->stop();
    }
    m_inputDevice = nullptr;
    m_outputDevice = nullptr;
    m_audioInput.reset();
    m_audioOutput.reset();
}

bool VoiceAudioBridge::isRunning() const
{
    return m_audioInput && m_audioOutput && m_inputDevice && m_outputDevice;
}

void VoiceAudioBridge::playPcm(const QByteArray& pcm16le)
{
    if (pcm16le.isEmpty()) {
        return;
    }
    if (!ensureOutputStarted()) {
        return;
    }
    m_outputDevice->write(pcm16le);
}

QAudioFormat VoiceAudioBridge::createPcmFormat() const
{
    QAudioFormat format;
    format.setSampleRate(24000);
    format.setChannelCount(1);
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);
    return format;
}

bool VoiceAudioBridge::ensureOutputStarted()
{
    if (m_outputDevice) {
        return true;
    }

    const QAudioFormat format = createPcmFormat();
    const QAudioDeviceInfo outputDeviceInfo = QAudioDeviceInfo::defaultOutputDevice();
    if (!outputDeviceInfo.isFormatSupported(format)) {
        emit audioError("Output device does not support required PCM format.");
        return false;
    }

    m_audioOutput.reset(new QAudioOutput(outputDeviceInfo, format, this));
    m_outputDevice = m_audioOutput->start();
    if (!m_outputDevice) {
        emit audioError("Failed to start output audio stream.");
        return false;
    }
    return true;
}
