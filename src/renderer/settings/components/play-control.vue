<template>
  <div>
    <div class="row">
      <div>{{ $t('language') }}</div>
      <div class="control">
        <v-btn-toggle color="primary" mandatory v-model="$store.settings.locale">
          <v-btn text small value="en">English</v-btn>
          <v-btn text small value="zh-cn">中文</v-btn>
        </v-btn-toggle>
      </div>
    </div>
    <div class="row">
      <div>{{ $t('transport') }}</div>
      <div class="control">
        <v-btn-toggle color="primary" mandatory v-model="$store.settings.player.streamingType">
          <v-btn text small value="tcp">TCP</v-btn>
          <v-btn text small value="udp">UDP</v-btn>
        </v-btn-toggle>
      </div>
    </div>
    <div class="row">
      <div>{{ $t('sync-type') }}</div>
      <div class="control">
        <v-btn-toggle color="primary" mandatory v-model="$store.settings.player.syncType">
          <v-btn text small value="audio">{{ $t('audio') }}</v-btn>
          <v-btn text small value="video">{{ $t('video-sync') }}</v-btn>
          <v-btn text small value="nodelay">{{ $t('nodelay') }}</v-btn>
        </v-btn-toggle>
      </div>
    </div>

    <div class="row">
      <div>{{ $t('video.hwaccel') }}</div>
      <div class="control">
        <v-btn-toggle color="primary" mandatory v-model="$store.settings.player.hwaccel">
          <v-btn text small value="none">None</v-btn>
          <v-btn text small value="auto">Auto</v-btn>
        </v-btn-toggle>
      </div>
    </div>

    <div class="row">
      <div>{{ $t('video.settings.disable_audio') }} (*)</div>
      <div class="control">
        <v-switch
          hide-details
          v-model="$store.settings.player.disableAudio"
        ></v-switch>
      </div>
    </div>

    <div class="row">
      <div>{{ $t('debug_report') }}</div>
      <div class="control">
        <v-switch
          hide-details
          v-model="$store.settings.player.debug"
        ></v-switch>
      </div>
    </div>

    <div class="row">
      <div>
        <div>{{ $t('log_loc') }}</div>
        <div class="secondary-text">{{ $store.settings.userDataPath }}\logs</div>
      </div>
      <div class="control">
        <v-btn small @click="openLogPath">{{ $t('open') }}</v-btn>
      </div>
    </div>
  
  </div>
</template>

<script>
import { ipcRenderer } from 'electron';

export default {
  watch: {
    '$store.settings.locale' (v) {
      this.$store.invalidateSettings()
    },
    '$store.settings.player.streamingType' (v) {
      this.$store.invalidateSettings()
    },
    '$store.settings.player.syncType' (v) {
      this.$store.invalidateSettings()
    },
    '$store.settings.player.hwaccel' (v) {
      this.$store.invalidateSettings()
    },
    '$store.settings.player.disableAudio' () {
      this.$store.invalidateSettings()
    },
    '$store.settings.player.debug' () {
      this.$store.invalidateSettings()
    }
  },
  methods: {
    openLogPath (e) {
      ipcRenderer.send('open-logs-path');
    }
  }
}
</script>
