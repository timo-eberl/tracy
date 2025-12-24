import { spawnSync } from 'child_process';
import fs from 'fs';
import path from 'path';

const isDebug = process.argv.includes('--debug');

// Auto-discover PCG source files
const pcgSrcDir = '../../dependencies/pcg-c/src';
const pcgSources = fs.readdirSync(pcgSrcDir)
    .filter(file => file.endsWith('.c'))
    .map(file => path.join(pcgSrcDir, file));

// Emscripten Flags (Shared across debug/release)
const emflags = [
	'-I../../include',
	'-I../../dependencies/pcg-c/include',
	'-sMODULARIZE=1',
	'-sEXPORT_ES6=1',
	'-sSHARED_MEMORY=1', '-sIMPORTED_MEMORY=1',	// required so shared memory can be used
	'--emit-tsd', 'tracy_c.d.ts',
	'-Wall', '-Wextra',							// enable all warnings
];
// Release Flags
const emRelease = [
	'-O3',
];
// Debug Flags
const emDebug = [
	'-g',
	'-gsource-map',
	'-O0',			// No optimizations
//	'-O1',			// Simple optimizations that make the code way faster (~6x) while preserving some debug information
					// debugging will be limited (e.g. some local variables are removed)
];

console.log(`Building WASM (${isDebug ? 'debug' : 'release'})...`);

const args = [
	'../../src/tracy.c',
	...pcgSources,
	...emflags,
	...(isDebug ? emDebug : emRelease),
	'-o', 'src/tracy_c.js'
];
const result = spawnSync('emcc', args, { stdio: 'inherit', shell: true });
process.exit(result.status);
