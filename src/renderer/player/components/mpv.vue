<template>
  <v-app>
    <div id="app" class="player-container"
      @contextmenu.stop.prevent="contextmenu" 
      @drop.stop="dropFile"  @dragover.prevent>

      <div
        class="video-wrap" 
        @click.stop="clickScreen"
        :class="{ zooming: !!zoomShape }"
        @mousedown.stop="mousedown"
        @mousemove="mousemove"
        @mouseup="mouseup"
        @mousewheel="mousewheel"
      >
        <x-mpv ref="player"
          id="the-player"
          :src="url"
          :hwaccel="$store.settings.player.hwaccel"
          :locale="$i18n.locale"
          :ai-switch="$store.settings.aiSwitchs"
          :online-detection="$store.settings.player.hotDetection"
          :screenshot-path="$store.settings.screenshotPath"
          :screenshot-format="$store.settings.screenshotFormat"
          :transport="$store.settings.player.streamingType"
          :video-sync="$store.settings.player.syncType"
          :disable-audio="$store.settings.player.disableAudio"
          :volume="volume"
          :mute="mute"
          show-loading
        >
        </x-mpv>
        <div class="player-bezel">
          <span class="player-bezel-play-icon" v-bind:class="{'ended-icon' : idle}" v-show="!isPlaying && !!playlist.length">
            <span :class="`mdi mdi-${idle ? (unauthed ? 'account-lock' : (isLive ? 'link-off' : 'replay')) : 'play'} mdi-light mdi-36px`"></span>
          </span>
        </div>
        <div class="player-crop-selection" :style="zoomShapeStyle"/>
      </div>

      <FFController
        :showOpen="showOpen"
        :showPlay="showPlay"
        :idle="idle"
        :pause="pause"
        :playlist="playlist"
        :snapshotting="screenshotting"
        :timePos="timePos"
        :duration="duration"
        :isLive="isLive"
        :mute="mute"
        :volume="volume"
        @open-files="openFiles"
        @open-stream="toggleAddress"
        @open-dir="openDir"
        @toggle-info="showInfoPanel = !showInfoPanel"
        @screenshot="doSreenshot"
        @multi-screenshot="doMultiSreenshot"
        @toggle-mute="mute=!mute"
        @toggle-effects="dialogEffects=!dialogEffects"
        @set-volume="(v) => volume = v"
        @notice="notice"/>

      <PlayList
        :playlist="playlist"
        v-show="showPlayList"
        :is-show-arrow="isShowArrow"
        @add-items="addNewItems"/>

      <InfoPanel 
        :show.sync="showInfoPanel"
        :path="path"
        :format="fileFormat"
        :size="fileSize"
        :width="width"
        :height="height"
        :fps="fps"
        :drops="drops"
        :duration="duration"
        :videoc="videoCodec"
        :audioc="audioCodec"
        :hwdec="hwdec"
        :sync="$store.settings.player.syncType"
      />

      <div class="player-status">
        <span class="player-cancel-zoom" @click.stop="$player.crop(undefined)" v-show="zooming">
          <v-icon color="white" dense>mdi-arrow-expand</v-icon>{{$t('video.zoom_reset')}}
        </span>
        <span class="player-speed" @click.stop="toggleSpeed(1)" v-show="speed!==1.0">
          <v-icon color="white" dense>mdi-fast-forward</v-icon><v-icon color="white">mdi-numeric-{{speed}}</v-icon>
        </span>
      </div>

      <v-dialog
        v-model="dialogEffects"
        width="500"
        :overlay-opacity="0"
      >
        <CardEqualizer />
      </v-dialog>

      <input type="file" ref="openFile" style="opacity: 0;" accept="video/*,.ps,.mkv" multiple>
      <input type="file" ref="openDir" style="opacity: 0;" accept="video/*,.ps,.mkv" webkitdirectory multiple>
    </div>

    <div class="player-notice" :style="{ opacity: noticeOpacity }">{{ noticeText }}</div>

    <v-dialog v-model="dialogAddress" width="600">
        <v-text-field solo rounded 
          hide-details
          v-model="streamingUri" 
          label="Media URL (HTTP/RTSP/...)" 
          required 
          append-icon="mdi-arrow-right-circle-outline"
          @keydown.enter="onAddressInput"
          @click:append="onAddressInput"></v-text-field>
    </v-dialog>
  </v-app>
