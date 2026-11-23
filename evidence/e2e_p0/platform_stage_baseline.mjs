// E2E-JOINT-P0 platform-stage read-only baseline via the registered browser/API entry.
// GET/HEAD only. No POST, no login, no write, no database access, no restart.
import { chromium } from 'file:///C:/Users/kason/AppData/Local/npm-cache/_npx/9833c18b2d85bc59/node_modules/playwright/index.mjs';
import net from 'node:net';
import fs from 'node:fs';

const HOST = '8.140.23.253';
const EDGE = 'C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe';
const out = {
  stage: 'platform',
  case: 'E2E-JOINT-P0',
  started_at: new Date().toISOString(),
  entry: 'existing_service_browser_test (Playwright + Edge, host)',
  http_probes: [],
  tcp_probes: [],
  control_ports: [],
  browser_navigation: null,
  note: 'Only GET/HEAD requests, browser navigation and TCP connect. No POST, no write, no login, no database access.'
};
out.host = HOST;

function tcpProbe(port, timeoutMs = 6000) {
  return new Promise((resolve) => {
    const t0 = process.hrtime.bigint();
    const rec = { port, started_at: new Date().toISOString() };
    const sock = new net.Socket();
    let settled = false;
    const done = (result) => {
      if (settled) return;
      settled = true;
      rec.result = result;
      if (rec.connect_ms === undefined) {
        rec.connect_ms = Number((Number(process.hrtime.bigint() - t0) / 1e6).toFixed(2));
      }
      rec.finished_at = new Date().toISOString();
      sock.destroy();
      resolve(rec);
    };
    sock.setTimeout(timeoutMs);
    sock.once('connect', () => {
      rec.connect_ms = Number((Number(process.hrtime.bigint() - t0) / 1e6).toFixed(2));
      // Give the peer 1.2 s to send any first byte, so a fake accept is visible.
      let first = null;
      sock.once('data', (b) => { first = b.length; });
      setTimeout(() => { rec.first_bytes = first; done('open'); }, 1200);
    });
    sock.once('timeout', () => done('timeout'));
    sock.once('error', (e) => { rec.error = e.code || String(e); done('error'); });
    sock.connect(port, HOST);
  });
}

const browser = await chromium.launch({ executablePath: EDGE, headless: true, timeout: 30000 });
const context = await browser.newContext();
const page = await context.newPage();

async function httpProbe(url) {
  const rec = { url, method: 'GET', started_at: new Date().toISOString() };
  const t0 = Date.now();
  try {
    const resp = await page.request.get(url, { timeout: 12000, maxRedirects: 0, failOnStatusCode: false });
    rec.status = resp.status();
    const h = resp.headers();
    rec.headers = Object.fromEntries(Object.entries(h).filter(([k]) =>
      ['allow', 'content-type', 'content-length', 'server', 'www-authenticate', 'location'].includes(k.toLowerCase())));
    rec.body_prefix = (await resp.text()).slice(0, 300);
  } catch (e) {
    rec.error = String(e && e.message ? e.message : e).split('\n')[0];
  }
  rec.elapsed_ms = Date.now() - t0;
  rec.finished_at = new Date().toISOString();
  out.http_probes.push(rec);
}

for (const url of [
  `http://${HOST}:18083/`,
  `http://${HOST}:18083/status`,
  `http://${HOST}:5000/`,
  `http://${HOST}:5000/upload_data`,
  `http://${HOST}:5000/login`
]) {
  await httpProbe(url);
}

try {
  const resp = await page.goto(`http://${HOST}:18083/`, { timeout: 15000, waitUntil: 'domcontentloaded' });
  out.browser_navigation = {
    url: `http://${HOST}:18083/`,
    status: resp ? resp.status() : null,
    title: await page.title().catch(() => null),
    finished_at: new Date().toISOString()
  };
  await page.screenshot({ path: 'evidence/e2e_p0/platform_stage_18083_navigation.png' }).catch(() => {});
} catch (e) {
  out.browser_navigation = {
    url: `http://${HOST}:18083/`,
    error: String(e && e.message ? e.message : e).split('\n')[0],
    finished_at: new Date().toISOString()
  };
}

await browser.close();

for (const port of [5000, 1883, 8883, 3306, 18083, 8083, 8084]) {
  out.tcp_probes.push(await tcpProbe(port));
}
// Control ports: none of these are project services. If they behave like the service
// ports, the host TCP path is intercepted and the "open" observations are invalid.
for (const port of [65000, 54321, 1]) {
  out.control_ports.push(await tcpProbe(port));
}

out.finished_at = new Date().toISOString();
fs.writeFileSync('evidence/e2e_p0/platform_stage_baseline_result.json', JSON.stringify(out, null, 2));
console.log('DONE');
