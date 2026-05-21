const fs = require('fs');
const path = 'src/data/screens.json';
const data = JSON.parse(fs.readFileSync(path, 'utf8'));

// 1. Remove propeller and scroll-pos from all screens
data.screens.forEach(screen => {
  screen.elements = screen.elements.filter(el => el.id !== 'propeller' && el.id !== 'scroll-pos');
});

// 2. Add P0 screen at the beginning
const p0Screen = {
  "id": "info-p0-global-status",
  "name": "P0 — Global Status",
  "description": "Live aggregate metrics for all active sensors and global flow animation.",
  "elements": [
    {
      "id": "hdr-title",
      "kind": "text",
      "x": 2,
      "y": 2,
      "content": "System Status",
      "emphasis": "strong"
    },
    {
      "id": "total-flow-label",
      "kind": "text",
      "x": 2,
      "y": 20,
      "content": "Total Flow (L/s):",
      "emphasis": "muted"
    },
    {
      "id": "total-flow-value",
      "kind": "text",
      "x": 2,
      "y": 32,
      "binding": "telemetry.total",
      "emphasis": "strong"
    },
    {
      "id": "flow-dots",
      "kind": "icon",
      "x": 40,
      "y": 70,
      "width": 55,
      "height": 55,
      "metadata": {
        "assetId": "flow-dots"
      }
    },
    {
      "id": "footer-hint",
      "kind": "text",
      "x": 2,
      "y": 226,
      "content": "↑↓ pages  ENTER 3s→idle",
      "emphasis": "muted"
    }
  ],
  "flows": [
    {
      "id": "f-next",
      "label": "Next page",
      "trigger": { "type": "button", "button": "down", "gesture": "short" },
      "targetScreenId": "info-p1-instant-flow",
      "actionId": "ui.action.page.next"
    },
    {
      "id": "f-prev",
      "label": "Prev page",
      "trigger": { "type": "button", "button": "up", "gesture": "short" },
      "targetScreenId": "info-p7-enter-config",
      "actionId": "ui.action.page.previous"
    },
    {
      "id": "f-idle",
      "label": "Enter idle",
      "trigger": { "type": "button", "button": "enter", "gesture": "long" },
      "targetScreenId": "state-idle",
      "actionId": "ui.action.mode.idle"
    }
  ]
};

data.screens.unshift(p0Screen);

// 3. Update P1's prev target to P0, and P7's next target to P0
const p1 = data.screens.find(s => s.id === 'info-p1-instant-flow');
const p1Prev = p1.flows.find(f => f.actionId === 'ui.action.page.previous');
if (p1Prev) p1Prev.targetScreenId = 'info-p0-global-status';

const p7 = data.screens.find(s => s.id === 'info-p7-enter-config');
const p7Next = p7.flows.find(f => f.actionId === 'ui.action.page.next');
if (p7Next) p7Next.targetScreenId = 'info-p0-global-status';

// write back
fs.writeFileSync(path, JSON.stringify(data, null, 2) + '\n');
console.log("screens.json updated.");
