import ModuleFactory from './ray_tracer.js';

function degToRad(degree) { return degree / 360 * 2 * Math.PI; }

export async function initTracy(context) {
	// Await initialization of WebAssembly Module
	const Module = await ModuleFactory();

	const ray_tracer = {};
	// Get access to the exported C functions
	ray_tracer.renderFull = Module.cwrap(
		'render_full', // function name
		'number', // return type
		['number', 'number', 'number', 'number', 'number', 'number', 'number', 'number'] // params
	);
	ray_tracer.renderFast = Module.cwrap(
		'render_fast', // function name
		'number', // return type
		['number', 'number', 'number', 'number', 'number', 'number', 'number', 'number'] // params
	);

	return {
		/**
		 * Renders a simple, fast preview.
		 */
		renderFast: (width, height, cameraRotation, cameraDistance, cameraFocusPoint) => {
			const bufferPtr = ray_tracer.renderFast(
				width, height, degToRad(cameraRotation.x), degToRad(cameraRotation.y),
				cameraDistance, cameraFocusPoint.x, cameraFocusPoint.y, cameraFocusPoint.z
			);
			updateImage(Module, context, bufferPtr, width, height);
		},

		/**
		 * Renders a full-quality image.
		 */
		renderFull: (width, height, cameraRotation, cameraDistance, cameraFocusPoint) => {
			const bufferPtr = ray_tracer.renderFull(
				width, height, degToRad(cameraRotation.x), degToRad(cameraRotation.y),
				cameraDistance, cameraFocusPoint.x, cameraFocusPoint.y, cameraFocusPoint.z
			);
			updateImage(Module, context, bufferPtr, width, height);
		}
	};
}

function updateImage(Module, ctx, bufferPtr, width, height) {
	// Create a view into the WebAssembly memory
	// HEAPU8.buffer is "all of memory", bufferPtr is an offset on it
	const rawArray = new Uint8ClampedArray(Module.HEAPU8.buffer, bufferPtr, width * height * 4);

	// Create an ImageData object from the raw pixel data
	const imageData = new ImageData(rawArray, width, height);

	// Draw the ImageData onto the canvas
	ctx.putImageData(imageData, 0, 0);
}
