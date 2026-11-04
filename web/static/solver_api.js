// Dual-Mode Backend Adapter: uses C++ HTTP server (/api/...) when available,
// and automatically falls back to client-side WebAssembly (solver_worker.js)
// when hosted statically (e.g. GitHub Pages).
let backendMode = null; // 'server' | 'wasm'
let backendDetectPromise = null;
let wasmWorker = null;
let wasmNextRequestId = 1;
let wasmActiveRequest = null;
let wasmLatestStatus = { status: 'Idle', progress: -1 };
let isWasmSingleThreaded = false;

function initWasmWorker() {
  if (wasmWorker) return wasmWorker;
  wasmLatestStatus = { status: 'Loading WebAssembly solver...', progress: -1 };
  wasmWorker = new Worker('solver_worker.js');
  wasmWorker.onmessage = (e) => {
    const msg = e.data;
    if (!msg || typeof msg !== 'object') return;
    if (msg.type === 'ready') {
      if (msg.singleThreaded || (!window.crossOriginIsolated && typeof SharedArrayBuffer === 'undefined')) {
        isWasmSingleThreaded = true;
        configureWasmThreadsUI(true);
      }
      wasmLatestStatus = {
        status: isWasmSingleThreaded ? 'Idle (Single-Threaded)' : 'Idle',
        progress: -1
      };
    } else if (msg.type === 'progress') {
      wasmLatestStatus = {
        status: msg.status || 'Solving...',
        progress: (typeof msg.progress === 'number') ? msg.progress : -1
      };
    } else if (msg.type === 'solve_result') {
      if (wasmActiveRequest && wasmActiveRequest.id === msg.id) {
        const req = wasmActiveRequest;
        wasmActiveRequest = null;
        try {
          const parsed = JSON.parse(msg.resultJson || '{}');
          if (parsed && parsed.success === false && typeof parsed.error === 'string' &&
              (parsed.error.startsWith('WebAssembly solver exception:') ||
               parsed.error.startsWith('WebAssembly solver aborted:') ||
               parsed.aborted)) {
            reportRuntimeErrorToSentry(new Error(parsed.error), {
              source: 'wasm_solver_exception'
            });
            if (parsed.aborted && wasmWorker) {
              try { wasmWorker.terminate(); } catch (_) {}
              wasmWorker = null;
            }
          }
          req.resolve({
            ok: parsed.success !== false,
            json: async () => parsed
          });
        } catch (err) {
          reportRuntimeErrorToSentry(err, { source: 'wasm_invalid_json' });
          req.resolve({
            ok: false,
            json: async () => ({
              success: false,
              error: 'Invalid JSON from WebAssembly solver'
            })
          });
        }
      }
    } else if (msg.type === 'error') {
      const errMsg = msg.error || 'WebAssembly worker error';
      reportRuntimeErrorToSentry(new Error(errMsg), { source: 'wasm_worker_abort' });
      setStatus('error', 'WebAssembly worker error: ' + errMsg);
      if (wasmWorker) {
        try { wasmWorker.terminate(); } catch (_) {}
        wasmWorker = null;
      }
      if (wasmActiveRequest) {
        const req = wasmActiveRequest;
        wasmActiveRequest = null;
        req.resolve({
          ok: false,
          json: async () => ({
            success: false,
            error: errMsg
          })
        });
      }
    }
  };
  wasmWorker.onerror = (err) => {
    const errMsg = 'Web Worker error: ' +
      (err && err.message ? err.message : 'failed to load solver_worker.js');
    reportRuntimeErrorToSentry(new Error(errMsg), { source: 'wasm_worker_onerror' });
    setStatus('error', errMsg);
    if (wasmWorker) {
      try { wasmWorker.terminate(); } catch (_) {}
      wasmWorker = null;
    }
    if (wasmActiveRequest) {
      const req = wasmActiveRequest;
      wasmActiveRequest = null;
      req.resolve({
        ok: false,
        json: async () => ({
          success: false,
          error: errMsg
        })
      });
    }
  };
  return wasmWorker;
}

const WASM_NUM_THREADS =
  (typeof navigator !== 'undefined' && navigator.hardwareConcurrency >= 8)
    ? 8
    : 4;

async function ensureCrossOriginIsolation() {
  if (window.crossOriginIsolated || typeof SharedArrayBuffer !== 'undefined') {
    try {
      sessionStorage.removeItem('bridge_coi_reloaded');
    } catch (_) {}
    return true;
  }
  if (!('serviceWorker' in navigator) || !window.isSecureContext) {
    return false;
  }
  try {
    const reg = await navigator.serviceWorker.register('coi_serviceworker.js');
    if (reg && (reg.installing || reg.waiting || reg.active)) {
      if (!sessionStorage.getItem('bridge_coi_reloaded')) {
        sessionStorage.setItem('bridge_coi_reloaded', '1');
        if (navigator.serviceWorker.controller) {
          window.location.reload();
        } else {
          navigator.serviceWorker.addEventListener('controllerchange', () => {
            window.location.reload();
          }, { once: true });
        }
        await new Promise((r) => setTimeout(r, 300));
      }
    }
  } catch (_) {}
  return Boolean(window.crossOriginIsolated || typeof SharedArrayBuffer !== 'undefined');
}

