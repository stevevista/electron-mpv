const { resolve } = require('path');
const webpack = require('webpack');
const merge = require('webpack-merge');
const TerserPlugin = require('terser-webpack-plugin');
const HardSourceWebpackPlugin = require('hard-source-webpack-plugin');

const SRC_PATH = resolve(__dirname, '..', 'src');

const BUILD_FLAGS = {
  NODE_WEB_PORT: 8089,
  WEB_PORT: 8088
};

const dev = process.env.DEV === '1';

process.env.NODE_ENV = dev ? 'development' : 'production';

process.env = {
  ...process.env,
  ...BUILD_FLAGS,
};

const config = {
  mode: dev ? 'development' : 'production',
  devtool: dev ? 'eval-source-map' : 'none',

  output: {
    path: resolve(__dirname, '..', 'build'),
    filename: '[name].bundle.js',
  },

  module: {
    rules: [
      {
        test: /\.(png|gif|jpg|svg)$/,
        include: resolve(SRC_PATH, 'renderer/resources/favicons'),
        use: [
          {
            loader: 'file-loader',
            options: {
              esModule: false,
            },
          },
        ],
      },
      {
        test: /\.(png|gif|jpg|woff2|ttf|svg)$/,
        include: SRC_PATH,
        exclude: resolve(SRC_PATH, 'renderer/resources/favicons'),
        use: [
          {
            loader: 'url-loader',
            options: {
              esModule: false,
            },
          },
        ],
      },
      {
        test: /\.(js|vue)$/,
        enforce: 'pre',
        exclude: /node_modules/,
        use: {
          loader: 'eslint-loader',
          options: {
            formatter: require('eslint-friendly-formatter')
          }
        }
      },
      {
        test: /\.vue$/,
        use: {
          loader: 'vue-loader',
          options: {
            extractCSS: process.env.NODE_ENV === 'production',
            loaders: {
              sass: 'vue-style-loader!css-loader!sass-loader?indentedSyntax=1',
              scss: 'vue-style-loader!css-loader!sass-loader',
              less: 'vue-style-loader!css-loader!less-loader'
            }
          }
        }
      },
      {
        test: /\.jsx$/,
        exclude: /node_modules/,
        use: [
          {
            loader: 'babel-loader',
            options: {
              presets: ['@babel/react', '@babel/env'],
              plugins: [
                ['@babel/plugin-proposal-decorators', { "legacy": true } ],
                '@babel/plugin-proposal-class-properties', '@babel/plugin-transform-runtime'
              ],
              sourceType: 'unambiguous'
            }
          }
        ]
      },
      {
        test: /\.js$/,
        exclude: /node_modules/,
        use: [
          {
            loader: 'babel-loader',
            options: {
              presets: ['@babel/env'],
              plugins: [
                ['@babel/plugin-proposal-decorators', { "legacy": true } ],
                '@babel/plugin-proposal-class-properties', '@babel/plugin-transform-runtime'
              ],
              sourceType: 'unambiguous'
            }
          }
        ]
      },
      {
        test: /\.scss$/,
        use: ['vue-style-loader', 'css-loader', 'sass-loader']
      },
      {
        test: /\.sass$/,
        use: ['vue-style-loader', 'css-loader', 
          {
            loader: 'sass-loader',
            // Requires sass-loader@^7.0.0
            options: {
              implementation: require('sass'),
              fiber: require('fibers'),
              indentedSyntax: true // optional
            }
          }
        ]
      },
      {
        test: /\.node$/,
        use: 'node-loader'
      },
      {
        test: /\.css$/,
        use: ['vue-style-loader', 'css-loader']
      },
      {
        test: /\.(woff2?|eot|ttf|otf)(\?.*)?$/,
        include: [resolve(__dirname, '..', 'node_modules/@mdi/font'), resolve(__dirname, '..', 'node_modules/@fontsource/roboto')],
        use: {
          loader: 'url-loader',
          query: {
            limit: 10000,
            name: 'fonts/[name]--[folder].[ext]'
          }
        }
      },
      {
        test: /\.(yml|yaml)$/,
        loader: 'yml-loader'
      }
    ]
  },

  node: {
    __dirname: false,
    __filename: false,
  },

  resolve: {
    extensions: ['.js', '.jsx', '.tsx', '.ts', '.json', '.vue', '.css', '.node'],
    alias: {
      '~': SRC_PATH,
      'adb-api': resolve(SRC_PATH, 'packages/adb'),
      'onvif-api': resolve(SRC_PATH, 'packages/onvif'),
      'nacl-mpv-plugin': resolve(SRC_PATH, 'nacl-mpv-plugin'),
      'vue$': 'vue/dist/vue.esm.js'
    },
  },

  plugins: [
    new webpack.EnvironmentPlugin(['NODE_ENV', ...Object.keys(BUILD_FLAGS)]),
  ],

  externals: {},

  optimization: {
    minimizer:
    !dev
      ? [
          new TerserPlugin({
            extractComments: true,
            terserOptions: {
              ecma: 8,
              output: {
                comments: false,
              },
            },
            parallel: true,
            cache: true,
          }),
        ]
      : []
  }
}

if (dev) {
  config.plugins.push(new HardSourceWebpackPlugin());
}

function getConfig(...cfg) {
  return merge(config, ...cfg);
}

module.exports = { getConfig, dev };
