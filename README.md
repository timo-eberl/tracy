# WASM + C Ray Tracer

This project is a simple, interactive 3D ray tracer built with C, compiled to WebAssembly, and rendered in a web browser.

## Dependencies

- [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html): The compiler toolchain for compiling C to WebAssembly
- [Node.js and npm](https://nodejs.org/): Used to manage development dependencies and run automation scripts

## Getting Started

Install local development tools (`live-server`, `nodemon`, etc.):

```bash
npm install
```

Perform C-to-WASM compilation, start a local web server, and watch for file changes:

```bash
npm run dev
```

## Debugging

You can debug your C code with Chrome and the [C/C++ DevTools Support (DWARF)](https://chromewebstore.google.com/detail/cc++-devtools-support-dwa/pdcpmagijalfljmkmjngeonclgbbannb) extension. Firefox's debugger doesn't work properly.

```bash
npm run dev:debug
```

Open the developer tools and navigate to `Sources`. You should see the c files and be able to debug them.
