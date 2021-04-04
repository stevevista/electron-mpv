import { app, ipcRenderer } from 'electron';
import { join, dirname } from 'path';

export const appPath = app
  ? app.getAppPath()
  : ipcRenderer.sendSync('get-path', 'app')

export const appRootPath = app
  ? process.env.NODE_ENV === 'development' ? app.getAppPath() : dirname(app.getPath('exe'))
  : ipcRenderer.sendSync('get-path', 'app-root')

export const distFile = (name) => process.env.NODE_ENV === 'development'
  ? `http://localhost:${process.env.WEB_PORT}/${name}.html` : join('file://', appPath, `build/${name}.html`)
