# WASM + C Ray Tracer

This project is a simple, interactive 3D ray tracer built with C, compiled to WebAssembly, and rendered in a web browser.

It is a learning project with the purpose to apply and understand the topics from the lecture "Moderne Techniken der Bildberechnung".

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

## Release Build

```bash
npm run vite:build
```

## Build a local version (without web)

Create the binary `tracy` that renders an image to `render.ppm`.

```bash
gcc -std=c11 src/render_to_image.c src/tracy.c -o tracy -lm -O3 -march=native -flto
```

## To-Do

> Concepts from the lecture "Moderne Techniken der Bildberechnung". Numbers in brackets indicate the chapter.

- [x] Pinhole camera (5)
- [ ] Ray geometry intersection (6.2)
  - [x] Sphere (6.2.4)
  - [ ] Triangle (6.2.3)
- [x] Tone mapping (4)
  - [x] Gamma correction with sRGB response curve (4.1.9)
  - [x] Global HDR to LDR tonemapping (4.2) using Reinhard
- [x] Anti-Aliasing
  - [x] ~~using supersampling (2.6.2)~~ removed in bba9955d658de2a85d978919f2bd15bbc4dc574f (was integrated into monte carlo rendering process)
  - [x] a gaussian kernel (2.5.6)
- [x] Rendering algorithms
  - [x] ~~Ray casting (5.2.1, 6.1.1, 10.2.2) with local illumination (10.3.13)~~ removed after switching to path tracing
  - [x] ~~Whitted ray tracing (12.1)~~ removed after switching to path tracing
  - [x] Path tracing (13.5)
- [x] Light sources (10.1)
  - [x] ~~Point light source (10.1.5)~~ removed after switching to path tracing
  - [x] Area light source (10.1.7)
- [ ] Material models (11)
  - [x] Lambertian BRDF (11.2.2)
  - [x] Perfect reflection and refraction (11.2.8)
  - [ ] Blinn-Phong BRDF (11.2.6)
  - [ ] Cook-Torrance (11.3.2)
- [ ] Thin lenses (7.3)
- [ ] Optimizations
  - [ ] Spatial data structures (12.3)
  - [ ] Importance Sampling (14.2)
  - [ ] Multithreading
  - [ ] Tiled rendering (Spatial coherency)
  - [ ] Multiple samples per pass (Temporal coherency)
  - [ ] Double Buffering (for incremental rendering)
- [ ] Incremental rendering (live update displayed image)
