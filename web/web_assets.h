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
          <option value="nine_card_hearts">9-Card Hearts Deal</option>
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
                      <label for="targetTricks">Tricks:</label>
                      <input type="text" inputmode="numeric" id="targetTricks" value="2" class="middle-number-input">
                    </div>
                    <div class="middle-control-group">
                      <label for="numIters">Iters:</label>
                      <input type="text" inputmode="numeric" id="numIters" value="10" class="middle-number-input">
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
            <button id="toggleInspectorBtn" class="btn btn-secondary-sm" title="Toggle Inspector Side Panel">Inspector ◧</button>
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
          <div class="tree-layout-wrapper">
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

            <!-- Vertical Tree Inspector Side Panel -->
            <aside id="treeInspectorPanel" class="tree-inspector-panel">
              <div class="inspector-header">
                <h3>Line Inspector</h3>
                <button id="closeInspectorBtn" class="inspector-close-btn" title="Collapse Side Panel">✕</button>
              </div>
              <div id="inspectorContent" class="inspector-content">
                <div class="inspector-placeholder">
                  Select a node in the decision tree to view trick-by-trick lines of play.
                </div>
              </div>
            </aside>
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

html, body {
  height: 100vh;
  min-height: 100vh;
}

body {
  font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
  background-color: var(--bg-dark);
  color: var(--text-main);
  line-height: 1.5;
  display: flex;
  flex-direction: column;
  overflow-x: hidden;
}

.app-header {
  background-color: #0b1120;
  border-bottom: 1px solid var(--panel-border);
  padding: 12px 24px;
  flex-shrink: 0;
}

.header-content {
  width: 100%;
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

.app-main {
  flex: 1;
  display: flex;
  flex-direction: column;
  min-height: 0;
  padding: 12px 20px 16px;
}

.grid-container {
  width: 100%;
  flex: 1;
  min-height: 0;
  display: grid;
  grid-template-columns: minmax(420px, 38%) 1fr;
  gap: 16px;
  align-items: stretch;
}

@media (max-width: 1024px) {
  html, body {
    height: auto;
  }

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
  min-width: 0;
  min-height: 0;
}

.deal-setup-panel {
  overflow-y: auto;
}

.decision-tree-panel {
  overflow: hidden;
}

.solve-action-container {
  margin-top: auto;
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
  flex: 1 1 auto;
  min-height: 200px;
  display: flex;
  align-items: center;
  justify-content: center;
  background-color: #1a382b;
  border-radius: 8px;
  overflow: hidden;
  box-shadow: 0 4px 12px rgba(0,0,0,0.3);
}

#dealSvg {
  width: 100%;
  height: 100%;
  display: block;
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
  font-size: 14px;
  font-weight: 800;
}

