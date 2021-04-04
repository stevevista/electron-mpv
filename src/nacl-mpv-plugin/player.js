const PluginBase = require('./base')

//
// Events:
const DEFAULT_TIMEOUTS = 10000

class MPVPlayer extends PluginBase {
  constructor (el) {
    super(el)

    this.next_oid = 1
    this.observers = {}; // items of id: fn
    this.next_gid = 1
    this.resolvers = {}; // items of id: {resolve, reject}
    this.next_assid = 1;
    this.hooks = []; // array of callbacks, id is index+1

    this.props = {
      fullscreen: false,
      mute: false,
      volume: 100,
      pause: false,
      'idle-active': true,
      'playlist-count': 0,
      'options/osd-font': 'sans-serif',
      'options/osd-font-size': 55,
      'options/osd-border-size': 3,
      'options/osd-duration': 1000
    }

    this.ignoreBinding = false
    
    this.install()
    this.init()
  }

  install () {
    const onMessage = ({data: e}) => {
      const handlers = this._eventHandlers[e.event];
      handlers && handlers.forEach(cb => cb(e));
    };

    const onKeyDown = this.handleKeyDown.bind(this)

    document.addEventListener('keydown', onKeyDown, false);
    this._disposables.push(() => {
      document.removeEventListener('keydown', onKeyDown, false)
    })

    this.$el.addEventListener('message', onMessage);
    this._disposables.push(() => {
      this.$el.removeEventListener('message', onMessage)
    })
  }

  onPropertyChangeDefault (name, value) {
    if (name === 'fullscreen') {
      if (value) {
        this.$el.webkitRequestFullscreen();
      } else {
        document.webkitExitFullscreen();
      }
    }
    this.props[name] = value
  }

  init () {
    [
      'video',
      'width',
      'height',
      'pause',
      'speed',
      'time-pos',
      'duration',
      'eof-reached',
      'filename',
      'path',
      'file-size',
      'file-format',
      'mute',
      'volume',
      'osd-dimensions',
      'fullscreen',
      'idle-active',
      'media-title',
      'playlist-pos',
      'playlist-count',
      'playlist',
      'video-codec',
      'audio-codec-name',
      'estimated-vf-fps',
      'estimated-frame-count',
      'hwdec-current',
      'options/demuxer-lavf-hacks',
      'options/demuxer-lavf-o' // {auth: user:pass}
    ].forEach(name => {
      this.observe_property(name)
    });

    this.register_event('property-change', e => {
      if (e.data === undefined) {
        return
      }
      const cb = this.observers[e.id];
      if (cb) {
        cb(e.name, e.data);
      } else if (e.id === undefined) {
        this.onPropertyChangeDefault(e.name, e.data)
        this._on_prop(e.name, e.data)
      }
    });

    this.register_event('get-property-reply', e => {
      if (!e.error) {
        this.onPropertyChangeDefault(e.name, e.data)
      }

      this._on_resolve(e, e => e.data)
    })

    this.register_event('command-reply', e => {
      this._on_resolve(e, e => e.result)
    })

    this.register_event('set-property-reply', e => {
      this._on_resolve(e, e => e.result)
    })

    this.register_event('hook', e => {
      const mp = this
      let state = 0; // 0:initial, 1:deferred, 2:continued
      function do_cont () {
        state = 2
        mp._postData('hook_continue', e.hook_id);
        return true
      }

      function err () { console.error('hook already continued') }
      function defer () {
        if (state === 2) {
          return err()
        }
        state = 1
        return true
      }
      function cont () { return state === 2 ? err() : do_cont() }

      const cb = e.id > 0 && this.hooks[e.id - 1];
      if (cb) {
        const prom = cb({ defer, cont });
        if (prom && typeof prom.finally === 'function') {
          return prom.finally(() => {
            return state === 0 ? do_cont() : true;
          })
        }
      }
      return state === 0 ? do_cont() : true;
    });

    // rtsp auth
    this.add_hook('on_load_fail', 0, async ({defer, cont}) => {
      const path = this.props['path']
      if (path && path.indexOf('rtsp:') === 0) {
        if (this.props['options/demuxer-lavf-hacks'] === 2) {
          // parse user:pass
          let user = ''
          let pass = ''
          let authinfo = this.props['options/demuxer-lavf-o'] || {}
          authinfo = authinfo['auth'] || ''
          
          const parts = authinfo.split(':')
          if (parts.length === 2 && parts[1][0] !== '/') {
            user = decodeURIComponent(parts[0])
            pass = decodeURIComponent(parts[1])
          }
          if (typeof this.auth_callback === 'function') {
            return this.auth_callback(path, user, pass, ({cancel, username, password}) => {
              if (!cancel && username && password) {
                const codecdauth = `${encodeURIComponent(username)}:${encodeURIComponent(password)}`
                if (authinfo !== codecdauth) {
                  this.load(path, 'replace', {'demuxer-lavf-hacks': 1, 'demuxer-lavf-o-add': `auth=${codecdauth}`});
                }
              }
              cont()
            })
          }
        }
      }

      cont()
    })
  }

