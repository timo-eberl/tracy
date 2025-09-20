import { defineConfig } from 'vite'

export default defineConfig({
  server: {
    watch: {
      // Tell Vite's file watcher to ignore all files ending in .c and .wasm
      // This mitigates duplicate reloads, when the c, wasm, and later the js file updates
      ignored: ['**/*.c'],
      ignored: ['**/*.wasm'],
    },
  },
})
