// Google Apps Script Web App: Sentry Feedback Tunnel for Bridge High Line.
//
// Ad-blockers and tracker-blockers (uBlock Origin / EasyPrivacy, Brave Shields,
// Privacy Badger, DNS filters) routinely block browser requests directly to
// `*.sentry.io`. When direct `fetch(SENTRY_ENVELOPE_URL)` fails in the browser,
// `web/static/telemetry_and_help.js` falls back to POSTing the feedback
// envelope to this Web App (`SENTRY_FEEDBACK_TUNNEL_URL`).
//
// Deployment (personal Google account at https://script.google.com):
//   1. Create a new Apps Script project and paste this file into `Code.gs`.
//   2. Click Deploy -> New deployment -> Select type: Web app.
//   3. Set "Execute as: Me" and "Who has access: Anyone".
//   4. Copy the `/exec` URL into `SENTRY_FEEDBACK_TUNNEL_URL` in
//      `web/static/telemetry_and_help.js`.

const ALLOWED_DSN =
  'https://4f773fa337d8e6b1e559790da9ee1fb1@' +
  'o4512261310709760.ingest.us.sentry.io/4512261325324288';
const SENTRY_ENVELOPE_URL =
  'https://o4512261310709760.ingest.us.sentry.io/api/4512261325324288/' +
  'envelope/?sentry_key=4f773fa337d8e6b1e559790da9ee1fb1' +
  '&sentry_version=7&sentry_client=bridge-high-line-tunnel%2F1.0.0';
const MAX_ENVELOPE_BYTES = 262144;

function jsonResponse(obj) {
  return ContentService
    .createTextOutput(JSON.stringify(obj))
    .setMimeType(ContentService.MimeType.JSON);
}

function doPost(e) {
  try {
    const raw = (e && e.postData && typeof e.postData.contents === 'string')
      ? e.postData.contents
      : '';
    if (!raw || raw.length > MAX_ENVELOPE_BYTES) {
      return jsonResponse({ ok: false, error: 'Invalid or oversized payload' });
    }
    const nl1 = raw.indexOf('\n');
    const nl2 = nl1 > 0 ? raw.indexOf('\n', nl1 + 1) : -1;
    if (nl1 <= 0 || nl2 <= nl1 + 1) {
      return jsonResponse({ ok: false, error: 'Malformed envelope' });
    }
    const header = JSON.parse(raw.slice(0, nl1));
    if (!header || header.dsn !== ALLOWED_DSN) {
      return jsonResponse({ ok: false, error: 'Unauthorized DSN' });
    }
    const item = JSON.parse(raw.slice(nl1 + 1, nl2));
    if (!item || (item.type !== 'feedback' && item.type !== 'event')) {
      return jsonResponse({ ok: false, error: 'Unsupported item type' });
    }

    const resp = UrlFetchApp.fetch(SENTRY_ENVELOPE_URL, {
      method: 'post',
      contentType: 'text/plain;charset=utf-8',
      payload: raw,
      muteHttpExceptions: true
    });
    const status = resp.getResponseCode();
    const text = resp.getContentText();
    if (status < 200 || status >= 300) {
      return jsonResponse({
        ok: false,
        status: status,
        error: 'Sentry HTTP ' + status + (text ? ': ' + text.slice(0, 120) : '')
      });
    }
    return jsonResponse({ ok: true, status: status, body: text });
  } catch (err) {
    return jsonResponse({
      ok: false,
      error: String(err && err.message ? err.message : err)
    });
  }
}
