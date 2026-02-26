import * as Tracy from "../src/tracy";

const canvas = document.querySelector("canvas") as HTMLCanvasElement;
const context = canvas.getContext("2d") as CanvasRenderingContext2D;

// UI Elements
const uiScene = document.getElementById("ui-scene") as HTMLSelectElement;
const uiRes = document.getElementById("ui-res") as HTMLSelectElement;
const uiDepth = document.getElementById("ui-depth") as HTMLInputElement;
const uiFilter = document.getElementById("ui-filter") as HTMLSelectElement;
const uiSpp = document.getElementById("ui-spp") as HTMLInputElement;
const uiStatus = document.getElementById("ui-status") as HTMLElement;

const cameraDistanceBounds = { min: 0.01, max: 5000 };
const cameraRotationXBounds = { min: -89.9, max: 89.9 };
const camera: Tracy.CameraProperties = {
	rotation: { x: 2.44, y: 0 },
	distance: 5.5,
	focusPoint: { x: 0, y: 1.25, z: 0 },
};

let tracy: Tracy.TracyModule;
let isMouseDown = false;
let cameraChanged = true; // Set to true so it renders immediately on load
let isWaitingForFirstSample = false;

function main() {
	setupCameraControls();
	tracy = Tracy.create(context);

	tracy.onFrame = (samplesCompleted) => { 
		isWaitingForFirstSample = false; 
		uiStatus.innerText = `Samples: ${samplesCompleted}`;
	};
	
	// Start the infinite loop that watches for camera changes
	requestAnimationFrame(renderLoop);
}

function clamp(v: number, min: number, max: number) { return Math.min(Math.max(v, min), max); }

function setupCameraControls() {
	canvas.onmousedown = (event) => { if (event.button === 0) isMouseDown = true; };
	document.onmouseup = (event) => { if (event.button === 0) isMouseDown = false; };
	
	document.onmousemove = (event) => {
		if (isMouseDown) {
			camera.rotation.x = clamp(
				camera.rotation.x + event.movementY * 0.2,
				cameraRotationXBounds.min, cameraRotationXBounds.max
			);
			camera.rotation.y -= event.movementX * 0.2;
			cameraChanged = true; // Signal that the render needs to restart
		}
	};
	
	canvas.onwheel = (event) => {
		let delta = -event.deltaY / 120.0;
		delta = 1.0 - (delta * 0.08);
		camera.distance = clamp(camera.distance * delta, cameraDistanceBounds.min, cameraDistanceBounds.max);
		cameraChanged = true; // Signal that the render needs to restart
	};

	// Listen for UI parameter changes to trigger a new render
	const triggerRender = () => { cameraChanged = true; };
	uiScene.addEventListener("change", triggerRender);
	uiRes.addEventListener("change", triggerRender);
	uiDepth.addEventListener("input", triggerRender);
	uiFilter.addEventListener("change", triggerRender);
	uiSpp.addEventListener("input", triggerRender);
}

async function drawFull() {
	canvas.classList.add('animated-border');

	// Determine render resolution explicitly
	const renderWidth = parseInt(uiRes.value, 10);
	const renderHeight = Math.floor(renderWidth * 0.75); // Maintain 4:3 aspect ratio

	// Read the live UI values right before sending them to the Web Worker
	// If tracy.cancel() is called elsewhere, this await instantly resolves cleanly
	await tracy.renderFull({
		scene: parseInt(uiScene.value, 10),
		maxDepth: parseInt(uiDepth.value, 10),
		width: renderWidth,
		height: renderHeight,
		filterType: parseInt(uiFilter.value, 10),
		camera: {
			rotation: camera.rotation,
			distance: camera.distance,
			focusPoint: camera.focusPoint
		},
		samplesPerPixel: parseInt(uiSpp.value, 10)
	});

	canvas.classList.remove('animated-border');
}

function renderLoop() {
	// Only restart if we aren't currently waiting for the first draw
	if (cameraChanged && !isWaitingForFirstSample) {
		cameraChanged = false;
		isWaitingForFirstSample = true;

		tracy.cancel(); // Instantly kill the ongoing render worker
		drawFull();     // Fire-and-forget a new progressive render
	}
	
	requestAnimationFrame(renderLoop);
}

main();
