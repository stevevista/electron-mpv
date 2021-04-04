<template>
  <div
    ref="playList"
    class="playList"
    @contextmenu.stop
  >
    <div class="my-arrow" v-show="isShowArrow">
      <v-icon color="white" @click.stop="hideList" v-text="isHidenList ? 'mdi-chevron-double-left' : 'mdi-chevron-double-right'"></v-icon>
    </div>
    <div class="content-container" v-show="!isHidenList"
      @drop.stop="addFile" @dragover.prevent>
      <v-toolbar
      color="rgba(0, 100, 100, 0.1)"
      dark
    >
      <v-btn icon @click="addItem">
        <v-icon>mdi-plus-box</v-icon>
      </v-btn>

      <v-btn icon @click="removeAll">
        <v-icon>mdi-close-box-multiple</v-icon>
      </v-btn>
    </v-toolbar>
      <v-list
        color="rgba(0, 100, 100, 0.1)"
      >
        <v-hover v-for="(item, index) in playlist"
          :key="item.id" v-slot:default="{ hover }">
        <v-list-item
          :value="index"
          :input-value="item.current"
          active-class="active_list-item"
          color="red"
          @dblclick="player.command('playlist-play-index', index)"
          @click="">
            <v-list-item-content>
              <v-list-item-title>{{ item.name }}</v-list-item-title>
              <v-list-item-subtitle>
                <span class="list-tem-text">{{ item.filename }}</span>
              </v-list-item-subtitle>
            </v-list-item-content>

          <v-list-item-action v-if="hover">
            <v-icon color="red" @click="player.command('playlist-remove', index)">
                  mdi-close-box
                </v-icon>
              </v-list-item-action>
        </v-list-item>
        </v-hover>
      </v-list>
    </div>
  </div>
</template>

<script>
export default {
  data () {
    return {
      isHidenList: true
    };
  },
  props: {
    playlist: {type: Array, default: () => []},
    isShowArrow: {
      type: Boolean,
      default: false
    }
  },

  mounted () {
    this.player = document.getElementById('the-player');
  },

  methods: {
    removeAll () {
      this.player.command('playlist-clear');
      this.player.command('playlist-remove', 'current');
    },
    addItem () {
      this.$emit('add-items');
    },
    hideList () {
      this.isHidenList = !this.isHidenList;
    },

    async addFile (e) {
      this.$emit('add-items', e.dataTransfer.items)
    }
  }
}
</script>

<style scoped lang="scss">
.playList {
  position: absolute;
  top: 0;
  right: 0;
  height: 100%;
  background-color: rgba(0, 0, 0, 0.4);
  border-left: 1px solid #2f2f31;
  .content-container {
    height: 100%;
    overflow:auto;
    position: relative;
    width: 300px;
  }
  .my-arrow {
    position: absolute;
    top: 50%;
    left: -31px;
    transform: translateY(-50%);
    width: 30px;
    height: 66px;
    line-height: 66px;
    text-align: center;
    cursor: pointer;
    > span {
      width: 100%;
      line-height: 66px;
    }
  }
  .top {
    padding: 15px 15px 5px;
    display: flex;
    flex-direction: row;
    justify-content: space-between;
    width: 100%;
    > span {
      font-size: 15px;
    }
    .my-icon {
      display: flex;
      flex-direction: row;
      align-items: center;
      font-size: 15px;
      position: relative;
      > span {
        cursor: pointer;
        &:hover {
          color: #1bb017;
        }
      }
      > span + span {
        margin-left: 10px;
      }
      > .delete {
        font-size: 14px;
      }
    }
  }
}

.list-content {
  overflow: auto;
}
.top {
  max-height: 40px;
  transition: width 1s;
  overflow: hidden;
}

.active_list-item {
  background-color: rgba(93, 238, 0, 0.5);
}

.list-tem-text {
  color: white;
  white-space: normal;
  word-break: break-all
}
</style>
