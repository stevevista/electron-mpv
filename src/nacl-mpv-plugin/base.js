class PluginBase {
  constructor (el) {
    this.$el = el
    this._disposables = []
    this._eventHandlers = {}
  }

  dispose () {
    this._disposables.forEach(f => f())
    this._disposables = []
  }

  _pushEventHandler (name, cb) {
    if (!this._eventHandlers[name]) {
      this._eventHandlers[name] = [cb]
    } else {
      this._eventHandlers[name].push(cb)
    }
  }

  _removeEventHandler (name, cb) {
    if (this._eventHandlers[name]) {
      this._eventHandlers[name] = this._eventHandlers[name].filter(h => h !== cb)
      if (!this._eventHandlers[name].length) {
        delete this._eventHandlers[name];
        return 0
      }
      return this._eventHandlers[name].length
    }
    return 0
  }

  _on_prop (name, value) {
    const handlers = this._eventHandlers['prop-change'];
    handlers && handlers.forEach(cb => cb(name, value));
  }

  _on_event (name, ...args) {
    const handlers = this._eventHandlers[name]
    handlers && handlers.forEach(cb => cb(...args));
  }
}

module.exports = PluginBase
