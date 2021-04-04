import { ipcMain, app } from 'electron';
import { platform } from 'os';
import path from 'path'
import { mime, pluginFilename } from 'nacl-mpv-plugin'
import { appRootPath } from '~/utils/dist'
import windows from './windows-manager';

app.commandLine.appendSwitch('no-sandbox');
app.commandLine.appendSwitch('ignore-gpu-blacklist');
app.commandLine.appendSwitch('register-pepper-plugins', `${path.resolve(appRootPath, pluginFilename)};${mime}`);

process.env['ELECTRON_DISABLE_SECURITY_WARNINGS'] = true;

app.commandLine.appendSwitch('--enable-transparent-visuals');

if (process.env.NODE_ENV === 'development') {
  app.commandLine.appendSwitch('remote-debugging-port', '9222');
}

// if (process.env.NODE_ENV === 'development') {
//   require('source-map-support').install();
// }

app.allowRendererProcessReuse = false;

ipcMain.setMaxListeners(0);

const gotTheLock = app.requestSingleInstanceLock();
if (!gotTheLock) {
  app.quit();
}

app.on('second-instance', async (e, argv) => {
  windows.open();
})

app.on('ready', () => {
  windows.open();

  app.on('activate', () => {
    if (windows.list.filter((x) => x !== null).length === 0) {
      windows.open();
    }
  });
})

process.on('uncaughtException', (error) => {
  console.error(error);
});

app.on('window-all-closed', () => {
  if (platform() !== 'darwin') {
    app.quit();
  }
});
