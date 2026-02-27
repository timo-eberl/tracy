import { defineConfig } from 'vite'

export default defineConfig({
  // Emscripten's multithreading outputs Web Workers using ES module syntax.
  // Vite defaults to classic 'iife' workers which break on top-level await.
  // Setting this to 'es' forces Vite to bundle workers as native ES modules.
  worker: {
    format: 'es',
  },
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