  handleKeyDown (e) {
    if (this.ignoreBinding) {
      return
    }

    e.preventDefault();
    if (e.key === 'f' || (e.key === 'Escape' && this.props.fullscreen)) {
      this.toggleFullscreen();
    } else {
      this.keypress(e);
    }
  }

  keypress ({key, shiftKey, ctrlKey, altKey}) {
    // Don't need modifier events.
    if ([
      'Escape', 'Shift', 'Control', 'Alt',
      'Compose', 'CapsLock', 'Meta'
    ].includes(key)) return;

    if (key.startsWith('Arrow')) {
      key = key.slice(5).toUpperCase();
      if (shiftKey) {
        key = `Shift+${key}`;
      }
    }
    if (ctrlKey) {
      key = `Ctrl+${key}`;
    }
    if (altKey) {
      key = `Alt+${key}`;
    }

    // Ignore exit keys for default keybindings settings.
    if ([
      'q', 'Q', 'ESC', 'POWER', 'STOP',
      'CLOSE_WIN', 'CLOSE_WIN', 'Ctrl+c',
      'AR_PLAY_HOLD', 'AR_CENTER_HOLD'
    ].includes(key)) return;

    this.command('keypress', key);
  }

  get_property (name, def) {
    const r = this.props[name]
    if (r === undefined) {
      return def
    }
    return r
  }

  property (name, value) {
    return this._async_to_promise((id) => this._postData('set_property', {name, value}, id), `set property: ${name} timeout`, DEFAULT_TIMEOUTS)
      .catch(e => {
        console.log('---------', name, e)
      })
  }

  option (name, value, postFix) {
    if (postFix) {
      this._postData('set_option', {name: `${name}-${postFix}`, value: value.toString()})
      return
    }
    return this.property(`options/${name}`, value);
  }

  _postData (type, data, id) {
    this.$el.postMessage({type, data, id});
  }

  _async_to_promise (fn, errmsg, timeouts) {
    return new Promise((resolve, reject) => {
      const id = this.next_gid++;
      const timer = setTimeout(() => {
        delete this.resolvers[id]
        reject(new Error(errmsg))
      }, timeouts)
      this.resolvers[id] = {resolve, reject, timer}
      fn(id);
    })
  }

  _on_resolve (e, unwrap) {
    const {resolve, reject, timer} = (this.resolvers[e.id] || {})
    if (resolve) {
      delete this.resolvers[e.id]
      clearTimeout(timer)
      if (e.error) {
        reject(new Error(e.error))
      } else {
        resolve(unwrap(e))
      }
    }
  }

  command (cmd, ...args) {
    const timeouts = cmd._timeout || DEFAULT_TIMEOUTS
    const command = cmd.name || cmd

    if (typeof cmd === 'string') {
      args = args.map(arg => arg.toString());
      cmd = [cmd].concat(args)
    } else {
      delete cmd._timeout
      cmd = JSON.stringify(cmd)
    }

    return this._async_to_promise((id) => this._postData('command', cmd, id), `${command} timeout`, timeouts)
  }

  on (name, cb) {
    return this.register_event(name, cb)
  }

  off (name, fn) {
    if (this._removeEventHandler(name, fn) === 0) {
      if (name !== 'prop-change') {
        this._postData('request_event', {event: name, enable: false});
      }
    }
  }

