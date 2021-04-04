const { ipcRenderer, webFrame } = require('electron');

(async function () {
  const w = await webFrame.executeJavaScript('window');
  w.require = (id) => {
    if (id === 'electron') {
      return { ipcRenderer };
    }
    return undefined;
  };
})();
