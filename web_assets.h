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
        </select>
      </div>
    </div>
  </header>

  <main class="app-main">
    <div class="grid-container">
      <!-- Left Column: Deal Setup & SVG Deal Diagram -->
      <section class="card-panel deal-setup-panel">
        <div class="panel-header">
          <h2>Deal Configuration</h2>
          <span class="panel-badge" id="dealEndingBadge">Full deal</span>
        </div>

        <!-- SVG Deal Diagram -->
        <div class="svg-deal-container">
          <svg id="dealSvg" viewBox="0 0 600 320" xmlns="http://www.w3.org/2000/svg">
            <rect width="100%" height="100%" fill="#1a382b" rx="10"/>

            <!-- Upper Left Compass Rose -->
            <g id="svgCompassRose" transform="translate(100, 57)">
              <circle r="14" fill="#0b1b13" stroke="#2d5e46" stroke-width="1.2"/>
              <circle r="1.8" fill="#6ee7b7"/>
              <line x1="0" y1="-10" x2="0" y2="10" stroke="#2d5e46" stroke-width="1" stroke-dasharray="2,2"/>
              <line x1="-10" y1="0" x2="10" y2="0" stroke="#2d5e46" stroke-width="1" stroke-dasharray="2,2"/>
              <text x="0" y="-18" class="compass-dir-label" text-anchor="middle" dominant-baseline="middle">N</text>
              <text x="0" y="20" class="compass-dir-label" text-anchor="middle" dominant-baseline="middle">S</text>
              <text x="-20" y="1" class="compass-dir-label" text-anchor="middle" dominant-baseline="middle">W</text>
              <text x="20" y="1" class="compass-dir-label" text-anchor="middle" dominant-baseline="middle">E</text>
            </g>

            <!-- Upper Right Unknown Cards Box -->
            <g class="hand-group" id="svgUnknownHand" transform="translate(415, 12)">
              <rect width="170" height="90" rx="8" class="unknown-box" id="boxUnknown"/>
              <text x="85" y="15" class="unknown-title" text-anchor="middle">UNKNOWN CARDS</text>
              <g transform="translate(12, 18)" id="cardsUnknownGroup"></g>
            </g>

            <!-- North Hand -->
            <g class="hand-group" id="svgNorthHand" transform="translate(195, 12)">
              <rect width="210" height="90" rx="8" class="hand-box" id="boxNorth"/>
              <g transform="translate(12, 12)" id="cardsNorthGroup"></g>
            </g>

            <!-- West Hand -->
            <g class="hand-group" id="svgWestHand" transform="translate(15, 115)">
              <rect width="170" height="90" rx="8" class="hand-box" id="boxWest"/>
              <g transform="translate(12, 12)" id="cardsWestGroup"></g>
            </g>

            <!-- Center Box (Middle Controls: Trump / Leader / Target / Iters) -->
            <g id="svgCenterBox" transform="translate(195, 115)">
              <rect width="210" height="90" rx="8" fill="#0f241a" stroke="#2d5e46" stroke-width="1.5"/>
              <foreignObject x="0" y="0" width="210" height="90">
                <div class="middle-controls" xmlns="http://www.w3.org/1999/xhtml">
                  <div class="middle-control-row">
                    <div class="middle-control-group">
                      <label for="trumpSelect">Trump:</label>
                      <select id="trumpSelect" class="middle-select">
                        <option value="SPADES">♠ S</option>
                        <option value="HEARTS">♥ H</option>
                        <option value="DIAMONDS">♦ D</option>
                        <option value="CLUBS">♣ C</option>
                        <option value="NOTRUMP">NT</option>
                      </select>
                    </div>
                    <div class="middle-control-group">
                      <label for="leaderSelect">Leader:</label>
                      <select id="leaderSelect" class="middle-select">
                        <option value="NORTH">N</option>
                        <option value="EAST">E</option>
                        <option value="SOUTH">S</option>
                        <option value="WEST">W</option>
                      </select>
                    </div>
                  </div>
                  <div class="middle-control-row">
                    <div class="middle-control-group">
                      <label for="targetTricks">Target:</label>
                      <input type="number" id="targetTricks" min="1" max="13" value="2" class="middle-number-input">
                    </div>
                    <div class="middle-control-group">
                      <label for="numIters">Iters:</label>
                      <input type="number" id="numIters" min="1" max="500" value="10" class="middle-number-input">
                    </div>
                  </div>
                </div>
              </foreignObject>
            </g>

            <!-- East Hand -->
            <g class="hand-group" id="svgEastHand" transform="translate(415, 115)">
              <rect width="170" height="90" rx="8" class="hand-box" id="boxEast"/>
              <g transform="translate(12, 12)" id="cardsEastGroup"></g>
            </g>

            <!-- South Hand -->
            <g class="hand-group" id="svgSouthHand" transform="translate(195, 218)">
              <rect width="210" height="90" rx="8" class="hand-box" id="boxSouth"/>
              <g transform="translate(12, 12)" id="cardsSouthGroup"></g>
            </g>
          </svg>
        </div>

        <!-- Text Inputs for Hands -->
        <div class="hand-inputs-accordion">
          <div class="hand-inputs-header">
            <h3>Hand Text Inputs</h3>
            <button id="loadTxtBtn" type="button" class="btn btn-secondary-sm">Load</button>
            <input type="file" id="fileInputTxt" accept=".txt,.deal" style="display: none;">
          </div>
          <div class="hand-input-row">
            <div class="hand-input-group">
              <label for="inputNorth">N:</label>
              <input type="text" id="inputNorth" class="text-input mini-text-input" value="S AQ; H -; D -; C -;">
            </div>
            <div class="hand-input-group">
              <label for="inputSouth">S:</label>
              <input type="text" id="inputSouth" class="text-input mini-text-input" value="S 87; H -; D -; C -;">
            </div>
          </div>
          <div class="hand-input-row">
            <div class="hand-input-group">
              <label for="inputEast">E:</label>
              <input type="text" id="inputEast" class="text-input mini-text-input" value="S 54; H -; D -; C -;">
            </div>
            <div class="hand-input-group">
              <label for="inputWest">W:</label>
              <input type="text" id="inputWest" class="text-input mini-text-input" value="S KJ; H -; D -; C -;">
            </div>
          </div>
        </div>

        <!-- Hand Requirements Toggle -->
        <details class="requirements-details">
          <summary>Hand Requirements (Fuzzy Search Constraints)</summary>
          <div class="requirements-form">
            <div class="req-player req-unknown">
              <h4>Unknown Cards (Defender Pool)</h4>
              <div class="req-fields">
                <input type="text" id="inputUnknownCards" class="text-input" placeholder="e.g. S Kxxx or H AT9 C 32" style="width: 100%;">
              </div>
            </div>
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
          <div class="tab-buttons">
            <button id="tabDecisionTreeBtn" class="tab-btn active" type="button">Decision Tree</button>
            <button id="tabSummaryBtn" class="tab-btn" type="button">Distribution Summary <span id="ruleCounterBadge" class="badge-counter hidden">0</span></button>
          </div>
          <div class="zoom-controls">
            <button id="expandAllBtn" class="btn btn-secondary-sm">Expand All</button>
            <button id="collapseAllBtn" class="btn btn-secondary-sm">Collapse All</button>
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
          <div id="cardStatSuccess" class="stat-card stat-card-toggle active" role="button" tabindex="0" title="Click to view Success distribution summary">
            <span class="stat-label">Success Rate</span>
            <span id="statSuccess" class="stat-value">-</span>
          </div>
          <div id="cardStatFailure" class="stat-card stat-card-toggle" role="button" tabindex="0" title="Click to view Failure distribution summary">
            <span class="stat-label">Failure Rate</span>
            <span id="statFailure" class="stat-value">-</span>
          </div>
        </div>

        <!-- Tab Panel 1: Decision Tree -->
        <div id="panelDecisionTree" class="tab-panel active">
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
        </div>

        <!-- Tab Panel 2: Distribution Summary -->
        <div id="panelDistributionSummary" class="tab-panel hidden">
          <div id="summaryModeHeader" class="summary-mode-header hidden">
            <span id="summaryModeBadge" class="mode-badge mode-success">Success Deals</span>
            <span id="summaryModeSub" class="mode-sub"></span>
          </div>
          <div id="summaryEmptyNotice" class="summary-empty">
            No distribution summary available for this deal. Run a fuzzy solve to view distribution statistics.
          </div>
          <div id="summaryContent" class="summary-content hidden">
            <div class="summary-section">
              <h4>Invariant Rules</h4>
              <ul id="summaryRulesList" class="summary-rules-list"></ul>
            </div>
            <div class="summary-section">
              <h4>Suit Length Bounds</h4>
              <div class="table-responsive">
                <table id="summaryBoundsTable" class="summary-table">
                  <thead>
                    <tr>
                      <th>Suit</th>
                      <th>Hand 1 (East) Range</th>
                      <th>Hand 2 (West) Range</th>
                      <th>Combined Range</th>
                    </tr>
                  </thead>
                  <tbody></tbody>
                </table>
              </div>
            </div>
            <div class="summary-section">
              <h4>Honor Card Possession Stats</h4>
              <div class="table-responsive">
                <table id="summaryHonorTable" class="summary-table">
                  <thead>
                    <tr>
                      <th>Suit</th>
                      <th>Honor</th>
                      <th>Hand 1 (East) %</th>
                      <th>Hand 2 (West) %</th>
                      <th>Both %</th>
                      <th>Neither %</th>
                    </tr>
                  </thead>
                  <tbody></tbody>
                </table>
              </div>
            </div>
          </div>
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
  margin: 12px auto 0;
  padding: 0 20px;
  display: grid;
  grid-template-columns: 580px 1fr;
  gap: 16px;
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
  padding: 14px;
  display: flex;
  flex-direction: column;
  gap: 10px;
}

