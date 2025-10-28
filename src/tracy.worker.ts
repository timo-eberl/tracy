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
	const { command, width, height, camera } = event.data as {
		command: 'renderFast' | 'renderFull', width:number, height:number, camera:CameraProperties
	};

	// Await initialization of WebAssembly Module
	const Module = await modulePromise;

	let renderFunction;
	if (command === 'renderFast') {
		renderFunction = Module._render_fast;
	}
	else if (command === 'renderFull') {
		renderFunction = Module._render_full;
	}
	else {
		console.error("Worker: Unknown command:", command);
		return;
	}

	const bufferPtr = renderFunction(
		width, height, degToRad(camera.rotation.x), degToRad(camera.rotation.y),
		camera.distance, camera.focusPoint.x, camera.focusPoint.y, camera.focusPoint.z
	);

	// Send the result back to the main thread.
	// The second argument is a list of "Transferable Objects".
	// This transfers ownership of the buffer's memory to the main thread instead of
	// copying it, which is extremely fast.
	self.postMessage(
		{ sharedMemory, bufferPtr, width, height }
	);
};
