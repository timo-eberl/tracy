// Get the canvas and its 2D rendering context
const canvas = document.querySelector("canvas");
const ctx = canvas.getContext("2d");
let width = canvas.clientWidth;
let height = canvas.clientHeight;
canvas.width = width;
canvas.height = height;

window.onresize = function() {
	width = canvas.clientWidth;
	height = canvas.clientHeight;
	canvas.width = width;
	canvas.height = height;
}

const cameraDistanceBounds = { min: 1.1, max: 40 };
const cameraRotationXBounds = { min: 0, max: 90 };
const cameraFocusPoint = { x: 0, y: 0.0, z: 0 };
let cameraRotation = { x: 15, y: 0 };
let cameraDistance = 5.0;
let isMouseDown = false;

setupCameraControls();

// Emscripten's Module object becomes available after the script loads
Module.onRuntimeInitialized = () => {
	// Get access to the exported C functions
	const render = Module.cwrap('render', null,
		['number', 'number', 'number', 'number', 'number', 'number', 'number', 'number']
	);
	const getImageBuffer = Module.cwrap('get_image_buffer', 'number',
		['number', 'number']
	);
	update(render, getImageBuffer);
};

function update(render, getImageBuffer) {
	const startTime = performance.now();

	render(
		width, height, degToRad(cameraRotation.x), degToRad(cameraRotation.y),
		cameraDistance, cameraFocusPoint.x, cameraFocusPoint.y, cameraFocusPoint.z
	);

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

	requestAnimationFrame(() => update(render, getImageBuffer))
};

function degToRad(degree) { return degree / 360 * 2 * Math.PI; }
function clamp(v, min, max) { return Math.min( Math.max(v, min), max ); }

function setupCameraControls() {
	// rotate with mouse
	document.onmousedown = function(event) { isMouseDown = true };
	document.onmouseup = function(event) { isMouseDown = false };
	document.onmousemove = function(event) {
		if (isMouseDown) {
			cameraRotation.x += event.movementY * 0.4;
			cameraRotation.y += event.movementX * 0.4;
		}
	};
	// zoom with mouse wheel
	document.onwheel = function (event) {
		let delta = event.wheelDeltaY / 120.0; // [-1;1]
		delta *= -0.08;
		delta += 1.0; // [0.9 - 1.1]
		cameraDistance *= delta;
	}
	// camera restrictions
	cameraDistance = Math.max(cameraDistanceBounds.min, cameraDistance);
	cameraDistance = Math.min(cameraDistanceBounds.max, cameraDistance);
	cameraRotation.x = clamp(cameraRotation.x, cameraRotationXBounds.min, cameraRotationXBounds.max);
};
