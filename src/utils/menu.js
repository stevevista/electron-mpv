import { ipcRenderer } from 'electron'

export function createMenuBuilder (closeCallback) {
  let menuActions = []
  let installed = false

  function onMenuAction (e, action) {
    if (action === '__close__') {
      typeof closeCallback === 'function' && closeCallback();
      return
    }

    const idx = parseInt(action)
    const callback = menuActions[idx]
    typeof callback === 'function' && callback();
  }

  function installListener () {
    if (!installed) {
      installed = true
      ipcRenderer.on('view-open-context-menu', onMenuAction)
    }
  }

  function dispose () {
    if (installed) {
      installed = false
      ipcRenderer.removeListener('view-open-context-menu', onMenuAction)
    }
  }

  function buileContextMenuItems (items) {
    if (!Array.isArray(items)) {
      return undefined
    }

    return items.map(i => {
      const { click, submenu } = i
      const item = {...i}
      delete item.click
      delete item.submenu

      if (typeof click === 'function') {
        const idx = menuActions.length
        menuActions.push(i.click)
        item.action = idx.toString()
      }

      if (submenu) {
        item.submenu = buileContextMenuItems(submenu)
      }

      return item
    })
  }

  function popup (items, x, y) {
    installListener();
    menuActions = []
    ipcRenderer.send('view-open-context-menu', buileContextMenuItems(items), x, y)
  }

  return {
    popup,
    dispose
  }
}