function configureWasmThreadsUI(isSingleThreaded = false) {
  const numThreadsSelect = document.getElementById('numThreads');
  if (!numThreadsSelect) return;
  const singleThread = isSingleThreaded || (!window.crossOriginIsolated && typeof SharedArrayBuffer === 'undefined');
  const threads = singleThread ? 1 : WASM_NUM_THREADS;
  numThreadsSelect.value = String(threads);
  numThreadsSelect.disabled = true;
  numThreadsSelect.title = singleThread
    ? 'Running client-side in WebAssembly (1 worker thread - single-threaded fallback active due to missing Cross-Origin Isolation)'
    : 'Running client-side in WebAssembly (' + WASM_NUM_THREADS + ' worker threads)';
}

async function ensureBackendMode() {
  if (backendMode) return backendMode;
  if (backendDetectPromise) return backendDetectPromise;
  backendDetectPromise = (async () => {
    if (window.BRIDGE_FORCE_WASM === true) {
      backendMode = 'wasm';
      setSentryGlobalAttribute('backend_mode', backendMode);
      const hasCOI = await ensureCrossOriginIsolation();
      if (!hasCOI) {
        isWasmSingleThreaded = true;
      }
      configureWasmThreadsUI(isWasmSingleThreaded);
      initWasmWorker();
      return backendMode;
    }
    try {
      const res = await fetch('/api/health', { cache: 'no-store' });
      if (res.ok) {
        const data = await res.json();
        if (data && data.status === 'ok') {
          backendMode = 'server';
          setSentryGlobalAttribute('backend_mode', backendMode);
          return backendMode;
        }
      }
    } catch (_) {}
    backendMode = 'wasm';
    setSentryGlobalAttribute('backend_mode', backendMode);
    const hasCOI = await ensureCrossOriginIsolation();
    if (!hasCOI) {
      isWasmSingleThreaded = true;
    }
    configureWasmThreadsUI(isWasmSingleThreaded);
    initWasmWorker();
    return backendMode;
  })();
  return backendDetectPromise;
}

async function apiSolve(payload, signal) {
  const mode = await ensureBackendMode();
  if (mode === 'server') {
    return fetch('/api/solve', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload),
      signal
    });
  }
  const worker = initWasmWorker();
  const reqId = wasmNextRequestId++;
  const singleThread = isWasmSingleThreaded || (!window.crossOriginIsolated && typeof SharedArrayBuffer === 'undefined');
  const threadsToUse = singleThread ? 1 : WASM_NUM_THREADS;
  wasmLatestStatus = {
    status: singleThread ? 'Starting solve (single-threaded)...' : 'Starting solve...',
    progress: -1
  };
  return new Promise((resolve) => {
    wasmActiveRequest = { id: reqId, resolve };
    if (signal) {
      signal.addEventListener('abort', () => {
        if (wasmActiveRequest && wasmActiveRequest.id === reqId) {
          wasmActiveRequest = null;
          resolve({
            ok: false,
            json: async () => ({
              success: false,
              stopped: true,
              error: 'Solve stopped by user.'
            })
          });
        }
      }, { once: true });
    }
    worker.postMessage({
      type: 'solve',
      id: reqId,
      payloadJson: JSON.stringify(
        Object.assign({}, payload, { num_threads: threadsToUse })
      )
    });
  });
}

async function apiSolveStatus() {
  const mode = await ensureBackendMode();
  if (mode === 'server') {
    return fetch('/api/solve_status');
  }
  const st = { status: wasmLatestStatus.status || 'Solving...' };
  if (typeof wasmLatestStatus.progress === 'number' && wasmLatestStatus.progress >= 0) {
    st.progress = wasmLatestStatus.progress;
  }
  return {
    ok: true,
    json: async () => st
  };
}

function apiStopSolve() {
  if (backendMode === 'wasm') {
    if (wasmWorker) {
      wasmWorker.terminate();
      wasmWorker = null;
    }
    if (wasmActiveRequest) {
      const req = wasmActiveRequest;
      wasmActiveRequest = null;
      req.resolve({
        ok: false,
        json: async () => ({
          success: false,
          stopped: true,
          error: 'Solve stopped by user.'
        })
      });
    }
    wasmLatestStatus = { status: 'Solve stopped.', progress: -1 };
    initWasmWorker();
    return;
  }
  try {
    fetch('/api/stop_solve', { method: 'POST' }).catch(() => {});
  } catch (_) {}
}
