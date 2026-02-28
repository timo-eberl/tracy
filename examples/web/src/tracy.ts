// public API of our module
export interface TracyModule {
	render: (settings: RenderSettings) => Promise<void>;
	cancel: () => void;
	onFrame?: (status: RenderStatus) => void;
}

export interface RenderSettings {
	scene: number,
	maxDepth: number,
	width: number,
	height: number,
	filterType: number,
	camera: CameraProperties,
	samplesPerPixel: number,
}

export interface RenderStatus {
	samplesCompleted: number,
	finished: boolean,
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
	let worker: Worker;

	let resolveCurrentRender: ((value: void) => void) | null = null;

	// Create the API object first so the worker can access api.onFrame
	const api: TracyModule = {
		render: (s) => callWorker(s),
		cancel: cancel
	};

	// Encapsulate worker initialization so we can respawn it
	function initWorker() {
		// Create the worker. The new URL(...) syntax is the modern standard for module-based workers.
		// This requires a bundler (like Vite, Webpack, Parcel) to work correctly.
		worker = new Worker(new URL('./tracy.worker.ts', import.meta.url), { type: 'module' });

		// Listen for messages (a rendered image) from the worker
		worker.onmessage = (event) => {
			const { sharedMemory, bufferPtr, width, height, status } = event.data as {
				sharedMemory: WebAssembly.Memory; bufferPtr: number; width: number; height: number;
				status: RenderStatus;
			};
			// Create a view into the WebAssembly memory
			// sharedMemory.buffer is "all of memory", bufferPtr is an offset on it
			const rawArray = new Uint8ClampedArray(sharedMemory.buffer, bufferPtr, width * height * 4);
			updateImage(context, rawArray, width, height);

			if (api.onFrame) api.onFrame(status);

			if (status.finished && resolveCurrentRender) {
				resolveCurrentRender();
				resolveCurrentRender = null;
			}
		};
	}

	initWorker();

	function cancel() {
		if (resolveCurrentRender) {
			worker.terminate(); // Force-kill the blocked thread
			initWorker();       // Instantly respawn a fresh worker & WASM environment

			resolveCurrentRender(); // Resolve the pending promise so the app doesn't hang
			resolveCurrentRender = null;
		}
	}

	function callWorker(s: RenderSettings) {
		if (resolveCurrentRender) {
			return Promise.reject(new Error("A render is already in progress."));
		}
		const renderPromise = new Promise<void>((resolve) => {
			resolveCurrentRender = resolve;
		});

		worker.postMessage(s);
		return renderPromise;
	}

	return api;
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
