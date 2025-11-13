import * as Tracy from "../src/tracy";

// Get the canvas and its 2D rendering context
const canvas = document.querySelector("canvas") as HTMLCanvasElement;
const context = canvas.getContext("2d") as CanvasRenderingContext2D;
let width = canvas.clientWidth;
let height = canvas.clientHeight;

window.onresize = function() {
	width = canvas.clientWidth;
	height = canvas.clientHeight;
}

const cameraDistanceBounds = { min: 0.01, max: 5000 };
const cameraRotationXBounds = { min: -89.9, max: 89.9 };
const camera : Tracy.CameraProperties = {
	rotation: { x: 2.44, y: 0 },
	distance: 5.5,
	focusPoint: { x: 0, y: 1.25, z: 0 },
};
let isMouseDown = false;
let isMouseOver = false;
let tracy : Tracy.TracyModule;

// state
let state
	: "preview"			// previewreal-time interactive preview
	| "waitforrender"	// wait until preview draw is finished
	| "rendering"		// full render is in process
	| "idle"			// full render is done
	= "rendering"		// we immidiately render something

async function main() {
	setupCameraControls();

	tracy = Tracy.create(context);

	// draw preview once, then start full render
	await drawPreview();
	state = "waitforrender";

	requestAnimationFrame(renderLoop);
}

function clamp(v : number, min : number, max : number) { return Math.min( Math.max(v, min), max ); }

function setupCameraControls() {
	// rotate with mouse (left mouse button)
	canvas.onmousedown = function(event) {
		if (event.button === 0) {
			isMouseDown = true;
			if (state === "idle") {
				state = "preview";
			}
		}
	};
	document.onmouseup = function(event) {
		if (event.button === 0) {
			isMouseDown = false
			if (!isMouseOver && state === "preview") {
				startRendering();
			}
		}
	};
	document.onmousemove = function(event) {
		if (isMouseDown && state === "preview") {
			camera.rotation.x = clamp(
				camera.rotation.x + event.movementY * 0.2,
				cameraRotationXBounds.min, cameraRotationXBounds.max
			);
			camera.rotation.y -= event.movementX * 0.2;
		}
	};
	// zoom with mouse wheel
	canvas.onwheel = function (event) {
		if (state === "idle") {
			state = "preview";
		}
		if (state === "preview") {
			let delta = -event.deltaY / 120.0; // [-1;1]
			delta *= -0.08;
			delta += 1.0; // [0.9 - 1.1]
			camera.distance *= delta;

			// camera restrictions
			camera.distance = Math.max(cameraDistanceBounds.min, camera.distance);
			camera.distance = Math.min(cameraDistanceBounds.max, camera.distance);
		}
	};
	canvas.onmouseover = function () { isMouseOver = true; };
	canvas.onmouseout = function () {
		isMouseOver = false;
		if (!isMouseDown && state === "preview") {
			startRendering();
		}
	};
};

async function drawPreview() {
	await tracy.renderFast(
		width/2, height/2,
		{ rotation: camera.rotation, distance: camera.distance, focusPoint: camera.focusPoint }
	);
}

async function drawFull() {
	canvas.classList.add('animated-border');

	console.log("Starting full render...")
	const startTime = performance.now();

	await tracy.renderFull(
		width, height,
		{ rotation: camera.rotation, distance: camera.distance, focusPoint: camera.focusPoint },
		50 // samples per pixel
	);

	const endTime = performance.now();
	console.log(`Full render took ${(endTime-startTime)} ms.`);

	canvas.classList.remove('animated-border');

	state = "idle";
}

async function renderLoop() {
	if (state === "preview") {
		await drawPreview();
	}
	else if (state === "waitforrender") {
		state = "rendering";
		drawFull().then(() => state = "idle" );
	}
	requestAnimationFrame(renderLoop);
}

function startRendering() {
	if (state !== "rendering") {
		state = "waitforrender";
	}
}

main();
