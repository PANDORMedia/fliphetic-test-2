// bridge/server.js
// Serial-to-HTTP bridge for the ESP32 button/LED tester. Reads the ESP's
// JSON-line output from the serial device and serves the most recent line at
// GET /state; the debug screen polls it. Dependency-free: pure Node, plus
// busybox `stty` to set the line mode.
//
// If the device is absent (ESP not flashed or not plugged in) the bridge
// still runs and serves {} — the screen just shows "no device".

const http = require('http');
const fs = require('fs');
const { execFileSync } = require('child_process');

const DEVICE = process.env.ESP_DEVICE || '/dev/esp';
const BAUD = process.env.ESP_BAUD || '115200';
const PORT = 8091;

let state = '{}';      // latest valid JSON line from the ESP
let connected = false;

function openSerial() {
  // put the tty in raw mode at the firmware's baud
  try {
    execFileSync('stty', [
      '-F', DEVICE, BAUD, 'raw', '-echo', 'cs8', '-parenb', '-cstopb',
    ]);
  } catch (e) {
    connected = false;       // device not present yet — retry
    setTimeout(openSerial, 3000);
    return;
  }

  const stream = fs.createReadStream(DEVICE);
  let buf = '';
  connected = true;

  stream.on('data', (chunk) => {
    buf += chunk.toString('utf8');
    let nl;
    while ((nl = buf.indexOf('\n')) >= 0) {
      const line = buf.slice(0, nl).trim();
      buf = buf.slice(nl + 1);
      if (line.startsWith('{') && line.endsWith('}')) {
        try {
          JSON.parse(line);   // validate — skips ESP boot-log noise
          state = line;
        } catch (e) { /* not our JSON */ }
      }
    }
    if (buf.length > 8192) buf = '';   // guard against a stuck line
  });

  const reopen = () => {
    connected = false;
    stream.destroy();
    setTimeout(openSerial, 3000);
  };
  stream.once('error', reopen);
  stream.once('close', reopen);
}

const server = http.createServer((req, res) => {
  res.setHeader('Access-Control-Allow-Origin', '*');
  if (req.method === 'GET' && req.url === '/state') {
    res.writeHead(200, { 'Content-Type': 'application/json' });
    res.end(state);
    return;
  }
  res.writeHead(200, { 'Content-Type': 'text/plain' });
  res.end(connected ? 'esp bridge: connected' : 'esp bridge: waiting for device');
});

server.listen(PORT, () => console.log('esp bridge listening on :' + PORT));
openSerial();
