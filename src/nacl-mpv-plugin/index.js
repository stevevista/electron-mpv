const mime = 'application/x-mpv'

const EXT = process.platform === 'win32' ? '.dll' : '.so'
const pluginFilename = `mpv-${process.platform}-${process.arch}-pepper_49${EXT}`

module.exports = {
  mime,
  pluginFilename
}
