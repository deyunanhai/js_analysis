/**
 * Helpers made available via require('js_analysis') once package is
 * installed.
 */

var path = require('path')

exports.path = process.platform === 'win32' ?
    path.join(__dirname, 'release', 'js_analysis.exe') :
    path.join(__dirname, 'release' ,'js_analysis')


exports.version = '0.1'
