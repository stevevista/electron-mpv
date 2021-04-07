// nameing with postfix 'Plugin', to workaround test script without window.require
const PlayerPlugin = require('./player')
const { mime: mimePlayer } = require('./index')

function attrTrue (val) {
  return val === '' ||
    (!!val && val !== '0' && val !== 'false');
}

class BasePlayerElement extends HTMLElement {
  constructor () {
    super()

    const shadowRoot = this.attachShadow({mode: 'closed'});
    shadowRoot.innerHTML = this.constructor._innerHTML

    this._root = shadowRoot

    const el = shadowRoot.querySelector('#player');

    let readyResolvers = []
    let loadError = null
  
    const onReady = (e) => {
      if (e.data && e.data.type === 'ready') {
        el.removeEventListener('message', onReady)

        if (e.data.data) {
          this.$player = new this.constructor._PluginClass(el)
          typeof this._init === 'function' && this._init();
          readyResolvers.forEach(([resolve]) => resolve())
        } else {
          loadError = new Error('load plugin fail')
          readyResolvers.forEach(([, reject]) => reject())
        }

        readyResolvers = []
      }
    }

    this.whenReady = () => new Promise((resolve, reject) => {
      if (this.$player) {
        return resolve();
      } else if (loadError) {
        return reject(loadError);
      }

      readyResolvers.push([resolve, reject])
    })
  
    el.addEventListener('message', onReady)
  }

  attributeChangedCallback (name, oldValue, newValue) {
    this.whenReady()
      .then(() => {
        this._attributeChangedCallback(name, oldValue, newValue)
      })
      .catch(console.error)
  }

  connectedCallback () {
    const isEnableLoading = attrTrue(this.getAttribute('show-loading'))
    // ensure can only once
    if (isEnableLoading && !this._root.querySelector('.xplayer-loading-icon')) {
      const style = document.createElement('style');
      style.textContent = `
        .xplayer-loading-icon {
          position: absolute;
          top: 50%;
          left: 50%;
          margin: -18px 0 0 -18px;
          height: 36px;
          width: 36px;
          pointer-events: none;
        }
        .xplayer-loading-icon svg {
          width: 100%;
          height: 100%;
        }
        .xplayer-loading-icon circle {
          fill: #fff;
          animation: xplayer-loading-dot-fade 0.8s ease infinite;
          opacity: 0;
          transform-origin: 4px 4px;
        }
        @keyframes xplayer-loading-dot-fade {
          0% {
            opacity: 0.7;
            transform: scale(1.2, 1.2);
          }
          50% {
            opacity: 0.25;
            transform: scale(0.9, 0.9);
          }
          to {
            opacity: 0.25;
            transform: scale(0.85, 0.85);
          }
        }
      `;

      const loadingIcon = document.createElement('span');
      loadingIcon.classList.add('xplayer-loading-icon')
      loadingIcon.innerHTML = `
        <svg height="100%" version="1.1" viewBox="0 0 22 22">
          <svg x="7" y="1">
            <circle cx="4" cy="4" r="2"></circle>
          </svg>
          <svg x="11" y="3">
            <circle style="animation-delay: 0.1s;" cx="4" cy="4" r="2"></circle>
          </svg>
          <svg x="13" y="7">
            <circle style="animation-delay: 0.2s;" cx="4" cy="4" r="2"></circle>
          </svg>
          <svg x="11" y="11">
            <circle style="animation-delay: 0.3s;" cx="4" cy="4" r="2"></circle>
          </svg>
          <svg x="7" y="13">
            <circle style="animation-delay: 0.4s;" cx="4" cy="4" r="2"></circle>
          </svg>
          <svg x="3" y="11">
            <circle style="animation-delay: 0.5s;" cx="4" cy="4" r="2"></circle>
          </svg>
          <svg x="1" y="7">
            <circle style="animation-delay: 0.6s;" cx="4" cy="4" r="2"></circle>
          </svg>
          <svg x="3" y="3">
            <circle style="animation-delay: 0.7s;" cx="4" cy="4" r="2"></circle>
          </svg>
        </svg>
      `;

      loadingIcon.style.display = 'none'

      this._root.appendChild(style);
      this._root.appendChild(loadingIcon);

      this.addEventListener('prop-change', (name, value) => {
        if (name === 'loading') {
          loadingIcon.style.display = value ? 'block' : 'none'
        }
      })
    }
  }

