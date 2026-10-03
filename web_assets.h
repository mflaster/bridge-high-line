#pragma once

#include <string>

namespace web_assets {

inline const char* INDEX_HTML = R"rawhtml(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Bridge Deal Solver & Decision Tree Explorer</title>
  <link rel="stylesheet" href="/styles.css">
</head>
<body>
  <header class="app-header">
    <div class="header-content">
      <div class="logo">
        <span class="spade-icon">♠</span>
        <h1>Bridge Deal Solver</h1>
      </div>
      <div class="preset-selector">
        <label for="presetSelect">Presets:</label>
        <select id="presetSelect" class="select-input">
          <option value="finesse">Simple Finesse (Spades)</option>
          <option value="endplay">4-Card Endplay Deal</option>
          <option value="squeeze">Squeeze Deal (NT)</option>
          <option value="fuzzy">Fuzzy Requirements Deal</option>
        </select>
        <button id="loadPresetBtn" class="btn btn-secondary">Load Preset</button>
      </div>
    </div>
  </header>

  <main class="app-main">
    <div class="grid-container">
      <!-- Left Column: Deal Setup & SVG Deal Diagram -->
      <section class="card-panel deal-setup-panel">
        <div class="panel-header">
          <h2>Deal Configuration</h2>
          <span class="panel-badge">Interactive SVG</span>
        </div>

        <!-- SVG Deal Diagram -->
        <div class="svg-deal-container">
          <svg id="dealSvg" viewBox="0 0 600 450" xmlns="http://www.w3.org/2000/svg">
            <rect width="100%" height="100%" fill="#1a382b" rx="12"/>
            
            <!-- Compass Center Box -->
            <g transform="translate(230, 160)">
              <rect width="140" height="110" rx="8" fill="#0f241a" stroke="#2d5e46" stroke-width="2"/>
              <text x="70" y="25" class="compass-label" text-anchor="middle">COMPASS</text>
              <text x="70" y="55" id="svgTrumpText" class="compass-trump" text-anchor="middle">Trump: ♠</text>
              <text x="70" y="80" id="svgLeaderText" class="compass-leader" text-anchor="middle">Leader: North</text>
            </g>

            <!-- North Hand -->
            <g class="hand-group" id="svgNorthHand" transform="translate(180, 15)">
              <rect width="240" height="130" rx="8" class="hand-box" id="boxNorth"/>
              <text x="120" y="22" class="hand-title" text-anchor="middle">NORTH (0)</text>
              <g transform="translate(15, 30)" id="cardsNorthGroup"></g>
            </g>

            <!-- West Hand -->
            <g class="hand-group" id="svgWestHand" transform="translate(15, 150)">
              <rect width="200" height="130" rx="8" class="hand-box" id="boxWest"/>
              <text x="100" y="22" class="hand-title" text-anchor="middle">WEST (3)</text>
              <g transform="translate(15, 30)" id="cardsWestGroup"></g>
            </g>

            <!-- East Hand -->
            <g class="hand-group" id="svgEastHand" transform="translate(385, 150)">
              <rect width="200" height="130" rx="8" class="hand-box" id="boxEast"/>
              <text x="100" y="22" class="hand-title" text-anchor="middle">EAST (1)</text>
              <g transform="translate(15, 30)" id="cardsEastGroup"></g>
            </g>

            <!-- South Hand -->
            <g class="hand-group" id="svgSouthHand" transform="translate(180, 285)">
              <rect width="240" height="130" rx="8" class="hand-box" id="boxSouth"/>
              <text x="120" y="22" class="hand-title" text-anchor="middle">SOUTH (2)</text>
              <g transform="translate(15, 30)" id="cardsSouthGroup"></g>
            </g>
          </svg>
        </div>

        <!-- Contract Controls -->
        <div class="controls-grid">
          <div class="control-group">
            <label for="trumpSelect">Trump Suit:</label>
            <select id="trumpSelect" class="select-input">
              <option value="SPADES">♠ Spades</option>
              <option value="HEARTS">♥ Hearts</option>
              <option value="DIAMONDS">♦ Diamonds</option>
              <option value="CLUBS">♣ Clubs</option>
              <option value="NOTRUMP">NT No Trump</option>
            </select>
          </div>

