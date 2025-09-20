import { initTracy } from "./tracy";

// Get the canvas and its 2D rendering context
const canvas = document.querySelector("canvas");
const context = canvas.getContext("2d");
let width = canvas.clientWidth;
let height = canvas.clientHeight;

window.onresize = function() {
	width = canvas.clientWidth;
	height = canvas.clientHeight;
}

const cameraDistanceBounds = { min: 0.01, max: 5000 };
const cameraRotationXBounds = { min: -89.9, max: 89.9 };
const cameraFocusPoint = { x: 0, y: 1.25, z: 0 };
let cameraRotation = { x: 2.44, y: 0 };
let cameraDistance = 5.5;
let isMouseDown = false;
let isMouseOver = false;
let preview = false;
let drawPreviewLoopHandle;

setupCameraControls();

const Tracy = await initTracy(context);

startPreview();
stopPreview();

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
	canvas.width = width/4;
	canvas.height = height/4;

	Tracy.renderFast(width/4, height/4, cameraRotation, cameraDistance, cameraFocusPoint);

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
	
			Tracy.renderFull(width, height, cameraRotation, cameraDistance, cameraFocusPoint);

			const endTime = performance.now();
			console.log(`Full render took ${(endTime-startTime)} ms.`);

			canvas.style.border = "";
		}, 10.0);
	}
}
