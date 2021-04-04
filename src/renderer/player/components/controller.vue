<template>
  <div class="player-bottom-controller" @contextmenu.stop>
    <!-- left -->
    <div class="player-icons player-icons-left">
      <div class="menu-buttons" v-show="showOpen">
        <icon-button
          icon="folder-open"
          @click.stop="$emit('open-files')"/>
        <icon-button
          icon="file-multiple"
          @click.stop="$emit('open-dir')"
          :tooltip="$t('video.openDir')"/>
        <icon-button
          icon="link-variant"
          @click.stop="$emit('open-stream')"
          :tooltip="$t('video.openUrl')"/>
      </div>
      <icon-button
          :icon="idle ? 'replay' : (pause ? 'play' : 'pause')"
          :disabled="!playlist.length"
          v-if="showPlay"
          @click.stop="player.toggle_play(true)"/>
      <icon-button
          icon="arrow-left-bold-box"
          @click.stop="player.prev_frame"
          :tooltip="$t('prev_frame')"
          :disabled="!showPlay || !playlist.length || idle"
          v-if="showPlay && !isLive"/>
      <icon-button
          icon="arrow-right-bold-box"
          @click.stop="player.next_frame"
          :tooltip="$t('next_frame')"
          :disabled="!showPlay || !playlist.length || idle"
          v-if="showPlay && !isLive"/>
      <icon-button
        :icon="repeatIcon"
        @click.stop="circleRepeat"
        :tooltip="$t('video.' + repeatIcon)"
        v-if="showPlay && !isLive"/>
      <VolumeControl
        :mute="mute"
        :volume="volume"
        @notice="(args) => $emit('notice', args)"
        @toggle-mute="$emit('toggle-mute')"
        @set-volume="(v, save) => $emit('set-volume', v, save)"
      />
      <span class="player-time">
        {{ secondToTime(timePos) }} / {{ secondToTime(duration) }}
      </span>
    </div>
    <!-- right -->
    <div class="player-icons player-icons-right">
      <icon-button
          icon="information-outline"
          :tooltip="$t('video.info')"
          @click.stop="$emit('toggle-info')"/>
      <icon-button
          icon="invert-colors"
          :tooltip="$t('equalizer')"
          @click.stop="$emit('toggle-effects')"/>
      <icon-button
          icon="face-recognition"
          :tooltip="$t('video.detection')"
          :inactive="!$store.settings.player.hotDetection"
          @click.stop="$store.togglePlayerSetting('hotDetection')"/>
      <GroupScreenshot
        :disabled="idle"
        :working="snapshotting"
        @screenshot="$emit('screenshot')"
        @screenshot-multi="$emit('multi-screenshot')"
        @screenshot-stop="$emit('multi-screenshot')"
      />
      <icon-button
          :icon="fullscreen ? 'fullscreen-exit' : 'crop-free'"
          @click.stop="player.property('fullscreen', !fullscreen)"
          :tooltip="$t('video.Full_screen')"/>
    </div>
    <!-- play bar -->
    <div class="player-bar-wrap" ref="playedBarWrap" v-show="!isLive">
      <div class="player-bar-time hidden">00:00</div>
      <div class="player-bar-preview"></div>
      <div class="player-bar">
        <div class="player-played" :style="`width: ${playedPercentage*100}%`">
          <span class="player-thumb" :style="`background: #b7daff`"></span>
        </div>
      </div>
    </div>
  </div>
</template>

<script>
/* eslint-disable no-template-curly-in-string */
/**
  * optimize control play progress

  * optimize get element's view position,for float dialog video player
  */
import IconButton from './icon-button'
import GroupScreenshot from './group-screenshot'
import VolumeControl from './volume-control'

