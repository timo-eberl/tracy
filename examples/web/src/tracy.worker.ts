/// <reference lib="webworker" />
// tell TypeScript this file is in a worker context -> avoids compile error

import { CameraProperties } from './tracy';
import ModuleFactory, { MainModule } from './tracy_c';

// We create a shared memory that we use to initialize our wasm module with
// This will be the memory that wasm uses for everything (radiance buffer, call stack, everything)
const sharedMemory = new WebAssembly.Memory({ initial: 256, maximum: 256, shared: true });
// This promise ensures the Wasm module is initialized only once.
const modulePromise: Promise<MainModule> = ModuleFactory({wasmMemory: sharedMemory});

function degToRad(degree: number) { return degree / 360 * 2 * Math.PI; }

// Listen for messages from the main thread
self.onmessage = async (event) => {
	const { command, width, height, camera, samplesPerPixel } = event.data as {
		command: 'renderFast' | 'renderFull', width: number, height: number,
		camera:CameraProperties, samplesPerPixel: number,
	};

	// Await initialization of WebAssembly Module
	const Module = await modulePromise;

	Module._render_init(
		width, height, degToRad(camera.rotation.x), degToRad(camera.rotation.y),
		camera.distance, camera.focusPoint.x, camera.focusPoint.y, camera.focusPoint.z
	);

	if (command === 'renderFast') {
		const bufferPtr = Module._render_fast();

		self.postMessage( { sharedMemory, bufferPtr, width, height, finished: true } );
	}
	else if (command === 'renderFull') {
		const samplesPerRun = 2;
		let samplesRemaining = samplesPerPixel;

		while (samplesRemaining > 0) {
			// either a full run, or whatever is left
			const samplesForThisRun = Math.min(samplesRemaining, samplesPerRun);
			const bufferPtr = Module._render_refine(samplesForThisRun);
			samplesRemaining -= samplesForThisRun;

			const isFinished = (samplesRemaining === 0);
			self.postMessage( { sharedMemory, bufferPtr, width, height, finished: isFinished } );
		}
	}
	else {
		console.error("Worker: Unknown command:", command);
		return;
	}
};
