import { resolve } from 'path';
import { app, ipcRenderer } from 'electron';

export const getPath = (...relativePaths) => {
  let path;

  if (app) {
    path = app.getPath('userData');
  } else {
    path = ipcRenderer.sendSync('get-path', 'userData');
  }

  return resolve(path, ...relativePaths).replace(/\\/g, '/');
};
