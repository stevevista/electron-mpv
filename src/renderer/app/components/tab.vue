<template>
  <div
    class="tab"
    @mousedown="mousedown"
  >
    <div
      class="tab-container"
      :class="{
        'tab-container--selected': isSelected && $store.tabs.length > 1
      }"
    >
      <div class="content">
        <div
          class="icon"
          :style="{ backgroundImage: `url(${tab.favicon})` }"
        />
      </div>
    </div>
  </div>
</template>

<script>
export default {
  props: {
    tab: Object
  },
  computed: {
    isSelected () {
      return this.$store.selectedTab === this.tab;
    }
  },
  methods: {
    mousedown ({ button }) {
      if (button === 0) {
        if (!this.isSelected) {
          this.$store.selectTab(this.tab);
        }
      }
    }
  }
}
</script>

<style lang="scss" scoped>
@import '~/renderer/styles/images.scss';
@import '~/renderer/styles/transparency.scss';
@import '~/renderer/styles/themes.scss';

.tab {
  height: 100%;
  width: 40px;
  will-change: width, transform;
  -webkit-app-region: no-drag;
  display: flex;
  backface-visibility: hidden;
}

.tab-container {
  position: relative;

  width: 100%;
  align-items: center;
  overflow: hidden;
  display: flex;
  backface-visibility: hidden;
  transition: 0.1s background-color;
  border-bottom: transparent !important;
  border: 2px solid;

  max-width: 100%;
  border-color: transparent;

  margin-top: 0px;
  border-radius: 0px;
  height: $DEFAULT_TAB_HEIGHT;

  box-shadow: none;

  background-color: transparent;

  &:hover {
    @include --background-color((light: rgba(255, 255, 255, 0.5), dark: rgba(255, 255, 255, 0.08)));
  }
}

.tab-container--selected {
  box-shadow: 0px 0px 6px 0px rgba(0,0,0,0.12);

  @include -background-color(toolbar-backgroundColor);

  &:hover {
    @include --background-color((light: #fcfcfc, dark: #393939));
  }
}

.icon {
  height: 16px;
  min-width: 16px;
  transition: 0.2s opacity, 0.2s min-width;
  @include center-icon;
}

.title {
  font-size: 12px;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
  transition: 0.2s margin-left;
  margin-left: 8px;
  min-width: 0;
  flex: 1;

  margin-left: var(--title-margin-left);
  @include -color(tab-textColor);
}

.title-selected {
  @include -color(tab-selected-textColor);
}

.content {
  overflow: hidden;
  z-index: 2;
  align-items: center;
  display: flex;
  margin-left: 10px;
  flex: 1;
}
</style>
