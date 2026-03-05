import * as Tracy from "../src/tracy";
import { camera, setCamera, setupCameraControls } from "./cameraControls";

// position in array matches scene id
const sceneCameras: Tracy.CameraProperties[] =[
	{ rotation: { x: 0, y: 0 }, distance: 5.5, focusPoint: { x: 0, y: 1.25, z: 0 } },
	{ rotation: { x: 0, y: 0 }, distance: 2.5, focusPoint: { x: 0, y: 0.4, z: 0 } },
	{ rotation: { x: 0.2, y: 0 }, distance: 6, focusPoint: { x: 0, y: 1.25, z: 0 } },
	{ rotation: { x: 0.2, y: 0.2 }, distance: 12, focusPoint: { x: 0, y: 1.3, z: 0 } },
];

const canvas = document.querySelector("canvas") as HTMLCanvasElement;
const context = canvas.getContext("2d") as CanvasRenderingContext2D;

// UI Elements
const uiScene = document.getElementById("ui-scene") as HTMLSelectElement;
const uiRes = document.getElementById("ui-res") as HTMLSelectElement;
const uiDepth = document.getElementById("ui-depth") as HTMLInputElement;
const uiFilter = document.getElementById("ui-filter") as HTMLSelectElement;
const uiSpp = document.getElementById("ui-spp") as HTMLInputElement;
const uiStatus = document.getElementById("ui-status") as HTMLElement;
const uiCamera = document.getElementById("ui-camera") as HTMLElement;

let tracy: Tracy.TracyModule;
let cameraChanged = true; // Set to true so it renders immediately on load

// Our state machine variables
let renderMode: 'none' | 'preview' | 'final' = 'none';
let settleTimer: number | null = null;

function updateCameraUI() {
	uiCamera.innerHTML = `Rot: ${camera.rotation.x.toFixed(2)}, ${camera.rotation.y.toFixed(2)}<br>` +
		`Dist: ${camera.distance.toFixed(2)}<br>` +
		`Focus: ${camera.focusPoint.x.toFixed(2)}, ${camera.focusPoint.y.toFixed(2)}, ${camera.focusPoint.z.toFixed(2)}`;
}

function main() {
	const initialSceneIdx = parseInt(uiScene.value, 10);
	setCamera(sceneCameras[initialSceneIdx]);

	setupCameraControls(canvas, () => { cameraChanged = true; });
	setupUIControls();
	updateCameraUI();

	// Only ONE worker pool, meaning zero OpenMP thread contention!
	tracy = Tracy.create(context);

	tracy.onFrame = (status) => {
		if (renderMode === 'preview') {
			uiStatus.innerText = "⚡ Previewing...";
			uiStatus.className = 'status-rendering';
		} else {
			uiStatus.innerText = [
				status.finished ? "✅ Render Completed" : "⏳ Rendering...",
				`Samples: ${status.samplesCompleted.toLocaleString()}`,
				`Time: ${(status.timeTakenMs / 1000).toFixed(2)}s`
			].join('\n');
			uiStatus.className = status.finished ? 'status-finished' : 'status-rendering';
		}
	};

	// Start the infinite loop that watches for camera changes
	requestAnimationFrame(renderLoop);
}

function setupUIControls() {
	// Listen for UI parameter changes to trigger a new render
	const triggerRender = () => { cameraChanged = true; };
	uiScene.addEventListener("change", () => {
		const sceneIdx = parseInt(uiScene.value, 10);
		setCamera(sceneCameras[sceneIdx]);
		triggerRender();
	});
	uiRes.addEventListener("change", triggerRender);
	uiDepth.addEventListener("input", triggerRender);
	uiFilter.addEventListener("change", triggerRender);
	uiSpp.addEventListener("input", triggerRender);
}

async function doPreview() {
	renderMode = 'preview';
	cameraChanged = false;

	// Determine render resolution explicitly
	const renderWidth = parseInt(uiRes.value, 10);
	const renderHeight = Math.floor(renderWidth * 0.75); // Maintain 4:3 aspect ratio

	// Lock the canvas display size to the target resolution so it doesn't shrink, 
	// and force the browser to upscale it crisply (nearest-neighbor)
	canvas.style.width = `${renderWidth}px`;
	canvas.style.height = `${renderHeight}px`;

	const previewWidth = Math.max(1, Math.floor(renderWidth / 2));
	const previewHeight = Math.floor(previewWidth * 0.75);

	// Read the live UI values right before sending them to the Web Worker
	await tracy.render({
		scene: parseInt(uiScene.value, 10),
		maxDepth: parseInt(uiDepth.value, 10),
		width: previewWidth,
		height: previewHeight,
		filterType: parseInt(uiFilter.value, 10),
		camera: {
			rotation: camera.rotation,
			distance: camera.distance,
			focusPoint: camera.focusPoint
		},
		samplesPerPixel: 1 // Single sample for fast preview
	});

	// Only release the mode if a cancel() hasn't already intervened
	if (renderMode === 'preview') {
		renderMode = 'none';
	}
}

async function startFinalRender() {
	settleTimer = null;
	// Extra safety check in case the user moved the mouse exactly as the timer fired
	if (cameraChanged) return;

	renderMode = 'final';

	const renderWidth = parseInt(uiRes.value, 10);
	const renderHeight = Math.floor(renderWidth * 0.75);

	// Ensure the screen size stays locked to the final resolution
	canvas.style.width = `${renderWidth}px`;
	canvas.style.height = `${renderHeight}px`;

	await tracy.render({
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

	if (renderMode === 'final') {
		renderMode = 'none';
	}
}

function renderLoop() {
	if (cameraChanged) {
		updateCameraUI();

		// The user moved the camera. Cancel any pending settle timer.
		if (settleTimer !== null) {
			clearTimeout(settleTimer);
			settleTimer = null;
		}

		// Only terminate the worker if we are interrupting a heavy final render
		if (renderMode === 'final') {
			tracy.cancel();
			renderMode = 'none';
		}

		// If nothing is rendering (or the final render was just cancelled), start preview.
		// NOTE: If we are currently in 'preview' mode, we do NOT cancel it! 
		// Previews are so fast we just let them finish gracefully and stack the next one.
		if (renderMode === 'none') {
			doPreview().then(() => {
				// Once the preview completes, if the user hasn't moved the mouse 
				// in the exact fraction of a second it took to render, start the timer
				if (!cameraChanged) {
					settleTimer = window.setTimeout(startFinalRender, 150);
				}
			});
		}
	}

	requestAnimationFrame(renderLoop);
}

main();
