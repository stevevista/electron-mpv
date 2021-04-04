# mpv embeded in web, using Yolov4 tiny model as detection

![Screenshot](/screen.png)

# Web Component Tag <x-mpv>

```html
<body>
  <x-mpv 
    id="player"
    src=""
    hwaccel="auto"
    locale="en-US"
    online-detection
    screenshot-path=""
    screenshot-format="png"
    transport="udp"
    video-sync="audio"
    disable-audio="false"
    volume="60"
    mute="false"
    show-loading
  >
  </x-mpv>

  <script>
    const el = document.getElementByID('player')
    el.addEventListener('start-file', () => {
      console.log('ready to play')
    })

    e.setAttribute('src', 'E:\\test.mp4')
  </script>
</body>
```

# Credits
- [mpv] https://github.com/mpv-player/mpv
- [FFmpeg] https://github.com/FFmpeg/FFmpeg
- [mpv.js] https://github.com/Kagami/mpv.js
- [darknet] https://github.com/AlexeyAB/darknet

# Requirements:
  - Windows 10 x64 (Not tested on Non-windows system or 32bit machine)
  - CMake 3.9 or ablove
  - Visual Studio 2017 or ablove
  - nodejs 12 or ablove
  - python3
  - [nacl sdk] (https://developer.chrome.com/docs/native-client/sdk/download/)


# NACL SDK:
* download nacl_sdk

## Prebuilt dev libraries:
* FFmpeg: https://github.com/BtbN/FFmpeg-Builds/releases/download/autobuild-2021-03-21-12-59/ffmpeg-n4.3.2-160-gfbb9368226-win64-lgpl-shared-4.3.zip

* https://github.com/ShiftMediaProject/libass/releases/download/0.15.0/libass_0.15.0_msvc15.zip

* https://github.com/ShiftMediaProject/SDL/releases/download/release-2.0.14/libsdl_release-2.0.14_msvc15.zip

* https://github.com/ShiftMediaProject/zlib/releases/download/v1.2.11-3/libzlib_v1.2.11-3_msvc15.zip

UNZIP PREBUILT LIBRARIES TO <SRC>/.third-party/prebuilt

COPY OPENGL HEADERS TO <SRC>/.third-party/prebuilt/include


## YOLOv4:
* **source code:** https://github.com/AlexeyAB/darknet
* **pretranied model:** * [yolov4-tiny.cfg](https://raw.githubusercontent.com/AlexeyAB/darknet/master/cfg/yolov4-tiny.cfg) - **40.2% mAP@0.5 - 371(1080Ti) FPS / 330(RTX2070) FPS** - 6.9 BFlops - 23.1 MB: [yolov4-tiny.weights](https://github.com/AlexeyAB/darknet/releases/download/darknet_yolo_v4_pre/yolov4-tiny.weights)

## Compile & Run
```sh
npm i
npm run configure:player
npm run build:player
npm run dev
```
