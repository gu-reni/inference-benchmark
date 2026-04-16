module.exports = {
  devServer: {
    port: 8082,
    proxy: {
      '/api/cpp': {
        target: 'http://localhost:8080',
        changeOrigin: true,
        pathRewrite: { '^/api/cpp': '/api' }
      },
      '/api/python': {
        target: 'http://localhost:8081',
        changeOrigin: true,
        pathRewrite: { '^/api/python': '/api' }
      }
    }
  }
}