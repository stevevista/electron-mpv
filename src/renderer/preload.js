import { ipcRenderer, webFrame } from 'electron';

(async function () {
  const w = await webFrame.executeJavaScript('window');
  w.require = (id) => {
    if (id === 'electron') {
      return { ipcRenderer };
    }
    return undefined;
  };
})();
