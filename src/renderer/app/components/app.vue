<template>
  <div id="app" :style="{height: '100%', display: 'flex', 'flex-direction': 'column'}">

    <div class="titlebar">
      <div class="tabbar">
          <div
            class="tabs-container"
          >
            <Tab 
              v-for="item in $store.tabs"
              :key="item.id"
              :tab="item"
            />
          </div>
      </div>
      <div class="main-title">{{ $store.windowTitle }}</div>

      <WindowsControls 
        v-if="platform !== 'darwin'"
        :style="{
            height: '100%',
            marginLeft: 8
        }"
      />
    </div>

    <div ref="view-placehodler" :style="{flex: '1'}" />
  </div>
</template>

<script>
import { platform } from 'os';
import WindowsControls from './windows-controls'
import Tab from './tab';

export default {
  components: {
    WindowsControls,
    Tab
  },
  data: () => ({
    platform: platform()
  }),
  mounted () {
    const resizeObserver = new ResizeObserver(([{ contentRect }]) => {
      this.$store.send('resize-height');
    });
    resizeObserver.observe(this.$refs['view-placehodler']);
  }
}
</script>

<style lang="scss">
@import '~/renderer/styles/themes.scss';
@import '~/renderer/styles/images.scss';
@import '~/renderer/styles/transparency.scss';

body {
  user-select: none;
  cursor: default;
  margin: 0;
  padding: 0;
  width: 100vw;
  height: 100vh;
  overflow: hidden;
  font-family: system-ui, sans-serif;
}

* {
  box-sizing: border-box;
}

.titlebar {
  position: relative;
  z-index: 100;
  display: flex;
  flex-flow: row;
  color: rgba(0, 0, 0, 0.8);
  width: 100%;

  &:before {
    position: absolute;
    z-index: 0;
    top: 4px;
    left: 4px;
    right: 4px;
    bottom: 0px;
    -webkit-app-region: drag;
    content: '';
  }

  @include -background-color(titlebar-backgroundColor);
  height: $DEFAULT_TAB_HEIGHT;

  align-items: center;

  padding-left: 4px;

  &:before {
    -webkit-app-region: drag;
  }
}

.tabbar {
  height: 100%;
  width: 100%;
  position: relative;
  overflow: hidden;
  align-items: center;
  margin-right: 32px;
  display: flex;
}

.tabs-container {
  height: 100%;
  width: 100%;
  position: relative;
  overflow: hidden;
  overflow-x: overlay;
  white-space: nowrap;
  display: flex;

  &::-webkit-scrollbar {
    height: 0px;
    display: none;
    background-color: transparent;
    opacity: 0;
  }
}

.main-title {
  font-size: 12px;
  width: inherit;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
  transition: 0.2s margin-left;
  margin-left: 8px;
  min-width: 0;
  @include -color(tab-textColor);
}
</style>