          <div class="control-group">
            <label for="leaderSelect">Leader:</label>
            <select id="leaderSelect" class="select-input">
              <option value="NORTH">North (N)</option>
              <option value="EAST">East (E)</option>
              <option value="SOUTH">South (S)</option>
              <option value="WEST">West (W)</option>
            </select>
          </div>

          <div class="control-group">
            <label for="targetTricks">Trick Target: <span id="targetTricksVal">2</span></label>
            <input type="range" id="targetTricks" min="1" max="13" value="2" class="range-input">
          </div>

          <div class="control-group">
            <label for="numIters">Simulations (Iters):</label>
            <input type="number" id="numIters" min="1" max="500" value="10" class="number-input">
          </div>
        </div>

        <!-- Text Inputs for Hands -->
        <div class="hand-inputs-accordion">
          <h3>Hand Text Inputs</h3>
          <div class="hand-input-row">
            <label>North:</label>
            <input type="text" id="inputNorth" class="text-input" value="S AQ; H -; D -; C -;">
          </div>
          <div class="hand-input-row">
            <label>East:</label>
            <input type="text" id="inputEast" class="text-input" value="S 54; H -; D -; C -;">
          </div>
          <div class="hand-input-row">
            <label>South:</label>
            <input type="text" id="inputSouth" class="text-input" value="S 87; H -; D -; C -;">
          </div>
          <div class="hand-input-row">
            <label>West:</label>
            <input type="text" id="inputWest" class="text-input" value="S KJ; H -; D -; C -;">
          </div>
        </div>

        <!-- Hand Requirements Toggle -->
        <details class="requirements-details">
          <summary>Hand Requirements (Fuzzy Search Constraints)</summary>
          <div class="requirements-form">
            <div class="req-player">
              <h4>West Requirements</h4>
              <div class="req-fields">
                <label>HCP Range: <input type="number" id="wHcpLow" placeholder="Min" class="mini-input"> - <input type="number" id="wHcpHigh" placeholder="Max" class="mini-input"></label>
                <label>Spades Min: <input type="number" id="wSpadesMin" placeholder="0" class="mini-input"></label>
              </div>
            </div>
            <div class="req-player">
              <h4>East Requirements</h4>
              <div class="req-fields">
                <label>HCP Range: <input type="number" id="eHcpLow" placeholder="Min" class="mini-input"> - <input type="number" id="eHcpHigh" placeholder="Max" class="mini-input"></label>
                <label>Spades Min: <input type="number" id="eSpadesMin" placeholder="0" class="mini-input"></label>
              </div>
            </div>
          </div>
        </details>

        <!-- Solve Action Button -->
        <div class="solve-action-container">
          <button id="solveBtn" class="btn btn-primary btn-large">
            <span class="btn-icon">⚡</span> Solve Deal & Build Tree
          </button>
        </div>
      </section>

      <!-- Right Column: Results & Decision Tree -->
      <section class="card-panel decision-tree-panel">
        <div class="panel-header">
          <h2>Solver Results & Decision Tree</h2>
          <div class="zoom-controls">
            <button id="zoomInBtn" class="btn btn-icon-only" title="Zoom In">+</button>
            <button id="zoomOutBtn" class="btn btn-icon-only" title="Zoom Out">-</button>
            <button id="resetZoomBtn" class="btn btn-secondary-sm">Reset View</button>
          </div>
        </div>

        <!-- Progress / Status Bar -->
        <div id="statusBox" class="status-box status-idle">
          <div class="status-indicator"></div>
          <span id="statusMessage">Ready to solve. Configure hands and click "Solve Deal".</span>
          <span id="execTime" class="exec-time"></span>
        </div>

        <!-- Result Stats Cards -->
        <div id="resultsSummary" class="results-summary hidden">
          <div class="stat-card">
            <span class="stat-label">Tricks Won</span>
            <span id="statTricks" class="stat-value">-</span>
          </div>
          <div class="stat-card">
            <span class="stat-label">Optimal Lead</span>
            <span id="statLead" class="stat-value">-</span>
          </div>
          <div class="stat-card">
            <span class="stat-label">Covered Boards</span>
            <span id="statBoards" class="stat-value">-</span>
          </div>
        </div>

