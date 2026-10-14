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
          <option value="safety_play">Safety Play Deal (NT)</option>
          <option value="twelve_card_spades">12-Card Spades Deal (4S)</option>
          <option value="restricted_choice">Restricted Choice (7S)</option>
          <option value="mollo_6s">Mollo 6S</option>
          <option value="two_way_finesse">2-Way Finesse</option>
        </select>
      </div>
    </div>
  </header>

  <main class="app-main">
    <div class="grid-container" id="gridContainer">
      <!-- Left Column: Deal Setup & SVG Deal Diagram -->
      <section class="card-panel deal-setup-panel" id="dealSetupPanel">
        <div class="panel-header">
          <h2>Deal Configuration</h2>
          <span class="panel-badge" id="dealEndingBadge">Full deal</span>
        </div>

        <!-- SVG Deal Diagram -->
        <div class="svg-deal-container">
          <svg id="dealSvg" viewBox="0 0 384 364" xmlns="http://www.w3.org/2000/svg">
            <rect width="100%" height="100%" fill="#1a382b" rx="10"/>

            <!-- Upper Left Compass Rose -->
            <g id="svgCompassRose" transform="translate(66, 64)">
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
            <g class="hand-group" id="svgUnknownHand" transform="translate(267, 12)">
              <rect width="102" height="104" rx="8" class="unknown-box" id="boxUnknown"/>
              <text x="51" y="15" class="unknown-title" text-anchor="middle">UNKNOWN CARDS</text>
              <g transform="translate(10, 20)" id="cardsUnknownGroup"></g>
            </g>

            <!-- North Hand -->
            <g class="hand-group" id="svgNorthHand" transform="translate(129, 12)">
              <rect width="126" height="104" rx="8" class="hand-box" id="boxNorth"/>
              <g transform="translate(10, 12)" id="cardsNorthGroup"></g>
            </g>

            <!-- West Hand -->
            <g class="hand-group" id="svgWestHand" transform="translate(15, 130)">
              <rect width="102" height="104" rx="8" class="hand-box" id="boxWest"/>
              <g transform="translate(10, 12)" id="cardsWestGroup"></g>
            </g>

            <!-- Center Box (Middle Controls: Trump / Leader / Target / Iters) -->
            <g id="svgCenterBox" transform="translate(129, 130)">
              <rect width="126" height="104" rx="8" fill="#0f241a" stroke="#2d5e46" stroke-width="1.5"/>
              <foreignObject x="0" y="0" width="126" height="104">
                <div class="middle-controls" xmlns="http://www.w3.org/1999/xhtml">
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
                  <div class="middle-control-group">
                    <label for="targetTricks">Tricks:</label>
                    <input type="text" inputmode="numeric" id="targetTricks" value="2" class="middle-number-input">
                  </div>
                  <div class="middle-control-group">
                    <label for="numIters">Iters:</label>
                    <input type="text" inputmode="numeric" id="numIters" value="10" class="middle-number-input">
                  </div>
                </div>
              </foreignObject>
            </g>

            <!-- East Hand -->
            <g class="hand-group" id="svgEastHand" transform="translate(267, 130)">
              <rect width="102" height="104" rx="8" class="hand-box" id="boxEast"/>
              <g transform="translate(10, 12)" id="cardsEastGroup"></g>
            </g>

            <!-- South Hand -->
            <g class="hand-group" id="svgSouthHand" transform="translate(129, 248)">
              <rect width="126" height="104" rx="8" class="hand-box" id="boxSouth"/>
              <g transform="translate(10, 12)" id="cardsSouthGroup"></g>
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

        <!-- Propose Line Controls -->
        <div id="proposeLineBar" class="propose-line-bar hidden">
          <div class="propose-line-header">
            <span id="proposeLineTitle" class="propose-line-title">Propose Line</span>
            <span id="proposeLineTurnBadge" class="propose-turn-badge"></span>
          </div>
          <div id="proposeLineCards" class="propose-line-cards"></div>
          <div class="propose-line-actions">
            <button id="computeNewLineBtn" type="button" class="btn btn-primary btn-sm" disabled>
              Compute new line
            </button>
            <button id="undoProposedCardBtn" type="button" class="btn btn-secondary-sm" disabled title="Undo last proposed card">
              Undo
            </button>
            <button id="resetLineBtn" type="button" class="btn btn-secondary-sm" disabled title="Reset back to original line">
              Reset
            </button>
          </div>
        </div>

        <!-- Solve Action Button -->
        <div class="solve-action-container">
          <button id="solveBtn" class="btn btn-primary btn-large">
            <span class="btn-icon">⚡</span> Solve Deal
          </button>
          <div class="threads-selector" title="Number of solver threads">
            <label for="numThreads">Threads:</label>
            <select id="numThreads" class="select-input">
              <option value="1">1</option>
              <option value="2">2</option>
              <option value="4">4</option>
              <option value="8">8</option>
              <option value="16" selected>16</option>
            </select>
          </div>
        </div>
      </section>

      <!-- Vertical Resizer Bar between Deal Configuration and Decision Tree Panel -->
      <div class="panel-resizer" id="panelResizer" role="separator" aria-orientation="vertical" aria-label="Resize deal configuration and decision tree panels" tabindex="0" title="Drag to resize panels">
        <div class="resizer-handle"></div>
      </div>

      <!-- Right Column: Results & Decision Tree -->
      <section class="card-panel decision-tree-panel" id="decisionTreePanel">
        <div class="panel-header">
          <div class="tab-buttons">
            <button id="tabDecisionTreeBtn" class="tab-btn active" type="button">Decision Tree</button>
            <button id="tabSummaryBtn" class="tab-btn" type="button">Distribution Summary <span id="ruleCounterBadge" class="badge-counter hidden">0</span></button>
            <button id="tabSampleHandsBtn" class="tab-btn" type="button">Sample Hands <span id="sampleHandsCounterBadge" class="badge-counter hidden">0</span></button>
            <button id="tabCompareLinesBtn" class="tab-btn" type="button">Compare Lines <span id="compareLinesCounterBadge" class="badge-counter hidden">0</span></button>
          </div>
          <div class="zoom-controls">
            <div class="tree-legend" title="Blue: a trick to play. Green: the contract is made. Red: the defense beats this line.">
              <span><span class="legend-swatch legend-play"></span>Play</span>
              <span><span class="legend-swatch legend-makes"></span>Makes</span>
              <span><span class="legend-swatch legend-fails"></span>Fails</span>
            </div>
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
          <div id="statusProgress" class="status-progress hidden" role="progressbar"
               aria-valuemin="0" aria-valuemax="100" aria-valuenow="0">
            <div id="statusProgressFill" class="status-progress-fill"></div>
          </div>
          <span id="execTime" class="exec-time"></span>
        </div>

        <!-- Result Stats Cards -->
        <div id="resultsSummary" class="results-summary hidden">
          <div class="stat-card">
            <span class="stat-label">Tricks Won</span>
            <span id="statTricks" class="stat-value">-</span>
          </div>
          <div class="stat-card">
            <span id="statLeadLabel" class="stat-label">Optimal Lead</span>
            <span id="statLead" class="stat-value">-</span>
          </div>
          <div id="cardStatSuccess" class="stat-card stat-card-toggle active" role="button" tabindex="0" title="Click to view Success deals in the Distribution Summary and Sample Hands tabs">
            <span class="stat-label">Success Rate</span>
            <span id="statSuccess" class="stat-value">-</span>
          </div>
          <div id="cardStatFailure" class="stat-card stat-card-toggle" role="button" tabindex="0" title="Click to view Failure deals in the Distribution Summary and Sample Hands tabs">
            <span class="stat-label">Failure Rate</span>
            <span id="statFailure" class="stat-value">-</span>
          </div>
        </div>

        <!-- Equivalent / Alternative Lines Selector Bar -->
        <div id="equivalentLinesBar" class="equivalent-lines-bar hidden">
          <div class="equivalent-lines-info">
            <span id="equivalentLinesBadge" class="equivalent-lines-badge">Multiple Equivalent Lines</span>
            <span id="equivalentLinesHint" class="equivalent-lines-hint">Choose an equally optimal line of play:</span>
          </div>
          <div class="equivalent-lines-controls">
            <button id="prevLineBtn" type="button" class="btn btn-secondary-sm line-nav-btn" title="Previous line">◀</button>
            <select id="equivalentLineSelect" class="equivalent-line-select" aria-label="Select line of play"></select>
            <button id="nextLineBtn" type="button" class="btn btn-secondary-sm line-nav-btn" title="Next line">▶</button>
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
                  <marker id="arrow-failure" viewBox="0 0 10 10" refX="6" refY="5" markerWidth="6" markerHeight="6" orient="auto-start-reverse">
                    <path d="M 0 0 L 10 5 L 0 10 z" fill="#b91c1c"/>
                  </marker>
                  <marker id="arrow-traced" viewBox="0 0 10 10" refX="6" refY="5" markerWidth="6" markerHeight="6" orient="auto-start-reverse">
                    <path d="M 0 0 L 10 5 L 0 10 z" fill="#22d3ee"/>
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
              <div class="inspector-trace-bar" id="inspectorTraceBar">
                <label for="traceHandInput" class="trace-hand-label">Hand #</label>
                <input type="text" inputmode="numeric" id="traceHandInput" class="trace-hand-input" placeholder="1..N" title="Enter a sampled hand number (1..N) to run the line of play on that hand">
                <button id="traceHandBtn" type="button" class="btn btn-secondary-sm trace-hand-btn">Trace</button>
                <button id="clearTraceHandBtn" type="button" class="btn btn-secondary-sm trace-hand-clear-btn hidden" title="Clear traced hand">Clear</button>
              </div>
              <div id="traceHandError" class="trace-hand-error hidden"></div>
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

        <!-- Tab Panel 3: Sample Hands -->
        <div id="panelSampleHands" class="tab-panel hidden">
          <div id="sampleHandsModeHeader" class="summary-mode-header hidden">
            <span id="sampleHandsModeBadge" class="mode-badge mode-success">Success Deals</span>
            <span id="sampleHandsModeSub" class="mode-sub"></span>
          </div>
          <div id="sampleHandsEmptyNotice" class="summary-empty">
            No sample hands available for this deal. Click "Solve Deal" to view sample hands.
          </div>
          <div id="sampleHandsGrid" class="sample-hands-grid hidden"></div>
        </div>

        <!-- Tab Panel 4: Compare Lines -->
        <div id="panelCompareLines" class="tab-panel hidden">
          <div id="compareLinesEmptyNotice" class="summary-empty">
            No lines computed yet. Click "Solve Deal" (and optionally compute alternate lines) to compare lines of play.
          </div>
          <div id="compareLinesContent" class="compare-lines-content hidden">
            <div class="summary-section">
              <div class="compare-lines-header-row">
                <h4>Computed Lines (select 2 or more to compare)</h4>
                <span id="compareLinesHint" class="compare-lines-hint"></span>
              </div>
              <div id="compareLinesList" class="compare-lines-list"></div>
            </div>
            <div id="compareLinesResults" class="compare-lines-results"></div>
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
  display: flex;
  flex-direction: row;
  align-items: stretch;
  gap: 0;
  position: relative;
  --left-panel-width: 38%;
}

