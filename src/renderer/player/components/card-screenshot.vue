<template>
  <v-card color="rgba(0, 0, 0, 0.7)">
    <div class="screenshot-settings">
      <v-list dense>
              <v-list-item>
                <v-list-item-content>
                  <v-list-item-title>
                    <span class="setting-item-label text-right">{{ $t('video.settings.snapshot_format') }}</span><span class="pr-4"/>
                    <v-btn-toggle color="primary" mandatory v-model="$store.settings.screenshotFormat">
                      <v-btn text small value="png">png</v-btn>
                      <v-btn text small value="jpeg">jpeg</v-btn>
                    </v-btn-toggle>
                  </v-list-item-title>
                </v-list-item-content>
              </v-list-item>

              <v-list-item>
                <v-list-item-content>
                  <v-list-item-title>
                    <span class="setting-item-label text-right">{{ $t('video.settings.snapshot_dir') }}</span><span class="pr-4"/>
                    <span><v-btn icon small @click="selectFolder"><v-icon>mdi-folder</v-icon></v-btn>{{ $store.settings.screenshotPath }}</span>
                  </v-list-item-title>
                </v-list-item-content>
              </v-list-item>

              <v-list-item>
                <v-list-item-content>
                  <v-list-item-title>
                    <span class="setting-item-label text-right">{{ $t('video.settings.shot_with_detection') }}</span><span class="pr-4"/>
                    <span class="setting-item-ctrl-content">
                      <v-switch
                        hide-details
                        v-model="$store.settings.screenshotWithAI"
                      ></v-switch>
                    </span>
                  </v-list-item-title>
                </v-list-item-content>
              </v-list-item>
            </v-list>
    </div>
  </v-card>
</template>

<script>
export default {
  watch: {
    '$store.settings.screenshotWithAI' () {
      this.$store.invalidateSettings()
    }
  },

  methods: {
    selectFolder () {
      this.$store.selectScreenshotFolder('')
    }
  }
}
</script>

<style lang="scss" scoped>
.screenshot-settings {
  height: 100%;
  .v-input--selection-controls {
    margin-top: 0;
    padding-top: 0;
  }

  .setting-item-label {
    width: 180px;
    display: inline-block;
  }

  .setting-item-ctrl-content {
    width: 50px;
    display: inline-block;
  }
}
</style>