</template>

<script>
import FFController from './controller';
import InfoPanel from './info-panel';
import PlayList from './play-list';
import CardEqualizer from './card-equalizer'
import { parse } from 'url'
import { ipcRenderer } from 'electron';
import { createMenuBuilder } from '~/utils/menu'

const allowedExtensions = ['mp4', 'ogg', 'mkv', 'avi', 'mov', 'wmv', '3gp', 'flv', 'f4v', 'h264', 'ps', 'mpeg']

function isAllowedFilename (filename) {
  const flieArr = filename.split('.')
  const suffix = flieArr[flieArr.length - 1];
  return allowedExtensions.indexOf(suffix) >= 0
}

async function traverseFileTree (allowAny, item, out, counts) {
  if (item.isFile) {
    // Get file
    await new Promise(resolve => {
      item.file(function (f) {
        if (allowAny || isAllowedFilename(f.name)) {
          out.push(f.path)
        }
        resolve()
      });
    })
    return 1
  } else if (item.isDirectory) {
    // Get folder contents
    const dirReader = item.createReader();
    await new Promise(resolve => {
      dirReader.readEntries(async (entries) => {
        for (let i = 0; i < entries.length; i++) {
          counts += await traverseFileTree(false, entries[i], out, counts);
          if (counts >= 100) {
            break
          }
        }
        resolve()
      });
    })
    return 0
  }
  return 0
}

