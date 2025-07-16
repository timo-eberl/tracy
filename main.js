// Get the canvas and its 2D rendering context
const canvas = document.querySelector("canvas");
const ctx = canvas.getContext("2d");
const width = canvas.clientWidth;
const height = canvas.clientHeight;
canvas.width = width;
canvas.height = height;

// Emscripten's Module object becomes available after the script loads
Module.onRuntimeInitialized = () => {
	// Get access to the exported C functions
	const render = Module.cwrap('render', null, ['number', 'number']);
	const getImageBuffer = Module.cwrap('get_image_buffer', 'number', ['number', 'number']);


	const startTime = performance.now();
	
	render(width, height);

	const endTime = performance.now();
	console.log(`Done! Rendering took ${(endTime - startTime).toFixed(2)} ms.`);

	// Get the memory address of the image buffer
	const bufferPtr = getImageBuffer(width, height);

	// Create a view into the WebAssembly memory
	// HEAPU8.buffer is "all of memory", bufferPtr is an offset on it
	const imageDataArray = new Uint8ClampedArray(Module.HEAPU8.buffer, bufferPtr, width * height * 4);

	// Create an ImageData object from the raw pixel data
	const imageData = new ImageData(imageDataArray, width, height);

	// Draw the ImageData onto the canvas
	ctx.putImageData(imageData, 0, 0);
};
