<template>
  <div
    :style="{display: 'flex'}"
    @mouseup="$emit('mouseup', $event)"
  >
    <WindowsControl minimize @click="minimize" />
    <WindowsControl :maximize="!$store.isMaximized" :restore="$store.isMaximized" @click="maximize" />
    <WindowsControl close @click="close" />
  </div>
</template>

<script>
import WindowsControl from './windows-control'
export default {
  components: { WindowsControl },
  methods: {
    minimize () {
      this.$store.send('window-minimize')
    },
    maximize () {
      this.$store.send('window-toggle-maximize')
    },
    close () {
      this.$store.send('window-close')
    }
  }
}
</script>
