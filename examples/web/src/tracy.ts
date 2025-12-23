// public API of our module
export interface TracyModule {
	renderFast: (settings: RenderSettings) => Promise<void>;
	renderFull: (settings: RenderSettings) => Promise<void>;
}

export interface RenderSettings {
	width: number,
	height: number,
	filterType: number,
	camera: CameraProperties,
	samplesPerPixel: number,
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

export function create(context: CanvasRenderingContext2D): TracyModule {
	// Create the worker. The new URL(...) syntax is the modern standard for module-based workers.
	// This requires a bundler (like Vite, Webpack, Parcel) to work correctly.
	const worker = new Worker(new URL('./tracy.worker.ts', import.meta.url), { type: 'module' });

	let resolveCurrentRender: ((value: void) => void) | null = null;

	// Listen for messages (the final rendered image) from the worker
	worker.onmessage = (event) => {
		const { sharedMemory, bufferPtr, width, height, finished } = event.data as {
			sharedMemory: WebAssembly.Memory; bufferPtr: number; width: number; height: number;
			finished: boolean
		};
		// Create a view into the WebAssembly memory
		// sharedMemory.buffer is "all of memory", bufferPtr is an offset on it
		const rawArray = new Uint8ClampedArray(sharedMemory.buffer, bufferPtr, width * height * 4);

		updateImage(context, rawArray, width, height);

		if (finished && resolveCurrentRender) {
			resolveCurrentRender();
			resolveCurrentRender = null;
		}
	};

	function callWorker(
		command: 'renderFast' | 'renderFull', s: RenderSettings
	) {
		if (resolveCurrentRender) {
			return Promise.reject(new Error("A render is already in progress."));
		}
		const renderPromise = new Promise<void>((resolve) => {
			resolveCurrentRender = resolve;
		});

		worker.postMessage( { command, s } );
		return renderPromise;
	}

	return {
		renderFast: (s) => callWorker('renderFast', s),
		renderFull: (s) => callWorker('renderFull', s),
	}
};

function updateImage(
	context: CanvasRenderingContext2D, arrayView: Uint8ClampedArray, width: number, height: number
) {
	// ImageData requires us to copy the data from the view to the shared memory
	// This is problematic because we potentially write to the buffer on the other thread
	// while copying it. Could be solved with double buffering.
	const arrayCopy = new Uint8ClampedArray(arrayView);
	// Create an ImageData object from the raw pixel data
	const imageData = new ImageData(arrayCopy, width, height);

	// Draw the ImageData onto the canvas
	context.canvas.width = width;
	context.canvas.height = height;
	context.putImageData(imageData, 0, 0);
}
