import Vue from 'vue'
import VueCompositionAPI from '@vue/composition-api'
import {ipcRenderer} from 'electron'
import App from './components/app'
import store from './store'
import i18n from '~/renderer/i18n'

Vue.config.productionTip = false

Vue.use(VueCompositionAPI)

ipcRenderer.setMaxListeners(0)

Object.defineProperties(Vue.prototype, {
  $store: {
    get () {
      return store
    }
  }
})

/* eslint-disable no-new */
new Vue({
  watch: {
    '$store.settings.locale': {
      immediate: true,
      handler (v) {
        this.$i18n.locale = v || 'en'
      }
    },
    '$store.settings.theme': {
      immediate: true,
      handler (v) {
        window.document.documentElement.setAttribute('data-theme', v);
      }
    }
  },
  i18n,
  render (createElement) {
    return createElement(App);
  }
}).$mount('#app')