        <!-- SVG Interactive Decision Tree -->
        <div class="tree-viewport" id="treeViewport">
          <svg id="treeSvg" width="100%" height="100%" xmlns="http://www.w3.org/2000/svg">
            <defs>
              <marker id="arrow" viewBox="0 0 10 10" refX="6" refY="5" markerWidth="6" markerHeight="6" orient="auto-start-reverse">
                <path d="M 0 0 L 10 5 L 0 10 z" fill="#4a6572"/>
              </marker>
            </defs>
            <g id="treeRootGroup">
              <text x="50%" y="50%" text-anchor="middle" fill="#6c757d" font-size="16">
                Click "Solve Deal" to render the interactive SVG decision tree.
              </text>
            </g>
          </svg>
        </div>

        <!-- Node Detail Banner -->
        <div id="nodeDetailBanner" class="node-detail-banner hidden">
          <h4>Selected Node Details</h4>
          <p id="nodeDetailContent"></p>
        </div>
      </section>
    </div>
  </main>

  <script src="/app.js"></script>
</body>
</html>
)rawhtml";

inline const char* STYLES_CSS = R"rawcss(
:root {
  --bg-dark: #0f172a;
  --panel-bg: #1e293b;
  --panel-border: #334155;
  --text-main: #f8fafc;
  --text-muted: #94a3b8;
  --primary: #3b82f6;
  --primary-hover: #2563eb;
  --accent-green: #10b981;
  --accent-amber: #f59e0b;
  --suit-red: #ef4444;
  --suit-black: #cbd5e1;
}

* {
  box-sizing: border-box;
  margin: 0;
  padding: 0;
}

body {
  font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
  background-color: var(--bg-dark);
  color: var(--text-main);
  line-height: 1.5;
  padding-bottom: 24px;
}

.app-header {
  background-color: #0b1120;
  border-bottom: 1px solid var(--panel-border);
  padding: 12px 24px;
}

.header-content {
  max-width: 1600px;
  margin: 0 auto;
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.logo {
  display: flex;
  align-items: center;
  gap: 10px;
}

.spade-icon {
  font-size: 28px;
  color: var(--primary);
}

.logo h1 {
  font-size: 20px;
  font-weight: 700;
  letter-spacing: -0.5px;
}

.preset-selector {
  display: flex;
  align-items: center;
  gap: 10px;
}

.grid-container {
  max-width: 1600px;
  margin: 20px auto 0;
  padding: 0 24px;
  display: grid;
  grid-template-columns: 580px 1fr;
  gap: 20px;
}

@media (max-width: 1200px) {
  .grid-container {
    grid-template-columns: 1fr;
  }
}

.card-panel {
  background-color: var(--panel-bg);
  border: 1px solid var(--panel-border);
  border-radius: 12px;
  padding: 20px;
  display: flex;
  flex-direction: column;
  gap: 16px;
}

.panel-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding-bottom: 12px;
  border-bottom: 1px solid var(--panel-border);
}

.panel-header h2 {
  font-size: 18px;
  font-weight: 600;
}

.panel-badge {
  background-color: rgba(59, 130, 246, 0.15);
  color: var(--primary);
  padding: 4px 10px;
  border-radius: 20px;
  font-size: 12px;
  font-weight: 600;
}

/* SVG Deal Container */
.svg-deal-container {
  width: 100%;
  border-radius: 8px;
  overflow: hidden;
  box-shadow: 0 4px 12px rgba(0,0,0,0.3);
}

.hand-box {
  fill: #11261d;
  stroke: #2a5a43;
  stroke-width: 1.5;
  transition: all 0.2s ease;
}

.hand-box:hover {
  stroke: var(--primary);
  fill: #163327;
}

.hand-title {
  fill: #6ee7b7;
  font-size: 12px;
  font-weight: 700;
  letter-spacing: 0.5px;
}

.compass-label {
  fill: #94a3b8;
  font-size: 11px;
  font-weight: 600;
}

.compass-trump, .compass-leader {
  fill: #f8fafc;
  font-size: 13px;
  font-weight: 600;
}

.card-suit-label {
  font-size: 13px;
  font-weight: bold;
}

