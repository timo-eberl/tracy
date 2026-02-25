# Tracy

Path Tracer written in C. An interactive web version is deployed [here](https://tracy.timoeberl.de/).

It started as a learning project with the purpose to apply and understand the topics from the lecture "Moderne Techniken der Bildberechnung" ("modern rendering techniques").

It is actively being developed with a focus on automated testing and profiling in the context of the lecture "System Engineering and Management".

## Benchmark Dashboard

As part of the project's focus on automated profiling, it features a fully automated CI/CD pipeline that tracks rendering performance, runtime trends, and image quality (using relMSE) across commits. 

You can view the automatically generated dashboard, containing convergence plots and visual difference maps against Mitsuba reference renders, on the [**`benchmarks` branch**](https://github.com/timo-eberl/tracy/tree/benchmarks).

## Project Structure

* **`src/` & `include/`**: The core path tracer implementation written in C.
* **`examples/`**: Frontends and wrappers demonstrating how to use the C library:
  * **`web/`**: The interactive web application (TypeScript, Vite) that runs the renderer via WebAssembly (Emscripten).
  * **`c_render/` & `zig_render/`**: Native command-line interfaces demonstrating rendering and EXR/TGA image generation.
* **`tests/`**: White-box unit tests written in Zig, alongside the relative Mean Squared Error (relMSE) metric calculators and benchmark runners.
* **`scripts/`**: Python and Bash automation scripts for running benchmarks, calculating EXR differences, and generating plots for the dashboard.
* **`mitsuba_scenes/`**: XML scene definitions for the Mitsuba 3 renderer, used to generate ground-truth reference images for the benchmarks.
* **`.github/`**: CI/CD pipelines that automatically build Docker images, run benchmarks, and update the performance dashboard.

## Native Development

The project includes usage examples in both C and Zig.

### Building with Zig (Cross-Platform)

Zig handles the compilation of the C code, the Zig example, and runs the test suite.

```bash
# Build and run the C CLI example
zig build run-c -Doptimize=ReleaseFast -Dtarget=native

# Build and run the Zig CLI example
zig build run-zig -Doptimize=ReleaseFast -Dtarget=native
```

You can optionally specify those parameters:

| Option                   | Functionality                                 |
| :----------------------- | :-------------------------------------------- |
| `-Dmultithreaded=true`   | Enables Multi-Threading using OpenMP          |
| `-Drussianroulette=true` | Enables Russian Roulette termination strategy |

## Unit Testing

This project uses Zig as a test runner to perform white-box unit testing on the C implementation. The unit tests are located in `tests/unit`.

```bash
# Run all unit tests
zig build test

# Run tests and print a summary of passed/failed tests
zig build test --summary all
```

## Performance Testing

The render benchmark runs multiple rendering iterations, saves the images as exr, computes the relMSE (relative mean square error) after each iteration and prints them to console. Configured in `render_config.yml`

```bash
python scripts/run_benchmarks.py
```

## Mitsuba Reference

`mitsuba_scenes` contains scene descriptions for the Mitsuba 3 renderer that match the scenes in our renderer exactly. To render it install Mitsuba 3 and run:

```bash
mitsuba mitsuba_scenes/scene_name/scene.xml
```

## Web Application

Simple, interactive web app that uses tracy (compiled to WebAssembly).

### Dependencies

- [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html): The compiler toolchain for compiling C to WebAssembly
- [Node.js and npm](https://nodejs.org/): Used to manage development dependencies and run automation scripts

### Getting Started

```bash
cd examples/web/
```

Install local development tools (`live-server`, `nodemon`, etc.):
```bash
npm install
```

Perform C-to-WASM compilation, start a local web server, and watch for file changes:
```bash
npm run dev
```

### Debugging

You can debug your C code with Chrome and the [C/C++ DevTools Support (DWARF)](https://chromewebstore.google.com/detail/cc++-devtools-support-dwa/pdcpmagijalfljmkmjngeonclgbbannb) extension. Firefox's debugger doesn't work properly.

```bash
npm run dev:debug
```

Open the developer tools and navigate to `Sources`. You should see the C files and be able to debug them.

### Release Build

```bash
npm run vite:build
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
- [x] Incremental rendering (live update image)
