import { join } from 'path';
import { BrowserView, Menu, app } from 'electron';

const TIRLBAR_HEIGHT = 32;

function buileMenuItems (items, webContents) {
  if (!Array.isArray(items)) {
    return undefined
  }

  return items.map(i => {
    if (i.type === 'separator') {
      return { type: i.type }
    }

    return {
      type: i.type,
      label: i.label,
      sublabel: i.sublabel,
      accelerator: i.accelerator,
      enabled: i.enabled,
      checked: i.checked,
      icon: i.icon && process.env.NODE_ENV !== 'development' ? join(app.getAppPath(), `build/${i.icon}`) : undefined,
      click: i.action ? () => {
        webContents.send('view-open-context-menu', i.action, ...(i.args || []))
      } : undefined,
      submenu: buileMenuItems(i.submenu, webContents)
    }
  })
}

class View {
  constructor (window, url) {
    this.browserView = new BrowserView({
      webPreferences: {
        preload: `${app.getAppPath()}/build/preload.bundle.js`,
        nodeIntegration: false,
        enableRemoteModule: false,
        contextIsolation: true,
        sandbox: true,
        plugins: true,
        nativeWindowOpen: true,
        webSecurity: true,
        javascript: true
      }
    })

    this.webContents.windowId = window.win.id;
    this.window = window;

    if (process.env.NODE_ENV === 'development') {
      this.webContents.openDevTools({ mode: 'detach' });
    }

    this.webContents.addListener('page-title-updated', (e, title) => {
      this.window.updateTitle(this.webContents.id);
    });

    this.webContents.loadURL(url);

    this.browserView.setAutoResize({
      width: true,
      height: true,
      horizontal: false,
      vertical: false
    });
  }

  get webContents () {
    return this.browserView.webContents;
  }

  get title () {
    return this.webContents.getTitle();
  }

  get id () {
    return this.webContents.id;
  }

  destroy () {
    this.browserView.destroy();
    this.browserView = null;
  }

  send (channel, ...args) {
    this.webContents.send(channel, ...args);
  }
}

export default class ViewManager {
  views = new Map();
  selectedId = 0;
  _fullscreen = false;

  get fullscreen () {
    return this._fullscreen;
  }

  set fullscreen (val) {
    this._fullscreen = val;
    this.updateBounds();
  }

  constructor (window) {
    this.window = window;
  
    window.handle('view-select', (e, id) => {
      this.select(id);
    });

    window.handle('views-create', (e, urls) => {
      return urls.map(url => this.create(url));
    });

    window.on('toggle-devtools', (e) => {
      this.views.get(this.selectedId).webContents.toggleDevTools();
    });

    window.on('view-open-context-menu', (e, items, x, y) => {
      if (!this.fullscreen && typeof y === 'number') {
        y = y + TIRLBAR_HEIGHT
      }

      Menu.buildFromTemplate(buileMenuItems(items, e.sender))
        .popup({
          window: this.win,
          callback: () => {
            e.sender.send('view-open-context-menu', '__close__')
          },
          x,
          y
        });
    })

    window.on('resize-height', () => {
      this.updateBounds();
    })
  }

  get selected () {
    return this.views.get(this.selectedId);
  }

  create (url) {
    const view = new View(this.window, url);

    const { webContents } = view.browserView;
    const { id } = view;

    this.views.set(id, view);

    webContents.once('destroyed', () => {
      this.views.delete(id);
    });
  
    return id
  }

  clear () {
    this.window.win.setBrowserView(null);
    this.views.forEach((x) => x.destroy());
  }

  select (id) {
    const { selected } = this;
    const view = this.views.get(id);

    if (!view) {
      return;
    }

    this.selectedId = id;

    if (selected) {
      this.window.win.removeBrowserView(selected.browserView);
    }

    this.window.win.addBrowserView(view.browserView);

    view.webContents.focus();

    this.window.updateTitle(id);

    this.updateBounds();
  }

  updateBounds () {
    const view = this.selected;

    if (!view) return;

    const { width, height } = this.window.win.getContentBounds();
    const y = this.fullscreen ? 0 : TIRLBAR_HEIGHT
    const newBounds = {
      x: 0,
      y,
      width,
      height: height - y
    };
    
    if (newBounds !== view.bounds) {
      view.browserView.setBounds(newBounds);
      view.bounds = newBounds;
    }
  }
}
