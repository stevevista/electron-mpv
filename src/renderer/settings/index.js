import { createApp } from '../player/vue3'
import App from './components/app'

const app = createApp(App)
document.title = app.$t('settings')
