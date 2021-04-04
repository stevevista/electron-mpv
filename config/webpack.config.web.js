const { getConfig } = require('./webpack.config.base');
const { join } = require('path');
const { VueLoaderPlugin } = require('vue-loader')
const HtmlWebpackPlugin = require('html-webpack-plugin');

const webConfig = getConfig({
  target: 'web',

  devServer: {
    contentBase: join(__dirname, '..', 'build'),
    port: process.env.WEB_PORT,
    hot: true,
    inline: true,
    disableHostCheck: true
  },

  plugins: [
    new VueLoaderPlugin(),
  ],

  externals: { electron: 'require("electron")' },

  output: {},
  entry: {},
  optimization: {
    splitChunks: {
      chunks: 'all',
      maxInitialRequests: Infinity,
    }
  }
});

const applyEntries = (config, entries) => {
  for (const entry of entries) {
    config.entry[entry] = [`./src/renderer/${entry}`];
    config.plugins.push(new HtmlWebpackPlugin({
      title: 'Bella',
      template: 'src/renderer/app-themed.html',
      filename: `${entry}.html`,
      chunks: [entry],
    }));
  }
}

applyEntries(webConfig, [
  'app',
  'settings',
  'player',
]);

module.exports = webConfig;
