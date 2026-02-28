/// <reference lib="webworker" />
// tell TypeScript this file is in a worker context -> avoids compile error

import { RenderSettings, RenderStatus } from './tracy';
import ModuleFactory, { MainModule } from './tracy_c';

// We create a shared memory that we use to initialize our wasm module with
// This will be the memory that wasm uses for everything (radiance buffer, call stack, everything)
const sharedMemory = new WebAssembly.Memory({ initial: 256, maximum: 8192, shared: true });
// This promise ensures the Wasm module is initialized only once.
const modulePromise: Promise<MainModule> = ModuleFactory({ wasmMemory: sharedMemory });

function degToRad(degree: number) { return degree / 360 * 2 * Math.PI; }

// Listen for messages from the main thread
self.onmessage = async (event) => {
	const s = event.data as RenderSettings;

	// Await initialization of WebAssembly Module
	const Module = await modulePromise;

	Module._render_init(
		s.scene, s.maxDepth, s.width, s.height, s.filterType,
		degToRad(s.camera.rotation.x), degToRad(s.camera.rotation.y), s.camera.distance,
		s.camera.focusPoint.x, s.camera.focusPoint.y, s.camera.focusPoint.z
	);

	let samplesPerRun = 1;
	let samplesRemaining = s.samplesPerPixel;
	let status: RenderStatus = {
		finished: false,
		samplesCompleted: 0,
		timeTakenMs: 0,
	};

	const startTime = performance.now();

	while (samplesRemaining > 0) {
		// either a full run, or whatever is left
		const samplesForThisRun = Math.min(samplesRemaining, samplesPerRun);
		Module._render_refine(samplesForThisRun);
		const bufferPtr = Module._update_image_ldr();
		samplesRemaining -= samplesForThisRun;

		status.finished = (samplesRemaining === 0);
		status.samplesCompleted += samplesForThisRun;
		status.timeTakenMs = performance.now() - startTime;

		self.postMessage({
			sharedMemory, bufferPtr, width: s.width, height: s.height, status
		});

		samplesPerRun++;
	}
};