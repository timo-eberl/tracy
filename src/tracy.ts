import ModuleFactory, { MainModule } from './tracy_c';

// public API of our module
export interface TracyModule {
	renderFast: (width: number, height: number, camera: CameraProperties) => void;
	renderFull: (width: number, height: number, camera: CameraProperties) => void;
}

export interface CameraProperties {
	rotation: {
		x: number;
		y: number;
	};
	distance: number;
	focusPoint: {
		x: number;
		y: number;
		z: number;
	};
}

export async function create(context: CanvasRenderingContext2D): Promise<TracyModule> {
	// Await initialization of WebAssembly Module
	const Module = await ModuleFactory();

	return {
		renderFast: (width, height, camera) => {
			const bufferPtr = Module._render_fast(
				width, height, degToRad(camera.rotation.x), degToRad(camera.rotation.y),
				camera.distance, camera.focusPoint.x, camera.focusPoint.y, camera.focusPoint.z
			);
			updateImage(Module, context, bufferPtr, width, height);
		},
		renderFull: (width, height, camera) => {
			const bufferPtr = Module._render_full(
				width, height, degToRad(camera.rotation.x), degToRad(camera.rotation.y),
				camera.distance, camera.focusPoint.x, camera.focusPoint.y, camera.focusPoint.z
			);
			updateImage(Module, context, bufferPtr, width, height);
		}
	};
}

function degToRad(degree: number) { return degree / 360 * 2 * Math.PI; }

function updateImage(
	Module: MainModule, context: CanvasRenderingContext2D, bufferPtr: number,
	width: number, height: number
) {
	// Create a view into the WebAssembly memory
	// HEAPU8.buffer is "all of memory", bufferPtr is an offset on it
	const rawArray = new Uint8ClampedArray(Module.HEAPU8.buffer, bufferPtr, width * height * 4);

	// Create an ImageData object from the raw pixel data
	const imageData = new ImageData(rawArray, width, height);

	// Draw the ImageData onto the canvas
	context.putImageData(imageData, 0, 0);
}