  disconnectedCallback () {
    this.stop()
  }

  async load (...args) {
    await this.whenReady()
    return this.$player.load(...args)
  }

  stop () {
    if (!this.pluginIsDestroyed()) {
      this.$player.quit()
    }
  }

  queryProperty (name) {
    if (this.$player) {
      return this.$player.props[name]
    } 
  }

  pluginIsDestroyed () {
    return !this.$player || typeof this.$player.$el.postMessage !== 'function'
  }
}

// replace addEventListener
const transferEvents = [
  'start-file',
  'end-file',
  'file-loaded',
  'detection',
  'record-path',
  'error-msg',
  'screenshot',
  'auth-require',
  'prop-change',
  'message'
]

const originListener = BasePlayerElement.prototype.addEventListener

BasePlayerElement.prototype.addEventListener = function (type, listener, ...args) {
  if (transferEvents.indexOf(type) >= 0) {
    this.whenReady()
      .then(() => {
        if (type === 'auth-require') {
          this.$player.on_auth_require(listener)
        } else if (type === 'message') {
          this.$player.$el.addEventListener(type, listener, ...args)
        } else {
          this.$player.register_event(type, listener)
        }
      })
  } else {
    originListener.call(this, type, listener, ...args)
  }
}

class PlayerElement extends BasePlayerElement {
  static get _innerHTML () {
    return `
      <embed id="player" type="${mimePlayer}" style="pointer-events:none; display: block; width: 100%; height: 100%" />
    `
  }

  static _PluginClass = PlayerPlugin

  // Specify observed attributes so that
  // attributeChangedCallback will work
  static get observedAttributes () {
    return [
      'src',
      'hwaccel',
      'ai-switch',
      'locale',
      'online-detection',
      'screenshot-path',
      'screenshot-format',
      'transport',
      'video-sync',
      'disable-audio',
      'volume',
      'mute'
    ];
  }

  _init () {
    // gloabl option
    this.$player.option('network-timeout', 10)
    this.$player.option('demuxer-lavf-o', 'stimeout=10000000', 'add')
    this.$player.option('osd-font-size', 30)
    this.$player.option('osd-duration', 2000)
    this.$player.option('screenshot-template', `pic-%tY%tm%td-%tH%tM-%ws`)

    const logPath = this.getAttribute('debug-log')
    if (logPath) {
      this.$player.option('log-file', logPath)
    }
  }