.panel-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding-bottom: 8px;
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

.unknown-box {
  fill: #1a2332;
  stroke: #f59e0b;
  stroke-width: 1.5;
  stroke-dasharray: 4,3;
  transition: all 0.2s ease;
}

.unknown-box:hover {
  fill: #222e42;
  stroke: #fbbf24;
}

.compass-dir-label {
  fill: #6ee7b7;
  font-size: 10px;
  font-weight: 700;
  font-family: sans-serif;
}

.unknown-title {
  fill: #f59e0b;
  font-size: 10px;
  font-weight: 700;
  letter-spacing: 0.5px;
}

.compass-trump, .compass-leader {
  fill: #f8fafc;
  font-size: 12px;
  font-weight: 600;
}

.card-suit-label {
  font-size: 12px;
  font-weight: bold;
}

.card-suit-s, .card-suit-c { fill: #f8fafc; }
.card-suit-h, .card-suit-d { fill: #f87171; }

.card-values {
  fill: #cbd5e1;
  font-size: 12px;
  font-family: monospace;
}

/* Form Controls */
.middle-controls {
  width: 100%;
  height: 100%;
  display: flex;
  flex-direction: column;
  justify-content: center;
  align-items: center;
  gap: 8px;
  padding: 8px 10px;
  box-sizing: border-box;
  font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
}

.middle-control-row {
  display: flex;
  justify-content: space-between;
  align-items: center;
  width: 100%;
  gap: 8px;
}

.middle-control-group {
  display: flex;
  align-items: center;
  gap: 5px;
  font-size: 12px;
  font-weight: 600;
  color: #a7f3d0;
}

.middle-select, .middle-number-input {
  background-color: #0b1b13;
  color: #f8fafc;
  border: 1px solid #2d5e46;
  border-radius: 4px;
  padding: 3px 5px;
  font-size: 12px;
  font-weight: 600;
  outline: none;
  cursor: pointer;
}

.middle-select:hover, .middle-number-input:hover {
  border-color: #6ee7b7;
}

.middle-select:focus, .middle-number-input:focus {
  border-color: #10b981;
  box-shadow: 0 0 0 2px rgba(16, 185, 129, 0.2);
}

.middle-number-input {
  width: 50px;
  text-align: center;
}

.select-input, .text-input, .number-input, .mini-input {
  background-color: #0f172a;
  border: 1px solid var(--panel-border);
  color: var(--text-main);
  padding: 6px 10px;
  border-radius: 6px;
  font-size: 13px;
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
  padding: 8px 12px;
  display: flex;
  flex-direction: column;
  gap: 6px;
}

.hand-inputs-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 2px;
}

.hand-inputs-header h3 {
  font-size: 13px;
  color: var(--text-muted);
  text-transform: uppercase;
  letter-spacing: 0.5px;
  margin: 0;
}

.hand-input-row {
  display: flex;
  align-items: center;
  gap: 12px;
}

.hand-input-group {
  display: flex;
  align-items: center;
  gap: 6px;
  flex: 1;
}

.hand-input-group label {
  width: auto;
  min-width: 16px;
  font-size: 13px;
  font-weight: 700;
  color: #94a3b8;
}

.mini-text-input {
  flex: 1;
  width: 100%;
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

.req-player.req-unknown {
  background-color: rgba(30, 41, 59, 0.7);
  border: 1px dashed rgba(245, 158, 11, 0.4);
  border-radius: 8px;
  padding: 8px 10px;
}

.req-player.req-unknown h4 {
  color: #f59e0b;
  font-weight: 600;
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
  grid-template-columns: repeat(4, 1fr);
  gap: 10px;
}

.stat-card {
  background-color: #0f172a;
  border-radius: 8px;
  padding: 10px;
  display: flex;
  flex-direction: column;
  align-items: center;
}

.stat-card-toggle {
  cursor: pointer;
  border: 1px solid transparent;
  transition: all 0.2s ease;
  user-select: none;
}

.stat-card-toggle:hover {
  border-color: var(--panel-border);
  background-color: #1a2436;
}

#cardStatSuccess .stat-value {
  color: #10b981;
}

#cardStatFailure .stat-value {
  color: #f59e0b;
}

