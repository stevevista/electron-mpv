<template>
  <div class="player-volume player-icons" ref="volumeButton" :title="volume">
    <icon-button
      :icon="volumeIcon"
      @click.stop="$emit('toggle-mute')"
    />
    <div class="player-volume-bar-wrap">
            <div class="player-volume-bar">
              <div class="player-volume-bar-inner" :style="`width: ${volumeLength}%`">
                <span class="player-thumb" :style="`background: #b7daff`"></span>
              </div>
            </div>
    </div>
  </div>
</template>

<script>
import IconButton from './icon-button'

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
  components: {
    IconButton
  },

  props: {
    mute: {type: Boolean, default: false},
    volume: {type: Number, default: 100}
  },

  computed: {
    volumeIcon () {
      if (this.mute) {
        return 'volume-mute';
      } else if (this.volume >= 95) {
        return 'volume-high';
      } else if (this.volume > 50) {
        return 'volume-medium';
      } else {
        return 'volume-low';
      }
    },
    volumeLength () {
      return this.mute ? 0 : this.volume
    }
  },

  mounted () {
    this.volumeButton = this.$refs.volumeButton;
    this.volumeBarEl = this.volumeButton.querySelector('.player-volume-bar');
    this.volumeBarWrapEl = this.volumeButton.querySelector('.player-volume-bar-wrap');
    this.initVolumeButton();
  },

  methods: {
    initVolumeButton () {
      const vWidth = 35;

      const volumeMove = (event) => {
        const e = event || window.event;
        const percentage = ((e.clientX || e.changedTouches[0].clientX) - getBoundingClientRectViewLeft(this.volumeBarEl) - 5.5) / vWidth;
        this.volumeChange(percentage);
      };
      const volumeUp = () => {
        document.removeEventListener('mouseup', volumeUp);
        document.removeEventListener('mousemove', volumeMove);
        this.volumeButton.classList.remove('player-volume-active');
      };

      this.volumeBarWrapEl.addEventListener('click', (event) => {
        const e = event || window.event;
        const percentage = ((e.clientX || e.changedTouches[0].clientX) - getBoundingClientRectViewLeft(this.volumeBarEl) - 5.5) / vWidth;
        this.volumeChange(percentage);
      });
      this.volumeBarWrapEl.addEventListener('mousedown', () => {
        document.addEventListener('mousemove', volumeMove);
        document.addEventListener('mouseup', volumeUp);
        this.volumeButton.classList.add('player-volume-active');
      });
    },

    volumeChange (percentage, nostorage) {
      percentage = parseFloat(percentage);
      if (!isNaN(percentage)) {
        percentage = Math.max(percentage, 0);
        percentage = Math.min(percentage, 1);
        this.$emit('notice', `${this.$t('volume')} ${(percentage * 100).toFixed(0)}%`);
        this.$emit('set-volume', parseInt(percentage * 100), !nostorage)
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

    .player-volume {
      position: relative;
      display: inline-block;
      cursor: pointer;
      height: 100%;

      &:hover {
        .player-volume-bar-wrap .player-volume-bar {
                        width: 45px;
        }
        .player-volume-bar-wrap .player-volume-bar .player-volume-bar-inner .player-thumb {
                        transform: scale(1);
        }
      }
      &.player-volume-active {
        .player-volume-bar-wrap .player-volume-bar {
                        width: 45px;
        }
        .player-volume-bar-wrap .player-volume-bar .player-volume-bar-inner .player-thumb {
                        transform: scale(1);
        }
      }
      .player-volume-bar-wrap {
        display: inline-block;
        margin: 0 10px 0 -5px;
        vertical-align: middle;
        height: 100%;
        .player-volume-bar {
          position: relative;
          top: 17px;
          width: 0;
          height: 3px;
          background: #aaa;
          transition: all 0.3s ease-in-out;
          .player-volume-bar-inner {
            background: #b7daff;
            position: absolute;
            bottom: 0;
            left: 0;
            height: 100%;
            transition: all 0.1s ease;
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
  }
}

</style>
