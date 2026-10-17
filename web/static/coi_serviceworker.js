// Service Worker enabling Cross-Origin Isolation (COOP/COEP) on static hosts
// such as GitHub Pages so SharedArrayBuffer and WebAssembly pthreads work.

self.addEventListener('install', () => {
  self.skipWaiting();
});

self.addEventListener('activate', (event) => {
  event.waitUntil(self.clients.claim());
});

self.addEventListener('fetch', (event) => {
  const req = event.request;
  if (req.method !== 'GET') {
    return;
  }
  if (req.cache === 'only-if-cached' && req.mode !== 'same-origin') {
    return;
  }

  event.respondWith(
    fetch(req).then((response) => {
      if (!response || response.status === 0) {
        return response;
      }
      const headers = new Headers(response.headers);
      headers.set('Cross-Origin-Opener-Policy', 'same-origin');
      headers.set('Cross-Origin-Embedder-Policy', 'require-corp');
      headers.set('Cross-Origin-Resource-Policy', 'cross-origin');
      return new Response(response.body, {
        status: response.status,
        statusText: response.statusText,
        headers: headers
      });
    })
  );
});
