<template>
  <div class="drawer-item"
    :title="title"
    @click="$emit('click', $event)"
    :style="{
      '--drawer-item--selected': selected ? 1 : 0
    }"
  >
    <div class="icon"
      v-if="icon"
      :style="{
        backgroundImage: `url(${icon})`
      }"
    />
    <slot />
  </div>
</template>

<script>
export default {
  props: {
    selected: Boolean,
    icon: String,
    title: { type: String, default: '' }
  }
}
</script>

<style lang="scss" scoped>
@import '~/renderer/styles/images.scss';
@import '~/renderer/styles/transparency.scss';
@import '~/renderer/styles/themes.scss';

.drawer-item {
  padding: 4px 16px;
  display: flex;
  height: 40px;
  border-radius: 4px;
  align-items: center;
  position: relative;
  cursor: pointer;

  &:before {
    opacity: var(--drawer-item--selected);
    content: '';
    position: absolute;
    left: 0;
    border-radius: 2px;
    width: 3px;
    height: 18px;
  }

  &:hover {
    @include --background-color((light: rgba(0, 0, 0, 0.04), dark: rgba(255, 255, 255, 0.06)));
  }

  &:before {
    @include --background-color((light: black, dark: white));
  }
}

.icon {
  height: 24px;
  width: 24px;
  min-width: 24px;
  opacity: $transparency-icons-inactive;
  margin-right: 16px;
  @include center-icon(20px);
  @include --filter((light: none, dark: invert(100%)));
}
</style>
