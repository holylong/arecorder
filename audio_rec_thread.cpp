#include "audio_rec_thread.h"
#include "config.h"
#include "audio_manager.h"
#include "ffmpeg_util.h"


#include <QDebug>
#include <QTime>
#include <QFile>

#ifdef _WIN32
#include <Windows.h>
#else
#endif

#if defined(__cplusplus) || defined(c_plusplus)
extern "C"{
    #include <libavdevice/avdevice.h>
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libavutil/avutil.h>
}
#endif

/// 一帧音频的大小(字节) = 通道数 x 采样个数 x 采样位数  2 x 1024 x 2
/// 每秒播放的音频字节大小(字节) = 通道数 x 采样个数 x 采样位数  每秒的个数(nb_samples)
/// 2 x nb_samples x 2(s16) = 88200
/// -->> nb_samples = 22500
/// 一帧音频的播放时长 每帧播放时间=1024*1000/44100 = 23.2ms

AudioRecorderThread::AudioRecorderThread(QObject *parent) : QThread(parent)
{
}

void AudioRecorderThread::run(){
    if(!VbConfig::Instance()->_savePcm && !VbConfig::Instance()->_saveWav){
        ReportMsgSignal(tr("ErrorNoSelectOutDir"));
        return;
    }
    _bRunning = true;
    AVFormatContext *ctx = avformat_alloc_context();
    ff_const59 AVInputFormat *fmt = av_find_input_format(FMT_NAME);
    if(!fmt){
        qDebug() << "please input valid fmt name:" <<FMT_NAME;
        return;
    }
    QString audioDev = "audio=";
    audioDev += VbConfig::Instance()->_selectDev;

    AVDictionary* options = nullptr;
    av_dict_set(&options, "ch_layout", "mono", 0);
    char srate[24];
    sprintf(srate, "%d\0", VbConfig::Instance()->_sampleRate.toInt());
    av_dict_set(&options, "sample_rate", srate, 0);

    int ret = avformat_open_input(&ctx, audioDev.toStdString().c_str(), fmt, &options);
    if(ret < 0){
        char errbuf[100];
        av_strerror(ret, errbuf, sizeof(errbuf));
        QString errMsg = "oepn device: ";
        errMsg += VbConfig::Instance()->_selectDev;
        errMsg += "\n";
        errMsg += "error: ";
        errMsg += errbuf;
        errMsg += QString::number(ret);
        qDebug() << errMsg;
        ReportMsgSignal(tr("ErrorOpenAudioInput"));
        
        return;
    }

    QFile fileWav, filePcm;
    WAVHeader header;
    QDateTime time = QDateTime::currentDateTime();

    AVStream* stream = ctx->streams[0];
    AVCodecParameters *params = stream->codecpar;
    AudioManager auManager;
    header.sampleRate = params->sample_rate;

    header.bitsPerSample = av_get_bits_per_sample(params->codec_id);
    header.numChannels = params->channels;
    qDebug() << "byterate:" << header.sampleRate << " channel:" << header.numChannels;
    if(params->codec_id >= AV_CODEC_ID_PCM_F32BE){
        header.audioFormat = 4;
    }

    header.blockAlign = header.bitsPerSample * header.numChannels >> 3;
    //header.blockAlign = params->block_align;
    header.byteRate = header.sampleRate*header.blockAlign;
    header.dataChunkDataSize = 0;

    if(VbConfig::Instance()->_saveWav){
        QString fname = VbConfig::Instance()->_saveDir;
        fname += "/";
        fname += time.toString("MM-dd-HH-mm-ss");
        fname += ".wav";

        fileWav.setFileName(fname);
        if(!fileWav.open(QFile::WriteOnly)){
            qDebug() << "open file error:" <<fname;
            avformat_close_input(&ctx);
            return;
        }
        fileWav.write((char*)&header, sizeof(WAVHeader));
    }

    if(VbConfig::Instance()->_savePcm){
        QString fname = VbConfig::Instance()->_saveDir;
        fname += "/";
        fname += time.toString("MM-dd-HH-mm-ss");
        fname += ".pcm";

        filePcm.setFileName(fname);
        if(!filePcm.open(QFile::WriteOnly)){
            qDebug() << "open file error:" <<fname;
            avformat_close_input(&ctx);
            return;
        }
    }

    AVPacket *pkt = av_packet_alloc();
    qDebug() << "audio recorder start";
    while(!isInterruptionRequested()){
        ret = av_read_frame(ctx, pkt);
        if(ret == 0){

            if(VbConfig::Instance()->_saveWav){
                fileWav.write((const char*)pkt->data, pkt->size);
            }
            if(VbConfig::Instance()->_savePcm){
                filePcm.write((const char*)pkt->data, pkt->size);
            }
            /// data size
            header.dataChunkDataSize += pkt->size;

            /// calculate time
            emit TimeChangeSignal(1000.0 * header.dataChunkDataSize /header.byteRate);
            av_packet_unref(pkt);
        }else if(ret == AVERROR(EAGAIN)){
            continue;
        }else{
            char errbuf[100];
            av_strerror(ret, errbuf, sizeof(errbuf));
            qDebug() << "av_read_frame error:" << errbuf << " " <<ret;
            break;
        }
    }

    av_packet_free(&pkt);

    if(VbConfig::Instance()->_saveWav){
        /// save data chunk
        fileWav.seek(sizeof(WAVHeader) - sizeof(header.dataChunkDataSize));
        fileWav.write((char*)&header.dataChunkDataSize, sizeof(header.dataChunkDataSize));
        header.riffChunkDataSize = fileWav.size() - sizeof(header.riffChunkId) - sizeof(header.riffChunkDataSize);
        /// save riff data
        fileWav.seek(sizeof(header.riffChunkId));
        fileWav.write((char*)&header.riffChunkDataSize, sizeof(header.riffChunkDataSize));
        fileWav.close();
    }

    if(VbConfig::Instance()->_savePcm){
        filePcm.close();
    }

    avformat_close_input(&ctx);
    _bRunning = false;
    qDebug() << "close audio input";
}



void AudioRecorderThread::stop()
{
    _bRunning = false;
}
