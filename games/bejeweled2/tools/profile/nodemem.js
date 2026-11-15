// A hook for the web build under Node (the performance pass, GAME_NOTES.md "Performance"): at exit it prints one NODEMEM line
// with the size of Node's array buffers (the wasm memory is nearly all of them: it never shrinks, so its size at exit is its peak), Node's peak resident set, and the processor
// and wall time of the whole process. Run as
//   node -r games/bejeweled2/tools/profile/nodemem.js games/bejeweled/tools/verify/wasmrun.js game build/web/bejeweled2.js <switches>
// in the run's folder. A bare `node -e 0` is about 40 MB resident, which the figures include.
const mems = [];
const start = process.hrtime.bigint();
function note(m) { if (m && m.buffer && !mems.includes(m)) mems.push(m); }
const instantiate = WebAssembly.instantiate;
WebAssembly.instantiate = function (...a) {
	return instantiate.apply(this, a).then((r) => { note((r.instance || r).exports.memory); return r; });
};
const Instance = WebAssembly.Instance;
WebAssembly.Instance = class extends Instance {
	constructor(...a) { super(...a); note(this.exports.memory); }
};
const Memory = WebAssembly.Memory;
WebAssembly.Memory = class extends Memory {
	constructor(...a) { super(...a); note(this); }
};
process.on('exit', () => {
	const cpu = process.cpuUsage();
	console.error('NODEMEM ' + JSON.stringify({
		wasm_mb: Math.max(process.memoryUsage().arrayBuffers, ...mems.map((m) => m.buffer.byteLength)) / 1048576,   // the wasm memory is the big array buffer
		peak_rss_mb: process.resourceUsage().maxRSS / 1024,
		cpu_s: (cpu.user + cpu.system) / 1e6,
		wall_s: Number(process.hrtime.bigint() - start) / 1e9,
	}));
});