  register_event (name, cb) {
    this._pushEventHandler(name, cb)

    if (name !== 'prop-change') {
      // special event
      // prop-change is not really player event
      this._postData('request_event', {event: name, enable: true});
    }
  }

  observe_property (name, type, fn) {
    let id
    if (typeof type === 'function') {
      fn = type
    }
  
    if (fn) {
      id = this.next_oid++;
      this.observers[id] = fn;
    }
    this._postData('observe_property', name, id);
  }

  unobserve_property (fn) {
    for (const id in this.observers) {
      if (this.observers[id] === fn) {
        delete this.observers[id];
        this._postData('unobserve_property', id);
      }
    }
  }

  get_property_async (name, timeouts = DEFAULT_TIMEOUTS) {
    return this._async_to_promise((id) => this._postData('get_property_async', name, id), `get property: ${name} timeout`, timeouts)
  }

  add_hook (name, pri, fn) {
    this.hooks.push(fn);
    // 50 (scripting docs default priority) maps to 0 (default in C API docs)
    this._postData('hook_add', {name, priority: pri - 50, id: this.hooks.length});
  }

  create_osd_overlay (format = 'ass-events') {
    const mp = this
    return {
      format,
      id: this.next_assid++,
      data: '',
      res_x: 0,
      res_y: 720,
      z: 0,

      update () {
        const cmd = {}; // shallow clone of `this', excluding methods
        for (const k in this) {
          if (typeof this[k] !== 'function') {
            cmd[k] = this[k];
          }
        }

        cmd.name = 'osd-overlay';
        cmd.res_x = Math.round(this.res_x);
        cmd.res_y = Math.round(this.res_y);
        mp.command(cmd);
        return true
      },

      remove () {
        mp.command({
          name: 'osd-overlay',
          id: this.id,
          format: 'none',
          data: ''
        });
        return true
      }
    };
  }

  set_osd_ass (res_x, res_y, data) {
    if (!this._legacy_overlay) {
      this._legacy_overlay = this.create_osd_overlay('ass-events');
    }

    var lo = this._legacy_overlay;
    if (lo.res_x === res_x && lo.res_y === res_y && lo.data === data) {
      return true;
    }

    this._legacy_overlay.res_x = res_x;
    this._legacy_overlay.res_y = res_y;
    this._legacy_overlay.data = data;
    return this._legacy_overlay.update();
  }

  get_osd_size () {
    const d = this.props['osd-dimensions'] || {}
    return {
      width: d.w,
      height: d.h,
      scaledWidth: d.w - d.ml - d.mr,
      scaledHeight: d.h - d.mt - d.mb,
      left: d.ml,
      right: d.mr,
      top: d.mt,
      bottom: d.mb,
      aspect: d.aspect
    };
  }

  // wrapped function

  // playlist
  load (source, mode = 'replace', options = {}) {
    if (Array.isArray(source)) {
      return this.load_files(source, mode, options)
    }

    // fix rtsp sync issuse
    if (source.match(/^rtsp:\/\/(.*)/)) {
      if (!options) {
        options = {}
      }
      options['video-sync'] = 'display-desync'
    }

    this.command('loadfile', source, mode, stringfyOptions(options));
  }

  // control
  togglePause () {
    this.command('cycle', 'pause')
  }

  toggleMute () {
    this.command('cycle', 'mute')
  }

  play (pos = 0) {
    if (this.get_property('playlist-count') === 0) {
      return
    }

    const playlistPos = this.get_property('playlist-pos')

    if (pos !== playlistPos) {
      this.property('playlist-pos', pos);
    } else {
      // if mpv is not idle and has files queued just set the pause state to false
      this.property('pause', false);
    }
  }

  async play_async (pos = 0) {
    const count = await this.get_property_async('playlist-count')
    if (count === 0 || pos >= count) {
      return
    }

    const playlistPos = await this.get_property_async('playlist-pos')
    if (pos === playlistPos) {
      return this.property('pause', false);
    }

    await new Promise((resolve, reject) => {
      this._playPromise(resolve, reject)
      this.property('playlist-pos', pos);
    })
  }

