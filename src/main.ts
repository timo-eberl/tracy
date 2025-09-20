import * as Tracy from "./tracy";

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
let preview = false;
let drawPreviewLoopHandle : number;
let tracy : Tracy.TracyModule;

async function main() {
	setupCameraControls();

	tracy = await Tracy.create(context);

	startPreview();
	stopPreview();
}

function clamp(v : number, min : number, max : number) { return Math.min( Math.max(v, min), max ); }

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
			camera.rotation.x = clamp(
				camera.rotation.x + event.movementY * 0.2,
				cameraRotationXBounds.min, cameraRotationXBounds.max
			);
			camera.rotation.y -= event.movementX * 0.2;
		}
	};
	// zoom with mouse wheel
	canvas.onwheel = function (event) {
		startPreview();
		let delta = event.deltaY / 120.0; // [-1;1]
		delta *= -0.08;
		delta += 1.0; // [0.9 - 1.1]
		camera.distance *= delta;

		// camera restrictions
		camera.distance = Math.max(cameraDistanceBounds.min, camera.distance);
		camera.distance = Math.min(cameraDistanceBounds.max, camera.distance);
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
	canvas.width = width/4;
	canvas.height = height/4;

	tracy.renderFast(
		width/4, height/4,
		{ rotation: camera.rotation, distance: camera.distance, focusPoint: camera.focusPoint }
	);

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
			canvas.width = width;
			canvas.height = height;

			const startTime = performance.now();
	
			tracy.renderFull(
				width, height, 
				{ rotation: camera.rotation, distance: camera.distance, focusPoint: camera.focusPoint }
			);

			const endTime = performance.now();
			console.log(`Full render took ${(endTime-startTime)} ms.`);

			canvas.style.border = "";
		}, 10.0);
	}
}

main();
