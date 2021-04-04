module.exports = {
  root: true,
  parser: "babel-eslint",
  parserOptions: {
    "ecmaVersion": 2017,
    sourceType: 'module'
  },
  env: {
    browser: true,
    node: true
  },
  extends: ['standard'],
  globals: {
    __static: true
  },
  plugins: [
    'html'
  ],
  'rules': {
    // allow paren-less arrow functions
    'arrow-parens': 0,
    // allow async-await
    'generator-star-spacing': 0,
    // allow debugger during development
    'no-debugger': process.env.NODE_ENV === 'production' ? 2 : 0,

    'semi': 0,
    'no-trailing-spaces': 0,
    'camelcase': 0,
    'no-useless-escape': 0,
    'standard/no-callback-literal': 0
  }
}
