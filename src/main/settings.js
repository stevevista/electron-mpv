import { ipcMain, nativeTheme, app, shell } from 'electron';
import { promises } from 'fs';
import defaultSettings from './default-settings';
import { getPath } from '~/utils/paths';
import path from 'path'
import windows from './windows-manager';

const CURRENT_SETTINGS_VERSION = 1

class Settings {
  object = defaultSettings;
  loaded = false;
  pendingSaving = 0;
  loadResolvers = []

  constructor () {
    ipcMain.on(
      'save-settings',
      (e, { settings }) => {
        this.object = { ...this.object, ...settings };
        this.addToSaveQueue();
      }
    );

    ipcMain.on('get-settings-sync', (e) => {
      this.whenLoaded(() => {
        e.returnValue = this.object;
      });
    });

    ipcMain.on('open-logs-path', () => {
      shell.openPath(path.join(this.object.userDataPath, 'logs'));
    })

    nativeTheme.on('updated', () => {
      this.update(true);
    });

    this.load();
  }

  whenLoaded = (resolve) => {
    if (typeof resolve === 'function') {
      if (!this.loaded) {
        this.loadResolvers.push(resolve)
      } else {
        resolve();
      }
    } else {
      return new Promise(this.whenLoaded);
    }
  }

  update = () => {
    if (!this.object.locale) {
      let systemLocale = app.getLocale().toLowerCase();
      if (systemLocale.indexOf('en-') === 0) {
        systemLocale = 'en'
      }
      this.object.locale = systemLocale;
    }

    let themeSource = this.object.theme;

    if (themeSource !== nativeTheme.themeSource) {
      nativeTheme.themeSource = themeSource;
    }

    for (const window of windows.list) {
      window.send('update-settings', this.object);

      window.viewManager.views.forEach(async (v) => {
        v.webContents.send('update-settings', this.object);
      });
    }
  }

  async load () {
    try {
      const file = await promises.readFile(getPath('settings.json'), 'utf8');
      const json = JSON.parse(file);

      this.object = {
        ...this.object,
        ...json,
        // deep merge
        player: {...this.object.player, ...(json.player || {})},
        theme: 'dark',
        version: CURRENT_SETTINGS_VERSION,
        host: defaultSettings.host
      };
    } catch (e) {
      console.debug(e)
    }

    this.update();
    this.loaded = true;
    this.loadResolvers.forEach(f => f())
    this.loadResolvers = []
  }

  async save () {
    try {
      const serialzieObject = {...this.object}
      delete serialzieObject.host;
      await promises.writeFile(
        getPath('settings.json'),
        JSON.stringify(serialzieObject)
      );
    } catch (e) {
      console.error(e);
    }

    this.pendingSaving = 0
  }

  addToSaveQueue () {
    this.update();
    if (this.pendingSaving++ === 0) {
      this.save();
    }
  }
}

export default new Settings()
