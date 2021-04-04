<template>
  <div>
    <blockquote class="blockquote">
      <b><u>{{ appName }} - {{ version }}</u></b>
    </blockquote>

    <blockquote class="blockquote">
      <span class="mdi mdi-24px mdi-electron-framework"></span> Electron, Copyright (c) 2013-2019 GitHub Inc.
    </blockquote>

    <blockquote class="blockquote">
      <span class="mdi mdi-24px mdi-google-chrome"></span> Chromium, Copyright (c) 2020 Google LLC.
    </blockquote>

    <blockquote class="blockquote">
      <img width="24" src="https://ffmpeg.org/favicon.ico" /> FFmpeg, Copyright (C) 1991, 1999 Free Software Foundation, Inc. LGPL 2.1 license.
    </blockquote>

    <blockquote class="blockquote">
      <img width="24" src="https://mpv.io/images/favicon-5d3b2a52.png" /> mpv player, Copyright (C) 1991, 1999 Free Software Foundation, Inc. LGPL 2.1 license.
    </blockquote>

    <blockquote class="blockquote">
      libass, Copyright (C) 2006-2016 libass contributors. ISC License.
    </blockquote>

    <blockquote class="blockquote">
      <span class="mdi mdi-24px mdi-vuejs"></span> Vue, Copyright (c) Copyright (c) 2013-present, Yuxi (Evan) You.
    </blockquote>

    <blockquote class="blockquote">
      <span class="mdi mdi-24px mdi-vuetify"></span> Vuetify, Copyright (c) 2016-2020 John Jeremy Leider.
    </blockquote>

    <blockquote class="blockquote">
      Material Design Icons, <a href="https://materialdesignicons.com/">[https://materialdesignicons.com/]</a> Copyright (c) 2014, Austin Andrews (http://materialdesignicons.com/), with Reserved Font Name Material Design Icons.
    </blockquote>

    <blockquote class="blockquote">
      Material Design Icons, Copyright (c) 2014, Google (<a href="http://www.google.com/design/">http://www.google.com/design/</a>) uses the license at https://github.com/google/material-design-icons/blob/master/LICENSE.
    </blockquote>
    <!--
    <blockquote class="blockquote">  
      libmfxhw64.dll, Copyright © 2007-2019, Intel Corporation.
    </blockquote>
    -->
  </div>
</template>

<script>
export default {
  computed: {
    version () { return this.$store.settings.host.version },
    appName () { return this.$store.settings.host.name }
  }
}
</script>

<style lang='scss'>
@import "~@mdi/font/scss/variables";
@import "~@mdi/font/scss/functions";

@font-face {
  font-family: '#{$mdi-font-name}';
  src: url('~@mdi/font/fonts/#{$mdi-filename}-webfont.woff2?v=#{$mdi-version}') format('woff2');
  font-weight: normal;
  font-style: normal;
}

@import "~@mdi/font/scss/core";
@import "~@mdi/font/scss/icons";
@import "~@mdi/font/scss/extras";
@import "~@mdi/font/scss/animated";

body {
  user-select: auto;
}
</style>
