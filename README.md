# recorder for desktop
一个桌面端的小录音机

# build
```
cmake -Bbuild_ninja -DQt5_DIR=<qt5 dir> -DFFMPEG_INC_DIR=<ffmpeg include> -DFFMPEG_LIBS_DIR=<ffmpeg libs path>
cmake --build build_ninja
```

# deps
1.[ffmpeg](https://github.com/ffmpeg/ffmpeg) 抓取音频数据
2.[qt](https://github.com/qt/qt5) 界面