@media (max-width: 1024px) {
  html, body {
    height: auto;
  }

  .grid-container {
    flex-direction: column;
    gap: 16px;
  }

  .deal-setup-panel {
    width: 100% !important;
    max-width: none !important;
    min-width: 0 !important;
  }

  .panel-resizer {
    display: none !important;
  }

  .decision-tree-panel {
    width: 100% !important;
    min-width: 0 !important;
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
  width: var(--left-panel-width, 38%);
  flex: 0 0 auto;
  min-width: 280px;
  max-width: calc(100% - 300px);
  overflow-y: auto;
}

.panel-resizer {
  width: 14px;
  margin: 0 1px;
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: col-resize;
  user-select: none;
  -webkit-user-select: none;
  flex-shrink: 0;
  z-index: 10;
  position: relative;
  touch-action: none;
}

.resizer-handle {
  width: 4px;
  height: 36px;
  background-color: var(--panel-border);
  border-radius: 2px;
  transition: background-color 0.15s ease, height 0.15s ease, width 0.15s ease, box-shadow 0.15s ease;
}

.panel-resizer:hover .resizer-handle,
.panel-resizer.resizing .resizer-handle {
  background-color: var(--primary);
  height: 56px;
  width: 5px;
  box-shadow: 0 0 8px rgba(59, 130, 246, 0.5);
}

.panel-resizer:focus-visible {
  outline: 2px solid var(--primary);
  outline-offset: -2px;
  border-radius: 4px;
}

.decision-tree-panel {
  flex: 1 1 0%;
  min-width: 300px;
  overflow: hidden;
}

body.is-resizing {
  cursor: col-resize !important;
  user-select: none !important;
  -webkit-user-select: none !important;
}

body.is-resizing * {
  cursor: col-resize !important;
}

.solve-action-container {
  margin-top: auto;
  display: flex;
  align-items: center;
  gap: 10px;
}

.solve-action-container .btn-large {
  flex: 1;
}

.threads-selector {
  display: flex;
  align-items: center;
  gap: 6px;
  font-size: 13px;
  font-weight: 600;
  color: var(--text-muted);
  flex-shrink: 0;
}

.threads-selector .select-input {
  padding: 9px 10px;
  cursor: pointer;
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
  font-size: 9px;
  font-weight: 700;
  letter-spacing: 0.3px;
}

.compass-trump, .compass-leader {
  fill: #f8fafc;
  font-size: 12px;
  font-weight: 600;
}

.card-suit-label {
  font-size: 13px;
  font-weight: 800;
}

.card-suit-s { fill: #f8fafc; color: #f8fafc; }
.card-suit-h { fill: #f87171; color: #f87171; }
.card-suit-d { fill: #fb923c; color: #fb923c; }
.card-suit-c { fill: #34d399; color: #34d399; }

.card-values {
  fill: #f1f5f9;
  font-size: 13px;
  font-weight: 700;
  font-family: monospace;
  letter-spacing: 0.5px;
}

.card-rank-played {
  text-decoration: line-through;
}

.card-rank-current {
  fill: #fbbf24;
  font-weight: 900;
  text-decoration: underline;
  filter: drop-shadow(0 0 4px rgba(251, 191, 36, 0.85));
}

.card-lead-box {
  fill: rgba(251, 191, 36, 0.18);
  stroke: #fbbf24;
  stroke-width: 1.5;
  rx: 3px;
  ry: 3px;
}

/* Form Controls */
.middle-controls {
  width: 100%;
  height: 100%;
  display: flex;
  flex-direction: column;
  justify-content: center;
  align-items: stretch;
  gap: 4px;
  padding: 6px 8px;
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
  justify-content: space-between;
  align-items: center;
  width: 100%;
  gap: 4px;
  font-size: 11px;
  font-weight: 600;
  color: #a7f3d0;
}

.middle-select, .middle-number-input {
  background-color: #0b1b13;
  color: #f8fafc;
  border: 1px solid #2d5e46;
  border-radius: 4px;
  padding: 2px 4px;
  font-size: 11px;
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
  width: 44px;
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

.status-progress {
  flex: 0 0 160px;
  height: 8px;
  border-radius: 4px;
  background-color: rgba(255, 255, 255, 0.12);
  overflow: hidden;
}

.status-progress-fill {
  height: 100%;
  width: 0;
  border-radius: 4px;
  background-color: currentColor;
  transition: width 0.3s ease;
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

.equivalent-lines-bar {
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-wrap: wrap;
  gap: 10px;
  padding: 8px 12px;
  background-color: #0f172a;
  border: 1px solid rgba(56, 189, 248, 0.35);
  border-radius: 8px;
}

.equivalent-lines-info {
  display: flex;
  align-items: center;
  gap: 8px;
  flex-wrap: wrap;
}

.equivalent-lines-badge {
  padding: 3px 9px;
  border-radius: 12px;
  font-size: 11px;
  font-weight: 700;
  text-transform: uppercase;
  letter-spacing: 0.4px;
  background-color: rgba(56, 189, 248, 0.18);
  color: #7dd3fc;
  border: 1px solid rgba(56, 189, 248, 0.45);
}

.equivalent-lines-hint {
  font-size: 12px;
  color: var(--text-muted);
}

.equivalent-lines-controls {
  display: flex;
  align-items: center;
  gap: 6px;
  flex: 1 1 260px;
  justify-content: flex-end;
}

.equivalent-line-select {
  flex: 1 1 auto;
  max-width: 480px;
  min-width: 180px;
  background-color: #1e293b;
  color: var(--text-main);
  border: 1px solid var(--panel-border);
  border-radius: 6px;
  padding: 5px 10px;
  font-size: 12.5px;
  font-family: monospace;
  cursor: pointer;
}

.equivalent-line-select:focus {
  outline: none;
  border-color: #38bdf8;
}

.line-nav-btn {
  padding: 4px 9px;
  font-size: 11px;
  line-height: 1.2;
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
  cursor: pointer;
  transition: all 0.15s ease;
}

.trick-row:hover {
  border-color: #60a5fa;
  background-color: #162238;
}

.trick-row.trick-row-selected {
  border-color: #fbbf24;
  background-color: #1e293b;
  box-shadow: 0 0 8px rgba(251, 191, 36, 0.35);
}

.trick-row.trick-row-past {
  opacity: 0.6;
}

.trick-columns-header {
  display: flex;
  flex-direction: row;
  align-items: center;
  gap: 6px;
  padding: 0 8px 2px 8px;
}

.trick-seat-col-header {
  text-align: center;
  font-family: monospace;
  font-size: 11px;
  font-weight: 800;
  letter-spacing: 0.04em;
  user-select: none;
}

.trick-seat-col-header.seat-col-ns {
  color: #38bdf8;
}

.trick-seat-col-header.seat-col-ew {
  color: #c084fc;
}

.trick-row-meta {
  display: flex;
  align-items: flex-start;
  align-self: stretch;
  gap: 4px;
  flex-shrink: 0;
  min-width: 30px;
}

.trick-label {
  font-size: 11px;
  font-weight: 700;
  color: #94a3b8;
}

.trick-label.clickable {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  background-color: #000000;
  border: 1px solid #3b82f6;
  color: #60a5fa;
  padding: 3px 6px;
  line-height: 15px;
  border-radius: 4px;
  margin-bottom: 6px;
  transition: all 0.15s ease;
  user-select: none;
}

.trick-label.clickable:hover {
  background-color: #2563eb;
  color: #ffffff;
  border-color: #60a5fa;
  box-shadow: 0 0 6px rgba(59, 130, 246, 0.5);
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
  color: #38bdf8;
}

.seat-badge-ew {
  color: #c084fc;
}

.card-chips-row {
  display: grid;
  grid-template-columns: repeat(4, minmax(0, 1fr));
  align-items: end;
  gap: 4px;
  flex: 1;
}

.card-chip {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  padding: 3px 6px;
  border-radius: 4px;
  font-family: monospace;
  font-size: 12px;
  font-weight: 600;
  background-color: #1e293b;
  border: 1px solid #334155;
  color: #f8fafc;
  margin-top: 6px;
}

.card-chip.card-lead {
  align-self: start;
  margin-top: 0;
  margin-bottom: 6px;
  background-color: #000000;
}

.card-chip-content {
  display: inline-flex;
  align-items: center;
  line-height: 1.1;
}

.card-chip .suit-symbol {
  font-size: 15px;
  line-height: 1;
  margin-right: 2px;
  display: inline-block;
  font-weight: bold;
}

.card-chip .card-rank {
  color: #f8fafc;
}

.card-chip.card-suit-s .suit-symbol { color: #f8fafc; }
.card-chip.card-suit-h .suit-symbol { color: #f87171; }
.card-chip.card-suit-d .suit-symbol { color: #fb923c; }
.card-chip.card-suit-c .suit-symbol { color: #34d399; }
.card-chip.card-suit-neutral .suit-symbol { color: #cbd5e1; }

.card-chip.card-winner {
  font-weight: 800 !important;
}

.card-chip.card-winner.card-winner-ns {
  border: 2px solid #38bdf8 !important;
  box-shadow: 0 0 6px rgba(56, 189, 248, 0.4);
}

.card-chip.card-winner.card-winner-ew {
  border: 2px solid #c084fc !important;
  box-shadow: 0 0 6px rgba(192, 132, 252, 0.4);
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

.node-card.centered-node {
  stroke: #f59e0b !important;
  stroke-width: 3.5px !important;
  filter: drop-shadow(0 0 8px rgba(245, 158, 11, 0.8));
}

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

.node-outcome-failure {
  fill: #450a0a;
  stroke: #ef4444;
}

.tree-link-failure {
  stroke: #991b1b;
  stroke-dasharray: 6 4;
}

.tree-edge-label-bg-failure {
  stroke: #b91c1c;
}

.tree-edge-label-failure {
  fill: #fca5a5;
}

.tree-legend {
  display: flex;
  align-items: center;
  gap: 10px;
  margin-right: 8px;
  font-size: 11px;
  color: #94a3b8;
  white-space: nowrap;
}

.tree-legend .legend-swatch {
  display: inline-block;
  width: 10px;
  height: 10px;
  border-radius: 2px;
  margin-right: 4px;
  vertical-align: -1px;
  border: 1.5px solid;
}

.tree-legend .legend-play { background: #1e293b; border-color: #3b82f6; }
.tree-legend .legend-makes { background: #064e3b; border-color: #10b981; }
.tree-legend .legend-fails { background: #450a0a; border-color: #ef4444; }

/* Sample Hands Tab Styles */
.sample-hands-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(195px, 1fr));
  gap: 10px;
  flex: 1;
  min-height: 0;
  overflow-y: auto;
  align-content: start;
  padding-right: 6px;
}

.sample-deal-card {
  background-color: #1a382b;
  border: 1px solid #2d5e46;
  border-radius: 8px;
  padding: 6px;
  display: flex;
  flex-direction: column;
  gap: 4px;
  position: relative;
  box-shadow: 0 2px 6px rgba(0, 0, 0, 0.25);
}

.sample-deal-id {
  position: absolute;
  top: 6px;
  left: 6px;
  font-size: 10px;
  font-weight: 700;
  color: #6ee7b7;
  background-color: #0b1b13;
  border: 1px solid #2d5e46;
  border-radius: 4px;
  padding: 1px 5px;
  font-family: monospace;
  line-height: 1.3;
}

.sample-deal-row-center {
  display: flex;
  justify-content: center;
  width: 100%;
}

.sample-deal-row-center .sample-hand-box {
  width: calc(50% - 2.5px);
}

.sample-deal-row-we {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 5px;
  width: 100%;
}

.sample-hand-box {
  background-color: #11261d;
  border: 1px solid #2a5a43;
  border-radius: 5px;
  padding: 3px 6px;
  display: flex;
  flex-direction: column;
  position: relative;
  min-width: 0;
}

.sample-hand-seat {
  position: absolute;
  top: 2px;
  right: 4px;
  font-size: 9px;
  font-weight: 700;
  color: rgba(110, 231, 183, 0.45);
  pointer-events: none;
}

.sample-suit-line {
  display: flex;
  align-items: center;
  gap: 3px;
  font-size: 11px;
  line-height: 1.22;
  font-family: monospace;
  font-weight: 700;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}

.sample-suit-sym {
  font-size: 11px;
  font-weight: 800;
  width: 10px;
  flex-shrink: 0;
  text-align: center;
}

.sample-suit-ranks {
  color: #f1f5f9;
  letter-spacing: 0.3px;
}

.sample-deal-card {
  cursor: pointer;
  transition: transform 0.15s ease, border-color 0.15s ease, box-shadow 0.15s ease;
}

.sample-deal-card:hover {
  transform: translateY(-1px);
  border-color: #22d3ee;
  box-shadow: 0 4px 10px rgba(34, 211, 238, 0.25);
}

.sample-deal-card.traced-deal-card {
  border-color: #22d3ee;
  box-shadow: 0 0 0 1px #22d3ee, 0 4px 12px rgba(34, 211, 238, 0.35);
}

/* Hand Trace Bar & Banner in Line Inspector */
.inspector-trace-bar {
  display: flex;
  align-items: center;
  gap: 6px;
  padding: 8px 12px;
  background-color: #0b1324;
  border-bottom: 1px solid var(--panel-border);
}

.trace-hand-label {
  font-size: 11px;
  font-weight: 700;
  color: var(--text-muted);
  text-transform: uppercase;
  letter-spacing: 0.4px;
  white-space: nowrap;
}

.trace-hand-input {
  width: 64px;
  padding: 4px 6px;
  font-size: 12px;
  font-family: monospace;
  background-color: #1e293b;
  border: 1px solid #334155;
  border-radius: 4px;
  color: #f8fafc;
}

.trace-hand-input:focus {
  outline: none;
  border-color: #22d3ee;
  box-shadow: 0 0 0 2px rgba(34, 211, 238, 0.2);
}

.trace-hand-btn,
.trace-hand-clear-btn {
  padding: 4px 8px;
  font-size: 11px;
  font-weight: 700;
  white-space: nowrap;
}

.trace-hand-btn {
  background-color: #0e7490;
  border-color: #06b6d4;
  color: #ecfeff;
}

.trace-hand-btn:hover {
  background-color: #0891b2;
  border-color: #22d3ee;
}

.trace-hand-error {
  padding: 6px 12px;
  font-size: 11px;
  color: #fca5a5;
  background-color: rgba(127, 29, 29, 0.35);
  border-bottom: 1px solid #7f1d1d;
}

.trace-hand-error.hidden,
.trace-hand-clear-btn.hidden {
  display: none !important;
}

.traced-hand-banner {
  background-color: #082f49;
  border: 1px solid #06b6d4;
  border-radius: 6px;
  padding: 8px 10px;
  display: flex;
  flex-direction: column;
  gap: 4px;
}

.traced-hand-banner.traced-hand-failure {
  background-color: #3b0a14;
  border-color: #f43f5e;
}

.traced-hand-banner-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  gap: 6px;
}

.traced-hand-title {
  font-size: 12px;
  font-weight: 700;
  color: #ecfeff;
  font-family: monospace;
}

.traced-hand-badge {
  font-size: 10px;
  font-weight: 800;
  padding: 1px 6px;
  border-radius: 4px;
  text-transform: uppercase;
  letter-spacing: 0.4px;
}

.traced-hand-badge.badge-makes {
  background-color: #065f46;
  color: #6ee7b7;
  border: 1px solid #10b981;
}

.traced-hand-badge.badge-fails {
  background-color: #7f1d1d;
  color: #fecaca;
  border: 1px solid #ef4444;
}

.traced-hand-holdings {
  font-size: 11px;
  font-family: monospace;
  color: #cbd5e1;
  display: flex;
  flex-direction: column;
  gap: 2px;
}

/* Highlighted Full Branch in Decision Tree */
.node-card.traced-branch-node {
  stroke: #22d3ee !important;
  stroke-width: 3px !important;
  filter: drop-shadow(0 0 7px rgba(34, 211, 238, 0.65));
}

.node-card.traced-branch-node.centered-node {
  stroke: #f59e0b !important;
  stroke-width: 3.5px !important;
  filter: drop-shadow(0 0 8px rgba(245, 158, 11, 0.85));
}

.tree-link.traced-branch-link {
  stroke: #22d3ee !important;
  stroke-width: 3.5px !important;
  filter: drop-shadow(0 0 4px rgba(34, 211, 238, 0.5));
}

.tree-edge-label-bg-traced {
  stroke: #22d3ee !important;
  stroke-width: 1.5px !important;
}

.tree-edge-label-traced {
  fill: #67e8f9 !important;
  font-weight: 700;
}

/* Propose Line Interactive Controls & SVG Card Highlighting */
.propose-line-bar {
  background-color: #0b192c;
  border: 1px solid #38bdf8;
  border-radius: 8px;
  padding: 8px 12px;
  display: flex;
  flex-direction: column;
  gap: 6px;
}

.propose-line-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  gap: 8px;
}

.propose-line-title {
  font-size: 12px;
  font-weight: 700;
  color: #7dd3fc;
  text-transform: uppercase;
  letter-spacing: 0.4px;
}

.propose-turn-badge {
  font-size: 11px;
  font-weight: 700;
  padding: 2px 8px;
  border-radius: 12px;
  background-color: rgba(56, 189, 248, 0.18);
  color: #bae6fd;
  border: 1px solid rgba(56, 189, 248, 0.4);
}

.propose-line-cards {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  gap: 6px;
  min-height: 22px;
  font-size: 12px;
  color: var(--text-muted);
}

.propose-card-pill {
  display: inline-flex;
  align-items: center;
  gap: 3px;
  padding: 2px 7px;
  border-radius: 4px;
  background-color: #1e293b;
  border: 1px solid #fbbf24;
  font-family: monospace;
  font-size: 12px;
  font-weight: 700;
  color: #fef3c7;
}

.propose-line-actions {
  display: flex;
  align-items: center;
  gap: 8px;
}

.propose-line-actions .btn-sm {
  flex: 1;
  padding: 6px 10px;
  font-size: 12px;
}

.btn:disabled {
  opacity: 0.45;
  cursor: not-allowed;
}

.card-rank-playable {
  cursor: pointer;
  fill: #fef08a;
  font-weight: 800;
  transition: fill 0.12s ease, filter 0.12s ease;
}

.card-rank-playable:hover {
  fill: #38bdf8;
  text-decoration: underline;
  filter: drop-shadow(0 0 4px rgba(56, 189, 248, 0.9));
}

.hand-box.hand-box-active-turn,
.unknown-box.hand-box-active-turn {
  stroke: #38bdf8 !important;
  stroke-width: 2.2px !important;
  stroke-dasharray: none !important;
  filter: drop-shadow(0 0 6px rgba(56, 189, 248, 0.65));
}

.node-proposed {
  fill: #172554;
  stroke: #38bdf8;
}

/* Compare Lines Tab Styles */
.compare-lines-content {
  display: flex;
  flex-direction: column;
  gap: 16px;
  flex: 1;
  min-height: 0;
  overflow-y: auto;
  padding-right: 6px;
}

.compare-lines-content.hidden {
  display: none !important;
}

.compare-lines-header-row {
  display: flex;
  justify-content: space-between;
  align-items: center;
  gap: 12px;
  margin-bottom: 12px;
  padding-bottom: 6px;
  border-bottom: 1px solid var(--panel-border);
}

.compare-lines-header-row h4 {
  margin-bottom: 0;
  padding-bottom: 0;
  border-bottom: none;
}

.compare-lines-hint {
  font-size: 12px;
  color: var(--text-muted);
}

.compare-lines-list {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.compare-line-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  padding: 10px 14px;
  background-color: #152238;
  border: 1px solid var(--panel-border);
  border-radius: 8px;
  cursor: pointer;
  transition: border-color 0.15s ease, background-color 0.15s ease;
}

.compare-line-row:hover {
  border-color: #60a5fa;
  background-color: #1a2b47;
}

.compare-line-row.selected {
  border-color: var(--primary);
  background-color: rgba(59, 130, 246, 0.12);
}

.compare-line-left {
  display: flex;
  align-items: center;
  gap: 10px;
  min-width: 0;
  flex: 1;
}

.compare-line-checkbox {
  width: 16px;
  height: 16px;
  accent-color: var(--primary);
  cursor: pointer;
  flex-shrink: 0;
}

.compare-line-title {
  font-size: 13.5px;
  font-weight: 600;
  color: var(--text-main);
  font-family: monospace;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.compare-line-right {
  display: flex;
  align-items: center;
  gap: 10px;
  flex-shrink: 0;
}

.compare-line-pct-badge {
  background-color: rgba(16, 185, 129, 0.15);
  color: #6ee7b7;
  border: 1px solid rgba(16, 185, 129, 0.35);
  border-radius: 6px;
  padding: 3px 10px;
  font-size: 12.5px;
  font-weight: 700;
  font-family: monospace;
}

.compare-lines-results {
  display: flex;
  flex-direction: column;
  gap: 16px;
}

.compare-pair-card {
  background-color: var(--bg-dark);
  border: 1px solid var(--panel-border);
  border-radius: 8px;
  padding: 16px;
  display: flex;
  flex-direction: column;
  gap: 14px;
}

.compare-pair-title {
  font-size: 14px;
  font-weight: 700;
  color: var(--text-main);
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  gap: 8px;
  padding-bottom: 8px;
  border-bottom: 1px solid var(--panel-border);
}

.compare-pair-vs {
  color: var(--text-muted);
  font-size: 12px;
  font-weight: 600;
  text-transform: uppercase;
}

.compare-chart-container {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.compare-stacked-bar {
  display: flex;
  width: 100%;
  height: 24px;
  border-radius: 6px;
  overflow: hidden;
  background-color: #1e293b;
  border: 1px solid var(--panel-border);
}

.compare-bar-seg {
  height: 100%;
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 11px;
  font-weight: 700;
  color: #ffffff;
  overflow: hidden;
  white-space: nowrap;
  transition: width 0.2s ease;
}

.compare-bar-both-pass { background-color: #059669; }
.compare-bar-only-a { background-color: #2563eb; }
.compare-bar-only-b { background-color: #d97706; }
.compare-bar-both-fail { background-color: #475569; }

.compare-chart-legend {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(180px, 1fr));
  gap: 8px;
  font-size: 12px;
}

.compare-legend-item {
  display: flex;
  align-items: center;
  gap: 6px;
  background-color: #152238;
  border: 1px solid #293548;
  border-radius: 6px;
  padding: 6px 10px;
}

.compare-legend-swatch {
  width: 10px;
  height: 10px;
  border-radius: 2px;
  flex-shrink: 0;
}

.compare-legend-label {
  color: var(--text-muted);
  flex: 1;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.compare-legend-value {
  font-family: monospace;
  font-weight: 700;
  color: var(--text-main);
}

.compare-diff-columns {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));
  gap: 14px;
}

.compare-diff-section {
  background-color: #131c2e;
  border: 1px solid var(--panel-border);
  border-radius: 8px;
  padding: 12px;
  display: flex;
  flex-direction: column;
  gap: 10px;
}

.compare-diff-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  gap: 8px;
  padding-bottom: 6px;
  border-bottom: 1px solid var(--panel-border);
}

.compare-diff-title {
  font-size: 12.5px;
  font-weight: 700;
  color: #93c5fd;
}

.compare-diff-section.diff-dir-b .compare-diff-title {
  color: #fcd34d;
}

.compare-diff-count {
  font-size: 11px;
  font-family: monospace;
  color: var(--text-muted);
}

.compare-diff-hands-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(185px, 1fr));
  gap: 8px;
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
  const statusProgress = document.getElementById('statusProgress');
  const statusProgressFill = document.getElementById('statusProgressFill');
  const execTime = document.getElementById('execTime');
  const resultsSummary = document.getElementById('resultsSummary');
  const statTricks = document.getElementById('statTricks');
  const statLeadLabel = document.getElementById('statLeadLabel');
  const statLead = document.getElementById('statLead');
  const cardStatSuccess = document.getElementById('cardStatSuccess');
  const cardStatFailure = document.getElementById('cardStatFailure');
  const statSuccess = document.getElementById('statSuccess');
  const statFailure = document.getElementById('statFailure');
  const equivalentLinesBar = document.getElementById('equivalentLinesBar');
  const equivalentLinesBadge = document.getElementById('equivalentLinesBadge');
  const equivalentLinesHint = document.getElementById('equivalentLinesHint');
  const equivalentLineSelect = document.getElementById('equivalentLineSelect');
  const prevLineBtn = document.getElementById('prevLineBtn');
  const nextLineBtn = document.getElementById('nextLineBtn');
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
  const traceHandInput = document.getElementById('traceHandInput');
  const traceHandBtn = document.getElementById('traceHandBtn');
  const clearTraceHandBtn = document.getElementById('clearTraceHandBtn');
  const traceHandError = document.getElementById('traceHandError');

  // Tab Switcher Elements
  const tabDecisionTreeBtn = document.getElementById('tabDecisionTreeBtn');
  const tabSummaryBtn = document.getElementById('tabSummaryBtn');
  const tabSampleHandsBtn = document.getElementById('tabSampleHandsBtn');
  const tabCompareLinesBtn = document.getElementById('tabCompareLinesBtn');
  const panelDecisionTree = document.getElementById('panelDecisionTree');
  const panelDistributionSummary = document.getElementById('panelDistributionSummary');
  const panelSampleHands = document.getElementById('panelSampleHands');
  const panelCompareLines = document.getElementById('panelCompareLines');
  const ruleCounterBadge = document.getElementById('ruleCounterBadge');
  const sampleHandsCounterBadge = document.getElementById('sampleHandsCounterBadge');
  const compareLinesCounterBadge = document.getElementById('compareLinesCounterBadge');
  const summaryEmptyNotice = document.getElementById('summaryEmptyNotice');
  const summaryContent = document.getElementById('summaryContent');
  const summaryRulesList = document.getElementById('summaryRulesList');
  const summaryBoundsTable = document.getElementById('summaryBoundsTable');
  const summaryHonorTable = document.getElementById('summaryHonorTable');

  const sampleHandsModeHeader = document.getElementById('sampleHandsModeHeader');
  const sampleHandsModeBadge = document.getElementById('sampleHandsModeBadge');
  const sampleHandsModeSub = document.getElementById('sampleHandsModeSub');
  const sampleHandsEmptyNotice = document.getElementById('sampleHandsEmptyNotice');
  const sampleHandsGrid = document.getElementById('sampleHandsGrid');

  const compareLinesEmptyNotice = document.getElementById('compareLinesEmptyNotice');
  const compareLinesContent = document.getElementById('compareLinesContent');
  const compareLinesHint = document.getElementById('compareLinesHint');
  const compareLinesList = document.getElementById('compareLinesList');
  const compareLinesResults = document.getElementById('compareLinesResults');

  // Propose Line Controls
  const proposeLineBar = document.getElementById('proposeLineBar');
  const proposeLineTitle = document.getElementById('proposeLineTitle');
  const proposeLineTurnBadge = document.getElementById('proposeLineTurnBadge');
  const proposeLineCards = document.getElementById('proposeLineCards');
  const computeNewLineBtn = document.getElementById('computeNewLineBtn');
  const undoProposedCardBtn = document.getElementById('undoProposedCardBtn');
  const resetLineBtn = document.getElementById('resetLineBtn');

  function activateTab(tabName) {
    if (tabDecisionTreeBtn) tabDecisionTreeBtn.classList.toggle('active', tabName === 'tree');
    if (tabSummaryBtn) tabSummaryBtn.classList.toggle('active', tabName === 'summary');
    if (tabSampleHandsBtn) tabSampleHandsBtn.classList.toggle('active', tabName === 'sample_hands');
    if (tabCompareLinesBtn) tabCompareLinesBtn.classList.toggle('active', tabName === 'compare_lines');

    if (panelDecisionTree) panelDecisionTree.classList.toggle('hidden', tabName !== 'tree');
    if (panelDistributionSummary) panelDistributionSummary.classList.toggle('hidden', tabName !== 'summary');
    if (panelSampleHands) panelSampleHands.classList.toggle('hidden', tabName !== 'sample_hands');
    if (panelCompareLines) panelCompareLines.classList.toggle('hidden', tabName !== 'compare_lines');
  }

  // Tab Switching Event Listeners
  if (tabDecisionTreeBtn) {
    tabDecisionTreeBtn.addEventListener('click', () => activateTab('tree'));
  }
  if (tabSummaryBtn) {
    tabSummaryBtn.addEventListener('click', () => activateTab('summary'));
  }
  if (tabSampleHandsBtn) {
    tabSampleHandsBtn.addEventListener('click', () => activateTab('sample_hands'));
  }
  if (tabCompareLinesBtn) {
    tabCompareLinesBtn.addEventListener('click', () => activateTab('compare_lines'));
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

  // Vertical Panel Resizer (Deal Configuration vs Decision Tree Panel)
  const gridContainer = document.getElementById('gridContainer') || document.querySelector('.grid-container');
  const dealSetupPanel = document.getElementById('dealSetupPanel') || document.querySelector('.deal-setup-panel');
  const panelResizer = document.getElementById('panelResizer');

  if (panelResizer && dealSetupPanel && gridContainer) {
    let isResizing = false;
    let resizeStartX = 0;
    let resizeStartWidth = 0;

    // Restore saved width from localStorage if available
    try {
      const savedWidth = localStorage.getItem('bridge_deal_setup_panel_width');
      if (savedWidth) {
        gridContainer.style.setProperty('--left-panel-width', savedWidth);
        dealSetupPanel.style.width = savedWidth;
      }
    } catch (e) {}

    const startResize = (e) => {
      isResizing = true;
      resizeStartX = e.clientX || (e.touches && e.touches[0] ? e.touches[0].clientX : 0);
      resizeStartWidth = dealSetupPanel.getBoundingClientRect().width;
      document.body.classList.add('is-resizing');
      panelResizer.classList.add('resizing');
      if (e.cancelable) e.preventDefault();
    };

    const doResize = (e) => {
      if (!isResizing) return;
      const currentX = e.clientX || (e.touches && e.touches[0] ? e.touches[0].clientX : 0);
      const dx = currentX - resizeStartX;
      const containerWidth = gridContainer.getBoundingClientRect().width;
      if (containerWidth <= 0) return;

      let newWidth = resizeStartWidth + dx;
      const minLeftWidth = 280;
      const resizerWidth = panelResizer.getBoundingClientRect().width || 14;
      const maxLeftWidth = containerWidth - 300 - resizerWidth;

      if (newWidth < minLeftWidth) newWidth = minLeftWidth;
      if (newWidth > maxLeftWidth) newWidth = maxLeftWidth;

      const pct = `${(newWidth / containerWidth) * 100}%`;
      gridContainer.style.setProperty('--left-panel-width', pct);
      dealSetupPanel.style.width = pct;

      window.dispatchEvent(new Event('resize'));
    };

    const stopResize = () => {
      if (!isResizing) return;
      isResizing = false;
      document.body.classList.remove('is-resizing');
      panelResizer.classList.remove('resizing');

      const currentWidthStyle = dealSetupPanel.style.width;
      if (currentWidthStyle) {
        try {
          localStorage.setItem('bridge_deal_setup_panel_width', currentWidthStyle);
        } catch (e) {}
      }
    };

    panelResizer.addEventListener('mousedown', startResize);
    panelResizer.addEventListener('touchstart', startResize, { passive: false });

    window.addEventListener('mousemove', doResize);
    window.addEventListener('touchmove', doResize, { passive: false });

    window.addEventListener('mouseup', stopResize);
    window.addEventListener('touchend', stopResize);
    window.addEventListener('touchcancel', stopResize);

    panelResizer.addEventListener('keydown', (e) => {
      if (e.key === 'ArrowLeft' || e.key === 'ArrowRight') {
        const containerWidth = gridContainer.getBoundingClientRect().width;
        if (containerWidth <= 0) return;
        const currentWidth = dealSetupPanel.getBoundingClientRect().width;
        const step = e.shiftKey ? 40 : 15;
        let newWidth = e.key === 'ArrowLeft' ? currentWidth - step : currentWidth + step;

        const minLeftWidth = 280;
        const resizerWidth = panelResizer.getBoundingClientRect().width || 14;
        const maxLeftWidth = containerWidth - 300 - resizerWidth;

        if (newWidth < minLeftWidth) newWidth = minLeftWidth;
        if (newWidth > maxLeftWidth) newWidth = maxLeftWidth;

        const pct = `${(newWidth / containerWidth) * 100}%`;
        gridContainer.style.setProperty('--left-panel-width', pct);
        dealSetupPanel.style.width = pct;
        try {
          localStorage.setItem('bridge_deal_setup_panel_width', pct);
        } catch (err) {}
        window.dispatchEvent(new Event('resize'));
        e.preventDefault();
      }
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
  let currentCenteredNode = null;
  let activeViewMode = 'success'; // 'success' or 'failure'
  let activeTracedBoardId = null;
  let activeTracedDeal = null;
  let activeTracedNodeSet = null;

  // Renders the decision tree for the current solve. Success and failure
  // outcomes live in one tree (failures as red leaves where the line runs
  // out), so this is independent of the Success/Failure view mode.
  function renderSolveTree() {
    if (currentSolveData && currentSolveData.tree &&
        Object.keys(currentSolveData.tree).length > 0) {
      currentTreeData = currentSolveData.tree;
      expandAndInspectNode(currentSolveData.tree, currentSolveData.tree, null, false);
    } else if (currentSolveData) {
      currentTreeData = null;
      currentCenteredNode = null;
      activeTrickCardStates = null;
      updateDealSvg();
      if (treeRootGroup) {
        treeRootGroup.innerHTML =
          `<text x="50%" y="50%" text-anchor="middle" fill="#f87171">No decision tree generated.</text>`;
      }
    }
  }

  function updateViewMode(mode) {
    activeViewMode = mode;
    const total = (currentSolveData && currentSolveData.total_boards) ? currentSolveData.total_boards : 1;
    const success = (currentSolveData && currentSolveData.success_boards !== undefined)
      ? currentSolveData.success_boards
      : ((currentSolveData && currentSolveData.covered_boards) || 0);
    const failure = (currentSolveData && currentSolveData.failure_boards !== undefined)
      ? currentSolveData.failure_boards
      : Math.max(0, total - success);

    if (mode === 'success') {
      if (cardStatSuccess) cardStatSuccess.classList.add('active');
      if (cardStatFailure) cardStatFailure.classList.remove('active');
      if (summaryModeBadge) {
        summaryModeBadge.className = 'mode-badge mode-success';
        summaryModeBadge.textContent = 'Success Deals';
      }
      if (sampleHandsModeBadge) {
        sampleHandsModeBadge.className = 'mode-badge mode-success';
        sampleHandsModeBadge.textContent = 'Covered Deals';
      }
      if (currentSolveData) {
        const pct = ((success / total) * 100).toFixed(1);
        if (summaryModeSub) {
          summaryModeSub.textContent = `(${success}/${total} boards, ${pct}%)`;
        }
        const sampleList = currentSolveData.sample_hands || [];
        if (sampleHandsModeSub) {
          sampleHandsModeSub.textContent =
            `(${success}/${total} boards, ${pct}% — showing ${sampleList.length})`;
        }
      }
      if (summaryModeHeader) summaryModeHeader.classList.remove('hidden');
      if (sampleHandsModeHeader) sampleHandsModeHeader.classList.remove('hidden');

      if (currentSolveData && currentSolveData.distribution_summary) {
        renderDistributionSummary(currentSolveData.distribution_summary);
      } else {
        clearDistributionSummary('No distribution summary available for success deals.');
      }

      if (currentSolveData && Array.isArray(currentSolveData.sample_hands) &&
          currentSolveData.sample_hands.length > 0) {
        renderSampleHands(currentSolveData.sample_hands);
      } else {
        clearSampleHands('No covered sample hands available.');
      }
    } else {
      if (cardStatFailure) cardStatFailure.classList.add('active');
      if (cardStatSuccess) cardStatSuccess.classList.remove('active');
      if (summaryModeBadge) {
        summaryModeBadge.className = 'mode-badge mode-failure';
        summaryModeBadge.textContent = 'Failure Deals';
      }
      if (sampleHandsModeBadge) {
        sampleHandsModeBadge.className = 'mode-badge mode-failure';
        sampleHandsModeBadge.textContent = 'Non-Covered (Failed) Deals';
      }
      if (currentSolveData) {
        const pct = ((failure / total) * 100).toFixed(1);
        if (summaryModeSub) {
          summaryModeSub.textContent = `(${failure}/${total} boards, ${pct}%)`;
        }
        const failSampleList = currentSolveData.failure_sample_hands || [];
        if (sampleHandsModeSub) {
          sampleHandsModeSub.textContent =
            `(${failure}/${total} boards, ${pct}% — showing ${failSampleList.length})`;
        }
      }
      if (summaryModeHeader) summaryModeHeader.classList.remove('hidden');
      if (sampleHandsModeHeader) sampleHandsModeHeader.classList.remove('hidden');

      if (currentSolveData && currentSolveData.failure_distribution_summary) {
        renderDistributionSummary(currentSolveData.failure_distribution_summary);
      } else {
        clearDistributionSummary('No failure distribution summary available (100% success rate or no failure deals).');
      }

      if (currentSolveData && Array.isArray(currentSolveData.failure_sample_hands) &&
          currentSolveData.failure_sample_hands.length > 0) {
        renderSampleHands(currentSolveData.failure_sample_hands);
      } else {
        clearSampleHands('No failed sample hands available (100% success rate or no failure deals).');
      }
    }
  }

  if (cardStatSuccess) {
    cardStatSuccess.addEventListener('click', () => {
      updateViewMode('success');
    });
    cardStatSuccess.addEventListener('keydown', (e) => {
      if (e.key === 'Enter' || e.key === ' ') {
        updateViewMode('success');
      }
    });
  }

  if (cardStatFailure) {
    cardStatFailure.addEventListener('click', () => {
      updateViewMode('failure');
    });
    cardStatFailure.addEventListener('keydown', (e) => {
      if (e.key === 'Enter' || e.key === ' ') {
        updateViewMode('failure');
      }
    });
  }

  let selectedCompareLineIndices = new Set();

  function formatLineSuccessPctPrefix(coveredOrSuccess, totalBoards) {
    const total = (typeof totalBoards === 'number' && totalBoards > 0) ? totalBoards : 1;
    const succ = (typeof coveredOrSuccess === 'number' && coveredOrSuccess >= 0) ? coveredOrSuccess : 0;
    const pct = ((succ / total) * 100).toFixed(1);
    return `${pct}% `;
  }

  function extractCoveredBoardIdsFromTree(node, outSet = new Set()) {
    if (!node) return outSet;
    if (!node.failure && (!node.children || node.children.length === 0) && Array.isArray(node.boards)) {
      node.boards.forEach(b => outSet.add(b));
    }
    if (Array.isArray(node.children)) {
      node.children.forEach(ch => extractCoveredBoardIdsFromTree(ch, outSet));
    }
    return outSet;
  }

  function buildSelectableLinesList() {
    const entries = [];
    const baseData = (originalSolveSnapshot && originalSolveSnapshot.data)
      ? originalSolveSnapshot.data
      : currentSolveData;
    const hasAlts = Array.isArray(proposedAltLines) && proposedAltLines.length > 0;

    if (baseData) {
      const baseTotal = baseData.total_boards || 1;
      const baseSuccess = baseData.success_boards !== undefined
        ? baseData.success_boards
        : (baseData.covered_boards || 0);
      const origLines = Array.isArray(baseData.lines) ? baseData.lines : [];
      if (origLines.length > 1) {
        origLines.forEach((lineObj, idx) => {
          let label = lineObj.label || `Line ${idx + 1}`;
          if (hasAlts) {
            label = label.replace(/^Line\s+(\d+):/i, 'Original line $1:');
          }
          const lineSucc = (lineObj && lineObj.covered_boards !== undefined)
            ? lineObj.covered_boards
            : baseSuccess;
          const pctPrefix = formatLineSuccessPctPrefix(lineSucc, baseTotal);
          let covIds = Array.isArray(lineObj && lineObj.covered_board_ids)
            ? lineObj.covered_board_ids
            : null;
          if (!covIds && lineObj && lineObj.tree) {
            covIds = Array.from(extractCoveredBoardIdsFromTree(lineObj.tree));
          }
          entries.push({
            kind: 'original',
            rawTitle: label,
            label: `${pctPrefix}${label}`,
            coveredBoards: lineSucc,
            totalBoards: baseTotal,
            coveredBoardIds: covIds || [],
            lineObj: lineObj,
            baseData: baseData
          });
        });
      } else {
        const singleLineObj = origLines.length === 1 ? origLines[0] : {
          tree: baseData.tree,
          best_lead: baseData.best_lead,
          covered_boards: baseData.covered_boards,
          covered_board_ids: baseData.covered_board_ids,
          distribution_summary: baseData.distribution_summary,
          failure_distribution_summary: baseData.failure_distribution_summary
        };
        const lineSucc = (singleLineObj && singleLineObj.covered_boards !== undefined)
          ? singleLineObj.covered_boards
          : baseSuccess;
        const pctPrefix = formatLineSuccessPctPrefix(lineSucc, baseTotal);
        let rawTitle = (singleLineObj && singleLineObj.label)
          ? singleLineObj.label.replace(/^Line\s+1:/i, 'Original line:')
          : 'Original line';
        let covIds = Array.isArray(singleLineObj && singleLineObj.covered_board_ids)
          ? singleLineObj.covered_board_ids
          : (Array.isArray(baseData.covered_board_ids) ? baseData.covered_board_ids : null);
        if (!covIds && singleLineObj && singleLineObj.tree) {
          covIds = Array.from(extractCoveredBoardIdsFromTree(singleLineObj.tree));
        }
        entries.push({
          kind: 'original',
          rawTitle: rawTitle,
          label: `${pctPrefix}Original line`,
          coveredBoards: lineSucc,
          totalBoards: baseTotal,
          coveredBoardIds: covIds || [],
          lineObj: singleLineObj,
          baseData: baseData
        });
      }
    }

    if (hasAlts) {
      proposedAltLines.forEach((altEntry) => {
        const altData = altEntry.data || {};
        const altTotal = altData.total_boards || 1;
        const altSuccess = altData.success_boards !== undefined
          ? altData.success_boards
          : (altData.covered_boards || 0);
        const pctPrefix = formatLineSuccessPctPrefix(altSuccess, altTotal);
        const altOrigLines = Array.isArray(altData.lines) ? altData.lines : [];
        let covIds = Array.isArray(altData.covered_board_ids)
          ? altData.covered_board_ids
          : (altOrigLines.length > 0 && Array.isArray(altOrigLines[0].covered_board_ids)
              ? altOrigLines[0].covered_board_ids
              : null);
        if (!covIds && altData.tree) {
          covIds = Array.from(extractCoveredBoardIdsFromTree(altData.tree));
        }
        entries.push({
          kind: 'alt',
          rawTitle: altEntry.label,
          label: `${pctPrefix}${altEntry.label}`,
          coveredBoards: altSuccess,
          totalBoards: altTotal,
          coveredBoardIds: covIds || [],
          altData: altEntry.data
        });
      });
    }

    return entries;
  }

  function clearCompareLines(msg = 'No lines computed yet. Click "Solve Deal" (and optionally compute alternate lines) to compare lines of play.') {
    selectedCompareLineIndices.clear();
    if (compareLinesCounterBadge) {
      compareLinesCounterBadge.textContent = '0';
      compareLinesCounterBadge.classList.add('hidden');
    }
    if (compareLinesContent) {
      compareLinesContent.classList.add('hidden');
    }
    if (compareLinesList) {
      compareLinesList.innerHTML = '';
    }
    if (compareLinesResults) {
      compareLinesResults.innerHTML = '';
    }
    if (compareLinesEmptyNotice) {
      compareLinesEmptyNotice.textContent = msg;
      compareLinesEmptyNotice.classList.remove('hidden');
    }
  }

  function findDealForCompare(boardId, entries) {
    for (const entry of entries) {
      const src = entry.baseData || entry.altData;
      if (src && Array.isArray(src.all_hands)) {
        const found = src.all_hands.find(d => d && d.board_id === boardId);
        if (found) return found;
      }
    }
    return findDealByBoardId(boardId);
  }

  function createCompareDealCard(deal, boardId, targetLineIdx) {
    const card = document.createElement('div');
    const boardNum = boardId + 1;
    card.className = `sample-deal-card${activeTracedBoardId === boardId ? ' traced-deal-card' : ''}`;
    card.dataset.boardId = String(boardId);
    card.title = `Click to load Hand #${boardNum} in the Decision Tree`;
    card.addEventListener('click', () => {
      if (typeof targetLineIdx === 'number' &&
          (!currentSolveData || currentSolveData.active_line_index !== targetLineIdx)) {
        selectEquivalentLine(targetLineIdx);
      }
      traceHandByNumber(boardNum);
    });

    const idBadge = document.createElement('span');
    idBadge.className = 'sample-deal-id';
    idBadge.textContent = `#${boardNum}`;
    card.appendChild(idBadge);

    const rowNorth = document.createElement('div');
    rowNorth.className = 'sample-deal-row-center';
    rowNorth.appendChild(createSampleHandBox('NORTH', deal ? deal.N : ''));
    card.appendChild(rowNorth);

    const rowWE = document.createElement('div');
    rowWE.className = 'sample-deal-row-we';
    rowWE.appendChild(createSampleHandBox('WEST', deal ? deal.W : ''));
    rowWE.appendChild(createSampleHandBox('EAST', deal ? deal.E : ''));
    card.appendChild(rowWE);

    const rowSouth = document.createElement('div');
    rowSouth.className = 'sample-deal-row-center';
    rowSouth.appendChild(createSampleHandBox('SOUTH', deal ? deal.S : ''));
    card.appendChild(rowSouth);

    return card;
  }

  function renderComparePairSection(entryA, idxA, entryB, idxB, allEntries) {
    const total = Math.max(1, entryA.totalBoards || entryB.totalBoards || 1);
    const setA = new Set(entryA.coveredBoardIds || []);
    const setB = new Set(entryB.coveredBoardIds || []);

    const bothPass = [];
    const onlyA = []; // Succeeds in A, fails in B
    const onlyB = []; // Fails in A, succeeds in B
    const bothFail = [];

    for (let b = 0; b < total; b++) {
      const inA = setA.has(b);
      const inB = setB.has(b);
      if (inA && inB) bothPass.push(b);
      else if (inA && !inB) onlyA.push(b);
      else if (!inA && inB) onlyB.push(b);
      else bothFail.push(b);
    }

    const pairCard = document.createElement('div');
    pairCard.className = 'compare-pair-card';

    const titleEl = document.createElement('div');
    titleEl.className = 'compare-pair-title';
    const spanA = document.createElement('span');
    spanA.textContent = entryA.rawTitle || entryA.label;
    const vsSpan = document.createElement('span');
    vsSpan.className = 'compare-pair-vs';
    vsSpan.textContent = 'vs';
    const spanB = document.createElement('span');
    spanB.textContent = entryB.rawTitle || entryB.label;
    titleEl.appendChild(spanA);
    titleEl.appendChild(vsSpan);
    titleEl.appendChild(spanB);
    pairCard.appendChild(titleEl);

    // Visual stacked bar chart + legend
    const chartContainer = document.createElement('div');
    chartContainer.className = 'compare-chart-container';

    const barEl = document.createElement('div');
    barEl.className = 'compare-stacked-bar';

    const segments = [
      { cls: 'compare-bar-both-pass', count: bothPass.length, label: 'Succeed in Both', color: '#059669' },
      { cls: 'compare-bar-only-a', count: onlyA.length, label: `Succeed in #${idxA + 1}, Fail in #${idxB + 1}`, color: '#2563eb' },
      { cls: 'compare-bar-only-b', count: onlyB.length, label: `Fail in #${idxA + 1}, Succeed in #${idxB + 1}`, color: '#d97706' },
      { cls: 'compare-bar-both-fail', count: bothFail.length, label: 'Fail in Both', color: '#475569' }
    ];

    segments.forEach(seg => {
      if (seg.count <= 0) return;
      const pct = (seg.count / total) * 100;
      const segEl = document.createElement('div');
      segEl.className = `compare-bar-seg ${seg.cls}`;
      segEl.style.width = `${pct.toFixed(2)}%`;
      const pctRound = pct.toFixed(1);
      segEl.title = `${seg.label}: ${seg.count}/${total} (${pctRound}%)`;
      if (pct >= 8) {
        segEl.textContent = `${seg.count} (${pctRound}%)`;
      }
      barEl.appendChild(segEl);
    });
    chartContainer.appendChild(barEl);

    const legendEl = document.createElement('div');
    legendEl.className = 'compare-chart-legend';
    segments.forEach(seg => {
      const pctRound = ((seg.count / total) * 100).toFixed(1);
      const item = document.createElement('div');
      item.className = 'compare-legend-item';
      item.innerHTML = `
        <span class="compare-legend-swatch" style="background-color: ${seg.color};"></span>
        <span class="compare-legend-label" title="${seg.label}">${seg.label}</span>
        <span class="compare-legend-value">${seg.count}/${total} (${pctRound}%)</span>
      `;
      legendEl.appendChild(item);
    });
    chartContainer.appendChild(legendEl);
    pairCard.appendChild(chartContainer);

    // Directional discrepancy hands (up to 10 hands per direction)
    const maxSampleDiffHands = 10;
    const diffCols = document.createElement('div');
    diffCols.className = 'compare-diff-columns';

    const buildDirectionSection = (boardIds, dirTitle, dirClass, targetLineIdx) => {
      const sec = document.createElement('div');
      sec.className = `compare-diff-section ${dirClass}`;

      const hdr = document.createElement('div');
      hdr.className = 'compare-diff-header';

      const tSpan = document.createElement('span');
      tSpan.className = 'compare-diff-title';
      tSpan.textContent = dirTitle;

      const cSpan = document.createElement('span');
      cSpan.className = 'compare-diff-count';
      const shownCount = Math.min(boardIds.length, maxSampleDiffHands);
      cSpan.textContent = boardIds.length > maxSampleDiffHands
        ? `Showing ${shownCount} of ${boardIds.length} deals`
        : `${boardIds.length} deal${boardIds.length === 1 ? '' : 's'}`;

      hdr.appendChild(tSpan);
      hdr.appendChild(cSpan);
      sec.appendChild(hdr);

      if (boardIds.length === 0) {
        const emptyEl = document.createElement('div');
        emptyEl.className = 'summary-empty';
        emptyEl.style.padding = '14px';
        emptyEl.textContent = 'No hands in this category.';
        sec.appendChild(emptyEl);
      } else {
        const grid = document.createElement('div');
        grid.className = 'compare-diff-hands-grid';
        boardIds.slice(0, maxSampleDiffHands).forEach(bId => {
          const deal = findDealForCompare(bId, allEntries);
          grid.appendChild(createCompareDealCard(deal, bId, targetLineIdx));
        });
        sec.appendChild(grid);
      }
      return sec;
    };

    const nameA = entryA.rawTitle || `Line #${idxA + 1}`;
    const nameB = entryB.rawTitle || `Line #${idxB + 1}`;
    diffCols.appendChild(buildDirectionSection(
      onlyA,
      `Succeeds in ${nameA}, Fails in ${nameB}`,
      'diff-dir-a',
      idxA
    ));
    diffCols.appendChild(buildDirectionSection(
      onlyB,
      `Fails in ${nameA}, Succeeds in ${nameB}`,
      'diff-dir-b',
      idxB
    ));
    pairCard.appendChild(diffCols);

    return pairCard;
  }

  function renderCompareLinesTab() {
    const entries = buildSelectableLinesList();
    if (!entries || entries.length === 0) {
      clearCompareLines();
      return;
    }

    if (compareLinesCounterBadge) {
      compareLinesCounterBadge.textContent = String(entries.length);
      compareLinesCounterBadge.classList.remove('hidden');
    }
    if (compareLinesEmptyNotice) {
      compareLinesEmptyNotice.classList.add('hidden');
    }
    if (compareLinesContent) {
      compareLinesContent.classList.remove('hidden');
    }

    // Prune out-of-range selected indices and auto-select the first 2 lines if <2 are selected
    Array.from(selectedCompareLineIndices).forEach(idx => {
      if (idx < 0 || idx >= entries.length) {
        selectedCompareLineIndices.delete(idx);
      }
    });
    if (selectedCompareLineIndices.size === 0 && entries.length >= 2) {
      selectedCompareLineIndices.add(0);
      selectedCompareLineIndices.add(entries.length - 1);
    } else if (selectedCompareLineIndices.size === 0 && entries.length === 1) {
      selectedCompareLineIndices.add(0);
    } else if (selectedCompareLineIndices.size === 1 && entries.length === 2) {
      // When a second line (e.g. an Alt line) is computed, auto-select both for immediate comparison
      selectedCompareLineIndices.add(0);
      selectedCompareLineIndices.add(1);
    }

    if (compareLinesHint) {
      compareLinesHint.textContent = entries.length < 2
        ? 'Only 1 line computed so far — propose a card on the deal diagram and click "Compute new line" to compare!'
        : `${selectedCompareLineIndices.size} of ${entries.length} selected`;
    }

    if (compareLinesList) {
      compareLinesList.innerHTML = '';
      entries.forEach((entry, idx) => {
        const isSelected = selectedCompareLineIndices.has(idx);
        const row = document.createElement('div');
        row.className = `compare-line-row${isSelected ? ' selected' : ''}`;
        row.dataset.lineIndex = String(idx);

        const left = document.createElement('div');
        left.className = 'compare-line-left';

        const cb = document.createElement('input');
        cb.type = 'checkbox';
        cb.className = 'compare-line-checkbox';
        cb.checked = isSelected;
        cb.setAttribute('aria-label', `Select ${entry.rawTitle || entry.label}`);

        const titleSpan = document.createElement('span');
        titleSpan.className = 'compare-line-title';
        titleSpan.textContent = entry.rawTitle || entry.label;
        titleSpan.title = entry.rawTitle || entry.label;

        left.appendChild(cb);
        left.appendChild(titleSpan);

        const right = document.createElement('div');
        right.className = 'compare-line-right';

        const total = entry.totalBoards || 1;
        const succ = entry.coveredBoards !== undefined ? entry.coveredBoards : 0;
        const pct = ((succ / total) * 100).toFixed(1);
        const pctBadge = document.createElement('span');
        pctBadge.className = 'compare-line-pct-badge';
        pctBadge.textContent = `${succ}/${total} (${pct}%)`;
        right.appendChild(pctBadge);

        const toggleSelection = () => {
          if (selectedCompareLineIndices.has(idx)) {
            selectedCompareLineIndices.delete(idx);
          } else {
            selectedCompareLineIndices.add(idx);
          }
          renderCompareLinesTab();
        };

        cb.addEventListener('click', (e) => {
          e.stopPropagation();
          toggleSelection();
        });
        row.addEventListener('click', () => {
          toggleSelection();
        });

        row.appendChild(left);
        row.appendChild(right);
        compareLinesList.appendChild(row);
      });
    }

    if (compareLinesResults) {
      compareLinesResults.innerHTML = '';
      const sortedSelected = Array.from(selectedCompareLineIndices).sort((a, b) => a - b);
      if (sortedSelected.length < 2) {
        const promptEl = document.createElement('div');
        promptEl.className = 'summary-empty';
        promptEl.textContent = entries.length < 2
          ? 'Compute at least 2 lines to compare their succeeding and failing deals.'
          : 'Select at least 2 lines above to view the comparison chart and differing deals.';
        compareLinesResults.appendChild(promptEl);
      } else {
        for (let i = 0; i < sortedSelected.length; i++) {
          for (let j = i + 1; j < sortedSelected.length; j++) {
            const idxA = sortedSelected[i];
            const idxB = sortedSelected[j];
            compareLinesResults.appendChild(
              renderComparePairSection(entries[idxA], idxA, entries[idxB], idxB, entries)
            );
          }
        }
      }
    }
  }

  function populateEquivalentLinesControls(data, selectedEntryIndex = null) {
    if (!equivalentLinesBar || !equivalentLineSelect) return;
    equivalentLineSelect.innerHTML = '';
    const entries = buildSelectableLinesList();
    const hasAlts = Array.isArray(proposedAltLines) && proposedAltLines.length > 0;

    if (entries.length > 1) {
      entries.forEach((entry, idx) => {
        const opt = document.createElement('option');
        opt.value = String(idx);
        opt.textContent = entry.label;
        equivalentLineSelect.appendChild(opt);
      });
      const activeIdx = (selectedEntryIndex !== null && selectedEntryIndex >= 0 && selectedEntryIndex < entries.length)
        ? selectedEntryIndex
        : 0;
      equivalentLineSelect.value = String(activeIdx);
      if (currentSolveData) {
        currentSolveData.active_line_index = activeIdx;
      }
      if (equivalentLinesBadge) {
        equivalentLinesBadge.textContent = hasAlts
          ? `${entries.length} Lines (Original + ${proposedAltLines.length} Alt)`
          : `${entries.length} Equivalent Lines`;
      }
      if (equivalentLinesHint) {
        equivalentLinesHint.textContent = hasAlts
          ? 'Switch between original and proposed lines:'
          : 'Choose an equally optimal line of play:';
      }
      equivalentLinesBar.classList.remove('hidden');
    } else {
      equivalentLinesBar.classList.add('hidden');
    }
    renderCompareLinesTab();
  }

  function selectEquivalentLine(rawIndex) {
    const entries = buildSelectableLinesList();
    if (entries.length === 0) return;

    const count = entries.length;
    const idx = ((parseInt(rawIndex, 10) || 0) % count + count) % count;
    const entry = entries[idx];
    if (!entry) return;

    if (entry.kind === 'alt' && entry.altData) {
      currentSolveData = JSON.parse(JSON.stringify(entry.altData));
      currentSolveData.active_line_index = idx;
      hasComputedProposedLine = true;
    } else if (entry.kind === 'original' && entry.baseData) {
      const baseCopy = JSON.parse(JSON.stringify(entry.baseData));
      const lineObj = entry.lineObj;
      if (lineObj) {
        baseCopy.tree = JSON.parse(JSON.stringify(lineObj.tree || {}));
        baseCopy.best_lead = lineObj.best_lead || baseCopy.best_lead || '-';
        baseCopy.distribution_summary = lineObj.distribution_summary || null;
        baseCopy.failure_distribution_summary =
          lineObj.failure_distribution_summary || null;

        const total = baseCopy.total_boards || 1;
        if (Array.isArray(lineObj.covered_board_ids) &&
            Array.isArray(baseCopy.all_hands)) {
          const coveredSet = new Set(lineObj.covered_board_ids);
          const sampleHands = [];
          const failureSampleHands = [];
          baseCopy.all_hands.forEach(handEntry => {
            const isCov = coveredSet.has(handEntry.board_id);
            handEntry.succeeded = isCov;
            if (isCov && sampleHands.length < 100) {
              sampleHands.push(handEntry);
            } else if (!isCov && failureSampleHands.length < 100) {
              failureSampleHands.push(handEntry);
            }
          });
          baseCopy.sample_hands = sampleHands;
          baseCopy.failure_sample_hands = failureSampleHands;
          baseCopy.success_boards = coveredSet.size;
          baseCopy.covered_boards = coveredSet.size;
          baseCopy.failure_boards = Math.max(0, total - coveredSet.size);
        }
      }
      currentSolveData = baseCopy;
      currentSolveData.active_line_index = idx;
      hasComputedProposedLine = (Array.isArray(proposedAltLines) && proposedAltLines.length > 0);
    }

    activeProposalContext = null;
    activeTrickCardStates = null;
    updateProposeLineBarUI();

    if (statTricks) {
      statTricks.textContent = currentSolveData.tricks_won !== undefined
        ? currentSolveData.tricks_won
        : '-';
    }
    updateLeadStat(currentSolveData);
    const total = currentSolveData.total_boards || 1;
    const success = currentSolveData.success_boards !== undefined
      ? currentSolveData.success_boards
      : (currentSolveData.covered_boards || 0);
    const failure = currentSolveData.failure_boards !== undefined
      ? currentSolveData.failure_boards
      : Math.max(0, total - success);
    const successPct = ((success / total) * 100).toFixed(1);
    const failurePct = ((failure / total) * 100).toFixed(1);
    if (statSuccess) statSuccess.textContent = `${success}/${total} (${successPct}%)`;
    if (statFailure) statFailure.textContent = `${failure}/${total} (${failurePct}%)`;

    if (equivalentLineSelect) {
      equivalentLineSelect.value = String(idx);
    }

    if (activeTracedBoardId !== null) {
      traceHandByNumber(activeTracedBoardId + 1);
      updateViewMode(activeViewMode);
    } else {
      renderSolveTree();
      updateViewMode(activeViewMode);
    }
  }

  if (equivalentLineSelect) {
    equivalentLineSelect.addEventListener('change', () => {
      selectEquivalentLine(parseInt(equivalentLineSelect.value, 10));
    });
  }

  if (prevLineBtn) {
    prevLineBtn.addEventListener('click', () => {
      const cur = (currentSolveData && typeof currentSolveData.active_line_index === 'number')
        ? currentSolveData.active_line_index
        : 0;
      selectEquivalentLine(cur - 1);
    });
  }

  if (nextLineBtn) {
    nextLineBtn.addEventListener('click', () => {
      const cur = (currentSolveData && typeof currentSolveData.active_line_index === 'number')
        ? currentSolveData.active_line_index
        : 0;
      selectEquivalentLine(cur + 1);
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
    },
    safety_play: {
      name: "Safety Play Deal (NT)",
      trump: 'NOTRUMP',
      leader: 'NORTH',
      target: 7,
      iters: 100,
      north: 'S A43; H A54; D 432; C -;',
      east: '',
      south: 'S 2; H 32; D AK8765; C -;',
      west: '',
      unknown_cards: 'S KQJT987 H KQJT987 D QJT9'
    },
    twelve_card_spades: {
      name: "12-Card Spades Deal (4S)",
      trump: 'SPADES',
      leader: 'NORTH',
      target: 9,
      iters: 500,
      north: 'S Q73; H JT87; D 87; C J95;',
      east: '',
      south: 'S AKJ652; H A43; D 5; C KQ;',
      west: '',
      unknown_cards: 'S T984 H KQ9652 D KQJT96 C AT876432'
    },
    restricted_choice: {
      name: "Restricted Choice (7S)",
      trump: 'SPADES',
      leader: 'NORTH',
      target: 13,
      iters: 50,
      north: 'S AT987; H AK; D AKQ; C AKQ;',
      east: '',
      south: 'S K654; H QJT; D JT9; C JT9;',
      west: '',
      unknown_cards: 'S QJ32 H 98765432 D 8765432 C 8765432'
    },
    mollo_6s: {
      name: "Mollo 6S",
      trump: 'SPADES',
      leader: 'EAST',
      cur_trick: ['DQ'],
      target: 12,
      iters: 50,
      north: 'S AKJT83; H AKQ; D -; C A652;',
      east: 'D Q;',
      south: 'S Q52; H JT984; D K2; C 874;',
      west: 'D A;',
      unknown_cards: 'S 9764 H 76532 D JT9876543 C KQJT93'
    },
    two_way_finesse: {
      name: "2-Way Finesse",
      trump: 'NOTRUMP',
      leader: 'NORTH',
      target: 10,
      iters: 50,
      north: 'S AJ97; H A2; D A2; C A2;',
      east: '',
      south: 'S KT86; H K3; D K3; C K3;',
      west: '',
      unknown_cards: 'S Q5432 H 98765 D 98765 C 98765'
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

  let activeTrickCardStates = null; // { N, E, S, W, U } -> { played: [...], current: [...] }
  let presetCurTrick = []; // Optional initial partial trick cards from preset (e.g. ['DQ'])
  let originalSolveSnapshot = null; // Saved original solve data & inputs before any proposed line
  let proposedAltLines = []; // Saved alternative proposed lines: [{ label, data }]
  let hasComputedProposedLine = false;
  let activeProposalContext = null; // { trickNum, selectedIdx, leaderIdx, nsTricksBefore, playedStates, prefixTrickRows, proposedCards: [] }

  function getActiveInitialCurTrick() {
    if (!Array.isArray(presetCurTrick) || presetCurTrick.length === 0) return [];
    const leaderIdx = getSeatIndex(leaderSelect ? leaderSelect.value : 'NORTH');
    const seatInputs = [
      inputNorth ? inputNorth.value : '',
      inputEast ? inputEast.value : '',
      inputSouth ? inputSouth.value : '',
      inputWest ? inputWest.value : ''
    ];
    const valid = [];
    for (let i = 0; i < presetCurTrick.length; i++) {
      const cStr = String(presetCurTrick[i] || '').trim().toUpperCase();
      if (cStr.length < 2) return [];
      const suit = cStr[0];
      const rank = cStr.slice(1) === '10' ? 'T' : cStr.slice(1);
      const seatIdx = (leaderIdx + i) % 4;
      const suits = parseHandToSuits(seatInputs[seatIdx]);
      const toks = tokenizeSuitRanks(suits[suit] || '-').map(t => (t === '10' ? 'T' : t.toUpperCase()));
      if (!toks.includes(rank)) return [];
      valid.push(`${suit}${rank}`);
    }
    return valid;
  }

  function buildInitialCurTrickCardStates() {
    const initCur = getActiveInitialCurTrick();
    if (initCur.length === 0) return null;
    const states = {
      N: { played: [], current: [] },
      E: { played: [], current: [] },
      S: { played: [], current: [] },
      W: { played: [], current: [] },
      U: { played: [], current: [] }
    };
    const leaderIdx = getSeatIndex(leaderSelect ? leaderSelect.value : 'NORTH');
    const trumpCode = getTrumpCode();
    let leadSuit = null;
    initCur.forEach((cStr, idx) => {
      const seatIdx = (leaderIdx + idx) % 4;
      const seatName = getSeatName(seatIdx);
      const parsed = parseSingleCardToken(cStr, leadSuit, trumpCode);
      if (idx === 0) leadSuit = parsed.suit;
      parsed.seatIdx = seatIdx;
      parsed.seatName = seatName;
      parsed.isLead = (idx === 0);
      if (states[seatName]) {
        states[seatName].current.push(parsed);
      }
    });
    return states;
  }

  function clonePlayStates(states) {
    if (!states) return null;
    const copy = {};
    ['N', 'E', 'S', 'W', 'U'].forEach(k => {
      copy[k] = {
        played: (states[k] && Array.isArray(states[k].played)) ? states[k].played.map(c => Object.assign({}, c)) : [],
        current: (states[k] && Array.isArray(states[k].current)) ? states[k].current.map(c => Object.assign({}, c)) : []
      };
    });
    return copy;
  }

  function updateLeadStat(data) {
    const hasForcedLead = data
      ? Boolean(data.is_forced_lead || getActiveInitialCurTrick().length > 0)
      : (getActiveInitialCurTrick().length > 0);
    if (statLeadLabel) {
      statLeadLabel.textContent = hasForcedLead ? 'Card Led' : 'Optimal Lead';
    }
    if (statLead) {
      statLead.textContent = (data && data.best_lead) ? data.best_lead : '-';
    }
  }

  function updateProposeLineBarUI() {
    const minProposedCards = activeProposalContext ? (activeProposalContext.initialCurCount || 0) : 0;
    const hasProposedCards = Boolean(
      activeProposalContext && activeProposalContext.proposedCards.length > minProposedCards
    );
    if (solveBtn) {
      solveBtn.disabled = hasProposedCards;
      solveBtn.title = hasProposedCards
        ? 'Use "Compute new line" to analyze the proposed card(s), or "Reset" / "Undo" to clear them'
        : '';
    }
    if (!proposeLineBar) return;
    if (!activeProposalContext && !hasComputedProposedLine) {
      proposeLineBar.classList.add('hidden');
      return;
    }
    proposeLineBar.classList.remove('hidden');

    if (activeProposalContext) {
      const { trickNum, leaderIdx, proposedCards } = activeProposalContext;
      if (proposeLineTitle) {
        proposeLineTitle.textContent = `Propose Trick #${trickNum}`;
      }
      if (proposeLineTurnBadge) {
        if (proposedCards.length < 4) {
          const nextSeatIdx = (leaderIdx + proposedCards.length) % 4;
          const nextSeatName = getSeatName(nextSeatIdx);
          proposeLineTurnBadge.textContent = `${nextSeatName} to play (${proposedCards.length}/4)`;
          proposeLineTurnBadge.classList.remove('hidden');
        } else {
          proposeLineTurnBadge.textContent = `Trick #${trickNum} complete (4/4)`;
          proposeLineTurnBadge.classList.remove('hidden');
        }
      }
      if (proposeLineCards) {
        proposeLineCards.innerHTML = '';
        if (proposedCards.length === 0) {
          const hint = document.createElement('span');
          const nextSeatName = getSeatName(leaderIdx);
          hint.textContent = `Click a legal card for ${nextSeatName} on the diagram above to propose a line.`;
          proposeLineCards.appendChild(hint);
        } else {
          proposedCards.forEach(c => {
            const pill = document.createElement('span');
            pill.className = 'propose-card-pill';
            pill.innerHTML = `${c.seatName}: <span class="${c.suitClass}">${c.suitSym}</span>${c.rankStr}`;
            proposeLineCards.appendChild(pill);
          });
        }
      }
      if (computeNewLineBtn) {
        computeNewLineBtn.disabled = !hasProposedCards;
      }
      if (undoProposedCardBtn) {
        undoProposedCardBtn.disabled = !hasProposedCards;
      }
    } else {
      if (proposeLineTitle) {
        proposeLineTitle.textContent = 'Proposed Line Active';
      }
      if (proposeLineTurnBadge) {
        proposeLineTurnBadge.textContent = 'Select a trick to propose further';
        proposeLineTurnBadge.classList.remove('hidden');
      }
      if (proposeLineCards) {
        proposeLineCards.innerHTML = '<span>Viewing solved continuation from proposed line.</span>';
      }
      if (computeNewLineBtn) computeNewLineBtn.disabled = true;
      if (undoProposedCardBtn) undoProposedCardBtn.disabled = true;
    }

    if (resetLineBtn) {
      resetLineBtn.disabled = !(hasComputedProposedLine || hasProposedCards);
    }
  }

  function applyProposalStateToSvg() {
    if (!activeProposalContext) return;
    const states = clonePlayStates(activeProposalContext.playedStates);
    activeProposalContext.proposedCards.forEach((c, idx) => {
      const entry = Object.assign({}, c, { isLead: idx === 0 });
      if (c.fromUnknown) {
        states.U.current.push(entry);
      }
      if (states[c.seatName]) {
        states[c.seatName].current.push(entry);
      }
    });
    activeTrickCardStates = states;
    updateProposeLineBarUI();
    updateDealSvg();
  }

  function handleProposeCardClick(suitKey, rankToken, fromUnknown, targetSeatIdx) {
    if (!activeProposalContext || activeProposalContext.proposedCards.length >= 4) return;
    const seatName = getSeatName(targetSeatIdx);
    const normRank = (rankToken === '10') ? 'T' : (rankToken === 'x' ? 'x' : rankToken.toUpperCase());
    const trumpCode = getTrumpCode();
    const leadSuit = activeProposalContext.proposedCards.length > 0
      ? activeProposalContext.proposedCards[0].suit
      : suitKey;
    const parsed = parseSingleCardToken(`${suitKey}${normRank}`, leadSuit, trumpCode);
    parsed.seatIdx = targetSeatIdx;
    parsed.seatName = seatName;
    parsed.fromUnknown = Boolean(fromUnknown);
    parsed.movedToken = rankToken;
    parsed.clickedToken = rankToken;
    activeProposalContext.proposedCards.push(parsed);
    applyProposalStateToSvg();
  }

  function tokenizeSuitRanks(cardsStr) {
    if (!cardsStr || cardsStr === '-') return [];
    const tokens = [];
    let i = 0;
    while (i < cardsStr.length) {
      if (cardsStr[i] === '1' && i + 1 < cardsStr.length && cardsStr[i + 1] === '0') {
        tokens.push('10');
        i += 2;
      } else {
        tokens.push(cardsStr[i]);
        i++;
      }
    }
    return tokens;
  }

  function rankMatchesPattern(actualRankToken, patternSuit, patternRank, actualSuit) {
    if (!patternSuit || patternSuit !== actualSuit) return false;
    const normActual = (actualRankToken === '10') ? 'T' : actualRankToken.toUpperCase();
    const normPat = (patternRank === '10') ? 'T' : patternRank;
    if (normPat === normActual) return true;
    if (normPat === 'x') {
      if (actualRankToken === 'x' || '23456789'.includes(normActual)) {
        return true;
      }
    } else if (normPat === 'X') {
      if (actualRankToken === 'X' || 'AKQJT'.includes(normActual)) {
        return true;
      }
    } else if (normPat === '_') {
      return true;
    }
    return false;
  }

  function rankOrderValue(rankToken) {
    if (rankToken === 'x') return 1;
    const norm = (rankToken === '10') ? 'T' : rankToken.toUpperCase();
    const order = {
      'A': 14, 'K': 13, 'Q': 12, 'J': 11, 'T': 10, 'X': 10,
      '9': 9, '8': 8, '7': 7, '6': 6, '5': 5, '4': 4, '3': 3, '2': 2,
      '_': 0
    };
    return order[norm] !== undefined ? order[norm] : 0;
  }

  function matchCardIndexInTokens(suitKey, rankTokens, usedMask, cardItem) {
    if (!cardItem || !cardItem.suit || cardItem.suit !== suitKey) return -1;
    const pRank = cardItem.rankStr || '';
    const normPat = (pRank === '10') ? 'T' : pRank;
    // 1. Try exact rank match on an unconsumed card
    for (let i = 0; i < rankTokens.length; i++) {
      if (usedMask[i]) continue;
      const normAct = (rankTokens[i] === '10') ? 'T' : rankTokens[i].toUpperCase();
      if (normAct === normPat) {
        return i;
      }
    }
    // 2. If pattern is wildcard ('x' or '_'), match lowest spot card / 'x' from the right
    if (normPat === 'x' || normPat === 'X' || normPat === '_') {
      for (let i = rankTokens.length - 1; i >= 0; i--) {
        if (usedMask[i]) continue;
        if (rankMatchesPattern(rankTokens[i], suitKey, normPat, suitKey)) {
          return i;
        }
      }
      // Fallback: any remaining unconsumed card in this suit
      for (let i = rankTokens.length - 1; i >= 0; i--) {
        if (usedMask[i]) continue;
        return i;
      }
    }
    // 3. If pattern is specific rank (e.g., 'K') and hand has 'x' wildcard
    for (let i = 0; i < rankTokens.length; i++) {
      if (usedMask[i]) continue;
      if (rankTokens[i] === 'x' || rankTokens[i] === 'X') {
        return i;
      }
    }
    return -1;
  }

  function buildSeatSuitTokenList(suitKey, baseRankTokens, seatPlayState, isUnknown = false) {
    if (!seatPlayState) {
      return baseRankTokens.map(tok => ({ token: tok, state: 'normal' }));
    }

    if (isUnknown) {
      // Remove cards consumed from the unknown pool by E/W
      const removedMask = new Array(baseRankTokens.length).fill(false);
      const allConsumed = [
        ...(seatPlayState.played || []),
        ...(seatPlayState.current || [])
      ];
      allConsumed.forEach(c => {
        const idx = matchCardIndexInTokens(suitKey, baseRankTokens, removedMask, c);
        if (idx !== -1) {
          removedMask[idx] = true;
        }
      });
      const remaining = [];
      for (let i = 0; i < baseRankTokens.length; i++) {
        if (!removedMask[i]) {
          remaining.push({ token: baseRankTokens[i], state: 'normal' });
        }
      }
      return remaining;
    }

    const usedMask = new Array(baseRankTokens.length).fill(false);
    const items = baseRankTokens.map(tok => ({ token: tok, state: 'normal', isLead: false }));

    const applyList = (cardList, targetState) => {
      if (!cardList) return;
      cardList.forEach(c => {
        if (!c || !c.suit || c.suit !== suitKey) return;
        const idx = matchCardIndexInTokens(suitKey, baseRankTokens, usedMask, c);
        if (idx !== -1) {
          usedMask[idx] = true;
          items[idx].state = targetState;
          if (targetState === 'current' && c.isLead) {
            items[idx].isLead = true;
          }
        } else {
          // Card came from the Unknown Cards pool into this defender's hand
          const tok = (c.movedToken || c.rankStr || 'x');
          items.push({
            token: tok,
            state: targetState,
            isLead: (targetState === 'current' && Boolean(c.isLead)),
            addedFromUnknown: true
          });
        }
      });
    };

    applyList(seatPlayState.played, 'played');
    applyList(seatPlayState.current, 'current');

    // If cards were added from Unknown Cards, sort descending by bridge rank
    if (items.some(it => it.addedFromUnknown)) {
      items.sort((a, b) => rankOrderValue(b.token) - rankOrderValue(a.token));
    }

    return items;
  }

  // Render Hand or Unknown Cards on SVG Diagram
  function renderSvgHand(groupId, handStr, isUnknown = false, seatKey = null, playabilityInfo = null) {
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

    const effectiveStates = activeTrickCardStates ||
      (!activeProposalContext ? buildInitialCurTrickCardStates() : null);
    const seatPlayState = (effectiveStates && seatKey) ? effectiveStates[seatKey] : null;
    const suitTokenLists = {};
    let hasCards = false;
    suitOrder.forEach(s => {
      const baseTokens = tokenizeSuitRanks(suitsData[s.key] || '-');
      const tokenItems = buildSeatSuitTokenList(s.key, baseTokens, seatPlayState, isUnknown);
      suitTokenLists[s.key] = tokenItems;
      if (tokenItems.length > 0) hasCards = true;
    });

    // Determine which suits are legal if this box is the active turn to play
    let legalSuits = null;
    if (playabilityInfo && playabilityInfo.isTurn) {
      const leadSuit = playabilityInfo.leadSuit;
      if (playabilityInfo.forceLeadSuit && leadSuit) {
        legalSuits = new Set([leadSuit]);
      } else if (!isUnknown && leadSuit) {
        const hasUnplayedLeadSuit = (suitTokenLists[leadSuit] || []).some(it => it.state === 'normal');
        if (hasUnplayedLeadSuit) {
          legalSuits = new Set([leadSuit]);
        } else {
          legalSuits = new Set(['S', 'H', 'D', 'C']);
        }
      } else {
        legalSuits = new Set(['S', 'H', 'D', 'C']);
      }
    }

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

    let yOffset = 0;
    const leadSpansToBox = [];
    suitOrder.forEach(s => {
      const textNode = document.createElementNS('http://www.w3.org/2000/svg', 'text');
      textNode.setAttribute('x', '0');
      textNode.setAttribute('y', yOffset + 13);
      
      const symbolSpan = document.createElementNS('http://www.w3.org/2000/svg', 'tspan');
      symbolSpan.setAttribute('class', `card-suit-label ${s.class}`);
      symbolSpan.textContent = s.symbol + ' ';
      textNode.appendChild(symbolSpan);

      const tokenItems = suitTokenLists[s.key] || [];
      if (tokenItems.length === 0) {
        const valSpan = document.createElementNS('http://www.w3.org/2000/svg', 'tspan');
        valSpan.setAttribute('class', 'card-values');
        valSpan.textContent = '-';
        textNode.appendChild(valSpan);
      } else {
        tokenItems.forEach(item => {
          const valSpan = document.createElementNS('http://www.w3.org/2000/svg', 'tspan');
          let cls = 'card-values';
          if (item.state === 'played') cls += ' card-rank-played';
          else if (item.state === 'current') cls += ' card-rank-current';
          if (item.isLead) cls += ' card-rank-lead';

          const isPlayable = Boolean(
            legalSuits &&
            legalSuits.has(s.key) &&
            item.state === 'normal'
          );
          if (isPlayable) {
            cls += ' card-rank-playable';
            valSpan.addEventListener('click', (e) => {
              e.stopPropagation();
              handleProposeCardClick(s.key, item.token, isUnknown, playabilityInfo.targetSeatIdx);
            });
          }

          valSpan.setAttribute('class', cls);
          valSpan.textContent = item.token;
          textNode.appendChild(valSpan);
          if (item.isLead) {
            leadSpansToBox.push({ span: valSpan, textNode: textNode });
          }
        });
      }

      group.appendChild(textNode);

      yOffset += 16.5;
    });

    // Draw a box around any led card in the current trick
    leadSpansToBox.forEach(({ span, textNode }) => {
      try {
        const bbox = span.getExtentOfChar(0);
        const charCount = span.getNumberOfChars();
        let width = bbox.width;
        if (charCount > 1) {
          const lastBox = span.getExtentOfChar(charCount - 1);
          width = (lastBox.x + lastBox.width) - bbox.x;
        }
        const rect = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
        rect.setAttribute('x', (bbox.x - 1.5).toFixed(1));
        rect.setAttribute('y', (bbox.y - 0.5).toFixed(1));
        rect.setAttribute('width', (width + 3).toFixed(1));
        rect.setAttribute('height', (bbox.height + 1).toFixed(1));
        rect.setAttribute('class', 'card-lead-box');
        group.insertBefore(rect, textNode);
      } catch (_) {}
    });
  }

  // Update SVG Deal Diagram from UI Inputs (or active traced hand when E/W inputs are empty)
  function updateDealSvg() {
    const eInput = inputEast ? inputEast.value : '';
    const wInput = inputWest ? inputWest.value : '';
    const useTracedEW = Boolean(
      activeTracedDeal &&
      (activeTracedDeal.E || activeTracedDeal.W) &&
      countHandCards(eInput) === 0 &&
      countHandCards(wInput) === 0
    );
    const eStr = useTracedEW ? (activeTracedDeal.E || '') : eInput;
    const wStr = useTracedEW ? (activeTracedDeal.W || '') : wInput;
    const uStr = (inputUnknownCards && !useTracedEW) ? inputUnknownCards.value.trim() : '';

    const eKnown = countHandCards(eStr) > 0;
    const wKnown = countHandCards(wStr) > 0;
    const uHasCards = countUnknownCardsString(uStr) > 0;

    let nextSeatIdx = null;
    let leadSuit = null;
    if (activeProposalContext && activeProposalContext.proposedCards.length < 4) {
      nextSeatIdx = (activeProposalContext.leaderIdx + activeProposalContext.proposedCards.length) % 4;
      if (activeProposalContext.proposedCards.length > 0) {
        leadSuit = activeProposalContext.proposedCards[0].suit;
      }
    }

    const seatHasUnplayedSuit = (handStr, seatKey, suitKey) => {
      if (!suitKey) return false;
      const suits = parseHandToSuits(handStr);
      const baseTokens = tokenizeSuitRanks(suits[suitKey] || '-');
      const st = activeTrickCardStates ? activeTrickCardStates[seatKey] : null;
      const items = buildSeatSuitTokenList(suitKey, baseTokens, st, false);
      return items.some(it => it.state === 'normal');
    };

    const eHasKnownLeadSuit = (nextSeatIdx === 1 && eKnown && leadSuit && seatHasUnplayedSuit(eStr, 'E', leadSuit));
    const wHasKnownLeadSuit = (nextSeatIdx === 3 && wKnown && leadSuit && seatHasUnplayedSuit(wStr, 'W', leadSuit));

    const nTurn = (nextSeatIdx === 0);
    const eTurnKnown = (nextSeatIdx === 1 && eKnown);
    const sTurn = (nextSeatIdx === 2);
    const wTurnKnown = (nextSeatIdx === 3 && wKnown);
    const uTurn = ((nextSeatIdx === 1 || nextSeatIdx === 3) && !useTracedEW && uHasCards);
    const forceUnknownLeadSuit = Boolean(eHasKnownLeadSuit || wHasKnownLeadSuit);

    const boxNorth = document.getElementById('boxNorth');
    const boxEast = document.getElementById('boxEast');
    const boxSouth = document.getElementById('boxSouth');
    const boxWest = document.getElementById('boxWest');
    const boxUnknown = document.getElementById('boxUnknown');
    if (boxNorth) boxNorth.classList.toggle('hand-box-active-turn', nTurn);
    if (boxEast) boxEast.classList.toggle('hand-box-active-turn', eTurnKnown);
    if (boxSouth) boxSouth.classList.toggle('hand-box-active-turn', sTurn);
    if (boxWest) boxWest.classList.toggle('hand-box-active-turn', wTurnKnown);
    if (boxUnknown) boxUnknown.classList.toggle('hand-box-active-turn', uTurn);

    renderSvgHand('cardsNorthGroup', inputNorth.value, false, 'N',
      nTurn ? { isTurn: true, leadSuit, targetSeatIdx: 0 } : null);
    renderSvgHand('cardsEastGroup', eStr, false, 'E',
      eTurnKnown ? { isTurn: true, leadSuit, targetSeatIdx: 1 } : null);
    renderSvgHand('cardsSouthGroup', inputSouth.value, false, 'S',
      sTurn ? { isTurn: true, leadSuit, targetSeatIdx: 2 } : null);
    renderSvgHand('cardsWestGroup', wStr, false, 'W',
      wTurnKnown ? { isTurn: true, leadSuit, targetSeatIdx: 3 } : null);

    if (inputUnknownCards) {
      renderSvgHand('cardsUnknownGroup', uStr, true, 'U',
        uTurn ? { isTurn: true, leadSuit, forceLeadSuit: forceUnknownLeadSuit, targetSeatIdx: nextSeatIdx } : null);
    }

    const svgTrumpText = document.getElementById('svgTrumpText');
    const svgLeaderText = document.getElementById('svgLeaderText');

    if (svgTrumpText && trumpSelect) {
      const trumpVal = trumpSelect.value;
      const suitSym = getSuitSymbol(trumpVal);
      let trumpClass = '';
      if (trumpVal === 'SPADES') trumpClass = 'card-suit-s';
      else if (trumpVal === 'HEARTS') trumpClass = 'card-suit-h';
      else if (trumpVal === 'DIAMONDS') trumpClass = 'card-suit-d';
      else if (trumpVal === 'CLUBS') trumpClass = 'card-suit-c';
      svgTrumpText.innerHTML = 'Trump: <tspan class="' + trumpClass + '">' + suitSym + '</tspan>';
    }
    if (svgLeaderText && leaderSelect) {
      svgLeaderText.textContent = 'Leader: ' + leaderSelect.value.charAt(0);
    }

    updateDealBadge();
  }

  // Sync Input Events
  [inputNorth, inputEast, inputSouth, inputWest, inputUnknownCards, trumpSelect, leaderSelect, targetTricks, numIters].forEach(el => {
    if (el) {
      el.addEventListener('input', () => {
        activeTrickCardStates = null;
        activeProposalContext = null;
        updateProposeLineBarUI();
        updateDealSvg();
      });
      el.addEventListener('change', () => {
        activeTrickCardStates = null;
        activeProposalContext = null;
        updateProposeLineBarUI();
        updateDealSvg();
      });
    }
  });

  function clearPanels(keepOriginalSnapshot = false) {
    currentTreeData = null;
    currentSolveData = null;
    currentCenteredNode = null;
    activeTrickCardStates = null;
    activeProposalContext = null;
    if (!keepOriginalSnapshot) {
      originalSolveSnapshot = null;
      proposedAltLines = [];
      hasComputedProposedLine = false;
    }
    updateProposeLineBarUI();
    activeViewMode = 'success';
    activeTracedBoardId = null;
    activeTracedDeal = null;
    activeTracedNodeSet = null;
    if (traceHandInput) {
      traceHandInput.value = '';
      traceHandInput.placeholder = '1..N';
    }
    if (clearTraceHandBtn) clearTraceHandBtn.classList.add('hidden');
    if (traceHandError) {
      traceHandError.textContent = '';
      traceHandError.classList.add('hidden');
    }
    updateDealSvg();

    if (execTime) execTime.textContent = '';

    if (resultsSummary) resultsSummary.classList.add('hidden');
    if (equivalentLinesBar) equivalentLinesBar.classList.add('hidden');
    if (equivalentLineSelect) equivalentLineSelect.innerHTML = '';
    if (statTricks) statTricks.textContent = '-';
    updateLeadStat(null);
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

    clearSampleHands('No sample hands available yet. Click "Solve Deal" to generate and inspect deals.');
    if (sampleHandsModeHeader) sampleHandsModeHeader.classList.add('hidden');
    if (sampleHandsModeBadge) {
      sampleHandsModeBadge.className = 'mode-badge mode-success';
      sampleHandsModeBadge.textContent = 'Covered Deals';
    }
    if (sampleHandsModeSub) sampleHandsModeSub.textContent = '';

    clearCompareLines();

    activateTab('tree');
  }

  function loadPreset(key) {
    const p = PRESETS[key];
    if (!p) return;

    presetCurTrick = Array.isArray(p.cur_trick) ? p.cur_trick.slice() : [];
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
    presetCurTrick = [];
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

      if (keyValues.cur_trick) {
        presetCurTrick = keyValues.cur_trick.split(/[\s,]+/).filter(Boolean);
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

    const numThreadsSelect = document.getElementById('numThreads');
    const parsedThreads = numThreadsSelect ? parseInt(numThreadsSelect.value, 10) : 16;

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
      num_iters: isNaN(parseInt(numIters.value, 10)) ? 10 : parseInt(numIters.value, 10),
      num_threads: isNaN(parsedThreads) ? 16 : parsedThreads
    };

    const initCurTrick = getActiveInitialCurTrick();
    if (initCurTrick.length > 0) {
      payload.cur_trick = initCurTrick;
    }

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

    let isSolving = true;
    const statusPollTimer = setInterval(async () => {
      if (!isSolving) return;
      try {
        const stRes = await fetch('/api/solve_status');
        if (!isSolving || !stRes.ok) return;
        const stData = await stRes.json();
        if (!isSolving) return;
        if (stData && stData.status) {
          const curSec = ((performance.now() - startTime) / 1000).toFixed(1);
          const progress = (typeof stData.progress === 'number') ? stData.progress : null;
          setStatus('running', stData.status, `${curSec} s`, progress);
        }
      } catch (_) {
        // Ignore transient polling errors while solving
      }
    }, 200);

    try {
      const response = await fetch('/api/solve', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(payload)
      });

      const data = await response.json();
      isSolving = false;
      clearInterval(statusPollTimer);
      const elapsed = (performance.now() - startTime).toFixed(1);

      if (!response.ok || !data.success) {
        setStatus('error', 'Solver failed: ' + (data.error || 'Unknown error'), `${elapsed} ms`);
        solveBtn.disabled = false;
        return;
      }

      let timeStr = `${elapsed} ms`;
      if (typeof data.solve_ms === 'number' && typeof data.invariant_ms === 'number') {
        timeStr = `solve: ${(data.solve_ms / 1000).toFixed(3)} s, invariant: ${data.invariant_ms.toFixed(1)} ms`;
      }

      const numLines = Array.isArray(data.lines) ? data.lines.length : (data.num_lines || 1);
      const completeMsg = numLines > 1
        ? `Solve complete! (${numLines} equivalent lines found)`
        : 'Solve complete!';
      setStatus('success', completeMsg, timeStr);
      execTime.textContent = timeStr;

      data.is_forced_lead = (initCurTrick.length > 0);
      currentSolveData = data;
      currentSolveData.active_line_index = 0;
      hasComputedProposedLine = false;
      proposedAltLines = [];
      activeProposalContext = null;
      originalSolveSnapshot = {
        data: JSON.parse(JSON.stringify(data)),
        inputs: {
          north: inputNorth.value,
          east: inputEast.value,
          south: inputSouth.value,
          west: inputWest.value,
          unknownCards: inputUnknownCards ? inputUnknownCards.value : '',
          trump: trumpSelect.value,
          leader: leaderSelect.value,
          targetTricks: targetTricks.value,
          numIters: numIters.value,
          curTrick: initCurTrick
        }
      };
      updateProposeLineBarUI();

      // Show summary
      resultsSummary.classList.remove('hidden');
      statTricks.textContent = data.tricks_won !== undefined ? data.tricks_won : '-';
      updateLeadStat(data);

      const total = data.total_boards || 1;
      const success = data.success_boards !== undefined ? data.success_boards : (data.covered_boards || 0);
      const failure = data.failure_boards !== undefined ? data.failure_boards : Math.max(0, total - success);
      const successPct = ((success / total) * 100).toFixed(1);
      const failurePct = ((failure / total) * 100).toFixed(1);

      if (statSuccess) statSuccess.textContent = `${success}/${total} (${successPct}%)`;
      if (statFailure) statFailure.textContent = `${failure}/${total} (${failurePct}%)`;
      if (traceHandInput) {
        traceHandInput.placeholder = `1..${total}`;
      }

      populateEquivalentLinesControls(data, 0);

      // Render the combined decision tree, then the Distribution Summary and
      // Sample Hands panels for the active view mode (Success vs Failure).
      renderSolveTree();
      updateViewMode(activeViewMode);

    } catch (err) {
      isSolving = false;
      clearInterval(statusPollTimer);
      console.error(err);
      setStatus('error', 'Network or server error: ' + err.message);
    } finally {
      isSolving = false;
      clearInterval(statusPollTimer);
      solveBtn.disabled = false;
    }
  });

  function formatSuitsToHandStr(suitsMap, isUnknownFormat = false) {
    const suitKeys = ['S', 'H', 'D', 'C'];
    if (isUnknownFormat) {
      const parts = [];
      suitKeys.forEach(s => {
        const tokens = suitsMap[s] || [];
        if (tokens.length > 0) {
          parts.push(`${s} ${tokens.join('')}`);
        }
      });
      return parts.join(' ');
    }
    return suitKeys.map(s => {
      const tokens = suitsMap[s] || [];
      return `${s} ${tokens.length > 0 ? tokens.join('') : '-'};`;
    }).join(' ');
  }

  function evaluateTrickWinner(cards, trumpCode) {
    if (!cards || cards.length === 0) return { winningIdx: 0, winnerCard: null };
    const leadSuit = cards[0].suit;
    let bestIdx = 0;
    let bestIsTrump = (trumpCode !== 'NT' && cards[0].suit === trumpCode);
    let bestVal = cards[0].rankVal;

    for (let i = 1; i < cards.length; i++) {
      const c = cards[i];
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
    return { winningIdx: bestIdx, winnerCard: cards[bestIdx] };
  }

  function getSeatFullString(seatIdx) {
    const names = ['NORTH', 'EAST', 'SOUTH', 'WEST'];
    return names[((seatIdx % 4) + 4) % 4];
  }

  function getSeatDisplayName(seatIdx) {
    const names = ['North', 'East', 'South', 'West'];
    return names[((seatIdx % 4) + 4) % 4];
  }

  function splicePrefixTricksOntoTree(prefixTricks, continuationTree, totalBoards, successBoards, failureBoards) {
    if (!prefixTricks || prefixTricks.length === 0) {
      return continuationTree;
    }

    let root = null;
    let prevNode = null;
    const allBoards = [];
    for (let i = 0; i < totalBoards; i++) allBoards.push(i);
    const boardIdStr = totalBoards > 0 ? '0' : null;

    prefixTricks.forEach((pt, idx) => {
      const trickTokens = pt.cards.map(c => {
        const r = (c.rankStr === '10') ? 'T' : c.rankStr;
        return `${c.suit || 'S'}${r}`;
      });
      const trickStr = trickTokens.join(' ');
      const winnerSeatIdx = pt.winnerSeatIdx !== undefined
        ? pt.winnerSeatIdx
        : (pt.cards[pt.winningCardIdx] ? pt.cards[pt.winningCardIdx].seatIdx : pt.leadSeatIdx);
      const winnerDisplayName = getSeatDisplayName(winnerSeatIdx);

      const node = {
        id: `prefix_trick_${pt.trickNum}`,
        type: 'action',
        trick: trickStr,
        trick_full: trickStr,
        label: `#${pt.trickNum}: ${trickStr} (${winnerDisplayName} wins)`,
        leader: pt.leadSeatIdx,
        winner: winnerSeatIdx,
        total_boards: totalBoards,
        covered_boards: successBoards,
        success_boards: successBoards,
        failure_boards: failureBoards,
        boards: allBoards,
        collapsed: false,
        proposed: true,
        children: []
      };
      if (boardIdStr !== null) {
        node.board_to_trick = { [boardIdStr]: trickStr };
      }

      if (idx > 0 && prevNode) {
        const prevWinnerIdx = prevNode.winner;
        node.branch_winner = prevWinnerIdx;
        node.condition = `when ${getSeatDisplayName(prevWinnerIdx)} wins`;
        prevNode.children.push(node);
      } else {
        root = node;
      }
      prevNode = node;
    });

    if (continuationTree && Object.keys(continuationTree).length > 0 && prevNode) {
      const lastWinnerIdx = prevNode.winner;
      const condStr = `when ${getSeatDisplayName(lastWinnerIdx)} wins`;
      const isContainerRoot = (!continuationTree.trick || continuationTree.trick === '') &&
                              Array.isArray(continuationTree.children) &&
                              continuationTree.children.length > 0;
      if (isContainerRoot) {
        continuationTree.children.forEach(ch => {
          if (!ch.condition) ch.condition = condStr;
          if (ch.branch_winner === undefined) ch.branch_winner = lastWinnerIdx;
          prevNode.children.push(ch);
        });
      } else {
        if (!continuationTree.condition) continuationTree.condition = condStr;
        if (continuationTree.branch_winner === undefined) continuationTree.branch_winner = lastWinnerIdx;
        prevNode.children.push(continuationTree);
      }
    }

    return root;
  }

  if (undoProposedCardBtn) {
    undoProposedCardBtn.addEventListener('click', () => {
      const minCards = activeProposalContext ? (activeProposalContext.initialCurCount || 0) : 0;
      if (!activeProposalContext || activeProposalContext.proposedCards.length <= minCards) return;
      activeProposalContext.proposedCards.pop();
      applyProposalStateToSvg();
    });
  }

  if (resetLineBtn) {
    resetLineBtn.addEventListener('click', () => {
      const minCards = activeProposalContext ? (activeProposalContext.initialCurCount || 0) : 0;
      if (activeProposalContext && activeProposalContext.proposedCards.length > minCards) {
        activeProposalContext.proposedCards = activeProposalContext.proposedCards.slice(0, minCards);
        applyProposalStateToSvg();
        return;
      }
      if (!originalSolveSnapshot) return;

      const snap = originalSolveSnapshot;
      if (snap.inputs) {
        inputNorth.value = snap.inputs.north;
        inputEast.value = snap.inputs.east;
        inputSouth.value = snap.inputs.south;
        inputWest.value = snap.inputs.west;
        if (inputUnknownCards) inputUnknownCards.value = snap.inputs.unknownCards;
        trumpSelect.value = snap.inputs.trump;
        leaderSelect.value = snap.inputs.leader;
        targetTricks.value = snap.inputs.targetTricks;
        numIters.value = snap.inputs.numIters;
        presetCurTrick = Array.isArray(snap.inputs.curTrick) ? snap.inputs.curTrick.slice() : [];
      }

      hasComputedProposedLine = false;
      proposedAltLines = [];
      activeProposalContext = null;
      activeTrickCardStates = null;
      clearTracedHandStateOnly();
      if (traceHandInput) traceHandInput.value = '';

      const data = JSON.parse(JSON.stringify(snap.data));
      currentSolveData = data;
      currentSolveData.active_line_index = 0;
      updateProposeLineBarUI();
      populateEquivalentLinesControls(data, 0);

      resultsSummary.classList.remove('hidden');
      statTricks.textContent = data.tricks_won !== undefined ? data.tricks_won : '-';
      updateLeadStat(data);

      const total = data.total_boards || 1;
      const success = data.success_boards !== undefined ? data.success_boards : (data.covered_boards || 0);
      const failure = data.failure_boards !== undefined ? data.failure_boards : Math.max(0, total - success);
      const successPct = ((success / total) * 100).toFixed(1);
      const failurePct = ((failure / total) * 100).toFixed(1);

      if (statSuccess) statSuccess.textContent = `${success}/${total} (${successPct}%)`;
      if (statFailure) statFailure.textContent = `${failure}/${total} (${failurePct}%)`;
      if (traceHandInput) {
        traceHandInput.placeholder = `1..${total}`;
      }

      setStatus('success', 'Restored original solved line.');
      renderSolveTree();
      updateViewMode(activeViewMode);
    });
  }

  function formatProposedCardsDescriptor(cards, trickNum) {
    if (!cards || cards.length === 0) return `Trick #${trickNum}`;
    const tokens = cards.map(c => {
      const rawTok = c.clickedToken || c.rankStr || 'x';
      const r = (rawTok === '10') ? 'T' : rawTok;
      const cardToken = `${c.suit || 'S'}${r}`;
      return c.fromUnknown ? `(${cardToken})` : cardToken;
    });
    const joined = tokens.join(' ');
    return trickNum > 1 ? `#${trickNum}: ${joined}` : joined;
  }

  if (computeNewLineBtn) {
    computeNewLineBtn.addEventListener('click', async () => {
      if (!activeProposalContext || activeProposalContext.proposedCards.length === 0) return;

      const ctx = activeProposalContext;
      const proposedCards = ctx.proposedCards.slice();
      const trumpCode = getTrumpCode();

      // Determine remaining cards in N, E, S, W, U after tricks 1..K-1
      const eInput = inputEast ? inputEast.value : '';
      const wInput = inputWest ? inputWest.value : '';
      const useTracedEW = Boolean(
        activeTracedDeal &&
        (activeTracedDeal.E || activeTracedDeal.W) &&
        countHandCards(eInput) === 0 &&
        countHandCards(wInput) === 0
      );
      const baseHands = {
        N: inputNorth.value,
        E: useTracedEW ? (activeTracedDeal.E || '') : eInput,
        S: inputSouth.value,
        W: useTracedEW ? (activeTracedDeal.W || '') : wInput,
        U: (inputUnknownCards && !useTracedEW) ? inputUnknownCards.value.trim() : ''
      };

      const remainingBeforeTrickK = { N: {}, E: {}, S: {}, W: {}, U: {} };
      ['N', 'E', 'S', 'W', 'U'].forEach(seatKey => {
        const isUnk = (seatKey === 'U');
        const suitsData = parseHandToSuits(baseHands[seatKey]);
        const playState = ctx.playedStates ? ctx.playedStates[seatKey] : null;
        ['S', 'H', 'D', 'C'].forEach(suitKey => {
          const baseTokens = tokenizeSuitRanks(suitsData[suitKey] || '-');
          const tokenItems = buildSeatSuitTokenList(suitKey, baseTokens, playState, isUnk);
          remainingBeforeTrickK[seatKey][suitKey] = tokenItems
            .filter(it => it.state === 'normal')
            .map(it => (it.token === '10' ? 'T' : it.token));
        });
      });

      const removeOneToken = (arr, rankTok) => {
        if (!arr || arr.length === 0) return '2';
        const normPat = (rankTok === '10') ? 'T' : rankTok;
        let idx = arr.findIndex(t => ((t === '10' ? 'T' : t).toUpperCase() === normPat.toUpperCase() && t !== 'x'));
        if (idx === -1 && normPat === 'x') {
          idx = arr.findIndex(t => t === 'x');
        }
        if (idx === -1) {
          idx = arr.findIndex(t => rankMatchesPattern(t, 'S', normPat, 'S'));
        }
        if (idx === -1) {
          idx = arr.length - 1;
        }
        const removed = arr.splice(idx, 1)[0];
        return (removed === '10') ? 'T' : removed;
      };

      const prefixTricksToSplice = ctx.prefixTrickRows.map(r => Object.assign({}, r));
      let nsTricksBeforeSolve = ctx.nsTricksBefore;
      let startTrickNumForSolve = ctx.trickNum;
      let leaderForSolve = getSeatFullString(ctx.leaderIdx);
      let curTrickForSolve = [];

      if (proposedCards.length === 4) {
        // Complete trick K: remove all 4 proposed cards from their hands/Unknown pool
        const completedCards = proposedCards.map(c => {
          let actualTok = c.clickedToken || c.rankStr;
          if (c.fromUnknown) {
            actualTok = removeOneToken(remainingBeforeTrickK.U[c.suit], actualTok);
          } else {
            actualTok = removeOneToken(remainingBeforeTrickK[c.seatName][c.suit], actualTok);
          }
          const resolvedRank = (actualTok === 'x') ? 'x' : actualTok.toUpperCase();
          const cardObj = parseSingleCardToken(`${c.suit}${resolvedRank}`, proposedCards[0].suit, trumpCode);
          cardObj.seatIdx = c.seatIdx;
          cardObj.seatName = c.seatName;
          return cardObj;
        });

        const { winningIdx, winnerCard } = evaluateTrickWinner(completedCards, trumpCode);
        const winnerSeatIdx = winnerCard ? winnerCard.seatIdx : ctx.leaderIdx;
        const winnerSeatName = getSeatName(winnerSeatIdx);
        if (winnerSeatName === 'N' || winnerSeatName === 'S') {
          nsTricksBeforeSolve++;
        }

        prefixTricksToSplice.push({
          trickNum: ctx.trickNum,
          leadSeatIdx: ctx.leaderIdx,
          leadSeatName: getSeatName(ctx.leaderIdx),
          cards: completedCards,
          winningCardIdx: winningIdx,
          winnerSeat: winnerSeatName,
          winnerSeatIdx: winnerSeatIdx
        });

        startTrickNumForSolve = ctx.trickNum + 1;
        leaderForSolve = getSeatFullString(winnerSeatIdx);
        curTrickForSolve = [];
      } else {
        // Partial trick K (1..3 cards): cards must remain in the hands passed to /api/solve
        // because HandleSolve plays cur_trick from the starting hands!
        // Any card clicked from Unknown Cards must be moved from U into E or W's hand.
        curTrickForSolve = proposedCards.map(c => {
          let actualTok = c.clickedToken || c.rankStr;
          if (c.fromUnknown) {
            actualTok = removeOneToken(remainingBeforeTrickK.U[c.suit], actualTok);
            remainingBeforeTrickK[c.seatName][c.suit].push(actualTok);
          }
          const r = (actualTok === '10') ? 'T' : actualTok;
          return `${c.suit}${r}`;
        });
      }

      const origTargetInput = (originalSolveSnapshot && originalSolveSnapshot.inputs)
        ? parseInt(originalSolveSnapshot.inputs.targetTricks, 10)
        : parseInt(targetTricks.value, 10);
      const fullTargetTricks = isNaN(origTargetInput) ? 1 : origTargetInput;
      const remainingMinTricks = Math.max(0, fullTargetTricks - nsTricksBeforeSolve);

      const numThreadsSelect = document.getElementById('numThreads');
      const parsedThreads = numThreadsSelect ? parseInt(numThreadsSelect.value, 10) : 16;

      const replayTricksPayload = prefixTricksToSplice.map(pt =>
        (pt.cards || []).map(c => {
          const r = (c.rankStr === '10') ? 'T' : (c.rankStr || 'x');
          return `${c.suit || 'S'}${r}`;
        })
      );

      const payload = {
        hands: {
          N: formatSuitsToHandStr(remainingBeforeTrickK.N, false),
          E: formatSuitsToHandStr(remainingBeforeTrickK.E, false),
          S: formatSuitsToHandStr(remainingBeforeTrickK.S, false),
          W: formatSuitsToHandStr(remainingBeforeTrickK.W, false)
        },
        trump: trumpSelect.value,
        leader: leaderForSolve,
        min_tricks: remainingMinTricks,
        num_iters: isNaN(parseInt(numIters.value, 10)) ? 10 : parseInt(numIters.value, 10),
        num_threads: isNaN(parsedThreads) ? 16 : parsedThreads,
        cur_trick: curTrickForSolve,
        start_trick_num: startTrickNumForSolve,
        initial_ns_tricks: nsTricksBeforeSolve,
        reuse_deals: true,
        replay_tricks: replayTricksPayload
      };

      const remUnknownStr = formatSuitsToHandStr(remainingBeforeTrickK.U, true);
      if (remUnknownStr) {
        payload.unknown_cards = remUnknownStr;
      }

      // Preserve any HCP requirements and record void suits when E or W showed
      // out (played a card of a different suit than the suit led) on a prefix
      // trick or earlier in the current trick.
      const reqs = {};
      const wHcpLowEl = document.getElementById('wHcpLow');
      const wHcpHighEl = document.getElementById('wHcpHigh');
      const wHcpLow = wHcpLowEl ? wHcpLowEl.value : '';
      const wHcpHigh = wHcpHighEl ? wHcpHighEl.value : '';
      if (wHcpLow || wHcpHigh) {
        reqs.W = {
          hcp_low: wHcpLow ? parseInt(wHcpLow, 10) : 0,
          hcp_high: wHcpHigh ? parseInt(wHcpHigh, 10) : 37
        };
      }
      const recordVoidIfShowedOut = (trickCards) => {
        if (!Array.isArray(trickCards) || trickCards.length < 2) return;
        const leadSuit = trickCards[0] && trickCards[0].suit;
        if (!leadSuit) return;
        for (let i = 1; i < trickCards.length; i++) {
          const c = trickCards[i];
          if (!c || !c.suit || c.suit === leadSuit) continue;
          if (c.seatName === 'E' || c.seatName === 'W') {
            if (!reqs[c.seatName]) reqs[c.seatName] = {};
            if (!reqs[c.seatName].suit_max) reqs[c.seatName].suit_max = {};
            reqs[c.seatName].suit_max[leadSuit] = 0;
          }
        }
      };
      prefixTricksToSplice.forEach(pt => recordVoidIfShowedOut(pt.cards));
      if (proposedCards.length > 1 && proposedCards.length < 4) {
        recordVoidIfShowedOut(proposedCards);
      }
      if (Object.keys(reqs).length > 0) {
        payload.requirements = reqs;
      }

      setStatus('running', `Computing new line from Trick #${ctx.trickNum}...`);
      computeNewLineBtn.disabled = true;
      solveBtn.disabled = true;
      const startTime = performance.now();

      let isSolving = true;
      const statusPollTimer = setInterval(async () => {
        if (!isSolving) return;
        try {
          const stRes = await fetch('/api/solve_status');
          if (!isSolving || !stRes.ok) return;
          const stData = await stRes.json();
          if (!isSolving) return;
          if (stData && stData.status) {
            const curSec = ((performance.now() - startTime) / 1000).toFixed(1);
            const progress = (typeof stData.progress === 'number') ? stData.progress : null;
            setStatus('running', stData.status, `${curSec} s`, progress);
          }
        } catch (_) {}
      }, 200);

      try {
        const response = await fetch('/api/solve', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify(payload)
        });

        const data = await response.json();
        isSolving = false;
        clearInterval(statusPollTimer);
        const elapsed = (performance.now() - startTime).toFixed(1);

        if (!response.ok || !data.success) {
          setStatus('error', 'Solver failed: ' + (data.error || 'Unknown error'), `${elapsed} ms`);
          computeNewLineBtn.disabled = false;
          solveBtn.disabled = false;
          return;
        }

        let timeStr = `${elapsed} ms`;
        if (typeof data.solve_ms === 'number' && typeof data.invariant_ms === 'number') {
          timeStr = `solve: ${(data.solve_ms / 1000).toFixed(3)} s, invariant: ${data.invariant_ms.toFixed(1)} ms`;
        }

        const total = data.total_boards || 1;
        const success = data.success_boards !== undefined ? data.success_boards : (data.covered_boards || 0);
        const failure = data.failure_boards !== undefined ? data.failure_boards : Math.max(0, total - success);

        if (prefixTricksToSplice.length > 0) {
          data.tree = splicePrefixTricksOntoTree(
            prefixTricksToSplice,
            data.tree,
            total,
            success,
            failure
          );
          if (Array.isArray(data.lines)) {
            data.lines.forEach(lineObj => {
              if (lineObj && lineObj.tree) {
                lineObj.tree = splicePrefixTricksOntoTree(
                  prefixTricksToSplice.map(r => Object.assign({}, r)),
                  lineObj.tree,
                  total,
                  success,
                  failure
                );
              }
            });
          }
          if (originalSolveSnapshot && originalSolveSnapshot.inputs) {
            data.leader = originalSolveSnapshot.inputs.leader;
          }
          if (originalSolveSnapshot && originalSolveSnapshot.data && originalSolveSnapshot.data.best_lead) {
            data.best_lead = originalSolveSnapshot.data.best_lead;
          }
        }

        const origForcedLead = Boolean(
          originalSolveSnapshot &&
          originalSolveSnapshot.inputs &&
          Array.isArray(originalSolveSnapshot.inputs.curTrick) &&
          originalSolveSnapshot.inputs.curTrick.length > 0
        );
        data.is_forced_lead = origForcedLead || (ctx.trickNum === 1 && proposedCards.length > 0);

        const altNumber = proposedAltLines.length + 1;
        const altDescriptor = formatProposedCardsDescriptor(proposedCards, ctx.trickNum);
        const altLabel = `Alt line ${altNumber}: ${altDescriptor}`;
        proposedAltLines.push({
          label: altLabel,
          data: JSON.parse(JSON.stringify(data))
        });

        hasComputedProposedLine = true;
        activeProposalContext = null;
        currentSolveData = data;
        clearTracedHandStateOnly();
        if (traceHandInput) {
          traceHandInput.value = '';
          traceHandInput.placeholder = `1..${total}`;
        }
        updateProposeLineBarUI();
        const allEntries = buildSelectableLinesList();
        const newEntryIdx = Math.max(0, allEntries.length - 1);
        populateEquivalentLinesControls(data, newEntryIdx);

        setStatus('success', `Computed ${altLabel}!`, timeStr);
        execTime.textContent = timeStr;

        resultsSummary.classList.remove('hidden');
        statTricks.textContent = data.tricks_won !== undefined ? data.tricks_won : '-';
        updateLeadStat(data);

        const successPct = ((success / total) * 100).toFixed(1);
        const failurePct = ((failure / total) * 100).toFixed(1);
        if (statSuccess) statSuccess.textContent = `${success}/${total} (${successPct}%)`;
        if (statFailure) statFailure.textContent = `${failure}/${total} (${failurePct}%)`;

        renderSolveTree();
        updateViewMode(activeViewMode);
      } catch (err) {
        isSolving = false;
        clearInterval(statusPollTimer);
        console.error(err);
        setStatus('error', 'Network or server error: ' + err.message);
      } finally {
        isSolving = false;
        clearInterval(statusPollTimer);
        solveBtn.disabled = false;
        updateProposeLineBarUI();
      }
    });
  }

  // `progress`, when a number in [0, 1], shows the estimated fraction of the
  // play-tree search finished; any other value hides the bar.
  function setStatus(type, msg, timeStr = '', progress = null) {
    statusBox.className = `status-box status-${type}`;
    statusMessage.textContent = msg;
    if (timeStr) execTime.textContent = timeStr;
    if (!statusProgress || !statusProgressFill) return;
    if (typeof progress === 'number' && progress >= 0) {
      const pct = Math.round(Math.min(1, progress) * 100);
      statusProgressFill.style.width = `${pct}%`;
      statusProgress.setAttribute('aria-valuenow', String(pct));
      statusProgress.classList.remove('hidden');
    } else {
      statusProgress.classList.add('hidden');
      statusProgressFill.style.width = '0%';
      statusProgress.setAttribute('aria-valuenow', '0');
    }
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

  function setTraceHandError(msg) {
    if (!traceHandError) return;
    if (msg) {
      traceHandError.textContent = msg;
      traceHandError.classList.remove('hidden');
    } else {
      traceHandError.textContent = '';
      traceHandError.classList.add('hidden');
    }
  }

  function updateSampleHandCardsSelection() {
    [sampleHandsGrid, compareLinesResults].forEach(container => {
      if (!container) return;
      const cards = container.querySelectorAll('.sample-deal-card');
      cards.forEach(card => {
        const bId = card.dataset.boardId !== undefined ? parseInt(card.dataset.boardId, 10) : -1;
        card.classList.toggle('traced-deal-card', activeTracedBoardId !== null && bId === activeTracedBoardId);
      });
    });
  }

  function clearTracedHandStateOnly() {
    activeTracedBoardId = null;
    activeTracedDeal = null;
    activeTracedNodeSet = null;
    if (clearTraceHandBtn) clearTraceHandBtn.classList.add('hidden');
    setTraceHandError('');
    updateSampleHandCardsSelection();
  }

  function clearTracedHand(rerender = true) {
    clearTracedHandStateOnly();
    if (traceHandInput) traceHandInput.value = '';
    if (rerender && currentTreeData) {
      expandAndInspectNode(currentTreeData, currentTreeData, null, false);
    } else {
      updateDealSvg();
    }
  }

  function findDealByBoardId(boardId) {
    if (!currentSolveData) return null;
    const lists = [
      currentSolveData.all_hands,
      currentSolveData.sample_hands,
      currentSolveData.failure_sample_hands
    ];
    for (const list of lists) {
      if (Array.isArray(list)) {
        const found = list.find(d => d && d.board_id === boardId);
        if (found) return found;
      }
    }
    return null;
  }

  function findLeafForBoard(node, boardId) {
    if (!node) return null;
    const isLeaf = !node.children || node.children.length === 0;
    const hasBoard = Array.isArray(node.boards) && node.boards.includes(boardId);
    if (isLeaf && hasBoard) {
      return node;
    }
    if (node.children && node.children.length > 0) {
      for (const child of node.children) {
        const found = findLeafForBoard(child, boardId);
        if (found) return found;
      }
    }
    if (hasBoard) {
      return node;
    }
    return null;
  }

  function traceHandByNumber(rawInput) {
    if (!currentSolveData || !currentTreeData) {
      setTraceHandError('Solve a deal first to trace a hand.');
      return false;
    }
    const cleaned = String(rawInput !== undefined && rawInput !== null ? rawInput : '').replace(/^#/, '').trim();
    const handNum = parseInt(cleaned, 10);
    const totalBoards = currentSolveData.total_boards || 1;
    if (!cleaned || isNaN(handNum) || handNum < 1 || handNum > totalBoards) {
      setTraceHandError(`Enter a hand number between 1 and ${totalBoards}.`);
      return false;
    }

    const boardId = handNum - 1;
    attachParentPointers(currentTreeData);
    const leaf = findLeafForBoard(currentTreeData, boardId);
    if (!leaf) {
      setTraceHandError(`Hand #${handNum} was not found in the decision tree.`);
      return false;
    }

    const pathToLeaf = [];
    let curr = leaf;
    while (curr) {
      pathToLeaf.unshift(curr);
      curr = curr._parent;
    }

    for (let i = 0; i < pathToLeaf.length; i++) {
      const current = pathToLeaf[i];
      current._collapsed = false;
      if (current.children && current.children.length > 0) {
        const nextInPath = pathToLeaf[i + 1];
        current.children.forEach(child => {
          if (child !== nextInPath) {
            collapseSubtree(child);
          }
        });
      }
    }

    activeTracedBoardId = boardId;
    activeTracedDeal = findDealByBoardId(boardId);
    activeTracedNodeSet = new Set(pathToLeaf);
    currentCenteredNode = leaf;

    if (traceHandInput) traceHandInput.value = String(handNum);
    if (clearTraceHandBtn) clearTraceHandBtn.classList.remove('hidden');
    setTraceHandError('');
    updateSampleHandCardsSelection();

    activateTab('tree');
    if (treeInspectorPanel) {
      treeInspectorPanel.classList.remove('collapsed');
    }

    renderDecisionTree(currentTreeData, false);

    // Center the branch in the tree viewport
    const midNode = pathToLeaf[Math.floor(pathToLeaf.length / 2)] || leaf;
    centerNodeInTree(midNode, null, false);
    currentCenteredNode = leaf;
    updateCenteredNodeHighlight();

    showNodeInspector(leaf, leaf);
    return true;
  }

  if (traceHandBtn) {
    traceHandBtn.addEventListener('click', () => {
      traceHandByNumber(traceHandInput ? traceHandInput.value : '');
    });
  }

  if (traceHandInput) {
    traceHandInput.addEventListener('keydown', (e) => {
      if (e.key === 'Enter') {
        e.preventDefault();
        traceHandByNumber(traceHandInput.value);
      }
    });
  }

  if (clearTraceHandBtn) {
    clearTraceHandBtn.addEventListener('click', () => {
      clearTracedHand(true);
    });
  }

  function expandAndInspectNode(targetNode, rootNode, clickEvent = null, switchTab = false) {
    if (!rootNode || !targetNode) return;
    attachParentPointers(rootNode);

    if (activeTracedNodeSet && !activeTracedNodeSet.has(targetNode)) {
      clearTracedHandStateOnly();
    }

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

    // Expand down from targetNode to (one) leaf, following the traced branch if active
    // or the child branch with the highest number of deals (preferring non-failure branches).
    let leaf = targetNode;
    while (leaf.children && leaf.children.length > 0) {
      leaf._collapsed = false;
      let chosenIdx = -1;
      if (activeTracedNodeSet) {
        const tracedIdx = leaf.children.findIndex(c => activeTracedNodeSet.has(c));
        if (tracedIdx !== -1) {
          chosenIdx = tracedIdx;
        }
      }
      if (chosenIdx === -1) {
        let bestIdx = 0;
        let bestIsSuccess = !leaf.children[0].failure;
        let bestDeals = (leaf.children[0].success_boards || 0) + (leaf.children[0].failure_boards || 0) ||
                        (Array.isArray(leaf.children[0].boards) ? leaf.children[0].boards.length : 0);
        for (let i = 1; i < leaf.children.length; i++) {
          const ch = leaf.children[i];
          const isSuccess = !ch.failure;
          const deals = (ch.success_boards || 0) + (ch.failure_boards || 0) ||
                        (Array.isArray(ch.boards) ? ch.boards.length : 0);
          if ((isSuccess && !bestIsSuccess) ||
              (isSuccess === bestIsSuccess && deals > bestDeals)) {
            bestIdx = i;
            bestIsSuccess = isSuccess;
            bestDeals = deals;
          }
        }
        chosenIdx = bestIdx;
      }
      const chosenChild = leaf.children[chosenIdx];
      for (let i = 0; i < leaf.children.length; i++) {
        if (i !== chosenIdx) {
          collapseSubtree(leaf.children[i]);
        }
      }
      leaf = chosenChild;
    }
    leaf._collapsed = false;

    currentCenteredNode = targetNode;

    // Re-render decision tree layout
    renderDecisionTree(rootNode);

    // Center targetNode under mouse (or in viewport if no mouse click inside treeViewport)
    centerNodeInTree(targetNode, clickEvent, switchTab);

    // Show line inspector for the reached leaf and select the clicked node's trick if applicable
    showNodeInspector(leaf, targetNode);
  }

  function centerNodeInTree(node, clickEvent = null, switchTab = false) {
    if (!node) return;
    currentCenteredNode = node;

    if (switchTab) {
      activateTab('tree');
    }

    let p = node._parent;
    let needRerender = false;
    while (p) {
      if (p._collapsed) {
        p._collapsed = false;
        needRerender = true;
      }
      p = p._parent;
    }

    const root = currentTreeData || getRoot(node);
    if (needRerender || node._x === undefined || node._y === undefined) {
      if (root) renderDecisionTree(root);
    } else {
      updateCenteredNodeHighlight();
    }

    if (node._x === undefined || node._y === undefined) return;

    const nodeWidth = 164;
    const nodeHeight = 78;
    const nodeCenterX = node._x + nodeWidth / 2;
    const nodeCenterY = node._y + nodeHeight / 2;

    const viewportWidth = treeViewport ? (treeViewport.clientWidth || treeViewport.offsetWidth || 800) : 800;
    const viewportHeight = treeViewport ? (treeViewport.clientHeight || treeViewport.offsetHeight || 600) : 600;

    let targetX = viewportWidth / 2;
    let targetY = viewportHeight / 2;

    if (clickEvent && treeViewport && typeof clickEvent.clientX === 'number' && typeof clickEvent.clientY === 'number') {
      const rect = treeViewport.getBoundingClientRect();
      if (rect &&
          clickEvent.clientX >= rect.left && clickEvent.clientX <= rect.right &&
          clickEvent.clientY >= rect.top && clickEvent.clientY <= rect.bottom) {
        targetX = clickEvent.clientX - rect.left;
        targetY = clickEvent.clientY - rect.top;
      }
    }

    panX = targetX - (nodeCenterX * zoomLevel);
    panY = targetY - (nodeCenterY * zoomLevel);

    updateTransform();
  }

  function getRoot(node) {
    let curr = node;
    while (curr && curr._parent) {
      curr = curr._parent;
    }
    return curr;
  }

  function updateCenteredNodeHighlight() {
    const allCards = treeRootGroup ? treeRootGroup.querySelectorAll('.node-card') : [];
    allCards.forEach(c => c.classList.remove('centered-node'));
    if (currentCenteredNode && currentCenteredNode._svgRect) {
      currentCenteredNode._svgRect.classList.add('centered-node');
    }
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
    const nodeWidth = 164;
    const nodeHeight = 78;
    const levelHeight = 140;
    const siblingGap = 24;

    let leafCounter = 0;

    function layoutNode(node, depth = 0, y = 0) {
      node._depth = depth;
      node._y = y;
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
        const gap = (node.children.length === 1) ? 31 : 62;
        const nextY = y + nodeHeight + gap;
        node.children.forEach(child => layoutNode(child, depth + 1, nextY));
        const firstChild = node.children[0];
        const lastChild = node.children[node.children.length - 1];
        node._x = (firstChild._x + lastChild._x) / 2;
      }
    }

    layoutNode(rootNode);

    // Pass 1: Draw Links & Edge Badges
    function drawLinks(node) {
      if (node.children && node.children.length > 0 && !node._collapsed) {
        node.children.forEach(child => {
          const isTracedEdge = Boolean(
            activeTracedNodeSet &&
            activeTracedNodeSet.has(node) &&
            activeTracedNodeSet.has(child)
          );
          const path = document.createElementNS('http://www.w3.org/2000/svg', 'path');
          const startX = node._x + nodeWidth / 2;
          const startY = node._y + nodeHeight;
          const endX = child._x + nodeWidth / 2;
          const endY = child._y;
          const midY = (startY + endY) / 2;

          const d = `M ${startX} ${startY} C ${startX} ${midY}, ${endX} ${midY}, ${endX} ${endY}`;
          path.setAttribute('d', d);
          let linkClass = child.failure ? 'tree-link tree-link-failure' : 'tree-link';
          if (isTracedEdge) {
            linkClass += ' traced-branch-link';
          }
          path.setAttribute('class', linkClass);
          path.setAttribute(
            'marker-end',
            isTracedEdge ? 'url(#arrow-traced)' : (child.failure ? 'url(#arrow-failure)' : 'url(#arrow)')
          );
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
            let bgClass = child.failure ? 'tree-edge-label-bg tree-edge-label-bg-failure' : 'tree-edge-label-bg';
            let txtClass = child.failure ? 'tree-edge-label tree-edge-label-failure' : 'tree-edge-label';
            if (isTracedEdge) {
              bgClass += ' tree-edge-label-bg-traced';
              txtClass += ' tree-edge-label-traced';
            }
            rect.setAttribute('class', bgClass);

            const text = document.createElementNS('http://www.w3.org/2000/svg', 'text');
            text.setAttribute('x', midX);
            text.setAttribute('y', midY + 4);
            text.setAttribute('text-anchor', 'middle');
            text.setAttribute('class', txtClass);
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
      if (node.type === 'outcome') {
        cardClass = node.failure ? 'node-outcome-failure' : 'node-outcome';
      }
      if (activeTracedNodeSet && activeTracedNodeSet.has(node)) {
        cardClass += ' traced-branch-node';
      }
      if (node === currentCenteredNode) cardClass += ' centered-node';
      rect.setAttribute('class', `node-card ${cardClass}`);
      node._svgRect = rect;

      g.appendChild(rect);

      // Title Text
      const title = document.createElementNS('http://www.w3.org/2000/svg', 'text');
      title.setAttribute('x', '8');
      title.setAttribute('y', '22');
      title.setAttribute('class', 'node-text-title');
      let rawTitle = node.label || node.trick || 'Node';
      if (rawTitle.startsWith('after ')) {
        rawTitle = rawTitle.substring(6);
      }
      title.textContent = truncateText(rawTitle, 28);
      g.appendChild(title);

      // Detail Text (Line 2 and optional Line 3)
      const detail = document.createElementNS('http://www.w3.org/2000/svg', 'text');
      detail.setAttribute('x', '8');
      detail.setAttribute('y', '42');
      detail.setAttribute('class', 'node-text-detail');
      const { primary: detailText, secondary: failLineText } = formatNodeCardDealLines(node);
      const typeLabel = (node.type === 'condition') ? 'BRANCH' : (node.type || 'action').toUpperCase();
      if (typeLabel === 'ACTION' || typeLabel === 'OUTCOME') {
        detail.textContent = detailText;
      } else {
        detail.textContent = detailText ? `${typeLabel} | ${detailText}` : typeLabel;
      }
      g.appendChild(detail);

      // 3rd Line: N/S Won on outcome nodes, or Fails count on non-outcome nodes
      if (node.type === 'outcome' && node.ns_tricks !== undefined) {
        const highlight = document.createElementNS('http://www.w3.org/2000/svg', 'text');
        highlight.setAttribute('x', '8');
        highlight.setAttribute('y', '62');
        highlight.setAttribute('class', 'node-text-highlight');
        if (node.failure) {
          highlight.setAttribute('fill', '#fca5a5');
        }
        highlight.textContent = `N/S Won: ${node.ns_tricks}`;
        g.appendChild(highlight);
      } else if (failLineText) {
        const failDetail = document.createElementNS('http://www.w3.org/2000/svg', 'text');
        failDetail.setAttribute('x', '8');
        failDetail.setAttribute('y', '62');
        failDetail.setAttribute('class', 'node-text-detail');
        failDetail.textContent = failLineText;
        g.appendChild(failDetail);
      }


      // Event Listeners for Interactivity
      g.addEventListener('click', (e) => {
        e.stopPropagation();
        expandAndInspectNode(node, rootNode, e);
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

  function formatNodeCardDealLines(node) {
    const total = (node.total_boards !== undefined && node.total_boards > 0) ? node.total_boards : 0;
    const withPct = (n) => total ? `${n} (${((n / total) * 100).toFixed(1)}%)` : `${n}`;
    if (node.failure) {
      const fails = node.failure_boards !== undefined ? node.failure_boards : (node.covered_boards || 0);
      return { primary: `Fails: ${withPct(fails)}`, secondary: '' };
    }
    const wins = node.success_boards !== undefined ? node.success_boards : node.covered_boards;
    if (wins === undefined) return { primary: '', secondary: '' };
    const fails = node.failure_boards || 0;
    if (fails > 0 && node.type !== 'outcome') {
      return {
        primary: `Makes: ${withPct(wins)}`,
        secondary: `Fails: ${withPct(fails)}`
      };
    }
    return { primary: `Deals: ${withPct(wins)}`, secondary: '' };
  }

  // Deal counts for a node card / inspector. A failure leaf reports the deals
  // that fail there; any other node reports the deals it makes and, when some
  // of the deals reaching it go on to fail, that count as well.
  function formatNodeDeals(node) {
    const { primary, secondary } = formatNodeCardDealLines(node);
    return secondary ? `${primary} · ${secondary}` : primary;
  }

  function showNodeInspector(node, clickedNode = null) {
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

    const isTracingBoard = (activeTracedBoardId !== null &&
      (!node || !Array.isArray(node.boards) || node.boards.length === 0 || node.boards.includes(activeTracedBoardId)));
    const tracedBoardKey = isTracingBoard ? String(activeTracedBoardId) : null;

    // Determine initial leader
    let initialLeader = getSeatIndex(leaderSelect ? leaderSelect.value : 'NORTH');
    if (currentSolveData && currentSolveData.leader) {
      initialLeader = getSeatIndex(currentSolveData.leader);
    }
    const trumpCode = getTrumpCode();

    let currentLeader = initialLeader;
    const trickRows = [];

    path.forEach((pathNode, stepIdx) => {
      let rawTrick = extractTrickString(pathNode);
      if (!rawTrick) return;

      let usedConcreteBoardTrick = false;
      if (tracedBoardKey !== null && pathNode.board_to_trick && pathNode.board_to_trick[tracedBoardKey]) {
        rawTrick = pathNode.board_to_trick[tracedBoardKey];
        usedConcreteBoardTrick = true;
      }

      const tokens = tokenizeTrick(rawTrick);
      if (tokens.length === 0) return;

      if (typeof pathNode.leader === 'number' && pathNode.leader >= 0 && pathNode.leader <= 3) {
        currentLeader = pathNode.leader;
      }

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

      // Check explicit winner and branch card from next node or pathNode
      let explicitWinnerSeat = null;
      let explicitWinnerCard = null;
      let explicitPlayedSeat = null;
      let explicitPlayedCard = null;

      if (!usedConcreteBoardTrick) {
        // 1. Next node in path specifies the selected branch rule (branch_winner / condition)
        if (stepIdx + 1 < path.length) {
          const nextNode = path[stepIdx + 1];
          if (nextNode) {
            if (typeof nextNode.branch_winner === 'number' &&
                nextNode.branch_winner >= 0 && nextNode.branch_winner <= 3) {
              explicitWinnerSeat = nextNode.branch_winner;
            }
            if (nextNode.condition) {
              const cond = nextNode.condition;
              const mWin = cond.match(/(?:when|if)\s+([0-3]|north|east|south|west|n|e|s|w)\s+wins(?:\s+(?:the\s+)?([A-Z0-9_,\(\)\s]+))?/i);
              if (mWin) {
                if (explicitWinnerSeat === null) {
                  explicitWinnerSeat = getSeatIndex(mWin[1]);
                }
                if (mWin[2]) {
                  const cardList = mWin[2].trim().split(/[\s,]+/).filter(Boolean);
                  if (cardList.length === 1) {
                    explicitWinnerCard = cardList[0].trim();
                  }
                }
              }
              const mPlay = cond.match(/(?:when|if)\s+([0-3]|north|east|south|west|n|e|s|w)\s+plays\s+([A-Z0-9_]+)/i);
              if (mPlay) {
                explicitPlayedSeat = getSeatIndex(mPlay[1]);
                explicitPlayedCard = mPlay[2].trim();
              } else {
                const mDiscard = cond.match(/(?:when|if)\s+([0-3]|north|east|south|west|n|e|s|w)\s+discards/i);
                if (mDiscard) {
                  explicitPlayedSeat = getSeatIndex(mDiscard[1]);
                  explicitPlayedCard = 'R_';
                }
              }
            }
          }
        }

        // 2. Use pathNode.winner (populated directly from C++ WinnerPlayCondition / BoardState)
        if (explicitWinnerSeat === null && typeof pathNode.winner === 'number' &&
            pathNode.winner >= 0 && pathNode.winner <= 3) {
          explicitWinnerSeat = pathNode.winner;
        }

        // 3. Fall back to pathNode.label if needed
        if (explicitWinnerSeat === null && pathNode.label) {
          const mWin = pathNode.label.match(/\((North|East|South|West|N|E|S|W)\s+wins/i);
          if (mWin) {
            explicitWinnerSeat = getSeatIndex(mWin[1]);
          }
        }

        if (explicitPlayedSeat !== null && explicitPlayedCard) {
          const playedIdx = parsedCards.findIndex(c => c.seatIdx === explicitPlayedSeat);
          if (playedIdx !== -1) {
            const updatedCard = parseSingleCardToken(explicitPlayedCard, leadSuit, trumpCode);
            updatedCard.seatIdx = explicitPlayedSeat;
            updatedCard.seatName = getSeatName(explicitPlayedSeat);
            parsedCards[playedIdx] = updatedCard;
          }
        }
      }

      // Determine winning card index and update card token if specific winner card is provided
      let winningCardIdx = -1;

      if (!usedConcreteBoardTrick && explicitWinnerSeat !== null) {
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

      if (!usedConcreteBoardTrick && winningCardIdx === -1 && explicitWinnerCard !== null) {
        const foundIdx = parsedCards.findIndex(c => c.raw.toUpperCase().includes(explicitWinnerCard.toUpperCase()));
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
        leadSeatIdx: currentLeader,
        leadSeatBadge: leadSeatBadge,
        leadSeatName: leadSeatName,
        cards: parsedCards,
        winningCardIdx: winningCardIdx,
        winnerSeat: winnerCard ? winnerCard.seatName : '',
        node: pathNode
      });

      // Next trick leader is the winner of this trick
      currentLeader = winnerCard.seatIdx;
    });

    // Render Inspector Panel HTML
    inspectorContent.innerHTML = '';

    // Traced Hand Banner (when tracing a specific board)
    if (isTracingBoard) {
      const handNum = activeTracedBoardId + 1;
      const isFail = Boolean(
        (activeTracedDeal && activeTracedDeal.succeeded === false) ||
        (node && node.failure)
      );
      const bannerEl = document.createElement('div');
      bannerEl.className = `traced-hand-banner${isFail ? ' traced-hand-failure' : ''}`;
      let holdingsHtml = '';
      if (activeTracedDeal) {
        const formatCompactHand = (hStr) => {
          const suits = parseHandToSuits(hStr || '');
          return [
            `<span class="card-suit-s">♠</span>${suits.S || '-'}`,
            `<span class="card-suit-h">♥</span>${suits.H || '-'}`,
            `<span class="card-suit-d">♦</span>${suits.D || '-'}`,
            `<span class="card-suit-c">♣</span>${suits.C || '-'}`
          ].join(' ');
        };
        holdingsHtml = `
          <div class="traced-hand-holdings">
            <div><strong>W:</strong> ${formatCompactHand(activeTracedDeal.W)}</div>
            <div><strong>E:</strong> ${formatCompactHand(activeTracedDeal.E)}</div>
          </div>
        `;
      }
      bannerEl.innerHTML = `
        <div class="traced-hand-banner-header">
          <span class="traced-hand-title">Traced Hand #${handNum}</span>
          <span class="traced-hand-badge ${isFail ? 'badge-fails' : 'badge-makes'}">${isFail ? 'FAILS' : 'MAKES'}</span>
        </div>
        ${holdingsHtml}
      `;
      inspectorContent.appendChild(bannerEl);
    }

    // Helper to update Deal SVG card states for a selected trick index
    function selectTrickInInspector(selectedIdx, rowElements, updateTreeHighlight = true) {
      if (selectedIdx === null || selectedIdx < 0 || selectedIdx >= trickRows.length) {
        activeTrickCardStates = null;
        activeProposalContext = null;
        updateProposeLineBarUI();
        if (updateTreeHighlight) {
          currentCenteredNode = null;
          updateCenteredNodeHighlight();
        }
        if (rowElements) {
          rowElements.forEach(el => {
            el.classList.remove('trick-row-selected', 'trick-row-past');
          });
        }
        updateDealSvg();
        return;
      }

      if (updateTreeHighlight && trickRows[selectedIdx].node) {
        currentCenteredNode = trickRows[selectedIdx].node;
        updateCenteredNodeHighlight();
      }

      const states = {
        N: { played: [], current: [] },
        E: { played: [], current: [] },
        S: { played: [], current: [] },
        W: { played: [], current: [] },
        U: { played: [], current: [] }
      };

      const hasTracedEW = Boolean(isTracingBoard && activeTracedDeal && (activeTracedDeal.E || activeTracedDeal.W));
      const eKnownSuits = parseHandToSuits(hasTracedEW ? (activeTracedDeal.E || '') : (inputEast ? inputEast.value : ''));
      const wKnownSuits = parseHandToSuits(hasTracedEW ? (activeTracedDeal.W || '') : (inputWest ? inputWest.value : ''));
      const eSuitTokens = {
        S: tokenizeSuitRanks(eKnownSuits.S || '-'),
        H: tokenizeSuitRanks(eKnownSuits.H || '-'),
        D: tokenizeSuitRanks(eKnownSuits.D || '-'),
        C: tokenizeSuitRanks(eKnownSuits.C || '-')
      };
      const eUsedMasks = {
        S: new Array(eSuitTokens.S.length).fill(false),
        H: new Array(eSuitTokens.H.length).fill(false),
        D: new Array(eSuitTokens.D.length).fill(false),
        C: new Array(eSuitTokens.C.length).fill(false)
      };
      const wSuitTokens = {
        S: tokenizeSuitRanks(wKnownSuits.S || '-'),
        H: tokenizeSuitRanks(wKnownSuits.H || '-'),
        D: tokenizeSuitRanks(wKnownSuits.D || '-'),
        C: tokenizeSuitRanks(wKnownSuits.C || '-')
      };
      const wUsedMasks = {
        S: new Array(wSuitTokens.S.length).fill(false),
        H: new Array(wSuitTokens.H.length).fill(false),
        D: new Array(wSuitTokens.D.length).fill(false),
        C: new Array(wSuitTokens.C.length).fill(false)
      };
      const uStr = inputUnknownCards ? inputUnknownCards.value.trim() : '';
      const uHasCards = !hasTracedEW && countUnknownCardsString(uStr) > 0;
      const uSuits = parseHandToSuits(uStr);
      const uSuitTokens = {
        S: tokenizeSuitRanks(uSuits.S || '-'),
        H: tokenizeSuitRanks(uSuits.H || '-'),
        D: tokenizeSuitRanks(uSuits.D || '-'),
        C: tokenizeSuitRanks(uSuits.C || '-')
      };
      const uUsedMasks = {
        S: new Array(uSuitTokens.S.length).fill(false),
        H: new Array(uSuitTokens.H.length).fill(false),
        D: new Array(uSuitTokens.D.length).fill(false),
        C: new Array(uSuitTokens.C.length).fill(false)
      };

      // Pick the traced board ID (if active) or a representative board covered by this inspected line
      let repBoardId = tracedBoardKey;
      if (repBoardId === null && node && Array.isArray(node.boards) && node.boards.length > 0) {
        repBoardId = String(node.boards[0]);
      } else if (repBoardId === null && trickRows[selectedIdx].node &&
                 Array.isArray(trickRows[selectedIdx].node.boards) &&
                 trickRows[selectedIdx].node.boards.length > 0) {
        repBoardId = String(trickRows[selectedIdx].node.boards[0]);
      }

      const resolveActualCardsForTrick = (tr) => {
        if (!tr || !tr.node) return tr.cards;
        let rawBoardTrick = null;
        if (repBoardId !== null && tr.node.board_to_trick && tr.node.board_to_trick[repBoardId]) {
          rawBoardTrick = tr.node.board_to_trick[repBoardId];
        } else if (tr.node.board_to_trick) {
          const keys = Object.keys(tr.node.board_to_trick);
          if (keys.length > 0) {
            rawBoardTrick = tr.node.board_to_trick[keys[0]];
          }
        } else if (tr.node.trick_full && tr.node.trick_full.includes('/')) {
          const slashParts = tr.node.trick_full.split('/');
          if (slashParts.length > 1) {
            rawBoardTrick = slashParts[1].trim();
          }
        }
        if (!rawBoardTrick) return tr.cards;
        const rawToks = tokenizeTrick(rawBoardTrick);
        if (rawToks.length !== tr.cards.length) return tr.cards;
        let leadSuit = null;
        return tr.cards.map((origCard, idx) => {
          const actualParsed = parseSingleCardToken(rawToks[idx], leadSuit, trumpCode);
          if (idx === 0) leadSuit = actualParsed.suit;
          // If the generalized card was unsuited ('__') or wildcard ('x'/'_'), use the concrete card from the board
          if (!origCard.suit || origCard.rankStr === '__' || origCard.rankStr === '_' ||
              origCard.rankStr === 'x' || origCard.rankStr === 'X') {
            return Object.assign({}, origCard, {
              suit: actualParsed.suit,
              rankStr: actualParsed.rankStr,
              rankVal: actualParsed.rankVal,
              suitSym: actualParsed.suitSym,
              suitClass: actualParsed.suitClass,
              displayStr: actualParsed.displayStr
            });
          }
          return origCard;
        });
      };

      const recordCard = (cardObj, bucket, isLead = false) => {
        if (!cardObj) return;
        const seat = cardObj.seatName;
        const entry = Object.assign({}, cardObj, { isLead });
        if ((seat === 'E' || seat === 'W') && uHasCards) {
          const knownTokens = (seat === 'E') ? eSuitTokens : wSuitTokens;
          const knownMasks = (seat === 'E') ? eUsedMasks : wUsedMasks;
          let matchedKnown = false;
          if (entry.suit && knownTokens[entry.suit]) {
            const kIdx = matchCardIndexInTokens(entry.suit, knownTokens[entry.suit], knownMasks[entry.suit], entry);
            if (kIdx !== -1) {
              knownMasks[entry.suit][kIdx] = true;
              matchedKnown = true;
            }
          }
          if (!matchedKnown) {
            let sKey = entry.suit;
            // Fallback if suit is still null (e.g. '__' without board_to_trick): pick first suit in Unknown Cards with remaining cards
            if (!sKey) {
              for (const candidateSuit of ['S', 'H', 'D', 'C']) {
                const remIdx = uSuitTokens[candidateSuit].findIndex((_, idx) => !uUsedMasks[candidateSuit][idx]);
                if (remIdx !== -1) {
                  sKey = candidateSuit;
                  entry.suit = candidateSuit;
                  break;
                }
              }
            }
            if (sKey && uSuitTokens[sKey]) {
              const uIdx = matchCardIndexInTokens(sKey, uSuitTokens[sKey], uUsedMasks[sKey], entry);
              if (uIdx !== -1) {
                uUsedMasks[sKey][uIdx] = true;
                const matchedTok = uSuitTokens[sKey][uIdx];
                const isPatWild = (entry.rankStr === 'x' || entry.rankStr === 'X' ||
                                   entry.rankStr === '_' || entry.rankStr === '__');
                entry.movedToken = isPatWild ? matchedTok : entry.rankStr;
              }
            }
            states.U[bucket].push(entry);
          }
        }
        if (seat && states[seat] && entry.suit) {
          states[seat][bucket].push(entry);
        }
      };

      let nsTricksBefore = 0;
      const prefixTrickRows = [];
      for (let i = 0; i < selectedIdx; i++) {
        const tr = trickRows[i];
        const resolvedCards = resolveActualCardsForTrick(tr);
        resolvedCards.forEach(c => recordCard(c, 'played', false));
        if (tr.winnerSeat === 'N' || tr.winnerSeat === 'S') {
          nsTricksBefore++;
        }
        prefixTrickRows.push({
          trickNum: tr.trickNum,
          leadSeatIdx: tr.leadSeatIdx,
          leadSeatName: tr.leadSeatName,
          cards: resolvedCards.map(c => Object.assign({}, c)),
          winningCardIdx: tr.winningCardIdx,
          winnerSeat: tr.winnerSeat,
          winnerSeatIdx: (tr.cards[tr.winningCardIdx] ? tr.cards[tr.winningCardIdx].seatIdx : tr.leadSeatIdx)
        });
      }

      const selectedTrickRow = trickRows[selectedIdx];
      const initCur = (selectedIdx === 0 && selectedTrickRow.trickNum === 1)
        ? getActiveInitialCurTrick()
        : [];
      const initialProposedCards = [];
      if (initCur.length > 0) {
        let leadSuit = null;
        initCur.forEach((cStr, idx) => {
          const seatIdx = (selectedTrickRow.leadSeatIdx + idx) % 4;
          const seatName = getSeatName(seatIdx);
          const parsed = parseSingleCardToken(cStr, leadSuit, trumpCode);
          if (idx === 0) leadSuit = parsed.suit;
          parsed.seatIdx = seatIdx;
          parsed.seatName = seatName;
          parsed.fromUnknown = false;
          parsed.movedToken = parsed.rankStr;
          parsed.clickedToken = parsed.rankStr;
          initialProposedCards.push(parsed);
        });
      }
      activeProposalContext = {
        trickNum: selectedTrickRow.trickNum,
        selectedIdx: selectedIdx,
        leaderIdx: selectedTrickRow.leadSeatIdx,
        nsTricksBefore: nsTricksBefore,
        playedStates: clonePlayStates(states),
        prefixTrickRows: prefixTrickRows,
        initialCurCount: initialProposedCards.length,
        proposedCards: initialProposedCards.slice()
      };

      if (rowElements) {
        rowElements.forEach((el, idx) => {
          el.classList.toggle('trick-row-selected', idx === selectedIdx);
          el.classList.toggle('trick-row-past', idx < selectedIdx);
        });
      }
      applyProposalStateToSvg();
    }

    // Reset card highlights when switching inspected node until a trick is clicked
    activeTrickCardStates = null;
    activeProposalContext = null;
    updateProposeLineBarUI();
    updateDealSvg();

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
      const rowElements = [];
      let selectedTrickIdx = null;

      // Column headers: N, E, S, W
      const headerRow = document.createElement('div');
      headerRow.className = 'trick-columns-header';
      headerRow.innerHTML = `
        <div class="trick-row-meta"></div>
        <div class="card-chips-row">
          <span class="trick-seat-col-header seat-col-ns">N</span>
          <span class="trick-seat-col-header seat-col-ew">E</span>
          <span class="trick-seat-col-header seat-col-ns">S</span>
          <span class="trick-seat-col-header seat-col-ew">W</span>
        </div>
      `;
      listContainer.appendChild(headerRow);

      trickRows.forEach((tr, rowIdx) => {
        const rowEl = document.createElement('div');
        rowEl.className = 'trick-row';
        rowEl.title = `Click to highlight Trick ${tr.trickNum} cards on deal diagram`;
        rowElements.push(rowEl);

        rowEl.addEventListener('click', () => {
          if (selectedTrickIdx === rowIdx) {
            selectedTrickIdx = null;
            selectTrickInInspector(null, rowElements);
          } else {
            selectedTrickIdx = rowIdx;
            selectTrickInInspector(rowIdx, rowElements);
            if (tr.node) {
              centerNodeInTree(tr.node);
            }
          }
        });

        const metaEl = document.createElement('div');
        metaEl.className = 'trick-row-meta';

        const labelEl = document.createElement('span');
        labelEl.className = 'trick-label clickable';
        labelEl.textContent = `#${tr.trickNum}`;
        labelEl.title = `Click to highlight Trick ${tr.trickNum} and center in decision tree`;

        labelEl.addEventListener('click', (e) => {
          e.stopPropagation();
          selectedTrickIdx = rowIdx;
          selectTrickInInspector(rowIdx, rowElements);
          if (tr.node) {
            centerNodeInTree(tr.node);
          }
        });

        metaEl.appendChild(labelEl);
        rowEl.appendChild(metaEl);

        const chipsRow = document.createElement('div');
        chipsRow.className = 'card-chips-row';

        const isWinnerEW = (tr.winnerSeat === 'E' || tr.winnerSeat === 'W');
        const winnerThemeClass = isWinnerEW ? 'card-winner-ew' : 'card-winner-ns';

        // Always display cards in fixed N (0), E (1), S (2), W (3) order
        [0, 1, 2, 3].forEach((seatIdx) => {
          const playIdx = tr.cards.findIndex(card => card.seatIdx === seatIdx);
          if (playIdx === -1) return;
          const c = tr.cards[playIdx];
          const chip = document.createElement('span');
          const isWinner = (playIdx === tr.winningCardIdx);
          const isLead = (seatIdx === tr.leadSeatIdx);
          const classes = ['card-chip', c.suitClass];
          if (isWinner) {
            classes.push('card-winner', winnerThemeClass);
          }
          if (isLead) {
            classes.push('card-lead');
          }
          chip.className = classes.join(' ');
          chip.title = `${isLead ? 'Led' : 'Played'} by ${c.seatName}${isWinner ? ' (Winner)' : ''}`;
          const innerHtml = c.suitSym
            ? `<span class="suit-symbol">${c.suitSym}</span><span class="card-rank">${c.rankStr}</span>`
            : `<span class="card-rank">${c.rankStr}</span>`;
          chip.innerHTML = `<span class="card-chip-content">${innerHtml}</span>`;
          chipsRow.appendChild(chip);
        });

        rowEl.appendChild(chipsRow);
        listContainer.appendChild(rowEl);
      });

      trickSection.appendChild(listContainer);

      if (clickedNode) {
        let clickedRowIdx = trickRows.findIndex(tr => tr.node === clickedNode);
        let isExactTrickNode = (clickedRowIdx !== -1);
        if (clickedRowIdx === -1) {
          let anc = clickedNode._parent;
          while (anc && clickedRowIdx === -1) {
            clickedRowIdx = trickRows.findIndex(tr => tr.node === anc);
            anc = anc._parent;
          }
        }
        if (clickedRowIdx !== -1) {
          selectedTrickIdx = clickedRowIdx;
          selectTrickInInspector(clickedRowIdx, rowElements, isExactTrickNode);
        }
      }
    }

    inspectorContent.appendChild(trickSection);
  }

  if (typeof window !== 'undefined') {
    window._getCurrentTreeData = () => currentTreeData;
    window._showNodeInspector = showNodeInspector;
    window._expandAndInspectNode = expandAndInspectNode;
    window._centerNodeInTree = centerNodeInTree;
    window._traceHandByNumber = traceHandByNumber;
    window._clearTracedHand = clearTracedHand;
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
    let rawClean = token.replace(/[\(\)]/g, '').trim();
    let cleanToken = rawClean.toUpperCase();
    let suit = null;
    let rankStr = '';

    if (cleanToken === 'R_' || cleanToken === 'R' || (cleanToken.startsWith('R') && cleanToken.length <= 2)) {
      suit = null;
      rankStr = cleanToken;
    } else if (cleanToken === '__' || cleanToken === '_' || cleanToken.startsWith('_')) {
      suit = null;
      rankStr = cleanToken;
    } else {
      if (['S', 'H', 'D', 'C'].includes(cleanToken.charAt(0))) {
        suit = cleanToken.charAt(0);
        rankStr = rawClean.substring(1);
      } else if (['S', 'H', 'D', 'C'].includes(cleanToken.charAt(cleanToken.length - 1))) {
        suit = cleanToken.charAt(cleanToken.length - 1);
        rankStr = rawClean.substring(0, rawClean.length - 1);
      } else {
        suit = leadSuit || 'S';
        rankStr = rawClean;
      }
    }

    if (rankStr === '10') rankStr = 'T';
    if (rankStr === '') rankStr = '_';
    if (rankStr !== 'x') rankStr = rankStr.toUpperCase();

    const rankValueMap = {
      'A': 14, 'K': 13, 'Q': 12, 'J': 11, 'T': 10, '10': 10, 'X': 10.5,
      '9': 9, '8': 8, '7': 7, '6': 6, '5': 5, '4': 4, '3': 3, '2': 2
    };
    const rankVal = rankValueMap[rankStr] || 1;

    let suitSym = '';
    let suitClass = 'card-suit-neutral';
    if (suit === 'S') { suitSym = '♠'; suitClass = 'card-suit-s'; }
    else if (suit === 'H') { suitSym = '♥'; suitClass = 'card-suit-h'; }
    else if (suit === 'D') { suitSym = '♦'; suitClass = 'card-suit-d'; }
    else if (suit === 'C') { suitSym = '♣'; suitClass = 'card-suit-c'; }

    const displayStr = suitSym ? `${suitSym}${rankStr}` : rankStr;

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

    // Check if node.label explicitly matches "Trick <N>:" or "#<N>:"
    if (node.label && /^(?:Trick\s+\d+|#\d+):/i.test(node.label.trim())) {
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
                               .replace(/\bhand [23]\b/gi, 'Hand 2 (West)');
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
        const suitNames = {
          S: '<span class="card-suit-s">♠</span> Spades',
          H: '<span class="card-suit-h">♥</span> Hearts',
          D: '<span class="card-suit-d">♦</span> Diamonds',
          C: '<span class="card-suit-c">♣</span> Clubs'
        };
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
        const suitNames = {
          S: '<span class="card-suit-s">♠</span> Spades',
          H: '<span class="card-suit-h">♥</span> Hearts',
          D: '<span class="card-suit-d">♦</span> Diamonds',
          C: '<span class="card-suit-c">♣</span> Clubs'
        };
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

  function createSampleHandBox(seatTitle, handStr) {
    const box = document.createElement('div');
    box.className = 'sample-hand-box';

    const titleEl = document.createElement('div');
    titleEl.className = 'sample-hand-seat';
    titleEl.textContent = seatTitle;
    box.appendChild(titleEl);

    const suitsData = parseHandToSuits(handStr || '');
    const suitOrder = [
      { key: 'S', symbol: '♠', cls: 'card-suit-s' },
      { key: 'H', symbol: '♥', cls: 'card-suit-h' },
      { key: 'D', symbol: '♦', cls: 'card-suit-d' },
      { key: 'C', symbol: '♣', cls: 'card-suit-c' }
    ];

    suitOrder.forEach(s => {
      const line = document.createElement('div');
      line.className = 'sample-suit-line';

      const symSpan = document.createElement('span');
      symSpan.className = `sample-suit-sym ${s.cls}`;
      symSpan.textContent = s.symbol;

      const ranksSpan = document.createElement('span');
      ranksSpan.className = 'sample-suit-ranks';
      ranksSpan.textContent = suitsData[s.key] ? suitsData[s.key] : '-';

      line.appendChild(symSpan);
      line.appendChild(ranksSpan);
      box.appendChild(line);
    });

    return box;
  }

  function renderSampleHands(handsList) {
    if (!handsList || !Array.isArray(handsList) || handsList.length === 0) {
      clearSampleHands('No sample hands available for this mode.');
      return;
    }

    if (sampleHandsCounterBadge) {
      sampleHandsCounterBadge.textContent = String(handsList.length);
      sampleHandsCounterBadge.classList.remove('hidden');
    }

    if (sampleHandsGrid) {
      sampleHandsGrid.innerHTML = '';
      const cappedList = handsList.slice(0, 100);
      cappedList.forEach((deal, idx) => {
        const card = document.createElement('div');
        const boardId = (deal.board_id !== undefined) ? deal.board_id : idx;
        const boardNum = boardId + 1;
        card.className = `sample-deal-card${activeTracedBoardId === boardId ? ' traced-deal-card' : ''}`;
        card.dataset.boardId = String(boardId);
        card.title = `Click to trace Hand #${boardNum} in the Decision Tree`;
        card.addEventListener('click', () => {
          traceHandByNumber(boardNum);
        });

        const idBadge = document.createElement('span');
        idBadge.className = 'sample-deal-id';
        idBadge.textContent = `#${boardNum}`;
        card.appendChild(idBadge);

        const rowNorth = document.createElement('div');
        rowNorth.className = 'sample-deal-row-center';
        rowNorth.appendChild(createSampleHandBox('NORTH', deal.N));
        card.appendChild(rowNorth);

        const rowWE = document.createElement('div');
        rowWE.className = 'sample-deal-row-we';
        rowWE.appendChild(createSampleHandBox('WEST', deal.W));
        rowWE.appendChild(createSampleHandBox('EAST', deal.E));
        card.appendChild(rowWE);

        const rowSouth = document.createElement('div');
        rowSouth.className = 'sample-deal-row-center';
        rowSouth.appendChild(createSampleHandBox('SOUTH', deal.S));
        card.appendChild(rowSouth);

        sampleHandsGrid.appendChild(card);
      });
      sampleHandsGrid.classList.remove('hidden');
    }

    if (sampleHandsEmptyNotice) {
      sampleHandsEmptyNotice.classList.add('hidden');
    }
  }

  function clearSampleHands(msg = 'No sample hands available yet. Click "Solve Deal" to generate and inspect deals.') {
    if (sampleHandsCounterBadge) {
      sampleHandsCounterBadge.textContent = '0';
      sampleHandsCounterBadge.classList.add('hidden');
    }
    if (sampleHandsGrid) {
      sampleHandsGrid.innerHTML = '';
      sampleHandsGrid.classList.add('hidden');
    }
    if (sampleHandsEmptyNotice) {
      sampleHandsEmptyNotice.textContent = msg;
      sampleHandsEmptyNotice.classList.remove('hidden');
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
  },
  "safety_play": {
    "name": "Safety Play Deal (NT)",
    "trump": "NOTRUMP",
    "leader": "NORTH",
    "target": 7,
    "iters": 100,
    "north": "S A43; H A54; D 432; C -;",
    "east": "",
    "south": "S 2; H 32; D AK8765; C -;",
    "west": "",
    "unknown_cards": "S KQJT987 H KQJT987 D QJT9"
  },
  "twelve_card_spades": {
    "name": "12-Card Spades Deal (4S)",
    "trump": "SPADES",
    "leader": "NORTH",
    "target": 9,
    "iters": 500,
    "north": "S Q73; H JT87; D 87; C J95;",
    "east": "",
    "south": "S AKJ652; H A43; D 5; C KQ;",
    "west": "",
    "unknown_cards": "S T984 H KQ9652 D KQJT96 C AT876432"
  },
  "restricted_choice": {
    "name": "Restricted Choice (7S)",
    "trump": "SPADES",
    "leader": "NORTH",
    "target": 13,
    "iters": 50,
    "north": "S AT987; H AK; D AKQ; C AKQ;",
    "east": "",
    "south": "S K654; H QJT; D JT9; C JT9;",
    "west": "",
    "unknown_cards": "S QJ32 H 98765432 D 8765432 C 8765432"
  },
  "mollo_6s": {
    "name": "Mollo 6S",
    "trump": "SPADES",
    "leader": "EAST",
    "cur_trick": ["DQ"],
    "target": 12,
    "iters": 50,
    "north": "S AKJT83; H AKQ; D -; C A652;",
    "east": "D Q;",
    "south": "S Q52; H JT984; D K2; C 874;",
    "west": "D A;",
    "unknown_cards": "S 9764 H 76532 D JT9876543 C KQJT93"
  },
  "two_way_finesse": {
    "name": "2-Way Finesse",
    "trump": "NOTRUMP",
    "leader": "NORTH",
    "target": 10,
    "iters": 50,
    "north": "S AJ97; H A2; D A2; C A2;",
    "east": "",
    "south": "S KT86; H K3; D K3; C K3;",
    "west": "",
    "unknown_cards": "S Q5432 H 98765 D 98765 C 98765"
  }
}

)rawjson";

} // namespace web_assets
