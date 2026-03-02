import * as Tracy from "../src/tracy";

export const cameraDistanceBounds = { min: 0.01, max: 5000 };
export const cameraRotationXBounds = { min: -89.9, max: 89.9 };
export const initialCamera: Tracy.CameraProperties = {
	rotation: { x: 2.44, y: 0 },
	distance: 5.5,
	focusPoint: { x: 0, y: 1.25, z: 0 },
};

export const camera: Tracy.CameraProperties = structuredClone(initialCamera);

export function resetCamera() {
	Object.assign(camera, structuredClone(initialCamera));
}

export function clamp(v: number, min: number, max: number) {
	return Math.min(Math.max(v, min), max);
}

export function setupCameraControls(canvas: HTMLCanvasElement, onChange: () => void) {
	let isMouseDown = false;

	canvas.onmousedown = (event) => { if (event.button === 0) isMouseDown = true; };
	document.onmouseup = (event) => { if (event.button === 0) isMouseDown = false; };

	document.onmousemove = (event) => {
		if (isMouseDown) {
			camera.rotation.x = clamp(
				camera.rotation.x + event.movementY * 0.2,
				cameraRotationXBounds.min, cameraRotationXBounds.max
			);
			camera.rotation.y -= event.movementX * 0.2;
			onChange(); // Signal that the render needs to restart
		}
	};

	canvas.onwheel = (event) => {
		let delta = -event.deltaY / 120.0;
		delta = 1.0 - (delta * 0.08);
		camera.distance = clamp(camera.distance * delta, cameraDistanceBounds.min, cameraDistanceBounds.max);
		onChange(); // Signal that the render needs to restart
	};

	// track touch events
	let primaryTouch = { identifier: undefined as number | undefined, x: 0, y: 0 };
	let secondaryTouch = { identifier: undefined as number | undefined, x: 0, y: 0 };

	canvas.ontouchstart = function (event: TouchEvent) {
		if (primaryTouch.identifier === undefined) {
			primaryTouch.identifier = event.changedTouches[0].identifier;
			primaryTouch.x = event.changedTouches[0].clientX;
			primaryTouch.y = event.changedTouches[0].clientY;
		}
		else if (secondaryTouch.identifier === undefined) {
			secondaryTouch.identifier = event.changedTouches[0].identifier;
			secondaryTouch.x = event.changedTouches[0].clientX;
			secondaryTouch.y = event.changedTouches[0].clientY;
		}
	};

	function touchend(event: TouchEvent) {
		if (primaryTouch.identifier === event.changedTouches[0].identifier) {
			primaryTouch.identifier = undefined;
			if (secondaryTouch.identifier !== undefined) {
				primaryTouch.identifier = secondaryTouch.identifier;
				primaryTouch.x = secondaryTouch.x;
				primaryTouch.y = secondaryTouch.y;
				secondaryTouch.identifier = undefined;
			}
		}
		if (secondaryTouch.identifier === event.changedTouches[0].identifier) {
			secondaryTouch.identifier = undefined;
		}
	}

	// measure touch movements, rotate and zoom accordingly
	document.ontouchend = touchend;
	document.ontouchcancel = touchend;
	document.ontouchmove = function (event: TouchEvent) {
		let primaryMovement = { x: 0, y: 0 };
		let secondaryMovement = { x: 0, y: 0 };
		const touches = event.changedTouches;

		// note that sometimes multiple events are generated for one touch point
		for (let i = 0; i < touches.length; i++) {
			const element = touches[i];
			if (primaryTouch.identifier === element.identifier) {
				primaryMovement.x += (element.clientX - primaryTouch.x);
				primaryMovement.y += (element.clientY - primaryTouch.y);
				primaryTouch.x = element.clientX;
				primaryTouch.y = element.clientY;
			}
			else if (secondaryTouch.identifier === element.identifier) {
				secondaryMovement.x += (element.clientX - secondaryTouch.x);
				secondaryMovement.y += (element.clientY - secondaryTouch.y);
				secondaryTouch.x = element.clientX;
				secondaryTouch.y = element.clientY;
			}
		}

		if (primaryMovement.x === 0 && primaryMovement.y === 0 && secondaryMovement.x === 0 && secondaryMovement.y === 0) {
			return;
		}

		// one touch point - only rotate
		if (primaryTouch.identifier !== undefined && secondaryTouch.identifier === undefined) {
			camera.rotation.x = clamp(
				camera.rotation.x + primaryMovement.y * 0.2,
				cameraRotationXBounds.min, cameraRotationXBounds.max
			);
			camera.rotation.y -= primaryMovement.x * 0.2;
			onChange();
		}
		// two touch points - rotate and zoom
		else if (primaryTouch.identifier !== undefined && secondaryTouch.identifier !== undefined) {
			// rotate
			camera.rotation.x = clamp(
				camera.rotation.x + primaryMovement.y * 0.1 + secondaryMovement.y * 0.1,
				cameraRotationXBounds.min, cameraRotationXBounds.max
			);
			camera.rotation.y -= primaryMovement.x * 0.1;
			camera.rotation.y -= secondaryMovement.x * 0.1;

			// 2d vector helper functions
			function dot(a: { x: number, y: number }, b: { x: number, y: number }) { return a.x * b.x + a.y * b.y; };
			function subtract(a: { x: number, y: number }, b: { x: number, y: number }) { return { x: a.x - b.x, y: a.y - b.y } };
			function normalize(v: { x: number, y: number }) {
				const length = Math.sqrt(v.x * v.x + v.y * v.y);
				return { x: v.x / length, y: v.y / length };
			};

			// zoom
			const primPos = { x: primaryTouch.x, y: primaryTouch.y };
			const secPos = { x: secondaryTouch.x, y: secondaryTouch.y };
			const primToSec = dot(primaryMovement, normalize(subtract(secPos, primPos)));
			const secToPrim = dot(secondaryMovement, normalize(subtract(primPos, secPos)));
			let delta = - primToSec - secToPrim; // negative = zoom out, positive = zoom in
			delta *= -0.002;
			delta += 1.0;
			camera.distance = clamp(camera.distance * delta, cameraDistanceBounds.min, cameraDistanceBounds.max);
			onChange();
		}
	};
}
