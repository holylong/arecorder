#ifndef CONFIG_H
#define CONFIG_H

#include <QString>
#include <QFile>
#include <QDir>
#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>

#ifdef Q_OS_WIN
    #define FMT_NAME "dshow"
#elif defined(USING_V4L)
    #define FMT_NAME "video4linux2,v4l2"
#elif defined(Q_OS_OSX)
    #define FMT_NAME "avfoundation"
#endif

class VbConfig{
public:
    static VbConfig* Instance(){
        static VbConfig config;
        return &config;
    }

    void LoadConfig(){
        _path = QDir::currentPath();
        _path += "/vbconfig.json";
        if(CheckExist(_path))
        {
            qDebug() << "vbconfig.json exist";
            QFile file(_path);
            bool ret;
            if((ret = file.open(QIODevice::ReadOnly)) == true)
            {
                qDebug() << "open fileok";
                QByteArray bytes = file.readAll();
                QJsonDocument doc;
                doc = QJsonDocument::fromJson(bytes);
                _obj = doc.object();
                qDebug() << "bytes len:" << bytes.length() << " data:"
                 << bytes.toStdString().c_str() << " " << doc.toJson().toStdString().c_str();
                file.close();

                if(_obj.contains("SaveWav"))_saveWav =_obj.value("SaveWav").toInt();
                if(_obj.contains("SavePcm"))_savePcm = _obj.value("SavePcm").toInt();
                if(_obj.contains("SaveDir"))_saveDir = _obj.value("SaveDir").toString();
                if(_obj.contains("SampleRate"))_sampleRate = _obj.value("SampleRate").toString();
                if(_obj.contains("SelectDev"))_selectDev = _obj.value("SelectDev").toString();
            }
        }else{
            qDebug() << "vbconfig.json not exist";
            _saveDir = QDir::currentPath();
        }
    }

    bool CheckExist(const QString& path){
       QFile file(path);
       if(file.exists())return true;
       else return false;
    }

    void SaveConfig(){
        QFile file(_path);
        bool ret;
        if((ret = file.open(QIODevice::WriteOnly)) == true)
        {
            QByteArray bytes;
            _obj.insert("SaveWav", _saveWav);
            _obj.insert("SavePcm", _savePcm);
            _obj.insert("SaveDir", _saveDir);
            _obj.insert("SampleRate", _sampleRate);
            _obj.insert("SelectDev", _selectDev);
            QJsonDocument doc(_obj);
            bytes = doc.toJson();
            file.write(bytes);
            file.close();
        }
    }

    void UpdateSaveWavType(int wav){
        _saveWav = wav;
        SaveConfig();
    }

    void UpdateSavePcmType(int pcm){
        _savePcm = pcm;
        SaveConfig();
    }


    void UpdateSaveDir(const QString& path){
        _saveDir = path;
        SaveConfig();
    }

    void UpdateSampleRate(const QString& rate)
    {
        _sampleRate = rate;
        SaveConfig();
    }

    void UpdateSelectDev(const QString& dev){
        _selectDev = dev;
        SaveConfig();
    }

public:
    int         _saveWav{0};
    int         _savePcm{1};
    QString     _saveDir{""};
    QString     _sampleRate{"44100"};
    QString     _selectDev{"audio=audiome (Realtek(R) Audio)"};
    QString     _path;
private:
    QJsonObject _obj;
};

#endif // CONFIG_H