function getBoundingClientRectViewLeft (element) {
  const scrollTop = window.scrollY || window.pageYOffset || document.body.scrollTop + ((document.documentElement && document.documentElement.scrollTop) || 0);

  if (element.getBoundingClientRect) {
    if (typeof getBoundingClientRectViewLeft.offset !== 'number') {
      let temp = document.createElement('div');
      temp.style.cssText = 'position:absolute;top:0;left:0;';
      document.body.appendChild(temp);
      getBoundingClientRectViewLeft.offset = -temp.getBoundingClientRect().top - scrollTop;
      document.body.removeChild(temp);
      temp = null;
    }
    const rect = element.getBoundingClientRect();
    const offset = getBoundingClientRectViewLeft.offset;

    return rect.left + offset;
  } else {
    // not support getBoundingClientRect
    return getElementViewLeft(element);
  }
}

/**
 * control play progress
 */
// get element's view position
function getElementViewLeft (element) {
  let actualLeft = element.offsetLeft;
  let current = element.offsetParent;
  const elementScrollLeft = document.body.scrollLeft + document.documentElement.scrollLeft;
  if (!document.fullscreenElement && !document.mozFullScreenElement && !document.webkitFullscreenElement) {
    while (current !== null) {
      actualLeft += current.offsetLeft;
      current = current.offsetParent;
    }
  } else {
    while (current !== null && current !== element) {
      actualLeft += current.offsetLeft;
      current = current.offsetParent;
    }
  }
  return actualLeft - elementScrollLeft;
}

export default {
  props: {
    showOpen: {default: false, type: Boolean},
    showPlay: {default: false, type: Boolean},
    snapshotting: {default: false, type: Boolean},
    idle: {default: true, type: Boolean},
    pause: {default: true, type: Boolean},
    playlist: {type: Array, default: () => []},
    timePos: {default: 0, type: Number},
    duration: {default: 0, type: Number},
    fullscreen: {default: false, type: Boolean},
    isLive: {default: false, type: Boolean},
    mute: {type: Boolean, default: false},
    volume: {type: Number, default: 100}
  },
  components: {
    IconButton,
    GroupScreenshot,
    VolumeControl
  },
  data () {
    return {
      repeat: 0
    }
  },
  computed: {
    playedPercentage () {
      return this.duration > 0 ? (this.timePos + 2 >= this.duration ? 1 : this.timePos / this.duration) : 0;
    },
    repeatIcon () {
      if (this.repeat === 1) {
        return 'repeat-once'
      } else if (this.repeat === 2) {
        return 'repeat'
      } else if (this.repeat === 3) {
        return 'playlist-music'
      } else {
        return 'repeat-off'
      }
    }
  },
  mounted () {
    this.player = document.getElementById('the-player');
  
    this.playedBarWrapEl = this.$refs.playedBarWrap;
    this.playedBarTimeEl = this.playedBarWrapEl.querySelector('.player-bar-time');

    this.initPlayedBar();
  },
  methods: {
    initPlayedBar () {
      const thumbMove = (e) => {
        if (this.duration > 0) {
          let percentage = ((e.clientX || e.changedTouches[0].clientX) - getBoundingClientRectViewLeft(this.playedBarWrapEl)) / this.playedBarWrapEl.clientWidth;
          percentage = Math.max(percentage, 0);
          percentage = Math.min(percentage, 1);
          this.timePos = percentage * this.duration
        }
      };

      const thumbUp = (e) => {
        document.removeEventListener('mouseup', thumbUp);
        document.removeEventListener('mousemove', thumbMove);
        let percentage = ((e.clientX || e.changedTouches[0].clientX) - getBoundingClientRectViewLeft(this.playedBarWrapEl)) * 100 / this.playedBarWrapEl.clientWidth;
        percentage = Math.max(percentage, 0);
        percentage = Math.min(percentage, 100);
        this.player.seekPercent(percentage);
        this.$store._dragging = false;
      };

      this.playedBarWrapEl.addEventListener('mousedown', () => {
        if (this.idle) {
          return
        }
        this.$store._dragging = true;
        document.addEventListener('mousemove', thumbMove);
        document.addEventListener('mouseup', thumbUp);
      });

      this.playedBarWrapEl.addEventListener('mousemove', (e) => {
        if (this.duration > 0) {
          const px = this.playedBarWrapEl.getBoundingClientRect().left;
          const tx = (e.clientX || e.changedTouches[0].clientX) - px;
          if (tx < 0 || tx > this.playedBarWrapEl.offsetWidth) {
            return;
          }
          const time = this.duration * (tx / this.playedBarWrapEl.offsetWidth);
          this.playedBarTimeEl.style.left = `${tx - (time >= 3600 ? 25 : 20)}px`;
          this.playedBarTimeEl.innerText = this.secondToTime(time);
          this.playedBarTimeEl.classList.remove('hidden');
        }
      });

      this.playedBarWrapEl.addEventListener('mouseenter', () => {
        if (this.duration > 0) {
          this.playedBarTimeEl.classList.remove('hidden');
        }
      });

      this.playedBarWrapEl.addEventListener('mouseleave', () => {
        if (this.duration > 0) {
          this.playedBarTimeEl.classList.add('hidden');
        }
      });
    },

    /**
     * Parse second to time string
     *
     * @param {Number} second
     * @return {String} 00:00 or 00:00:00
     */
    secondToTime (second) {
      second = second || 0;
      if (second === 0 || second === Infinity || second < 0 || second.toString() === 'NaN') {
        return '00:00';
      }
      const add0 = (num) => (num < 10 ? '0' + num : '' + num);
      const hour = Math.floor(second / 3600);
      const min = Math.floor((second - hour * 3600) / 60);
      const sec = Math.floor(second - hour * 3600 - min * 60);
      return (hour > 0 ? [hour, min, sec] : [min, sec]).map(add0).join(':');
    },

    circleRepeat () {
      if (this.repeat >= 2) {
        this.repeat = 0;
      } else {
        this.repeat++;
      }
      if (this.repeat === 0) {
        this.player.option('loop-file', 'no');
        this.player.option('loop-playlist', 'no');
      } else if (this.repeat === 1) {
        this.player.option('loop-file', 'inf');
      } else if (this.repeat === 2) {
        this.player.option('loop-file', 'no');
        this.player.option('loop-playlist', 'inf');
      }
    }
  }
}
</script>

