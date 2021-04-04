<template>
  <v-card :min-height="200" color="rgba(0, 0, 0, 0.7)">
    <v-container>
      <div class="equalizer-settings">
          <v-row class="pr-2">
            <v-col :cols="3">
              <span class="white--text text-right">Brightness</span>
            </v-col>
            <v-col :cols="9">
              <v-slider
                v-model="brightness"
                max="100"
                min="-100"
                hint
                dark
                dense
                hide-details
                thumb-label
                :thumb-size="24"
                thumb-color="primary"
              ></v-slider>
            </v-col>
          </v-row>

          <v-row class="pr-2">
            <v-col :cols="3">
              <span class="white--text text-right">Contrast</span>
            </v-col>
            <v-col :cols="9">
              <v-slider
                v-model="contrast"
                max="100"
                min="-100"
                hint
                dark
                dense
                hide-details
                thumb-label
                :thumb-size="24"
                thumb-color="primary"
              ></v-slider>
            </v-col>
          </v-row>

          <v-row class="pr-2">
            <v-col :cols="3">
              <span class="white--text text-right">Gamma</span>
            </v-col>
            <v-col :cols="9">
              <v-slider
                v-model="gamma"
                max="100"
                min="-100"
                hint
                dark
                dense
                hide-details
                thumb-label
                :thumb-size="24"
                thumb-color="primary"
              ></v-slider>
            </v-col>
          </v-row>

          <v-row class="pr-2">
            <v-col :cols="3">
              <span class="white--text text-right">Saturation</span>
            </v-col>
            <v-col :cols="9">
              <v-slider
                v-model="saturation"
                max="100"
                min="-100"
                hint
                dark
                dense
                hide-details
                thumb-label
                :thumb-size="24"
                thumb-color="primary"
              ></v-slider>
            </v-col>
          </v-row>

          <v-row class="pr-2">
            <v-col :cols="3">
              <span class="white--text text-right">Hue</span>
            </v-col>
            <v-col :cols="9">
              <v-slider
                v-model="hue"
                max="100"
                min="-100"
                hint
                dark
                dense
                hide-details
                thumb-label
                :thumb-size="24"
                thumb-color="primary"
              ></v-slider>
            </v-col>
          </v-row>

          <v-row class="pr-2">
            <v-col :cols="3">
              <span class="white--text text-right">Colormatrix</span>
            </v-col>
            <v-col :cols="9">
              <v-radio-group row dark hide-details v-model="primary">
                <v-radio
                  label="Auto"
                  value="auto"
                ></v-radio>
                <v-radio
                  label="BT.601"
                  value="bt.601-625"
                ></v-radio>
                <v-radio
                  label="BT.709"
                  value="bt.709"
                ></v-radio>
              </v-radio-group>
            </v-col>
          </v-row>
      </div>
        </v-container>

        <v-card-text>
          <v-btn
            color="primary"
            @click="resetEqualizer"
          >
            Reset
          </v-btn>
        </v-card-text>
  </v-card>
</template>

<script>
export default {
  data () {
    return {
      contrast: 0,
      brightness: 0,
      gamma: 0,
      saturation: 0,
      hue: 0,
      primary: 'auto'
    }
  },

  watch: {
    contrast (v) {
      this.player.property('contrast', v)
    },

    brightness (v) {
      this.player.property('brightness', v)
    },

    gamma (v) {
      this.player.property('gamma', v)
    },

    saturation (v) {
      this.player.property('saturation', v)
    },

    hue (v) {
      this.player.property('hue', v)
    },

    primary (v) {
      if (v === 'auto' || !v) {
        this.player.option('target-prim', 'auto')
      } else {
        this.player.option('target-prim', v)
      }
    }
  },

  mounted () {
    this.player = document.getElementById('the-player');
  },

  methods: {
    resetEqualizer () {
      this.contrast = 0
      this.brightness = 0
      this.gamma = 0
      this.saturation = 0
      this.hue = 0
      this.primary = 'auto'
    }
  }
}
</script>

<style lang="scss" scoped>
.equalizer-settings {
  height: 100%;
  .v-input--selection-controls {
    margin-top: 0;
    padding-top: 0;
  }
}
</style>
