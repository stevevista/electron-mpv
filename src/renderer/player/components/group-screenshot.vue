<template>
  <div class="menu-buttons">
    <icon-button
          :disabled="disabled"
          :spin="working"
          :icon="working ? 'camera-iris' : 'camera'"
          :tooltip="$t(working ? 'video.snapshot_working' : 'video.snapshot')"
          @click.stop="$emit(working ? 'screenshot-stop' : 'screenshot')"/>
    <icon-button
          :disabled="working || disabled"
          icon="image-multiple"
          @click.stop="$emit('screenshot-multi')"
          :tooltip="$t('video.shot_each')"/>
    <icon-button
          :disabled="working"
          icon="settings"
          @click.stop="dialogScreenshot=true"
          :tooltip="$t('common.setting')"/>

    <v-dialog
        v-model="dialogScreenshot"
        width="500"
      >
      <CardScreenshot />
    </v-dialog>
  </div>
</template>

<script>
import IconButton from './icon-button'
import CardScreenshot from './card-screenshot'

export default {
  components: {
    IconButton,
    CardScreenshot
  },
  props: {
    disabled: {type: Boolean, default: false},
    working: {type: Boolean, default: false}
  },
  data () {
    return {
      dialogScreenshot: false
    }
  }
}
</script>

<style lang="scss" scoped>
@import '../styles.scss';

.player-icons {
  @include player-buttons-style;
}
</style>
