// public API of our module
export interface TracyModule {
	renderFast: (width: number, height: number, camera: CameraProperties) => Promise<void>;
	renderFull: (width: number, height: number, camera: CameraProperties) => Promise<void>;
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
		const { imageData, width, height } = event.data;

		updateImage(context, imageData, width, height);

		if (resolveCurrentRender) {
			resolveCurrentRender();
			resolveCurrentRender = null;
		}
	};

	function callWorker(
		command: 'renderFast' | 'renderFull', width: number, height: number,
		camera: CameraProperties
	) {
		if (resolveCurrentRender) {
			return Promise.reject(new Error("A render is already in progress."));
		}
		const renderPromise = new Promise<void>((resolve) => {
			resolveCurrentRender = resolve;
		});

		worker.postMessage({ command, width, height, camera });
		return renderPromise;
	}

	return {
		renderFast: (w, h, cam) => callWorker('renderFast', w, h, cam),
		renderFull: (w, h, cam) => callWorker('renderFull', w, h, cam),
	}
};

function updateImage(
	context: CanvasRenderingContext2D, buffer: ArrayBuffer, width: number, height: number
) {
	const rawArray = new Uint8ClampedArray(buffer);

	// Create an ImageData object from the raw pixel data
	const imageData = new ImageData(rawArray, width, height);

	// Draw the ImageData onto the canvas
	context.canvas.width = width;
	context.canvas.height = height;
	context.putImageData(imageData, 0, 0);
}
