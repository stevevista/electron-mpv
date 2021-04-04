"use strict";

const {BrowserWindow, app, dialog, ipcMain} = require("electron");
const path = require("path");

app.commandLine.appendSwitch('no-sandbox');
app.commandLine.appendSwitch("ignore-gpu-blacklist");
app.commandLine.appendSwitch("register-pepper-plugins", `${__dirname}\\build\\Release\\pepper_mpv.dll;application/x-mpvjs`);

app.on("ready", () => {
  const win = new BrowserWindow({
    width: 1280,
    height: 574,
    autoHideMenuBar: true,
    useContentSize: process.platform !== "linux",
    title: "mpv.js example player",
    webPreferences: {
      plugins: true,
      preload: path.resolve(__dirname, 'preload.js'),
      nodeIntegration: false, 
      webSecurity: false,
      javascript: true
    },
  });
  win.setMenu(null);
  win.loadURL(`file://${__dirname}/index.html`);
  win.webContents.openDevTools({ mode: 'detach' });
});

app.on("window-all-closed", () => {
  app.quit();
});

ipcMain.handle('open-file', async () => {
  const { filePaths } = await dialog.showOpenDialog({filters: [
    {name: "Videos", extensions: ["mkv", "webm", "mp4", "mov", "avi", "ps"]},
    {name: "All files", extensions: ["*"]},
  ]});
  if (filePaths && filePaths[0]) {
    return filePaths[0]
  }
})
