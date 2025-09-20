import ModuleFactory from './ray_tracer.js';

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

const cameraDistanceBounds = { min: 0.01, max: 5000 };
const cameraRotationXBounds = { min: -89.9, max: 89.9 };
const cameraFocusPoint = { x: 0, y: 1.25, z: 0 };
let cameraRotation = { x: 2.44, y: 0 };
let cameraDistance = 5.5;
let isMouseDown = false;
let isMouseOver = false;
let preview = false;
const tracy = { renderFull: undefined, renderFast: undefined };
let drawPreviewLoopHandle;

setupCameraControls();

// Await initialization of WebAssembly Module
const Module = await ModuleFactory();

// Get access to the exported C functions
tracy.renderFull = Module.cwrap(
	'render_full', // function name
	'number', // return type
	['number', 'number', 'number', 'number', 'number', 'number', 'number', 'number'] // params
);
tracy.renderFast = Module.cwrap(
	'render_fast', // function name
	'number', // return type
	['number', 'number', 'number', 'number', 'number', 'number', 'number', 'number'] // params
);

startPreview();
stopPreview();

function drawPreview() {
	const bufferPtr = tracy.renderFast(
		width, height, degToRad(cameraRotation.x), degToRad(cameraRotation.y),
		cameraDistance, cameraFocusPoint.x, cameraFocusPoint.y, cameraFocusPoint.z
	);
	updateImage(bufferPtr);
}

function drawFull() {
	const bufferPtr = tracy.renderFull(
		width, height, degToRad(cameraRotation.x), degToRad(cameraRotation.y),
		cameraDistance, cameraFocusPoint.x, cameraFocusPoint.y, cameraFocusPoint.z
	);
	updateImage(bufferPtr);
}

function updateImage(bufferPtr) {
	// Create a view into the WebAssembly memory
	// HEAPU8.buffer is "all of memory", bufferPtr is an offset on it
	const rawArray = new Uint8ClampedArray(Module.HEAPU8.buffer, bufferPtr, width * height * 4);

	// Create an ImageData object from the raw pixel data
	const imageData = new ImageData(rawArray, width, height);

	// Draw the ImageData onto the canvas
	ctx.putImageData(imageData, 0, 0);
}

function degToRad(degree) { return degree / 360 * 2 * Math.PI; }
function clamp(v, min, max) { return Math.min( Math.max(v, min), max ); }

function setupCameraControls() {
	// rotate with mouse (left mouse button)
	canvas.onmousedown = function(event) {
		if (event.button === 0) {
			isMouseDown = true;
			startPreview();
		}
	};
	document.onmouseup = function(event) {
		if (event.button === 0) {
			isMouseDown = false
			if (!isMouseOver) {
				stopPreview();
			}
		}
	};
	document.onmousemove = function(event) {
		if (isMouseDown) {
			cameraRotation.x = clamp(
				cameraRotation.x + event.movementY * 0.2,
				cameraRotationXBounds.min, cameraRotationXBounds.max
			);
			cameraRotation.y -= event.movementX * 0.2;
		}
	};
	// zoom with mouse wheel
	canvas.onwheel = function (event) {
		startPreview();
		let delta = event.wheelDeltaY / 120.0; // [-1;1]
		delta *= -0.08;
		delta += 1.0; // [0.9 - 1.1]
		cameraDistance *= delta;

		// camera restrictions
		cameraDistance = Math.max(cameraDistanceBounds.min, cameraDistance);
		cameraDistance = Math.min(cameraDistanceBounds.max, cameraDistance);
	};
	canvas.onmouseover = function () { isMouseOver = true; };
	canvas.onmouseout = function () {
		isMouseOver = false;
		if (!isMouseDown) {
			stopPreview();
		}
	};
};

function drawPreviewLoop() {
	drawPreview();

	if (preview) {
		drawPreviewLoopHandle = requestAnimationFrame(drawPreviewLoop);
	}
}

function startPreview() {
	if (!preview) {
		preview = true;
		drawPreviewLoop();
	}
}

function stopPreview() {
	if (preview) {
		if (drawPreviewLoopHandle) {
			cancelAnimationFrame(drawPreviewLoopHandle);
		}
		preview = false;
		canvas.style.border = "dashed red 5px";
		console.log("Starting full render...")
		setTimeout(function () {
			const startTime = performance.now();
			drawFull();
			const endTime = performance.now();
			console.log(`Full render took ${(endTime-startTime)} ms.`);

			canvas.style.border = "";
		}, 10.0);
	}
}