  async load_async (source, mode = 'replace', options = undefined) {
    // if the mode is append resolve the promise because nothing
    // will be output by the mpv player
    // checking whether this source can be played or not is done when
    // the source is played
    if (mode === 'append') {
      return this.load(source, mode, options);
    }
    
    const idle = await this.get_property_async('idle-active')

    // if the mode is append-play and there are already songs in the playlist
    // resolve the promise since nothing will be output
    if (mode === 'append-play' && !idle) {
      return this.load(source, mode, options);
    }

    await new Promise((resolve, reject) => {
      this._playPromise(resolve, reject)
      this.load(source, mode, options);
    })
  }

  _playPromise (resolve, reject) {
    let started = false

    function onStartFile (e) {
      started = true
    }

    function onFileLoaded (e) {
      unregister();
      return resolve();
    }

    function onEndFile (e) {
      if (started) {
        unregister();
        return reject(new Error(e.error || 'play error'));
      }
    }

    function unregister () {
      this.off('start-file', onStartFile)
      this.off('file-loaded', onFileLoaded)
      this.off('end-file', onEndFile)
    }
  
    this.on('start-file', onStartFile)
    this.on('file-loaded', onFileLoaded)
    this.on('end-file', onEndFile)
  }

  toggleFullscreen () {
    this.command('cycle', 'fullscreen')
  }

  seekPercent (target) {
    this.seek(target, 'absolute-percent')
  }

  seek (target, flag = 'relative') {
    this.command('seek', target, flag, 'exact')
  }

  scale (factor) {
    this.property('options/video-zoom', Math.log2(factor))
  }

  get_property_bool (name, def) {
    const v = this.get_property(name, def)
    return v && v !== 'no'
  }

  get_property_number (name, def) {
    const v = this.get_property(name, def)
    return +v
  }

  get_property_native (name, def) {
    return this.get_property(name, def)
  }

  commandv (...args) {
    return this.command(...args)
  }

  command_native (...args) {
    return this.command(...args)
  }

  // player interface
  load_files (urls, mode, options) {
    this.load(urls[0], mode === 'append' ? 'append-play' : 'replace', options);
    for (let i = 1; i < urls.length; i++) {
      this.load(urls[i], 'append', options);
    }
  }

  quit () {
    this.property('playlist-pos', -1)
  }

  toggle_play (disable_rtsp) {
    if (this.props['idle-active']) {
      this.play()
    } else {
      // disable rtsp toggle
      if (disable_rtsp && this.props['file-format'].indexOf('rtsp') >= 0) {
        return
      }
      this.togglePause()
    }
  }

  next_frame () {
    return this.command('frame-step')
  }

  prev_frame () {
    return this.command('frame-back-step')
  }

  on_auth_require (cb) {
    this.auth_callback = cb
  }

  // rect: [x0, y0, x1, y1]
  crop (rect) {
    if (!rect) {
      this.scale(1)
      this.option('video-pan-x', 0)
      this.option('video-pan-y', 0)
      this._on_prop('zooming', false)
      return
    }

    if (this.props['idle-active']) {
      return
    }

    const dim = this.get_osd_size()
    let {scaledWidth, scaledHeight, width, height, left, top} = dim
    const par = scaledWidth / scaledHeight
    const dar = width / height
    let w, h
    if (par > dar) {
      w = width
      h = width / par
    } else {
      h = height
      w = height * par 
    }
    const scale = Math.max(scaledWidth / (rect[2] - rect[0]), scaledHeight / (rect[3] - rect[1]))
    let expectDx = (left - rect[0]) / scaledWidth
    let expectDy = (top - rect[1]) / scaledHeight

    this.scale(scale)
    scaledWidth = w * scale
    scaledHeight = h * scale
    expectDx *= scaledWidth
    expectDy *= scaledHeight
    let deltaX = (width - scaledWidth) / 2
    let deltaY = (height - scaledHeight) / 2

    let panX = (expectDx - deltaX) / scaledWidth
    let panY = (expectDy - deltaY) / scaledHeight
    this.option('video-pan-x', panX)
    this.option('video-pan-y', panY)

    this._on_prop('zooming', true)
  }
}

function stringfyOptions (options) {
  return Object.keys(options)
    .map(k => `${k}=${options[k]}`)
    .join(',')
}

module.exports = MPVPlayer
