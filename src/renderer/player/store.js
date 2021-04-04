import { observable } from '~/utils/observable';
import { ipcRenderer } from 'electron'

export class Store {
  @observable
  settings = ipcRenderer.sendSync('get-settings-sync');

  @observable
  selectedSection = 'player';

  constructor () {
    ipcRenderer.on('update-settings', (e, settings) => {
      this.settings = { ...this.settings, ...settings };
    });
  }

  invalidateSettings () {
    ipcRenderer.send('save-settings', { settings: this.settings });
  }

  toggleSetting (key, save = true) {
    this.settings[key] = !this.settings[key];
    if (save) {
      this.invalidateSettings();
    }
  }

  changeSetting (key, val, save = true) {
    this.settings[key] = val;
    if (save) {
      this.invalidateSettings();
    }
  }

  togglePlayerSetting (key, save = true) {
    this.settings.player[key] = !this.settings.player[key];
    if (save) {
      this.invalidateSettings();
    }
  }

  changePlayerSetting (key, val, save = true) {
    this.settings.player[key] = val;
    if (save) {
      this.invalidateSettings();
    }
  }

  async provideRtspAuth (options) {
    return null
  }

  toggleAI (fea) {
    if (fea === 'all') {
      if (this.settings.aiSwitchs.length) {
        this.settings.aiSwitchs = []
      } else {
        this.settings.aiSwitchs = ['face', 'pedestrian', 'vehicle', 'face_detail', 'pedestrian_detail', 'car_detail']
      }
    } else {
      if (this.settings.aiSwitchs.indexOf(fea) < 0) {
        this.settings.aiSwitchs = this.settings.aiSwitchs.concat([fea])
      } else {
        this.settings.aiSwitchs = this.settings.aiSwitchs.filter(item => item !== fea)
      }
    }

    this.invalidateSettings()
  }

  isAIFeatureOn (fea) {
    return this.settings.aiSwitchs.indexOf(fea) >= 0
  }

  get hasDetectionOn () {
    return this.settings.aiSwitchs.length > 0;
  }

  async ensureScreenshotDirectoryExists () {
    // select directory
    let dir = this.settings.screenshotPath;
    if (dir) {
      const stat = await ipcRenderer.invoke('shell-stat-path', dir)
      if (stat && stat.isDirectory) {
        return true
      }
    }
    
    const selDir = await this.selectScreenshotFolder()
    return !!selDir
  }

  async selectScreenshotFolder () {
    const dir = await ipcRenderer.invoke('shell-open-file-dialog', {directory: true, defaultPath: this.settings.screenshotPath || ''})
    if (dir) {
      this.settings.screenshotPath = dir;
      this.invalidateSettings()
    }
    return dir
  }
}

export default new Store();
