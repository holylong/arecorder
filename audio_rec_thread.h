#ifndef AUDIORECORDERTHREAD_H
#define AUDIORECORDERTHREAD_H

#include <QThread>

/// wav header 44
typedef struct {
    /// RIFF chunk id
    uint8_t riffChunkId[4] = {'R', 'I', 'F', 'F'};
    /// RIFF chunk data size, file total len - 8
    uint32_t riffChunkDataSize;

    uint8_t format[4] = {'W','A','V','E'};
    uint8_t fmtChunkId[4]={'f','m','t',' '};
    /// fmt chunk data size, save pcm is 16
    uint32_t fmtChunkDataSize = 16;

    /// audio encode type, 1 pcm, 3 floating point
    uint16_t audioFormat = 1;

    uint16_t numChannels;

    uint32_t sampleRate;

    /// byterate = sampleRate * blockAlign
    uint32_t byteRate;

    /// 一个样本的字节数 = bitsPerSample * numchannels >> 3
    uint16_t blockAlign;

    /// 位深度
    uint16_t bitsPerSample;

    uint8_t dataChunkId[4] = {'d','a','t','a'};

    /// pcm文件长度
    uint32_t dataChunkDataSize;

}WAVHeader;

class AudioRecorderThread : public QThread
{
    Q_OBJECT
public:
    explicit AudioRecorderThread(QObject *parent = nullptr);

    void stop();

    bool IsRecorderRunning() const {return _bRunning;}

protected:
    void run() override;

signals:
    void TimeChangeSignal(qulonglong);
    void ReportMsgSignal(const QString&);

private:
    bool _bRunning{false};
};

#endif // AUDIORECORDERTHREAD_H
