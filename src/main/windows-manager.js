import { ipcMain, shell, app, BrowserWindow, dialog } from 'electron'
import Window from './window';
import fs from 'fs'
import { appRootPath } from '~/utils/dist'

class WindowsManager {
  list = []

  open () {
    const window = new Window();
    this.list.push(window);

    window.win.on('focus', () => {
      this.current = window;
    });

    window.win.on('close', () => {
      this.list = this.list.filter(
        (x) => x.win.id !== window.win.id
      );
    })

    return window;
  }

  constructor () {
    ipcMain.on('get-path', (e, name) => {
      if (name === 'app') {
        e.returnValue = app.getAppPath();
      } else if (name === 'app-root') {
        e.returnValue = appRootPath
      } else {
        e.returnValue = app.getPath(name);
      }
    });
    
    ipcMain.on('view-focus', (e) => {
      e.sender.focus()
    });
    
    ipcMain.on('shell-open-item', (e, path, isdir) => {
      if (isdir) {
        shell.openPath(path);
      } else {
        shell.showItemInFolder(path);
      }
    })

    ipcMain.handle('shell-stat-path', async (e, path) => {
      try {
        const stat = await fs.promises.stat(path)
        if (stat) {
          return {
            isDirectory: stat.isDirectory()
          }
        }
      } catch (e) {
        console.error(e)
      }

      return null
    })

    // a generic api for reomte disabled renderer
    ipcMain.handle('shell-open-file-dialog', async (e, {defaultPath = '', directory = false, save = false, filters}) => {
      if (directory || !save) {
        const result = await dialog.showOpenDialog(
          BrowserWindow.fromWebContents(e.sender), {
            defaultPath,
            filters,
            properties: directory ? ['openDirectory'] : undefined
          })

        if (result && !result.canceled) {
          return result.filePaths[0]
        }
      } else {
        const { canceled, filePath } = await dialog.showSaveDialog({
          defaultPath,
          filters
        });

        if (!canceled && filePath) {
          return filePath
        }
      }
  
      return ''
    })

    ipcMain.on('view-open-context-menu', (e, ...args) => {
      const id = e.sender.windowId;
      ipcMain.emit(`view-open-context-menu-${id}`, e, ...args)
    })

    ipcMain.on('window-inintial', (e, ...args) => {
      const id = e.sender.windowId;
      ipcMain.emit(`window-inintial-${id}`, e, ...args)
    })
  }
}

export default new WindowsManager()