.card-suit-s, .card-suit-c { fill: #f8fafc; }
.card-suit-h, .card-suit-d { fill: #f87171; }

.card-values {
  fill: #f1f5f9;
  font-size: 14px;
  font-weight: 700;
  font-family: monospace;
  letter-spacing: 0.5px;
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

.middle-number-input::-webkit-outer-spin-button,
.middle-number-input::-webkit-inner-spin-button {
  -webkit-appearance: none;
  margin: 0;
}

.middle-number-input {
  -moz-appearance: textfield;
  appearance: textfield;
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
  font-size: 14px;
  font-weight: 600;
  font-family: monospace;
  letter-spacing: 0.3px;
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

/* Tree Viewport & Side Inspector Layout */
.tree-layout-wrapper {
  display: flex;
  flex-direction: row;
  gap: 12px;
  width: 100%;
  flex: 1;
  min-height: 320px;
  position: relative;
}

@media (max-width: 1024px) {
  .tree-layout-wrapper {
    height: 60vh;
    min-height: 360px;
  }
}

.tree-viewport {
  flex: 1;
  height: 100%;
  min-width: 0;
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

/* Vertical Side Panel Tree Inspector */
.tree-inspector-panel {
  width: clamp(240px, 22vw, 380px);
  max-width: 40%;
  height: 100%;
  flex-shrink: 0;
  background-color: #0b1120;
  border: 1px solid var(--panel-border);
  border-radius: 8px;
  display: flex;
  flex-direction: column;
  overflow: hidden;
  transition: all 0.2s ease;
  box-shadow: -2px 0 8px rgba(0, 0, 0, 0.2);
}

.tree-inspector-panel.collapsed {
  display: none !important;
}

.inspector-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 10px 12px;
  background-color: #0f172a;
  border-bottom: 1px solid var(--panel-border);
}

.inspector-header h3 {
  font-size: 13px;
  font-weight: 700;
  color: var(--primary);
  text-transform: uppercase;
  letter-spacing: 0.5px;
  margin: 0;
}

.inspector-close-btn {
  background: transparent;
  border: none;
  color: var(--text-muted);
  font-size: 14px;
  font-weight: bold;
  cursor: pointer;
  padding: 2px 6px;
  border-radius: 4px;
  transition: all 0.15s ease;
}

.inspector-close-btn:hover {
  color: #ffffff;
  background-color: #334155;
}

.inspector-content {
  padding: 12px;
  overflow-y: auto;
  flex: 1;
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.inspector-placeholder {
  color: var(--text-muted);
  font-size: 12px;
  font-style: italic;
  text-align: center;
  margin-top: 40px;
  padding: 0 12px;
}

.node-summary-card {
  background-color: #0f172a;
  border: 1px solid var(--panel-border);
  border-radius: 6px;
  padding: 10px;
}

.node-summary-title {
  font-size: 12px;
  font-weight: 700;
  color: #f8fafc;
  margin-bottom: 4px;
}

.node-summary-detail {
  font-size: 11px;
  color: var(--text-muted);
  font-family: monospace;
}

.trick-section-header {
  font-size: 11px;
  font-weight: 700;
  color: var(--text-muted);
  text-transform: uppercase;
  letter-spacing: 0.5px;
  margin-bottom: 6px;
  padding-bottom: 4px;
  border-bottom: 1px solid rgba(255, 255, 255, 0.05);
}

.trick-list {
  display: flex;
  flex-direction: column;
  gap: 6px;
}

.trick-row {
  background-color: #0f172a;
  border: 1px solid var(--panel-border);
  border-radius: 6px;
  padding: 4px 8px;
  display: flex;
  flex-direction: row;
  align-items: center;
  gap: 6px;
}

.trick-row-meta {
  display: flex;
  align-items: center;
  gap: 4px;
  flex-shrink: 0;
}

.trick-label {
  font-size: 11px;
  font-weight: 700;
  color: #94a3b8;
}

.seat-badge {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  padding: 2px 5px;
  border-radius: 4px;
  background-color: #334155;
  font-weight: 800;
  font-size: 11px;
  font-family: monospace;
}

.seat-badge-ns {
  color: #6ee7b7;
}

.seat-badge-ew {
  color: #fcd34d;
}

.card-chips-row {
  display: flex;
  align-items: center;
  gap: 4px;
  flex-wrap: wrap;
}

.card-chip {
  display: inline-flex;
  align-items: center;
  padding: 2px 5px;
  border-radius: 4px;
  font-family: monospace;
  font-size: 11px;
  font-weight: 600;
  background-color: #1e293b;
  border: 1px solid #334155;
  color: #f8fafc;
}

.card-chip.card-suit-h, .card-chip.card-suit-d {
  color: #f87171;
}

.card-chip.card-winner {
  font-weight: 800 !important;
}

.card-chip.card-winner.card-winner-ns {
  border: 2px solid #10b981 !important;
  box-shadow: 0 0 6px rgba(16, 185, 129, 0.4);
}

.card-chip.card-winner.card-winner-ew {
  border: 2px solid #f59e0b !important;
  box-shadow: 0 0 6px rgba(245, 158, 11, 0.4);
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
  pointer-events: none;
}

.node-text-detail {
  fill: #94a3b8;
  font-size: 11px;
  pointer-events: none;
}

.node-text-highlight {
  fill: #ffffff;
  font-size: 12px;
  font-weight: 700;
  pointer-events: none;
}

.tree-link {
  fill: none;
  stroke: #334155;
  stroke-width: 2;
  transition: stroke 0.2s ease;
}

.tree-edge-label-bg {
  fill: #0f172a;
  stroke: #475569;
  stroke-width: 1px;
}

.tree-edge-label {
  fill: #cbd5e1;
  font-size: 11px;
  font-weight: 600;
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
  flex: 1;
  min-height: 0;
  overflow-y: auto;
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
  const toggleInspectorBtn = document.getElementById('toggleInspectorBtn');
  const closeInspectorBtn = document.getElementById('closeInspectorBtn');
  const treeInspectorPanel = document.getElementById('treeInspectorPanel');
  const inspectorContent = document.getElementById('inspectorContent');
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

  // Inspector Side Panel Toggle Listeners
  if (toggleInspectorBtn && treeInspectorPanel) {
    toggleInspectorBtn.addEventListener('click', () => {
      treeInspectorPanel.classList.toggle('collapsed');
    });
  }

  if (closeInspectorBtn && treeInspectorPanel) {
    closeInspectorBtn.addEventListener('click', () => {
      treeInspectorPanel.classList.add('collapsed');
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

  // Default Presets Definition
  let PRESETS = {
    finesse: {
      name: "Simple Finesse (Spades)",
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
      name: "4-Card Endplay Deal",
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
      name: "Squeeze Deal (NT)",
      trump: 'NOTRUMP',
      leader: 'NORTH',
      target: 6,
      iters: 15,
      north: 'S A43; H -; D AKQ; C -;',
      east: '',
      south: 'S K2; H K; D 432; C -;',
      west: '',
      unknown_cards: 'S QJT987 H AQJT98'
    },
    nine_card_hearts: {
      name: "9-Card Hearts Deal",
      trump: 'HEARTS',
      leader: 'NORTH',
      target: 6,
      iters: 20,
      north: 'S 532; H T7; D T32; C 6;',
      east: '',
      south: 'S K9; H -; D A854; C AKJ;',
      west: '',
      unknown_cards: 'S AJTQ64 D KQJ976 C T43Q98'
    }
  };

  async function fetchPresets() {
    try {
      const res = await fetch('/api/presets');
      if (res.ok) {
        const loaded = await res.json();
        if (loaded && typeof loaded === 'object' && Object.keys(loaded).length > 0) {
          PRESETS = loaded;
          populatePresetSelect();
        }
      }
    } catch (e) {
      // Fall back to default embedded PRESETS
    }
  }

  function populatePresetSelect() {
    if (!presetSelect) return;
    const currentVal = presetSelect.value;
    presetSelect.innerHTML = '';
    for (const [key, p] of Object.entries(PRESETS)) {
      const opt = document.createElement('option');
      opt.value = key;
      opt.textContent = p.name || key;
      presetSelect.appendChild(opt);
    }
    if (PRESETS[currentVal]) {
      presetSelect.value = currentVal;
    } else if (presetSelect.options.length > 0) {
      presetSelect.value = presetSelect.options[0].value;
    }
    if (presetSelect.value) {
      loadPreset(presetSelect.value);
    }
  }

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

    let curSuit = null;
    let i = 0;
    while (i < str.length) {
      const c = str.charAt(i);
      const upperC = c.toUpperCase();

      if (suits.hasOwnProperty(upperC)) {
        curSuit = upperC;
      } else {
        if (!curSuit) {
          curSuit = 'S';
        }
        if (c === '1' && i + 1 < str.length && str.charAt(i + 1) === '0') {
          suits[curSuit] += '10';
          i += 2;
          continue;
        } else if (c !== ' ' && c !== ';' && c !== ',' && c !== '-') {
          suits[curSuit] += c;
        }
      }
      i++;
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
      textNode.setAttribute('y', yOffset + 13);
      
      const symbolSpan = document.createElementNS('http://www.w3.org/2000/svg', 'tspan');
      symbolSpan.setAttribute('class', `card-suit-label ${s.class}`);
      symbolSpan.textContent = s.symbol + ' ';

      const valSpan = document.createElementNS('http://www.w3.org/2000/svg', 'tspan');
      valSpan.setAttribute('class', 'card-values');
      valSpan.textContent = cards;

      textNode.appendChild(symbolSpan);
      textNode.appendChild(valSpan);
      group.appendChild(textNode);

      yOffset += 16.5;
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

    if (svgTrumpText && trumpSelect) {
      const trumpVal = trumpSelect.value;
      const suitSym = getSuitSymbol(trumpVal);
      const isRedTrump = (trumpVal === 'HEARTS' || trumpVal === 'DIAMONDS');
      svgTrumpText.innerHTML = 'Trump: <tspan class="' + (isRedTrump ? 'card-suit-h' : '') + '">' + suitSym + '</tspan>';
    }
    if (svgLeaderText && leaderSelect) {
      svgLeaderText.textContent = 'Leader: ' + leaderSelect.value.charAt(0);
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

  function clearPanels() {
    currentTreeData = null;
    currentSolveData = null;
    activeViewMode = 'success';

    if (execTime) execTime.textContent = '';

    if (resultsSummary) resultsSummary.classList.add('hidden');
    if (statTricks) statTricks.textContent = '-';
    if (statLead) statLead.textContent = '-';
    if (statSuccess) statSuccess.textContent = '-';
    if (statFailure) statFailure.textContent = '-';

    if (treeRootGroup) {
      treeRootGroup.innerHTML = `
        <text x="50%" y="50%" text-anchor="middle" fill="#6c757d" font-size="16">
          Click "Solve Deal" to render the interactive SVG decision tree.
        </text>`;
    }
    panX = 40;
    panY = 40;
    zoomLevel = 1.0;
    updateTransform();

    if (nodeDetailBanner) nodeDetailBanner.classList.add('hidden');
    if (nodeDetailContent) nodeDetailContent.textContent = '';
    if (inspectorContent) {
      inspectorContent.innerHTML = `
        <div class="inspector-placeholder">
          Select a node in the decision tree to view trick-by-trick lines of play.
        </div>`;
    }

    clearDistributionSummary('No distribution summary available for this deal. Run a fuzzy solve to view distribution statistics.');
    if (summaryModeHeader) summaryModeHeader.classList.add('hidden');
    if (summaryModeBadge) {
      summaryModeBadge.className = 'mode-badge mode-success';
      summaryModeBadge.textContent = 'Success Deals';
    }
    if (summaryModeSub) summaryModeSub.textContent = '';

    if (tabDecisionTreeBtn && tabSummaryBtn) {
      tabDecisionTreeBtn.classList.add('active');
      tabSummaryBtn.classList.remove('active');
      if (panelDecisionTree) panelDecisionTree.classList.remove('hidden');
      if (panelDistributionSummary) panelDistributionSummary.classList.add('hidden');
    }
  }

  function loadPreset(key) {
    const p = PRESETS[key];
    if (!p) return;

    clearPanels();
    setStatus('idle', 'Ready to solve. Configure hands and click "Solve Deal".');

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
    clearPanels();
    setStatus('idle', 'Ready to solve. Configure hands and click "Solve Deal".');
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
    clearPanels();
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
      min_tricks: isNaN(parseInt(targetTricks.value, 10)) ? 1 : parseInt(targetTricks.value, 10),
      num_iters: isNaN(parseInt(numIters.value, 10)) ? 10 : parseInt(numIters.value, 10)
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

      let timeStr = `${elapsed} ms`;
      if (typeof data.solve_ms === 'number' && typeof data.invariant_ms === 'number') {
        timeStr = `solve: ${data.solve_ms.toFixed(1)} ms, invariant: ${data.invariant_ms.toFixed(1)} ms`;
      }

      setStatus('success', 'Solve complete!', timeStr);
      execTime.textContent = timeStr;

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
        expandAndInspectNode(data.tree, data.tree);
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
    if (isCollapsed) {
      const isContainerRoot = (node._depth === 0 && (!node.trick || node.trick === '') && node.children && node.children.length > 1);
      node._collapsed = isContainerRoot ? false : true;
    } else {
      node._collapsed = false;
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

  function collapseSubtree(n) {
    if (!n) return;
    n._collapsed = true;
    if (n.children && n.children.length > 0) {
      n.children.forEach(c => collapseSubtree(c));
    }
  }

  function attachParentPointers(node, parent = null) {
    if (!node) return;
    node._parent = parent;
    if (node.children && node.children.length > 0) {
      node.children.forEach(c => attachParentPointers(c, node));
    }
  }

  function expandAndInspectNode(targetNode, rootNode) {
    if (!rootNode || !targetNode) return;
    attachParentPointers(rootNode);

    // Build path from rootNode to targetNode using _parent pointers
    const pathToTarget = [];
    let curr = targetNode;
    while (curr) {
      pathToTarget.unshift(curr);
      curr = curr._parent;
    }

    // Expand all nodes along pathToTarget and collapse non-path sibling branches
    for (let i = 0; i < pathToTarget.length; i++) {
      const current = pathToTarget[i];
      current._collapsed = false;
      if (current.children && current.children.length > 0) {
        const nextInPath = pathToTarget[i + 1];
        current.children.forEach(child => {
          if (child !== nextInPath) {
            collapseSubtree(child);
          }
        });
      }
    }

    // Expand down from targetNode to (one) leaf taking left branches (children[0])
    let leaf = targetNode;
    while (leaf.children && leaf.children.length > 0) {
      leaf._collapsed = false;
      const leftChild = leaf.children[0];
      for (let i = 1; i < leaf.children.length; i++) {
        collapseSubtree(leaf.children[i]);
      }
      leaf = leftChild;
    }
    leaf._collapsed = false;

    // Re-render decision tree layout
    renderDecisionTree(rootNode);

    // Show line inspector for the reached leaf
    showNodeInspector(leaf);
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

    function attachParentPointers(node, parent = null) {
      if (!node) return;
      node._parent = parent;
      if (node.children && node.children.length > 0) {
        node.children.forEach(c => attachParentPointers(c, node));
      }
    }
    attachParentPointers(rootNode);

    // Calculate layout coordinates recursively
    const nodeWidth = 240;
    const nodeHeight = 78;
    const levelHeight = 140;
    const siblingGap = 40;

    let leafCounter = 0;

    function layoutNode(node, depth = 0) {
      node._depth = depth;
      if (node._collapsed === undefined) {
        if (node.collapsed !== undefined) {
          node._collapsed = node.collapsed;
        } else {
          const isContainerRoot = (depth === 0 && (!node.trick || node.trick === '') && node.children && node.children.length > 1);
          node._collapsed = isContainerRoot ? false : true;
        }
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

    // Pass 1: Draw Links & Edge Badges
    function drawLinks(node) {
      if (node.children && node.children.length > 0 && !node._collapsed) {
        node.children.forEach(child => {
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

          if (child.condition) {
            const midX = (startX + endX) / 2;
            const labelGroup = document.createElementNS('http://www.w3.org/2000/svg', 'g');
            labelGroup.setAttribute('class', 'tree-edge-label-group');

            let condText = child.condition;
            if (condText.startsWith('after ')) {
              condText = condText.substring(6);
            }
            const labelWidth = Math.max(54, condText.length * 7 + 12);
            const labelHeight = 18;

            const rect = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
            rect.setAttribute('x', midX - labelWidth / 2);
            rect.setAttribute('y', midY - labelHeight / 2);
            rect.setAttribute('width', labelWidth);
            rect.setAttribute('height', labelHeight);
            rect.setAttribute('rx', '4');
            rect.setAttribute('class', 'tree-edge-label-bg');

            const text = document.createElementNS('http://www.w3.org/2000/svg', 'text');
            text.setAttribute('x', midX);
            text.setAttribute('y', midY + 4);
            text.setAttribute('text-anchor', 'middle');
            text.setAttribute('class', 'tree-edge-label');
            text.textContent = condText;

            labelGroup.appendChild(rect);
            labelGroup.appendChild(text);
            treeRootGroup.appendChild(labelGroup);
          }

          drawLinks(child);
        });
      }
    }

    // Pass 2: Draw Node Cards
    function drawNodes(node) {
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
      title.setAttribute('y', '22');
      title.setAttribute('class', 'node-text-title');
      let rawTitle = node.label || node.trick || 'Node';
      if (rawTitle.startsWith('after ')) {
        rawTitle = rawTitle.substring(6);
      }
      title.textContent = truncateText(rawTitle, 40);
      g.appendChild(title);

      // Detail Text
      const detail = document.createElementNS('http://www.w3.org/2000/svg', 'text');
      detail.setAttribute('x', '12');
      detail.setAttribute('y', '42');
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

      // 3rd Line: N/S Won on its own line for leaf/outcome nodes
      if (node.type === 'outcome' && node.ns_tricks !== undefined) {
        const highlight = document.createElementNS('http://www.w3.org/2000/svg', 'text');
        highlight.setAttribute('x', '12');
        highlight.setAttribute('y', '62');
        highlight.setAttribute('class', 'node-text-highlight');
        highlight.textContent = `N/S Won: ${node.ns_tricks}`;
        g.appendChild(highlight);
      }

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
        expandAndInspectNode(node, rootNode);
      });

      treeRootGroup.appendChild(g);

      if (node.children && node.children.length > 0 && !node._collapsed) {
        node.children.forEach(child => drawNodes(child));
      }
    }

    drawLinks(rootNode);
    drawNodes(rootNode);
  }

  function truncateText(str, maxLen) {
    if (!str) return '';
    return str.length > maxLen ? str.substring(0, maxLen - 3) + '...' : str;
  }

  function showNodeInspector(node) {
    if (treeInspectorPanel) {
      treeInspectorPanel.classList.remove('collapsed');
    }
    if (!inspectorContent) return;

    // Traverse parent pointers to assemble root-to-node path
    const path = [];
    let curr = node;
    while (curr) {
      path.unshift(curr);
      curr = curr._parent;
    }

    // Determine initial leader
    let initialLeader = getSeatIndex(leaderSelect ? leaderSelect.value : 'NORTH');
    if (currentSolveData && currentSolveData.leader) {
      initialLeader = getSeatIndex(currentSolveData.leader);
    }
    const trumpCode = getTrumpCode();

    let currentLeader = initialLeader;
    const trickRows = [];

    path.forEach((pathNode, stepIdx) => {
      const rawTrick = extractTrickString(pathNode);
      if (!rawTrick) return;

      const tokens = tokenizeTrick(rawTrick);
      if (tokens.length === 0) return;

      // Parse cards
      const parsedCards = [];
      let leadSuit = null;

      tokens.forEach((tok, i) => {
        const cardObj = parseSingleCardToken(tok, leadSuit, trumpCode);
        if (i === 0) {
          leadSuit = cardObj.suit;
        }
        cardObj.seatIdx = (currentLeader + i) % 4;
        cardObj.seatName = getSeatName(cardObj.seatIdx);
        parsedCards.push(cardObj);
      });

      // Check explicit winner from next node condition (if present) or pathNode label
      let explicitWinnerSeat = null;
      let explicitWinnerCard = null;

      // 1. Next node condition in path takes priority as it specifies the selected branch rule
      if (stepIdx + 1 < path.length) {
        const nextNode = path[stepIdx + 1];
        if (nextNode && nextNode.condition) {
          const cond = nextNode.condition;
          const mSeat = cond.match(/(?:when|if)\s+([0-3]|north|east|south|west|n|e|s|w)\b/i);
          if (mSeat) {
            explicitWinnerSeat = getSeatIndex(mSeat[1]);
          }
          const mCard = cond.match(/wins\s+(?:the\s+)?([A-Z0-9_,\(\)\s]+)/i);
          if (mCard) {
            let rawCardMatch = mCard[1].trim();
            let firstTok = rawCardMatch.split(/[\s,]+/)[0].trim();
            if (firstTok) {
              explicitWinnerCard = firstTok.toUpperCase();
            }
          }
        }
      }

      // 2. Fall back to pathNode.label if nextNode condition didn't specify a winner seat
      if (explicitWinnerSeat === null && pathNode.label) {
        const mWin = pathNode.label.match(/\((North|East|South|West|N|E|S|W)\s+wins/i);
        if (mWin) {
          explicitWinnerSeat = getSeatIndex(mWin[1]);
        }
      }

      // Determine winning card index and update card token if specific winner card is provided
      let winningCardIdx = -1;

      if (explicitWinnerSeat !== null) {
        const foundIdx = parsedCards.findIndex(c => c.seatIdx === explicitWinnerSeat);
        if (foundIdx !== -1) {
          winningCardIdx = foundIdx;
          if (explicitWinnerCard) {
            // Update the played card for this seat if specified in the branch condition
            const updatedCard = parseSingleCardToken(explicitWinnerCard, leadSuit, trumpCode);
            updatedCard.seatIdx = explicitWinnerSeat;
            updatedCard.seatName = getSeatName(explicitWinnerSeat);
            parsedCards[foundIdx] = updatedCard;
          }
        }
      }

      if (winningCardIdx === -1 && explicitWinnerCard !== null) {
        const foundIdx = parsedCards.findIndex(c => c.raw.toUpperCase().includes(explicitWinnerCard));
        if (foundIdx !== -1) {
          winningCardIdx = foundIdx;
        }
      }

      if (winningCardIdx === -1) {
        // Trump suit evaluation rules
        let bestIdx = 0;
        let bestIsTrump = (trumpCode !== 'NT' && parsedCards[0].suit === trumpCode);
        let bestVal = parsedCards[0].rankVal;

        for (let i = 1; i < parsedCards.length; i++) {
          const c = parsedCards[i];
          const isTrump = (trumpCode !== 'NT' && c.suit === trumpCode);

          if (isTrump && !bestIsTrump) {
            bestIdx = i;
            bestIsTrump = true;
            bestVal = c.rankVal;
          } else if (isTrump && bestIsTrump) {
            if (c.rankVal > bestVal) {
              bestIdx = i;
              bestVal = c.rankVal;
            }
          } else if (!isTrump && !bestIsTrump) {
            if (c.suit === leadSuit && c.rankVal > bestVal) {
              bestIdx = i;
              bestVal = c.rankVal;
            }
          }
        }
        winningCardIdx = bestIdx;
      }

      const winnerCard = parsedCards[winningCardIdx];
      const leadSeatName = getSeatName(currentLeader);
      const leadSeatBadge = `(${leadSeatName})`;

      trickRows.push({
        trickNum: trickRows.length + 1,
        leadSeatBadge: leadSeatBadge,
        leadSeatName: leadSeatName,
        cards: parsedCards,
        winningCardIdx: winningCardIdx,
        winnerSeat: winnerCard ? winnerCard.seatName : ''
      });

      // Next trick leader is the winner of this trick
      currentLeader = winnerCard.seatIdx;
    });

    // Render Inspector Panel HTML
    inspectorContent.innerHTML = '';

    // Node Summary Card
    const summaryCard = document.createElement('div');
    summaryCard.className = 'node-summary-card';

    let successInfo = '';
    if (node.success_boards !== undefined && node.total_boards !== undefined) {
      const sPct = Math.round((node.success_boards / node.total_boards) * 100);
      successInfo = `${node.success_boards}/${node.total_boards} (${sPct}%)`;
    } else if (node.covered_boards !== undefined) {
      successInfo = `${node.covered_boards}`;
    } else {
      successInfo = '-';
    }

    const nsTricksInfo = node.ns_tricks !== undefined ? ` | N/S Won: ${node.ns_tricks}` : '';

    summaryCard.innerHTML = `
      <div class="node-summary-title">Node #${node.id} (${(node.type || 'node').toUpperCase()})</div>
      <div class="node-summary-detail">Label: "${truncateText(node.label || node.trick || 'Root', 35)}"</div>
      <div class="node-summary-detail">Success: ${successInfo}${nsTricksInfo}</div>
    `;
    inspectorContent.appendChild(summaryCard);

    // Trick Section
    const trickSection = document.createElement('div');
    trickSection.innerHTML = `<div class="trick-section-header">Line of Play (${trickRows.length} Trick${trickRows.length !== 1 ? 's' : ''})</div>`;

    if (trickRows.length === 0) {
      const emptyNote = document.createElement('div');
      emptyNote.className = 'inspector-placeholder';
      emptyNote.textContent = 'Root node selected. Click a decision node in the tree below to inspect played tricks.';
      trickSection.appendChild(emptyNote);
    } else {
      const listContainer = document.createElement('div');
      listContainer.className = 'trick-list';

      trickRows.forEach(tr => {
        const rowEl = document.createElement('div');
        rowEl.className = 'trick-row';

        const metaEl = document.createElement('div');
        metaEl.className = 'trick-row-meta';

        const labelEl = document.createElement('span');
        labelEl.className = 'trick-label';
        labelEl.textContent = `#${tr.trickNum}`;

        const isLeaderEW = (tr.leadSeatName === 'E' || tr.leadSeatName === 'W');
        const badgeEl = document.createElement('span');
        badgeEl.className = `seat-badge ${isLeaderEW ? 'seat-badge-ew' : 'seat-badge-ns'}`;
        badgeEl.textContent = tr.leadSeatBadge;

        metaEl.appendChild(labelEl);
        metaEl.appendChild(badgeEl);
        rowEl.appendChild(metaEl);

        const chipsRow = document.createElement('div');
        chipsRow.className = 'card-chips-row';

        const isWinnerEW = (tr.winnerSeat === 'E' || tr.winnerSeat === 'W');
        const winnerThemeClass = isWinnerEW ? 'card-winner-ew' : 'card-winner-ns';

        tr.cards.forEach((c, idx) => {
          const chip = document.createElement('span');
          const isWinner = (idx === tr.winningCardIdx);
          chip.className = `card-chip ${c.suitClass} ${isWinner ? `card-winner ${winnerThemeClass}` : ''}`;
          chip.title = `Played by ${c.seatName}${isWinner ? ' (Winner)' : ''}`;
          chip.textContent = c.displayStr;
          chipsRow.appendChild(chip);
        });

        rowEl.appendChild(chipsRow);
        listContainer.appendChild(rowEl);
      });

      trickSection.appendChild(listContainer);
    }

    inspectorContent.appendChild(trickSection);
  }

  if (typeof window !== 'undefined') {
    window._getCurrentTreeData = () => currentTreeData;
    window._showNodeInspector = showNodeInspector;
    window._expandAndInspectNode = expandAndInspectNode;
  }

  function getSeatIndex(seatStr) {
    if (!seatStr) return 0;
    const s = seatStr.trim().toUpperCase();
    if (s.startsWith('N') || s === '0') return 0; // NORTH
    if (s.startsWith('E') || s === '1' || s === '3') {
      if (s === '1') return 1; // EAST in C++ enum (NORTH=0, EAST=1, SOUTH=2, WEST=3)
      if (s === '3') return 3; // WEST
      if (s.startsWith('E')) return 1;
    }
    if (s.startsWith('S') || s === '2') return 2; // SOUTH
    if (s.startsWith('W')) return 3; // WEST
    return 0;
  }

  function getSeatName(seatIdx) {
    const seats = ['N', 'E', 'S', 'W'];
    return seats[((seatIdx % 4) + 4) % 4];
  }

  function getTrumpCode() {
    if (!trumpSelect) return 'S';
    const val = trumpSelect.value;
    if (val === 'SPADES') return 'S';
    if (val === 'HEARTS') return 'H';
    if (val === 'DIAMONDS') return 'D';
    if (val === 'CLUBS') return 'C';
    if (val === 'NOTRUMP') return 'NT';
    return 'S';
  }

  function tokenizeTrick(trickStr) {
    if (!trickStr) return [];
    const cleaned = trickStr.replace(/,/g, ' ');
    const regex = /\([^\)]+\)|[S|H|D|C][A|K|Q|J|T|10|9|8|7|6|5|4|3|2|x|_]+|[A|K|Q|J|T|10|9|8|7|6|5|4|3|2]+[S|H|D|C]|\S+/gi;
    const matches = cleaned.match(regex) || [];
    return matches.filter(m => m.trim().length > 0);
  }

  function parseSingleCardToken(token, leadSuit, trumpSuit) {
    let cleanToken = token.replace(/[\(\)]/g, '').trim().toUpperCase();
    let suit = null;
    let rankStr = '';

    if (cleanToken === 'R_' || cleanToken === 'R') {
      suit = trumpSuit !== 'NT' ? trumpSuit : (leadSuit || 'S');
      rankStr = '_';
    } else if (cleanToken === '__' || cleanToken === '_') {
      suit = leadSuit || 'S';
      rankStr = '_';
    } else {
      if (['S', 'H', 'D', 'C'].includes(cleanToken.charAt(0))) {
        suit = cleanToken.charAt(0);
        rankStr = cleanToken.substring(1);
      } else if (['S', 'H', 'D', 'C'].includes(cleanToken.charAt(cleanToken.length - 1))) {
        suit = cleanToken.charAt(cleanToken.length - 1);
        rankStr = cleanToken.substring(0, cleanToken.length - 1);
      } else {
        suit = leadSuit || 'S';
        rankStr = cleanToken;
      }
    }

    if (rankStr === '10') rankStr = 'T';
    if (rankStr === '') rankStr = '_';

    const rankValueMap = {
      'A': 14, 'K': 13, 'Q': 12, 'J': 11, 'T': 10, '10': 10,
      '9': 9, '8': 8, '7': 7, '6': 6, '5': 5, '4': 4, '3': 3, '2': 2
    };
    const rankVal = rankValueMap[rankStr] || 1;

    let suitSym = '♠';
    let suitClass = 'card-suit-s';
    if (suit === 'H') { suitSym = '♥'; suitClass = 'card-suit-h'; }
    else if (suit === 'D') { suitSym = '♦'; suitClass = 'card-suit-d'; }
    else if (suit === 'C') { suitSym = '♣'; suitClass = 'card-suit-c'; }

    const displayStr = `${suitSym}${rankStr}`;

    return {
      raw: token,
      suit: suit,
      rankStr: rankStr,
      rankVal: rankVal,
      suitSym: suitSym,
      suitClass: suitClass,
      displayStr: displayStr
    };
  }

  function extractTrickString(node) {
    if (!node) return null;

    // Check if node.label explicitly matches "Trick <N>:"
    if (node.label && /^Trick\s+\d+:/i.test(node.label.trim())) {
      let lbl = node.label.trim();
      const colonIdx = lbl.indexOf(':');
      if (colonIdx !== -1) {
        let cardsPart = lbl.substring(colonIdx + 1).trim();
        // Remove trailing parenthesized outcome/winner metadata
        cardsPart = cardsPart.replace(/\s*\([^)]*(?:wins|tricks|won|succeeded|failed)[^)]*\)\s*/gi, '').trim();
        if (cardsPart.length > 0) {
          return cardsPart;
        }
      }
    }

    // Fall back to node.trick if present and valid
    if (node.trick && typeof node.trick === 'string') {
      const trimmed = node.trick.trim();
      if (trimmed.length > 0 &&
          !/^solve/i.test(trimmed) &&
          !/^optimal/i.test(trimmed) &&
          !/is led/i.test(trimmed) &&
          !/variants/i.test(trimmed) &&
          !/hand /i.test(trimmed)) {
        return trimmed;
      }
    }

    return null;
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

  // Initial UI Render & Fetch Presets
  fetchPresets().then(() => {
    if (presetSelect && presetSelect.value) {
      loadPreset(presetSelect.value);
    } else {
      updateDealSvg();
    }
  });
});

)rawjs";

inline const char* PRESETS_JSON = R"rawjson(
{
  "finesse": {
    "name": "Simple Finesse (Spades)",
    "trump": "SPADES",
    "leader": "SOUTH",
    "target": 2,
    "iters": 10,
    "north": "S AQ; H -; D -; C -;",
    "east": "",
    "south": "S 87; H -; D -; C -;",
    "west": "",
    "unknown_cards": "S Kxxx"
  },
  "endplay": {
    "name": "4-Card Endplay Deal",
    "trump": "HEARTS",
    "leader": "SOUTH",
    "target": 3,
    "iters": 30,
    "north": "S AQT; H 2; D -; C -;",
    "east": "",
    "south": "S 876; H A; D -; C -;",
    "west": "",
    "unknown_cards": "S KJ9543 D 34"
  },
  "squeeze": {
    "name": "Squeeze Deal (NT)",
    "trump": "NOTRUMP",
    "leader": "NORTH",
    "target": 6,
    "iters": 15,
    "north": "S A43; H -; D AKQ; C -;",
    "east": "",
    "south": "S K2; H K; D 432; C -;",
    "west": "",
    "unknown_cards": "S QJT987 H AQJT98"
  },
  "nine_card_hearts": {
    "name": "9-Card Hearts Deal",
    "trump": "HEARTS",
    "leader": "NORTH",
    "target": 6,
    "iters": 20,
    "north": "S 532; H T7; D T32; C 6;",
    "east": "",
    "south": "S K9; H -; D A854; C AKJ;",
    "west": "",
    "unknown_cards": "S AJTQ64 D KQJ976 C T43Q98"
  }
}

)rawjson";

} // namespace web_assets
