import * as Tracy from "../src/tracy";

// Get the canvas and its 2D rendering context
const canvas = document.querySelector("canvas") as HTMLCanvasElement;
const context = canvas.getContext("2d") as CanvasRenderingContext2D;
let width = canvas.clientWidth;
let height = canvas.clientHeight;

window.onresize = function () {
	width = canvas.clientWidth;
	height = canvas.clientHeight;
}

const cameraDistanceBounds = { min: 0.01, max: 5000 };
const cameraRotationXBounds = { min: -89.9, max: 89.9 };
const camera: Tracy.CameraProperties = {
	rotation: { x: 2.44, y: 0 },
	distance: 5.5,
	focusPoint: { x: 0, y: 1.25, z: 0 },
};

let tracy: Tracy.TracyModule;
let isMouseDown = false;

// Simple state flags
let isRendering = false;
let previewRequested = false;

async function main() {
	setupCameraControls();
	tracy = Tracy.create(context);

	// Setup Button Controls
	document.getElementById("btn-start")!.onclick = () => {
		if (!isRendering) {
			isRendering = true;
			drawFull();
		}
	};

	document.getElementById("btn-stop")!.onclick = () => {
		if (isRendering) {
			tracy.cancel();
			// We don't need to set isRendering = false here, 
			// because tracy.cancel() resolves the promise in drawFull() which handles cleanup.
		}
	};

	// draw preview once on load
	await drawPreview();

	requestAnimationFrame(renderLoop);
}

function clamp(v: number, min: number, max: number) { return Math.min(Math.max(v, min), max); }

function setupCameraControls() {
	// rotate with mouse (left mouse button)
	canvas.onmousedown = function (event) {
		// Only allow camera changes if we are not currently doing a full render
		if (event.button === 0 && !isRendering) {
			isMouseDown = true;
		}
	};
	document.onmouseup = function (event) {
		if (event.button === 0) {
			isMouseDown = false;
		}
	};
	document.onmousemove = function (event) {
		if (isMouseDown && !isRendering) {
			camera.rotation.x = clamp(
				camera.rotation.x + event.movementY * 0.2,
				cameraRotationXBounds.min, cameraRotationXBounds.max
			);
			camera.rotation.y -= event.movementX * 0.2;
			previewRequested = true;
		}
	};
	// zoom with mouse wheel
	canvas.onwheel = function (event) {
		if (!isRendering) {
			let delta = -event.deltaY / 120.0; // [-1;1]
			delta *= -0.08;
			delta += 1.0; // [0.9 - 1.1]
			camera.distance *= delta;

			// camera restrictions
			camera.distance = Math.max(cameraDistanceBounds.min, camera.distance);
			camera.distance = Math.min(cameraDistanceBounds.max, camera.distance);
			previewRequested = true;
		}
	};
}

async function drawPreview() {
	await tracy.renderFast({
		scene: 1,
		maxDepth: 2,
		width: width / 2,
		height: height / 2,
		filterType: 1,
		camera: {
			rotation: camera.rotation,
			distance: camera.distance,
			focusPoint: camera.focusPoint
		},
		samplesPerPixel: 3
	});
}

async function drawFull() {
	canvas.classList.add('animated-border');

	const num_samples = Infinity;

	console.log(`Starting full render with ${num_samples} samples...`)
	const startTime = performance.now();

	// If tracy.cancel() is called, this promise immediately resolves cleanly.
	await tracy.renderFull({
		scene: 1,
		maxDepth: 6,
		width: width,
		height: height,
		filterType: 0,
		camera: {
			rotation: camera.rotation,
			distance: camera.distance,
			focusPoint: camera.focusPoint
		},
		samplesPerPixel: num_samples
	});

	const endTime = performance.now();
	console.log(`Full render finished or was cancelled. Took ${(endTime - startTime)} ms.`);

	canvas.classList.remove('animated-border');
	isRendering = false; // Reset state
}

async function renderLoop() {
	// Only process previews if the user moved the camera and we aren't rendering
	if (!isRendering && previewRequested) {
		previewRequested = false;
		try {
			await drawPreview();
		} catch(e) {
			// Ignore rejection if a preview overlaps unexpectedly
		}
	}
	
	requestAnimationFrame(renderLoop);
}

main();
