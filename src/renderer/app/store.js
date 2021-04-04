import { ipcRenderer } from 'electron'
import defaultSettings from '~/main/default-settings';
import { observable } from '~/utils/observable';
import ICON_SETTINGS_PNG from '~/renderer/resources/icons/settings.png'
import ICON_PLAYER_PNG from '~/renderer/resources/icons/player.png'
import { distFile } from '~/utils/dist'

class Store {
  @observable
  settings = defaultSettings

  @observable
  tabs = [];

  @observable
  isFullscreen = false;

  @observable
  isHTMLFullscreen = false;

  @observable
  isMaximized = false

  @observable
  selectedTab = null

  @observable
  windowTitle = ''

  constructor () {
    ipcRenderer.send('window-inintial')

    ipcRenderer.on('window-inintial', (e, {windowId, settings, isMaximized}) => {
      this.windowId = windowId
      this.isMaximized = isMaximized
      this.updateSettings(settings);
      
      if (this.windowId === 1) {
        this.addTabs([
          {
            url: distFile('player'),
            favicon: ICON_PLAYER_PNG,
            active: true
          },
          {
            url: distFile('settings'),
            favicon: ICON_SETTINGS_PNG,
            active: false
          }
        ]);
      } else {
        this.addTabs([
          {
            url: distFile('player'),
            favicon: ICON_PLAYER_PNG,
            active: true
          }
        ]);
      }
    })

    ipcRenderer.on('update-settings', (e, settings) => {
      this.updateSettings(settings);
    });

    ipcRenderer.on('maximized', (e, v) => {
      this.isMaximized = v;
    })

    ipcRenderer.on('fullscreen', (e, fullscreen) => {
      this.isFullscreen = fullscreen;
    });

    ipcRenderer.on('html-fullscreen', (e, fullscreen) => {
      this.isHTMLFullscreen = fullscreen;
    });

    ipcRenderer.on('title-updated', (e, title) => {
      this.windowTitle = title
    })
  }

  updateSettings (newSettings) {
    this.settings = { ...this.settings, ...newSettings };
  }

  getTabById (id) {
    return this.tabs.find((x) => x.id === id);
  }

  async addTabs (options) {
    if (options.length && !options.filter(o => o.active).length) {
      options[options.length - 1].active = true
    }

    const ids = await ipcRenderer.invoke(
      `views-create-${this.windowId}`,
      options.map(o => o.url)
    );

    options.forEach((option, i) => {
      const tab = {
        id: ids[i],
        favicon: option.favicon
      };
      this.tabs.push(tab);
      if (option.active) {
        this.selectTab(tab);
      }
    })
  }

  async selectTab (tab) {
    if (this.selectedTab === tab) {
      return
    }

    this.selectedTab = tab
    await ipcRenderer.invoke(
      `view-select-${this.windowId}`,
      tab.id
    );
  }

  send (channel, ...args) {
    ipcRenderer.send(`${channel}-${this.windowId}`, ...args);
  }
}

export default new Store()
