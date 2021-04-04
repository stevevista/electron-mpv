<template>
  <div 
    class= "drawer"
    :class="{
      'drawer-dense': dense
    }"
    :style="{
      padding: dense ? 0 : '0 32px',
      width: `${dense ? 56 : 320}px`
    }"
  >
    <div class="header" v-if="title">
      <div class="title"> {{ title }} </div>
    </div>
    <div class="menu-items">
      <slot />
    </div>
  </div>
</template>

<script>
export default {
  props: {
    dense: Boolean,
    title: { type: String, default: '' }
  }
}
</script>

<style lang="scss" scoped>
@import '~/renderer/styles/scroll.scss';
@import '~/renderer/styles/images.scss';
@import '~/renderer/styles/transparency.scss';
@import '~/renderer/styles/themes.scss';

.drawer {
  height: 100%;
  left: 0;
  display: flex;
  flex-flow: column;
  transition: 0.2s width;
  @include -background-color(pages-navigationDrawer2-backgroundColor);
}

.drawer-dense {
  @include -background-color(pages-navigationDrawer1-backgroundColor);
}

.menu-items {
  display: flex;
  flex-flow: column;
  flex: 1;
  margin-top: 24px;
  padding-bottom: 24px;
  overflow: hidden auto;
  @include no-buttons(6px, rgba(0, 0, 0, 0.04), rgba(0, 0, 0, 0.12));
}

.header {
  display: flex;
  margin-top: 32px;
  align-items: center;
}

.title {
  font-size: 24px;
  font-weight: 300;
}

.input {
  border: none;
  outline: none;

  width: 100%;
  padding-left: 42px;
  background-color: transparent;
  height: 100%;
  font-size: 14px;

  @include --color((light: rgba(0, 0, 0, $transparency-text-high), dark: white));
  &::placeholder {
    color: rgba(0, 0, 0, $transparency-text-medium);
    @include --color((light: rgba(0, 0, 0, $transparency-text-medium), dark: rgba(255, 255, 255, 0.54)));
  }
}

</style>
