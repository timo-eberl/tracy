/// <reference lib="webworker" />
// tell TypeScript this file is in a worker context -> avoids compile error

import ModuleFactory, { MainModule } from './tracy_c';
import type { CameraProperties } from './tracy'; // Assuming you export the types

// This promise ensures the Wasm module is initialized only once.
const modulePromise: Promise<MainModule> = ModuleFactory();

function degToRad(degree: number) { return degree / 360 * 2 * Math.PI; }

// Listen for messages from the main thread
self.onmessage = async (event) => {
	const { command, width, height, camera } = event.data;

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

	// Create a view into the WebAssembly memory
	// HEAPU8.buffer is "all of memory", bufferPtr is an offset on it
	const rawArray = new Uint8ClampedArray(Module.HEAPU8.buffer, bufferPtr, width * height * 4);

	const copiedImageArray = rawArray.slice(); // create a true copy

	// Send the result back to the main thread.
	// The second argument is a list of "Transferable Objects".
	// This transfers ownership of the buffer's memory to the main thread instead of
	// copying it, which is extremely fast.
	self.postMessage(
		{ imageData: copiedImageArray.buffer, width, height }, [copiedImageArray.buffer]
	);
};
