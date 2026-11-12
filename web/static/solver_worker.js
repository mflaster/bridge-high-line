// Web Worker running the Emscripten-compiled C++ Bridge Solver (bridge_wasm.js)

let wasmReady = false;
let wasmLoadError = null;
let isSingleThreaded = false;
const pendingMessages = [];

self.onBridgeSolverProgress = function(status, progress) {
  self.postMessage({
    type: 'progress',
    status: status,
    progress: (typeof progress === 'number' && progress >= 0) ? progress : null
  });
};

self.Module = {
  mainScriptUrlOrBlob: new URL('bridge_wasm.js', self.location.href).href,
  locateFile: function(path) {
    if (path.endsWith('.wasm')) {
      return new URL('bridge_wasm.wasm', self.location.href).href;
    }
    return path;
  },
  onRuntimeInitialized: function() {
    wasmReady = true;
    self.postMessage({
      type: 'ready',
      singleThreaded: isSingleThreaded,
      num_threads: isSingleThreaded ? 1 : null
    });
    while (pendingMessages.length > 0) {
      handleWorkerMessage(pendingMessages.shift());
    }
  },
  onAbort: function(reason) {
    wasmReady = false;
    wasmLoadError = 'WebAssembly solver aborted: ' + String(reason || 'unknown');
    self.postMessage({ type: 'error', error: wasmLoadError });
  }
};

function handleWorkerMessage(msg) {
  if (!msg || typeof msg !== 'object') return;
  if (msg.type === 'solve') {
    try {
      let payloadJsonStr = msg.payloadJson || '{}';
      if (isSingleThreaded) {
        try {
          const parsed = JSON.parse(payloadJsonStr);
          parsed.num_threads = 1;
          payloadJsonStr = JSON.stringify(parsed);
        } catch (_) {}
      }
      const resultJson = self.Module.ccall(
        'wasm_solve_json',
        'string',
        ['string'],
        [payloadJsonStr]
      );
      self.postMessage({
        type: 'solve_result',
        id: msg.id,
        resultJson: resultJson
      });
    } catch (err) {
      const errDetail = (err && err.message) ? err.message : String(err);
      const fullErr = wasmLoadError
        ? (wasmLoadError + ' (' + errDetail + ')')
        : ('WebAssembly solver exception: ' + errDetail);
      self.postMessage({
        type: 'solve_result',
        id: msg.id,
        resultJson: JSON.stringify({
          success: false,
          aborted: Boolean(wasmLoadError) ||
            Boolean(err && err.name === 'RuntimeError'),
          error: fullErr
        })
      });
    }
  } else if (msg.type === 'stop') {
    try {
      self.Module.ccall('wasm_stop_solve', 'string', [], []);
    } catch (_) {}
  }
}

self.onmessage = function(e) {
  const msg = e.data;
  if (wasmLoadError) {
    if (msg && msg.type === 'solve') {
      self.postMessage({
        type: 'solve_result',
        id: msg.id,
        resultJson: JSON.stringify({ success: false, error: wasmLoadError })
      });
    }
    return;
  }
  if (!wasmReady) {
    pendingMessages.push(msg);
    return;
  }
  handleWorkerMessage(msg);
};

// Catch missing SharedArrayBuffer states and prepare for single-threaded execution
if (typeof SharedArrayBuffer === 'undefined') {
  isSingleThreaded = true;
  if (typeof self.SharedArrayBuffer === 'undefined' && typeof self.ArrayBuffer !== 'undefined') {
    self.SharedArrayBuffer = self.ArrayBuffer;
  }
  if (typeof WebAssembly !== 'undefined' && WebAssembly.Memory) {
    const OrigMemory = WebAssembly.Memory;
    WebAssembly.Memory = function(descriptor) {
      if (descriptor && descriptor.shared) {
        try {
          return new OrigMemory(descriptor);
        } catch (_) {
          const unsharedDesc = Object.assign({}, descriptor);
          delete unsharedDesc.shared;
          return new OrigMemory(unsharedDesc);
        }
      }
      return new OrigMemory(descriptor);
    };
    WebAssembly.Memory.prototype = OrigMemory.prototype;
  }
}

try {
  importScripts('bridge_wasm.js');
} catch (err) {
  wasmLoadError =
    'Failed to load bridge_wasm.js: ' + (err && err.message ? err.message : String(err));
  self.postMessage({ type: 'error', error: wasmLoadError });
}
