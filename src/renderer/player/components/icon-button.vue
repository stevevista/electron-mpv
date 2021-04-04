<template functional>
  <button 
    class="player-icon"
    :disabled="props.disabled" @click="listeners['click']($event)"
    :style="{
      opacity: props.disabled ? 0.4 : 1.0
    }"
    :title="props.tooltip">
    <span :class="`mdi mdi-${props.icon} mdi-light ${props.inactive ? `mdi-inactive` : ''}  ${props.spin ? `mdi-spin` : ''} mdi-24px`"></span>
  </button>
</template>

<script>
export default {
  props: {
    icon: {type: String, default: ''},
    disabled: {type: Boolean, default: false},
    inactive: {type: Boolean, default: false},
    spin: {type: Boolean, default: false},
    tooltip: {type: String, default: null}
  }
}
</script>
