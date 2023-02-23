#ifndef FFMPEG_UTIL_H
#define FFMPEG_UTIL_H


#include <vector>
#include <iostream>
#include <QDebug>
#include <Windows.h>
#include <cstdint>
#include <objbase.h>
#include <strmif.h>
#include <amvideo.h>
#include <dvdmedia.h>
#include <uuids.h>

#if defined(__cplusplus) || defined(c_plusplus)
extern "C"{
    #include <libavdevice/avdevice.h>
    #include <libavformat/avformat.h>
    #include "libavutil/avassert.h"
}
#endif

class FFmpegUtil{
public:
    static std::vector<std::string> ListDevices(){
          std::vector<std::string> vecs;
          AVDeviceInfoList* devlist = nullptr;
          AVDictionary* options = nullptr;
          av_dict_copy(&options, nullptr, 0);
          av_dict_set(&options, "list_devices", "true", 1);
          avdevice_list_input_sources(av_find_input_format("dshow"), "dummy", options, &devlist);
          if (devlist == nullptr)
              printf("devlsit is null\n");
          else
              printf("has dev\n");
          return vecs;
    }

    static char* WideCharToUtf8(wchar_t* w)
    {
        int l = WideCharToMultiByte(CP_UTF8, 0, w, -1, nullptr, 0, nullptr, nullptr);
        char* s = new char[l];
        if (s)
            WideCharToMultiByte(CP_UTF8, 0, w, -1, s, l, nullptr, nullptr);
        return s;
    }

    static std::string UTF8ToGBK(const std::string& strUTF8)
    {
        int len = MultiByteToWideChar(CP_UTF8, 0, strUTF8.c_str(), -1, NULL, 0);
        TCHAR* wszGBK = new TCHAR[len + 1];
        memset(wszGBK, 0, len * 2 + 2);
        MultiByteToWideChar(CP_UTF8, 0, strUTF8.c_str(), -1, wszGBK, len);

        len = WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, NULL, 0, NULL, NULL);
        char* szGBK = new char[len + 1];
        memset(szGBK, 0, len + 1);
        WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, szGBK, len, NULL, NULL);
        std::string strTemp(szGBK);
        delete[]szGBK;
        delete[]wszGBK;
        return strTemp;
    }

    static QVector<QPair<QString, QString>> GetDeviceList()
    {
        IMoniker* m = nullptr;
        QVector<QPair<QString, QString>> devices;

        ICreateDevEnum* devenum = nullptr;
        if (CoCreateInstance(CLSID_SystemDeviceEnum, nullptr, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, reinterpret_cast<void**>(&devenum)) != S_OK)
            return devices;

        IEnumMoniker* classenum = nullptr;
        if (devenum->CreateClassEnumerator(CLSID_AudioInputDeviceCategory, reinterpret_cast<IEnumMoniker**>(&classenum), 0) != S_OK)
            return devices;

        while (classenum->Next(1, &m, nullptr) == S_OK) {
            VARIANT var;
            IPropertyBag* bag = nullptr;
            LPMALLOC coMalloc = nullptr;
            IBindCtx* bindCtx = nullptr;
            LPOLESTR olestr = nullptr;
            char* devIdString = nullptr, * devHumanName = nullptr;

            if (CoGetMalloc(1, &coMalloc) != S_OK)
                goto fail;
            if (CreateBindCtx(0, &bindCtx) != S_OK)
                goto fail;

            // Get an uuid for the device that we can pass to ffmpeg directly
            if (m->GetDisplayName(bindCtx, nullptr, &olestr) != S_OK)
                goto fail;
            devIdString = WideCharToUtf8(olestr);

            // replace ':' with '_' since FFmpeg uses : to delimitate sources
            for (size_t i = 0; i < strlen(devIdString); ++i)
                if (devIdString[i] == ':')
                    devIdString[i] = '_';

            // Get a human friendly name/description
            if (m->BindToStorage(nullptr, nullptr, IID_IPropertyBag, reinterpret_cast<void**>(&bag)) != S_OK)
                goto fail;

            var.vt = VT_BSTR;
            if (bag->Read(L"FriendlyName", &var, nullptr) != S_OK)
                goto fail;
            devHumanName = WideCharToUtf8(var.bstrVal);

            devices += {QString("video=") + QString::fromUtf8(devIdString), QString::fromUtf8(devHumanName)};

        fail:
            if (olestr && coMalloc)
                coMalloc->Free(olestr);
            if (bindCtx)
                bindCtx->Release();
            delete[] devIdString;
            delete[] devHumanName;
            if (bag)
                bag->Release();
            m->Release();
        }
        classenum->Release();

        return devices;
    }
};

#endif // FFMPEG_UTIL_H