<style lang="scss" scoped>
@import '../styles.scss';

.player-bottom-controller {
  .player-icons {
    @include player-buttons-style;
  }

  .player-bar-wrap {
        padding: 15px 0;
        cursor: pointer;
        position: absolute;
        bottom: 33px;
        width: calc(100% - 40px);
        height: 3px;
        &:hover {
            .player-bar .player-played .player-thumb {
                transform: scale(1);
            }
        }
        .player-bar-preview {
            position: absolute;
            background: #fff;
            pointer-events: none;
            display: none;
            background-size: 16000px 100%;
        }
        .player-bar-preview-canvas {
            position: absolute;
            width: 100%;
            height: 100%;
            z-index: 1;
            pointer-events: none;
        }
        .player-bar-time {
            &.hidden {
                opacity: 0;
            }
            position: absolute;
            left: 0px;
            top: -20px;
            border-radius: 4px;
            padding: 5px 7px;
            background-color: rgba(0, 0, 0, 0.62);
            color: #fff;
            font-size: 12px;
            text-align: center;
            opacity: 1;
            transition: opacity .1s ease-in-out;
            word-wrap: normal;
            word-break: normal;
            z-index: 2;
            pointer-events: none;
        }
        .player-bar {
            position: relative;
            height: 3px;
            width: 100%;
            background: rgba(255, 255, 255, .2);
            cursor: pointer;
            .player-played {
              background: #b7daff;
              position: absolute;
                left: 0;
                top: 0;
                bottom: 0;
                height: 3px;
                will-change: width;
                .player-thumb {
                    position: absolute;
                    top: 0;
                    right: 5px;
                    margin-top: -4px;
                    margin-right: -10px;
                    height: 11px;
                    width: 11px;
                    border-radius: 50%;
                    cursor: pointer;
                    transition: all .3s ease-in-out;
                    transform: scale(0);
                }
            }
    }
  }
}
</style>
