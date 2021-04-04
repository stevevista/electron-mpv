<template>
  <div
    @click="$emit('click', $event)"
    @mouseenter="hover=true"
    @mouseleave="hover=false"
    :style="{
      height: '100%',
      width: '45px',
      minWidth: '45px',
      position: 'relative',
      transition: '0.2s background-color',
      backgroundColor: hover
            ? !close
              ? 'rgba(168, 168, 168, 0.4)'
              : '#e81123'
            : 'transparent',
      pointerEvents: disabled ? 'none' : 'auto',
      WebkitAppRegion: 'no-drag'
    }" 
    
  >
    <div 
      class="icon"
      :style="{
        width: '100%',
        height: '100%',
        transition: '0.2s filter',
        filter: (close && hover) ? 'invert(100%)' : 'none',
        backgroundPosition: 'center',
        backgroundSize: '11px',
        backgroundRepeat: 'no-repeat',
        backgroundImage: `url(${icon})`,
        opacity: disabled ? 0.54 : 1
      }"
    >
    </div>
  </div>
</template>

<script>
import closeIcon from '~/renderer/resources/icons/win-close.svg';
import minimizeIcon from '~/renderer/resources/icons/minimize.svg';
import maximizeIcon from '~/renderer/resources/icons/maximize.svg';
import restoreIcon from '~/renderer/resources/icons/restore.svg';
export default {
  props: {
    maximize: Boolean,
    close: Boolean,
    minimize: Boolean,
    restore: Boolean,
    disabled: Boolean
  },
  data: () => ({
    hover: false
  }),
  computed: {
    icon () {
      if (this.close) return closeIcon;
      else if (this.minimize) return minimizeIcon;
      else if (this.maximize) return maximizeIcon;
      else if (this.restore) return restoreIcon;
    }
  }
}
</script>

<style lang="scss" scoped>
.icon {
  [data-theme=dark] & {
    filter: invert(100%) !important;
  }
}
</style>
