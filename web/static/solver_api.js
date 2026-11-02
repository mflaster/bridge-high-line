// Dual-Mode Backend Adapter: uses C++ HTTP server (/api/...) when available,
// and automatically falls back to client-side WebAssembly (solver_worker.js)
// when hosted statically (e.g. GitHub Pages).
let backendMode = null; // 'server' | 'wasm'
let backendDetectPromise = null;
let wasmWorker = null;
let wasmNextRequestId = 1;
let wasmActiveRequest = null;
let wasmLatestStatus = { status: 'Idle', progress: -1 };

function initWasmWorker() {
  if (wasmWorker) return wasmWorker;
  wasmLatestStatus = { status: 'Loading WebAssembly solver...', progress: -1 };
  wasmWorker = new Worker('solver_worker.js');
  wasmWorker.onmessage = (e) => {
    const msg = e.data;
    if (!msg || typeof msg !== 'object') return;
    if (msg.type === 'ready') {
      wasmLatestStatus = { status: 'Idle', progress: -1 };
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

async function ensureCrossOriginIsolation(allowReload = true) {
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
    if (allowReload && reg && (reg.installing || reg.waiting || reg.active)) {
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
  return typeof SharedArrayBuffer !== 'undefined';
}

function configureWasmThreadsUI() {
  const numThreadsSelect = document.getElementById('numThreads');
  if (!numThreadsSelect) return;
  numThreadsSelect.value = String(WASM_NUM_THREADS);
  numThreadsSelect.disabled = true;
  numThreadsSelect.title =
    'Running client-side in WebAssembly (' + WASM_NUM_THREADS + ' worker threads)';
}

async function ensureBackendMode() {
  if (backendMode) return backendMode;
  if (backendDetectPromise) return backendDetectPromise;
  backendDetectPromise = (async () => {
    if (window.BRIDGE_FORCE_WASM === true) {
      backendMode = 'wasm';
      if (typeof window !== 'undefined') window.backendMode = backendMode;
      setSentryGlobalAttribute('backend_mode', backendMode);
      configureWasmThreadsUI();
      await ensureCrossOriginIsolation();
      initWasmWorker();
      return backendMode;
    }
    try {
      const res = await fetch('/api/health', { cache: 'no-store' });
      if (res.ok) {
        const data = await res.json();
        if (data && data.status === 'ok') {
          backendMode = 'server';
          if (typeof window !== 'undefined') window.backendMode = backendMode;
          setSentryGlobalAttribute('backend_mode', backendMode);
          return backendMode;
        }
      }
    } catch (_) {}
    backendMode = 'wasm';
    if (typeof window !== 'undefined') window.backendMode = backendMode;
    setSentryGlobalAttribute('backend_mode', backendMode);
    configureWasmThreadsUI();
    await ensureCrossOriginIsolation();
    initWasmWorker();
    return backendMode;
  })();
  return backendDetectPromise;
}

function dispatchWasmSolve(payload, signal) {
  const worker = initWasmWorker();
  const reqId = wasmNextRequestId++;
  wasmLatestStatus = { status: 'Solving in WebAssembly...', progress: -1 };
  return new Promise((resolve) => {
    if (signal && signal.aborted) {
      resolve({
        ok: false,
        json: async () => ({
          success: false,
          stopped: true,
          error: 'Solve stopped by user.'
        })
      });
      return;
    }
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
        Object.assign({}, payload, { num_threads: WASM_NUM_THREADS })
      )
    });
  });
}

async function apiSolve(payload, signal) {
  const mode = await ensureBackendMode();
  if (mode === 'server') {
    try {
      return await fetch('/api/solve', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(payload),
        signal
      });
    } catch (err) {
      // If the solve was aborted by the user, re-throw to allow normal cancellation handling.
      if ((signal && signal.aborted) || (err && err.name === 'AbortError')) {
        throw err;
      }

      // Server fetch failed due to network / connection drop; fall back to WebAssembly.
      backendMode = 'wasm';
      if (typeof window !== 'undefined') window.backendMode = backendMode;
      backendDetectPromise = Promise.resolve('wasm');

      // Update Sentry attributes and report fallback transition event.
      setSentryGlobalAttribute('backend_mode', backendMode);
      if (typeof emitSentryMetricCount === 'function') {
        emitSentryMetricCount('backend_fallback_to_wasm', 1, {
          source: 'fetch_rejection',
          error: (err && err.message) || String(err)
        });
      }
      if (typeof window !== 'undefined' && window.Sentry) {
        if (typeof window.Sentry.addBreadcrumb === 'function') {
          window.Sentry.addBreadcrumb({
            category: 'backend',
            message: 'Server fetch rejected; falling back to WebAssembly: ' +
              ((err && err.message) || String(err)),
            level: 'warning'
          });
        }
        if (typeof window.Sentry.captureMessage === 'function') {
          window.Sentry.captureMessage(
            'Server fetch rejected; falling back to WebAssembly: ' +
              ((err && err.message) || String(err)),
            'warning'
          );
        }
      }
      if (typeof reportRuntimeErrorToSentry === 'function') {
        reportRuntimeErrorToSentry(err, {
          source: 'server_fetch_fallback_to_wasm',
          previous_mode: 'server',
          target_mode: 'wasm'
        });
      }

      // Lock thread controls to WebAssembly specifications.
      configureWasmThreadsUI();

      // Update status message informing user of local WebAssembly execution.
      const fallbackStatusMsg = 'Server unreachable. Running client-side WebAssembly solver...';
      if (typeof setStatus === 'function') {
        setStatus('running', fallbackStatusMsg);
      } else if (typeof window !== 'undefined' && typeof window.setStatus === 'function') {
        window.setStatus('running', fallbackStatusMsg);
      }

      // Ensure cross-origin isolation without reloading the browser, then initialize worker.
      await ensureCrossOriginIsolation(false);
      initWasmWorker();

      // Dispatch solve payload to local WebAssembly worker and return worker promise.
      return dispatchWasmSolve(payload, signal);
    }
  }
  return dispatchWasmSolve(payload, signal);
}

async function apiSolveStatus() {
  const mode = await ensureBackendMode();
  if (mode === 'server') {
    try {
      return await fetch('/api/solve_status');
    } catch (_) {
      if (backendMode === 'wasm') {
        const st = { status: wasmLatestStatus.status || 'Solving in WebAssembly...' };
        if (typeof wasmLatestStatus.progress === 'number' && wasmLatestStatus.progress >= 0) {
          st.progress = wasmLatestStatus.progress;
        }
        return {
          ok: true,
          json: async () => st
        };
      }
      return {
        ok: false,
        json: async () => ({ status: 'Connecting...' })
      };
    }
  }
  const st = { status: wasmLatestStatus.status || 'Solving in WebAssembly...' };
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

