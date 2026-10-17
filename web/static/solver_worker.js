// Web Worker running the Emscripten-compiled C++ Bridge Solver (bridge_wasm.js)

let wasmReady = false;
let wasmLoadError = null;
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
  onRuntimeInitialized: function() {
    wasmReady = true;
    self.postMessage({ type: 'ready' });
    while (pendingMessages.length > 0) {
      handleWorkerMessage(pendingMessages.shift());
    }
  },
  onAbort: function(reason) {
    wasmLoadError = 'WebAssembly solver aborted: ' + String(reason || 'unknown');
    self.postMessage({ type: 'error', error: wasmLoadError });
  }
};

function handleWorkerMessage(msg) {
  if (!msg || typeof msg !== 'object') return;
  if (msg.type === 'solve') {
    try {
      const resultJson = self.Module.ccall(
        'wasm_solve_json',
        'string',
        ['string'],
        [msg.payloadJson || '{}']
      );
      self.postMessage({
        type: 'solve_result',
        id: msg.id,
        resultJson: resultJson
      });
    } catch (err) {
      self.postMessage({
        type: 'solve_result',
        id: msg.id,
        resultJson: JSON.stringify({
          success: false,
          error: 'WebAssembly solver exception: ' + (err && err.message ? err.message : String(err))
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

if (typeof SharedArrayBuffer === 'undefined') {
  wasmLoadError =
    'WebAssembly multithreading requires SharedArrayBuffer (Cross-Origin Isolation). ' +
    'Please reload the page after the service worker activates.';
  self.postMessage({ type: 'error', error: wasmLoadError });
} else {
  try {
    importScripts('bridge_wasm.js');
  } catch (err) {
    wasmLoadError =
      'Failed to load bridge_wasm.js: ' + (err && err.message ? err.message : String(err));
    self.postMessage({ type: 'error', error: wasmLoadError });
  }
}
