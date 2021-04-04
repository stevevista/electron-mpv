<template>
  <v-app>
    <div id="app" class="container">
      <NavigationDrawer :title="$t('settings')">
        <NavigationDrawerItem
          :icon="ICON_PLAYER"
          @click="$store.selectedSection='player'"
          :selected="$store.selectedSection==='player'"
        >
          {{ $t('player') }}
        </NavigationDrawerItem>
        <NavigationDrawerItem
          :icon="ICON_VERSION"
          @click="$store.selectedSection='about'"
          :selected="$store.selectedSection==='about'"
        >
          {{ $t('about') }}
        </NavigationDrawerItem>
      </NavigationDrawer>
      <div class="pages-content">
        <div class="pages-left-content" :style="{ maxWidth: '800px', marginTop: '56px' }">
          <PlayControl v-if="$store.selectedSection === 'player'" />
          <About v-if="$store.selectedSection === 'about'" />
        </div>
      </div>
    </div>
  </v-app>
</template>

<script>
import NavigationDrawer from './navigation-drawer';
import NavigationDrawerItem from './navigation-drawer-item';
import PlayControl from './play-control';
import About from './about';
import ICON_VERSION from '~/renderer/resources/icons/version.svg'
import ICON_PLAYER from '~/renderer/resources/icons/player.svg'
import ICON_SETTINGS from '~/renderer/resources/icons/settings.svg'

export default {
  components: {
    NavigationDrawer,
    NavigationDrawerItem,
    PlayControl,
    About
  },
  data: () => ({
  }),
  computed: {
    ICON_SETTINGS () { return ICON_SETTINGS },
    ICON_PLAYER () { return ICON_PLAYER },
    ICON_VERSION () { return ICON_VERSION }
  }
}
</script>

<style lang="scss">
@import '~/renderer/styles/pages.scss';

.header {
  margin-top: 4px;
  margin-bottom: 16px;
  font-size: 20px;
  display: flex;
  align-items: center;
}

.title {
  font-size: 14px;
}

.secondary-text {
  opacity: 0.54;
  font-weight: 400;
  margin-top: 4px;
  font-size: 12px;
}

.control {
  margin-left: auto;
}

.row {
  width: 100%;
  display: flex;
  align-items: center;
  min-height: 48px;

  cursor: pointer;
  &:last-of-type {
    border: none;
  }

  [data-theme=light] & {
    border-bottom: 1px solid rgba(0, 0, 0, 0.12);
  }
  [data-theme=dark] & {
    border-bottom: 1px solid rgba(255, 255, 255, 0.12);
  }
}

.container {
  @include pages-container(false);
}
</style>
