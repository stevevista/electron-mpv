import Vue from 'vue'
import VueCompositionAPI from '@vue/composition-api'
import Vuetify, {
  VApp,
  VBtn,
  VBadge,
  VCard,
  VCardText,
  VCardTitle,
  VContainer,
  VIcon,
  VBtnToggle,
  VList,
  VListItem,
  VListItemAction,
  VListItemTitle,
  VListItemContent,
  VListItemSubtitle,
  VToolbar,
  VRadio,
  VRadioGroup,
  VSwitch,
  VSlider,
  VSpacer,
  VCardActions,
  VDialog,
  VRow,
  VCol,
  VTextField,
  VHover
} from 'vuetify/lib'
import 'nacl-mpv-plugin/player-components'
import store from './store'
import i18n from '~/renderer/i18n'
import zhHans from 'vuetify/es5/locale/zh-Hans'
import '~/renderer/styles/web-ui-style.scss'

// import '@mdi/font/css/materialdesignicons.css'
import './style.scss'

Vue.use(VueCompositionAPI)

Vue.config.ignoredElements = ['x-mpv', 'x-live']
Vue.config.productionTip = false

Object.defineProperties(Vue.prototype, {
  $store: {
    get () {
      return store
    }
  }
})

Vue.use(Vuetify, {
  components: {
    VApp,
    VBtn,
    VBadge,
    VCard,
    VCardText,
    VCardTitle,
    VContainer,
    VIcon,
    VBtnToggle,
    VListItem,
    VListItemAction,
    VList,
    VListItemTitle,
    VListItemContent,
    VListItemSubtitle,
    VToolbar,
    VRadio,
    VRadioGroup,
    VSwitch,
    VSlider,
    VSpacer,
    VCardActions,
    VDialog,
    VRow,
    VCol,
    VTextField,
    VHover
  }
});

/* eslint-disable no-new */
export const createApp = (App) => {
  const app = new Vue({
    watch: {
      '$store.settings.locale': {
        immediate: true,
        handler (v) {
          // support vuetify
          this.$vuetify.lang.current = v
          this.$i18n.locale = v
        }
      },
      '$store.settings.theme': {
        immediate: true,
        handler (v) {
          // support vuetify
          this.$vuetify.theme.dark = v === 'dark'
          window.document.documentElement.setAttribute('data-theme', v);
        }
      }
    },
    i18n,
    vuetify: new Vuetify({
      lang: {
        locales: { 'zh-cn': zhHans },
        current: i18n.locale
      },
      theme: {
        dark: true
      }
    }),
    render (createElement) {
      return createElement(App);
    }
  }).$mount('#app');

  return app
}
