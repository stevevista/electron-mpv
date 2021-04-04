const { getConfig, dev } = require('./webpack.config.base');
const { spawn, execSync } = require('child_process');

let electronProcess;

const mainConfig = getConfig({
  target: 'electron-main',
  devtool: dev ? 'inline-source-map' : 'none',
  watch: dev,
  entry: {
    main: dev ? ['./src/main/debug-support', './src/main'] :
              ['./src/main'],
  },
  plugins: [
    // new BundleAnalyzerPlugin(),
  ]
});

const preloadConfig = getConfig({
  target: 'electron-renderer',
  devtool: 'none',
  watch: dev,
  entry: {
    'preload': './src/renderer/preload',
  },
  plugins: [],
});

if (process.env.START === '1') {
  mainConfig.plugins.push({
    apply: (compiler) => {
      compiler.hooks.afterEmit.tap('AfterEmitPlugin', () => {
        if (electronProcess) {
          try {
            if (process.platform === 'win32') {
              execSync(`taskkill /pid ${electronProcess.pid} /f /t`);
            } else {
              electronProcess.kill();
            }

            electronProcess = null;
          } catch (e) {}
        }

        electronProcess = spawn('npm', ['start'], {
          shell: true,
          env: process.env,
          stdio: 'inherit',
        });
      });
    },
  });
}

module.exports = [mainConfig, preloadConfig];
