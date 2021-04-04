import Vue from 'vue'
import VueI18n from 'vue-i18n'

Vue.use(VueI18n)

const files = require.context('.', false, /\.yml$/)
const messages = {}

files.keys().forEach(key => {
  messages[key.replace(/(\.\/|\.yml)/g, '')] = files(key)
})

const i18n = new VueI18n({
  locale: 'en',
  messages
});

export default i18n;