export default {
  components: {
    FFController,
    InfoPanel,
    PlayList,
    CardEqualizer
  },

  data () {
    return {
      noticeText: '',
      noticeOpacity: 0,
      dialogAddress: false,
      streamingUri: '',

      zoomingState: 0,
      zoomShape: undefined,
      zoomStartX: 0,
      zoomStartY: 0,
      isDrawing: false,

      openContextAppend: false,
      showOpen: true,
      showPlay: true,
      showPlayList: true,
      url: null,
      showInfoPanel: false,
      isShowArrow: true,
      menuVisible: false,
      dialogEffects: false,
      // player properties
      zooming: false,
      screenshotting: false,
      idle: true,
      pause: false,
      fullscreen: false,
      videoCodec: '',
      audioCodec: '',
      fileSize: 0,
      fileFormat: '',
      filename: '',
      path: '',
      playlist: [],
      fps: 0,
      timePos: 0,
      duration: -1,
      frameCount: 0,
      drops: 0,
      hwdec: '',
      width: 0,
      height: 0,
      speed: 1.0,
      mute: false,
      volume: 100,
      unauthed: false
    }
  },

  computed: {
    isLive () {
      return this.fileFormat === 'rtsp'
    },

    isPlaying () {
      return !this.pause && !this.idle
    },

    zoomShapeStyle () {
      if (!this.zoomShape) {
        return {
          display: 'none'
        }
      }

      return {
        left: this.zoomShape[0] + 'px',
        top: this.zoomShape[1] + 'px',
        width: (this.zoomShape[2] - this.zoomShape[0]) + 'px',
        height: (this.zoomShape[3] - this.zoomShape[1]) + 'px'
      }
    }
  },
  watch: {
    path (v) {
      const xaddr = parse(v)
      if (xaddr && xaddr.host) {
        if (xaddr.protocol === 'ws:' || xaddr.protocol === 'wss:') {
          window.document.title = `Websocket [${xaddr.host}]`;
        } else {
          window.document.title = `RTSP [${xaddr.host}]`;
        }
      } else {
        if (v) {
          // dos local file
          const group = this.path.split('\\');
          window.document.title = group.pop();
        } else {
          window.document.title = this.$t('player');
        }
      }
    },

    dialogAddress (v) {
      this.$player.enableKeys(!v)
    },

    idle (v) {
      if (v) {
        this.zoomingState = 0
        this.zoomShape = undefined
      }
    }
  },

  created () {
    this.menuBuilder = createMenuBuilder(() => {
      setTimeout(() => {
        this.menuVisible = false
      }, 400)
    })

    this.$once('hook:beforeDestory', () => {
      this.menuBuilder.dispose()
    })
  },

  mounted () {
    const el = this.$refs.player;
    this.$player = el

    el.addEventListener('start-file', (e) => {
      // reset
      this.videoCodec = ''
      this.audioCodec = ''
      this.fileSize = 0
      this.unauthed = false
      this.fps = 0
      this.drops = 0
    })
  
    el.addEventListener('auth-require', (url, user, pass, cb) => {
      this.unauthed = true

      this.$store.provideRtspAuth({
        url,
        username: user,
        password: pass
      }, ({username, password}) => {
        if (username && password) {
          cb({username, password})
        } else {
          cb({cancel: true})
        }
      })
    })

    el.addEventListener('prop-change', (name, value) => {
      if (name === 'volume' || name === 'mute') {
        // prevent looping
        return
      }

      if (name === 'track-list') {
        for (const t of value) {
          if (t.codec === 'eia_608') {
            el.option('sid', t.id)
            break
          }
        }
        return
      }
  
      const alias = {
        'file-size': 'fileSize',
        'file-format': 'fileFormat',
        'video-codec': 'videoCodec',
        'audio-codec-name': 'audioCodec',
        'estimated-vf-fps': 'fps',
        'time-pos': 'timePos',
        'estimated-frame-count': 'frameCount',
        'idle-active': 'idle',
        'hwdec-current': 'hwdec'
      }
      const key = alias[name] || name
      this[key] = value
    })

    if (this.$store.settings.player.debug) {
      this.$player.option('log-file', this.$store.settings.userDataPath + '/logs/nivm-player-%t.log')
    } else {
      //  this.$player.option('log-file', null)
    }

    this.$refs.openFile.addEventListener('change', (e) => {
      const files = []
      for (const f of this.$refs.openFile.files) {
        files.push(f.path)
      }

      if (files.length) {
        this.$player.load(files, this.openContextAppend ? 'append' : 'replace');
      }

      // focus to window
      // so to enable key bindings
      ipcRenderer.send('view-focus')
    });

    this.$refs.openDir.addEventListener('change', (e) => {
      let files = []
      for (const f of this.$refs.openDir.files) {
        if (isAllowedFilename(f.name)) {
          files.push(f.path)
        }
      }
      files = files.slice(0, 100)
      if (files.length) {
        this.$player.load(files, this.openContextAppend ? 'append' : 'replace');
      }

      // focus to window
      // so to enable key bindings
      ipcRenderer.send('view-focus')
    });
  },

  methods: {
    contextmenu (e) {
      this.menuBuilder.popup([
        ...(this.zooming ? [
          {
            label: this.$t('video.zoom_reset'),
            click: () => {
              this.$player.crop(undefined)
            }
          },
          {
            type: 'separator'
          }
        ] : []),
        ...(this.showOpen ? [
          {
            label: this.$t('open'),
            submenu: [
              {
                label: this.$t('common.openFile'),
                click: () => {
                  this.openFiles()
                }
              },
              {
                label: this.$t('video.openDir'),
                click: () => {
                  this.openDir()
                }
              },
              {
                label: this.$t('video.openUrl'),
                click: () => {
                  this.toggleAddress()
                }
              }
            ]
          },
          {
            type: 'separator'
          }
        ] : []),
        ...(this.showPlay ? [
          {
            label: this.isPlaying ? this.$t('common.suspend') : this.$t('common.play'),
            enabled: !!this.playlist.length,
            click: () => {
              this.toggle()
            }
          },
          {
            label: this.$t('video.stop'),
            enabled: !this.idle,
            click: () => {
              this.$player.stop()
            }
          },
          {
            type: 'separator'
          },
          {
            label: `${this.$t('video.speed')} X 2`,
            enabled: !this.idle,
            args: [2],
            checked: this.speed === 2,
            type: 'checkbox',
            click: () => {
              this.toggleSpeed(2)
            }
          },
          {
            label: `${this.$t('video.speed')} X 4`,
            enabled: !this.idle,
            args: [4],
            checked: this.speed === 4,
            type: 'checkbox',
            click: () => {
              this.toggleSpeed(4)
            }
          }
        ] : [])
      ])
      this.menuVisible = true
    },

    async doSreenshot () {
      if (await this.$store.ensureScreenshotDirectoryExists()) {
        this.$player.screenshot({subtitles: this.$store.settings.screenshotWithAI})
      }
    },

    async doMultiSreenshot () {
      if (await this.$store.ensureScreenshotDirectoryExists()) {
        this.$player.screenshot({each: true, subtitles: this.$store.settings.screenshotWithAI})
      }
    },

    async openDir (append) {
      this.openContextAppend = append
      this.$refs.openDir.click()
    },

    async openFiles (append) {
      this.openContextAppend = append
      this.$refs.openFile.click()
    },

    async addNewItems (dropItems) {
      if (dropItems) {
        // from drop
        let files = []
        const items = dropItems;
        for (var i = 0; i < items.length; i++) {
          // webkitGetAsEntry is where the magic happens
          var item = items[i].webkitGetAsEntry();
          if (item) {
            await traverseFileTree(items.length === 1, item, files, 0);
          }
        }

        if (files.length) {
          this.$player.load(files, 'append');
        }
      } else {
        this.openFiles(true)
      }
    },

    async dropFile (e) {
      let files = []
      const items = e.dataTransfer.items;
      for (var i = 0; i < items.length; i++) {
        // webkitGetAsEntry is where the magic happens
        var item = items[i].webkitGetAsEntry();
        if (item) {
          await traverseFileTree(items.length === 1, item, files, 0);
        }
      }

      if (files.length) {
        this.$player.load(files);
      }
    },

    toggle () {
      this.$player.toggle_play(true)
    },

    toggleSpeed (rate) {
      this.$player.property('speed', this.speed === rate ? 1.0 : rate)
    },

    notice (text, time = 2000, opacity = 0.8) {
      this.noticeText = text;
      this.noticeOpacity = opacity;
      if (this.noticeTime) {
        clearTimeout(this.noticeTime);
      }
      if (time > 0) {
        this.noticeTime = setTimeout(() => {
          this.noticeOpacity = 0;
        }, time);
      }
    },

    toggleAddress () {
      this.dialogAddress = true;
      this.streamingUri = this.url
    },

    onAddressInput () {
      if (this.streamingUri) {
        this.url = this.streamingUri
      }
      this.dialogAddress = false;
    },

    mousedown (e) {
      if (e.button === 0) {
        e.preventDefault();
        this.zoomingState = 1
        this.zoomStartX = e.offsetX
        this.zoomStartY = e.offsetY
      }
    },

    mousemove (e) {
      if (this.zoomingState !== 1) {
        return
      }
      let x = e.offsetX
      let y = e.offsetY
      const x0 = Math.min(this.zoomStartX, x)
      const y0 = Math.min(this.zoomStartY, y)
      const x1 = Math.max(this.zoomStartX, x)
      const y1 = Math.max(this.zoomStartY, y)
      const w = Math.abs(x1 - x0)
      const h = Math.abs(y1 - y0)

      if (w > 10 && h > 10) {
        this.zoomShape = [x0, y0, x1, y1]
      } else {
        this.zoomShape = undefined
      }
    },

    mouseup (e) {
      if (e.button === 0) {
        e.preventDefault();
        this.zoomingState = 0
        if (this.zoomShape !== undefined) {
          e.stopPropagation();
          this.$player.crop(this.zoomShape)
          this.zoomShape = undefined
          this.isDrawing = true
          setTimeout(() => {
            this.isDrawing = false
          }, 300)
          return false
        }
      }
    },

    mousewheel (e) {
      if (e.ctrlKey) {
        e.preventDefault();
        const {offsetX, offsetY} = e
        const {offsetWidth, offsetHeight} = e.target
        const x0 = offsetX
        const x1 = offsetWidth - offsetX
        const dx = Math.min(x0, x1)
        const dxx = parseInt(dx / 4)
        const y0 = offsetY
        const y1 = offsetHeight - offsetY
        const dy = Math.min(y0, y1)
        const dyy = parseInt(dy / 4)

        const dx0 = x0 === dx ? dxx : (dxx * (x0 / x1))
        const dx1 = x1 === dx ? dxx : (dxx * (x1 / x0))
        const dy0 = y0 === dy ? dyy : (dyy * (y0 / y1))
        const dy1 = y1 === dy ? dyy : (dyy * (y1 / y0))

        if (e.wheelDelta > 0) {
          const zoomShape = [parseInt(dx0), parseInt(dy0), parseInt(offsetWidth - dx1), parseInt(offsetHeight - dy1)]
          this.$player.crop(zoomShape)
        } else if (e.wheelDelta < 0) {
          const zoomShape = [-parseInt(dx0), -parseInt(dy0), parseInt(offsetWidth + dx1), parseInt(offsetHeight + dy1)]
          this.$player.crop(zoomShape)
        } 
      }
    },

    clickScreen (e) {
      if (!this.isDrawing && !this.menuVisible) {
        this.toggle();
      }
    }
  }
}
</script>