#cardStatSuccess.active {
  border-color: #10b981;
  background-color: #0d2820;
  box-shadow: 0 0 10px rgba(16, 185, 129, 0.25);
}

#cardStatFailure.active {
  border-color: #f59e0b;
  background-color: #262115;
  box-shadow: 0 0 10px rgba(245, 158, 11, 0.25);
}

.summary-mode-header {
  display: flex;
  align-items: center;
  gap: 10px;
  margin-bottom: 12px;
  padding: 8px 12px;
  background-color: #0f172a;
  border-radius: 6px;
  border: 1px solid var(--panel-border);
}

.mode-badge {
  padding: 3px 8px;
  border-radius: 12px;
  font-size: 11px;
  font-weight: 700;
  text-transform: uppercase;
}

.mode-badge.mode-success {
  background-color: rgba(16, 185, 129, 0.2);
  color: #6ee7b7;
  border: 1px solid rgba(16, 185, 129, 0.4);
}

.mode-badge.mode-failure {
  background-color: rgba(245, 158, 11, 0.2);
  color: #fcd34d;
  border: 1px solid rgba(245, 158, 11, 0.4);
}

.mode-sub {
  font-size: 12px;
  color: var(--text-muted);
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

/* Tabbed Inspector Navigation Header */
.tab-buttons {
  display: flex;
  gap: 4px;
}

.tab-btn {
  background: transparent;
  border: none;
  border-bottom: 2px solid transparent;
  color: var(--text-muted);
  font-size: 14px;
  font-weight: 600;
  padding: 8px 16px;
  cursor: pointer;
  display: inline-flex;
  align-items: center;
  gap: 6px;
  transition: all 0.2s ease;
}

.tab-btn:hover {
  color: var(--text-main);
}

.tab-btn.active {
  color: var(--primary);
  border-bottom-color: var(--primary);
}

.badge-counter {
  background-color: var(--primary);
  color: #ffffff;
  border-radius: 12px;
  padding: 2px 8px;
  font-size: 11px;
  font-weight: 700;
}

.badge-counter.hidden, .tab-panel.hidden, .summary-content.hidden, .summary-empty.hidden {
  display: none !important;
}

.tab-panel {
  display: flex;
  flex-direction: column;
  flex: 1;
  min-height: 0;
  gap: 16px;
}

.tab-panel.hidden {
  display: none !important;
}

/* Distribution Summary Tab Styles */
.summary-empty {
  padding: 30px;
  text-align: center;
  color: var(--text-muted);
  font-size: 14px;
  background-color: var(--bg-dark);
  border-radius: 8px;
  border: 1px dashed var(--panel-border);
}

.summary-content {
  display: flex;
  flex-direction: column;
  gap: 20px;
  overflow-y: auto;
  max-height: 520px;
  padding-right: 6px;
}

.summary-section {
  background-color: var(--bg-dark);
  border: 1px solid var(--panel-border);
  border-radius: 8px;
  padding: 16px;
}

.summary-section h4 {
  font-size: 14px;
  font-weight: 600;
  color: var(--primary);
  margin-bottom: 12px;
  padding-bottom: 6px;
  border-bottom: 1px solid var(--panel-border);
}

.summary-rules-list {
  list-style: none;
  padding: 0;
  margin: 0;
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.summary-rules-list li {
  font-family: monospace;
  font-size: 13px;
  color: #34d399;
  background-color: rgba(16, 185, 129, 0.1);
  padding: 6px 12px;
  border-radius: 6px;
  border-left: 3px solid var(--accent-green);
}

.table-responsive {
  overflow-x: auto;
}

.summary-table {
  width: 100%;
  border-collapse: collapse;
  font-size: 13px;
  text-align: left;
}

.summary-table th, .summary-table td {
  padding: 8px 12px;
  border-bottom: 1px solid var(--panel-border);
}

.summary-table th {
  color: var(--text-muted);
  font-weight: 600;
  background-color: rgba(255, 255, 255, 0.03);
}

.summary-table tr:hover td {
  background-color: rgba(255, 255, 255, 0.02);
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
  const solveBtn = document.getElementById('solveBtn');

  const inputNorth = document.getElementById('inputNorth');
  const inputEast = document.getElementById('inputEast');
  const inputSouth = document.getElementById('inputSouth');
  const inputWest = document.getElementById('inputWest');
  const inputUnknownCards = document.getElementById('inputUnknownCards');

  const statusBox = document.getElementById('statusBox');
  const statusMessage = document.getElementById('statusMessage');
  const execTime = document.getElementById('execTime');
  const resultsSummary = document.getElementById('resultsSummary');
  const statTricks = document.getElementById('statTricks');
  const statLead = document.getElementById('statLead');
  const cardStatSuccess = document.getElementById('cardStatSuccess');
  const cardStatFailure = document.getElementById('cardStatFailure');
  const statSuccess = document.getElementById('statSuccess');
  const statFailure = document.getElementById('statFailure');
  const summaryModeHeader = document.getElementById('summaryModeHeader');
  const summaryModeBadge = document.getElementById('summaryModeBadge');
  const summaryModeSub = document.getElementById('summaryModeSub');

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

  // Tab Switcher Elements
  const tabDecisionTreeBtn = document.getElementById('tabDecisionTreeBtn');
  const tabSummaryBtn = document.getElementById('tabSummaryBtn');
  const panelDecisionTree = document.getElementById('panelDecisionTree');
  const panelDistributionSummary = document.getElementById('panelDistributionSummary');
  const ruleCounterBadge = document.getElementById('ruleCounterBadge');
  const summaryEmptyNotice = document.getElementById('summaryEmptyNotice');
  const summaryContent = document.getElementById('summaryContent');
  const summaryRulesList = document.getElementById('summaryRulesList');
  const summaryBoundsTable = document.getElementById('summaryBoundsTable');
  const summaryHonorTable = document.getElementById('summaryHonorTable');

  // Tab Switching Event Listeners
  if (tabDecisionTreeBtn && tabSummaryBtn) {
    tabDecisionTreeBtn.addEventListener('click', () => {
      tabDecisionTreeBtn.classList.add('active');
      tabSummaryBtn.classList.remove('active');
      panelDecisionTree.classList.remove('hidden');
      panelDistributionSummary.classList.add('hidden');
    });

    tabSummaryBtn.addEventListener('click', () => {
      tabSummaryBtn.classList.add('active');
      tabDecisionTreeBtn.classList.remove('active');
      panelDistributionSummary.classList.remove('hidden');
      panelDecisionTree.classList.add('hidden');
    });
  }

  // Zoom & Pan State
  let zoomLevel = 1.0;
  let panX = 0;
  let panY = 0;
  let isDragging = false;
  let startX = 0;
  let startY = 0;
  let currentTreeData = null;
  let currentSolveData = null;
  let activeViewMode = 'success'; // 'success' or 'failure'

  function updateViewMode(mode) {
    activeViewMode = mode;
    if (mode === 'success') {
      if (cardStatSuccess) cardStatSuccess.classList.add('active');
      if (cardStatFailure) cardStatFailure.classList.remove('active');
      if (summaryModeBadge) {
        summaryModeBadge.className = 'mode-badge mode-success';
        summaryModeBadge.textContent = 'Success Deals';
      }
      if (summaryModeSub && currentSolveData) {
        const total = currentSolveData.total_boards || 1;
        const success = currentSolveData.success_boards !== undefined ? currentSolveData.success_boards : (currentSolveData.covered_boards || 0);
        const pct = Math.round((success / total) * 100);
        summaryModeSub.textContent = `(${success}/${total} boards, ${pct}%)`;
      }
      if (summaryModeHeader) summaryModeHeader.classList.remove('hidden');
      if (currentSolveData && currentSolveData.distribution_summary) {
        renderDistributionSummary(currentSolveData.distribution_summary);
      } else {
        clearDistributionSummary('No distribution summary available for success deals.');
      }
    } else {
      if (cardStatFailure) cardStatFailure.classList.add('active');
      if (cardStatSuccess) cardStatSuccess.classList.remove('active');
      if (summaryModeBadge) {
        summaryModeBadge.className = 'mode-badge mode-failure';
        summaryModeBadge.textContent = 'Failure Deals';
      }
      if (summaryModeSub && currentSolveData) {
        const total = currentSolveData.total_boards || 1;
        const success = currentSolveData.success_boards !== undefined ? currentSolveData.success_boards : (currentSolveData.covered_boards || 0);
        const failure = currentSolveData.failure_boards !== undefined ? currentSolveData.failure_boards : Math.max(0, total - success);
        const pct = Math.round((failure / total) * 100);
        summaryModeSub.textContent = `(${failure}/${total} boards, ${pct}%)`;
      }
      if (summaryModeHeader) summaryModeHeader.classList.remove('hidden');
      if (currentSolveData && currentSolveData.failure_distribution_summary) {
        renderDistributionSummary(currentSolveData.failure_distribution_summary);
      } else {
        clearDistributionSummary('No failure distribution summary available (100% success rate or no failure deals).');
      }
    }
  }

  if (cardStatSuccess) {
    cardStatSuccess.addEventListener('click', () => {
      updateViewMode('success');
      if (tabSummaryBtn && panelDistributionSummary.classList.contains('hidden')) tabSummaryBtn.click();
    });
    cardStatSuccess.addEventListener('keydown', (e) => {
      if (e.key === 'Enter' || e.key === ' ') {
        updateViewMode('success');
        if (tabSummaryBtn && panelDistributionSummary.classList.contains('hidden')) tabSummaryBtn.click();
      }
    });
  }

  if (cardStatFailure) {
    cardStatFailure.addEventListener('click', () => {
      updateViewMode('failure');
      if (tabSummaryBtn && panelDistributionSummary.classList.contains('hidden')) tabSummaryBtn.click();
    });
    cardStatFailure.addEventListener('keydown', (e) => {
      if (e.key === 'Enter' || e.key === ' ') {
        updateViewMode('failure');
        if (tabSummaryBtn && panelDistributionSummary.classList.contains('hidden')) tabSummaryBtn.click();
      }
    });
  }

  // Presets Definition
  const PRESETS = {
    finesse: {
      trump: 'SPADES',
      leader: 'SOUTH',
      target: 2,
      iters: 10,
      north: 'S AQ; H -; D -; C -;',
      east: '',
      south: 'S 87; H -; D -; C -;',
      west: '',
      unknown_cards: 'S Kxxx'
    },
    endplay: {
      trump: 'HEARTS',
      leader: 'SOUTH',
      target: 3,
      iters: 30,
      north: 'S AQT; H 2; D -; C -;',
      east: '',
      south: 'S 876; H A; D -; C -;',
      west: '',
      unknown_cards: 'S KJ9543 D 34'
    },
    squeeze: {
      trump: 'NOTRUMP',
      leader: 'NORTH',
      target: 3,
      iters: 15,
      north: 'S AQ; H A; D K; C -;',
      east: 'S K9; H K; D A; C -;',
      south: 'S J8; H 2; D 2; C -;',
      west: 'S T7; H Q; D Q; C -;',
      unknown_cards: ''
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

  function countHandCards(str) {
    if (!str) return 0;
    const suits = parseHandToSuits(str);
    let cnt = 0;
    ['S', 'H', 'D', 'C'].forEach(s => {
      const val = suits[s];
      if (val && val !== '-') {
        cnt += val.length;
      }
    });
    return cnt;
  }

  function countUnknownCardsString(str) {
    if (!str) return 0;
    let cnt = 0;
    let i = 0;
    while (i < str.length) {
      const c = str.charAt(i);
      if (c === '1' && i + 1 < str.length && str.charAt(i + 1) === '0') {
        cnt++;
        i += 2;
        continue;
      }
      if ("AKQJT23456789xX".indexOf(c) !== -1) {
        cnt++;
      }
      i++;
    }
    return cnt;
  }

  function updateDealBadge() {
    const dealEndingBadge = document.getElementById('dealEndingBadge');
    if (!dealEndingBadge) return;

    const nCards = countHandCards(inputNorth.value);
    const eCards = countHandCards(inputEast.value);
    const sCards = countHandCards(inputSouth.value);
    const wCards = countHandCards(inputWest.value);

    let unkCards = 0;
    if (inputUnknownCards && inputUnknownCards.value.trim()) {
      unkCards = countUnknownCardsString(inputUnknownCards.value.trim());
    }

    let maxCards = Math.max(nCards, eCards, sCards, wCards);
    if (eCards === 0 && wCards === 0 && unkCards > 0) {
      maxCards = Math.max(nCards, sCards, Math.ceil(unkCards / 2));
    }

    if (maxCards > 0 && maxCards <= 12) {
      dealEndingBadge.textContent = `${maxCards} card ending`;
    } else {
      dealEndingBadge.textContent = 'Full deal';
    }
  }

  // Render Hand or Unknown Cards on SVG Diagram
  function renderSvgHand(groupId, handStr, isUnknown = false) {
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
    let hasCards = false;
    suitOrder.forEach(s => {
      const cards = suitsData[s.key];
      if (cards && cards !== '-') hasCards = true;
    });

    if (isUnknown && !hasCards) {
      const textNode = document.createElementNS('http://www.w3.org/2000/svg', 'text');
      textNode.setAttribute('x', '0');
      textNode.setAttribute('y', '22');
      textNode.setAttribute('fill', '#64748b');
      textNode.setAttribute('font-size', '12');
      textNode.setAttribute('font-style', 'italic');
      textNode.textContent = '(none)';
      group.appendChild(textNode);
      return;
    }

    suitOrder.forEach(s => {
      const cards = suitsData[s.key] || '-';
      const textNode = document.createElementNS('http://www.w3.org/2000/svg', 'text');
      textNode.setAttribute('x', '0');
      textNode.setAttribute('y', yOffset + 12);
      
      const symbolSpan = document.createElementNS('http://www.w3.org/2000/svg', 'tspan');
      symbolSpan.setAttribute('class', `card-suit-label ${s.class}`);
      symbolSpan.textContent = s.symbol + ' ';

      const valSpan = document.createElementNS('http://www.w3.org/2000/svg', 'tspan');
      valSpan.setAttribute('class', 'card-values');
      valSpan.textContent = cards;

      textNode.appendChild(symbolSpan);
      textNode.appendChild(valSpan);
      group.appendChild(textNode);

      yOffset += 15;
    });
  }

  // Update SVG Deal Diagram from UI Inputs
  function updateDealSvg() {
    renderSvgHand('cardsNorthGroup', inputNorth.value);
    renderSvgHand('cardsEastGroup', inputEast.value);
    renderSvgHand('cardsSouthGroup', inputSouth.value);
    renderSvgHand('cardsWestGroup', inputWest.value);

    if (inputUnknownCards) {
      renderSvgHand('cardsUnknownGroup', inputUnknownCards.value.trim(), true);
    }

    const svgTrumpText = document.getElementById('svgTrumpText');
    const svgLeaderText = document.getElementById('svgLeaderText');
    const targetTricksVal = document.getElementById('targetTricksVal');

    if (svgTrumpText && trumpSelect) {
      const trumpVal = trumpSelect.value;
      const suitSym = getSuitSymbol(trumpVal);
      const isRedTrump = (trumpVal === 'HEARTS' || trumpVal === 'DIAMONDS');
      svgTrumpText.innerHTML = 'Trump: <tspan class="' + (isRedTrump ? 'card-suit-h' : '') + '">' + suitSym + '</tspan>';
    }
    if (svgLeaderText && leaderSelect) {
      svgLeaderText.textContent = 'Leader: ' + leaderSelect.value.charAt(0);
    }
    if (targetTricksVal && targetTricks) {
      targetTricksVal.textContent = targetTricks.value;
    }

    updateDealBadge();
  }

  // Sync Input Events
  [inputNorth, inputEast, inputSouth, inputWest, inputUnknownCards, trumpSelect, leaderSelect, targetTricks, numIters].forEach(el => {
    if (el) {
      el.addEventListener('input', updateDealSvg);
      el.addEventListener('change', updateDealSvg);
    }
  });

  function loadPreset(key) {
    const p = PRESETS[key];
    if (!p) return;

    trumpSelect.value = p.trump;
    leaderSelect.value = p.leader;
    targetTricks.value = p.target;
    numIters.value = p.iters;

    inputNorth.value = p.north || '';
    inputEast.value = p.east || '';
    inputSouth.value = p.south || '';
    inputWest.value = p.west || '';
    if (inputUnknownCards) {
      inputUnknownCards.value = p.unknown_cards || '';
    }

    updateDealSvg();
  }

  // Load Preset on Dropdown Selection
  if (presetSelect) {
    presetSelect.addEventListener('change', () => {
      loadPreset(presetSelect.value);
    });
  }

  // Load TXT / Deal File Handler
  const loadTxtBtn = document.getElementById('loadTxtBtn');
  const fileInputTxt = document.getElementById('fileInputTxt');

  if (loadTxtBtn && fileInputTxt) {
    loadTxtBtn.addEventListener('click', () => {
      fileInputTxt.value = '';
      fileInputTxt.click();
    });

    fileInputTxt.addEventListener('change', (e) => {
      const file = e.target.files[0];
      if (!file) return;

      const reader = new FileReader();
      reader.onload = (event) => {
        const text = event.target.result;
        parseAndApplyDealText(text);
      };
      reader.readAsText(file);
    });
  }

  function parseAndApplyDealText(text) {
    if (!text) return;
    const lines = text.split('\n');
    let hasKeys = false;
    const keyValues = {};
    const plainLines = [];

    lines.forEach(line => {
      let cleaned = line.trim();
      const hashPos = cleaned.indexOf('#');
      if (hashPos !== -1) cleaned = cleaned.substring(0, hashPos).trim();
      const slashPos = cleaned.indexOf('//');
      if (slashPos !== -1) cleaned = cleaned.substring(0, slashPos).trim();
      if (!cleaned) return;

      if (cleaned.includes('=') || cleaned.includes(':')) {
        const delim = cleaned.includes('=') ? '=' : ':';
        const parts = cleaned.split(delim);
        const k = parts[0].trim().toLowerCase();
        const v = parts.slice(1).join(delim).trim();
        keyValues[k] = v;
        hasKeys = true;
      } else {
        plainLines.push(cleaned);
      }
    });

    if (hasKeys) {
      if (keyValues.north !== undefined || keyValues.n !== undefined) inputNorth.value = keyValues.north || keyValues.n || '';
      if (keyValues.east !== undefined || keyValues.e !== undefined) inputEast.value = keyValues.east || keyValues.e || '';
      if (keyValues.south !== undefined || keyValues.s !== undefined) inputSouth.value = keyValues.south || keyValues.s || '';
      if (keyValues.west !== undefined || keyValues.w !== undefined) inputWest.value = keyValues.west || keyValues.w || '';

      const unknownVal = keyValues.unknown_cards || keyValues.unknown || keyValues.unknowns;
      if (unknownVal !== undefined && inputUnknownCards) {
        inputUnknownCards.value = unknownVal;
      }

      if (keyValues.trump || keyValues.trumps) {
        const tVal = (keyValues.trump || keyValues.trumps).toUpperCase();
        if (tVal.includes('SPADE') || tVal === 'S') trumpSelect.value = 'SPADES';
        else if (tVal.includes('HEART') || tVal === 'H') trumpSelect.value = 'HEARTS';
        else if (tVal.includes('DIAMOND') || tVal === 'D') trumpSelect.value = 'DIAMONDS';
        else if (tVal.includes('CLUB') || tVal === 'C') trumpSelect.value = 'CLUBS';
        else if (tVal.includes('NO') || tVal.includes('NT')) trumpSelect.value = 'NOTRUMP';
      }

      if (keyValues.leader || keyValues.lead) {
        const lVal = (keyValues.leader || keyValues.lead).toUpperCase();
        if (lVal.startsWith('N')) leaderSelect.value = 'NORTH';
        else if (lVal.startsWith('E')) leaderSelect.value = 'EAST';
        else if (lVal.startsWith('S')) leaderSelect.value = 'SOUTH';
        else if (lVal.startsWith('W')) leaderSelect.value = 'WEST';
      }

      if (keyValues.target || keyValues.target_tricks || keyValues.min_tricks) {
        const tgt = parseInt(keyValues.target || keyValues.target_tricks || keyValues.min_tricks, 10);
        if (!isNaN(tgt)) targetTricks.value = tgt;
      }

      if (keyValues.iters || keyValues.num_iters || keyValues.iterations) {
        const it = parseInt(keyValues.iters || keyValues.num_iters || keyValues.iterations, 10);
        if (!isNaN(it)) numIters.value = it;
      }
    } else if (plainLines.length >= 4) {
      inputNorth.value = plainLines[0];
      inputEast.value = plainLines[1];
      inputSouth.value = plainLines[2];
      inputWest.value = plainLines[3];
    }

    updateDealSvg();
  }

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

    if (inputUnknownCards && inputUnknownCards.value.trim()) {
      payload.unknown_cards = inputUnknownCards.value.trim();
    }

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

      currentSolveData = data;

      // Show summary
      resultsSummary.classList.remove('hidden');
      statTricks.textContent = data.tricks_won !== undefined ? data.tricks_won : '-';
      statLead.textContent = data.best_lead || '-';

      const total = data.total_boards || 1;
      const success = data.success_boards !== undefined ? data.success_boards : (data.covered_boards || 0);
      const failure = data.failure_boards !== undefined ? data.failure_boards : Math.max(0, total - success);
      const successPct = Math.round((success / total) * 100);
      const failurePct = Math.round((failure / total) * 100);

      if (statSuccess) statSuccess.textContent = `${success}/${total} (${successPct}%)`;
      if (statFailure) statFailure.textContent = `${failure}/${total} (${failurePct}%)`;

      // Render Decision Tree
      if (data.tree) {
        currentTreeData = data.tree;
        renderDecisionTree(data.tree, true);
      } else {
        treeRootGroup.innerHTML = `<text x="50%" y="50%" text-anchor="middle" fill="#f87171">No decision tree generated.</text>`;
      }

      // Update View Mode (Success vs Failure)
      updateViewMode(activeViewMode);

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

  // Expand / Collapse Controls
  const expandAllBtn = document.getElementById('expandAllBtn');
  const collapseAllBtn = document.getElementById('collapseAllBtn');

  function setAllCollapsed(node, isCollapsed) {
    if (node._depth !== undefined && node._depth > 0) {
      node._collapsed = isCollapsed;
    }
    if (node.children) {
      node.children.forEach(c => setAllCollapsed(c, isCollapsed));
    }
  }

  if (expandAllBtn) {
    expandAllBtn.addEventListener('click', () => {
      if (currentTreeData) {
        setAllCollapsed(currentTreeData, false);
        renderDecisionTree(currentTreeData, false);
      }
    });
  }

  if (collapseAllBtn) {
    collapseAllBtn.addEventListener('click', () => {
      if (currentTreeData) {
        setAllCollapsed(currentTreeData, true);
        renderDecisionTree(currentTreeData, false);
      }
    });
  }

  // Decision Tree SVG Rendering
  function renderDecisionTree(rootNode, resetView = false) {
    treeRootGroup.innerHTML = '';
    if (resetView) {
      panX = 40;
      panY = 40;
      zoomLevel = 1.0;
    }
    updateTransform();

    // Calculate layout coordinates recursively
    const nodeWidth = 220;
    const nodeHeight = 70;
    const levelHeight = 110;
    const siblingGap = 30;

    let leafCounter = 0;

    function layoutNode(node, depth = 0) {
      node._depth = depth;
      if (node._collapsed === undefined) {
        node._collapsed = (node.collapsed !== undefined ? node.collapsed : depth >= 1);
      }
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
      let detailText = '';
      if (node.success_boards !== undefined && node.total_boards !== undefined) {
        const pct = Math.round((node.success_boards / node.total_boards) * 100);
        detailText = `Success: ${node.success_boards}/${node.total_boards} (${pct}%)`;
      } else if (node.covered_boards !== undefined) {
        detailText = `Success: ${node.covered_boards}`;
      }
      detail.textContent = `${node.type.toUpperCase()} | ${detailText}`;
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
    let boardsInfo = '';
    if (node.success_boards !== undefined && node.total_boards !== undefined) {
      const sPct = Math.round((node.success_boards / node.total_boards) * 100);
      const fPct = Math.round((node.failure_boards / node.total_boards) * 100);
      boardsInfo = `Success: ${node.success_boards}/${node.total_boards} (${sPct}%) | Failure: ${node.failure_boards}/${node.total_boards} (${fPct}%)`;
    } else {
      boardsInfo = `Success: ${node.covered_boards || 0}`;
    }
    nodeDetailContent.textContent = `ID: ${node.id} | Type: ${node.type} | Label: "${node.label || ''}" | Trick: "${node.trick || ''}" | ${boardsInfo}`;
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

  function renderDistributionSummary(dist) {
    if (!dist) {
      clearDistributionSummary();
      return;
    }
    const rules = dist.invariant_rules || [];
    const bounds = dist.suit_bounds || {};
    const honors = dist.honor_stats || {};

    // Update Badge Counter
    if (ruleCounterBadge) {
      ruleCounterBadge.textContent = rules.length;
      if (rules.length > 0) {
        ruleCounterBadge.classList.remove('hidden');
      } else {
        ruleCounterBadge.classList.add('hidden');
      }
    }

    // Invariant Rules
    if (summaryRulesList) {
      summaryRulesList.innerHTML = '';
      if (rules.length > 0) {
        rules.forEach(r => {
          const li = document.createElement('li');
          let formattedRule = r.replace(/\bhand 1\b/gi, 'Hand 1 (East)')
                               .replace(/\bhand 3\b/gi, 'Hand 2 (West)')
                               .replace(/\bhand 2\b/gi, 'Hand 2 (West)');
          li.textContent = formattedRule;
          summaryRulesList.appendChild(li);
        });
      } else {
        const li = document.createElement('li');
        li.textContent = 'No invariant suit rules found.';
        li.style.color = '#94a3b8';
        li.style.borderLeftColor = '#64748b';
        summaryRulesList.appendChild(li);
      }
    }

    // Suit Bounds
    if (summaryBoundsTable) {
      const boundsTbody = summaryBoundsTable.querySelector('tbody');
      if (boundsTbody) {
        boundsTbody.innerHTML = '';
        const suitNames = { S: '♠ Spades', H: '♥ Hearts', D: '♦ Diamonds', C: '♣ Clubs' };
        ['S', 'H', 'D', 'C'].forEach(s => {
          if (bounds[s]) {
            const b = bounds[s];
            const tr = document.createElement('tr');
            tr.innerHTML = `
              <td><strong>${suitNames[s] || s}</strong></td>
              <td>${b.min_hand1} - ${b.max_hand1}</td>
              <td>${b.min_hand2} - ${b.max_hand2}</td>
              <td>${b.min_combined} - ${b.max_combined}</td>
            `;
            boundsTbody.appendChild(tr);
          }
        });
      }
    }

    // Honor Stats
    if (summaryHonorTable) {
      const honorTbody = summaryHonorTable.querySelector('tbody');
      if (honorTbody) {
        honorTbody.innerHTML = '';
        const suitNames = { S: '♠ Spades', H: '♥ Hearts', D: '♦ Diamonds', C: '♣ Clubs' };
        ['S', 'H', 'D', 'C'].forEach(s => {
          if (honors[s]) {
            Object.keys(honors[s]).forEach(rankKey => {
              const st = honors[s][rankKey];
              const total = dist.total_deals || 1;
              const h1Pct = ((st.hand1_count / total) * 100).toFixed(1) + '%';
              const h2Pct = ((st.hand2_count / total) * 100).toFixed(1) + '%';
              const bothPct = ((st.both_count / total) * 100).toFixed(1) + '%';
              const neitherPct = ((st.neither_count / total) * 100).toFixed(1) + '%';

              const tr = document.createElement('tr');
              tr.innerHTML = `
                <td>${suitNames[s] || s}</td>
                <td><strong>${rankKey}</strong></td>
                <td>${h1Pct}</td>
                <td>${h2Pct}</td>
                <td>${bothPct}</td>
                <td>${neitherPct}</td>
              `;
              honorTbody.appendChild(tr);
            });
          }
        });
      }
    }

    if (summaryEmptyNotice) summaryEmptyNotice.classList.add('hidden');
    if (summaryContent) summaryContent.classList.remove('hidden');
  }

  function clearDistributionSummary(msg = 'No distribution summary available for this deal. Run a fuzzy solve to view distribution statistics.') {
    if (ruleCounterBadge) {
      ruleCounterBadge.textContent = '0';
      ruleCounterBadge.classList.add('hidden');
    }
    if (summaryContent) summaryContent.classList.add('hidden');
    if (summaryEmptyNotice) {
      summaryEmptyNotice.textContent = msg;
      summaryEmptyNotice.classList.remove('hidden');
    }
  }

  // Initial UI Render
  if (presetSelect) {
    loadPreset(presetSelect.value);
  } else {
    updateDealSvg();
  }
});
)rawjs";

} // namespace web_assets