  _attributeChangedCallback (name, oldValue, newValue) {
    switch (name) {
      case 'src':
        if (newValue) {
          const url = decodeURIComponent(newValue)
          this.$player.load(url);
        }
        break
      
      case 'locale':
        this._updateAISwitchAndLocale(null, newValue)
        break;
      case 'ai-switch':
        this._updateAISwitchAndLocale(newValue, null)
        break;
      case 'online-detection':
        this.$player.option('hot-detection', attrTrue(newValue))
        break
      case 'screenshot-path':
        if (newValue) {
          this.$player.option('screenshot-directory', newValue)
        }
        break
      case 'screenshot-format':
        if (newValue) {
          this.$player.option('screenshot-format', newValue)
        }
        break
      case 'transport':
        if (newValue !== null) {
          this.$player.option('rtsp-transport', newValue)
        }
        break
      case 'video-sync':
        if (newValue !== null) {
          if (newValue === 'nodelay') {
            this.$player.option('audio-buffer', 0)
            this.$player.option('vd-lavc-threads', 1)
            this.$player.option('cache-pause', false)
            this.$player.option('interpolation', false)
            this.$player.option('stream-buffer-size', '4k')
            this.$player.option('video-sync', 'desync')
            this.$player.option('video-latency-hacks', true)
            this.$player.option('vd-lavc-o', 'flags=low_delay+unaligned')
            this.$player.option('untimed', true)
          } else {
            this.$player.option('audio-buffer', 0.2)
            this.$player.option('vd-lavc-threads', 0)
            this.$player.option('cache-pause', true)
            this.$player.option('interpolation', true)
            this.$player.option('stream-buffer-size', '128k')
            this.$player.option('video-latency-hacks', false)
            this.$player.option('vd-lavc-o', 'flags=unaligned')
            this.$player.option('untimed', false)
            if (newValue === 'audio') {
              this.$player.option('video-sync', 'audio')
            }
          }
        }
        break
      case 'hwaccel':
        if (newValue !== null) {
          if (newValue === 'none' || !newValue) {
            this.$player.option('hwdec', 'no')
          } else {
            this.$player.option('hwdec', 'auto')
          }
        }
        break
      case 'disable-audio':
        this.$player.option('audio', attrTrue(newValue) ? 'no' : 'auto')
        break

      case 'volume': {
        const vol = parseInt(newValue)
        if (!isNaN(vol)) {
          this.$player.property('volume', vol)
        }
        break
      }

      case 'mute': {
        const val = attrTrue(newValue)
        this.$player.property('mute', val)
        break
      }
    }
  }

  _updateAISwitchAndLocale (_aiSwitch, _locale) {
    const aiSwitch = _aiSwitch === null ? this.getAttribute('ai-switch') : _aiSwitch
    const locale = _locale === null ? this.getAttribute('locale') : _locale

    if (aiSwitch === null) {
      return
    }

    if (!aiSwitch) {
      this.$player.option('sub-visibility', false)
    } else {
      this.$player.option('sub-visibility', true)
      if (locale === null) {
        this.$player.option('sub-ass-styles', `${aiSwitch}`)
      } else {
        // include ai locale
        this.$player.option('sub-ass-styles', `${aiSwitch},|${locale}`)
      }
    }
  }

  enableKeys (v) {
    this.whenReady()
      .then(() => {
        this.$player.ignoreBinding = !v
      })
  }

  screenshot (...args) {
    if (args[0] === false) {
    } else {
      const {each, subtitles} = (args[0] || {})
      if (each) {
        const screenshotting = this.$player.props.screenshotting
        this.$player.triggerProp('screenshotting', !screenshotting)
      }
      this.$player.command('osd-auto', 'screenshot', (each ? 'each-frame+' : '') + (subtitles ? 'subtitles' : 'video'))
    }
  }
}

// generate function
[
  'option',
  'property',
  'command',
  'seekPercent',
  'seek',
  'toggle_play',
  'crop',
  'prev_frame',
  'next_frame'
].forEach(name => {
  PlayerElement.prototype[name] = async function (...args) {
    await this.whenReady()
    return this.$player[name](...args)
  }
})

customElements.define('x-mpv', PlayerElement)

/*
------------ local player -----------------
<x-player
  src="string"
  debug-log="string"
  hwaccel="string"
  locale="string"
  ai-switch="string array[a,b,c,...]"
  online-detection="boolean"
  screenshot-path="string"
  screenshot-format="jpeg|png"
  transport="tcp|udp"
  video-sync="audio|video|nodelay"
  disable-audio="boolean"
  volume="int[0~100]"
  mute="boolean"
  show-loading="boolean" # persistant
>
</x-player>

[api]
object.queryProperty(name) -> value or undefined
object.load(src, mode, options)
object.play()
object.stop()
object.option(name, value)
object.property(name, value)
object.command({...})
object.seekPercent(...)
object.seek(...)
object.toggle_play()
object.prev_frame()
object.next_frame()
object.crop([x0, y0, x1, y1])
object.screenshot({each, subtitles} | false)

[events] object.addEventListener(event, listener)
prop-change: (name, value)
start-file: empty
file-loaded: empty
end-file: empty
auth-require: (url)
*/
