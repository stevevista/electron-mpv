import { BrowserWindow, app, ipcMain } from 'electron';
import ViewManager from './views';
import settings from './settings';
import { distFile } from '~/utils/dist'

export default class Window {
  _disposes = []

  constructor () {
    this.win = new BrowserWindow({
      frame: false,
      width: 1248,
      height: 702,
      webPreferences: {
        plugins: false,
        nodeIntegration: false,
        contextIsolation: true,
        javascript: true,
        enableRemoteModule: false,
        preload: `${app.getAppPath()}/build/preload.bundle.js`
      },
      show: false
    })

    this.win.webContents.windowId = this.win.id;

    this.viewManager = new ViewManager(this);
    this.installWindowListeners()

    this.win.show();

    this.win.on('close', (event) => {
      this._disposes.forEach(f => f())
      this.win.setBrowserView(null);
      this.viewManager.clear();
    })

    if (process.env.NODE_ENV === 'development') {
      this.webContents.openDevTools({ mode: 'detach' });
    }
    this.win.loadURL(distFile('app'));
    
    this.win.on('enter-full-screen', () => {
      this.send('fullscreen', true);
      this.viewManager.fixBounds();
    });

    this.win.on('leave-full-screen', () => {
      this.send('fullscreen', false);
      this.viewManager.fixBounds();
    });

    this.win.on('enter-html-full-screen', () => {
      this.viewManager.fullscreen = true;
      this.send('html-fullscreen', true);
    });

    this.win.on('leave-html-full-screen', () => {
      this.viewManager.fullscreen = false;
      this.send('html-fullscreen', false);
    });

    this.win.on('maximize', () => {
      this.send('maximized', true);
    });

    this.win.on('unmaximize', () => {
      this.send('maximized', false);
    });
  }

  get id () {
    return this.win.id;
  }

  get webContents () {
    return this.win.webContents;
  }

  send (channel, ...args) {
    this.webContents.send(channel, ...args);
  }

  // add id parameter
  // fix issue when win is not currently selected
  updateTitle (id) {
    const { selected } = this.viewManager
    if (!selected || selected.id !== id) {
      return
    }

    const { title } = selected;
    this.win.setTitle(
      title.trim() === '' ? app.name : `${title} - ${app.name}`
    );

    this.send('title-updated', title);
  }

  installWindowListeners () {
    this.on('window-inintial', async (e) => {
      await settings.whenLoaded()
      e.sender.send('window-inintial', {
        windowId: this.id,
        settings: settings.object,
        isMaximized: this.win.isMaximized()
      })
    })

    this.on('window-focus', () => {
      this.win.focus();
      this.webContents.focus();
    });

    this.on('window-toggle-maximize', () => {
      if (this.win.isMaximized()) {
        this.win.unmaximize();
      } else {
        this.win.maximize();
      }
    });

    this.on('window-minimize', () => {
      this.win.minimize();
    });
  
    this.on('window-close', () => {
      this.win.close();
    });
  }

  on (channlePrefix, handle) {
    const channel = `${channlePrefix}-${this.id}`
    ipcMain.on(channel, handle)
    this._disposes.push(() => {
      ipcMain.removeListener(channel, handle)
    })
  }

  handle (channlePrefix, handle) {
    const channel = `${channlePrefix}-${this.id}`
    ipcMain.handle(channel, handle)
    this._disposes.push(() => {
      ipcMain.removeHandler(channel)
    })
  }
}
