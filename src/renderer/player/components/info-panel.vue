<template>
  <div class="xplayer-info-panel" v-show="show">
    <div class="xplayer-info-panel-close" @click="$emit('update:show', false)">[x]</div>
    <div class="xplayer-info-panel-item">
      <span class="xplayer-info-panel-item-title">{{ $t('video.source_path') }}</span>
      <span class="xplayer-info-panel-item-data">{{ shortUrl }}</span>
    </div>
    <div class="xplayer-info-panel-item">
      <span class="xplayer-info-panel-item-title">{{ $t('video.format') }}</span>
      <span class="xplayer-info-panel-item-data">{{ format }} {{ isNetworkUrl ? `[${$store.settings.player.streamingType}]` : '' }}</span>
    </div>
    <div class="xplayer-info-panel-item" v-show="size">
      <span class="xplayer-info-panel-item-title">{{ $t('video.source_size') }}</span>
      <span class="xplayer-info-panel-item-data">{{ sizeDisplay }}</span>
    </div>
    <div class="xplayer-info-panel-item" v-show="width && height">
      <span class="xplayer-info-panel-item-title">{{ $t('video.resolution') }}</span>
      <span class="xplayer-info-panel-item-data">{{ width }} x {{ height }}</span>
    </div>
    <div class="xplayer-info-panel-item" v-show="fps">
      <span class="xplayer-info-panel-item-title">{{ $t('video.fps') }}</span>
      <span class="xplayer-info-panel-item-data">{{ fps && fps.toFixed(2) }}</span>
    </div>
    <div class="xplayer-info-panel-item" v-show="drops">
      <span class="xplayer-info-panel-item-title">{{ $t('video.drops') }}</span>
      <span class="xplayer-info-panel-item-data">{{ drops && drops.toFixed(2) }}</span>
    </div>
    <div class="xplayer-info-panel-item" v-show="duration > 0">
      <span class="xplayer-info-panel-item-title">{{ $t('video.duration') }}</span>
      <span class="xplayer-info-panel-item-data">{{ formatDuration(duration) }}</span>
    </div>
    <div class="xplayer-info-panel-item">
      <span class="xplayer-info-panel-item-title">{{ $t('video.video') }}</span>
      <span class="xplayer-info-panel-item-data">{{ videoc }}</span>
    </div>
    <div class="xplayer-info-panel-item">
      <span class="xplayer-info-panel-item-title">{{ $t('audio') }}</span>
      <span class="xplayer-info-panel-item-data">{{ audioc }}</span>
    </div>
    <div class="xplayer-info-panel-item" v-show="sync">
      <span class="xplayer-info-panel-item-title">{{ $t('sync-type') }}</span>
      <span class="xplayer-info-panel-item-data">{{ sync }}</span>
    </div>
    <div class="xplayer-info-panel-item" v-show="hwdec">
      <span class="xplayer-info-panel-item-title">{{ $t('video.hwaccel') }}</span>
      <span class="xplayer-info-panel-item-data">{{ hwdec }}</span>
    </div>
  </div>
</template>

<script>
function filesize (bytes) {
  const options = {}

  options.calculate = function () {
    const type = ['K', 'B']
    const magnitude = (Math.log(bytes) / Math.log(1024)) | 0
    const result = (bytes / Math.pow(1024, magnitude))
    const fixed = result.toFixed(2)

    const suffix = magnitude
      ? (type[0] + 'MGTPEZY')[magnitude - 1] + type[1]
      : ((fixed | 0) === 1 ? 'Byte' : 'Bytes')

    return {
      suffix,
      result,
      fixed
    }
  }

  options.to = function (unit) {
    let position = ['B', 'K', 'M', 'G', 'T'].indexOf(typeof unit === 'string' ? unit[0].toUpperCase() : 'B')
    var result = bytes

    if (position === -1 || position === 0) return result.toFixed(2)
    for (; position > 0; position--) result /= 1024
    return result.toFixed(2)
  }

  options.human = function () {
    var output = options.calculate()
    return output.fixed + ' ' + output.suffix
  }

  return options;
}

export default {
  props: {
    show: {
      type: Boolean,
      default: false
    },
    path: {type: String, default: ''},
    format: {type: String, default: ''},
    size: {type: Number},
    width: {type: Number},
    height: {type: Number},
    fps: {type: Number},
    drops: {type: Number},
    duration: {type: Number, default: -1},
    videoc: {type: String, default: ''},
    audioc: {type: String, default: ''},
    hwdec: {type: String, default: ''},
    sync: {type: String, default: ''}
  },

  computed: {
    shortUrl () {
      if (!this.path) {
        return ''
      }
      if (/^(rtsp|http|udp|tcp):\/\//.test(this.path)) {
        if (this.path.length < 38) {
          return this.path
        }
        return this.path.slice(0, 38) + ' ...';
      }
      // show only filename
      const group = this.path.split('\\');
      return group.pop();
    },
    isNetworkUrl () {
      return this.path && /^(rtsp|http|udp|tcp):\/\//.test(this.path);
    },
    sizeDisplay () {
      return filesize(this.size).human();
    }
  },

  methods: {
    formatDuration (secs) {
      if (typeof secs !== 'number' || isNaN(secs) || secs === Infinity || secs < 0) {
        return ''
      }
      function align (s) {
        return s < 10 ? '0' + s : String(s)
      }
      secs = Math.floor(secs);
      let mins = Math.floor(secs / 60);
      let hours = Math.floor(mins / 60);
      secs %= 60;
      mins %= 60;
      return align(hours) + ':' + align(mins) + ':' + align(secs)
    }
  }
}
</script>

<style lang="scss" scoped>
@import '../styles.scss';

.xplayer-info-panel {
  @include player-info-panel-style;
}
</style>
