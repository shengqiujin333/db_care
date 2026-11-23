import { chromium } from 'file:///C:/Users/kason/AppData/Local/npm-cache/_npx/9833c18b2d85bc59/node_modules/playwright/index.mjs';
import net from 'node:net';
import fs from 'node:fs';

const HOST = '8.140.23.253';
const out = {
  started_at: new Date().toISOString(),
  browser: 'C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe',
  base_url: `http://${HOST}:5000`,
  http_probes: [],
  tcp_probes: [],
  note: 'Only GET requests and TCP connect. No POST, no write, no login attempt, no database access.'
};

function tcpProbe(port, timeoutMs = 5000) {
  return new Promise((resolve) => {
    const rec = { port, started_at: new Date().toISOString() };
    const sock = new net.Socket();
    const done = (result) => {
      rec.result = result;
      rec.finished_at = new Date().toISOString();
      sock.destroy();
      resolve(rec);
    };
    sock.setTimeout(timeoutMs);
    sock.once('connect', () => done('open'));
    sock.once('timeout', () => done('timeout'));
    sock.once('error', (e) => { rec.error = e.code || String(e); done('error'); });
    sock.connect(port, HOST);
  });
}

const browser = await chromium.launch({
  executablePath: 'C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe',
  headless: true,
  timeout: 30000
});
const context = await browser.newContext();
const page = await context.newPage();

async function httpProbe(path) {
  const rec = { path, method: 'GET', started_at: new Date().toISOString() };
  try {
    const resp = await page.request.get(out.base_url + path, { timeout: 15000, maxRedirects: 0, failOnStatusCode: false });
    rec.status = resp.status();
    const h = resp.headers();
    rec.headers = Object.fromEntries(Object.entries(h).filter(([k]) =>
      ['allow', 'content-type', 'content-length', 'server', 'www-authenticate', 'access-control-allow-origin'].includes(k.toLowerCase())));
    rec.body_prefix = (await resp.text()).slice(0, 300);
  } catch (e) {
    rec.error = String(e && e.message ? e.message : e);
  }
  rec.finished_at = new Date().toISOString();
  out.http_probes.push(rec);
}

for (const p of ['/', '/upload_data', '/login', '/setmac', '/nonexistent-probe-e2e']) {
  await httpProbe(p);
}

out.tcp_probes.push(await tcpProbe(5000));
for (const port of [1883, 8883, 3306, 18083, 8083, 8084]) {
  out.tcp_probes.push(await tcpProbe(port, 4000));
}

out.finished_at = new Date().toISOString();
await browser.close();
fs.writeFileSync('evidence/e2e_p0/platform_baseline_result.json', JSON.stringify(out, null, 2));
console.log('DONE');
