import { defineConfig } from 'vite'

export default defineConfig({
  server: {
    watch: {
      // Tell Vite's file watcher to ignore all c and wasm files.
      // This mitigates duplicate reloads, when the c and later the js file updates
      ignored: ['**/*.c', '**/*.h', '**/*.wasm', '**/*.wasm.map'],
    },
    headers: {
      'Cross-Origin-Opener-Policy': 'same-origin',
      'Cross-Origin-Embedder-Policy': 'require-corp',
    },
  },
})