<style lang="scss">
#app {
  height: 100vh;
  overflow: hidden;
}

.player-container {
  flex: 1;
  overflow: hidden;
  height: 100%;
  line-height: 1;
  position: relative;
  user-select: none;

  * {
    box-sizing: content-box;
  }

  &:-webkit-full-screen {
    width: 100%;
    height: 100%;
    background: #000;
    position: fixed;
    z-index: 100000;
    left: 0;
    top: 0;
    margin: 0;
    padding: 0;
    transform: translate(0, 0);
  }

  .player-bottom-controller {
    position: absolute;
    bottom: 0;
    left: 0;
    right: 0;
    height: 41px;
    padding: 0 20px;
    user-select: none;
    transition: all 0.3s ease;
  }

  &.player-hide-controller {
    cursor: none;
    .player-bottom-controller {
      opacity: 0;
      transform: translateY(100%);
    }
  }
  &.player-show-controller {
    .player-bottom-controller {
      opacity: 1;
    }
  }
}

.player-container {
  .player-status {
    position: absolute;
    top: 2px;
    right: 2px;
    font-size: 12px;
    span {
      border-radius: 2px;
      padding: 4px 6px;
      color: #fff;
      border:1px dashed;
      transition: all .3s ease-in-out;
      overflow: hidden;
    }
    .player-speed {
      background-color: rgba(255, 0, 0, 0.3);
    }
    .player-cancel-zoom {
      background-color: rgba(128, 0, 128, 0.3);
    }
  }
}

.player-notice {
    opacity: 0;
    position: absolute;
    bottom: 60px;
    left: 20px;
    font-size: 14px;
    border-radius: 2px;
    background: rgba(28, 28, 28, 0.9);
    padding: 7px 20px;
    transition: all .3s ease-in-out;
    overflow: hidden;
    color: #fff;
    pointer-events: none;
}

.zooming {
  cursor: crosshair;
}

.video-wrap {
  height: 100%;
  position: relative;
  font-size: 0;

  .player-bezel {
    position: absolute;
    left: 0;
    right: 0;
    top: 0;
    bottom: 0;
    font-size: 22px;
    color: #fff;
    pointer-events: none;

    .player-bezel-play-icon {
        position: absolute;
        top: 50%;
        left: 50%;
        margin: -30px 0 0 -30px;
        height: 60px;
        width: 60px;
        padding: 13px;
        box-sizing: border-box;
        background: rgba(255, 255, 255, .1);
        border-radius: 50%;
        pointer-events: none;
        &.ended-icon {
          background: rgba(0, 0, 0, 0.7);
        }
    }
  }

  .player-crop-selection {
    position: absolute;
    border-color: #fff;
    border:3px dashed;
    pointer-events:none;
    color: #fff;
  }
}
</style>
