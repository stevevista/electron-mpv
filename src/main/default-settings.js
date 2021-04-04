import { app } from 'electron';
import pkg from '~/../package.json'

export default {
  theme: 'light',
  locale: '',
  userDataPath: app
    ? app.getPath('userData')
    : '',
  // for player
  screenshotPath: null,
  aiSwitchs: ['pedestrian', 'face', 'vehicle'],
  screenshotFormat: 'jpeg',
  screenshotWithAI: true,
  player: {
    streamingType: 'tcp',
    disableAudio: false,
    hwaccel: 'none',
    debug: false,
    syncType: 'audio',
    hotDetection: false
  },
  host: {
    version: pkg.version,
    name: pkg.name
  }
}
