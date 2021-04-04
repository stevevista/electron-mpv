import { createApp } from './vue3'
import App from './components/mpv'

const app = createApp(App)
document.title = app.$t('player')