.card-suit-s, .card-suit-c { fill: #f8fafc; }
.card-suit-h, .card-suit-d { fill: #f87171; }

.card-values {
  fill: #cbd5e1;
  font-size: 13px;
  font-family: monospace;
}

/* Form Controls */
.controls-grid {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 12px;
}

.control-group {
  display: flex;
  flex-direction: column;
  gap: 4px;
}

.control-group label {
  font-size: 13px;
  color: var(--text-muted);
  font-weight: 500;
}

.select-input, .text-input, .number-input, .mini-input {
  background-color: #0f172a;
  border: 1px solid var(--panel-border);
  color: var(--text-main);
  padding: 8px 12px;
  border-radius: 6px;
  font-size: 14px;
  outline: none;
}

.select-input:focus, .text-input:focus, .number-input:focus {
  border-color: var(--primary);
}

.mini-input {
  width: 60px;
  padding: 4px 6px;
  font-size: 12px;
}

.range-input {
  accent-color: var(--primary);
  cursor: pointer;
}

.hand-inputs-accordion {
  background-color: #0f172a;
  border-radius: 8px;
  padding: 12px;
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.hand-inputs-accordion h3 {
  font-size: 13px;
  color: var(--text-muted);
  text-transform: uppercase;
  letter-spacing: 0.5px;
}

.hand-input-row {
  display: flex;
  align-items: center;
  gap: 10px;
}

.hand-input-row label {
  width: 50px;
  font-size: 13px;
  font-weight: 600;
  color: #94a3b8;
}

.hand-input-row input {
  flex: 1;
}

.requirements-details {
  background-color: #0f172a;
  border-radius: 8px;
  padding: 10px 14px;
  font-size: 13px;
}

.requirements-details summary {
  cursor: pointer;
  font-weight: 600;
  color: var(--primary);
}

.requirements-form {
  margin-top: 10px;
  display: flex;
  flex-direction: column;
  gap: 10px;
}

.req-player h4 {
  font-size: 12px;
  color: var(--text-muted);
}

.req-fields {
  display: flex;
  gap: 12px;
  margin-top: 4px;
}

.btn {
  padding: 8px 16px;
  border-radius: 6px;
  font-size: 14px;
  font-weight: 600;
  cursor: pointer;
  border: none;
  transition: all 0.15s ease;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  gap: 6px;
}

.btn-primary {
  background-color: var(--primary);
  color: #ffffff;
}

.btn-primary:hover {
  background-color: var(--primary-hover);
}

.btn-secondary {
  background-color: #334155;
  color: var(--text-main);
}

.btn-secondary:hover {
  background-color: #475569;
}

.btn-secondary-sm {
  background-color: #334155;
  color: var(--text-main);
  padding: 4px 8px;
  font-size: 12px;
}

.btn-icon-only {
  background-color: #334155;
  color: var(--text-main);
  width: 28px;
  height: 28px;
  padding: 0;
  font-size: 16px;
  border-radius: 4px;
}

.btn-large {
  width: 100%;
  padding: 12px;
  font-size: 16px;
}

/* Status & Results */
.status-box {
  padding: 10px 14px;
  border-radius: 8px;
  display: flex;
  align-items: center;
  gap: 10px;
  font-size: 14px;
}

.status-idle { background-color: #0f172a; color: var(--text-muted); }
.status-running { background-color: rgba(245, 158, 11, 0.15); color: #fcd34d; border: 1px solid rgba(245, 158, 11, 0.3); }
.status-success { background-color: rgba(16, 185, 129, 0.15); color: #6ee7b7; border: 1px solid rgba(16, 185, 129, 0.3); }
.status-error { background-color: rgba(239, 68, 68, 0.15); color: #fca5a5; border: 1px solid rgba(239, 68, 68, 0.3); }

.status-indicator {
  width: 10px;
  height: 10px;
  border-radius: 50%;
  background-color: currentColor;
}

.exec-time {
  margin-left: auto;
  font-size: 12px;
  opacity: 0.8;
  font-family: monospace;
}

.results-summary {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 12px;
}

.stat-card {
  background-color: #0f172a;
  border-radius: 8px;
  padding: 12px;
  display: flex;
  flex-direction: column;
  align-items: center;
}

.stat-label {
  font-size: 11px;
  color: var(--text-muted);
  text-transform: uppercase;
}

.stat-value {
  font-size: 20px;
  font-weight: 700;
  color: var(--primary);
}

.hidden { display: none !important; }

/* Tree Viewport */
.tree-viewport {
  width: 100%;
  height: 520px;
  background-color: #0b1120;
  border: 1px solid var(--panel-border);
  border-radius: 8px;
  overflow: hidden;
  position: relative;
  cursor: grab;
}

.tree-viewport:active {
  cursor: grabbing;
}

/* Tree Nodes SVG Styling */
.node-card {
  rx: 8;
  ry: 8;
  stroke-width: 1.5;
  transition: all 0.15s ease;
  cursor: pointer;
}

.node-action { fill: #1e293b; stroke: #3b82f6; }
.node-condition { fill: #1e1b4b; stroke: #818cf8; }
.node-outcome { fill: #064e3b; stroke: #10b981; }

.node-card:hover {
  filter: brightness(1.2);
  stroke-width: 2.5;
}

.node-text-title {
  fill: #ffffff;
  font-size: 13px;
  font-weight: 600;
}

.node-text-detail {
  fill: #94a3b8;
  font-size: 11px;
}

.tree-link {
  fill: none;
  stroke: #334155;
  stroke-width: 2;
  transition: stroke 0.2s ease;
}

.node-detail-banner {
  background-color: #0f172a;
  border: 1px solid var(--panel-border);
  border-radius: 8px;
  padding: 12px;
}

.node-detail-banner h4 {
  font-size: 13px;
  color: var(--primary);
  margin-bottom: 4px;
}

.node-detail-banner p {
  font-size: 13px;
  color: var(--text-main);
  font-family: monospace;
}
)rawcss";

inline const char* APP_JS = R"rawjs(
document.addEventListener('DOMContentLoaded', () => {
  // DOM Elements
  const trumpSelect = document.getElementById('trumpSelect');
  const leaderSelect = document.getElementById('leaderSelect');
  const targetTricks = document.getElementById('targetTricks');
  const targetTricksVal = document.getElementById('targetTricksVal');
  const numIters = document.getElementById('numIters');
  const presetSelect = document.getElementById('presetSelect');
  const loadPresetBtn = document.getElementById('loadPresetBtn');
  const solveBtn = document.getElementById('solveBtn');

  const inputNorth = document.getElementById('inputNorth');
  const inputEast = document.getElementById('inputEast');
  const inputSouth = document.getElementById('inputSouth');
  const inputWest = document.getElementById('inputWest');

  const statusBox = document.getElementById('statusBox');
  const statusMessage = document.getElementById('statusMessage');
  const execTime = document.getElementById('execTime');
  const resultsSummary = document.getElementById('resultsSummary');
  const statTricks = document.getElementById('statTricks');
  const statLead = document.getElementById('statLead');
  const statBoards = document.getElementById('statBoards');

  const svgTrumpText = document.getElementById('svgTrumpText');
  const svgLeaderText = document.getElementById('svgLeaderText');
  const treeSvg = document.getElementById('treeSvg');
  const treeRootGroup = document.getElementById('treeRootGroup');
  const treeViewport = document.getElementById('treeViewport');

  const zoomInBtn = document.getElementById('zoomInBtn');
  const zoomOutBtn = document.getElementById('zoomOutBtn');
  const resetZoomBtn = document.getElementById('resetZoomBtn');
  const nodeDetailBanner = document.getElementById('nodeDetailBanner');
  const nodeDetailContent = document.getElementById('nodeDetailContent');

  // Zoom & Pan State
  let zoomLevel = 1.0;
  let panX = 0;
  let panY = 0;
  let isDragging = false;
  let startX = 0;
  let startY = 0;
  let currentTreeData = null;

  // Presets Definition
  const PRESETS = {
    finesse: {
      trump: 'SPADES',
      leader: 'NORTH',
      target: 2,
      iters: 10,
      north: 'S AQ; H -; D -; C -;',
      east: 'S 54; H -; D -; C -;',
      south: 'S 87; H -; D -; C -;',
      west: 'S KJ; H -; D -; C -;'
    },
    endplay: {
      trump: 'HEARTS',
      leader: 'NORTH',
      target: 3,
      iters: 20,
      north: 'S AK; H K2; D -; C -;',
      east: 'S 54; H QJ; D -; C -;',
      south: 'S QJ; H A3; D -; C -;',
      west: 'S 87; H T9; D -; C -;'
    },
    squeeze: {
      trump: 'NOTRUMP',
      leader: 'NORTH',
      target: 3,
      iters: 15,
      north: 'S AQ; H A; D K; C -;',
      east: 'S K9; H K; D A; C -;',
      south: 'S J8; H 2; D 2; C -;',
      west: 'S T7; H Q; D Q; C -;'
    },
    fuzzy: {
      trump: 'SPADES',
      leader: 'NORTH',
      target: 2,
      iters: 30,
      north: 'S AQ9',
      east: 'S 543',
      south: 'S 876',
      west: 'S KJT'
    }
  };

  // Suit Symbols & Formatting
  function getSuitSymbol(suitStr) {
    switch (suitStr) {
      case 'SPADES': return '♠';
      case 'HEARTS': return '♥';
      case 'DIAMONDS': return '♦';
      case 'CLUBS': return '♣';
      default: return 'NT';
    }
  }

  // Parse Hand String to Suit Cards
  function parseHandToSuits(str) {
    const suits = { S: '', H: '', D: '', C: '' };
    if (!str) return suits;
    
    // Check if standard format "S AK2; H QJ3; ..."
    const parts = str.split(';');
    parts.forEach(p => {
      const trimmed = p.trim();
      if (!trimmed) return;
      const suitChar = trimmed.charAt(0).toUpperCase();
      if (suits.hasOwnProperty(suitChar)) {
        suits[suitChar] = trimmed.substring(1).trim();
      }
    });

    // If empty, try simple parsing
    if (!suits.S && !suits.H && !suits.D && !suits.C) {
      let curSuit = 'S';
      for (let i = 0; i < str.length; i++) {
        const c = str.charAt(i).toUpperCase();
        if (suits.hasOwnProperty(c)) {
          curSuit = c;
        } else if (c !== ' ' && c !== '-') {
          suits[curSuit] += c;
        }
      }
    }
    return suits;
  }

  // Render Hand on SVG Compass
  function renderSvgHand(groupId, handStr) {
    const group = document.getElementById(groupId);
    if (!group) return;
    group.innerHTML = '';

    const suitsData = parseHandToSuits(handStr);
    const suitOrder = [
      { key: 'S', symbol: '♠', class: 'card-suit-s' },
      { key: 'H', symbol: '♥', class: 'card-suit-h' },
      { key: 'D', symbol: '♦', class: 'card-suit-d' },
      { key: 'C', symbol: '♣', class: 'card-suit-c' }
    ];

    let yOffset = 0;
    suitOrder.forEach(s => {
      const cards = suitsData[s.key] || '-';
      const textNode = document.createElementNS('http://www.w3.org/2000/svg', 'text');
      textNode.setAttribute('x', '0');
      textNode.setAttribute('y', yOffset + 15);
      
      const symbolSpan = document.createElementNS('http://www.w3.org/2000/svg', 'tspan');
      symbolSpan.setAttribute('class', `card-suit-label ${s.class}`);
      symbolSpan.textContent = s.symbol + ' ';

      const valSpan = document.createElementNS('http://www.w3.org/2000/svg', 'tspan');
      valSpan.setAttribute('class', 'card-values');
      valSpan.textContent = cards;

      textNode.appendChild(symbolSpan);
      textNode.appendChild(valSpan);
      group.appendChild(textNode);

      yOffset += 22;
    });
  }

  // Update SVG Deal Diagram from UI Inputs
  function updateDealSvg() {
    renderSvgHand('cardsNorthGroup', inputNorth.value);
    renderSvgHand('cardsEastGroup', inputEast.value);
    renderSvgHand('cardsSouthGroup', inputSouth.value);
    renderSvgHand('cardsWestGroup', inputWest.value);

    svgTrumpText.textContent = 'Trump: ' + getSuitSymbol(trumpSelect.value) + ' ' + trumpSelect.value;
    svgLeaderText.textContent = 'Leader: ' + leaderSelect.value;
    targetTricksVal.textContent = targetTricks.value;
  }

  // Sync Input Events
  [inputNorth, inputEast, inputSouth, inputWest, trumpSelect, leaderSelect, targetTricks].forEach(el => {
    el.addEventListener('input', updateDealSvg);
    el.addEventListener('change', updateDealSvg);
  });

  // Load Preset
  loadPresetBtn.addEventListener('click', () => {
    const key = presetSelect.value;
    const p = PRESETS[key];
    if (!p) return;

    trumpSelect.value = p.trump;
    leaderSelect.value = p.leader;
    targetTricks.value = p.target;
    numIters.value = p.iters;

    inputNorth.value = p.north;
    inputEast.value = p.east;
    inputSouth.value = p.south;
    inputWest.value = p.west;

    updateDealSvg();
  });

  // Solve Deal Event
  solveBtn.addEventListener('click', async () => {
    setStatus('running', 'Solving deal and analyzing optimal decision tree...');
    solveBtn.disabled = true;
    const startTime = performance.now();

    const payload = {
      hands: {
        N: inputNorth.value,
        E: inputEast.value,
        S: inputSouth.value,
        W: inputWest.value
      },
      trump: trumpSelect.value,
      leader: leaderSelect.value,
      min_tricks: parseInt(targetTricks.value, 10),
      num_iters: parseInt(numIters.value, 10)
    };

    // Include hand requirements if entered
    const wHcpLow = document.getElementById('wHcpLow').value;
    const wHcpHigh = document.getElementById('wHcpHigh').value;
    if (wHcpLow || wHcpHigh) {
      payload.requirements = {
        W: {
          hcp_low: wHcpLow ? parseInt(wHcpLow, 10) : 0,
          hcp_high: wHcpHigh ? parseInt(wHcpHigh, 10) : 37
        }
      };
    }

    try {
      const response = await fetch('/api/solve', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(payload)
      });

      const data = await response.json();
      const elapsed = (performance.now() - startTime).toFixed(1);

      if (!response.ok || !data.success) {
        setStatus('error', 'Solver failed: ' + (data.error || 'Unknown error'), `${elapsed} ms`);
        solveBtn.disabled = false;
        return;
      }

      setStatus('success', 'Solve complete!', `${elapsed} ms`);
      execTime.textContent = `${elapsed} ms`;

      // Show summary
      resultsSummary.classList.remove('hidden');
      statTricks.textContent = data.tricks_won !== undefined ? data.tricks_won : '-';
      statLead.textContent = data.best_lead || '-';
      statBoards.textContent = data.covered_boards !== undefined ? data.covered_boards : '-';

      // Render Decision Tree
      if (data.tree) {
        currentTreeData = data.tree;
        renderDecisionTree(data.tree);
      } else {
        treeRootGroup.innerHTML = `<text x="50%" y="50%" text-anchor="middle" fill="#f87171">No decision tree generated.</text>`;
      }

    } catch (err) {
      console.error(err);
      setStatus('error', 'Network or server error: ' + err.message);
    } finally {
      solveBtn.disabled = false;
    }
  });

  function setStatus(type, msg, timeStr = '') {
    statusBox.className = `status-box status-${type}`;
    statusMessage.textContent = msg;
    if (timeStr) execTime.textContent = timeStr;
  }

  // Decision Tree SVG Rendering
  function renderDecisionTree(rootNode) {
    treeRootGroup.innerHTML = '';
    panX = 40;
    panY = 40;
    zoomLevel = 1.0;
    updateTransform();

    // Calculate layout coordinates recursively
    const nodeWidth = 220;
    const nodeHeight = 70;
    const levelHeight = 110;
    const siblingGap = 30;

    let leafCounter = 0;

    function layoutNode(node, depth = 0) {
      node._depth = depth;
      if (!node.children || node.children.length === 0 || node._collapsed) {
        node._x = leafCounter * (nodeWidth + siblingGap);
        leafCounter++;
        node._width = nodeWidth;
      } else {
        node.children.forEach(child => layoutNode(child, depth + 1));
        const firstChild = node.children[0];
        const lastChild = node.children[node.children.length - 1];
        node._x = (firstChild._x + lastChild._x) / 2;
      }
      node._y = depth * levelHeight;
    }

    layoutNode(rootNode);

    // Draw Links & Nodes recursively
    function drawTree(node) {
      if (node.children && node.children.length > 0 && !node._collapsed) {
        node.children.forEach(child => {
          // Curved path link
          const path = document.createElementNS('http://www.w3.org/2000/svg', 'path');
          const startX = node._x + nodeWidth / 2;
          const startY = node._y + nodeHeight;
          const endX = child._x + nodeWidth / 2;
          const endY = child._y;
          const midY = (startY + endY) / 2;

          const d = `M ${startX} ${startY} C ${startX} ${midY}, ${endX} ${midY}, ${endX} ${endY}`;
          path.setAttribute('d', d);
          path.setAttribute('class', 'tree-link');
          path.setAttribute('marker-end', 'url(#arrow)');
          treeRootGroup.appendChild(path);

          drawTree(child);
        });
      }

      // Render Node Group
      const g = document.createElementNS('http://www.w3.org/2000/svg', 'g');
      g.setAttribute('transform', `translate(${node._x}, ${node._y})`);
      g.style.cursor = 'pointer';

      const rect = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
      rect.setAttribute('width', nodeWidth);
      rect.setAttribute('height', nodeHeight);

      let cardClass = 'node-action';
      if (node.type === 'condition') cardClass = 'node-condition';
      if (node.type === 'outcome') cardClass = 'node-outcome';
      rect.setAttribute('class', `node-card ${cardClass}`);

      g.appendChild(rect);

      // Title Text
      const title = document.createElementNS('http://www.w3.org/2000/svg', 'text');
      title.setAttribute('x', '12');
      title.setAttribute('y', '26');
      title.setAttribute('class', 'node-text-title');
      title.textContent = truncateText(node.label || node.trick || 'Node', 26);
      g.appendChild(title);

      // Detail Text
      const detail = document.createElementNS('http://www.w3.org/2000/svg', 'text');
      detail.setAttribute('x', '12');
      detail.setAttribute('y', '48');
      detail.setAttribute('class', 'node-text-detail');
      const boardsText = node.covered_boards !== undefined ? `Boards: ${node.covered_boards}` : '';
      detail.textContent = `${node.type.toUpperCase()} | ${boardsText}`;
      g.appendChild(detail);

      // Expand/Collapse Indicator if children exist
      if (node.children && node.children.length > 0) {
        const circle = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
        circle.setAttribute('cx', nodeWidth - 16);
        circle.setAttribute('cy', 16);
        circle.setAttribute('r', '8');
        circle.setAttribute('fill', '#334155');
        g.appendChild(circle);

        const badge = document.createElementNS('http://www.w3.org/2000/svg', 'text');
        badge.setAttribute('x', nodeWidth - 16);
        badge.setAttribute('y', 20);
        badge.setAttribute('text-anchor', 'middle');
        badge.setAttribute('fill', '#ffffff');
        badge.setAttribute('font-size', '10');
        badge.setAttribute('font-weight', 'bold');
        badge.textContent = node._collapsed ? '+' : '−';
        g.appendChild(badge);
      }

      // Event Listeners for Interactivity
      g.addEventListener('click', (e) => {
        e.stopPropagation();
        if (node.children && node.children.length > 0) {
          node._collapsed = !node._collapsed;
          renderDecisionTree(rootNode);
        }
        showNodeDetails(node);
      });

      treeRootGroup.appendChild(g);
    }

    drawTree(rootNode);
  }

  function truncateText(str, maxLen) {
    if (!str) return '';
    return str.length > maxLen ? str.substring(0, maxLen - 3) + '...' : str;
  }

  function showNodeDetails(node) {
    nodeDetailBanner.classList.remove('hidden');
    nodeDetailContent.textContent = `ID: ${node.id} | Type: ${node.type} | Label: "${node.label || ''}" | Trick: "${node.trick || ''}" | Boards: ${node.covered_boards || 0}`;
  }

  // Pan & Zoom Implementation
  function updateTransform() {
    treeRootGroup.setAttribute('transform', `translate(${panX}, ${panY}) scale(${zoomLevel})`);
  }

  treeViewport.addEventListener('mousedown', (e) => {
    isDragging = true;
    startX = e.clientX - panX;
    startY = e.clientY - panY;
  });

  window.addEventListener('mousemove', (e) => {
    if (!isDragging) return;
    panX = e.clientX - startX;
    panY = e.clientY - startY;
    updateTransform();
  });

  window.addEventListener('mouseup', () => { isDragging = false; });

  treeViewport.addEventListener('wheel', (e) => {
    e.preventDefault();
    const zoomFactor = e.deltaY < 0 ? 1.1 : 0.9;
    zoomLevel = Math.max(0.3, Math.min(3.0, zoomLevel * zoomFactor));
    updateTransform();
  });

  zoomInBtn.addEventListener('click', () => { zoomLevel = Math.min(3.0, zoomLevel * 1.2); updateTransform(); });
  zoomOutBtn.addEventListener('click', () => { zoomLevel = Math.max(0.3, zoomLevel / 1.2); updateTransform(); });
  resetZoomBtn.addEventListener('click', () => { zoomLevel = 1.0; panX = 40; panY = 40; updateTransform(); });

  // Initial UI Render
  updateDealSvg();
});
)rawjs";

} // namespace web_assets
