/// <reference lib="webworker" />
// tell TypeScript this file is in a worker context -> avoids compile error

import { CameraProperties, RenderSettings } from './tracy';
import ModuleFactory, { MainModule } from './tracy_c';

// We create a shared memory that we use to initialize our wasm module with
// This will be the memory that wasm uses for everything (radiance buffer, call stack, everything)
const sharedMemory = new WebAssembly.Memory({ initial: 256, maximum: 256, shared: true });
// This promise ensures the Wasm module is initialized only once.
const modulePromise: Promise<MainModule> = ModuleFactory({wasmMemory: sharedMemory});

function degToRad(degree: number) { return degree / 360 * 2 * Math.PI; }

// Listen for messages from the main thread
self.onmessage = async (event) => {
	const { command, s } = event.data as {
		command: 'renderFast' | 'renderFull', s: RenderSettings,
	};

	// Await initialization of WebAssembly Module
	const Module = await modulePromise;

	Module._render_init(
		s.width, s.height, s.filterType, degToRad(s.camera.rotation.x), degToRad(s.camera.rotation.y),
		s.camera.distance, s.camera.focusPoint.x, s.camera.focusPoint.y, s.camera.focusPoint.z
	);

	if (command === 'renderFast') {
		const bufferPtr = Module._render_refine(2);
		self.postMessage({
			sharedMemory, bufferPtr, width: s.width, height: s.height, finished: true
		});
	}
	else if (command === 'renderFull') {
		let samplesPerRun = 1;
		let samplesRemaining = s.samplesPerPixel;

		while (samplesRemaining > 0) {
			// either a full run, or whatever is left
			const samplesForThisRun = Math.min(samplesRemaining, samplesPerRun);
			const bufferPtr = Module._render_refine(samplesForThisRun);
			samplesRemaining -= samplesForThisRun;

			const isFinished = (samplesRemaining === 0);
			self.postMessage({
				sharedMemory, bufferPtr, width: s.width, height: s.height, finished: isFinished
			});

			samplesPerRun++;
		}
	}
	else {
		console.error("Worker: Unknown command:", command);
		return;
	}
};
