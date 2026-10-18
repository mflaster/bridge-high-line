#pragma once

#include <string>

namespace web_assets {

inline const char* INDEX_HTML = R"rawhtml(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=5.0">
  <meta name="theme-color" content="#0b1120">
  <meta name="apple-mobile-web-app-capable" content="yes">
  <meta name="mobile-web-app-capable" content="yes">
  <meta name="apple-mobile-web-app-status-bar-style" content="black-translucent">
  <meta name="apple-mobile-web-app-title" content="Bridge Solver">
  <title>Bridge Deal Solver & Decision Tree Explorer</title>
  <script src="https://browser.sentry-cdn.com/11.8.0/bundle.logs.metrics.min.js" crossorigin="anonymous"></script>
  <link rel="stylesheet" href="styles.css">
</head>
<body>
  <header class="app-header">
    <div class="header-content">
      <div class="logo">
        <span class="spade-icon">♠</span>
        <h1>Bridge Deal Solver</h1>
      </div>
      <div class="preset-selector">
        <label for="presetSelect">Hand Selector:</label>
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
          <option value="hand_8_261014">Hand 8 from 261014</option>
        </select>
        <a id="openHelpBtn" href="help.html" target="_blank" rel="noopener noreferrer" class="feedback-header-btn" title="Open Help &amp; Strategy Guide in a new tab">
          ❓ Help ↗
        </a>
        <button id="openFeedbackBtn" type="button" class="feedback-header-btn" title="Send feedback or report an issue">
          💬 Feedback
        </button>
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
                    <label for="numIters">Rand deals:</label>
                    <input type="text" inputmode="numeric" id="numIters" value="100" class="middle-number-input">
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
            <div class="hand-inputs-actions">
              <button id="recropPhotoBtn" type="button" class="btn btn-secondary-sm hidden" title="Re-open photo/PDF crop &amp; board scanner">Crop Photo</button>
              <button id="loadTxtBtn" type="button" class="btn btn-secondary-sm" title="Load from camera photo, image, PDF, PBN, or .txt/.deal file">Load</button>
              <input type="file" id="fileInputTxt" accept="image/*,.pdf,application/pdf,.pbn,.txt,.deal" style="display: none;">
            </div>
          </div>
          <div id="declarerRotateBar" class="declarer-rotate-bar">
            <span class="declarer-rotate-label" title="Rotates the 4 compass hands so Declarer is placed at North (Hand 0) and Dummy at South (Hand 2)">Declarer (→ N):</span>
            <div class="declarer-seat-group" role="group" aria-label="Select Declarer compass seat">
              <button type="button" class="declarer-seat-btn active" data-seat="N">N</button>
              <button type="button" class="declarer-seat-btn" data-seat="E">E</button>
              <button type="button" class="declarer-seat-btn" data-seat="S">S</button>
              <button type="button" class="declarer-seat-btn" data-seat="W">W</button>
            </div>
            <span id="declarerMappingHint" class="declarer-mapping-hint">N=Decl · S=Dummy</span>
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
          <button id="stopSolveActionBtn" type="button" class="btn btn-stop btn-large hidden" title="Stop current solve">
            <span class="btn-icon">■</span> Stop
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
        <div id="statusBox" class="status-box status-idle" role="status" aria-live="polite">
          <div class="status-indicator"></div>
          <span id="statusMessage">Ready to solve. Configure hands and click "Solve Deal".</span>
          <button id="statusFeedbackBtn" type="button" class="btn btn-secondary-sm status-feedback-btn hidden" title="Report issue with current error">Report Issue</button>
          <div id="statusProgress" class="status-progress hidden" role="progressbar"
               aria-valuemin="0" aria-valuemax="100" aria-valuenow="0">
            <div id="statusProgressFill" class="status-progress-fill"></div>
          </div>
          <span id="execTime" class="exec-time"></span>
          <button id="stopSolveBtn" type="button" class="btn btn-stop-sm hidden" title="Stop current solve">■ Stop</button>
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
          <div id="cardStatActual" class="stat-card stat-card-toggle hidden" role="button" tabindex="0" title="Click to view and trace the Actual Deal (#0)">
            <span class="stat-label">Actual (#0)</span>
            <span id="statActual" class="stat-value">-</span>
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

  <!-- Photo Crop & On-Device Hand Record Scanner Modal -->
  <div id="photoScanModal" class="modal-overlay hidden" role="dialog" aria-modal="true" aria-labelledby="photoScanModalTitle">
    <div class="modal-card photo-scan-modal-card">
      <div class="modal-header">
        <div class="modal-title-group">
          <h2 id="photoScanModalTitle">Scan Hand Record Photo</h2>
          <span class="modal-subtitle">Drag crop handles around a single 4-hand board diagram (or tap any highlighted board)</span>
        </div>
        <button id="closePhotoModalBtn" type="button" class="inspector-close-btn" title="Close modal">✕</button>
      </div>

      <div class="photo-scan-body">
        <!-- Left / Top: Interactive Crop Canvas & Toolbar -->
        <div class="photo-crop-column">
          <div class="photo-crop-toolbar">
            <div id="pdfPageBar" class="pdf-page-bar hidden">
              <button id="pdfPrevPageBtn" type="button" class="btn btn-secondary-sm" title="Previous PDF page">◀</button>
              <span id="pdfPageIndicator" class="pdf-page-indicator">Page 1 / 1</span>
              <button id="pdfNextPageBtn" type="button" class="btn btn-secondary-sm" title="Next PDF page">▶</button>
            </div>
            <button id="cropRotateBtn" type="button" class="btn btn-secondary-sm" title="Rotate photo 90° clockwise">↻ Rotate 90°</button>
            <button id="cropZoomToBoxBtn" type="button" class="btn btn-secondary-sm" title="Zoom view to current crop box">🔍 Zoom Crop</button>
            <button id="cropResetViewBtn" type="button" class="btn btn-secondary-sm" title="Fit full page/photo in view">Fit Page</button>
            <button id="cropNewPhotoBtn" type="button" class="btn btn-secondary-sm" title="Select another file">New File</button>
          </div>

          <div id="detectedBoardsBar" class="detected-boards-bar hidden">
            <span class="detected-boards-label">Detected Boards:</span>
            <div id="detectedBoardsList" class="detected-boards-list"></div>
          </div>

          <div id="cropCanvasViewport" class="crop-canvas-viewport">
            <canvas id="cropCanvas"></canvas>
          </div>
        </div>

        <!-- Right / Bottom: Scanned Hands Preview, Declarer Selection & Actions -->
        <div class="photo-scan-controls-column">
          <div id="scanStatusBadge" class="scan-status-badge scan-status-pending">
            Scanning board diagram...
          </div>

          <div class="scan-declarer-box">
            <div class="scan-declarer-header">
              <span class="scan-declarer-title">Who is Declarer?</span>
              <span id="modalDeclarerHint" class="scan-declarer-hint">North → Hand 0 (Declarer) · South → Hand 2 (Dummy)</span>
            </div>
            <div class="declarer-seat-group declarer-seat-group-lg" role="group" aria-label="Select Declarer in modal">
              <button type="button" class="modal-declarer-btn active" data-seat="N">North (N)</button>
              <button type="button" class="modal-declarer-btn" data-seat="E">East (E)</button>
              <button type="button" class="modal-declarer-btn" data-seat="S">South (S)</button>
              <button type="button" class="modal-declarer-btn" data-seat="W">West (W)</button>
            </div>
          </div>

          <div class="scan-contract-box">
            <div class="scan-declarer-header">
              <span class="scan-declarer-title">Contract</span>
              <span id="modalContractHint" class="scan-declarer-hint">4♠ (10 tricks)</span>
            </div>
            <div class="scan-contract-controls">
              <div class="declarer-seat-group contract-level-group" role="group" aria-label="Select Contract Level">
                <button type="button" class="modal-contract-level-btn" data-level="1">1</button>
                <button type="button" class="modal-contract-level-btn" data-level="2">2</button>
                <button type="button" class="modal-contract-level-btn" data-level="3">3</button>
                <button type="button" class="modal-contract-level-btn active" data-level="4">4</button>
                <button type="button" class="modal-contract-level-btn" data-level="5">5</button>
                <button type="button" class="modal-contract-level-btn" data-level="6">6</button>
                <button type="button" class="modal-contract-level-btn" data-level="7">7</button>
              </div>
              <div class="declarer-seat-group contract-strain-group" role="group" aria-label="Select Contract Strain">
                <button type="button" class="modal-contract-strain-btn" data-strain="CLUBS"><span class="card-suit-c">♣</span>C</button>
                <button type="button" class="modal-contract-strain-btn" data-strain="DIAMONDS"><span class="card-suit-d">♦</span>D</button>
                <button type="button" class="modal-contract-strain-btn" data-strain="HEARTS"><span class="card-suit-h">♥</span>H</button>
                <button type="button" class="modal-contract-strain-btn active" data-strain="SPADES"><span class="card-suit-s">♠</span>S</button>
                <button type="button" class="modal-contract-strain-btn" data-strain="NOTRUMP">NT</button>
              </div>
            </div>
          </div>

          <div class="scan-initial-play-box">
            <div class="scan-declarer-header">
              <span class="scan-declarer-title">Opening Lead / Initial Play</span>
              <span id="modalNextLeadHint" class="scan-declarer-hint">Opening lead: E (LHO)</span>
            </div>
            <div class="scan-initial-input-row">
              <input type="text" id="scanInitialCardsInput" class="text-input mini-text-input" placeholder="Optional: e.g. DQ or DQ D2 DA DK S7 (or tap below)">
              <button id="scanUndoCardBtn" type="button" class="btn btn-secondary-sm" disabled title="Undo last card">Undo</button>
              <button id="scanClearCardsBtn" type="button" class="btn btn-secondary-sm" disabled title="Clear all initial cards">Clear</button>
            </div>
            <div id="scanInitialPlayError" class="scan-initial-error hidden"></div>
            <div id="scanInitialPlayedList" class="scan-initial-played-list hidden"></div>
            <div id="scanInitialCardPicker" class="scan-initial-card-picker"></div>
          </div>

          <div class="scan-hands-preview">
            <div class="scan-hands-header">
              <span>Scanned Compass Hands (editable)</span>
              <button id="rescanCropBtn" type="button" class="btn btn-secondary-sm" title="Re-run OCR on current crop">↻ Re-scan</button>
            </div>
            <div class="scan-hand-field">
              <label for="scanInputN"><span class="seat-pill">N</span><span id="scanRoleN" class="scan-role-tag">Declarer</span></label>
              <input type="text" id="scanInputN" class="text-input mini-text-input" placeholder="S ...; H ...; D ...; C ...;">
            </div>
            <div class="scan-hand-field">
              <label for="scanInputE"><span class="seat-pill">E</span><span id="scanRoleE" class="scan-role-tag">Defender (LHO)</span></label>
              <input type="text" id="scanInputE" class="text-input mini-text-input" placeholder="S ...; H ...; D ...; C ...;">
            </div>
            <div class="scan-hand-field">
              <label for="scanInputS"><span class="seat-pill">S</span><span id="scanRoleS" class="scan-role-tag">Dummy</span></label>
              <input type="text" id="scanInputS" class="text-input mini-text-input" placeholder="S ...; H ...; D ...; C ...;">
            </div>
            <div class="scan-hand-field">
              <label for="scanInputW"><span class="seat-pill">W</span><span id="scanRoleW" class="scan-role-tag">Defender (RHO)</span></label>
              <input type="text" id="scanInputW" class="text-input mini-text-input" placeholder="S ...; H ...; D ...; C ...;">
            </div>
          </div>

          <div class="modal-footer-actions">
            <button id="cancelPhotoModalBtn" type="button" class="btn btn-secondary">Cancel</button>
            <button id="applyPhotoModalBtn" type="button" class="btn btn-primary">Apply to Deal</button>
          </div>
        </div>
      </div>
    </div>
  </div>

  <!-- User Feedback Modal -->
  <div id="feedbackModal" class="feedback-modal-overlay hidden" role="dialog" aria-modal="true" aria-labelledby="feedbackModalTitle">
    <div class="feedback-modal-card">
      <div class="feedback-modal-header">
        <h3 id="feedbackModalTitle">Send Feedback</h3>
        <button id="closeFeedbackModalBtn" type="button" class="feedback-modal-close" aria-label="Close feedback dialog">&times;</button>
      </div>
      <p class="feedback-modal-subtitle">
        Report a bug, unexpected line of play, or suggestion.
      </p>
      <form id="feedbackForm" class="feedback-form">
        <div class="feedback-form-row">
          <div class="feedback-field">
            <label for="feedbackNameInput">Name (optional)</label>
            <input type="text" id="feedbackNameInput" class="text-input" placeholder="Your name" autocomplete="name">
          </div>
          <div class="feedback-field">
            <label for="feedbackEmailInput">Email (optional)</label>
            <input type="email" id="feedbackEmailInput" class="text-input" placeholder="you@example.com" autocomplete="email">
          </div>
        </div>
        <div class="feedback-field">
          <label for="feedbackMessageInput">Feedback / Issue Description *</label>
          <textarea id="feedbackMessageInput" class="text-input feedback-textarea" rows="4" required placeholder="What happened, or what would you like to see improved?"></textarea>
        </div>
        <div class="feedback-checkbox-group">
          <label class="feedback-checkbox-label">
            <input type="checkbox" id="feedbackIncludeDealCheckbox" checked>
            <span>Attach current deal &amp; solver state</span>
          </label>
          <label class="feedback-checkbox-label">
            <input type="checkbox" id="feedbackIncludeExplorerCheckbox" checked>
            <span>Attach decision-tree &amp; line-explorer state</span>
          </label>
        </div>
        <div id="feedbackStatusMsg" class="feedback-status-msg hidden"></div>
        <div class="feedback-modal-actions">
          <button id="cancelFeedbackBtn" type="button" class="btn-secondary">Cancel</button>
          <button id="submitFeedbackBtn" type="submit" class="btn-primary">Send Feedback</button>
        </div>
      </form>
    </div>
  </div>

  <!-- Help & Strategy Guide Modal -->
  <div id="helpModal" class="feedback-modal-overlay hidden" role="dialog" aria-modal="true" aria-labelledby="helpModalTitle">
    <div class="feedback-modal-card help-modal-card">
      <div class="feedback-modal-header">
        <h3 id="helpModalTitle">Help &amp; Strategy Guide</h3>
        <div class="help-modal-header-actions">
          <a id="helpOpenNewTabLink" href="help.html" target="_blank" rel="noopener noreferrer" class="feedback-header-btn">Open in New Tab ↗</a>
          <button id="closeHelpModalBtn" type="button" class="feedback-modal-close" aria-label="Close help dialog">&times;</button>
        </div>
      </div>
      <nav id="helpTopicsNav" class="help-topics-nav" aria-label="Help topics">
        <button type="button" class="help-topic-btn active" data-topic="loading">📥 Loading Hands</button>
        <button type="button" class="help-topic-btn" data-topic="tree">🌳 Decision Tree &amp; Inspector</button>
        <button type="button" class="help-topic-btn" data-topic="lines">🔀 Alternative Lines &amp; Compare</button>
        <button type="button" class="help-topic-btn" data-topic="summaries">📊 Summaries &amp; Sample Hands</button>
        <button type="button" class="help-topic-btn" data-topic="strategy">🧠 How the Solver Thinks</button>
      </nav>
      <div id="helpModalBody" class="help-markdown-body">
        <p class="help-loading">Loading guide...</p>
      </div>
    </div>
  </div>

  <script src="app.js"></script>
</body>
</html>

)rawhtml";

inline const char* HELP_HTML = R"rawhtml(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=5.0">
  <meta name="theme-color" content="#0b1120">
  <title>Help &amp; Strategy Guide — Bridge Deal Solver</title>
  <link rel="stylesheet" href="styles.css">
</head>
<body class="help-page-body">
  <header class="app-header">
    <div class="header-content">
      <div class="logo">
        <a href="index.html" class="help-logo-link" title="Back to Bridge Deal Solver">
          <span class="spade-icon">♠</span>
          <h1>Bridge Deal Solver — Help &amp; Strategy Guide</h1>
        </a>
      </div>
      <div class="preset-selector">
        <a href="index.html" class="feedback-header-btn help-back-btn">
          ♠ Open Solver
        </a>
      </div>
    </div>
  </header>

  <main class="help-page-main">
    <div class="card-panel help-page-card">
      <div class="help-page-top">
        <h2 id="helpPageTitle" class="help-page-title">Loading Hands &amp; Deal Setup</h2>
        <nav id="helpPageTopicsNav" class="help-topics-nav" aria-label="Help topics">
          <button type="button" class="help-topic-btn active" data-topic="loading">📥 Loading Hands</button>
          <button type="button" class="help-topic-btn" data-topic="tree">🌳 Decision Tree &amp; Inspector</button>
          <button type="button" class="help-topic-btn" data-topic="lines">🔀 Alternative Lines &amp; Compare</button>
          <button type="button" class="help-topic-btn" data-topic="summaries">📊 Summaries &amp; Sample Hands</button>
          <button type="button" class="help-topic-btn" data-topic="strategy">🧠 How the Solver Thinks</button>
          <button type="button" class="help-topic-btn" data-topic="all">📑 All Topics</button>
        </nav>
      </div>
      <div id="helpPageContent" class="help-markdown-body help-page-markdown">
        <p class="help-loading">Loading guide...</p>
      </div>
    </div>
  </main>

  <script>
    document.addEventListener('DOMContentLoaded', () => {
      const helpPageTitle = document.getElementById('helpPageTitle');
      const helpPageTopicsNav = document.getElementById('helpPageTopicsNav');
      const helpPageContent = document.getElementById('helpPageContent');

      const helpTopicsHtml = {
        loading: '',
        tree: '',
        lines: '',
        summaries: '',
        strategy: '',
        all: ''
      };

      const helpTopicTitles = {
        loading: 'Loading Hands & Deal Setup',
        tree: 'Decision Tree & Line Inspector',
        lines: 'Trying & Comparing Alternative Lines',
        summaries: 'Summaries, Sample Hands & Actual Deal (#0)',
        strategy: 'How the Solver Thinks About Bridge',
        all: 'Complete User Guide & Bridge Strategy'
      };

      function escapeHelpHtml(str) {
        return String(str)
          .replace(/&/g, '&amp;')
          .replace(/</g, '&lt;')
          .replace(/>/g, '&gt;')
          .replace(/"/g, '&quot;');
      }

      function formatHelpInline(text) {
        const codeSlots = [];
        let s = String(text).replace(/`([^`]+)`/g, (_, code) => {
          const idx = codeSlots.length;
          codeSlots.push(`<code>${escapeHelpHtml(code)}</code>`);
          return `\u0000CODE${idx}\u0000`;
        });
        s = escapeHelpHtml(s);
        s = s.replace(/!\[([^\]]*)\]\(([^)]+)\)/g, (_, alt, src) => {
          return `<img src="${src}" alt="${alt}" class="help-inline-img" loading="lazy">`;
        });
        s = s.replace(/\[([^\]]+)\]\(([^)]+)\)/g, (_, label, href) => {
          let url = href;
          if (url.startsWith('../')) {
            url = 'https://github.com/mflaster/bridge-high-line/blob/main/' + url.slice(3);
          }
          return `<a href="${url}" target="_blank" rel="noopener noreferrer">${label}</a>`;
        });
        s = s.replace(/\*\*([^*]+)\*\*/g, '<strong>$1</strong>');
        s = s.replace(/\*([^*]+)\*/g, '<em>$1</em>');
        s = s.replace(/\u0000CODE(\d+)\u0000/g, (_, idx) => codeSlots[Number(idx)] || '');
        return s;
      }

      function renderHelpListItems(items) {
        return items.map(it => {
          let inner = formatHelpInline(it.text);
          if (it.subItems && it.subItems.length > 0) {
            const tag = it.subType === 'ol' ? 'ol' : 'ul';
            inner += `<${tag}>` +
              it.subItems.map(sub => `<li>${formatHelpInline(sub)}</li>`).join('') +
              `</${tag}>`;
          }
          return `<li>${inner}</li>`;
        }).join('');
      }

      function renderHelpMarkdown(md) {
        const lines = String(md).split(/\r?\n/);
        const out = [];
        let i = 0;
        while (i < lines.length) {
          const line = lines[i];
          if (!line.trim()) {
            i++;
            continue;
          }
          const imgMatch = line.trim().match(/^!\[([^\]]*)\]\(([^)]+)\)$/);
          if (imgMatch) {
            const alt = escapeHelpHtml(imgMatch[1]);
            const src = escapeHelpHtml(imgMatch[2]);
            out.push(
              `<figure class="help-figure">` +
              `<img src="${src}" alt="${alt}" loading="lazy" onerror="if(!this.dataset.fb){this.dataset.fb='1';this.src='../docs/'+this.getAttribute('src');}">` +
              (alt ? `<figcaption>${alt}</figcaption>` : '') +
              `</figure>`
            );
            i++;
            continue;
          }
          if (line.startsWith('```mermaid')) {
            i++;
            const steps = [];
            while (i < lines.length && !lines[i].startsWith('```')) {
              const matches = lines[i].matchAll(/\["([^"]+)"\]/g);
              for (const m of matches) {
                steps.push(m[1]);
              }
              i++;
            }
            if (i < lines.length) i++;
            if (steps.length > 0) {
              out.push('<div class="help-pipeline">' + steps.map(st => {
                const parts = st.split(/<br\s*\/?>/i).map(p => escapeHelpHtml(p));
                return `<div class="help-pipeline-step">${parts.join('<br>')}</div>`;
              }).join('') + '</div>');
            }
            continue;
          }
          if (line.startsWith('```')) {
            i++;
            const codeLines = [];
            while (i < lines.length && !lines[i].startsWith('```')) {
              codeLines.push(escapeHelpHtml(lines[i]));
              i++;
            }
            if (i < lines.length) i++;
            out.push(`<pre><code>${codeLines.join('\n')}</code></pre>`);
            continue;
          }
          if (line.startsWith('---')) {
            out.push('<hr>');
            i++;
            continue;
          }
          const hMatch = line.match(/^(#{1,3})\s+(.*)$/);
          if (hMatch) {
            const lvl = hMatch[1].length;
            if (lvl === 1 && out.length === 0) {
              i++;
              continue;
            }
            out.push(`<h${lvl}>${formatHelpInline(hMatch[2])}</h${lvl}>`);
            i++;
            continue;
          }
          if (line.startsWith('|')) {
            const tableLines = [];
            while (i < lines.length && lines[i].startsWith('|')) {
              tableLines.push(lines[i]);
              i++;
            }
            if (tableLines.length >= 2) {
              const parseRow = r => r.trim().replace(/^\||\|$/g, '').split('|').map(c => c.trim());
              const headers = parseRow(tableLines[0]);
              const bodyRows = tableLines.slice(2).map(parseRow);
              out.push('<table><thead><tr>' +
                headers.map(h => `<th>${formatHelpInline(h)}</th>`).join('') +
                '</tr></thead><tbody>' +
                bodyRows.map(r => '<tr>' + r.map(c => `<td>${formatHelpInline(c)}</td>`).join('') + '</tr>').join('') +
                '</tbody></table>');
            }
            continue;
          }
          if (line.startsWith('> ')) {
            const qLines = [];
            while (i < lines.length && lines[i].startsWith('> ')) {
              qLines.push(lines[i].slice(2).trim());
              i++;
            }
            out.push(`<blockquote>${formatHelpInline(qLines.join(' '))}</blockquote>`);
            continue;
          }
          if (line.startsWith('- ')) {
            const items = [];
            while (i < lines.length && (lines[i].startsWith('- ') || lines[i].startsWith('  '))) {
              const cur = lines[i];
              if (cur.startsWith('- ')) {
                items.push({ text: cur.slice(2).trim(), subType: null, subItems: [] });
              } else if (items.length > 0) {
                const last = items[items.length - 1];
                const trimmed = cur.trim();
                if (trimmed.startsWith('- ')) {
                  last.subType = 'ul';
                  last.subItems.push(trimmed.slice(2).trim());
                } else if (/^\d+\.\s+/.test(trimmed)) {
                  last.subType = 'ol';
                  last.subItems.push(trimmed.replace(/^\d+\.\s+/, '').trim());
                } else if (last.subItems.length > 0) {
                  last.subItems[last.subItems.length - 1] += ' ' + trimmed;
                } else {
                  last.text += ' ' + trimmed;
                }
              }
              i++;
            }
            out.push('<ul>' + renderHelpListItems(items) + '</ul>');
            continue;
          }
          if (/^\d+\.\s+/.test(line)) {
            const items = [];
            while (i < lines.length && (/^\d+\.\s+/.test(lines[i]) || lines[i].startsWith('  '))) {
              const cur = lines[i];
              if (/^\d+\.\s+/.test(cur)) {
                items.push({ text: cur.replace(/^\d+\.\s+/, '').trim(), subType: null, subItems: [] });
              } else if (items.length > 0) {
                const last = items[items.length - 1];
                const trimmed = cur.trim();
                if (trimmed.startsWith('- ')) {
                  last.subType = 'ul';
                  last.subItems.push(trimmed.slice(2).trim());
                } else if (/^\d+\.\s+/.test(trimmed)) {
                  last.subType = 'ol';
                  last.subItems.push(trimmed.replace(/^\d+\.\s+/, '').trim());
                } else if (last.subItems.length > 0) {
                  last.subItems[last.subItems.length - 1] += ' ' + trimmed;
                } else {
                  last.text += ' ' + trimmed;
                }
              }
              i++;
            }
            out.push('<ol>' + renderHelpListItems(items) + '</ol>');
            continue;
          }
          const pLines = [];
          while (
            i < lines.length &&
            lines[i].trim() &&
            !lines[i].trim().match(/^!\[([^\]]*)\]\(([^)]+)\)$/) &&
            !lines[i].startsWith('#') &&
            !lines[i].startsWith('---') &&
            !lines[i].startsWith('```') &&
            !lines[i].startsWith('|') &&
            !lines[i].startsWith('> ') &&
            !lines[i].startsWith('- ') &&
            !/^\d+\.\s+/.test(lines[i])
          ) {
            pLines.push(lines[i].trim());
            i++;
          }
          out.push(`<p>${formatHelpInline(pLines.join(' '))}</p>`);
        }
        return out.join('\n');
      }

      async function fetchHelpMarkdown(fileName) {
        let res = await fetch(fileName, { cache: 'no-cache' });
        if (!res.ok) {
          res = await fetch(`../docs/${fileName}`, { cache: 'no-cache' });
        }
        if (!res.ok) throw new Error(`HTTP ${res.status}`);
        return res.text();
      }

      function resolveTopicFromUrl() {
        const params = new URLSearchParams(window.location.search);
        const qTopic = params.get('topic') || params.get('help');
        if (qTopic && helpTopicsHtml[qTopic] !== undefined) return qTopic;
        const hash = window.location.hash.replace(/^#/, '');
        if (hash === 'how-it-works') return 'strategy';
        if (hash.startsWith('help=')) {
          const h = hash.slice(5);
          if (helpTopicsHtml[h] !== undefined) return h;
        }
        if (helpTopicsHtml[hash] !== undefined) return hash;
        return 'loading';
      }

      function selectTopic(topic, updateHash = true) {
        const target = helpTopicsHtml[topic] !== undefined ? topic : 'loading';
        if (helpPageTopicsNav) {
          helpPageTopicsNav.querySelectorAll('.help-topic-btn').forEach(btn => {
            btn.classList.toggle('active', btn.dataset.topic === target);
          });
        }
        if (helpPageTitle && helpTopicTitles[target]) {
          helpPageTitle.textContent = helpTopicTitles[target];
        }
        if (helpPageContent) {
          helpPageContent.innerHTML = helpTopicsHtml[target] || '<p>No content available.</p>';
        }
        if (updateHash) {
          history.replaceState(null, '', `#${target}`);
        }
        window.scrollTo({ top: 0, behavior: 'smooth' });
      }

      if (helpPageTopicsNav) {
        helpPageTopicsNav.querySelectorAll('.help-topic-btn').forEach(btn => {
          btn.addEventListener('click', () => {
            if (btn.dataset.topic) {
              selectTopic(btn.dataset.topic, true);
            }
          });
        });
      }

      window.addEventListener('hashchange', () => {
        selectTopic(resolveTopicFromUrl(), false);
      });

      Promise.all([
        fetchHelpMarkdown('user-guide.md'),
        fetchHelpMarkdown('bridge-strategy.md')
      ]).then(([userGuideMd, strategyMd]) => {
        const sections = String(userGuideMd).split(/\n(?=##\s+)/);
        const topicKeys = ['loading', 'tree', 'lines', 'summaries'];
        let topicIdx = 0;
        for (const sec of sections) {
          const trimmed = sec.trim().replace(/\n---\s*$/, '').trim();
          if (!trimmed.startsWith('## ')) continue;
          if (topicIdx < topicKeys.length) {
            helpTopicsHtml[topicKeys[topicIdx]] = renderHelpMarkdown(trimmed);
            topicIdx++;
          }
        }
        helpTopicsHtml.strategy = renderHelpMarkdown(strategyMd);
        helpTopicsHtml.all =
          renderHelpMarkdown(userGuideMd) +
          '<hr><h1>How the Solver Thinks About Bridge</h1>' +
          helpTopicsHtml.strategy;
        selectTopic(resolveTopicFromUrl(), false);
      }).catch(err => {
        if (helpPageContent) {
          helpPageContent.innerHTML = `<p>Could not load guide (${escapeHelpHtml(err.message || err)}).</p>`;
        }
      });
    });
  </script>
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

.preset-selector .select-input {
  max-width: 320px;
  text-overflow: ellipsis;
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
  padding: 6px 6px;
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
  white-space: nowrap;
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
  width: 38px;
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

.btn.hidden {
  display: none !important;
}

.btn-stop {
  background-color: #dc2626;
  color: #ffffff;
}

.btn-stop:hover:not(:disabled) {
  background-color: #b91c1c;
}

.btn-stop-sm {
  background-color: rgba(220, 38, 38, 0.85);
  color: #ffffff;
  border: 1px solid rgba(252, 165, 165, 0.45);
  padding: 4px 10px;
  font-size: 12px;
  font-weight: 700;
  border-radius: 5px;
  flex-shrink: 0;
}

.btn-stop-sm:hover:not(:disabled) {
  background-color: #dc2626;
  border-color: #fca5a5;
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

.status-feedback-btn {
  white-space: nowrap;
  font-size: 12px;
  padding: 3px 10px;
  background-color: rgba(239, 68, 68, 0.2);
  border: 1px solid rgba(239, 68, 68, 0.5);
  color: #fca5a5;
  cursor: pointer;
  border-radius: 4px;
  transition: all 0.15s ease;
}

.status-feedback-btn:hover:not(:disabled) {
  background-color: rgba(239, 68, 68, 0.35);
  border-color: #f87171;
  color: #ffffff;
}

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
  grid-template-columns: repeat(auto-fit, minmax(110px, 1fr));
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

#cardStatActual .stat-value {
  color: #38bdf8;
}

#cardStatActual.actual-makes .stat-value {
  color: #10b981;
}

#cardStatActual.actual-fails .stat-value {
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

#cardStatActual.active {
  border-color: #38bdf8;
  background-color: #0f2438;
  box-shadow: 0 0 10px rgba(56, 189, 248, 0.25);
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

.mode-badge.mode-actual {
  background-color: rgba(56, 189, 248, 0.2);
  color: #7dd3fc;
  border: 1px solid rgba(56, 189, 248, 0.45);
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
  touch-action: none;
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
  border-radius: 9999px;
  padding: 3px 8px;
  font-weight: 800 !important;
}

.card-chip.card-winner.card-winner-ns {
  border: 2px solid #38bdf8 !important;
  box-shadow: 0 0 8px rgba(56, 189, 248, 0.5), inset 0 0 4px rgba(56, 189, 248, 0.2);
}

.card-chip.card-winner.card-winner-ew {
  border: 2px solid #c084fc !important;
  box-shadow: 0 0 8px rgba(192, 132, 252, 0.5), inset 0 0 4px rgba(192, 132, 252, 0.2);
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

.sample-deal-card-actual {
  border-color: rgba(56, 189, 248, 0.55);
  background-color: #132c3d;
}

.sample-deal-id-actual {
  color: #7dd3fc;
  background-color: #0b1b2b;
  border-color: rgba(56, 189, 248, 0.55);
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

/* Mobile Responsive Layout */
@media (max-width: 768px) {
  .app-header {
    padding: 10px 12px;
  }

  .header-content {
    flex-wrap: wrap;
    gap: 8px;
  }

  .logo h1 {
    font-size: 17px;
  }

  .preset-selector {
    width: 100%;
    justify-content: space-between;
  }

  .preset-selector .select-input {
    flex: 1;
    min-width: 0;
    max-width: none;
  }

  .app-main {
    padding: 8px 10px 16px;
  }

  .card-panel {
    padding: 10px;
  }

  .hand-input-row {
    flex-direction: column;
    align-items: stretch;
    gap: 6px;
  }

  .req-fields {
    flex-direction: column;
    gap: 6px;
  }

  .decision-tree-panel > .panel-header {
    flex-direction: column;
    align-items: stretch;
    gap: 8px;
  }

  .tab-buttons {
    overflow-x: auto;
    -webkit-overflow-scrolling: touch;
    padding-bottom: 2px;
  }

  .tab-btn {
    padding: 6px 10px;
    font-size: 12.5px;
    white-space: nowrap;
  }

  .zoom-controls {
    display: flex;
    flex-wrap: wrap;
    align-items: center;
    gap: 6px;
  }

  .tree-legend {
    margin-right: auto;
  }

  .status-box {
    flex-wrap: wrap;
    padding: 8px 10px;
    font-size: 13px;
  }

  .status-progress {
    flex: 1 1 100%;
    order: 4;
  }

  .results-summary {
    grid-template-columns: repeat(2, 1fr);
    gap: 8px;
  }

  .tree-layout-wrapper {
    flex-direction: column;
    height: auto;
    min-height: 0;
  }

  .tree-viewport {
    height: 360px;
    min-height: 320px;
    flex: none;
    touch-action: none;
  }

  .tree-inspector-panel {
    width: 100%;
    max-width: 100%;
    height: auto;
    max-height: 360px;
  }
}

/* Hand Inputs Actions & Declarer Rotation Bar */
.hand-inputs-actions {
  display: flex;
  align-items: center;
  gap: 6px;
}

.declarer-rotate-bar {
  display: flex;
  align-items: center;
  flex-wrap: wrap;
  gap: 8px;
  padding: 4px 8px;
  background-color: #152238;
  border: 1px solid #293548;
  border-radius: 6px;
  margin-bottom: 2px;
}

.declarer-rotate-label {
  font-size: 11.5px;
  font-weight: 700;
  color: #93c5fd;
  white-space: nowrap;
}

.declarer-seat-group {
  display: inline-flex;
  background-color: #0b1120;
  border: 1px solid var(--panel-border);
  border-radius: 6px;
  padding: 2px;
  gap: 2px;
}

.declarer-seat-btn,
.modal-declarer-btn {
  background: transparent;
  color: var(--text-muted);
  border: none;
  border-radius: 4px;
  padding: 2px 9px;
  font-size: 11.5px;
  font-weight: 700;
  cursor: pointer;
  transition: all 0.15s ease;
}

.declarer-seat-btn:hover,
.modal-declarer-btn:hover {
  color: var(--text-main);
  background-color: rgba(59, 130, 246, 0.15);
}

.declarer-seat-btn.active,
.modal-declarer-btn.active {
  background-color: var(--primary);
  color: #ffffff;
}

.declarer-mapping-hint {
  font-size: 11px;
  color: var(--text-muted);
  font-family: monospace;
  margin-left: auto;
}

/* Photo Crop & On-Device Hand Scanner Modal */
.modal-overlay {
  position: fixed;
  inset: 0;
  background-color: rgba(6, 11, 22, 0.82);
  backdrop-filter: blur(4px);
  z-index: 1000;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 16px;
}

.modal-overlay.hidden {
  display: none !important;
}

.photo-scan-modal-card {
  background-color: var(--panel-bg);
  border: 1px solid var(--panel-border);
  border-radius: 12px;
  width: 100%;
  max-width: 1040px;
  max-height: 92vh;
  display: flex;
  flex-direction: column;
  box-shadow: 0 20px 50px rgba(0, 0, 0, 0.65);
  overflow: hidden;
}

.modal-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 12px 16px;
  background-color: #0b1120;
  border-bottom: 1px solid var(--panel-border);
  gap: 12px;
}

.modal-title-group {
  display: flex;
  flex-direction: column;
  gap: 2px;
}

.modal-title-group h2 {
  font-size: 16px;
  font-weight: 700;
  color: var(--text-main);
}

.modal-subtitle {
  font-size: 12px;
  color: var(--text-muted);
}

.photo-scan-body {
  display: grid;
  grid-template-columns: 1.25fr 1fr;
  gap: 16px;
  padding: 14px 16px;
  overflow-y: auto;
  min-height: 0;
}

.photo-crop-column {
  display: flex;
  flex-direction: column;
  gap: 8px;
  min-width: 0;
}

.photo-crop-toolbar {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  gap: 6px;
}

.pdf-page-bar {
  display: inline-flex;
  align-items: center;
  gap: 5px;
  background-color: #0f172a;
  border: 1px solid #38bdf8;
  border-radius: 6px;
  padding: 2px 6px;
}

.pdf-page-bar.hidden {
  display: none !important;
}

.pdf-page-indicator {
  font-size: 11px;
  font-weight: 700;
  color: #e0f2fe;
  padding: 0 4px;
  white-space: nowrap;
}

.detected-boards-bar {
  display: flex;
  align-items: center;
  gap: 6px;
  background-color: #0f172a;
  border: 1px solid var(--panel-border);
  border-radius: 6px;
  padding: 5px 8px;
  overflow-x: auto;
}

.detected-boards-bar.hidden {
  display: none !important;
}

.detected-boards-label {
  font-size: 11px;
  font-weight: 700;
  color: #38bdf8;
  white-space: nowrap;
}

.detected-boards-list {
  display: flex;
  align-items: center;
  gap: 4px;
}

.board-pill-btn {
  background-color: #1e293b;
  border: 1px solid #334155;
  color: #cbd5e1;
  border-radius: 4px;
  padding: 2px 7px;
  font-size: 11px;
  font-weight: 700;
  cursor: pointer;
  white-space: nowrap;
}

.board-pill-btn:hover {
  border-color: #38bdf8;
  color: #ffffff;
}

.board-pill-btn.active {
  background-color: #0284c7;
  border-color: #38bdf8;
  color: #ffffff;
}

.crop-canvas-viewport {
  position: relative;
  width: 100%;
  height: 410px;
  background-color: #090e1a;
  border: 1px solid var(--panel-border);
  border-radius: 8px;
  overflow: hidden;
  display: flex;
  align-items: center;
  justify-content: center;
  touch-action: none;
  user-select: none;
  -webkit-user-select: none;
}

#cropCanvas {
  display: block;
  width: 100%;
  height: 100%;
  cursor: crosshair;
  touch-action: none;
}

.photo-scan-controls-column {
  display: flex;
  flex-direction: column;
  gap: 12px;
  min-width: 0;
}

.scan-status-badge {
  padding: 8px 12px;
  border-radius: 8px;
  font-size: 12.5px;
  font-weight: 700;
  display: flex;
  align-items: center;
  gap: 8px;
}

.scan-status-ok {
  background-color: rgba(16, 185, 129, 0.15);
  border: 1px solid rgba(16, 185, 129, 0.45);
  color: #34d399;
}

.scan-status-warn {
  background-color: rgba(245, 158, 11, 0.15);
  border: 1px solid rgba(245, 158, 11, 0.45);
  color: #fbbf24;
}

.scan-status-pending {
  background-color: rgba(59, 130, 246, 0.15);
  border: 1px solid rgba(59, 130, 246, 0.4);
  color: #93c5fd;
}

.scan-declarer-box,
.scan-contract-box,
.scan-initial-play-box {
  background-color: #0f172a;
  border: 1px solid var(--panel-border);
  border-radius: 8px;
  padding: 9px 12px;
  display: flex;
  flex-direction: column;
  gap: 7px;
}

.scan-declarer-header {
  display: flex;
  justify-content: space-between;
  align-items: baseline;
  flex-wrap: wrap;
  gap: 6px;
}

.scan-declarer-title {
  font-size: 12.5px;
  font-weight: 700;
  color: var(--text-main);
}

.scan-declarer-hint {
  font-size: 11px;
  color: #93c5fd;
  font-family: monospace;
}

.declarer-seat-group-lg {
  width: 100%;
  display: grid;
  grid-template-columns: repeat(4, 1fr);
  padding: 3px;
  gap: 4px;
}

.modal-declarer-btn {
  padding: 5px 8px;
  font-size: 12px;
  text-align: center;
}

.scan-contract-controls {
  display: flex;
  align-items: center;
  gap: 6px;
  flex-wrap: wrap;
}

.contract-level-group {
  flex: 1 1 auto;
  display: grid;
  grid-template-columns: repeat(7, 1fr);
  padding: 2px;
  gap: 2px;
}

.contract-strain-group {
  flex: 1 1 auto;
  display: grid;
  grid-template-columns: repeat(5, 1fr);
  padding: 2px;
  gap: 2px;
}

.modal-contract-level-btn,
.modal-contract-strain-btn {
  background: transparent;
  color: var(--text-muted);
  border: none;
  border-radius: 4px;
  padding: 4px 5px;
  font-size: 12px;
  font-weight: 700;
  cursor: pointer;
  transition: all 0.15s ease;
  text-align: center;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  gap: 2px;
}

.modal-contract-level-btn:hover,
.modal-contract-strain-btn:hover {
  color: var(--text-main);
  background-color: rgba(59, 130, 246, 0.15);
}

.modal-contract-level-btn.active,
.modal-contract-strain-btn.active {
  background-color: var(--primary);
  color: #ffffff;
}

.scan-initial-input-row {
  display: flex;
  align-items: center;
  gap: 6px;
}

.scan-initial-input-row .text-input {
  flex: 1;
  min-width: 0;
}

.scan-initial-error {
  font-size: 11px;
  color: #fca5a5;
  background-color: rgba(127, 29, 29, 0.35);
  border: 1px solid #7f1d1d;
  border-radius: 4px;
  padding: 3px 8px;
}

.scan-initial-error.hidden,
.scan-initial-played-list.hidden {
  display: none !important;
}

.scan-initial-played-list {
  display: flex;
  flex-direction: column;
  gap: 4px;
  max-height: 92px;
  overflow-y: auto;
  padding: 4px 6px;
  background-color: #090e1a;
  border: 1px solid #1e293b;
  border-radius: 6px;
}

.scan-played-trick-row {
  display: flex;
  align-items: center;
  flex-wrap: wrap;
  gap: 5px;
  font-size: 11.5px;
  font-family: monospace;
}

.scan-played-trick-label {
  color: #94a3b8;
  font-weight: 700;
  min-width: 24px;
}

.scan-played-card-pill {
  display: inline-flex;
  align-items: center;
  gap: 2px;
  padding: 1px 5px;
  border-radius: 4px;
  background-color: #1e293b;
  border: 1px solid #334155;
  color: #f8fafc;
  font-weight: 700;
}

.scan-played-card-pill.winner-pill {
  border-color: #10b981;
  background-color: rgba(16, 185, 129, 0.16);
}

.scan-played-winner-tag {
  font-size: 10.5px;
  color: #6ee7b7;
  font-weight: 700;
  margin-left: 2px;
}

.scan-initial-card-picker {
  display: flex;
  flex-direction: column;
  gap: 4px;
  padding: 6px 8px;
  background-color: #0b1324;
  border: 1px solid #1e293b;
  border-radius: 6px;
}

.scan-picker-header {
  font-size: 11px;
  font-weight: 700;
  color: #cbd5e1;
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.scan-picker-suits {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  gap: 6px 10px;
}

.scan-picker-suit-group {
  display: inline-flex;
  align-items: center;
  gap: 3px;
}

.scan-picker-suit-sym {
  font-size: 13px;
  font-weight: 800;
  margin-right: 1px;
}

.scan-picker-card-btn {
  background-color: #1e293b;
  border: 1px solid #334155;
  color: #fef08a;
  border-radius: 4px;
  padding: 1px 5px;
  font-size: 11.5px;
  font-family: monospace;
  font-weight: 700;
  cursor: pointer;
  line-height: 1.3;
  transition: all 0.12s ease;
}

.scan-picker-card-btn:hover:not(:disabled) {
  background-color: #0284c7;
  border-color: #38bdf8;
  color: #ffffff;
}

.scan-picker-card-btn:disabled {
  opacity: 0.28;
  cursor: not-allowed;
  color: #64748b;
}

.scan-hands-preview {
  background-color: #0f172a;
  border: 1px solid var(--panel-border);
  border-radius: 8px;
  padding: 10px 12px;
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.scan-hands-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  font-size: 12px;
  font-weight: 700;
  color: var(--text-muted);
  text-transform: uppercase;
  letter-spacing: 0.4px;
}

.scan-hand-field {
  display: flex;
  flex-direction: column;
  gap: 3px;
}

.scan-hand-field label {
  display: flex;
  align-items: center;
  gap: 6px;
  font-size: 12px;
  font-weight: 700;
}

.seat-pill {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 20px;
  height: 20px;
  border-radius: 4px;
  background-color: #1e293b;
  border: 1px solid #475569;
  color: #f8fafc;
  font-size: 11.5px;
  font-weight: 800;
}

.scan-role-tag {
  font-size: 11px;
  font-weight: 600;
  color: var(--text-muted);
}

.scan-role-tag.role-declarer {
  color: #34d399;
  font-weight: 700;
}

.scan-role-tag.role-dummy {
  color: #60a5fa;
  font-weight: 700;
}

.modal-footer-actions {
  margin-top: auto;
  display: flex;
  justify-content: flex-end;
  gap: 10px;
  padding-top: 4px;
}

@media (max-width: 820px) {
  .modal-overlay {
    padding: 6px;
  }

  .photo-scan-modal-card {
    max-height: 96vh;
  }

  .photo-scan-body {
    grid-template-columns: 1fr;
    gap: 12px;
    padding: 10px 12px;
  }

  .crop-canvas-viewport {
    height: 300px;
  }

  .modal-footer-actions .btn {
    flex: 1;
  }
}


/* User Feedback Button & Modal */
.feedback-header-btn {
  background-color: #1e293b;
  color: var(--text-main);
  border: 1px solid var(--panel-border);
  border-radius: 6px;
  padding: 6px 12px;
  font-size: 13px;
  font-weight: 600;
  cursor: pointer;
  white-space: nowrap;
  transition: background-color 0.15s, border-color 0.15s;
}

.feedback-header-btn:hover {
  background-color: #334155;
  border-color: var(--primary);
}

.feedback-modal-overlay {
  position: fixed;
  inset: 0;
  background-color: rgba(2, 6, 23, 0.75);
  backdrop-filter: blur(2px);
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 16px;
  z-index: 2000;
}

.feedback-modal-overlay.hidden {
  display: none;
}

.feedback-modal-card {
  width: 100%;
  max-width: 480px;
  background-color: var(--panel-bg);
  border: 1px solid var(--panel-border);
  border-radius: 12px;
  padding: 20px;
  box-shadow: 0 20px 40px rgba(0, 0, 0, 0.5);
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.feedback-modal-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
}

.feedback-modal-header h3 {
  font-size: 18px;
  font-weight: 700;
  color: var(--text-main);
}

.feedback-modal-close {
  background: transparent;
  border: none;
  color: var(--text-muted);
  font-size: 24px;
  line-height: 1;
  cursor: pointer;
  padding: 2px 6px;
  border-radius: 4px;
}

.feedback-modal-close:hover {
  color: var(--text-main);
  background-color: rgba(255, 255, 255, 0.08);
}

.feedback-modal-subtitle {
  font-size: 13px;
  color: var(--text-muted);
}

.feedback-form {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.feedback-form-row {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 10px;
}

@media (max-width: 480px) {
  .feedback-form-row {
    grid-template-columns: 1fr;
  }
}

.feedback-field {
  display: flex;
  flex-direction: column;
  gap: 4px;
}

.feedback-field label {
  font-size: 12.5px;
  font-weight: 600;
  color: var(--text-muted);
}

.feedback-textarea {
  resize: vertical;
  min-height: 90px;
  font-family: inherit;
}

.feedback-checkbox-group {
  display: flex;
  flex-direction: column;
  gap: 6px;
}

.feedback-checkbox-label {
  display: flex;
  align-items: center;
  gap: 8px;
  font-size: 13px;
  color: var(--text-muted);
  cursor: pointer;
  user-select: none;
}

.feedback-status-msg {
  font-size: 13px;
  padding: 8px 10px;
  border-radius: 6px;
}

.feedback-status-msg.hidden {
  display: none;
}

.feedback-status-msg.success {
  background-color: rgba(16, 185, 129, 0.15);
  border: 1px solid rgba(16, 185, 129, 0.4);
  color: #6ee7b7;
}

.feedback-status-msg.error {
  background-color: rgba(239, 68, 68, 0.15);
  border: 1px solid rgba(239, 68, 68, 0.4);
  color: #fca5a5;
}

.feedback-modal-actions {
  display: flex;
  justify-content: flex-end;
  gap: 10px;
  margin-top: 4px;
}

.btn-secondary {
  background-color: transparent;
  color: var(--text-muted);
  border: 1px solid var(--panel-border);
  border-radius: 6px;
  padding: 8px 14px;
  font-size: 13.5px;
  font-weight: 600;
  cursor: pointer;
}

.btn-secondary:hover {
  color: var(--text-main);
  background-color: rgba(255, 255, 255, 0.06);
}

/* Help & Strategy Guide Modal */
.help-modal-card {
  max-width: 940px;
  max-height: 88vh;
  padding: 20px 24px;
}

.help-topics-nav {
  display: flex;
  flex-wrap: wrap;
  gap: 6px;
  padding-bottom: 10px;
  border-bottom: 1px solid var(--panel-border);
}

.help-topic-btn {
  background-color: #0f172a;
  color: var(--text-muted);
  border: 1px solid var(--panel-border);
  border-radius: 6px;
  padding: 5px 10px;
  font-size: 12px;
  font-weight: 600;
  cursor: pointer;
  white-space: nowrap;
  transition: background-color 0.15s ease, color 0.15s ease, border-color 0.15s ease;
}

.help-topic-btn:hover {
  color: var(--text-main);
  border-color: #60a5fa;
  background-color: #1e293b;
}

.help-topic-btn.active {
  background-color: var(--primary);
  color: #ffffff;
  border-color: var(--primary);
}

.help-markdown-body {
  overflow-y: auto;
  padding-right: 8px;
  color: #e2e8f0;
  font-size: 14.5px;
  line-height: 1.65;
}

.help-markdown-body > h2:first-child {
  margin-top: 4px;
}

.help-markdown-body h1 {
  font-size: 22px;
  font-weight: 700;
  color: #f8fafc;
  margin: 0 0 10px 0;
}

.help-markdown-body h2 {
  font-size: 17.5px;
  font-weight: 700;
  color: #93c5fd;
  margin: 20px 0 8px 0;
}

.help-markdown-body h3 {
  font-size: 15px;
  font-weight: 600;
  color: #cbd5e1;
  margin: 16px 0 6px 0;
}

.help-markdown-body p {
  margin: 0 0 12px 0;
}

.help-markdown-body ul,
.help-markdown-body ol {
  margin: 0 0 12px 22px;
  padding: 0;
}

.help-markdown-body li {
  margin-bottom: 6px;
}

.help-markdown-body hr {
  border: none;
  border-top: 1px solid var(--panel-border);
  margin: 18px 0;
}

.help-markdown-body blockquote {
  margin: 10px 0 14px 0;
  padding: 10px 14px;
  background-color: rgba(59, 130, 246, 0.1);
  border-left: 3px solid var(--primary);
  border-radius: 6px;
  color: #f1f5f9;
}

.help-markdown-body code {
  font-family: ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, monospace;
  font-size: 12.5px;
  background-color: rgba(15, 23, 42, 0.75);
  border: 1px solid rgba(148, 163, 184, 0.25);
  border-radius: 4px;
  padding: 1px 5px;
  color: #bae6fd;
}

.help-markdown-body a {
  color: #60a5fa;
  text-decoration: none;
}

.help-markdown-body a:hover {
  text-decoration: underline;
}

.help-pipeline {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(210px, 1fr));
  gap: 8px;
  margin: 12px 0 16px 0;
}

.help-pipeline-step {
  background-color: #0f172a;
  border: 1px solid var(--panel-border);
  border-radius: 8px;
  padding: 8px 10px;
  font-size: 12.5px;
  line-height: 1.35;
  color: #e2e8f0;
  text-align: center;
}

.help-markdown-body table {
  width: 100%;
  border-collapse: collapse;
  margin: 12px 0 8px 0;
  font-size: 13px;
}

.help-markdown-body th,
.help-markdown-body td {
  border: 1px solid var(--panel-border);
  padding: 7px 10px;
  text-align: left;
  vertical-align: top;
}

.help-markdown-body th {
  background-color: #0f172a;
  color: #f8fafc;
  font-weight: 600;
}

.help-loading {
  color: var(--text-muted);
  font-style: italic;
}

.help-modal-header-actions {
  display: flex;
  align-items: center;
  gap: 8px;
}

/* Standalone Help Page (help.html) & Screenshot Figures */
.help-page-body {
  height: auto;
  min-height: 100vh;
  overflow-y: auto;
}

.help-logo-link {
  display: flex;
  align-items: center;
  gap: 10px;
  color: inherit;
  text-decoration: none;
}

.help-logo-link:hover h1 {
  color: #93c5fd;
}

.help-back-btn {
  text-decoration: none;
  display: inline-flex;
  align-items: center;
  gap: 6px;
}

.help-page-main {
  max-width: 1040px;
  width: 100%;
  margin: 0 auto;
  padding: 20px 18px 48px 18px;
}

.help-page-card {
  padding: 22px 28px 32px 28px;
  height: auto;
  overflow: visible;
}

.help-page-top {
  display: flex;
  flex-direction: column;
  gap: 12px;
  margin-bottom: 16px;
}

.help-page-title {
  font-size: 20px;
  font-weight: 700;
  color: #f8fafc;
  margin: 0;
}

.help-page-markdown {
  overflow: visible;
  padding-right: 0;
  font-size: 15px;
  line-height: 1.7;
}

.help-figure {
  margin: 16px 0 20px 0;
  background-color: #0b1120;
  border: 1px solid var(--panel-border);
  border-radius: 10px;
  padding: 10px;
  box-shadow: 0 8px 24px rgba(0, 0, 0, 0.35);
}

.help-figure img {
  display: block;
  width: 100%;
  max-width: 100%;
  height: auto;
  border-radius: 6px;
  border: 1px solid rgba(148, 163, 184, 0.2);
}

.help-figure figcaption {
  margin-top: 8px;
  font-size: 12.5px;
  color: var(--text-muted);
  text-align: center;
}


)rawcss";

inline const char* APP_JS = R"rawjs(
document.addEventListener('DOMContentLoaded', () => {
  if (typeof window !== 'undefined') {
    window.setStatus = setStatus;
    window.openFeedbackModal = openFeedbackModal;
    window.submitSentryFeedback = submitSentryFeedback;
    window.initWasmWorker = initWasmWorker;
  }

  // DOM Elements
  const trumpSelect = document.getElementById('trumpSelect');
  const leaderSelect = document.getElementById('leaderSelect');
  const targetTricks = document.getElementById('targetTricks');
  const numIters = document.getElementById('numIters');
  const presetSelect = document.getElementById('presetSelect');
  const solveBtn = document.getElementById('solveBtn');
  const stopSolveBtn = document.getElementById('stopSolveBtn');
  const stopSolveActionBtn = document.getElementById('stopSolveActionBtn');

  const inputNorth = document.getElementById('inputNorth');
  const inputEast = document.getElementById('inputEast');
  const inputSouth = document.getElementById('inputSouth');
  const inputWest = document.getElementById('inputWest');
  const inputUnknownCards = document.getElementById('inputUnknownCards');

  const statusBox = document.getElementById('statusBox');
  const statusMessage = document.getElementById('statusMessage');
  const statusFeedbackBtn = document.getElementById('statusFeedbackBtn');
  const statusProgress = document.getElementById('statusProgress');
  const statusProgressFill = document.getElementById('statusProgressFill');
  const execTime = document.getElementById('execTime');
  let lastStatusErrorText = '';
  const resultsSummary = document.getElementById('resultsSummary');
  const statTricks = document.getElementById('statTricks');
  const statLeadLabel = document.getElementById('statLeadLabel');
  const statLead = document.getElementById('statLead');
  const cardStatSuccess = document.getElementById('cardStatSuccess');
  const cardStatFailure = document.getElementById('cardStatFailure');
  const cardStatActual = document.getElementById('cardStatActual');
  const statSuccess = document.getElementById('statSuccess');

  // Session Custom Hands & Active Hand Source State
  let loadedHandSource = null;
  let activeLoadedFileInfo = null;
  let pendingModalFileInfo = null;
  const sessionCustomHands = [];

  function formatCleanBoardLabel(rawBoard) {
    if (rawBoard === null || rawBoard === undefined || rawBoard === '') return '1';
    const str = String(rawBoard).trim();
    const clean = str.replace(/^(board\s*#?|hand\s*#?|#)/i, '').trim();
    return clean || str || '1';
  }

  function captureCurrentDealState() {
    const wHcpLowEl = document.getElementById('wHcpLow');
    const wHcpHighEl = document.getElementById('wHcpHigh');
    return {
      north: inputNorth ? inputNorth.value : '',
      east: inputEast ? inputEast.value : '',
      south: inputSouth ? inputSouth.value : '',
      west: inputWest ? inputWest.value : '',
      unknown_cards: inputUnknownCards ? inputUnknownCards.value : '',
      trump: trumpSelect ? trumpSelect.value : 'SPADES',
      leader: leaderSelect ? leaderSelect.value : 'EAST',
      target: targetTricks ? targetTricks.value : '1',
      iters: numIters ? numIters.value : '100',
      cur_trick: Array.isArray(presetCurTrick) ? presetCurTrick.slice() : [],
      loadedBoardHands: loadedBoardHands ? { ...loadedBoardHands } : null,
      loadedActualDeal: loadedActualDeal ? { ...loadedActualDeal } : null,
      importedSuitVoids: {
        E: new Set(importedSuitVoids && importedSuitVoids.E ? importedSuitVoids.E : []),
        W: new Set(importedSuitVoids && importedSuitVoids.W ? importedSuitVoids.W : [])
      },
      declarerSeat: currentDeclarerSeat || 'N',
      wHcpLow: wHcpLowEl ? wHcpLowEl.value : '',
      wHcpHigh: wHcpHighEl ? wHcpHighEl.value : ''
    };
  }

  function registerSessionCustomHand(sourceObj) {
    if (!sourceObj) {
      loadedHandSource = null;
      return;
    }

    const fileName = sourceObj.fileName || sourceObj.file_name || 'file';
    const rawBoard = sourceObj.boardLabel || sourceObj.board_label || '1';
    const cleanBoard = formatCleanBoardLabel(rawBoard);
    const fileFormat = sourceObj.fileFormat || sourceObj.file_format || 'pdf';
    const loadType = sourceObj.loadType || sourceObj.load_type || fileFormat;
    const label = `Hand ${cleanBoard} from ${fileName}`;
    const key = `custom:${fileName}:${cleanBoard}`;

    const source = {
      fileName,
      boardLabel: cleanBoard,
      fileFormat,
      loadType,
      file_name: fileName,
      board_label: cleanBoard,
      file_format: fileFormat,
      load_type: loadType
    };

    const entry = {
      key,
      label,
      source,
      state: captureCurrentDealState()
    };

    const existingIdx = sessionCustomHands.findIndex(h => h.key === key);
    if (existingIdx >= 0) {
      sessionCustomHands[existingIdx] = entry;
    } else {
      sessionCustomHands.push(entry);
    }

    loadedHandSource = source;
    renderHandSelectorOptions(key);
  }

  function updateActiveSessionCustomHandState() {
    if (!presetSelect || !presetSelect.value) return;
    const entry = sessionCustomHands.find(h => h.key === presetSelect.value);
    if (entry) {
      entry.state = captureCurrentDealState();
    }
  }

  function applySessionCustomHand(entry) {
    if (!entry || !entry.state) return;
    const s = entry.state;
    loadedHandSource = entry.source;
    loadedBoardHands = s.loadedBoardHands ? { ...s.loadedBoardHands } : null;
    loadedActualDeal = s.loadedActualDeal ? { ...s.loadedActualDeal } : null;
    importedSuitVoids = {
      E: new Set(s.importedSuitVoids && s.importedSuitVoids.E ? s.importedSuitVoids.E : []),
      W: new Set(s.importedSuitVoids && s.importedSuitVoids.W ? s.importedSuitVoids.W : [])
    };
    presetCurTrick = Array.isArray(s.cur_trick) ? s.cur_trick.slice() : [];
    updateDeclarerBarUI(s.declarerSeat || 'N');
    clearPanels();
    setStatus('idle', `Loaded ${entry.label}. Configure hands and click "Solve Deal".`);

    if (trumpSelect) trumpSelect.value = s.trump;
    if (leaderSelect) leaderSelect.value = s.leader;
    if (targetTricks) targetTricks.value = s.target;
    if (numIters) numIters.value = s.iters;

    if (inputNorth) inputNorth.value = s.north || '';
    if (inputEast) inputEast.value = s.east || '';
    if (inputSouth) inputSouth.value = s.south || '';
    if (inputWest) inputWest.value = s.west || '';
    if (inputUnknownCards) inputUnknownCards.value = s.unknown_cards || '';

    const wHcpLowEl = document.getElementById('wHcpLow');
    const wHcpHighEl = document.getElementById('wHcpHigh');
    if (wHcpLowEl) wHcpLowEl.value = s.wHcpLow || '';
    if (wHcpHighEl) wHcpHighEl.value = s.wHcpHigh || '';

    updateDealSvg();
  }
  const statFailure = document.getElementById('statFailure');
  const statActual = document.getElementById('statActual');
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

  let activeTabName = 'tree';
  let currentInspectedState = null;

  function activateTab(tabName) {
    activeTabName = tabName;
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
      currentInspectedState = null;
      activeTrickCardStates = null;
      updateDealSvg();
      if (treeRootGroup) {
        treeRootGroup.innerHTML =
          `<text x="50%" y="50%" text-anchor="middle" fill="#f87171">No decision tree generated.</text>`;
      }
    }
  }

  function updateActualStat(data) {
    if (!cardStatActual || !statActual) return;
    if (data && data.actual_hand) {
      cardStatActual.classList.remove('hidden');
      const makes = Boolean(data.actual_hand.succeeded);
      cardStatActual.classList.toggle('actual-makes', makes);
      cardStatActual.classList.toggle('actual-fails', !makes);
      const tricksPart = data.actual_hand.ns_tricks !== undefined
        ? ` (${data.actual_hand.ns_tricks})`
        : '';
      statActual.textContent = `${makes ? 'MAKES' : 'FAILS'}${tricksPart}`;
    } else {
      cardStatActual.classList.add('hidden');
      cardStatActual.classList.remove('active', 'actual-makes', 'actual-fails');
      statActual.textContent = '-';
      if (activeViewMode === 'actual') {
        activeViewMode = 'success';
      }
    }
  }

  function updateViewMode(mode) {
    if (mode === 'actual' && (!currentSolveData || !currentSolveData.actual_hand)) {
      mode = 'success';
    }
    activeViewMode = mode;
    const total = (currentSolveData && currentSolveData.total_boards) ? currentSolveData.total_boards : 1;
    const success = (currentSolveData && currentSolveData.success_boards !== undefined)
      ? currentSolveData.success_boards
      : ((currentSolveData && currentSolveData.covered_boards) || 0);
    const failure = (currentSolveData && currentSolveData.failure_boards !== undefined)
      ? currentSolveData.failure_boards
      : Math.max(0, total - success);

    if (cardStatSuccess) cardStatSuccess.classList.toggle('active', mode === 'success');
    if (cardStatFailure) cardStatFailure.classList.toggle('active', mode === 'failure');
    if (cardStatActual) cardStatActual.classList.toggle('active', mode === 'actual');

    if (mode === 'actual' && currentSolveData && currentSolveData.actual_hand) {
      const ah = currentSolveData.actual_hand;
      const statusText = ah.succeeded ? 'MAKES' : 'FAILS';
      const tricksText = ah.ns_tricks !== undefined ? `, ${ah.ns_tricks} tricks won by N/S` : '';
      if (summaryModeBadge) {
        summaryModeBadge.className = 'mode-badge mode-actual';
        summaryModeBadge.textContent = 'Actual Deal (#0)';
      }
      if (sampleHandsModeBadge) {
        sampleHandsModeBadge.className = 'mode-badge mode-actual';
        sampleHandsModeBadge.textContent = 'Actual Deal (#0)';
      }
      if (summaryModeSub) {
        summaryModeSub.textContent = `(${statusText}${tricksText})`;
      }
      if (sampleHandsModeSub) {
        sampleHandsModeSub.textContent =
          `(${statusText}${tricksText} — click card to trace #0 in Decision Tree)`;
      }
      if (summaryModeHeader) summaryModeHeader.classList.remove('hidden');
      if (sampleHandsModeHeader) sampleHandsModeHeader.classList.remove('hidden');

      const distSummary = ah.succeeded
        ? currentSolveData.distribution_summary
        : currentSolveData.failure_distribution_summary;
      if (distSummary) {
        renderDistributionSummary(distSummary);
      } else {
        clearDistributionSummary(`Actual Deal (#0) ${statusText}${tricksText}.`);
      }
      renderSampleHands([ah]);
      return;
    }

    if (mode === 'success') {
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
            `(${success}/${total} boards, ${pct}% — showing ${sampleList.length}${currentSolveData.actual_hand ? ' + Actual #0' : ''})`;
        }
      }
      if (summaryModeHeader) summaryModeHeader.classList.remove('hidden');
      if (sampleHandsModeHeader) sampleHandsModeHeader.classList.remove('hidden');

      if (currentSolveData && currentSolveData.distribution_summary) {
        renderDistributionSummary(currentSolveData.distribution_summary);
      } else {
        clearDistributionSummary('No distribution summary available for success deals.');
      }

      const handsToShow = [];
      if (currentSolveData && currentSolveData.actual_hand) {
        handsToShow.push(currentSolveData.actual_hand);
      }
      if (currentSolveData && Array.isArray(currentSolveData.sample_hands)) {
        handsToShow.push(...currentSolveData.sample_hands);
      }
      if (handsToShow.length > 0) {
        renderSampleHands(handsToShow);
      } else {
        clearSampleHands('No covered sample hands available.');
      }
    } else {
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
            `(${failure}/${total} boards, ${pct}% — showing ${failSampleList.length}${currentSolveData.actual_hand ? ' + Actual #0' : ''})`;
        }
      }
      if (summaryModeHeader) summaryModeHeader.classList.remove('hidden');
      if (sampleHandsModeHeader) sampleHandsModeHeader.classList.remove('hidden');

      if (currentSolveData && currentSolveData.failure_distribution_summary) {
        renderDistributionSummary(currentSolveData.failure_distribution_summary);
      } else {
        clearDistributionSummary('No failure distribution summary available (100% success rate or no failure deals).');
      }

      const failHandsToShow = [];
      if (currentSolveData && currentSolveData.actual_hand) {
        failHandsToShow.push(currentSolveData.actual_hand);
      }
      if (currentSolveData && Array.isArray(currentSolveData.failure_sample_hands)) {
        failHandsToShow.push(...currentSolveData.failure_sample_hands);
      }
      if (failHandsToShow.length > 0) {
        renderSampleHands(failHandsToShow);
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

  if (cardStatActual) {
    const activateActualMode = () => {
      updateViewMode('actual');
      if (panelDecisionTree && !panelDecisionTree.classList.contains('hidden')) {
        traceHandByNumber(0, false);
      }
    };
    cardStatActual.addEventListener('click', activateActualMode);
    cardStatActual.addEventListener('keydown', (e) => {
      if (e.key === 'Enter' || e.key === ' ') {
        activateActualMode();
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
          actual_hand: baseData.actual_hand,
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

    emitSentryMetricCount('equivalent_line_switched', 1, {
      line_kind: entry.kind,
      line_index: idx
    });

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
        if (lineObj.actual_hand) {
          baseCopy.actual_hand = lineObj.actual_hand;
        }

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
    updateActualStat(currentSolveData);
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
      iters: 100,
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
      iters: 100,
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
      iters: 100,
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
      iters: 100,
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
      iters: 100,
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
      iters: 100,
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
      iters: 100,
      north: 'S AJ97; H A2; D A2; C A2;',
      east: '',
      south: 'S KT86; H K3; D K3; C K3;',
      west: '',
      unknown_cards: 'S Q5432 H 98765 D 98765 C 98765'
    },
    hand_8_261014: {
      name: "Hand 8 from 261014",
      trump: 'HEARTS',
      leader: 'WEST',
      cur_trick: ['CJ'],
      target: 9,
      iters: 300,
      north: 'S Q; H KJT52; D J83; C KQ9;',
      east: '',
      south: 'S KT9543; H A97; D K; C 75;',
      west: 'S -; H -; D -; C J;',
      unknown_cards: 'S AJ8762 H Q8643 D AQT976542 C T86'
    }
  };

  // Dual-Mode Backend Adapter: uses C++ HTTP server (/api/...) when available,
  // and automatically falls back to client-side WebAssembly (solver_worker.js)
  // when hosted statically (e.g. GitHub Pages).
  let backendMode = null; // 'server' | 'wasm'
  let backendDetectPromise = null;
  let wasmWorker = null;
  let wasmNextRequestId = 1;
  let wasmActiveRequest = null;
  let wasmLatestStatus = { status: 'Idle', progress: -1 };

  function initWasmWorker() {
    if (wasmWorker) return wasmWorker;
    wasmLatestStatus = { status: 'Loading WebAssembly solver...', progress: -1 };
    wasmWorker = new Worker('solver_worker.js');
    wasmWorker.onmessage = (e) => {
      const msg = e.data;
      if (!msg || typeof msg !== 'object') return;
      if (msg.type === 'ready') {
        wasmLatestStatus = { status: 'Idle', progress: -1 };
      } else if (msg.type === 'progress') {
        wasmLatestStatus = {
          status: msg.status || 'Solving...',
          progress: (typeof msg.progress === 'number') ? msg.progress : -1
        };
      } else if (msg.type === 'solve_result') {
        if (wasmActiveRequest && wasmActiveRequest.id === msg.id) {
          const req = wasmActiveRequest;
          wasmActiveRequest = null;
          try {
            const parsed = JSON.parse(msg.resultJson || '{}');
            if (parsed && parsed.success === false && typeof parsed.error === 'string' &&
                parsed.error.startsWith('WebAssembly solver exception:')) {
              reportRuntimeErrorToSentry(new Error(parsed.error), {
                source: 'wasm_solver_exception'
              });
            }
            req.resolve({
              ok: parsed.success !== false,
              json: async () => parsed
            });
          } catch (err) {
            reportRuntimeErrorToSentry(err, { source: 'wasm_invalid_json' });
            req.resolve({
              ok: false,
              json: async () => ({
                success: false,
                error: 'Invalid JSON from WebAssembly solver'
              })
            });
          }
        }
      } else if (msg.type === 'error') {
        const errMsg = msg.error || 'WebAssembly worker error';
        reportRuntimeErrorToSentry(new Error(errMsg), { source: 'wasm_worker_abort' });
        setStatus('error', 'WebAssembly worker error: ' + errMsg);
        if (wasmActiveRequest) {
          const req = wasmActiveRequest;
          wasmActiveRequest = null;
          req.resolve({
            ok: false,
            json: async () => ({
              success: false,
              error: errMsg
            })
          });
        }
      }
    };
    wasmWorker.onerror = (err) => {
      const errMsg = 'Web Worker error: ' +
        (err && err.message ? err.message : 'failed to load solver_worker.js');
      reportRuntimeErrorToSentry(new Error(errMsg), { source: 'wasm_worker_onerror' });
      setStatus('error', errMsg);
      if (wasmActiveRequest) {
        const req = wasmActiveRequest;
        wasmActiveRequest = null;
        req.resolve({
          ok: false,
          json: async () => ({
            success: false,
            error: errMsg
          })
        });
      }
    };
    return wasmWorker;
  }

  const WASM_NUM_THREADS = 4;

  async function ensureCrossOriginIsolation() {
    if (window.crossOriginIsolated || typeof SharedArrayBuffer !== 'undefined') {
      try {
        sessionStorage.removeItem('bridge_coi_reloaded');
      } catch (_) {}
      return true;
    }
    if (!('serviceWorker' in navigator) || !window.isSecureContext) {
      return false;
    }
    try {
      const reg = await navigator.serviceWorker.register('coi_serviceworker.js');
      if (reg && (reg.installing || reg.waiting || reg.active)) {
        if (!sessionStorage.getItem('bridge_coi_reloaded')) {
          sessionStorage.setItem('bridge_coi_reloaded', '1');
          if (navigator.serviceWorker.controller) {
            window.location.reload();
          } else {
            navigator.serviceWorker.addEventListener('controllerchange', () => {
              window.location.reload();
            }, { once: true });
          }
          await new Promise((r) => setTimeout(r, 300));
        }
      }
    } catch (_) {}
    return typeof SharedArrayBuffer !== 'undefined';
  }

  function configureWasmThreadsUI() {
    const numThreadsSelect = document.getElementById('numThreads');
    if (!numThreadsSelect) return;
    numThreadsSelect.value = String(WASM_NUM_THREADS);
    numThreadsSelect.disabled = true;
    numThreadsSelect.title =
      'Running client-side in WebAssembly (' + WASM_NUM_THREADS + ' worker threads)';
  }

  async function ensureBackendMode() {
    if (backendMode) return backendMode;
    if (backendDetectPromise) return backendDetectPromise;
    backendDetectPromise = (async () => {
      if (window.BRIDGE_FORCE_WASM === true) {
        backendMode = 'wasm';
        setSentryGlobalAttribute('backend_mode', backendMode);
        configureWasmThreadsUI();
        await ensureCrossOriginIsolation();
        initWasmWorker();
        return backendMode;
      }
      try {
        const res = await fetch('/api/health', { cache: 'no-store' });
        if (res.ok) {
          const data = await res.json();
          if (data && data.status === 'ok') {
            backendMode = 'server';
            setSentryGlobalAttribute('backend_mode', backendMode);
            return backendMode;
          }
        }
      } catch (_) {}
      backendMode = 'wasm';
      setSentryGlobalAttribute('backend_mode', backendMode);
      configureWasmThreadsUI();
      await ensureCrossOriginIsolation();
      initWasmWorker();
      return backendMode;
    })();
    return backendDetectPromise;
  }

  async function apiSolve(payload, signal) {
    const mode = await ensureBackendMode();
    if (mode === 'server') {
      return fetch('/api/solve', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(payload),
        signal
      });
    }
    const worker = initWasmWorker();
    const reqId = wasmNextRequestId++;
    wasmLatestStatus = { status: 'Starting solve...', progress: -1 };
    return new Promise((resolve) => {
      wasmActiveRequest = { id: reqId, resolve };
      if (signal) {
        signal.addEventListener('abort', () => {
          if (wasmActiveRequest && wasmActiveRequest.id === reqId) {
            wasmActiveRequest = null;
            resolve({
              ok: false,
              json: async () => ({
                success: false,
                stopped: true,
                error: 'Solve stopped by user.'
              })
            });
          }
        }, { once: true });
      }
      worker.postMessage({
        type: 'solve',
        id: reqId,
        payloadJson: JSON.stringify(
          Object.assign({}, payload, { num_threads: WASM_NUM_THREADS })
        )
      });
    });
  }

  async function apiSolveStatus() {
    const mode = await ensureBackendMode();
    if (mode === 'server') {
      return fetch('/api/solve_status');
    }
    const st = { status: wasmLatestStatus.status || 'Solving...' };
    if (typeof wasmLatestStatus.progress === 'number' && wasmLatestStatus.progress >= 0) {
      st.progress = wasmLatestStatus.progress;
    }
    return {
      ok: true,
      json: async () => st
    };
  }

  function apiStopSolve() {
    if (backendMode === 'wasm') {
      if (wasmWorker) {
        wasmWorker.terminate();
        wasmWorker = null;
      }
      if (wasmActiveRequest) {
        const req = wasmActiveRequest;
        wasmActiveRequest = null;
        req.resolve({
          ok: false,
          json: async () => ({
            success: false,
            stopped: true,
            error: 'Solve stopped by user.'
          })
        });
      }
      wasmLatestStatus = { status: 'Solve stopped.', progress: -1 };
      initWasmWorker();
      return;
    }
    try {
      fetch('/api/stop_solve', { method: 'POST' }).catch(() => {});
    } catch (_) {}
  }

  async function fetchPresets() {
    try {
      const res = await fetch('/api/presets', { cache: 'no-cache' });
      if (res.ok) {
        const loaded = await res.json();
        if (loaded && typeof loaded === 'object' && Object.keys(loaded).length > 0) {
          PRESETS = loaded;
          populatePresetSelect();
          return;
        }
      }
    } catch (e) {
      // Try static presets.json fallback next
    }
    try {
      const res = await fetch('presets.json', { cache: 'no-cache' });
      if (res.ok) {
        const loaded = await res.json();
        if (loaded && typeof loaded === 'object' && Object.keys(loaded).length > 0) {
          PRESETS = loaded;
          populatePresetSelect();
          ensureBackendMode();
          return;
        }
      }
    } catch (e) {
      // Fall back to default embedded PRESETS
    }
    ensureBackendMode();
  }

  function renderHandSelectorOptions(selectedVal) {
    if (!presetSelect) return;
    const targetVal = selectedVal !== undefined ? selectedVal : presetSelect.value;
    presetSelect.innerHTML = '';

    if (sessionCustomHands.length > 0) {
      const customGroup = document.createElement('optgroup');
      customGroup.label = 'Loaded Hands';
      for (const h of sessionCustomHands) {
        const opt = document.createElement('option');
        opt.value = h.key;
        opt.textContent = h.label;
        customGroup.appendChild(opt);
      }
      presetSelect.appendChild(customGroup);

      const presetGroup = document.createElement('optgroup');
      presetGroup.label = 'Presets';
      for (const [key, p] of Object.entries(PRESETS)) {
        const opt = document.createElement('option');
        opt.value = key;
        opt.textContent = p.name || key;
        presetGroup.appendChild(opt);
      }
      presetSelect.appendChild(presetGroup);
    } else {
      for (const [key, p] of Object.entries(PRESETS)) {
        const opt = document.createElement('option');
        opt.value = key;
        opt.textContent = p.name || key;
        presetSelect.appendChild(opt);
      }
    }

    const hasCustom = sessionCustomHands.some(h => h.key === targetVal);
    if (hasCustom || PRESETS[targetVal]) {
      presetSelect.value = targetVal;
    } else if (presetSelect.options.length > 0) {
      presetSelect.value = presetSelect.options[0].value;
    }
  }

  function populatePresetSelect() {
    if (!presetSelect) return;
    renderHandSelectorOptions();
    if (presetSelect.value && PRESETS[presetSelect.value]) {
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
        cnt += val.replace(/10/g, 'T').length;
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
        if (tokenItems.length > 8) {
          textNode.setAttribute('textLength', isUnknown ? '84' : '106');
          textNode.setAttribute('lengthAdjust', 'spacingAndGlyphs');
        }
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
      (activeTracedDeal.E || activeTracedDeal.W)
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
    currentInspectedState = null;
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
    updateActualStat(null);
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

  let loadedBoardHands = null; // Stores original { N, E, S, W } compass hands when a 4-hand deal is scanned/loaded
  let loadedActualDeal = null; // Stores rotated { N: decl, E: lho, S: dummy, W: rho } for Board #0 (Actual)
  let importedSuitVoids = { E: new Set(), W: new Set() }; // Voids revealed during imported initial play
  let currentDeclarerSeat = 'N';
  let modalContractLevel = 4;
  let modalContractStrain = 'SPADES';

  function combineHandsToUnknownString(handA, handB) {
    const suitsA = parseHandToSuits(handA || '');
    const suitsB = parseHandToSuits(handB || '');
    const rankOrder = ['A', 'K', 'Q', 'J', 'T', '9', '8', '7', '6', '5', '4', '3', '2'];
    const parts = [];
    ['S', 'H', 'D', 'C'].forEach(s => {
      const combined = ((suitsA[s] || '') + (suitsB[s] || '')).replace(/10/g, 'T').toUpperCase();
      const sorted = rankOrder.filter(r => combined.includes(r)).join('');
      if (sorted.length > 0) {
        parts.push(`${s} ${sorted}`);
      }
    });
    return parts.join(' ');
  }

  function getActiveActualDeal() {
    if (!loadedActualDeal || (!loadedActualDeal.E && !loadedActualDeal.W)) {
      return null;
    }
    const curUnk = inputUnknownCards ? inputUnknownCards.value.trim() : '';
    if (!curUnk) return null;
    const eVal = inputEast ? inputEast.value.trim() : '';
    const wVal = inputWest ? inputWest.value.trim() : '';
    const expectedUnk = combineHandsToUnknownString(loadedActualDeal.E, loadedActualDeal.W);
    const normCurUnk = combineHandsToUnknownString(curUnk, combineHandsToUnknownString(eVal, wVal));
    if (expectedUnk !== normCurUnk) return null;
    return {
      N: inputNorth ? inputNorth.value.trim() : loadedActualDeal.N,
      E: loadedActualDeal.E,
      S: inputSouth ? inputSouth.value.trim() : loadedActualDeal.S,
      W: loadedActualDeal.W
    };
  }

  function updateDeclarerBarUI(seat) {
    currentDeclarerSeat = seat;
    const seatNames = { N: 'North', E: 'East', S: 'South', W: 'West' };
    const order = ['N', 'E', 'S', 'W'];
    const idx = order.indexOf(seat);
    const dummySeat = order[(idx + 2) % 4];
    const lhoSeat = order[(idx + 1) % 4];
    const rhoSeat = order[(idx + 3) % 4];

    document.querySelectorAll('.declarer-seat-btn').forEach(btn => {
      btn.classList.toggle('active', btn.dataset.seat === seat);
    });
    document.querySelectorAll('.modal-declarer-btn').forEach(btn => {
      btn.classList.toggle('active', btn.dataset.seat === seat);
    });

    const declarerMappingHint = document.getElementById('declarerMappingHint');
    if (declarerMappingHint) {
      declarerMappingHint.textContent = (seat === 'N')
        ? 'N=Decl · S=Dummy'
        : `N=${seat}(Decl) · S=${dummySeat}(Dummy)`;
    }

    const modalDeclarerHint = document.getElementById('modalDeclarerHint');
    if (modalDeclarerHint) {
      modalDeclarerHint.textContent = `${seatNames[seat]} → Hand 0 (N) · ${seatNames[dummySeat]} → Hand 2 (S)`;
    }

    const roleMap = {
      [seat]: { text: 'Declarer (→ N)', cls: 'scan-role-tag role-declarer' },
      [dummySeat]: { text: 'Dummy (→ S)', cls: 'scan-role-tag role-dummy' },
      [lhoSeat]: { text: 'Defender (LHO → Unk)', cls: 'scan-role-tag' },
      [rhoSeat]: { text: 'Defender (RHO → Unk)', cls: 'scan-role-tag' }
    };
    ['N', 'E', 'S', 'W'].forEach(s => {
      const el = document.getElementById(`scanRole${s}`);
      if (el && roleMap[s]) {
        el.textContent = roleMap[s].text;
        el.className = roleMap[s].cls;
      }
    });

    if (typeof updateModalContractAndPlayUI === 'function') {
      updateModalContractAndPlayUI();
    }
  }

  function applyDeclarerRotation(seat, fromModal = false) {
    const order = ['N', 'E', 'S', 'W'];
    const idx = order.indexOf(seat);
    if (idx === -1) return;

    // If loadedBoardHands is not yet set, capture from inputs if all 4 hands are present
    if (!loadedBoardHands) {
      const nVal = inputNorth.value.trim();
      const eVal = inputEast.value.trim();
      const sVal = inputSouth.value.trim();
      const wVal = inputWest.value.trim();
      if (countHandCards(nVal) > 0 && countHandCards(eVal) > 0 &&
          countHandCards(sVal) > 0 && countHandCards(wVal) > 0) {
        loadedBoardHands = { N: nVal, E: eVal, S: sVal, W: wVal };
      } else if (seat === 'S' && countHandCards(nVal) > 0 && countHandCards(sVal) > 0 &&
                 countHandCards(eVal) === 0 && countHandCards(wVal) === 0) {
        // Flip N and S on single-dummy preset
        inputNorth.value = sVal;
        inputSouth.value = nVal;
        updateDeclarerBarUI(seat);
        clearPanels();
        updateDealSvg();
        return;
      } else {
        updateDeclarerBarUI(seat);
        return;
      }
    }

    const declSeat = order[idx];
    const lhoSeat = order[(idx + 1) % 4];
    const dummySeat = order[(idx + 2) % 4];
    const rhoSeat = order[(idx + 3) % 4];

    if (fromModal && typeof simulateModalInitialPlay === 'function') {
      const sim = simulateModalInitialPlay(loadedBoardHands, seat, modalContractStrain);
      trumpSelect.value = modalContractStrain;
      const baseTarget = 6 + modalContractLevel;
      const remTarget = Math.max(1, baseTarget - sim.nsCompletedTricksWon);
      targetTricks.value = remTarget;

      const rotatedLeaderIdx = (sim.currentLeaderOrigIdx - idx + 4) % 4;
      leaderSelect.value = getSeatFullString(rotatedLeaderIdx);

      const declAfterComp = formatSuitsToHandStr(sim.remHandsAfterCompleted[declSeat], false);
      const lhoAfterComp = formatSuitsToHandStr(sim.remHandsAfterCompleted[lhoSeat], false);
      const dummyAfterComp = formatSuitsToHandStr(sim.remHandsAfterCompleted[dummySeat], false);
      const rhoAfterComp = formatSuitsToHandStr(sim.remHandsAfterCompleted[rhoSeat], false);

      loadedActualDeal = {
        N: declAfterComp,
        E: lhoAfterComp,
        S: dummyAfterComp,
        W: rhoAfterComp
      };

      const partialDef = {
        E: { S: [], H: [], D: [], C: [] },
        W: { S: [], H: [], D: [], C: [] }
      };
      sim.currentTrickCards.forEach(c => {
        if (c.rotatedSeatIdx === 1) {
          partialDef.E[c.suit].push(c.rankStr === '10' ? 'T' : c.rankStr);
        } else if (c.rotatedSeatIdx === 3) {
          partialDef.W[c.suit].push(c.rankStr === '10' ? 'T' : c.rankStr);
        }
      });
      const hasPartialE = ['S', 'H', 'D', 'C'].some(s => partialDef.E[s].length > 0);
      const hasPartialW = ['S', 'H', 'D', 'C'].some(s => partialDef.W[s].length > 0);

      const lhoAfterAll = formatSuitsToHandStr(sim.remHandsAfterAll[lhoSeat], false);
      const rhoAfterAll = formatSuitsToHandStr(sim.remHandsAfterAll[rhoSeat], false);

      importedSuitVoids = {
        E: new Set(sim.voidSuits.E),
        W: new Set(sim.voidSuits.W)
      };
      presetCurTrick = sim.currentTrickCards.map(c => c.code);
      clearPanels();

      inputNorth.value = declAfterComp;
      inputSouth.value = dummyAfterComp;
      inputEast.value = hasPartialE ? formatSuitsToHandStr(partialDef.E, false) : '';
      inputWest.value = hasPartialW ? formatSuitsToHandStr(partialDef.W, false) : '';
      if (inputUnknownCards) {
        inputUnknownCards.value = combineHandsToUnknownString(lhoAfterAll, rhoAfterAll);
      }

      updateDeclarerBarUI(seat);
      const strainSym = getSuitSymbol(modalContractStrain);
      const contractDesc = `${modalContractLevel}${strainSym} by ${declSeat}`;
      const playDesc = (sim.completedTricks.length > 0 || sim.currentTrickCards.length > 0)
        ? ` (${sim.completedTricks.length} completed trick${sim.completedTricks.length !== 1 ? 's' : ''}` +
          (sim.currentTrickCards.length > 0 ? `, current trick: ${presetCurTrick.join(' ')}` : '') + ')'
        : '';
      setStatus('idle', `Loaded ${contractDesc}${playDesc}. Declarer rotated to North, Dummy (${dummySeat}) to South.`);
      updateDealSvg();
      return;
    }

    const declHand = loadedBoardHands[declSeat] || '';
    const lhoHand = loadedBoardHands[lhoSeat] || '';
    const dummyHand = loadedBoardHands[dummySeat] || '';
    const rhoHand = loadedBoardHands[rhoSeat] || '';

    loadedActualDeal = {
      N: declHand,
      E: lhoHand,
      S: dummyHand,
      W: rhoHand
    };

    importedSuitVoids = { E: new Set(), W: new Set() };
    presetCurTrick = [];
    leaderSelect.value = 'EAST';
    clearPanels();
    inputNorth.value = declHand;
    inputSouth.value = dummyHand;
    inputEast.value = '';
    inputWest.value = '';
    if (inputUnknownCards) {
      inputUnknownCards.value = combineHandsToUnknownString(lhoHand, rhoHand);
    }

    updateDeclarerBarUI(seat);
    setStatus('idle', `Loaded board with ${declSeat} as Declarer (rotated to North) and ${dummySeat} as Dummy (South).`);
    updateDealSvg();
  }

  document.querySelectorAll('.declarer-seat-btn').forEach(btn => {
    btn.addEventListener('click', () => {
      applyDeclarerRotation(btn.dataset.seat, false);
      updateActiveSessionCustomHandState();
    });
  });

  function loadPreset(key, isInitialLoad = false) {
    const customEntry = sessionCustomHands.find(h => h.key === key);
    if (customEntry) {
      applySessionCustomHand(customEntry);
      if (!isInitialLoad) {
        emitSentryMetricCount('hand_loaded', 1, {
          load_type: 'session_custom',
          preset: customEntry.key
        });
      }
      return;
    }
    loadedHandSource = null;
    const p = PRESETS[key];
    if (!p) return;

    loadedBoardHands = null;
    loadedActualDeal = null;
    importedSuitVoids = { E: new Set(), W: new Set() };
    updateDeclarerBarUI('N');
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
    if (!isInitialLoad) {
      emitSentryMetricCount('hand_loaded', 1, {
        load_type: 'preset',
        preset: key
      });
    }
  }

  // Load Hand on Dropdown Selection
  if (presetSelect) {
    presetSelect.addEventListener('change', () => {
      loadPreset(presetSelect.value);
    });
  }

  // Load Photo / Image or TXT / Deal File Handler
  const loadTxtBtn = document.getElementById('loadTxtBtn');
  const recropPhotoBtn = document.getElementById('recropPhotoBtn');
  const fileInputTxt = document.getElementById('fileInputTxt');

  if (loadTxtBtn && fileInputTxt) {
    loadTxtBtn.addEventListener('click', () => {
      fileInputTxt.value = '';
      fileInputTxt.click();
    });

    fileInputTxt.addEventListener('change', (e) => {
      const file = e.target.files[0];
      if (!file) return;

      const ext = (file.name || '').split('.').pop().toLowerCase();
      const isPdf = (file.type === 'application/pdf') ||
                    /\.pdf$/i.test(file.name || '');
      if (isPdf) {
        pendingModalFileInfo = { fileName: file.name, fileFormat: 'pdf', loadType: 'pdf' };
        openPhotoScanModalWithPdfFile(file);
        return;
      }

      const isImage = (file.type && file.type.startsWith('image/')) ||
                      /\.(png|jpe?g|webp|gif|bmp)$/i.test(file.name || '');
      if (isImage) {
        pendingModalFileInfo = { fileName: file.name, fileFormat: 'image', loadType: 'image' };
        openPhotoScanModalWithFile(file);
        return;
      }

      const isPbnExt = /\.pbn$/i.test(file.name || '');
      const reader = new FileReader();
      reader.onload = (event) => {
        const text = event.target.result;
        if (isPbnExt || (text && text.includes('[Deal "'))) {
          pendingModalFileInfo = { fileName: file.name, fileFormat: 'pbn', loadType: 'pbn' };
          const pbnBoards = parsePbnBoardsFromText(text);
          if (pbnBoards.length > 0) {
            openPhotoScanModalWithPbnBoards(pbnBoards);
            return;
          }
        }
        activeLoadedFileInfo = { fileName: file.name, fileFormat: ext || 'txt', loadType: 'txt' };
        pendingModalFileInfo = null;
        parseAndApplyDealText(text, file.name);
      };
      reader.readAsText(file);
    });
  }

  if (recropPhotoBtn) {
    recropPhotoBtn.addEventListener('click', () => {
      openPhotoScanModalExisting();
    });
  }

  function parseAndApplyDealText(text, sourceFileName = null) {
    if (!text) return;
    loadedBoardHands = null;
    loadedActualDeal = null;
    importedSuitVoids = { E: new Set(), W: new Set() };
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

    const fileSource = sourceFileName ? { fileName: sourceFileName, fileFormat: 'txt', loadType: 'txt' } : activeLoadedFileInfo;
    const boardVal = keyValues.board || keyValues.board_number || keyValues.hand || '1';
    const sourceMeta = fileSource ? {
      fileName: fileSource.fileName,
      boardLabel: boardVal,
      fileFormat: fileSource.fileFormat || 'txt',
      loadType: fileSource.loadType || 'txt'
    } : null;

    if (countHandCards(inputNorth.value) > 0 && countHandCards(inputEast.value) > 0 &&
        countHandCards(inputSouth.value) > 0 && countHandCards(inputWest.value) > 0) {
      loadedBoardHands = {
        N: inputNorth.value.trim(),
        E: inputEast.value.trim(),
        S: inputSouth.value.trim(),
        W: inputWest.value.trim()
      };
      const declKey = (keyValues.declarer || '').trim().toUpperCase().charAt(0);
      if (['N', 'E', 'S', 'W'].includes(declKey)) {
        applyDeclarerRotation(declKey);
        if (sourceMeta) registerSessionCustomHand(sourceMeta);
        emitSentryMetricCount('hand_loaded', 1, {
          load_type: (sourceMeta && sourceMeta.loadType) || 'txt'
        });
        return;
      }
    }
    updateDeclarerBarUI('N');
    updateDealSvg();
    if (sourceMeta) registerSessionCustomHand(sourceMeta);
    emitSentryMetricCount('hand_loaded', 1, {
      load_type: (sourceMeta && sourceMeta.loadType) || 'txt'
    });
  }

  // =========================================================================
  // On-Device Bridge Hand Record OCR & Interactive Crop Modal
  // =========================================================================
  const OCR_TEMPLATE_DATA = {"aspect":{"2":0.739,"3":0.703,"4":0.78,"5":0.712,"6":0.718,"7":0.703,"8":0.728,"9":0.731,"K":0.902,"J":0.6,"A":1.016,"Q":0.985},"tpl":{"2":[0.031,0.09,0.118,0.092,0.051,0.02,0.008,0.049,0.098,0.119,0.109,0.061,0.061,0.114,0.118,0.055,0.003,0.001,0,0.007,0.056,0.107,0.119,0.091,0.079,0.119,0.108,0.034,0,0,0,0,0.027,0.087,0.12,0.109,0.049,0.08,0.059,0.012,0,0,0,0,0.023,0.083,0.12,0.112,0.002,0.003,0.002,0.001,0,0,0,0,0.045,0.105,0.12,0.095,0,0,0,0,0,0,0,0.015,0.081,0.119,0.119,0.074,0,0,0,0,0,0,0.001,0.049,0.112,0.119,0.095,0.036,0,0,0,0,0,0.017,0.053,0.098,0.109,0.078,0.033,0.004,0,0,0,0,0.023,0.08,0.116,0.108,0.058,0.016,0.001,0,0,0,0,0.022,0.082,0.115,0.12,0.071,0.014,0.001,0,0,0,0,0.011,0.069,0.12,0.118,0.088,0.022,0,0,0,0,0.004,0.031,0.074,0.107,0.095,0.059,0.019,0,0,0,0,0,0.035,0.095,0.119,0.091,0.023,0.001,0,0,0,0,0,0,0.075,0.12,0.12,0.097,0.06,0.06,0.06,0.06,0.06,0.06,0.06,0.051,0.103,0.12,0.12,0.12,0.12,0.12,0.12,0.12,0.12,0.12,0.12,0.111,0.118,0.12,0.12,0.12,0.12,0.12,0.12,0.12,0.12,0.12,0.12,0.12],"3":[0.053,0.112,0.126,0.093,0.044,0.011,0.033,0.088,0.126,0.119,0.069,0.013,0.085,0.126,0.113,0.044,0,0,0,0.036,0.11,0.126,0.1,0.037,0.1,0.125,0.093,0.02,0,0,0,0.015,0.086,0.126,0.111,0.048,0.034,0.041,0.028,0.005,0,0,0,0.018,0.088,0.126,0.105,0.042,0,0,0,0,0.001,0.001,0.014,0.067,0.117,0.122,0.075,0.016,0,0,0,0,0.032,0.064,0.076,0.113,0.123,0.088,0.028,0,0,0,0,0.005,0.07,0.12,0.121,0.126,0.123,0.077,0.025,0.008,0,0,0,0.003,0.036,0.052,0.056,0.077,0.111,0.121,0.101,0.051,0,0,0,0,0,0,0,0.01,0.07,0.118,0.126,0.092,0,0,0,0,0,0,0,0,0.029,0.092,0.126,0.111,0.049,0.049,0.028,0.002,0,0,0,0,0.015,0.078,0.126,0.119,0.114,0.117,0.073,0.01,0,0,0,0,0.029,0.092,0.126,0.11,0.093,0.124,0.109,0.05,0.009,0,0,0.022,0.079,0.12,0.121,0.078,0.04,0.101,0.125,0.106,0.065,0.049,0.049,0.087,0.122,0.122,0.085,0.027,0.011,0.049,0.098,0.12,0.114,0.112,0.112,0.121,0.115,0.083,0.032,0.001,0.001,0.008,0.041,0.074,0.084,0.109,0.096,0.077,0.059,0.025,0.004,0],"4":[0,0,0,0,0,0.029,0.1,0.13,0.129,0.066,0.002,0,0,0,0,0,0.005,0.066,0.125,0.129,0.129,0.066,0.003,0,0,0,0,0.001,0.032,0.102,0.116,0.117,0.129,0.067,0.003,0,0,0,0,0.03,0.093,0.109,0.061,0.087,0.129,0.067,0.002,0,0,0,0.008,0.08,0.126,0.067,0.01,0.077,0.13,0.066,0.002,0,0,0.001,0.037,0.113,0.105,0.032,0.006,0.077,0.13,0.066,0.002,0,0,0.013,0.08,0.119,0.062,0.007,0.007,0.077,0.13,0.066,0.002,0,0.018,0.069,0.114,0.071,0.013,0,0.007,0.078,0.13,0.066,0.002,0,0.066,0.119,0.094,0.016,0,0,0.007,0.077,0.13,0.066,0.001,0,0.105,0.129,0.101,0.064,0.064,0.064,0.068,0.103,0.13,0.097,0.064,0.06,0.127,0.13,0.13,0.13,0.13,0.13,0.13,0.13,0.13,0.13,0.13,0.126,0.083,0.087,0.087,0.087,0.087,0.087,0.089,0.112,0.13,0.108,0.087,0.084,0.002,0.002,0.002,0.002,0.002,0.002,0.009,0.078,0.13,0.067,0.003,0.002,0,0,0,0,0,0,0.007,0.077,0.13,0.065,0.001,0,0,0,0,0,0,0,0.007,0.077,0.13,0.065,0,0,0,0,0,0,0,0,0.007,0.077,0.13,0.065,0,0],"5":[0.007,0.067,0.121,0.105,0.067,0.06,0.06,0.06,0.06,0.06,0.06,0.03,0.022,0.084,0.123,0.08,0.009,0.002,0.002,0.002,0.002,0.002,0.002,0.001,0.031,0.093,0.123,0.069,0,0,0,0,0,0,0,0,0.044,0.105,0.121,0.056,0.008,0.032,0.042,0.022,0.001,0,0,0,0.066,0.121,0.12,0.085,0.071,0.107,0.122,0.093,0.061,0.031,0.001,0,0.078,0.123,0.123,0.122,0.12,0.121,0.121,0.122,0.122,0.093,0.038,0.006,0.087,0.123,0.119,0.092,0.057,0.05,0.05,0.077,0.114,0.123,0.102,0.046,0.054,0.073,0.061,0.025,0.003,0,0,0.011,0.06,0.11,0.123,0.097,0,0,0,0,0,0,0,0,0.019,0.081,0.123,0.119,0,0,0,0,0,0,0,0,0.005,0.067,0.123,0.123,0.028,0.028,0.016,0.001,0,0,0,0,0.004,0.066,0.123,0.123,0.1,0.102,0.063,0.008,0,0,0,0,0.022,0.083,0.123,0.11,0.103,0.123,0.101,0.037,0.005,0,0,0.012,0.065,0.113,0.12,0.075,0.063,0.114,0.12,0.087,0.041,0.027,0.027,0.063,0.109,0.121,0.094,0.035,0.024,0.073,0.108,0.115,0.095,0.089,0.089,0.109,0.113,0.088,0.045,0.008,0.004,0.021,0.063,0.095,0.099,0.11,0.104,0.096,0.073,0.033,0.008,0],"6":[0.011,0.059,0.105,0.101,0.066,0.027,0.006,0.045,0.094,0.114,0.101,0.046,0.035,0.092,0.114,0.072,0.012,0.001,0,0.007,0.057,0.105,0.113,0.067,0.061,0.11,0.106,0.042,0,0,0,0,0.023,0.079,0.112,0.069,0.084,0.114,0.077,0.014,0.001,0.001,0.001,0.001,0.003,0.021,0.038,0.021,0.103,0.114,0.06,0.018,0.05,0.056,0.056,0.056,0.044,0.016,0,0,0.111,0.111,0.068,0.062,0.105,0.11,0.11,0.112,0.1,0.07,0.026,0,0.113,0.112,0.098,0.097,0.084,0.056,0.052,0.085,0.108,0.112,0.079,0.027,0.113,0.114,0.108,0.06,0.016,0.003,0.002,0.018,0.064,0.105,0.11,0.073,0.112,0.114,0.094,0.025,0,0,0,0,0.02,0.078,0.114,0.1,0.111,0.114,0.08,0.015,0,0,0,0,0.006,0.063,0.114,0.111,0.099,0.114,0.077,0.013,0,0,0,0,0.006,0.062,0.113,0.11,0.08,0.114,0.094,0.026,0,0,0,0,0.017,0.073,0.114,0.1,0.052,0.103,0.109,0.059,0.008,0,0,0.009,0.055,0.101,0.11,0.069,0.019,0.068,0.106,0.1,0.055,0.026,0.026,0.05,0.099,0.112,0.088,0.034,0.004,0.029,0.07,0.106,0.099,0.084,0.084,0.097,0.108,0.091,0.05,0.01,0,0.004,0.021,0.067,0.087,0.099,0.102,0.089,0.075,0.045,0.014,0],"7":[0.084,0.084,0.084,0.084,0.084,0.084,0.084,0.088,0.13,0.169,0.167,0.133,0.006,0.006,0.006,0.006,0.006,0.006,0.008,0.041,0.124,0.167,0.138,0.065,0,0,0,0,0,0.003,0.019,0.092,0.161,0.152,0.084,0.017,0,0,0,0,0.001,0.015,0.081,0.151,0.151,0.087,0.021,0.001,0,0,0,0,0.004,0.052,0.147,0.168,0.101,0.017,0,0,0,0,0,0.002,0.008,0.085,0.167,0.144,0.057,0.001,0,0,0,0,0.001,0.006,0.024,0.116,0.166,0.105,0.015,0,0,0,0,0,0.004,0.014,0.075,0.153,0.138,0.052,0,0,0,0,0,0,0.006,0.039,0.126,0.165,0.095,0.01,0,0,0,0,0,0.001,0.006,0.06,0.148,0.151,0.062,0,0,0,0,0,0,0.004,0.01,0.081,0.16,0.13,0.035,0,0,0,0,0,0.001,0.006,0.029,0.114,0.163,0.1,0.009,0,0,0,0,0,0.003,0.007,0.048,0.135,0.163,0.075,0,0,0,0,0,0,0.004,0.008,0.059,0.144,0.161,0.066,0,0,0,0,0,0,0.008,0.01,0.071,0.154,0.159,0.061,0,0,0,0,0,0,0.011,0.011,0.084,0.161,0.152,0.054,0,0,0,0,0,0],"8":[0.02,0.074,0.111,0.09,0.042,0.015,0.012,0.059,0.106,0.111,0.074,0.019,0.044,0.1,0.113,0.058,0.002,0.001,0.001,0.021,0.085,0.113,0.1,0.044,0.057,0.112,0.112,0.041,0,0,0,0.004,0.062,0.113,0.111,0.055,0.05,0.106,0.112,0.048,0,0,0,0.007,0.067,0.113,0.109,0.053,0.022,0.078,0.113,0.086,0.025,0.001,0.001,0.043,0.097,0.112,0.082,0.026,0.003,0.032,0.085,0.11,0.082,0.057,0.057,0.091,0.112,0.084,0.03,0.002,0.002,0.011,0.064,0.112,0.11,0.101,0.102,0.111,0.109,0.062,0.011,0.002,0.03,0.072,0.101,0.088,0.058,0.034,0.037,0.069,0.098,0.1,0.072,0.027,0.077,0.113,0.103,0.04,0,0,0,0.009,0.062,0.105,0.113,0.071,0.102,0.113,0.081,0.017,0,0,0,0,0.022,0.079,0.113,0.097,0.112,0.113,0.066,0.006,0,0,0,0,0.006,0.062,0.113,0.109,0.104,0.113,0.076,0.013,0,0,0,0,0.012,0.069,0.113,0.107,0.074,0.11,0.103,0.04,0.004,0,0,0.006,0.049,0.098,0.112,0.081,0.038,0.091,0.113,0.082,0.038,0.023,0.023,0.046,0.096,0.113,0.096,0.043,0.01,0.054,0.095,0.104,0.089,0.081,0.081,0.094,0.11,0.096,0.055,0.013,0,0.016,0.046,0.079,0.089,0.101,0.101,0.089,0.078,0.048,0.014,0],"9":[0.035,0.09,0.113,0.09,0.045,0.016,0.012,0.059,0.105,0.108,0.066,0.014,0.065,0.113,0.11,0.052,0.002,0,0.001,0.016,0.078,0.113,0.098,0.042,0.084,0.113,0.094,0.025,0,0,0,0.001,0.044,0.099,0.113,0.068,0.107,0.113,0.069,0.008,0,0,0,0,0.019,0.076,0.113,0.093,0.112,0.113,0.065,0.006,0,0,0,0,0.019,0.076,0.113,0.106,0.101,0.113,0.085,0.019,0,0,0,0,0.039,0.095,0.113,0.11,0.079,0.113,0.106,0.04,0.004,0,0,0.009,0.065,0.11,0.113,0.113,0.04,0.093,0.113,0.089,0.054,0.044,0.044,0.066,0.096,0.106,0.113,0.112,0.003,0.031,0.084,0.113,0.113,0.113,0.113,0.103,0.06,0.075,0.113,0.107,0,0,0.025,0.063,0.095,0.102,0.079,0.044,0.019,0.063,0.113,0.102,0,0,0,0.009,0.033,0.038,0.019,0.001,0.024,0.081,0.113,0.087,0.029,0.045,0.038,0.011,0,0,0,0.001,0.049,0.104,0.113,0.064,0.065,0.111,0.106,0.038,0.001,0,0.001,0.024,0.086,0.113,0.094,0.037,0.039,0.094,0.113,0.071,0.021,0.018,0.021,0.069,0.111,0.1,0.052,0.008,0.013,0.06,0.104,0.101,0.077,0.076,0.077,0.101,0.103,0.062,0.015,0,0,0.019,0.062,0.092,0.101,0.108,0.1,0.09,0.06,0.019,0,0],"K":[0.126,0.094,0.014,0,0,0.001,0.025,0.092,0.125,0.093,0.024,0,0.126,0.094,0.015,0,0,0.016,0.075,0.118,0.114,0.051,0,0,0.126,0.094,0.015,0,0.002,0.051,0.115,0.113,0.069,0.015,0,0,0.126,0.094,0.015,0.01,0.051,0.103,0.112,0.059,0.011,0,0,0,0.126,0.094,0.015,0.048,0.115,0.112,0.054,0.007,0,0,0,0,0.126,0.095,0.037,0.091,0.126,0.099,0.022,0,0,0,0,0,0.126,0.111,0.093,0.118,0.124,0.113,0.044,0,0,0,0,0,0.126,0.126,0.126,0.088,0.072,0.115,0.102,0.036,0,0,0,0,0.126,0.126,0.096,0.024,0.008,0.083,0.126,0.083,0.008,0,0,0,0.126,0.112,0.043,0.001,0,0.057,0.126,0.109,0.037,0,0,0,0.126,0.096,0.017,0,0,0.024,0.098,0.125,0.086,0.017,0,0,0.126,0.094,0.014,0,0,0.002,0.029,0.099,0.122,0.059,0.001,0,0.126,0.094,0.014,0,0,0,0,0.064,0.126,0.102,0.027,0,0.126,0.093,0.014,0,0,0,0,0.03,0.103,0.123,0.081,0.023,0.126,0.093,0.014,0,0,0,0,0.005,0.061,0.119,0.118,0.065,0.126,0.094,0.015,0,0,0,0,0,0.021,0.098,0.126,0.105],"J":[0,0,0,0,0,0,0,0,0.06,0.122,0.124,0.124,0,0,0,0,0,0,0,0,0.06,0.122,0.124,0.124,0,0,0,0,0,0,0,0,0.06,0.122,0.124,0.124,0,0,0,0,0,0,0,0,0.06,0.122,0.124,0.124,0,0,0,0,0,0,0,0,0.06,0.122,0.124,0.124,0,0,0,0,0,0,0,0,0.06,0.122,0.124,0.124,0,0,0,0,0,0,0,0,0.06,0.122,0.124,0.124,0,0,0,0,0,0,0,0,0.06,0.122,0.124,0.124,0,0,0,0,0,0,0,0,0.06,0.122,0.124,0.124,0.005,0.006,0.002,0,0,0,0,0,0.06,0.122,0.124,0.124,0.047,0.06,0.023,0.001,0,0,0,0,0.06,0.122,0.124,0.123,0.109,0.112,0.053,0.002,0,0,0,0.004,0.065,0.123,0.124,0.118,0.115,0.124,0.08,0.023,0.006,0.002,0.005,0.032,0.089,0.124,0.116,0.096,0.095,0.121,0.111,0.085,0.047,0.021,0.048,0.09,0.114,0.123,0.09,0.049,0.058,0.105,0.12,0.121,0.1,0.081,0.102,0.123,0.122,0.107,0.053,0.01,0.018,0.065,0.098,0.105,0.112,0.117,0.109,0.105,0.091,0.056,0.017,0],"A":[0,0,0,0.011,0.094,0.132,0.134,0.072,0,0,0,0,0,0,0,0.023,0.109,0.114,0.128,0.084,0.002,0,0,0,0,0,0.001,0.05,0.123,0.074,0.101,0.104,0.015,0,0,0,0,0,0.012,0.09,0.124,0.038,0.062,0.12,0.05,0,0,0,0,0,0.036,0.124,0.099,0.012,0.027,0.113,0.079,0,0,0,0,0,0.046,0.132,0.085,0.004,0.016,0.105,0.105,0.025,0,0,0,0.001,0.066,0.134,0.067,0.001,0.004,0.082,0.13,0.052,0,0,0,0.01,0.096,0.121,0.037,0,0,0.058,0.133,0.08,0.008,0,0,0.032,0.121,0.122,0.076,0.062,0.062,0.087,0.134,0.115,0.026,0,0,0.045,0.132,0.133,0.131,0.131,0.131,0.132,0.134,0.127,0.05,0,0.001,0.062,0.132,0.108,0.094,0.097,0.099,0.102,0.125,0.134,0.071,0,0.006,0.088,0.104,0.031,0.019,0.021,0.022,0.024,0.069,0.13,0.093,0.012,0.034,0.113,0.074,0.001,0,0,0,0,0.019,0.101,0.117,0.042,0.068,0.132,0.055,0,0,0,0,0,0.002,0.077,0.128,0.076,0.092,0.132,0.044,0,0,0,0,0,0,0.064,0.133,0.108,0.122,0.118,0.028,0,0,0,0,0,0,0.04,0.122,0.124],"Q":[0.002,0.046,0.128,0.108,0.057,0.013,0.009,0.055,0.113,0.127,0.048,0,0.005,0.079,0.129,0.058,0.002,0,0,0.004,0.057,0.127,0.088,0.013,0.041,0.118,0.089,0.012,0,0,0,0,0.01,0.085,0.124,0.053,0.08,0.136,0.05,0,0,0,0,0,0,0.047,0.132,0.091,0.1,0.13,0.037,0,0,0,0,0,0,0.032,0.123,0.101,0.122,0.121,0.029,0,0,0,0,0,0,0.025,0.117,0.12,0.137,0.11,0.017,0,0,0,0,0,0,0.023,0.116,0.128,0.124,0.119,0.027,0,0,0,0,0,0,0.024,0.116,0.121,0.102,0.13,0.037,0,0,0,0,0,0,0.03,0.122,0.104,0.083,0.135,0.046,0,0,0,0,0,0,0.044,0.133,0.094,0.046,0.12,0.082,0.008,0,0.01,0.066,0.061,0.022,0.079,0.129,0.061,0.008,0.083,0.123,0.049,0,0.014,0.093,0.132,0.1,0.124,0.093,0.017,0.002,0.048,0.129,0.106,0.051,0.01,0.04,0.121,0.136,0.134,0.055,0,0,0.011,0.066,0.123,0.126,0.094,0.094,0.126,0.121,0.119,0.089,0.042,0,0,0.006,0.052,0.079,0.091,0.088,0.079,0.048,0.061,0.126,0.126,0,0,0,0.006,0.01,0.022,0.019,0.01,0.005,0.018,0.079,0.109]}};

  const OCR_RANKS = ['A', 'K', 'Q', 'J', 'T', '9', '8', '7', '6', '5', '4', '3', '2'];
  const OCR_TW = 12, OCR_TH = 16;

  function computeBinaryMask(img) {
    const { width, height, data } = img;
    const stride = data.length === width * height * 4 ? 4 : 3;
    const gray = new Float32Array(width * height);
    const intImg = new Float64Array((width + 1) * (height + 1));
    for (let y = 0; y < height; y++) {
      let rowSum = 0;
      for (let x = 0; x < width; x++) {
        const p = (y * width + x) * stride;
        const lum = 0.299 * data[p] + 0.587 * data[p + 1] + 0.114 * data[p + 2];
        gray[y * width + x] = lum;
        rowSum += lum;
        intImg[(y + 1) * (width + 1) + (x + 1)] =
            intImg[y * (width + 1) + (x + 1)] + rowSum;
      }
    }
    const mask = new Uint8Array(width * height);
    const rad = Math.max(12, Math.round(Math.min(width, height) / 14));
    for (let y = 0; y < height; y++) {
      const y0 = Math.max(0, y - rad);
      const y1 = Math.min(height - 1, y + rad);
      for (let x = 0; x < width; x++) {
        const x0 = Math.max(0, x - rad);
        const x1 = Math.min(width - 1, x + rad);
        const area = (y1 - y0 + 1) * (x1 - x0 + 1);
        const sum = intImg[(y1 + 1) * (width + 1) + (x1 + 1)] -
                    intImg[y0 * (width + 1) + (x1 + 1)] -
                    intImg[(y1 + 1) * (width + 1) + x0] +
                    intImg[y0 * (width + 1) + x0];
        const localMean = sum / area;
        if (gray[y * width + x] < localMean * 0.78 && gray[y * width + x] < 175) {
          mask[y * width + x] = 1;
        }
      }
    }
    const vBorders = [];
    for (let x = 0; x < width; x++) {
      let cnt = 0;
      for (let y = 0; y < height; y++) if (mask[y * width + x]) cnt++;
      if (cnt > height * 0.45) {
        vBorders.push(x);
        for (let y = 0; y < height; y++) mask[y * width + x] = 0;
      }
    }
    for (let y = 0; y < height; y++) {
      let cnt = 0;
      for (let x = 0; x < width; x++) if (mask[y * width + x]) cnt++;
      if (cnt > width * 0.45) {
        for (let x = 0; x < width; x++) mask[y * width + x] = 0;
      }
    }
    return { mask, gray, vBorders };
  }

  function findHandAnchorsBW(img, mask) {
    const { width, height } = img;
    const visited = new Uint8Array(width * height);
    const blobs = [];
    for (let y = 0; y < height; y++) {
      for (let x = 0; x < width; x++) {
        const p = y * width + x;
        if (!mask[p] || visited[p]) continue;
        let minX = x, maxX = x, minY = y, maxY = y, count = 0;
        const q = [p];
        visited[p] = 1;
        while (q.length) {
          const cur = q.pop();
          const cy = Math.floor(cur / width), cx = cur % width;
          count++;
          if (cx < minX) minX = cx;
          if (cx > maxX) maxX = cx;
          if (cy < minY) minY = cy;
          if (cy > maxY) maxY = cy;
          for (let dy = -1; dy <= 1; dy++) {
            for (let dx = -1; dx <= 1; dx++) {
              const nx = cx + dx, ny = cy + dy;
              if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                const np = ny * width + nx;
                if (mask[np] && !visited[np]) {
                  visited[np] = 1;
                  q.push(np);
                }
              }
            }
          }
        }
        const bw = maxX - minX + 1, bh = maxY - minY + 1;
        if (count >= 12 && bw >= 6 && bh >= 6 && bw <= height * 0.08 && bh <= height * 0.08 &&
            bw / bh >= 0.55 && bw / bh <= 1.45) {
          blobs.push({ minX, maxX, minY, maxY, cx: (minX + maxX) / 2, cy: (minY + maxY) / 2, w: bw, h: bh });
        }
      }
    }
    const stacks = [];
    for (let i = 0; i < blobs.length; i++) {
      const b0 = blobs[i];
      if (b0.cx >= width * 0.86) continue;
      const aligned = blobs.filter(b =>
        Math.abs(b.cx - b0.cx) <= Math.max(5, b0.w * 0.45) &&
        Math.abs(b.h - b0.h) <= Math.max(5, b0.h * 0.45) &&
        b.cy >= b0.cy
      ).sort((a, b) => a.cy - b.cy);
      for (let j = 0; j < aligned.length; j++) {
        for (let k = j + 1; k < aligned.length; k++) {
          const b1 = aligned[j], b2 = aligned[k];
          const dy = b2.cy - b1.cy;
          if (dy < b0.h * 1.05 || dy > b0.h * 2.5) continue;
          if (Math.abs((b1.cy - b0.cy) - dy) > Math.max(4, dy * 0.25)) continue;
          const b3 = aligned.find(b => Math.abs((b.cy - b2.cy) - dy) <= Math.max(4, dy * 0.25));
          if (b3) {
            stacks.push({
              heart: b1,
              diamond: b2,
              cx: (b0.cx + b1.cx + b2.cx + b3.cx) / 4,
              cy: (b1.cy + b2.cy) / 2,
              dy: (b3.cy - b0.cy) / 3,
              xStart: Math.round(Math.max(b0.maxX, b1.maxX, b2.maxX, b3.maxX) + Math.max(3, b0.w * 0.22)),
              iconW: (b0.w + b1.w + b2.w + b3.w) / 4,
              iconH: (b0.h + b1.h + b2.h + b3.h) / 4
            });
          }
        }
      }
    }
    return selectBestCompassQuad(stacks, width, height);
  }

  function selectBestCompassQuad(pairs, width, height) {
    if (!pairs || pairs.length < 4) return null;
    let bestQuad = null, bestScore = 1e9;
    for (let iN = 0; iN < pairs.length; iN++) {
      for (let iS = 0; iS < pairs.length; iS++) {
        if (iS === iN) continue;
        const N = pairs[iN], S = pairs[iS];
        if (S.cy - N.cy < height * 0.28) continue;
        if (Math.abs(N.cx - S.cx) > width * 0.18) continue;
        for (let iW = 0; iW < pairs.length; iW++) {
          if (iW === iN || iW === iS) continue;
          for (let iE = 0; iE < pairs.length; iE++) {
            if (iE === iN || iE === iS || iE === iW) continue;
            const W = pairs[iW], E = pairs[iE];
            if (E.cx - W.cx < width * 0.32) continue;
            if (W.cx >= N.cx - width * 0.08 || E.cx <= N.cx + width * 0.08) continue;
            if (Math.abs(W.cy - E.cy) > height * 0.12) continue;
            const midY = (W.cy + E.cy) / 2;
            const midX = (W.cx + E.cx) / 2;
            if (midY <= N.cy + N.dy || midY >= S.cy - S.dy) continue;
            const score =
              Math.abs(N.cx - S.cx) * 2.5 +
              Math.abs(W.cy - E.cy) * 2.5 +
              Math.abs((N.cx + S.cx) / 2 - midX) * 1.5 +
              Math.abs((N.cy + S.cy) / 2 - midY) * 1.5;
            if (score < bestScore) {
              bestScore = score;
              bestQuad = { N, W, E, S };
            }
          }
        }
      }
    }
    return bestQuad;
  }

  function findHandAnchors(img, mask) {
    const { width, height, data } = img;
    const stride = data.length === width * height * 4 ? 4 : 3;
    const visited = new Uint8Array(width * height);
    const redBlobs = [];
    for (let y = 0; y < height; y++) {
      for (let x = 0; x < width; x++) {
        const p = y * width + x;
        if (visited[p]) continue;
        const r = data[p * stride], g = data[p * stride + 1], b = data[p * stride + 2];
        if (r - Math.max(g, b) > 35 && r > 90) {
          let minX = x, maxX = x, minY = y, maxY = y, count = 0;
          const q = [p];
          visited[p] = 1;
          while (q.length) {
            const cur = q.pop();
            const cy = Math.floor(cur / width), cx = cur % width;
            count++;
            if (cx < minX) minX = cx;
            if (cx > maxX) maxX = cx;
            if (cy < minY) minY = cy;
            if (cy > maxY) maxY = cy;
            for (let dy = -1; dy <= 1; dy++) {
              for (let dx = -1; dx <= 1; dx++) {
                const nx = cx + dx, ny = cy + dy;
                if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                  const np = ny * width + nx;
                  if (!visited[np]) {
                    const nr = data[np * stride], ng = data[np * stride + 1], nb = data[np * stride + 2];
                    if (nr - Math.max(ng, nb) > 35 && nr > 90) {
                      visited[np] = 1;
                      q.push(np);
                    }
                  }
                }
              }
            }
          }
          const bw = maxX - minX + 1, bh = maxY - minY + 1;
          if (count >= 12 && bw >= 6 && bh >= 6 && bw <= height * 0.12 && bh <= height * 0.12) {
            redBlobs.push({
              minX, maxX, minY, maxY,
              cx: (minX + maxX) / 2,
              cy: (minY + maxY) / 2,
              w: bw, h: bh, count
            });
          }
        }
      }
    }

    const pairs = [];
    for (let i = 0; i < redBlobs.length; i++) {
      for (let j = 0; j < redBlobs.length; j++) {
        if (i === j) continue;
        const a = redBlobs[i], b = redBlobs[j];
        const dy = b.cy - a.cy;
        if (Math.abs(a.cx - b.cx) <= Math.max(8, a.w * 0.6) &&
            dy >= a.h * 1.05 && dy <= a.h * 2.6 &&
            a.cx < width * 0.86) {
          const spadeY = Math.round(a.cy - dy);
          const clubY = Math.round(b.cy + dy);
          if (spadeY < 2 || clubY >= height - 2) continue;
          pairs.push({
            heart: a,
            diamond: b,
            cx: (a.cx + b.cx) / 2,
            cy: (a.cy + b.cy) / 2,
            dy,
            xStart: Math.round(Math.max(a.maxX, b.maxX) + Math.max(3, a.w * 0.22)),
            iconW: (a.w + b.w) / 2,
            iconH: (a.h + b.h) / 2
          });
        }
      }
    }
    const quad = selectBestCompassQuad(pairs, width, height);
    if (quad) return quad;
    return mask ? findHandAnchorsBW(img, mask) : null;
  }

  function rasterizeGlyphPatch(mask, width, box) {
    const vec = new Float32Array(OCR_TW * OCR_TH);
    let norm = 0;
    for (let ty = 0; ty < OCR_TH; ty++) {
      const sy0 = box.y0 + (ty / OCR_TH) * box.h;
      const sy1 = box.y0 + ((ty + 1) / OCR_TH) * box.h;
      for (let tx = 0; tx < OCR_TW; tx++) {
        const sx0 = box.x0 + (tx / OCR_TW) * box.w;
        const sx1 = box.x0 + ((tx + 1) / OCR_TW) * box.w;
        let sum = 0, cnt = 0;
        for (let y = Math.floor(sy0); y <= Math.min(box.y1, Math.floor(sy1)); y++) {
          for (let x = Math.floor(sx0); x <= Math.min(box.x1, Math.floor(sx1)); x++) {
            sum += mask[y * width + x];
            cnt++;
          }
        }
        const val = cnt > 0 ? sum / cnt : 0;
        vec[ty * OCR_TW + tx] = val;
        norm += val * val;
      }
    }
    norm = Math.sqrt(norm) || 1;
    for (let i = 0; i < vec.length; i++) vec[i] /= norm;
    return vec;
  }

  function segmentSuitStrip(mask, width, height, x0, x1, yCenter, dy) {
    const y0 = Math.max(0, Math.round(yCenter - dy * 0.42));
    const y1 = Math.min(height - 1, Math.round(yCenter + dy * 0.42));
    const colDark = [];
    for (let x = x0; x < x1; x++) {
      let dark = 0;
      for (let y = y0; y <= y1; y++) {
        if (mask[y * width + x]) dark++;
      }
      colDark.push(dark);
    }
    const rawSegs = [];
    let inSeg = false, segStart = 0, blankRun = 0;
    for (let i = 0; i < colDark.length; i++) {
      if (colDark[i] > 0) {
        if (!inSeg) {
          inSeg = true;
          segStart = x0 + i;
        }
        blankRun = 0;
      } else if (inSeg) {
        inSeg = false;
        rawSegs.push([segStart, x0 + i - 1]);
      } else {
        blankRun++;
        if (rawSegs.length > 0 && blankRun > dy * 0.62) break;
      }
    }
    if (inSeg) rawSegs.push([segStart, x1 - 1]);

    const boxes = [];
    function addBox(sx0, sx1) {
      let sy0 = y1, sy1 = y0, ink = 0;
      for (let y = y0; y <= y1; y++) {
        for (let x = sx0; x <= sx1; x++) {
          if (mask[y * width + x]) {
            ink++;
            if (y < sy0) sy0 = y;
            if (y > sy1) sy1 = y;
          }
        }
      }
      if (ink < 3) return;
      const w = sx1 - sx0 + 1;
      const h = sy1 - sy0 + 1;
      if (h <= dy * 0.25 && w >= dy * 0.35) {
        boxes.push({ x0: sx0, x1: sx1, y0: sy0, y1: sy1, w, h, isVoid: true });
        return;
      }
      if (w > h * 1.18 && h >= dy * 0.5) {
        let bestSplit = -1, minCol = 1e9;
        const search0 = sx0 + Math.max(4, Math.floor(w * 0.25));
        const search1 = sx0 + Math.min(w - 5, Math.ceil(w * 0.75));
        for (let x = search0; x <= search1; x++) {
          let c = 0;
          for (let y = sy0; y <= sy1; y++) {
            if (mask[y * width + x]) c++;
          }
          if (c < minCol) {
            minCol = c;
            bestSplit = x;
          }
        }
        if (bestSplit > sx0 && minCol <= 3) {
          addBox(sx0, bestSplit - 1);
          addBox(bestSplit + 1, sx1);
          return;
        }
      }
      boxes.push({ x0: sx0, x1: sx1, y0: sy0, y1: sy1, w, h, isVoid: false });
    }
    for (const [sx0, sx1] of rawSegs) addBox(sx0, sx1);

    const clean = boxes.filter(b => b.isVoid || b.h >= dy * 0.5);
    if (clean.length === 1 && clean[0].isVoid) {
      return [];
    }
    const nonVoid = clean.filter(b => !b.isVoid);

    const tokens = [];
    for (let i = 0; i < nonVoid.length; i++) {
      const b = nonVoid[i];
      const aspect = b.w / b.h;
      if (aspect < 0.50 && i + 1 < nonVoid.length && (nonVoid[i + 1].w / nonVoid[i + 1].h) >= 0.52) {
        const next = nonVoid[i + 1];
        tokens.push({
          isTen: true,
          oneBox: b,
          zeroBox: next,
          w: next.x1 - b.x0 + 1,
          h: Math.max(b.h, next.h)
        });
        i++;
      } else {
        tokens.push({
          isTen: false,
          box: b,
          w: b.w,
          h: b.h,
          aspect,
          vec: rasterizeGlyphPatch(mask, width, b)
        });
      }
    }
    return tokens;
  }

  function scoreOcrToken(tok, rank) {
    if (rank === 'T') {
      return tok.isTen ? 2.5 : -4.0;
    }
    if (tok.isTen) {
      return -4.0;
    }
    const tpl = OCR_TEMPLATE_DATA.tpl[rank];
    let dot = 0;
    for (let i = 0; i < OCR_TW * OCR_TH; i++) dot += tok.vec[i] * tpl[i];
    const aspectDiff = Math.abs(tok.aspect - OCR_TEMPLATE_DATA.aspect[rank]);
    return dot * 2.0 - aspectDiff * 1.5;
  }

  function solveSuitPartitionDP(suitTokensByHand) {
    const dirs = ['N', 'W', 'E', 'S'];
    const lens = dirs.map(d => suitTokensByHand[d].length);
    const totalSlots = lens[0] + lens[1] + lens[2] + lens[3];

    if (totalSlots === 13) {
      const [L0, L1, L2, L3] = lens;
      const S0 = L0 + 1, S1 = L1 + 1, S2 = L2 + 1, S3 = L3 + 1;
      const numStates = S0 * S1 * S2 * S3;
      const dp = new Float32Array(numStates).fill(-1e9);
      const parent = new Int32Array(13 * numStates).fill(-1);
      dp[0] = 0;

      for (let rIdx = 0; rIdx < 13; rIdx++) {
        const rank = OCR_RANKS[rIdx];
        const nextDp = new Float32Array(numStates).fill(-1e9);
        for (let i0 = 0; i0 <= L0; i0++) {
          for (let i1 = 0; i1 <= L1; i1++) {
            for (let i2 = 0; i2 <= L2; i2++) {
              for (let i3 = 0; i3 <= L3; i3++) {
                if (i0 + i1 + i2 + i3 !== rIdx) continue;
                const st = ((i0 * S1 + i1) * S2 + i2) * S3 + i3;
                const curVal = dp[st];
                if (curVal <= -1e8) continue;
                const idxs = [i0, i1, i2, i3];
                for (let h = 0; h < 4; h++) {
                  if (idxs[h] < lens[h]) {
                    const sc = curVal + scoreOcrToken(suitTokensByHand[dirs[h]][idxs[h]], rank);
                    const n0 = i0 + (h === 0 ? 1 : 0);
                    const n1 = i1 + (h === 1 ? 1 : 0);
                    const n2 = i2 + (h === 2 ? 1 : 0);
                    const n3 = i3 + (h === 3 ? 1 : 0);
                    const nst = ((n0 * S1 + n1) * S2 + n2) * S3 + n3;
                    if (sc > nextDp[nst]) {
                      nextDp[nst] = sc;
                      parent[rIdx * numStates + nst] = h;
                    }
                  }
                }
              }
            }
          }
        }
        dp.set(nextDp);
      }
      const res = { N: [], W: [], E: [], S: [] };
      let c0 = L0, c1 = L1, c2 = L2, c3 = L3;
      for (let rIdx = 12; rIdx >= 0; rIdx--) {
        const st = ((c0 * S1 + c1) * S2 + c2) * S3 + c3;
        const h = parent[rIdx * numStates + st];
        if (h >= 0 && h < 4) {
          res[dirs[h]].unshift(OCR_RANKS[rIdx]);
          if (h === 0) c0--;
          else if (h === 1) c1--;
          else if (h === 2) c2--;
          else c3--;
        }
      }
      return res;
    }

    const res = {};
    for (const d of dirs) {
      const toks = suitTokensByHand[d];
      const m = toks.length;
      if (m === 0) { res[d] = []; continue; }
      const K = Math.min(m, 13);
      const dp = Array.from({ length: K }, () => new Float32Array(13).fill(-1e9));
      const par = Array.from({ length: K }, () => new Int32Array(13).fill(-1));
      for (let r = 0; r < 13; r++) dp[0][r] = scoreOcrToken(toks[0], OCR_RANKS[r]);
      for (let k = 1; k < K; k++) {
        for (let r = k; r < 13; r++) {
          const s = scoreOcrToken(toks[k], OCR_RANKS[r]);
          for (let pr = k - 1; pr < r; pr++) {
            if (dp[k - 1][pr] + s > dp[k][r]) {
              dp[k][r] = dp[k - 1][pr] + s;
              par[k][r] = pr;
            }
          }
        }
      }
      let bestR = 0;
      for (let r = 1; r < 13; r++) if (dp[K - 1][r] > dp[K - 1][bestR]) bestR = r;
      const seq = [];
      for (let k = K - 1; k >= 0; k--) {
        seq.unshift(OCR_RANKS[bestR]);
        bestR = par[k][bestR];
      }
      res[d] = seq;
    }
    return res;
  }

  function formatSuitRanksArrayToHandString(suitsArr) {
    const suitKeys = ['S', 'H', 'D', 'C'];
    return suitKeys.map((sk, idx) => {
      const cards = (suitsArr[idx] && suitsArr[idx].length > 0) ? suitsArr[idx].join('') : '-';
      return `${sk} ${cards};`;
    }).join(' ');
  }

  function recognizeBoardFromImageData(img) {
    const { mask, vBorders } = computeBinaryMask(img);
    const anchors = findHandAnchors(img, mask);
    if (!anchors) return null;

    const dirs = ['N', 'W', 'E', 'S'];
    const tokensBySuitAndHand = [{}, {}, {}, {}];
    for (const dir of dirs) {
      const h = anchors[dir];
      let xEnd = (dir === 'W') ? Math.floor(anchors.E.cx - anchors.E.iconW) : img.width - 4;
      for (const bx of vBorders) {
        if (bx > h.xStart + h.dy * 1.2 && bx < xEnd) {
          xEnd = bx - 2;
          break;
        }
      }
      const rowsY = [h.heart.cy - h.dy, h.heart.cy, h.diamond.cy, h.diamond.cy + h.dy];
      for (let sIdx = 0; sIdx < 4; sIdx++) {
        tokensBySuitAndHand[sIdx][dir] = segmentSuitStrip(
          mask, img.width, img.height, h.xStart, xEnd, rowsY[sIdx], h.dy
        );
      }
    }
    const recognized = { N: [], W: [], E: [], S: [] };
    for (let sIdx = 0; sIdx < 4; sIdx++) {
      const suitRes = solveSuitPartitionDP(tokensBySuitAndHand[sIdx]);
      for (const dir of dirs) {
        recognized[dir].push(suitRes[dir]);
      }
    }
    const hands = {
      N: formatSuitRanksArrayToHandString(recognized.N),
      E: formatSuitRanksArrayToHandString(recognized.E),
      S: formatSuitRanksArrayToHandString(recognized.S),
      W: formatSuitRanksArrayToHandString(recognized.W)
    };
    return { hands, anchors, raw: recognized };
  }

  function detectAllBoardsInImageData(img) {
    const { width, height, data } = img;
    const stride = data.length === width * height * 4 ? 4 : 3;
    const scale = Math.max(1, Math.floor(Math.max(width, height) / 1100));
    const sw = Math.floor(width / scale), sh = Math.floor(height / scale);
    const visited = new Uint8Array(sw * sh);
    const redBlobs = [];
    for (let y = 0; y < sh; y++) {
      for (let x = 0; x < sw; x++) {
        const p = y * sw + x;
        if (visited[p]) continue;
        const origP = (y * scale * width + x * scale) * stride;
        const r = data[origP], g = data[origP + 1], b = data[origP + 2];
        if (r - Math.max(g, b) > 35 && r > 90) {
          let minX = x, maxX = x, minY = y, maxY = y, count = 0;
          const q = [p];
          visited[p] = 1;
          while (q.length) {
            const cur = q.pop();
            const cy = Math.floor(cur / sw), cx = cur % sw;
            count++;
            if (cx < minX) minX = cx;
            if (cx > maxX) maxX = cx;
            if (cy < minY) minY = cy;
            if (cy > maxY) maxY = cy;
            for (let dy = -1; dy <= 1; dy++) {
              for (let dx = -1; dx <= 1; dx++) {
                const nx = cx + dx, ny = cy + dy;
                if (nx >= 0 && nx < sw && ny >= 0 && ny < sh) {
                  const np = ny * sw + nx;
                  if (!visited[np]) {
                    const op = (ny * scale * width + nx * scale) * stride;
                    const nr = data[op], ng = data[op + 1], nb = data[op + 2];
                    if (nr - Math.max(ng, nb) > 35 && nr > 90) {
                      visited[np] = 1;
                      q.push(np);
                    }
                  }
                }
              }
            }
          }
          const bw = (maxX - minX + 1) * scale, bh = (maxY - minY + 1) * scale;
          if (count * scale * scale >= 12 && bw >= 5 && bh >= 5 && bw <= 90 && bh <= 90) {
            redBlobs.push({
              minX: minX * scale, maxX: maxX * scale,
              minY: minY * scale, maxY: maxY * scale,
              cx: ((minX + maxX) / 2) * scale,
              cy: ((minY + maxY) / 2) * scale,
              w: bw, h: bh
            });
          }
        }
      }
    }
    const pairs = [];
    for (let i = 0; i < redBlobs.length; i++) {
      for (let j = 0; j < redBlobs.length; j++) {
        if (i === j) continue;
        const a = redBlobs[i], b = redBlobs[j];
        const dy = b.cy - a.cy;
        if (Math.abs(a.cx - b.cx) <= Math.max(8, a.w * 0.6) &&
            dy >= a.h * 1.05 && dy <= a.h * 2.6) {
          pairs.push({
            cx: (a.cx + b.cx) / 2,
            cy: (a.cy + b.cy) / 2,
            dy,
            w: (a.w + b.w) / 2
          });
        }
      }
    }
    const quads = [];
    const used = new Set();
    pairs.sort((a, b) => a.cy - b.cy || a.cx - b.cx);
    for (let iN = 0; iN < pairs.length; iN++) {
      if (used.has(iN)) continue;
      const N = pairs[iN];
      let bestQ = null, bestSc = 1e9;
      for (let iS = 0; iS < pairs.length; iS++) {
        if (iS === iN || used.has(iS)) continue;
        const S = pairs[iS];
        const vDist = S.cy - N.cy;
        if (vDist < N.dy * 4.5 || vDist > N.dy * 12) continue;
        if (Math.abs(N.cx - S.cx) > N.dy * 2.0) continue;
        const midY = (N.cy + S.cy) / 2;
        for (let iW = 0; iW < pairs.length; iW++) {
          if (used.has(iW)) continue;
          const W = pairs[iW];
          if (Math.abs(W.cy - midY) > N.dy * 1.8) continue;
          const dxW = N.cx - W.cx;
          if (dxW < N.dy * 2.5 || dxW > N.dy * 9.0) continue;
          for (let iE = 0; iE < pairs.length; iE++) {
            if (used.has(iE)) continue;
            const E = pairs[iE];
            if (Math.abs(E.cy - W.cy) > N.dy * 1.2) continue;
            const dxE = E.cx - N.cx;
            if (dxE < N.dy * 2.5 || dxE > N.dy * 9.0) continue;
            const sc = Math.abs(N.cx - S.cx) * 2 + Math.abs(W.cy - E.cy) * 2 + Math.abs(dxW - dxE);
            if (sc < bestSc) {
              bestSc = sc;
              bestQ = { iN, iS, iW, iE, N, S, W, E };
            }
          }
        }
      }
      if (bestQ) {
        used.add(bestQ.iN); used.add(bestQ.iS); used.add(bestQ.iW); used.add(bestQ.iE);
        const { N, S, W, E } = bestQ;
        const dy = (N.dy + S.dy + W.dy + E.dy) / 4;
        const spanX = E.cx - W.cx;
        const x0 = Math.max(0, Math.round(W.cx - dy * 1.1));
        const y0 = Math.max(0, Math.round(N.cy - dy * 2.8));
        const x1 = Math.min(width, Math.round(E.cx + spanX * 0.56));
        const y1 = Math.min(height, Math.round(S.cy + dy * 3.0));
        quads.push({ x: x0, y: y0, w: x1 - x0, h: y1 - y0 });
      }
    }
    quads.sort((a, b) => (Math.abs(a.y - b.y) < 50 ? a.x - b.x : a.y - b.y));
    return quads;
  }

  function validateFourHandsDeck(hands) {
    const counts = { N: 0, E: 0, S: 0, W: 0 };
    const seen = new Map();
    const duplicates = [];
    ['N', 'E', 'S', 'W'].forEach(seat => {
      const suits = parseHandToSuits(hands[seat] || '');
      ['S', 'H', 'D', 'C'].forEach(sk => {
        const str = (suits[sk] || '').replace(/10/g, 'T').toUpperCase();
        for (let i = 0; i < str.length; i++) {
          const r = str.charAt(i);
          if (OCR_RANKS.includes(r)) {
            counts[seat]++;
            const key = `${sk}${r}`;
            if (seen.has(key)) {
              duplicates.push(`${key} (${seen.get(key)} & ${seat})`);
            } else {
              seen.set(key, seat);
            }
          }
        }
      });
    });
    const total = counts.N + counts.E + counts.S + counts.W;
    const isExact52 = (
      counts.N === 13 && counts.E === 13 && counts.S === 13 && counts.W === 13 &&
      seen.size === 52 && duplicates.length === 0
    );
    return { isExact52, counts, uniqueCount: seen.size, duplicates };
  }

  // Modal DOM Elements & Interactive Crop State
  const photoScanModal = document.getElementById('photoScanModal');
  const closePhotoModalBtn = document.getElementById('closePhotoModalBtn');
  const cancelPhotoModalBtn = document.getElementById('cancelPhotoModalBtn');
  const applyPhotoModalBtn = document.getElementById('applyPhotoModalBtn');
  const cropCanvas = document.getElementById('cropCanvas');
  const cropCanvasViewport = document.getElementById('cropCanvasViewport');
  const pdfPageBar = document.getElementById('pdfPageBar');
  const pdfPrevPageBtn = document.getElementById('pdfPrevPageBtn');
  const pdfNextPageBtn = document.getElementById('pdfNextPageBtn');
  const pdfPageIndicator = document.getElementById('pdfPageIndicator');
  const cropRotateBtn = document.getElementById('cropRotateBtn');
  const cropZoomToBoxBtn = document.getElementById('cropZoomToBoxBtn');
  const cropResetViewBtn = document.getElementById('cropResetViewBtn');
  const cropNewPhotoBtn = document.getElementById('cropNewPhotoBtn');
  const rescanCropBtn = document.getElementById('rescanCropBtn');
  const detectedBoardsBar = document.getElementById('detectedBoardsBar');
  const detectedBoardsList = document.getElementById('detectedBoardsList');
  const scanStatusBadge = document.getElementById('scanStatusBadge');
  const scanInputN = document.getElementById('scanInputN');
  const scanInputE = document.getElementById('scanInputE');
  const scanInputS = document.getElementById('scanInputS');
  const scanInputW = document.getElementById('scanInputW');
  const modalContractHint = document.getElementById('modalContractHint');
  const modalNextLeadHint = document.getElementById('modalNextLeadHint');
  const scanInitialCardsInput = document.getElementById('scanInitialCardsInput');
  const scanUndoCardBtn = document.getElementById('scanUndoCardBtn');
  const scanClearCardsBtn = document.getElementById('scanClearCardsBtn');
  const scanInitialPlayError = document.getElementById('scanInitialPlayError');
  const scanInitialPlayedList = document.getElementById('scanInitialPlayedList');
  const scanInitialCardPicker = document.getElementById('scanInitialCardPicker');

  let scanSourceCanvas = null; // Offscreen canvas holding current (possibly rotated) image
  let detectedBoardBoxes = []; // Array of { x, y, w, h, boardLabel?, pbnHands?, pbnDeclarer?, pbnContract? } in scanSourceCanvas coords
  let activeDetectedBoardIdx = -1;
  let lastSelectedBoardLabel = '1';
  let cropBox = { x: 0, y: 0, w: 100, h: 100 }; // In scanSourceCanvas coords
  let lastDetectedAnchors = null; // In cropBox-relative coords
  let cropViewZoom = 1.0;
  let cropViewOffsetX = 0;
  let cropViewOffsetY = 0;
  let cropDragMode = null; // 'move', 'nw', 'ne', 'sw', 'se', 'n', 's', 'w', 'e', 'pan'
  let cropDragStartImg = { x: 0, y: 0 };
  let cropDragStartClient = { x: 0, y: 0 };
  let cropDragStartBox = null;
  let cropPinchDist = 0;
  let cropPinchStartZoom = 1.0;

  // PDF & PBN Multi-Page State
  let activePdfDoc = null;
  let activePdfPbnBoards = [];
  let activePdfPageNum = 1;
  let activePdfTotalPages = 1;
  let pdfPageBoardCounts = {};
  let pdfJsLoadPromise = null;

  function parseModalSingleCardToken(tok) {
    if (!tok) return null;
    const s = tok.trim()
      .replace(/♠/g, 'S')
      .replace(/♥/g, 'H')
      .replace(/♦/g, 'D')
      .replace(/♣/g, 'C')
      .toUpperCase();
    if (!s) return null;
    let suit = null;
    let rankPart = '';
    if ('SHDC'.includes(s[0])) {
      suit = s[0];
      rankPart = s.slice(1);
    } else if ('SHDC'.includes(s[s.length - 1])) {
      suit = s[s.length - 1];
      rankPart = s.slice(0, -1);
    } else {
      return null;
    }
    if (rankPart === '10') rankPart = 'T';
    if (rankPart.length !== 1 || !'AKQJT98765432'.includes(rankPart)) {
      return null;
    }
    return { suit, rank: rankPart, code: `${suit}${rankPart}` };
  }

  function cloneModalHandsMap(handsMap) {
    const copy = {};
    ['N', 'E', 'S', 'W'].forEach(seat => {
      copy[seat] = {
        S: (handsMap[seat] && handsMap[seat].S) ? handsMap[seat].S.slice() : [],
        H: (handsMap[seat] && handsMap[seat].H) ? handsMap[seat].H.slice() : [],
        D: (handsMap[seat] && handsMap[seat].D) ? handsMap[seat].D.slice() : [],
        C: (handsMap[seat] && handsMap[seat].C) ? handsMap[seat].C.slice() : []
      };
    });
    return copy;
  }

  function strainToTrumpCode(strain) {
    if (strain === 'SPADES') return 'S';
    if (strain === 'HEARTS') return 'H';
    if (strain === 'DIAMONDS') return 'D';
    if (strain === 'CLUBS') return 'C';
    return 'NT';
  }

  function simulateModalInitialPlay(handsObj, declarerSeat, contractStrain) {
    const order = ['N', 'E', 'S', 'W'];
    const declIdx = Math.max(0, order.indexOf(declarerSeat || currentDeclarerSeat || 'N'));
    const trumpCode = strainToTrumpCode(contractStrain || modalContractStrain);

    const srcHands = handsObj || {
      N: scanInputN ? scanInputN.value : '',
      E: scanInputE ? scanInputE.value : '',
      S: scanInputS ? scanInputS.value : '',
      W: scanInputW ? scanInputW.value : ''
    };

    const initialHandsMap = {};
    order.forEach(seat => {
      const suits = parseHandToSuits(srcHands[seat] || '');
      initialHandsMap[seat] = {};
      ['S', 'H', 'D', 'C'].forEach(s => {
        initialHandsMap[seat][s] = tokenizeSuitRanks(suits[s] || '-')
          .map(t => (t === '10' ? 'T' : t.toUpperCase()))
          .filter(t => 'AKQJT98765432'.includes(t));
      });
    });

    const remHands = cloneModalHandsMap(initialHandsMap);
    let remHandsAfterCompleted = cloneModalHandsMap(initialHandsMap);

    let currentLeaderOrigIdx = (declIdx + 1) % 4; // LHO leads to Trick 1
    const completedTricks = [];
    let currentTrickCards = [];
    let nsCompletedTricksWon = 0;
    const voidSuits = { E: new Set(), W: new Set() };
    const playedCodes = new Set();
    let error = null;

    const rawInput = scanInitialCardsInput ? scanInitialCardsInput.value.trim() : '';
    const rawTokens = rawInput ? rawInput.split(/[\s,;/]+/).filter(Boolean) : [];

    for (let i = 0; i < rawTokens.length; i++) {
      const rawTok = rawTokens[i];
      const turnOffset = currentTrickCards.length;
      const seatOrigIdx = (currentLeaderOrigIdx + turnOffset) % 4;
      const seatKey = order[seatOrigIdx];
      const rotatedIdx = (seatOrigIdx - declIdx + 4) % 4;

      const parsed = parseModalSingleCardToken(rawTok);
      if (!parsed) {
        error = `Invalid card "${rawTok}" (expected e.g. DQ, H10, SA).`;
        break;
      }

      if (playedCodes.has(parsed.code)) {
        error = `${parsed.code} has already been played.`;
        break;
      }

      const seatSuitArr = remHands[seatKey][parsed.suit] || [];
      const rankIdx = seatSuitArr.indexOf(parsed.rank);
      if (rankIdx === -1) {
        const holder = order.find(other =>
          (remHands[other][parsed.suit] || []).includes(parsed.rank)
        );
        if (holder) {
          error = `It is ${seatKey}'s turn, but ${parsed.code} is held by ${holder}.`;
        } else {
          error = `${seatKey} does not hold ${parsed.code}.`;
        }
        break;
      }

      if (turnOffset > 0) {
        const leadSuit = currentTrickCards[0].suit;
        if (parsed.suit !== leadSuit) {
          if ((remHands[seatKey][leadSuit] || []).length > 0) {
            error = `${seatKey} must follow suit to ${leadSuit} before playing ${parsed.code}.`;
            break;
          }
          if (rotatedIdx === 1) voidSuits.E.add(leadSuit);
          if (rotatedIdx === 3) voidSuits.W.add(leadSuit);
        }
      }

      seatSuitArr.splice(rankIdx, 1);
      playedCodes.add(parsed.code);

      const leadSuitForCard = currentTrickCards.length > 0 ? currentTrickCards[0].suit : parsed.suit;
      const cardObj = parseSingleCardToken(parsed.code, leadSuitForCard, trumpCode);
      cardObj.code = parsed.code;
      cardObj.origSeatIdx = seatOrigIdx;
      cardObj.origSeat = seatKey;
      cardObj.rotatedSeatIdx = rotatedIdx;
      cardObj.rotatedSeat = order[rotatedIdx];
      currentTrickCards.push(cardObj);

      if (currentTrickCards.length === 4) {
        const { winningIdx, winnerCard } = evaluateTrickWinner(currentTrickCards, trumpCode);
        const winnerOrigIdx = winnerCard ? winnerCard.origSeatIdx : currentLeaderOrigIdx;
        const winnerRotatedIdx = (winnerOrigIdx - declIdx + 4) % 4;
        if (winnerRotatedIdx === 0 || winnerRotatedIdx === 2) {
          nsCompletedTricksWon++;
        }
        completedTricks.push({
          trickNum: completedTricks.length + 1,
          leaderOrigIdx: currentLeaderOrigIdx,
          leaderSeat: order[currentLeaderOrigIdx],
          cards: currentTrickCards.slice(),
          winningIdx,
          winnerOrigIdx,
          winnerSeat: order[winnerOrigIdx],
          winnerRotatedIdx,
          winnerRotatedSeat: order[winnerRotatedIdx]
        });
        remHandsAfterCompleted = cloneModalHandsMap(remHands);
        currentLeaderOrigIdx = winnerOrigIdx;
        currentTrickCards = [];
      }
    }

    const nextTrickNum = completedTricks.length + 1;
    const nextSeatOrigIdx = (currentLeaderOrigIdx + currentTrickCards.length) % 4;
    const nextSeatKey = order[nextSeatOrigIdx];

    return {
      completedTricks,
      currentTrickCards,
      currentLeaderOrigIdx,
      nextTrickNum,
      nextSeatOrigIdx,
      nextSeatKey,
      nsCompletedTricksWon,
      voidSuits,
      remHandsAfterCompleted,
      remHandsAfterAll: remHands,
      error
    };
  }

  function appendModalCardToken(cardCode) {
    if (!scanInitialCardsInput) return;
    const cur = scanInitialCardsInput.value.trim();
    scanInitialCardsInput.value = cur ? `${cur} ${cardCode}` : cardCode;
    updateModalContractAndPlayUI();
  }

  function updateModalContractAndPlayUI() {
    document.querySelectorAll('.modal-contract-level-btn').forEach(btn => {
      btn.classList.toggle('active', parseInt(btn.dataset.level, 10) === modalContractLevel);
    });
    document.querySelectorAll('.modal-contract-strain-btn').forEach(btn => {
      btn.classList.toggle('active', btn.dataset.strain === modalContractStrain);
    });

    const sim = simulateModalInitialPlay();
    const strainSym = getSuitSymbol(modalContractStrain);
    const totalTarget = 6 + modalContractLevel;
    const remTarget = Math.max(1, totalTarget - sim.nsCompletedTricksWon);

    if (modalContractHint) {
      if (sim.nsCompletedTricksWon > 0) {
        modalContractHint.textContent =
          `${modalContractLevel}${strainSym} (${remTarget} more trick${remTarget !== 1 ? 's' : ''} needed; ${sim.nsCompletedTricksWon} won)`;
      } else {
        modalContractHint.textContent = `${modalContractLevel}${strainSym} (${totalTarget} tricks)`;
      }
    }

    const order = ['N', 'E', 'S', 'W'];
    const seatNames = { N: 'North', E: 'East', S: 'South', W: 'West' };
    const declIdx = Math.max(0, order.indexOf(currentDeclarerSeat || 'N'));
    const getRoleLabel = (origIdx) => {
      if (origIdx === declIdx) return 'Declarer';
      if (origIdx === (declIdx + 1) % 4) return 'LHO';
      if (origIdx === (declIdx + 2) % 4) return 'Dummy';
      return 'RHO';
    };

    if (modalNextLeadHint) {
      const nextRole = getRoleLabel(sim.nextSeatOrigIdx);
      const nextName = seatNames[sim.nextSeatKey] || sim.nextSeatKey;
      if (sim.completedTricks.length === 0 && sim.currentTrickCards.length === 0) {
        modalNextLeadHint.textContent = `Trick 1 opening lead by ${nextName} (${nextRole})`;
      } else if (sim.currentTrickCards.length === 0) {
        modalNextLeadHint.textContent = `Trick ${sim.nextTrickNum} lead by ${nextName} (${nextRole})`;
      } else {
        modalNextLeadHint.textContent =
          `Trick ${sim.nextTrickNum} (${sim.currentTrickCards.length}/4): ${nextName} (${nextRole}) to play`;
      }
    }

    const hasInputText = Boolean(scanInitialCardsInput && scanInitialCardsInput.value.trim().length > 0);
    if (scanUndoCardBtn) scanUndoCardBtn.disabled = !hasInputText;
    if (scanClearCardsBtn) scanClearCardsBtn.disabled = !hasInputText;

    if (scanInitialPlayError) {
      if (sim.error) {
        scanInitialPlayError.textContent = sim.error;
        scanInitialPlayError.classList.remove('hidden');
      } else {
        scanInitialPlayError.textContent = '';
        scanInitialPlayError.classList.add('hidden');
      }
    }

    if (scanInitialPlayedList) {
      scanInitialPlayedList.innerHTML = '';
      if (sim.completedTricks.length === 0 && sim.currentTrickCards.length === 0) {
        scanInitialPlayedList.classList.add('hidden');
      } else {
        scanInitialPlayedList.classList.remove('hidden');
        const formatCardsHtml = (cards) => cards.map(c =>
          `<span class="${c.suitClass}">${c.suitSym}</span>${c.rankStr}`
        ).join(' ');

        sim.completedTricks.forEach(tr => {
          const pill = document.createElement('span');
          pill.className = 'scan-played-trick-pill';
          pill.innerHTML =
            `<strong>T${tr.trickNum} (${tr.leaderSeat}):</strong> ${formatCardsHtml(tr.cards)} ` +
            `<span class="trick-winner-tag">→ ${tr.winnerSeat} wins</span>`;
          scanInitialPlayedList.appendChild(pill);
        });

        if (sim.currentTrickCards.length > 0) {
          const pill = document.createElement('span');
          pill.className = 'scan-played-trick-pill partial-trick';
          const leadSeat = order[sim.currentLeaderOrigIdx];
          pill.innerHTML =
            `<strong>T${sim.nextTrickNum} (${leadSeat}):</strong> ${formatCardsHtml(sim.currentTrickCards)} ` +
            `<span class="trick-winner-tag">(${sim.currentTrickCards.length}/4)</span>`;
          scanInitialPlayedList.appendChild(pill);
        }
      }
    }

    if (scanInitialCardPicker) {
      scanInitialCardPicker.innerHTML = '';
      if (sim.completedTricks.length >= 13) {
        scanInitialCardPicker.textContent = 'All 13 tricks completed.';
        return;
      }
      const nextSeatSuits = sim.remHandsAfterAll[sim.nextSeatKey] || { S: [], H: [], D: [], C: [] };
      const totalRem = ['S', 'H', 'D', 'C'].reduce((acc, s) => acc + (nextSeatSuits[s] ? nextSeatSuits[s].length : 0), 0);
      if (totalRem === 0) {
        return;
      }

      let legalSuits = new Set(['S', 'H', 'D', 'C']);
      if (sim.currentTrickCards.length > 0) {
        const leadSuit = sim.currentTrickCards[0].suit;
        if (nextSeatSuits[leadSuit] && nextSeatSuits[leadSuit].length > 0) {
          legalSuits = new Set([leadSuit]);
        }
      }

      const nextRole = getRoleLabel(sim.nextSeatOrigIdx);
      const header = document.createElement('div');
      header.className = 'scan-picker-header';
      header.innerHTML =
        `Tap a card from <strong>${seatNames[sim.nextSeatKey]} (${nextRole})</strong> to add to initial play:`;
      scanInitialCardPicker.appendChild(header);

      const suitsRow = document.createElement('div');
      suitsRow.className = 'scan-picker-suits';
      const suitDefs = [
        { key: 'S', sym: '♠', cls: 'card-suit-s' },
        { key: 'H', sym: '♥', cls: 'card-suit-h' },
        { key: 'D', sym: '♦', cls: 'card-suit-d' },
        { key: 'C', sym: '♣', cls: 'card-suit-c' }
      ];

      suitDefs.forEach(sd => {
        const group = document.createElement('div');
        group.className = 'scan-picker-suit-group';
        const symSpan = document.createElement('span');
        symSpan.className = `scan-picker-suit-sym ${sd.cls}`;
        symSpan.textContent = sd.sym;
        group.appendChild(symSpan);

        const ranks = nextSeatSuits[sd.key] || [];
        if (ranks.length === 0) {
          const emptySpan = document.createElement('span');
          emptySpan.style.color = '#64748b';
          emptySpan.style.fontSize = '0.72rem';
          emptySpan.textContent = '-';
          group.appendChild(emptySpan);
        } else {
          const isSuitLegal = legalSuits.has(sd.key) && !sim.error;
          ranks.forEach(r => {
            const btn = document.createElement('button');
            btn.type = 'button';
            btn.className = 'scan-picker-card-btn';
            btn.textContent = r;
            btn.disabled = !isSuitLegal;
            btn.title = isSuitLegal
              ? `Play ${sd.sym}${r} from ${seatNames[sim.nextSeatKey]}`
              : `Must follow suit to ${sim.currentTrickCards[0] ? sim.currentTrickCards[0].suit : ''}`;
            btn.addEventListener('click', () => {
              appendModalCardToken(`${sd.key}${r}`);
            });
            group.appendChild(btn);
          });
        }
        suitsRow.appendChild(group);
      });
      scanInitialCardPicker.appendChild(suitsRow);
    }
  }

  function syncModalContractFromMainUI() {
    if (trumpSelect && trumpSelect.value) {
      modalContractStrain = trumpSelect.value;
    }
    if (targetTricks && targetTricks.value) {
      const tgt = parseInt(targetTricks.value, 10);
      if (!isNaN(tgt)) {
        modalContractLevel = Math.max(1, Math.min(7, tgt - 6));
      }
    }
  }

  function applyBoardMetadataToModal(box) {
    if (!box) return;
    if (box.pbnDeclarer && ['N', 'E', 'S', 'W'].includes(box.pbnDeclarer)) {
      currentDeclarerSeat = box.pbnDeclarer;
    }
    if (box.pbnContract) {
      if (box.pbnContract.level >= 1 && box.pbnContract.level <= 7) {
        modalContractLevel = box.pbnContract.level;
      }
      if (box.pbnContract.strain) {
        modalContractStrain = box.pbnContract.strain;
      }
    }
    updateDeclarerBarUI(currentDeclarerSeat);
  }

  function parsePbnDealString(dealStr) {
    const m = (dealStr || '').trim().match(/^([NESW]):\s*(.+)$/i);
    if (!m) return null;
    const firstSeat = m[1].toUpperCase();
    const handTokens = m[2].trim().split(/\s+/);
    if (handTokens.length < 4) return null;
    const order = ['N', 'E', 'S', 'W'];
    const startIdx = order.indexOf(firstSeat);
    if (startIdx === -1) return null;
    const suitKeys = ['S', 'H', 'D', 'C'];
    const hands = {};
    for (let i = 0; i < 4; i++) {
      const seat = order[(startIdx + i) % 4];
      const suits = handTokens[i].split('.');
      if (suits.length !== 4) return null;
      hands[seat] = suitKeys.map((sk, sIdx) => {
        const r = (suits[sIdx] || '').replace(/10/g, 'T').replace(/-/g, '').toUpperCase();
        return `${sk} ${r.length > 0 ? r : '-'};`;
      }).join(' ');
    }
    return hands;
  }

  function parsePbnBoardsFromText(text) {
    if (!text || !text.includes('[Deal ')) return [];
    const boards = [];
    const lines = text.split(/\r?\n/);
    let curBoard = null;
    let curDeclarer = null;
    let curContract = null;
    for (const line of lines) {
      const bm = line.match(/^\[Board\s+"([^"]+)"\]/i);
      if (bm) curBoard = bm[1].trim();
      const decm = line.match(/^\[Declarer\s+"([NESW])"\]/i);
      if (decm) {
        curDeclarer = decm[1].toUpperCase();
        if (boards.length > 0 && !curBoard && !boards[boards.length - 1].declarer) {
          boards[boards.length - 1].declarer = curDeclarer;
        }
      }
      const cm = line.match(/^\[Contract\s+"([1-7])(NT|N|S|H|D|C)/i);
      if (cm) {
        const lvl = parseInt(cm[1], 10);
        const stRaw = cm[2].toUpperCase();
        const strainMap = {
          C: 'CLUBS', D: 'DIAMONDS', H: 'HEARTS', S: 'SPADES', N: 'NOTRUMP', NT: 'NOTRUMP'
        };
        curContract = { level: lvl, strain: strainMap[stRaw] || 'NOTRUMP' };
        if (boards.length > 0 && !curBoard && !boards[boards.length - 1].contract) {
          boards[boards.length - 1].contract = curContract;
        }
      }
      const dm = line.match(/^\[Deal\s+"([^"]+)"\]/i);
      if (dm) {
        const hands = parsePbnDealString(dm[1]);
        if (hands) {
          boards.push({
            board: curBoard || String(boards.length + 1),
            hands,
            declarer: curDeclarer,
            contract: curContract
          });
        }
        curBoard = null;
        curDeclarer = null;
        curContract = null;
      }
    }
    return boards;
  }

  async function tryDecompressPdfStream(chunk, format) {
    if (typeof DecompressionStream === 'undefined') return null;
    try {
      const ds = new DecompressionStream(format);
      const writer = ds.writable.getWriter();
      const writePromise = writer.write(chunk).then(() => writer.close()).catch(() => {});
      const reader = ds.readable.getReader();
      const out = [];
      let total = 0;
      while (true) {
        const { value, done } = await reader.read();
        if (done) break;
        out.push(value);
        total += value.byteLength;
        if (total > 2 * 1024 * 1024) break;
      }
      await writePromise;
      const merged = new Uint8Array(total);
      let offset = 0;
      for (const part of out) {
        merged.set(part, offset);
        offset += part.byteLength;
      }
      return new TextDecoder('latin1').decode(merged);
    } catch (_) {
      return null;
    }
  }

  async function extractPbnBoardsFromPdfBuffer(arrayBuffer) {
    const bytes = new Uint8Array(arrayBuffer);
    const raw = new TextDecoder('latin1').decode(bytes);
    const direct = parsePbnBoardsFromText(raw);
    if (direct.length > 0) return direct;

    let pos = 0;
    while (true) {
      const sIdx = raw.indexOf('stream', pos);
      if (sIdx === -1) break;
      let dataStart = sIdx + 6;
      if (raw[dataStart] === '\r' && raw[dataStart + 1] === '\n') dataStart += 2;
      else if (raw[dataStart] === '\n' || raw[dataStart] === '\r') dataStart += 1;
      else { pos = sIdx + 6; continue; }

      const endIdx = raw.indexOf('endstream', dataStart);
      if (endIdx === -1) break;
      let dataEnd = endIdx;
      if (raw[dataEnd - 2] === '\r' && raw[dataEnd - 1] === '\n') dataEnd -= 2;
      else if (raw[dataEnd - 1] === '\n' || raw[dataEnd - 1] === '\r') dataEnd -= 1;

      const len = dataEnd - dataStart;
      if (len > 16 && len < 1024 * 1024) {
        const chunk = bytes.subarray(dataStart, dataEnd);
        let inflated = await tryDecompressPdfStream(chunk, 'deflate');
        if (!inflated) inflated = await tryDecompressPdfStream(chunk, 'deflate-raw');
        if (inflated && inflated.includes('[Deal ')) {
          const boards = parsePbnBoardsFromText(inflated);
          if (boards.length > 0) return boards;
        }
      }
      pos = endIdx + 9;
    }
    return [];
  }

  function loadPdfJs() {
    if (window.pdfjsLib) return Promise.resolve(window.pdfjsLib);
    if (pdfJsLoadPromise) return pdfJsLoadPromise;
    pdfJsLoadPromise = new Promise((resolve, reject) => {
      const script = document.createElement('script');
      script.src = 'https://cdnjs.cloudflare.com/ajax/libs/pdf.js/3.11.174/pdf.min.js';
      script.async = true;
      script.onload = () => {
        if (window.pdfjsLib) {
          window.pdfjsLib.GlobalWorkerOptions.workerSrc =
            'https://cdnjs.cloudflare.com/ajax/libs/pdf.js/3.11.174/pdf.worker.min.js';
          resolve(window.pdfjsLib);
        } else {
          pdfJsLoadPromise = null;
          reject(new Error('pdfjsLib unavailable'));
        }
      };
      script.onerror = () => {
        pdfJsLoadPromise = null;
        reject(new Error('Failed to load pdf.js'));
      };
      document.head.appendChild(script);
    });
    return pdfJsLoadPromise;
  }

  function renderPbnBoardsSheetToCanvas(boards, pageNum) {
    const perPage = 16;
    const startIdx = (pageNum - 1) * perPage;
    const pageBoards = boards.slice(startIdx, startIdx + perPage);
    const cols = Math.min(4, Math.max(1, pageBoards.length));
    const rows = Math.max(1, Math.ceil(pageBoards.length / cols));
    const cellW = 420;
    const cellH = 260;
    const pad = 20;

    const off = document.createElement('canvas');
    off.width = cols * cellW + pad * 2;
    off.height = rows * cellH + pad * 2;
    const ctx = off.getContext('2d');
    ctx.fillStyle = '#ffffff';
    ctx.fillRect(0, 0, off.width, off.height);

    const boxes = [];
    const suitMeta = [
      { key: 'S', sym: '♠', color: '#0f172a' },
      { key: 'H', sym: '♥', color: '#dc2626' },
      { key: 'D', sym: '♦', color: '#dc2626' },
      { key: 'C', sym: '♣', color: '#0f172a' }
    ];

    pageBoards.forEach((b, i) => {
      const col = i % cols;
      const row = Math.floor(i / cols);
      const bx = pad + col * cellW;
      const by = pad + row * cellH;

      ctx.strokeStyle = '#cbd5e1';
      ctx.lineWidth = 1.5;
      ctx.strokeRect(bx + 4, by + 4, cellW - 8, cellH - 8);

      ctx.fillStyle = '#0f172a';
      ctx.font = 'bold 14px sans-serif';
      ctx.fillText(`Board ${b.board}`, bx + 14, by + 24);

      const drawHandAt = (handStr, hx, hy) => {
        const suits = parseHandToSuits(handStr || '');
        suitMeta.forEach((sm, sIdx) => {
          const y = hy + sIdx * 17;
          ctx.fillStyle = sm.color;
          ctx.font = 'bold 14px sans-serif';
          ctx.fillText(sm.sym, hx, y);
          ctx.fillStyle = '#0f172a';
          ctx.font = '13.5px sans-serif';
          const ranks = (suits[sm.key] || '-').replace(/T/g, '10');
          ctx.fillText(ranks, hx + 16, y);
        });
      };

      drawHandAt(b.hands.N, bx + 150, by + 26);
      drawHandAt(b.hands.W, bx + 16, by + 104);
      drawHandAt(b.hands.E, bx + 268, by + 104);
      drawHandAt(b.hands.S, bx + 150, by + 184);

      boxes.push({
        x: bx + 4,
        y: by + 4,
        w: cellW - 8,
        h: cellH - 8,
        boardLabel: String(b.board),
        pbnHands: b.hands,
        pbnDeclarer: b.declarer || null,
        pbnContract: b.contract || null
      });
    });

    return { canvas: off, boxes };
  }

  function updatePdfPageBarUI() {
    if (!pdfPageBar) return;
    if (activePdfTotalPages > 1) {
      pdfPageBar.classList.remove('hidden');
      if (pdfPageIndicator) {
        pdfPageIndicator.textContent = `Page ${activePdfPageNum} / ${activePdfTotalPages}`;
      }
      if (pdfPrevPageBtn) pdfPrevPageBtn.disabled = (activePdfPageNum <= 1);
      if (pdfNextPageBtn) pdfNextPageBtn.disabled = (activePdfPageNum >= activePdfTotalPages);
    } else {
      pdfPageBar.classList.add('hidden');
    }
  }

  async function renderActivePdfPage() {
    updatePdfPageBarUI();
    if (scanInitialCardsInput) scanInitialCardsInput.value = '';
    let pageBoardOffset = 0;
    for (let p = 1; p < activePdfPageNum; p++) {
      pageBoardOffset += (pdfPageBoardCounts[p] || 16);
    }

    if (activePdfDoc) {
      if (scanStatusBadge) {
        scanStatusBadge.className = 'scan-status-badge scan-status-pending';
        scanStatusBadge.textContent = `Rendering PDF page ${activePdfPageNum} of ${activePdfTotalPages}...`;
      }
      const page = await activePdfDoc.getPage(activePdfPageNum);
      const viewport = page.getViewport({ scale: 2.2 });
      const off = document.createElement('canvas');
      off.width = Math.round(viewport.width);
      off.height = Math.round(viewport.height);
      const ctx = off.getContext('2d');
      ctx.fillStyle = '#ffffff';
      ctx.fillRect(0, 0, off.width, off.height);
      await page.render({ canvasContext: ctx, viewport }).promise;
      scanSourceCanvas = off;
      runFullPhotoBoardDetection(pageBoardOffset);
      pdfPageBoardCounts[activePdfPageNum] = detectedBoardBoxes.length || 16;
      return;
    }

    if (activePdfPbnBoards.length > 0) {
      const { canvas, boxes } = renderPbnBoardsSheetToCanvas(activePdfPbnBoards, activePdfPageNum);
      scanSourceCanvas = canvas;
      detectedBoardBoxes = boxes;
      pdfPageBoardCounts[activePdfPageNum] = boxes.length;
      if (detectedBoardBoxes.length > 0) {
        activeDetectedBoardIdx = 0;
        cropBox = { ...detectedBoardBoxes[0] };
        applyBoardMetadataToModal(detectedBoardBoxes[0]);
      }
      renderDetectedBoardsPills();
      fitCropPhotoView();
      if (detectedBoardBoxes.length > 1) {
        zoomCropViewToBox();
      }
      scanCurrentCropRegion();
    }
  }

  function openPhotoScanModalWithPbnBoards(pbnBoards) {
    activePdfDoc = null;
    activePdfPbnBoards = pbnBoards;
    activePdfPageNum = 1;
    activePdfTotalPages = Math.max(1, Math.ceil(pbnBoards.length / 16));
    pdfPageBoardCounts = {};
    syncModalContractFromMainUI();
    if (scanInitialCardsInput) scanInitialCardsInput.value = '';
    if (recropPhotoBtn) recropPhotoBtn.classList.remove('hidden');
    if (photoScanModal) photoScanModal.classList.remove('hidden');
    updateDeclarerBarUI(currentDeclarerSeat);
    requestAnimationFrame(() => {
      renderActivePdfPage();
    });
  }

  async function openPhotoScanModalWithPdfFile(file) {
    activePdfDoc = null;
    activePdfPbnBoards = [];
    activePdfPageNum = 1;
    activePdfTotalPages = 1;
    pdfPageBoardCounts = {};
    syncModalContractFromMainUI();
    if (scanInitialCardsInput) scanInitialCardsInput.value = '';

    if (photoScanModal) photoScanModal.classList.remove('hidden');
    if (recropPhotoBtn) recropPhotoBtn.classList.remove('hidden');
    updateDeclarerBarUI(currentDeclarerSeat);
    if (scanStatusBadge) {
      scanStatusBadge.className = 'scan-status-badge scan-status-pending';
      scanStatusBadge.textContent = 'Loading PDF hand record...';
    }

    const pdfStartMs = performance.now();
    try {
      const arrayBuffer = await file.arrayBuffer();
      activePdfPbnBoards = await extractPbnBoardsFromPdfBuffer(arrayBuffer);

      // Immediately render embedded PBN boards if present so UI responds in <5ms,
      // then upgrade to visual PDF rendering once pdf.js finishes loading.
      if (activePdfPbnBoards.length > 0) {
        activePdfTotalPages = Math.max(1, Math.ceil(activePdfPbnBoards.length / 16));
        await renderActivePdfPage();
      }

      try {
        const pdfjsLib = await loadPdfJs();
        const loadingTask = pdfjsLib.getDocument({ data: new Uint8Array(arrayBuffer) });
        activePdfDoc = await loadingTask.promise;
        activePdfTotalPages = activePdfDoc.numPages;
        activePdfPageNum = 1;
        await renderActivePdfPage();
      } catch (pdfErr) {
        if (activePdfPbnBoards.length === 0) {
          reportRuntimeErrorToSentry(pdfErr, {
            source: 'pdf_render_error',
            file_name: file && file.name ? file.name : ''
          });
          if (scanStatusBadge) {
            scanStatusBadge.className = 'scan-status-badge scan-status-warn';
            scanStatusBadge.textContent = '⚠ Could not render PDF offline and no embedded PBN deals were found.';
          }
        }
      }
      emitSentryMetricDistribution(
        'pdf_parse_ms',
        Number((performance.now() - pdfStartMs).toFixed(1)),
        'millisecond',
        {
          has_pbn: activePdfPbnBoards.length > 0,
          page_count: activePdfTotalPages
        }
      );
    } catch (err) {
      reportRuntimeErrorToSentry(err, {
        source: 'pdf_read_error',
        file_name: file && file.name ? file.name : ''
      });
      if (scanStatusBadge) {
        scanStatusBadge.className = 'scan-status-badge scan-status-warn';
        scanStatusBadge.textContent = '⚠ Failed to read PDF file.';
      }
    }
  }

  if (pdfPrevPageBtn) {
    pdfPrevPageBtn.addEventListener('click', () => {
      if (activePdfPageNum > 1) {
        activePdfPageNum--;
        renderActivePdfPage();
      }
    });
  }

  if (pdfNextPageBtn) {
    pdfNextPageBtn.addEventListener('click', () => {
      if (activePdfPageNum < activePdfTotalPages) {
        activePdfPageNum++;
        renderActivePdfPage();
      }
    });
  }

  function openPhotoScanModalWithFile(file) {
    activePdfDoc = null;
    activePdfPbnBoards = [];
    activePdfPageNum = 1;
    activePdfTotalPages = 1;
    pdfPageBoardCounts = {};
    syncModalContractFromMainUI();
    if (scanInitialCardsInput) scanInitialCardsInput.value = '';
    updatePdfPageBarUI();

    const reader = new FileReader();
    reader.onload = (ev) => {
      const img = new Image();
      img.onload = () => {
        const off = document.createElement('canvas');
        off.width = img.naturalWidth || img.width;
        off.height = img.naturalHeight || img.height;
        const ctx = off.getContext('2d');
        ctx.drawImage(img, 0, 0);
        scanSourceCanvas = off;
        if (recropPhotoBtn) recropPhotoBtn.classList.remove('hidden');
        openPhotoScanModalExisting(true);
      };
      img.src = ev.target.result;
    };
    reader.readAsDataURL(file);
  }

  function openPhotoScanModalExisting(resetDetection = false) {
    if (!scanSourceCanvas || !photoScanModal) return;
    if (resetDetection) {
      syncModalContractFromMainUI();
      if (scanInitialCardsInput) scanInitialCardsInput.value = '';
    }
    photoScanModal.classList.remove('hidden');
    updateDeclarerBarUI(currentDeclarerSeat);
    updatePdfPageBarUI();

    requestAnimationFrame(() => {
      if (resetDetection) {
        runFullPhotoBoardDetection();
      } else {
        fitCropPhotoView();
        renderCropCanvas();
        updateModalContractAndPlayUI();
      }
    });
  }

  function closePhotoScanModal() {
    pendingModalFileInfo = null;
    if (photoScanModal) photoScanModal.classList.add('hidden');
  }

  function runFullPhotoBoardDetection(boardOffset = 0) {
    if (!scanSourceCanvas) return;
    const ctx = scanSourceCanvas.getContext('2d');
    const fullData = ctx.getImageData(0, 0, scanSourceCanvas.width, scanSourceCanvas.height);
    const rawBoxes = detectAllBoardsInImageData(fullData);
    detectedBoardBoxes = rawBoxes.map((b, idx) => {
      const pbnEntry = activePdfPbnBoards[boardOffset + idx];
      return {
        ...b,
        boardLabel: pbnEntry ? String(pbnEntry.board) : String(boardOffset + idx + 1),
        pbnHands: pbnEntry ? pbnEntry.hands : null,
        pbnDeclarer: pbnEntry ? (pbnEntry.declarer || null) : null,
        pbnContract: pbnEntry ? (pbnEntry.contract || null) : null
      };
    });

    if (detectedBoardBoxes.length > 0) {
      activeDetectedBoardIdx = 0;
      lastSelectedBoardLabel = detectedBoardBoxes[0].boardLabel || '1';
      cropBox = { ...detectedBoardBoxes[0] };
      applyBoardMetadataToModal(detectedBoardBoxes[0]);
    } else {
      activeDetectedBoardIdx = -1;
      lastSelectedBoardLabel = '1';
      const padX = Math.round(scanSourceCanvas.width * 0.06);
      const padY = Math.round(scanSourceCanvas.height * 0.06);
      cropBox = {
        x: padX,
        y: padY,
        w: Math.max(40, scanSourceCanvas.width - 2 * padX),
        h: Math.max(40, scanSourceCanvas.height - 2 * padY)
      };
    }

    renderDetectedBoardsPills();
    fitCropPhotoView();
    if (detectedBoardBoxes.length > 1) {
      zoomCropViewToBox();
    }
    scanCurrentCropRegion();
  }

  function renderDetectedBoardsPills() {
    if (!detectedBoardsBar || !detectedBoardsList) return;
    detectedBoardsList.innerHTML = '';
    if (detectedBoardBoxes.length <= 1 && activePdfTotalPages <= 1) {
      detectedBoardsBar.classList.add('hidden');
      return;
    }
    detectedBoardsBar.classList.remove('hidden');
    detectedBoardBoxes.forEach((box, idx) => {
      const btn = document.createElement('button');
      btn.type = 'button';
      btn.className = `board-pill-btn${idx === activeDetectedBoardIdx ? ' active' : ''}`;
      const label = box.boardLabel || String(idx + 1);
      btn.textContent = `#${label}`;
      btn.addEventListener('click', () => {
        if (activeDetectedBoardIdx !== idx && scanInitialCardsInput) {
          scanInitialCardsInput.value = '';
        }
        activeDetectedBoardIdx = idx;
        const selectedBox = detectedBoardBoxes[idx];
        lastSelectedBoardLabel = (selectedBox && selectedBox.boardLabel) ? selectedBox.boardLabel : String(idx + 1);
        cropBox = { ...detectedBoardBoxes[idx] };
        applyBoardMetadataToModal(detectedBoardBoxes[idx]);
        renderDetectedBoardsPills();
        zoomCropViewToBox();
        scanCurrentCropRegion();
      });
      detectedBoardsList.appendChild(btn);
    });
  }

  function fitCropPhotoView() {
    if (!scanSourceCanvas || !cropCanvasViewport || !cropCanvas) return;
    const rect = cropCanvasViewport.getBoundingClientRect();
    const cw = Math.max(260, Math.floor(rect.width));
    const ch = Math.max(220, Math.floor(rect.height));
    cropCanvas.width = cw;
    cropCanvas.height = ch;

    const scaleX = (cw - 24) / scanSourceCanvas.width;
    const scaleY = (ch - 24) / scanSourceCanvas.height;
    cropViewZoom = Math.min(scaleX, scaleY);
    cropViewOffsetX = (cw - scanSourceCanvas.width * cropViewZoom) / 2;
    cropViewOffsetY = (ch - scanSourceCanvas.height * cropViewZoom) / 2;
    renderCropCanvas();
  }

  function zoomCropViewToBox() {
    if (!scanSourceCanvas || !cropCanvas) return;
    const cw = cropCanvas.width;
    const ch = cropCanvas.height;
    const scaleX = (cw * 0.82) / Math.max(40, cropBox.w);
    const scaleY = (ch * 0.82) / Math.max(40, cropBox.h);
    cropViewZoom = Math.min(scaleX, scaleY);
    const boxCenterX = cropBox.x + cropBox.w / 2;
    const boxCenterY = cropBox.y + cropBox.h / 2;
    cropViewOffsetX = cw / 2 - boxCenterX * cropViewZoom;
    cropViewOffsetY = ch / 2 - boxCenterY * cropViewZoom;
    renderCropCanvas();
  }

  function clientToImageCoords(clientX, clientY) {
    const rect = cropCanvas.getBoundingClientRect();
    const cx = (clientX - rect.left) * (cropCanvas.width / Math.max(1, rect.width));
    const cy = (clientY - rect.top) * (cropCanvas.height / Math.max(1, rect.height));
    return {
      x: (cx - cropViewOffsetX) / cropViewZoom,
      y: (cy - cropViewOffsetY) / cropViewZoom,
      cx,
      cy
    };
  }

  function renderCropCanvas() {
    if (!cropCanvas || !scanSourceCanvas) return;
    const ctx = cropCanvas.getContext('2d');
    const cw = cropCanvas.width, ch = cropCanvas.height;
    ctx.clearRect(0, 0, cw, ch);

    ctx.save();
    ctx.translate(cropViewOffsetX, cropViewOffsetY);
    ctx.scale(cropViewZoom, cropViewZoom);
    ctx.drawImage(scanSourceCanvas, 0, 0);
    ctx.restore();

    const bx = cropViewOffsetX + cropBox.x * cropViewZoom;
    const by = cropViewOffsetY + cropBox.y * cropViewZoom;
    const bw = cropBox.w * cropViewZoom;
    const bh = cropBox.h * cropViewZoom;

    // Dim outside cropBox
    ctx.fillStyle = 'rgba(8, 13, 26, 0.62)';
    ctx.fillRect(0, 0, cw, Math.max(0, by));
    ctx.fillRect(0, Math.min(ch, by + bh), cw, Math.max(0, ch - (by + bh)));
    ctx.fillRect(0, Math.max(0, by), Math.max(0, bx), Math.max(0, bh));
    ctx.fillRect(Math.min(cw, bx + bw), Math.max(0, by), Math.max(0, cw - (bx + bw)), Math.max(0, bh));

    // Draw other detected board outlines
    detectedBoardBoxes.forEach((db, idx) => {
      if (idx === activeDetectedBoardIdx) return;
      const dx = cropViewOffsetX + db.x * cropViewZoom;
      const dy = cropViewOffsetY + db.y * cropViewZoom;
      const dw = db.w * cropViewZoom;
      const dh = db.h * cropViewZoom;
      const label = db.boardLabel || String(idx + 1);
      ctx.save();
      ctx.strokeStyle = 'rgba(56, 189, 248, 0.65)';
      ctx.lineWidth = 1.5;
      ctx.setLineDash([5, 4]);
      ctx.strokeRect(dx, dy, dw, dh);
      ctx.fillStyle = 'rgba(2, 132, 199, 0.85)';
      ctx.fillRect(dx + 4, dy + 4, 28, 16);
      ctx.fillStyle = '#ffffff';
      ctx.font = 'bold 10px sans-serif';
      ctx.fillText(`#${label}`, dx + 7, dy + 15);
      ctx.restore();
    });

    // Draw active crop rectangle & subtle 3x3 compass grid
    ctx.save();
    ctx.strokeStyle = 'rgba(148, 163, 184, 0.28)';
    ctx.lineWidth = 1;
    ctx.setLineDash([3, 3]);
    ctx.beginPath();
    ctx.moveTo(bx + bw / 3, by); ctx.lineTo(bx + bw / 3, by + bh);
    ctx.moveTo(bx + (2 * bw) / 3, by); ctx.lineTo(bx + (2 * bw) / 3, by + bh);
    ctx.moveTo(bx, by + bh / 3); ctx.lineTo(bx + bw, by + bh / 3);
    ctx.moveTo(bx, by + (2 * bh) / 3); ctx.lineTo(bx + bw, by + (2 * bh) / 3);
    ctx.stroke();
    ctx.restore();

    // Draw detected N/W/E/S suit anchor markers inside cropBox
    if (lastDetectedAnchors) {
      ctx.save();
      ['N', 'W', 'E', 'S'].forEach(dir => {
        const a = lastDetectedAnchors[dir];
        if (!a) return;
        const ax = bx + a.cx * cropViewZoom;
        const ay = by + a.cy * cropViewZoom;
        ctx.fillStyle = 'rgba(16, 185, 129, 0.9)';
        ctx.beginPath();
        ctx.arc(ax, ay, 5, 0, Math.PI * 2);
        ctx.fill();
      });
      ctx.restore();
    }

    // Active crop border and 8 resize handles
    ctx.strokeStyle = '#38bdf8';
    ctx.lineWidth = 2;
    ctx.strokeRect(bx, by, bw, bh);

    const handles = [
      [bx, by], [bx + bw / 2, by], [bx + bw, by],
      [bx, by + bh / 2], [bx + bw, by + bh / 2],
      [bx, by + bh], [bx + bw / 2, by + bh], [bx + bw, by + bh]
    ];
    ctx.fillStyle = '#38bdf8';
    ctx.strokeStyle = '#0b1120';
    ctx.lineWidth = 1.5;
    handles.forEach(([hx, hy]) => {
      ctx.beginPath();
      ctx.arc(hx, hy, 5.5, 0, Math.PI * 2);
      ctx.fill();
      ctx.stroke();
    });
  }

  function scanCurrentCropRegion() {
    if (!scanSourceCanvas) return;
    const sx = Math.max(0, Math.round(cropBox.x));
    const sy = Math.max(0, Math.round(cropBox.y));
    const sw = Math.min(scanSourceCanvas.width - sx, Math.max(32, Math.round(cropBox.w)));
    const sh = Math.min(scanSourceCanvas.height - sy, Math.max(32, Math.round(cropBox.h)));

    // Normalize crop width to ~420px for consistent OCR resolution
    const targetW = 420;
    const scale = (sw < 340 || sw > 560) ? (targetW / sw) : 1.0;
    const tw = Math.max(64, Math.round(sw * scale));
    const th = Math.max(64, Math.round(sh * scale));

    const tempCanvas = document.createElement('canvas');
    tempCanvas.width = tw;
    tempCanvas.height = th;
    const tCtx = tempCanvas.getContext('2d');
    tCtx.drawImage(scanSourceCanvas, sx, sy, sw, sh, 0, 0, tw, th);
    const imgData = tCtx.getImageData(0, 0, tw, th);

    const ocrStartMs = performance.now();
    const res = recognizeBoardFromImageData(imgData);
    emitSentryMetricDistribution(
      'ocr_scan_ms',
      Number((performance.now() - ocrStartMs).toFixed(1)),
      'millisecond',
      { detected: Boolean(res) }
    );
    const activeBox = (activeDetectedBoardIdx >= 0 && detectedBoardBoxes[activeDetectedBoardIdx])
      ? detectedBoardBoxes[activeDetectedBoardIdx]
      : null;
    const fallbackPbnHands = activeBox && activeBox.pbnHands ? activeBox.pbnHands : null;

    if (!res && !fallbackPbnHands) {
      lastDetectedAnchors = null;
      if (scanStatusBadge) {
        scanStatusBadge.className = 'scan-status-badge scan-status-warn';
        scanStatusBadge.textContent = '⚠ Adjust crop box to frame a single 4-hand board diagram.';
      }
      renderCropCanvas();
      return;
    }

    if (res && res.anchors) {
      lastDetectedAnchors = {};
      ['N', 'W', 'E', 'S'].forEach(dir => {
        lastDetectedAnchors[dir] = {
          cx: res.anchors[dir].cx / scale,
          cy: res.anchors[dir].cy / scale
        };
      });
    } else {
      lastDetectedAnchors = null;
    }

    const useHands = (fallbackPbnHands && (!res || !validateFourHandsDeck(res.hands).isExact52))
      ? fallbackPbnHands
      : (res ? res.hands : fallbackPbnHands);

    if (scanInputN) scanInputN.value = useHands.N;
    if (scanInputE) scanInputE.value = useHands.E;
    if (scanInputS) scanInputS.value = useHands.S;
    if (scanInputW) scanInputW.value = useHands.W;

    updateScanValidationBadge();
    updateModalContractAndPlayUI();
    renderCropCanvas();
  }

  function updateScanValidationBadge() {
    if (!scanStatusBadge) return;
    const hands = {
      N: scanInputN ? scanInputN.value : '',
      E: scanInputE ? scanInputE.value : '',
      S: scanInputS ? scanInputS.value : '',
      W: scanInputW ? scanInputW.value : ''
    };
    const v = validateFourHandsDeck(hands);
    if (v.isExact52) {
      scanStatusBadge.className = 'scan-status-badge scan-status-ok';
      scanStatusBadge.textContent = '✓ 52/52 cards verified (13 N · 13 E · 13 S · 13 W)';
    } else {
      scanStatusBadge.className = 'scan-status-badge scan-status-warn';
      const dupText = v.duplicates.length > 0 ? ` · Dup: ${v.duplicates.slice(0, 2).join(', ')}` : '';
      scanStatusBadge.textContent =
        `⚠ Card counts: N=${v.counts.N}, E=${v.counts.E}, S=${v.counts.S}, W=${v.counts.W} (${v.uniqueCount}/52 unique)${dupText}`;
    }
  }

  [scanInputN, scanInputE, scanInputS, scanInputW].forEach(inp => {
    if (inp) {
      inp.addEventListener('input', () => {
        updateScanValidationBadge();
        updateModalContractAndPlayUI();
      });
    }
  });

  function hitTestCropHandle(cx, cy) {
    const bx = cropViewOffsetX + cropBox.x * cropViewZoom;
    const by = cropViewOffsetY + cropBox.y * cropViewZoom;
    const bw = cropBox.w * cropViewZoom;
    const bh = cropBox.h * cropViewZoom;
    const r = 20;

    if (Math.hypot(cx - bx, cy - by) <= r) return 'nw';
    if (Math.hypot(cx - (bx + bw), cy - by) <= r) return 'ne';
    if (Math.hypot(cx - bx, cy - (by + bh)) <= r) return 'sw';
    if (Math.hypot(cx - (bx + bw), cy - (by + bh)) <= r) return 'se';
    if (Math.hypot(cx - (bx + bw / 2), cy - by) <= r) return 'n';
    if (Math.hypot(cx - (bx + bw / 2), cy - (by + bh)) <= r) return 's';
    if (Math.hypot(cx - bx, cy - (by + bh / 2)) <= r) return 'w';
    if (Math.hypot(cx - (bx + bw), cy - (by + bh / 2)) <= r) return 'e';
    if (cx >= bx && cx <= bx + bw && cy >= by && cy <= by + bh) return 'move';
    return 'pan';
  }

  function startCropPointer(clientX, clientY) {
    if (!scanSourceCanvas) return;
    const pt = clientToImageCoords(clientX, clientY);
    cropDragMode = hitTestCropHandle(pt.cx, pt.cy);
    cropDragStartImg = { x: pt.x, y: pt.y };
    cropDragStartClient = { x: pt.cx, y: pt.cy, rawX: clientX, rawY: clientY };
    cropDragStartBox = { ...cropBox, offsetX: cropViewOffsetX, offsetY: cropViewOffsetY };
  }

  function moveCropPointer(clientX, clientY) {
    if (!cropDragMode || !scanSourceCanvas || !cropDragStartBox) return;
    const pt = clientToImageCoords(clientX, clientY);
    const dx = pt.x - cropDragStartImg.x;
    const dy = pt.y - cropDragStartImg.y;
    const minSize = 48;
    const maxW = scanSourceCanvas.width;
    const maxH = scanSourceCanvas.height;

    if (cropDragMode === 'pan') {
      cropViewOffsetX = cropDragStartBox.offsetX + (pt.cx - cropDragStartClient.x);
      cropViewOffsetY = cropDragStartBox.offsetY + (pt.cy - cropDragStartClient.y);
      renderCropCanvas();
      return;
    }

    activeDetectedBoardIdx = -1;
    let { x, y, w, h } = cropDragStartBox;
    if (cropDragMode === 'move') {
      x = Math.max(0, Math.min(maxW - w, x + dx));
      y = Math.max(0, Math.min(maxH - h, y + dy));
    } else {
      if (cropDragMode.includes('w')) {
        const nx = Math.max(0, Math.min(x + w - minSize, x + dx));
        w = (x + w) - nx;
        x = nx;
      }
      if (cropDragMode.includes('e')) {
        w = Math.max(minSize, Math.min(maxW - x, w + dx));
      }
      if (cropDragMode.includes('n')) {
        const ny = Math.max(0, Math.min(y + h - minSize, y + dy));
        h = (y + h) - ny;
        y = ny;
      }
      if (cropDragMode.includes('s')) {
        h = Math.max(minSize, Math.min(maxH - y, h + dy));
      }
    }
    cropBox = { x, y, w, h };
    renderCropCanvas();
  }

  function endCropPointer(clientX, clientY) {
    if (!cropDragMode) return;
    const mode = cropDragMode;
    cropDragMode = null;

    // Check if user tapped on another detected board box
    if (mode === 'pan' && clientX !== undefined && cropDragStartClient) {
      const moved = Math.hypot(clientX - cropDragStartClient.rawX, clientY - cropDragStartClient.rawY);
      if (moved < 8) {
        const pt = clientToImageCoords(clientX, clientY);
        const hitIdx = detectedBoardBoxes.findIndex(b =>
          pt.x >= b.x && pt.x <= b.x + b.w && pt.y >= b.y && pt.y <= b.y + b.h
        );
        if (hitIdx !== -1) {
          if (activeDetectedBoardIdx !== hitIdx && scanInitialCardsInput) {
            scanInitialCardsInput.value = '';
          }
          activeDetectedBoardIdx = hitIdx;
          const hitBox = detectedBoardBoxes[hitIdx];
          lastSelectedBoardLabel = (hitBox && hitBox.boardLabel) ? hitBox.boardLabel : String(hitIdx + 1);
          cropBox = { ...detectedBoardBoxes[hitIdx] };
          applyBoardMetadataToModal(detectedBoardBoxes[hitIdx]);
          renderDetectedBoardsPills();
          scanCurrentCropRegion();
          return;
        }
      }
    }

    if (mode !== 'pan') {
      renderDetectedBoardsPills();
      scanCurrentCropRegion();
    }
  }

  if (cropCanvas) {
    cropCanvas.addEventListener('mousedown', (e) => {
      e.preventDefault();
      startCropPointer(e.clientX, e.clientY);
    });
    window.addEventListener('mousemove', (e) => {
      if (!cropDragMode) return;
      moveCropPointer(e.clientX, e.clientY);
    });
    window.addEventListener('mouseup', (e) => {
      if (!cropDragMode) return;
      endCropPointer(e.clientX, e.clientY);
    });

    cropCanvas.addEventListener('wheel', (e) => {
      if (!scanSourceCanvas) return;
      e.preventDefault();
      const pt = clientToImageCoords(e.clientX, e.clientY);
      const factor = e.deltaY < 0 ? 1.15 : 0.87;
      const newZoom = Math.max(0.15, Math.min(6.0, cropViewZoom * factor));
      cropViewOffsetX = pt.cx - pt.x * newZoom;
      cropViewOffsetY = pt.cy - pt.y * newZoom;
      cropViewZoom = newZoom;
      renderCropCanvas();
    }, { passive: false });

    cropCanvas.addEventListener('touchstart', (e) => {
      if (e.touches.length === 1) {
        e.preventDefault();
        cropPinchDist = 0;
        startCropPointer(e.touches[0].clientX, e.touches[0].clientY);
      } else if (e.touches.length === 2) {
        e.preventDefault();
        cropDragMode = null;
        const dx = e.touches[0].clientX - e.touches[1].clientX;
        const dy = e.touches[0].clientY - e.touches[1].clientY;
        cropPinchDist = Math.hypot(dx, dy);
        cropPinchStartZoom = cropViewZoom;
      }
    }, { passive: false });

    cropCanvas.addEventListener('touchmove', (e) => {
      if (e.touches.length === 1 && cropDragMode) {
        e.preventDefault();
        moveCropPointer(e.touches[0].clientX, e.touches[0].clientY);
      } else if (e.touches.length === 2 && cropPinchDist > 0) {
        e.preventDefault();
        const midClientX = (e.touches[0].clientX + e.touches[1].clientX) / 2;
        const midClientY = (e.touches[0].clientY + e.touches[1].clientY) / 2;
        const pt = clientToImageCoords(midClientX, midClientY);
        const dx = e.touches[0].clientX - e.touches[1].clientX;
        const dy = e.touches[0].clientY - e.touches[1].clientY;
        const dist = Math.hypot(dx, dy);
        if (dist > 0) {
          const newZoom = Math.max(0.15, Math.min(6.0, cropPinchStartZoom * (dist / cropPinchDist)));
          cropViewOffsetX = pt.cx - pt.x * newZoom;
          cropViewOffsetY = pt.cy - pt.y * newZoom;
          cropViewZoom = newZoom;
          renderCropCanvas();
        }
      }
    }, { passive: false });

    cropCanvas.addEventListener('touchend', (e) => {
      if (cropDragMode) {
        const t = (e.changedTouches && e.changedTouches[0]) ? e.changedTouches[0] : null;
        endCropPointer(t ? t.clientX : undefined, t ? t.clientY : undefined);
      }
      cropPinchDist = 0;
    });
  }

  if (cropRotateBtn) {
    cropRotateBtn.addEventListener('click', () => {
      if (!scanSourceCanvas) return;
      const rot = document.createElement('canvas');
      rot.width = scanSourceCanvas.height;
      rot.height = scanSourceCanvas.width;
      const rCtx = rot.getContext('2d');
      rCtx.translate(rot.width / 2, rot.height / 2);
      rCtx.rotate(Math.PI / 2);
      rCtx.drawImage(scanSourceCanvas, -scanSourceCanvas.width / 2, -scanSourceCanvas.height / 2);
      scanSourceCanvas = rot;
      runFullPhotoBoardDetection();
    });
  }

  if (cropZoomToBoxBtn) {
    cropZoomToBoxBtn.addEventListener('click', zoomCropViewToBox);
  }

  if (cropResetViewBtn) {
    cropResetViewBtn.addEventListener('click', fitCropPhotoView);
  }

  if (cropNewPhotoBtn && fileInputTxt) {
    cropNewPhotoBtn.addEventListener('click', () => {
      fileInputTxt.value = '';
      fileInputTxt.click();
    });
  }

  if (rescanCropBtn) {
    rescanCropBtn.addEventListener('click', scanCurrentCropRegion);
  }

  document.querySelectorAll('.modal-declarer-btn').forEach(btn => {
    btn.addEventListener('click', () => {
      updateDeclarerBarUI(btn.dataset.seat);
    });
  });

  document.querySelectorAll('.modal-contract-level-btn').forEach(btn => {
    btn.addEventListener('click', () => {
      const lvl = parseInt(btn.dataset.level, 10);
      if (!isNaN(lvl) && lvl >= 1 && lvl <= 7) {
        modalContractLevel = lvl;
        updateModalContractAndPlayUI();
      }
    });
  });

  document.querySelectorAll('.modal-contract-strain-btn').forEach(btn => {
    btn.addEventListener('click', () => {
      if (btn.dataset.strain) {
        modalContractStrain = btn.dataset.strain;
        updateModalContractAndPlayUI();
      }
    });
  });

  if (scanInitialCardsInput) {
    scanInitialCardsInput.addEventListener('input', () => {
      updateModalContractAndPlayUI();
    });
  }

  if (scanUndoCardBtn && scanInitialCardsInput) {
    scanUndoCardBtn.addEventListener('click', () => {
      const toks = scanInitialCardsInput.value.trim().split(/[\s,;/]+/).filter(Boolean);
      if (toks.length > 0) {
        toks.pop();
        scanInitialCardsInput.value = toks.join(' ');
        updateModalContractAndPlayUI();
      }
    });
  }

  if (scanClearCardsBtn && scanInitialCardsInput) {
    scanClearCardsBtn.addEventListener('click', () => {
      scanInitialCardsInput.value = '';
      updateModalContractAndPlayUI();
    });
  }

  if (closePhotoModalBtn) closePhotoModalBtn.addEventListener('click', closePhotoScanModal);
  if (cancelPhotoModalBtn) cancelPhotoModalBtn.addEventListener('click', closePhotoScanModal);

  if (applyPhotoModalBtn) {
    applyPhotoModalBtn.addEventListener('click', () => {
      if (pendingModalFileInfo) {
        activeLoadedFileInfo = pendingModalFileInfo;
        pendingModalFileInfo = null;
      }

      loadedBoardHands = {
        N: (scanInputN ? scanInputN.value : '').trim(),
        E: (scanInputE ? scanInputE.value : '').trim(),
        S: (scanInputS ? scanInputS.value : '').trim(),
        W: (scanInputW ? scanInputW.value : '').trim()
      };
      closePhotoScanModal();

      let activeBox = (activeDetectedBoardIdx >= 0 && detectedBoardBoxes[activeDetectedBoardIdx])
        ? detectedBoardBoxes[activeDetectedBoardIdx]
        : null;

      if (!activeBox && cropBox && detectedBoardBoxes && detectedBoardBoxes.length > 0) {
        let bestOverlap = 0;
        let bestBox = null;
        for (const box of detectedBoardBoxes) {
          const overlapX = Math.max(0, Math.min(cropBox.x + cropBox.w, box.x + box.w) - Math.max(cropBox.x, box.x));
          const overlapY = Math.max(0, Math.min(cropBox.y + cropBox.h, box.y + box.h) - Math.max(cropBox.y, box.y));
          const overlapArea = overlapX * overlapY;
          if (overlapArea > bestOverlap) {
            bestOverlap = overlapArea;
            bestBox = box;
          }
        }
        if (bestBox && bestOverlap > 0) {
          activeBox = bestBox;
        }
      }

      const boardLabel = activeBox && activeBox.boardLabel
        ? activeBox.boardLabel
        : (lastSelectedBoardLabel || String(activeDetectedBoardIdx >= 0 ? activeDetectedBoardIdx + 1 : 1));

      let sourceMeta = null;
      if (activeLoadedFileInfo) {
        sourceMeta = {
          fileName: activeLoadedFileInfo.fileName,
          boardLabel: boardLabel,
          fileFormat: activeLoadedFileInfo.fileFormat || 'pdf',
          loadType: activeLoadedFileInfo.loadType || 'pdf'
        };
      } else if (loadedHandSource) {
        sourceMeta = {
          ...loadedHandSource,
          boardLabel: boardLabel
        };
      } else {
        sourceMeta = {
          fileName: 'imported_deal',
          boardLabel: boardLabel,
          fileFormat: 'pdf',
          loadType: 'file_import'
        };
      }

      applyDeclarerRotation(currentDeclarerSeat, true);
      registerSessionCustomHand(sourceMeta);
      emitSentryMetricCount('hand_loaded', 1, {
        load_type: (sourceMeta && sourceMeta.loadType) || 'file_import'
      });
    });
  }

  let activeSolveAbortController = null;
  let solveStoppedByUser = false;

  async function requestStopSolve() {
    solveStoppedByUser = true;
    if (stopSolveBtn) stopSolveBtn.disabled = true;
    if (stopSolveActionBtn) stopSolveActionBtn.disabled = true;
    apiStopSolve();
    if (activeSolveAbortController) {
      activeSolveAbortController.abort();
    }
  }

  if (stopSolveBtn) {
    stopSolveBtn.addEventListener('click', requestStopSolve);
  }
  if (stopSolveActionBtn) {
    stopSolveActionBtn.addEventListener('click', requestStopSolve);
  }

  // Solve Deal Event
  solveBtn.addEventListener('click', async () => {
    clearPanels();
    solveStoppedByUser = false;
    activeSolveAbortController = new AbortController();
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
      num_iters: isNaN(parseInt(numIters.value, 10)) ? 100 : parseInt(numIters.value, 10),
      num_threads: isNaN(parsedThreads) ? 16 : parsedThreads
    };

    const initCurTrick = getActiveInitialCurTrick();
    if (initCurTrick.length > 0) {
      payload.cur_trick = initCurTrick;
    }

    if (inputUnknownCards && inputUnknownCards.value.trim()) {
      payload.unknown_cards = inputUnknownCards.value.trim();
    }

    const activeActualDeal = getActiveActualDeal();
    if (activeActualDeal) {
      payload.actual_deal = activeActualDeal;
    }

    // Include hand requirements if entered or if voids were revealed in initial play
    const reqs = {};
    const wHcpLow = document.getElementById('wHcpLow').value;
    const wHcpHigh = document.getElementById('wHcpHigh').value;
    if (wHcpLow || wHcpHigh) {
      reqs.W = {
        hcp_low: wHcpLow ? parseInt(wHcpLow, 10) : 0,
        hcp_high: wHcpHigh ? parseInt(wHcpHigh, 10) : 37
      };
    }
    ['E', 'W'].forEach(seatKey => {
      const voidSet = importedSuitVoids && importedSuitVoids[seatKey];
      if (voidSet && voidSet.size > 0) {
        if (!reqs[seatKey]) reqs[seatKey] = {};
        if (!reqs[seatKey].suit_max) reqs[seatKey].suit_max = {};
        voidSet.forEach(s => {
          reqs[seatKey].suit_max[s] = 0;
        });
      }
    });
    if (Object.keys(reqs).length > 0) {
      payload.requirements = reqs;
    }

    emitSentryMetricCount('solve_started', 1, {
      solve_kind: 'main',
      trump: payload.trump
    });

    let isSolving = true;
    const statusPollTimer = setInterval(async () => {
      if (!isSolving || solveStoppedByUser) return;
      try {
        const stRes = await apiSolveStatus();
        if (!isSolving || solveStoppedByUser || !stRes.ok) return;
        const stData = await stRes.json();
        if (!isSolving || solveStoppedByUser) return;
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
      const response = await apiSolve(payload, activeSolveAbortController.signal);

      const data = await response.json();
      isSolving = false;
      clearInterval(statusPollTimer);
      const elapsed = (performance.now() - startTime).toFixed(1);
      const elapsedSec = ((performance.now() - startTime) / 1000).toFixed(1);

      if (solveStoppedByUser || (data && data.stopped)) {
        setStatus('idle', 'Solve stopped by user.', `${elapsedSec} s`);
        emitSentryMetricCount('solve_stopped', 1, {
          solve_kind: 'main',
          backend_mode: backendMode || 'unknown'
        });
        return;
      }

      if (!response.ok || !data.success) {
        setStatus('error', 'Solver failed: ' + (data.error || 'Unknown error'), `${elapsed} ms`);
        emitSentryMetricCount('solve_failed', 1, {
          solve_kind: 'main',
          backend_mode: backendMode || 'unknown'
        });
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
          curTrick: initCurTrick,
          actualDeal: activeActualDeal ? Object.assign({}, activeActualDeal) : null
        }
      };
      updateProposeLineBarUI();

      // Show summary
      resultsSummary.classList.remove('hidden');
      statTricks.textContent = data.tricks_won !== undefined ? data.tricks_won : '-';
      updateLeadStat(data);
      updateActualStat(data);

      const total = data.total_boards || 1;
      const success = data.success_boards !== undefined ? data.success_boards : (data.covered_boards || 0);
      const failure = data.failure_boards !== undefined ? data.failure_boards : Math.max(0, total - success);
      const successPct = ((success / total) * 100).toFixed(1);
      const failurePct = ((failure / total) * 100).toFixed(1);

      if (statSuccess) statSuccess.textContent = `${success}/${total} (${successPct}%)`;
      if (statFailure) statFailure.textContent = `${failure}/${total} (${failurePct}%)`;
      if (traceHandInput) {
        traceHandInput.placeholder = data.actual_hand ? `0..${total}` : `1..${total}`;
      }

      const solveAttrs = {
        solve_kind: 'main',
        backend_mode: backendMode || 'unknown',
        trump: payload.trump,
        source_type: loadedHandSource
          ? (loadedHandSource.loadType || 'custom')
          : 'preset'
      };
      emitSentryMetricCount('solve_completed', 1, solveAttrs);
      emitSentryMetricDistribution(
        'solve_duration_ms',
        Number(elapsed),
        'millisecond',
        solveAttrs
      );
      emitSentryMetricGauge('solve_num_iters', payload.num_iters, 'none', solveAttrs);
      emitSentryMetricGauge(
        'solve_success_pct',
        Number(successPct),
        'percent',
        solveAttrs
      );

      populateEquivalentLinesControls(data, 0);

      // Render the combined decision tree, then the Distribution Summary and
      // Sample Hands panels for the active view mode (Success vs Failure).
      renderSolveTree();
      updateViewMode(activeViewMode);

    } catch (err) {
      isSolving = false;
      clearInterval(statusPollTimer);
      const elapsedSec = ((performance.now() - startTime) / 1000).toFixed(1);
      if (solveStoppedByUser || (err && err.name === 'AbortError')) {
        setStatus('idle', 'Solve stopped by user.', `${elapsedSec} s`);
        emitSentryMetricCount('solve_stopped', 1, {
          solve_kind: 'main',
          backend_mode: backendMode || 'unknown'
        });
      } else {
        console.error(err);
        setStatus('error', 'Network or server error: ' + err.message);
        emitSentryMetricCount('solve_failed', 1, {
          solve_kind: 'main',
          backend_mode: backendMode || 'unknown'
        });
      }
    } finally {
      isSolving = false;
      clearInterval(statusPollTimer);
      activeSolveAbortController = null;
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
    const allBoards = [-1];
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
      node.board_to_trick = { '-1': trickStr };
      if (boardIdStr !== null) {
        node.board_to_trick[boardIdStr] = trickStr;
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
      updateActualStat(data);

      const total = data.total_boards || 1;
      const success = data.success_boards !== undefined ? data.success_boards : (data.covered_boards || 0);
      const failure = data.failure_boards !== undefined ? data.failure_boards : Math.max(0, total - success);
      const successPct = ((success / total) * 100).toFixed(1);
      const failurePct = ((failure / total) * 100).toFixed(1);

      if (statSuccess) statSuccess.textContent = `${success}/${total} (${successPct}%)`;
      if (statFailure) statFailure.textContent = `${failure}/${total} (${failurePct}%)`;
      if (traceHandInput) {
        traceHandInput.placeholder = data.actual_hand ? `0..${total}` : `1..${total}`;
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
        num_iters: isNaN(parseInt(numIters.value, 10)) ? 100 : parseInt(numIters.value, 10),
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

      const snapActualDeal = (originalSolveSnapshot && originalSolveSnapshot.inputs && originalSolveSnapshot.inputs.actualDeal)
        ? originalSolveSnapshot.inputs.actualDeal
        : getActiveActualDeal();
      if (snapActualDeal) {
        payload.actual_deal = snapActualDeal;
        payload.initial_leader = (originalSolveSnapshot && originalSolveSnapshot.inputs && originalSolveSnapshot.inputs.leader)
          ? originalSolveSnapshot.inputs.leader
          : leaderSelect.value;
      }

      // Preserve any HCP requirements and record void suits when E or W showed
      // out (played a card of a different suit than the suit led) on a prefix
      // trick, earlier in the current trick, or during imported initial play.
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
      ['E', 'W'].forEach(seatKey => {
        const voidSet = importedSuitVoids && importedSuitVoids[seatKey];
        if (voidSet && voidSet.size > 0) {
          if (!reqs[seatKey]) reqs[seatKey] = {};
          if (!reqs[seatKey].suit_max) reqs[seatKey].suit_max = {};
          voidSet.forEach(s => {
            reqs[seatKey].suit_max[s] = 0;
          });
        }
      });
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

      solveStoppedByUser = false;
      activeSolveAbortController = new AbortController();
      setStatus('running', `Computing new line from Trick #${ctx.trickNum}...`);
      computeNewLineBtn.disabled = true;
      solveBtn.disabled = true;
      const startTime = performance.now();
      emitSentryMetricCount('propose_line_used', 1, {
        trick_num: ctx.trickNum,
        cards_count: proposedCards.length
      });
      emitSentryMetricCount('solve_started', 1, {
        solve_kind: 'proposed_line',
        trump: payload.trump
      });

      let isSolving = true;
      const statusPollTimer = setInterval(async () => {
        if (!isSolving || solveStoppedByUser) return;
        try {
          const stRes = await apiSolveStatus();
          if (!isSolving || solveStoppedByUser || !stRes.ok) return;
          const stData = await stRes.json();
          if (!isSolving || solveStoppedByUser) return;
          if (stData && stData.status) {
            const curSec = ((performance.now() - startTime) / 1000).toFixed(1);
            const progress = (typeof stData.progress === 'number') ? stData.progress : null;
            setStatus('running', stData.status, `${curSec} s`, progress);
          }
        } catch (_) {}
      }, 200);

      try {
        const response = await apiSolve(payload, activeSolveAbortController.signal);

        const data = await response.json();
        isSolving = false;
        clearInterval(statusPollTimer);
        const elapsed = (performance.now() - startTime).toFixed(1);
        const elapsedSec = ((performance.now() - startTime) / 1000).toFixed(1);

        if (solveStoppedByUser || (data && data.stopped)) {
          setStatus('idle', 'Solve stopped by user.', `${elapsedSec} s`);
          emitSentryMetricCount('solve_stopped', 1, {
            solve_kind: 'proposed_line',
            backend_mode: backendMode || 'unknown'
          });
          return;
        }

        if (!response.ok || !data.success) {
          setStatus('error', 'Solver failed: ' + (data.error || 'Unknown error'), `${elapsed} ms`);
          emitSentryMetricCount('solve_failed', 1, {
            solve_kind: 'proposed_line',
            backend_mode: backendMode || 'unknown'
          });
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
          traceHandInput.placeholder = data.actual_hand ? `0..${total}` : `1..${total}`;
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
        updateActualStat(data);

        const successPct = ((success / total) * 100).toFixed(1);
        const failurePct = ((failure / total) * 100).toFixed(1);
        if (statSuccess) statSuccess.textContent = `${success}/${total} (${successPct}%)`;
        if (statFailure) statFailure.textContent = `${failure}/${total} (${failurePct}%)`;

        const propSolveAttrs = {
          solve_kind: 'proposed_line',
          backend_mode: backendMode || 'unknown',
          trump: payload.trump
        };
        emitSentryMetricCount('solve_completed', 1, propSolveAttrs);
        emitSentryMetricDistribution(
          'solve_duration_ms',
          Number(elapsed),
          'millisecond',
          propSolveAttrs
        );
        emitSentryMetricGauge(
          'solve_num_iters',
          payload.num_iters,
          'none',
          propSolveAttrs
        );
        emitSentryMetricGauge(
          'solve_success_pct',
          Number(successPct),
          'percent',
          propSolveAttrs
        );

        renderSolveTree();
        updateViewMode(activeViewMode);
      } catch (err) {
        isSolving = false;
        clearInterval(statusPollTimer);
        const elapsedSec = ((performance.now() - startTime) / 1000).toFixed(1);
        if (solveStoppedByUser || (err && err.name === 'AbortError')) {
          setStatus('idle', 'Solve stopped by user.', `${elapsedSec} s`);
          emitSentryMetricCount('solve_stopped', 1, {
            solve_kind: 'proposed_line',
            backend_mode: backendMode || 'unknown'
          });
        } else {
          console.error(err);
          setStatus('error', 'Network or server error: ' + err.message);
          emitSentryMetricCount('solve_failed', 1, {
            solve_kind: 'proposed_line',
            backend_mode: backendMode || 'unknown'
          });
        }
      } finally {
        isSolving = false;
        clearInterval(statusPollTimer);
        activeSolveAbortController = null;
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
    if (type === 'error') {
      lastStatusErrorText = msg;
    }
    if (statusFeedbackBtn) {
      statusFeedbackBtn.classList.toggle('hidden', type !== 'error');
    }
    if (timeStr) execTime.textContent = timeStr;
    const isRunning = (type === 'running');
    if (stopSolveBtn) {
      stopSolveBtn.classList.toggle('hidden', !isRunning);
      if (isRunning) stopSolveBtn.disabled = false;
    }
    if (stopSolveActionBtn) {
      stopSolveActionBtn.classList.toggle('hidden', !isRunning);
      if (isRunning) stopSolveActionBtn.disabled = false;
    }
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
        const bId = card.dataset.boardId !== undefined ? parseInt(card.dataset.boardId, 10) : null;
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
    if (boardId === -1 && currentSolveData.actual_hand) {
      return currentSolveData.actual_hand;
    }
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

  function traceHandByNumber(rawInput, switchTab = true) {
    if (!currentSolveData || !currentTreeData) {
      setTraceHandError('Solve a deal first to trace a hand.');
      return false;
    }
    const cleaned = String(rawInput !== undefined && rawInput !== null ? rawInput : '').replace(/^#/, '').trim();
    const handNum = parseInt(cleaned, 10);
    const totalBoards = currentSolveData.total_boards || 1;
    const minHandNum = currentSolveData.actual_hand ? 0 : 1;
    if (!cleaned || isNaN(handNum) || handNum < minHandNum || handNum > totalBoards) {
      setTraceHandError(`Enter a hand number between ${minHandNum} and ${totalBoards}.`);
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

    if (activeTracedBoardId !== boardId) {
      emitSentryMetricCount('trace_board_used', 1, { hand_num: handNum });
    }
    activeTracedBoardId = boardId;
    activeTracedDeal = findDealByBoardId(boardId);
    activeTracedNodeSet = new Set(pathToLeaf);
    currentCenteredNode = leaf;

    if (traceHandInput) traceHandInput.value = String(handNum);
    if (clearTraceHandBtn) clearTraceHandBtn.classList.remove('hidden');
    setTraceHandError('');
    updateSampleHandCardsSelection();

    if (switchTab) {
      activateTab('tree');
    }
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

  function getNodeFailFraction(node) {
    if (!node) return 0;
    if (node.failure) return 1;
    const wins = Math.max(
      0,
      node.success_boards !== undefined ? node.success_boards : (node.covered_boards || 0)
    );
    const fails = Math.max(0, node.failure_boards || 0);
    const reaching = wins + fails;
    if (reaching <= 0 || fails <= 0) return 0;
    return Math.min(1, Math.max(0, fails / reaching));
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

    const treeDefs = document.createElementNS('http://www.w3.org/2000/svg', 'defs');
    treeRootGroup.appendChild(treeDefs);
    let failGradCounter = 0;

    function attachParentPointers(node, parent = null) {
      if (!node) return;
      node._parent = parent;
      if (node.children && node.children.length > 0) {
        node.children.forEach(c => attachParentPointers(c, node));
      }
    }
    attachParentPointers(rootNode);

    // Calculate layout coordinates recursively using vertical contour packing
    const nodeWidth = 164;
    const nodeHeight = 78;
    const siblingGap = 20;
    const verticalPad = 16;

    function getEdgeLabelWidth(child) {
      if (!child || !child.condition) return 0;
      let condText = child.condition;
      if (condText.startsWith('after ')) {
        condText = condText.substring(6);
      }
      return Math.max(54, condText.length * 7 + 12);
    }

    function shiftSubtree(node, dx) {
      if (!node || dx === 0) return;
      node._x += dx;
      if (node._boxes) {
        for (const b of node._boxes) {
          b.xMin += dx;
          b.xMax += dx;
        }
      }
      if (node.children && node.children.length > 0 && !node._collapsed) {
        node.children.forEach(c => shiftSubtree(c, dx));
      }
    }

    function layoutNode(node, depth = 0, y = 0) {
      node._depth = depth;
      node._y = y;
      node._width = nodeWidth;
      if (node._collapsed === undefined) {
        if (node.collapsed !== undefined) {
          node._collapsed = node.collapsed;
        } else {
          const isContainerRoot = (depth === 0 && (!node.trick || node.trick === '') && node.children && node.children.length > 1);
          node._collapsed = isContainerRoot ? false : true;
        }
      }
      if (!node.children || node.children.length === 0 || node._collapsed) {
        node._x = 0;
        node._boxes = [{
          yMin: y,
          yMax: y + nodeHeight,
          xMin: 0,
          xMax: nodeWidth
        }];
        return;
      }

      const gap = (node.children.length === 1) ? 31 : 62;
      const nextY = y + nodeHeight + gap;
      node.children.forEach(child => layoutNode(child, depth + 1, nextY));

      const combinedBoxes = [...node.children[0]._boxes];
      for (let i = 1; i < node.children.length; i++) {
        const prevChild = node.children[i - 1];
        const currChild = node.children[i];

        let minShift = (prevChild._x + nodeWidth + siblingGap) - currChild._x;

        const wPrev = getEdgeLabelWidth(prevChild);
        const wCurr = getEdgeLabelWidth(currChild);
        if (wPrev > 0 && wCurr > 0) {
          const labelShift = (prevChild._x + wPrev + wCurr + 16) - currChild._x;
          if (labelShift > minShift) {
            minShift = labelShift;
          }
        }

        for (const bCurr of currChild._boxes) {
          for (const bPrev of combinedBoxes) {
            if (bCurr.yMin < bPrev.yMax + verticalPad &&
                bCurr.yMax + verticalPad > bPrev.yMin) {
              const reqShift = (bPrev.xMax + siblingGap) - bCurr.xMin;
              if (reqShift > minShift) {
                minShift = reqShift;
              }
            }
          }
        }

        shiftSubtree(currChild, minShift);
        combinedBoxes.push(...currChild._boxes);
      }

      const firstChild = node.children[0];
      const lastChild = node.children[node.children.length - 1];
      node._x = (firstChild._x + lastChild._x) / 2;

      const startX = node._x + nodeWidth / 2;
      const startY = y + nodeHeight;
      const midY = (startY + nextY) / 2;
      for (const child of node.children) {
        const lw = getEdgeLabelWidth(child);
        if (lw > 0) {
          const endX = child._x + nodeWidth / 2;
          const midX = (startX + endX) / 2;
          combinedBoxes.push({
            yMin: midY - 9,
            yMax: midY + 9,
            xMin: midX - lw / 2,
            xMax: midX + lw / 2
          });
        }
      }

      combinedBoxes.push({
        yMin: y,
        yMax: y + nodeHeight,
        xMin: node._x,
        xMax: node._x + nodeWidth
      });
      node._boxes = combinedBoxes;
    }

    layoutNode(rootNode);
    if (rootNode._boxes && rootNode._boxes.length > 0) {
      let minX = Infinity;
      for (const b of rootNode._boxes) {
        if (b.xMin < minX) minX = b.xMin;
      }
      if (Number.isFinite(minX) && minX !== 0) {
        shiftSubtree(rootNode, -minX);
      }
    }

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

      const failFrac = getNodeFailFraction(node);
      if (failFrac > 0 && failFrac < 1) {
        const gradId = `node-fail-grad-${failGradCounter++}`;
        const splitPct = ((1 - failFrac) * 100).toFixed(2) + '%';
        const baseColor = (node.type === 'condition')
          ? '#1e1b4b'
          : (node.type === 'outcome' ? '#064e3b' : '#1e293b');
        const failColor = '#450a0a';
        const grad = document.createElementNS('http://www.w3.org/2000/svg', 'linearGradient');
        grad.setAttribute('id', gradId);
        grad.setAttribute('x1', '0%');
        grad.setAttribute('y1', '0%');
        grad.setAttribute('x2', '100%');
        grad.setAttribute('y2', '0%');

        const stop1 = document.createElementNS('http://www.w3.org/2000/svg', 'stop');
        stop1.setAttribute('offset', '0%');
        stop1.setAttribute('stop-color', baseColor);
        const stop2 = document.createElementNS('http://www.w3.org/2000/svg', 'stop');
        stop2.setAttribute('offset', splitPct);
        stop2.setAttribute('stop-color', baseColor);
        const stop3 = document.createElementNS('http://www.w3.org/2000/svg', 'stop');
        stop3.setAttribute('offset', splitPct);
        stop3.setAttribute('stop-color', failColor);
        const stop4 = document.createElementNS('http://www.w3.org/2000/svg', 'stop');
        stop4.setAttribute('offset', '100%');
        stop4.setAttribute('stop-color', failColor);

        grad.appendChild(stop1);
        grad.appendChild(stop2);
        grad.appendChild(stop3);
        grad.appendChild(stop4);
        treeDefs.appendChild(grad);
        rect.style.fill = `url(#${gradId})`;
      } else if (failFrac >= 1 && !node.failure) {
        rect.style.fill = '#450a0a';
      }

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
      detail.textContent = detailText;
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
    const withPct = (n, denom = total) => denom > 0 ? `${n} (${((n / denom) * 100).toFixed(1)}%)` : `${n}`;
    if (node.failure) {
      const fails = node.failure_boards !== undefined ? node.failure_boards : (node.covered_boards || 0);
      if (fails === 0 && Array.isArray(node.boards) && node.boards.includes(-1)) {
        return { primary: 'Fails: Actual (#0)', secondary: '' };
      }
      return { primary: `Fails: ${withPct(fails)}`, secondary: '' };
    }
    const wins = node.success_boards !== undefined ? node.success_boards : node.covered_boards;
    if (wins === undefined) return { primary: '', secondary: '' };
    const fails = node.failure_boards || 0;
    if (fails > 0 && node.type !== 'outcome') {
      const branchTotal = wins + fails;
      return {
        primary: `Makes: ${withPct(wins, branchTotal)}`,
        secondary: `Fails: ${withPct(fails, branchTotal)}`
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
      console.assert(tokens.length === 4, 'Expected 4 cards in trick:', rawTrick);

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

    currentInspectedState = {
      leafLabel: node ? (node.label || '') : '',
      selectedTrickNum: null,
      trickSummaries: trickRows.map(tr => {
        const cardsStr = tr.cards
          .map(c => `${c.seatName}:${c.suit || ''}${c.rankStr || ''}`)
          .join(' ');
        return `#${tr.trickNum} (${tr.leadSeatName}): ${cardsStr} -> ${tr.winnerSeat} wins`;
      })
    };

    // Render Inspector Panel HTML
    inspectorContent.innerHTML = '';

    // Traced Hand Banner (when tracing a specific board)
    if (isTracingBoard) {
      const handNum = activeTracedBoardId + 1;
      const isActualHand = (activeTracedBoardId === -1);
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
      const handTitleText = isActualHand
        ? 'Traced Hand #0 (Actual Layout)'
        : `Traced Hand #${handNum}`;
      bannerEl.innerHTML = `
        <div class="traced-hand-banner-header">
          <span class="traced-hand-title">${handTitleText}</span>
          <span class="traced-hand-badge ${isFail ? 'badge-fails' : 'badge-makes'}">${isFail ? 'FAILS' : 'MAKES'}</span>
        </div>
        ${holdingsHtml}
      `;
      inspectorContent.appendChild(bannerEl);
    }

    // Helper to update Deal SVG card states for a selected trick index
    function selectTrickInInspector(selectedIdx, rowElements, updateTreeHighlight = true) {
      if (selectedIdx === null || selectedIdx < 0 || selectedIdx >= trickRows.length) {
        if (currentInspectedState) currentInspectedState.selectedTrickNum = null;
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

      if (currentInspectedState) {
        currentInspectedState.selectedTrickNum = trickRows[selectedIdx].trickNum;
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
          console.assert(playIdx !== -1, 'Expected all 4 seats in trick', tr);
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
    window.setStatus = setStatus;
    window.openFeedbackModal = openFeedbackModal;
    window.initWasmWorker = initWasmWorker;
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
          !/^defender/i.test(trimmed) &&
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

  function zoomAroundPoint(targetZoom, focalX, focalY) {
    const newZoom = Math.max(0.3, Math.min(3.0, targetZoom));
    const worldX = (focalX - panX) / zoomLevel;
    const worldY = (focalY - panY) / zoomLevel;
    zoomLevel = newZoom;
    panX = focalX - worldX * newZoom;
    panY = focalY - worldY * newZoom;
    updateTransform();
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
    const rect = treeViewport.getBoundingClientRect();
    const focalX = e.clientX - rect.left;
    const focalY = e.clientY - rect.top;
    const zoomFactor = e.deltaY < 0 ? 1.1 : 0.9;
    zoomAroundPoint(zoomLevel * zoomFactor, focalX, focalY);
  });

  // Mobile Touch Pan & Pinch-to-Zoom
  let touchDragging = false;
  let touchStartClientX = 0;
  let touchStartClientY = 0;
  let touchStartPanX = 0;
  let touchStartPanY = 0;
  let pinchStartDist = 0;
  let pinchStartZoom = 1.0;
  let pinchStartPanX = 0;
  let pinchStartPanY = 0;
  let pinchStartMidX = 0;
  let pinchStartMidY = 0;

  treeViewport.addEventListener('touchstart', (e) => {
    if (e.touches.length === 1) {
      touchDragging = true;
      pinchStartDist = 0;
      touchStartClientX = e.touches[0].clientX;
      touchStartClientY = e.touches[0].clientY;
      touchStartPanX = panX;
      touchStartPanY = panY;
    } else if (e.touches.length === 2) {
      touchDragging = false;
      const rect = treeViewport.getBoundingClientRect();
      const dx = e.touches[0].clientX - e.touches[1].clientX;
      const dy = e.touches[0].clientY - e.touches[1].clientY;
      pinchStartDist = Math.hypot(dx, dy);
      pinchStartZoom = zoomLevel;
      pinchStartPanX = panX;
      pinchStartPanY = panY;
      pinchStartMidX = (e.touches[0].clientX + e.touches[1].clientX) / 2 - rect.left;
      pinchStartMidY = (e.touches[0].clientY + e.touches[1].clientY) / 2 - rect.top;
    }
  }, { passive: true });

  treeViewport.addEventListener('touchmove', (e) => {
    if (e.touches.length === 1 && touchDragging) {
      const dx = e.touches[0].clientX - touchStartClientX;
      const dy = e.touches[0].clientY - touchStartClientY;
      if (Math.hypot(dx, dy) > 4) {
        e.preventDefault();
        panX = touchStartPanX + dx;
        panY = touchStartPanY + dy;
        updateTransform();
      }
    } else if (e.touches.length === 2 && pinchStartDist > 0) {
      e.preventDefault();
      const rect = treeViewport.getBoundingClientRect();
      const dx = e.touches[0].clientX - e.touches[1].clientX;
      const dy = e.touches[0].clientY - e.touches[1].clientY;
      const dist = Math.hypot(dx, dy);
      if (dist > 0) {
        const midX = (e.touches[0].clientX + e.touches[1].clientX) / 2 - rect.left;
        const midY = (e.touches[0].clientY + e.touches[1].clientY) / 2 - rect.top;
        const newZoom = Math.max(0.3, Math.min(3.0, pinchStartZoom * (dist / pinchStartDist)));
        const worldX = (pinchStartMidX - pinchStartPanX) / pinchStartZoom;
        const worldY = (pinchStartMidY - pinchStartPanY) / pinchStartZoom;
        zoomLevel = newZoom;
        panX = midX - worldX * newZoom;
        panY = midY - worldY * newZoom;
        updateTransform();
      }
    }
  }, { passive: false });

  treeViewport.addEventListener('touchend', (e) => {
    pinchStartDist = 0;
    if (e.touches.length === 1) {
      touchDragging = true;
      touchStartClientX = e.touches[0].clientX;
      touchStartClientY = e.touches[0].clientY;
      touchStartPanX = panX;
      touchStartPanY = panY;
    } else {
      touchDragging = false;
    }
  });

  treeViewport.addEventListener('touchcancel', () => {
    touchDragging = false;
    pinchStartDist = 0;
  });

  zoomInBtn.addEventListener('click', () => {
    const rect = treeViewport.getBoundingClientRect();
    zoomAroundPoint(zoomLevel * 1.2, rect.width / 2, rect.height / 2);
  });
  zoomOutBtn.addEventListener('click', () => {
    const rect = treeViewport.getBoundingClientRect();
    zoomAroundPoint(zoomLevel / 1.2, rect.width / 2, rect.height / 2);
  });
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
      const cappedList = handsList.slice(0, 101);
      cappedList.forEach((deal, idx) => {
        const card = document.createElement('div');
        const boardId = (deal.board_id !== undefined) ? deal.board_id : idx;
        const boardNum = boardId + 1;
        const isActual = (boardId === -1);
        card.className = `sample-deal-card${isActual ? ' sample-deal-card-actual' : ''}${activeTracedBoardId === boardId ? ' traced-deal-card' : ''}`;
        card.dataset.boardId = String(boardId);
        card.title = isActual
          ? 'Click to trace Hand #0 (Actual Layout) in the Decision Tree'
          : `Click to trace Hand #${boardNum} in the Decision Tree`;
        card.addEventListener('click', () => {
          traceHandByNumber(boardNum);
        });

        const idBadge = document.createElement('span');
        idBadge.className = `sample-deal-id${isActual ? ' sample-deal-id-actual' : ''}`;
        if (isActual) {
          const outcomeTag = deal.succeeded !== undefined
            ? ` (${deal.succeeded ? 'MAKES' : 'FAILS'})`
            : '';
          idBadge.textContent = `#0 · ACTUAL${outcomeTag}`;
        } else {
          idBadge.textContent = `#${boardNum}`;
        }
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

  // Sentry User Feedback, Error Reporting & Metrics
  const SENTRY_DSN = 'https://4f773fa337d8e6b1e559790da9ee1fb1@o4512261310709760.ingest.us.sentry.io/4512261325324288';
  const SENTRY_ENVELOPE_URL = 'https://o4512261310709760.ingest.us.sentry.io/api/4512261325324288/envelope/?sentry_key=4f773fa337d8e6b1e559790da9ee1fb1&sentry_version=7&sentry_client=bridge-high-line-web%2F1.0.0';
  const ANON_USER_ID_STORAGE_KEY = 'bridge_anon_user_id';
  let cachedAnonUserId = null;

  const openFeedbackBtn = document.getElementById('openFeedbackBtn');
  const feedbackModal = document.getElementById('feedbackModal');
  const closeFeedbackModalBtn = document.getElementById('closeFeedbackModalBtn');
  const cancelFeedbackBtn = document.getElementById('cancelFeedbackBtn');
  const feedbackForm = document.getElementById('feedbackForm');
  const feedbackNameInput = document.getElementById('feedbackNameInput');
  const feedbackEmailInput = document.getElementById('feedbackEmailInput');
  const feedbackMessageInput = document.getElementById('feedbackMessageInput');
  const feedbackIncludeDealCheckbox = document.getElementById('feedbackIncludeDealCheckbox');
  const feedbackIncludeExplorerCheckbox = document.getElementById('feedbackIncludeExplorerCheckbox');
  const feedbackStatusMsg = document.getElementById('feedbackStatusMsg');
  const submitFeedbackBtn = document.getElementById('submitFeedbackBtn');

  function generateSentryEventId() {
    if (window.crypto && typeof window.crypto.randomUUID === 'function') {
      return window.crypto.randomUUID().replace(/-/g, '');
    }
    let id = '';
    const hex = '0123456789abcdef';
    for (let i = 0; i < 32; i++) {
      id += hex[Math.floor(Math.random() * 16)];
    }
    return id;
  }

  function getOrCreateAnonUserId() {
    if (cachedAnonUserId) return cachedAnonUserId;
    try {
      const saved = localStorage.getItem(ANON_USER_ID_STORAGE_KEY);
      if (saved && /^[0-9a-f-]{16,64}$/i.test(saved)) {
        cachedAnonUserId = saved;
        return cachedAnonUserId;
      }
    } catch (_) {}
    cachedAnonUserId = generateSentryEventId();
    try {
      localStorage.setItem(ANON_USER_ID_STORAGE_KEY, cachedAnonUserId);
    } catch (_) {}
    return cachedAnonUserId;
  }

  function sanitizeSentryAttributes(attrs) {
    if (!attrs || typeof attrs !== 'object') return undefined;
    const clean = {};
    for (const [k, v] of Object.entries(attrs)) {
      if (typeof v === 'string' || typeof v === 'number' ||
          typeof v === 'boolean') {
        clean[k] = v;
      }
    }
    return Object.keys(clean).length > 0 ? clean : undefined;
  }

  function setSentryGlobalAttribute(key, value) {
    if (!window.Sentry || typeof window.Sentry.setAttribute !== 'function') {
      return;
    }
    if (typeof value === 'string' || typeof value === 'number' ||
        typeof value === 'boolean') {
      window.Sentry.setAttribute(key, value);
    }
  }

  function emitSentryMetricCount(name, value = 1, attributes = null) {
    if (!window.Sentry || !window.Sentry.metrics ||
        typeof window.Sentry.metrics.count !== 'function') {
      return;
    }
    const cleanAttrs = sanitizeSentryAttributes(attributes);
    window.Sentry.metrics.count(
      name,
      value,
      cleanAttrs ? { attributes: cleanAttrs } : undefined
    );
  }

  function emitSentryMetricDistribution(name, value, unit = 'none',
                                        attributes = null) {
    if (!window.Sentry || !window.Sentry.metrics ||
        typeof window.Sentry.metrics.distribution !== 'function') {
      return;
    }
    const cleanAttrs = sanitizeSentryAttributes(attributes);
    const opts = { unit };
    if (cleanAttrs) opts.attributes = cleanAttrs;
    window.Sentry.metrics.distribution(name, value, opts);
  }

  function emitSentryMetricGauge(name, value, unit = 'none',
                                 attributes = null) {
    if (!window.Sentry || !window.Sentry.metrics ||
        typeof window.Sentry.metrics.gauge !== 'function') {
      return;
    }
    const cleanAttrs = sanitizeSentryAttributes(attributes);
    const opts = { unit };
    if (cleanAttrs) opts.attributes = cleanAttrs;
    window.Sentry.metrics.gauge(name, value, opts);
  }

  function collectFeedbackDealSnapshot() {
    const snap = {
      preset: presetSelect ? presetSelect.value : '',
      loaded_hand_source: loadedHandSource ? {
        file_name: loadedHandSource.fileName || loadedHandSource.file_name || '',
        board_label: loadedHandSource.boardLabel || loadedHandSource.board_label || '',
        file_format: loadedHandSource.fileFormat || loadedHandSource.file_format || '',
        load_type: loadedHandSource.loadType || loadedHandSource.load_type || ''
      } : null,
      hands: {
        N: inputNorth ? inputNorth.value : '',
        E: inputEast ? inputEast.value : '',
        S: inputSouth ? inputSouth.value : '',
        W: inputWest ? inputWest.value : ''
      },
      unknown_cards: inputUnknownCards ? inputUnknownCards.value.trim() : '',
      trump: trumpSelect ? trumpSelect.value : '',
      leader: leaderSelect ? leaderSelect.value : '',
      min_tricks: targetTricks ? (parseInt(targetTricks.value, 10) || 1) : 1,
      num_iters: numIters ? (parseInt(numIters.value, 10) || 100) : 100,
      cur_trick: getActiveInitialCurTrick(),
      proposed_cards: (activeProposalContext && Array.isArray(activeProposalContext.proposedCards))
        ? activeProposalContext.proposedCards.map(c => `${c.suit}${c.rank}`)
        : []
    };
    const wHcpLowEl = document.getElementById('wHcpLow');
    const wHcpHighEl = document.getElementById('wHcpHigh');
    if ((wHcpLowEl && wHcpLowEl.value) || (wHcpHighEl && wHcpHighEl.value)) {
      snap.w_hcp = {
        low: wHcpLowEl ? wHcpLowEl.value : '',
        high: wHcpHighEl ? wHcpHighEl.value : ''
      };
    }
    if (currentSolveData) {
      snap.last_solve = {
        success: Boolean(currentSolveData.success),
        tricks_won: currentSolveData.tricks_won,
        best_lead: currentSolveData.best_lead || ''
      };
    }
    return snap;
  }

  function countDecisionTreeNodes(root) {
    let total = 0;
    let visible = 0;
    function walk(node, parentCollapsed) {
      if (!node) return;
      total++;
      if (!parentCollapsed) visible++;
      const nextCollapsed = parentCollapsed || Boolean(node._collapsed);
      if (Array.isArray(node.children)) {
        node.children.forEach(ch => walk(ch, nextCollapsed));
      }
    }
    walk(root, false);
    return { total, visible };
  }

  function collectFeedbackExplorerSnapshot() {
    if (currentTreeData) {
      attachParentPointers(currentTreeData);
    }
    const { total: totalNodes, visible: visibleNodes } =
      countDecisionTreeNodes(currentTreeData);

    let selectedNodeInfo = null;
    if (currentCenteredNode) {
      const pathNodes = [];
      let curr = currentCenteredNode;
      while (curr) {
        pathNodes.unshift(curr);
        curr = curr._parent;
      }
      const pathFromRoot = [];
      pathNodes.forEach((pn, idx) => {
        const tr = extractTrickString(pn) || '';
        const cond = pn.condition || '';
        const lbl = pn.label || '';
        if (tr || cond || lbl) {
          pathFromRoot.push(
            `#${idx}${cond ? ' (' + cond + ')' : ''}: ${tr || lbl}`
          );
        }
      });
      selectedNodeInfo = {
        label: currentCenteredNode.label || '',
        trick: extractTrickString(currentCenteredNode) || '',
        condition: currentCenteredNode.condition || '',
        depth: Math.max(0, pathNodes.length - 1),
        is_leaf: Boolean(!currentCenteredNode.children ||
                         currentCenteredNode.children.length === 0),
        failure: Boolean(currentCenteredNode.failure),
        deals: formatNodeDeals(currentCenteredNode),
        path_from_root: pathFromRoot
      };
    }

    const decisionTree = {
      has_tree: Boolean(currentTreeData),
      active_tab: activeTabName || 'tree',
      view_mode: activeViewMode || 'success',
      zoom_level: typeof zoomLevel === 'number'
        ? Number(zoomLevel.toFixed(2))
        : 1.0,
      pan: {
        x: typeof panX === 'number' ? Math.round(panX) : 0,
        y: typeof panY === 'number' ? Math.round(panY) : 0
      },
      total_nodes: totalNodes,
      visible_nodes: visibleNodes,
      selected_node: selectedNodeInfo
    };

    const selectableLines = currentSolveData ? buildSelectableLinesList() : [];
    const activeLineIdx = (currentSolveData &&
                           typeof currentSolveData.active_line_index === 'number')
      ? currentSolveData.active_line_index
      : 0;
    const activeLineEntry = selectableLines[activeLineIdx] || null;
    const inspectorOpen = Boolean(
      treeInspectorPanel && !treeInspectorPanel.classList.contains('collapsed')
    );
    const tracedHandNum = activeTracedBoardId !== null
      ? (activeTracedBoardId + 1)
      : null;

    const lineExplorer = {
      inspector_open: inspectorOpen,
      active_line_index: activeLineIdx,
      active_line_label: activeLineEntry
        ? (activeLineEntry.rawTitle || activeLineEntry.label || '')
        : '',
      total_selectable_lines: selectableLines.length,
      proposed_alt_lines_count: Array.isArray(proposedAltLines)
        ? proposedAltLines.length
        : 0,
      traced_hand: tracedHandNum,
      traced_hand_makes: activeTracedDeal
        ? Boolean(activeTracedDeal.succeeded)
        : null,
      inspected_leaf_label: (currentInspectedState &&
                             currentInspectedState.leafLabel) || '',
      selected_trick_num: currentInspectedState
        ? currentInspectedState.selectedTrickNum
        : null,
      trick_line: (currentInspectedState &&
                   Array.isArray(currentInspectedState.trickSummaries))
        ? currentInspectedState.trickSummaries.slice()
        : [],
      propose_line: activeProposalContext ? {
        trick_num: activeProposalContext.trickNum,
        leader: getSeatName(activeProposalContext.leaderIdx),
        proposed_cards: (activeProposalContext.proposedCards || []).map(
          c => `${c.seatName}:${c.suit || ''}${c.rankStr || ''}`
        )
      } : null,
      compare_lines_selected: Array.from(selectedCompareLineIndices || [])
        .sort((a, b) => a - b)
    };

    return {
      decision_tree: decisionTree,
      line_explorer: lineExplorer
    };
  }

  function initSentryRuntimeErrorReporting() {
    if (!window.Sentry) return;
    const runInit = () => {
      if (!window.Sentry || typeof window.Sentry.init !== 'function') return;
      const environment = window.location.hostname.includes('github.io')
        ? 'github-pages'
        : 'local';
      const anonUserId = getOrCreateAnonUserId();
      window.Sentry.init({
        dsn: SENTRY_DSN,
        environment,
        sendDefaultPii: true,
        beforeSend(event) {
          try {
            const dealSnapshot = collectFeedbackDealSnapshot();
            const explorerSnapshot = collectFeedbackExplorerSnapshot();
            event.contexts = {
              ...(event.contexts || {}),
              deal: dealSnapshot,
              decision_tree: explorerSnapshot.decision_tree,
              line_explorer: explorerSnapshot.line_explorer
            };
            const src = dealSnapshot.loaded_hand_source;
            event.tags = {
              ...(event.tags || {}),
              preset: dealSnapshot.preset || 'custom',
              backend_mode: backendMode || 'unknown',
              trump: dealSnapshot.trump || '',
              leader: dealSnapshot.leader || '',
              active_tab: explorerSnapshot.decision_tree.active_tab,
              view_mode: explorerSnapshot.decision_tree.view_mode,
              ...(src && src.file_name ? { file_name: src.file_name } : {}),
              ...(src && src.board_label ? { board_label: src.board_label } : {})
            };
          } catch (_) {}
          return event;
        }
      });
      if (typeof window.Sentry.setUser === 'function') {
        window.Sentry.setUser({ id: anonUserId, ip_address: '{{auto}}' });
      }
      if (typeof window.Sentry.setAttributes === 'function') {
        window.Sentry.setAttributes({ browser_id: anonUserId, environment });
      }
      emitSentryMetricCount('page_loaded', 1, { environment });
    };
    if (typeof window.Sentry.onLoad === 'function') {
      window.Sentry.onLoad(runInit);
    } else {
      runInit();
    }
  }

  function reportRuntimeErrorToSentry(err, extra = {}) {
    if (!window.Sentry || typeof window.Sentry.captureException !== 'function') return;
    const errorObj = (err instanceof Error)
      ? err
      : new Error(typeof err === 'string' ? err : JSON.stringify(err));
    const tags = {};
    const extraData = {};
    if (extra && typeof extra === 'object') {
      for (const [k, v] of Object.entries(extra)) {
        if (k === 'source') {
          tags.error_source = String(v);
        } else {
          extraData[k] = v;
        }
      }
    }
    window.Sentry.captureException(errorObj, { tags, extra: extraData });
  }

  initSentryRuntimeErrorReporting();

  async function submitSentryFeedback({
    name,
    email,
    message,
    includeDeal,
    includeExplorer
  }) {
    const feedbackEventId = generateSentryEventId();
    const nowIso = new Date().toISOString();
    const dealSnapshot = includeDeal ? collectFeedbackDealSnapshot() : null;
    const explorerSnapshot = includeExplorer
      ? collectFeedbackExplorerSnapshot()
      : null;
    const environment = window.location.hostname.includes('github.io') ? 'github-pages' : 'local';
    const anonUserId = getOrCreateAnonUserId();

    const userObj = {
      id: anonUserId,
      ip_address: '{{auto}}',
      name: name || 'Anonymous',
      ...(email ? { email } : {})
    };

    const feedbackMessage = (message || '').slice(0, 4096);
    const feedbackPayload = {
      event_id: feedbackEventId,
      timestamp: nowIso,
      level: 'info',
      platform: 'javascript',
      environment,
      user: userObj,
      tags: {
        preset: dealSnapshot ? (dealSnapshot.preset || 'custom') : 'none',
        file_name: (dealSnapshot && dealSnapshot.loaded_hand_source) ? dealSnapshot.loaded_hand_source.file_name : '',
        board_label: (dealSnapshot && dealSnapshot.loaded_hand_source) ? dealSnapshot.loaded_hand_source.board_label : '',
        trump: dealSnapshot ? (dealSnapshot.trump || '') : '',
        leader: dealSnapshot ? (dealSnapshot.leader || '') : '',
        backend_mode: backendMode || 'unknown',
        ...(explorerSnapshot ? {
          active_tab: explorerSnapshot.decision_tree.active_tab,
          view_mode: explorerSnapshot.decision_tree.view_mode
        } : {})
      },
      contexts: {
        feedback: {
          name: name || 'Anonymous',
          ...(email ? { contact_email: email } : {}),
          message: feedbackMessage,
          url: window.location.href
        },
        ...(dealSnapshot ? { deal: dealSnapshot } : {}),
        ...(explorerSnapshot ? {
          decision_tree: explorerSnapshot.decision_tree,
          line_explorer: explorerSnapshot.line_explorer
        } : {})
      }
    };

    const feedbackEnvelope = [
      JSON.stringify({ event_id: feedbackEventId, sent_at: nowIso, dsn: SENTRY_DSN }),
      JSON.stringify({ type: 'feedback', content_type: 'application/json' }),
      JSON.stringify(feedbackPayload)
    ].join('\n') + '\n';

    const res = await fetch(SENTRY_ENVELOPE_URL, {
      method: 'POST',
      body: feedbackEnvelope
    });

    if (!res.ok) {
      const errText = await res.text().catch(() => '');
      throw new Error(
        `Sentry returned HTTP ${res.status}${errText ? ': ' + errText.slice(0, 120) : ''}`
      );
    }
    emitSentryMetricCount('feedback_submitted', 1, {
      include_deal: Boolean(includeDeal),
      include_explorer: Boolean(includeExplorer)
    });
    return feedbackEventId;
  }

  function openFeedbackModal(initialMessage) {
    if (!feedbackModal) return;
    try {
      const savedName = localStorage.getItem('bridge_feedback_name');
      const savedEmail = localStorage.getItem('bridge_feedback_email');
      if (savedName && feedbackNameInput && !feedbackNameInput.value) {
        feedbackNameInput.value = savedName;
      }
      if (savedEmail && feedbackEmailInput && !feedbackEmailInput.value) {
        feedbackEmailInput.value = savedEmail;
      }
    } catch (_) {}
    if (typeof initialMessage === 'string' && feedbackMessageInput) {
      feedbackMessageInput.value = initialMessage;
    }
    if (feedbackStatusMsg) {
      feedbackStatusMsg.className = 'feedback-status-msg hidden';
      feedbackStatusMsg.textContent = '';
    }
    feedbackModal.classList.remove('hidden');
    if (feedbackMessageInput) {
      setTimeout(() => feedbackMessageInput.focus(), 20);
    }
  }

  function closeFeedbackModal() {
    if (!feedbackModal) return;
    feedbackModal.classList.add('hidden');
  }

  if (openFeedbackBtn) {
    openFeedbackBtn.addEventListener('click', openFeedbackModal);
  }
  if (statusFeedbackBtn) {
    statusFeedbackBtn.addEventListener('click', () => {
      const errorMsg = lastStatusErrorText || (statusMessage ? statusMessage.textContent : 'Status Error');
      const trumpVal = trumpSelect ? trumpSelect.value : 'SPADES';
      const leaderVal = leaderSelect ? leaderSelect.value : 'NORTH';
      const targetVal = targetTricks ? targetTricks.value : '1';
      const itersVal = numIters ? numIters.value : '100';
      const backendVal = backendMode || 'unknown';

      const reportText = `[Status Error Report]
Error: ${errorMsg}
Backend: ${backendVal}
Deal Context: Trump=${trumpVal}, Leader=${leaderVal}, Target=${targetVal}, Iterations=${itersVal}`;

      openFeedbackModal(reportText);
    });
  }
  if (closeFeedbackModalBtn) {
    closeFeedbackModalBtn.addEventListener('click', closeFeedbackModal);
  }
  if (cancelFeedbackBtn) {
    cancelFeedbackBtn.addEventListener('click', closeFeedbackModal);
  }
  if (feedbackModal) {
    feedbackModal.addEventListener('click', (e) => {
      if (e.target === feedbackModal) {
        closeFeedbackModal();
      }
    });
  }
  window.addEventListener('keydown', (e) => {
    if (e.key === 'Escape' && feedbackModal && !feedbackModal.classList.contains('hidden')) {
      closeFeedbackModal();
    }
  });

  if (feedbackForm) {
    feedbackForm.addEventListener('submit', async (e) => {
      e.preventDefault();
      const name = feedbackNameInput ? feedbackNameInput.value.trim() : '';
      const email = feedbackEmailInput ? feedbackEmailInput.value.trim() : '';
      const message = feedbackMessageInput ? feedbackMessageInput.value.trim() : '';
      const includeDeal = feedbackIncludeDealCheckbox ? feedbackIncludeDealCheckbox.checked : true;
      const includeExplorer = feedbackIncludeExplorerCheckbox
        ? feedbackIncludeExplorerCheckbox.checked
        : true;

      if (!message) return;

      try {
        if (name) localStorage.setItem('bridge_feedback_name', name);
        if (email) localStorage.setItem('bridge_feedback_email', email);
      } catch (_) {}

      if (submitFeedbackBtn) {
        submitFeedbackBtn.disabled = true;
        submitFeedbackBtn.textContent = 'Sending...';
      }
      if (feedbackStatusMsg) {
        feedbackStatusMsg.className = 'feedback-status-msg hidden';
      }

      try {
        await submitSentryFeedback({
          name,
          email,
          message,
          includeDeal,
          includeExplorer
        });
        if (feedbackMessageInput) feedbackMessageInput.value = '';
        if (feedbackStatusMsg) {
          feedbackStatusMsg.textContent = 'Thank you! Your feedback has been recorded.';
          feedbackStatusMsg.className = 'feedback-status-msg success';
        }
        setTimeout(() => {
          closeFeedbackModal();
        }, 1200);
      } catch (err) {
        if (feedbackStatusMsg) {
          feedbackStatusMsg.textContent = `Failed to send feedback: ${err.message || err}`;
          feedbackStatusMsg.className = 'feedback-status-msg error';
        }
      } finally {
        if (submitFeedbackBtn) {
          submitFeedbackBtn.disabled = false;
          submitFeedbackBtn.textContent = 'Send Feedback';
        }
      }
    });
  }

  const openHelpBtn = document.getElementById('openHelpBtn');
  const helpModal = document.getElementById('helpModal');
  const helpModalTitle = document.getElementById('helpModalTitle');
  const helpTopicsNav = document.getElementById('helpTopicsNav');
  const closeHelpModalBtn = document.getElementById('closeHelpModalBtn');
  const helpModalBody = document.getElementById('helpModalBody');
  const helpOpenNewTabLink = document.getElementById('helpOpenNewTabLink');
  let helpGuideLoaded = false;
  let activeHelpTopic = 'loading';
  const helpTopicsHtml = {
    loading: '',
    tree: '',
    lines: '',
    summaries: '',
    strategy: ''
  };
  const helpTopicTitles = {
    loading: 'Help — Loading Hands & Deal Setup',
    tree: 'Help — Decision Tree & Line Inspector',
    lines: 'Help — Trying & Comparing Alternative Lines',
    summaries: 'Help — Summaries, Sample Hands & Actual Deal (#0)',
    strategy: 'Help — How the Solver Thinks About Bridge'
  };

  function escapeHelpHtml(str) {
    return String(str)
      .replace(/&/g, '&amp;')
      .replace(/</g, '&lt;')
      .replace(/>/g, '&gt;')
      .replace(/"/g, '&quot;');
  }

  function formatHelpInline(text) {
    const codeSlots = [];
    let s = String(text).replace(/`([^`]+)`/g, (_, code) => {
      const idx = codeSlots.length;
      codeSlots.push(`<code>${escapeHelpHtml(code)}</code>`);
      return `\u0000CODE${idx}\u0000`;
    });
    s = escapeHelpHtml(s);
    s = s.replace(/!\[([^\]]*)\]\(([^)]+)\)/g, (_, alt, src) => {
      return `<img src="${src}" alt="${alt}" class="help-inline-img" loading="lazy">`;
    });
    s = s.replace(/\[([^\]]+)\]\(([^)]+)\)/g, (_, label, href) => {
      let url = href;
      if (url.startsWith('../')) {
        url = 'https://github.com/mflaster/bridge-high-line/blob/main/' + url.slice(3);
      }
      return `<a href="${url}" target="_blank" rel="noopener noreferrer">${label}</a>`;
    });
    s = s.replace(/\*\*([^*]+)\*\*/g, '<strong>$1</strong>');
    s = s.replace(/\*([^*]+)\*/g, '<em>$1</em>');
    s = s.replace(/\u0000CODE(\d+)\u0000/g, (_, idx) => codeSlots[Number(idx)] || '');
    return s;
  }

  function renderHelpListItems(items) {
    return items.map(it => {
      let inner = formatHelpInline(it.text);
      if (it.subItems && it.subItems.length > 0) {
        const tag = it.subType === 'ol' ? 'ol' : 'ul';
        inner += `<${tag}>` +
          it.subItems.map(sub => `<li>${formatHelpInline(sub)}</li>`).join('') +
          `</${tag}>`;
      }
      return `<li>${inner}</li>`;
    }).join('');
  }

  function renderBridgeStrategyMarkdown(md) {
    const lines = String(md).split(/\r?\n/);
    const out = [];
    let i = 0;
    while (i < lines.length) {
      const line = lines[i];
      if (!line.trim()) {
        i++;
        continue;
      }
      const imgMatch = line.trim().match(/^!\[([^\]]*)\]\(([^)]+)\)$/);
      if (imgMatch) {
        const alt = escapeHelpHtml(imgMatch[1]);
        const src = escapeHelpHtml(imgMatch[2]);
        out.push(
          `<figure class="help-figure">` +
          `<img src="${src}" alt="${alt}" loading="lazy" onerror="if(!this.dataset.fb){this.dataset.fb='1';this.src='../docs/'+this.getAttribute('src');}">` +
          (alt ? `<figcaption>${alt}</figcaption>` : '') +
          `</figure>`
        );
        i++;
        continue;
      }
      if (line.startsWith('```mermaid')) {
        i++;
        const steps = [];
        while (i < lines.length && !lines[i].startsWith('```')) {
          const matches = lines[i].matchAll(/\["([^"]+)"\]/g);
          for (const m of matches) {
            steps.push(m[1]);
          }
          i++;
        }
        if (i < lines.length) i++;
        if (steps.length > 0) {
          out.push('<div class="help-pipeline">' + steps.map(st => {
            const parts = st.split(/<br\s*\/?>/i).map(p => escapeHelpHtml(p));
            return `<div class="help-pipeline-step">${parts.join('<br>')}</div>`;
          }).join('') + '</div>');
        }
        continue;
      }
      if (line.startsWith('```')) {
        i++;
        const codeLines = [];
        while (i < lines.length && !lines[i].startsWith('```')) {
          codeLines.push(escapeHelpHtml(lines[i]));
          i++;
        }
        if (i < lines.length) i++;
        out.push(`<pre><code>${codeLines.join('\n')}</code></pre>`);
        continue;
      }
      if (line.startsWith('---')) {
        out.push('<hr>');
        i++;
        continue;
      }
      const hMatch = line.match(/^(#{1,3})\s+(.*)$/);
      if (hMatch) {
        const lvl = hMatch[1].length;
        if (lvl === 1 && out.length === 0) {
          i++;
          continue;
        }
        out.push(`<h${lvl}>${formatHelpInline(hMatch[2])}</h${lvl}>`);
        i++;
        continue;
      }
      if (line.startsWith('|')) {
        const tableLines = [];
        while (i < lines.length && lines[i].startsWith('|')) {
          tableLines.push(lines[i]);
          i++;
        }
        if (tableLines.length >= 2) {
          const parseRow = r => r.trim().replace(/^\||\|$/g, '').split('|').map(c => c.trim());
          const headers = parseRow(tableLines[0]);
          const bodyRows = tableLines.slice(2).map(parseRow);
          out.push('<table><thead><tr>' +
            headers.map(h => `<th>${formatHelpInline(h)}</th>`).join('') +
            '</tr></thead><tbody>' +
            bodyRows.map(r => '<tr>' + r.map(c => `<td>${formatHelpInline(c)}</td>`).join('') + '</tr>').join('') +
            '</tbody></table>');
        }
        continue;
      }
      if (line.startsWith('> ')) {
        const qLines = [];
        while (i < lines.length && lines[i].startsWith('> ')) {
          qLines.push(lines[i].slice(2).trim());
          i++;
        }
        out.push(`<blockquote>${formatHelpInline(qLines.join(' '))}</blockquote>`);
        continue;
      }
      if (line.startsWith('- ')) {
        const items = [];
        while (i < lines.length && (lines[i].startsWith('- ') || lines[i].startsWith('  '))) {
          const cur = lines[i];
          if (cur.startsWith('- ')) {
            items.push({ text: cur.slice(2).trim(), subType: null, subItems: [] });
          } else if (items.length > 0) {
            const last = items[items.length - 1];
            const trimmed = cur.trim();
            if (trimmed.startsWith('- ')) {
              last.subType = 'ul';
              last.subItems.push(trimmed.slice(2).trim());
            } else if (/^\d+\.\s+/.test(trimmed)) {
              last.subType = 'ol';
              last.subItems.push(trimmed.replace(/^\d+\.\s+/, '').trim());
            } else if (last.subItems.length > 0) {
              last.subItems[last.subItems.length - 1] += ' ' + trimmed;
            } else {
              last.text += ' ' + trimmed;
            }
          }
          i++;
        }
        out.push('<ul>' + renderHelpListItems(items) + '</ul>');
        continue;
      }
      if (/^\d+\.\s+/.test(line)) {
        const items = [];
        while (i < lines.length && (/^\d+\.\s+/.test(lines[i]) || lines[i].startsWith('  '))) {
          const cur = lines[i];
          if (/^\d+\.\s+/.test(cur)) {
            items.push({ text: cur.replace(/^\d+\.\s+/, '').trim(), subType: null, subItems: [] });
          } else if (items.length > 0) {
            const last = items[items.length - 1];
            const trimmed = cur.trim();
            if (trimmed.startsWith('- ')) {
              last.subType = 'ul';
              last.subItems.push(trimmed.slice(2).trim());
            } else if (/^\d+\.\s+/.test(trimmed)) {
              last.subType = 'ol';
              last.subItems.push(trimmed.replace(/^\d+\.\s+/, '').trim());
            } else if (last.subItems.length > 0) {
              last.subItems[last.subItems.length - 1] += ' ' + trimmed;
            } else {
              last.text += ' ' + trimmed;
            }
          }
          i++;
        }
        out.push('<ol>' + renderHelpListItems(items) + '</ol>');
        continue;
      }
      const pLines = [];
      while (
        i < lines.length &&
        lines[i].trim() &&
        !lines[i].trim().match(/^!\[([^\]]*)\]\(([^)]+)\)$/) &&
        !lines[i].startsWith('#') &&
        !lines[i].startsWith('---') &&
        !lines[i].startsWith('```') &&
        !lines[i].startsWith('|') &&
        !lines[i].startsWith('> ') &&
        !lines[i].startsWith('- ') &&
        !/^\d+\.\s+/.test(lines[i])
      ) {
        pLines.push(lines[i].trim());
        i++;
      }
      out.push(`<p>${formatHelpInline(pLines.join(' '))}</p>`);
    }
    return out.join('\n');
  }

  function parseUserGuideIntoTopics(md) {
    const sections = String(md).split(/\n(?=##\s+)/);
    const topicKeys = ['loading', 'tree', 'lines', 'summaries'];
    let topicIdx = 0;
    for (const sec of sections) {
      const trimmed = sec.trim().replace(/\n---\s*$/, '').trim();
      if (!trimmed.startsWith('## ')) continue;
      if (topicIdx < topicKeys.length) {
        helpTopicsHtml[topicKeys[topicIdx]] = renderBridgeStrategyMarkdown(trimmed);
        topicIdx++;
      }
    }
  }

  async function fetchHelpMarkdown(fileName) {
    let res = await fetch(fileName, { cache: 'no-cache' });
    if (!res.ok) {
      res = await fetch(`../docs/${fileName}`, { cache: 'no-cache' });
    }
    if (!res.ok) throw new Error(`HTTP ${res.status}`);
    return res.text();
  }

  function selectHelpTopic(topic) {
    const targetTopic = helpTopicsHtml[topic] !== undefined ? topic : 'loading';
    activeHelpTopic = targetTopic;
    if (helpTopicsNav) {
      helpTopicsNav.querySelectorAll('.help-topic-btn').forEach(btn => {
        btn.classList.toggle('active', btn.dataset.topic === targetTopic);
      });
    }
    if (helpModalTitle && helpTopicTitles[targetTopic]) {
      helpModalTitle.textContent = helpTopicTitles[targetTopic];
    }
    if (helpOpenNewTabLink) {
      helpOpenNewTabLink.href = `help.html#${targetTopic}`;
    }
    if (helpModalBody && helpGuideLoaded) {
      helpModalBody.innerHTML = helpTopicsHtml[targetTopic] || '<p>No content available.</p>';
      helpModalBody.scrollTop = 0;
    }
  }

  async function openHelpModal(initialTopic = null) {
    if (!helpModal) return;
    if (typeof initialTopic === 'string' && helpTopicsHtml[initialTopic] !== undefined) {
      activeHelpTopic = initialTopic;
    }
    helpModal.classList.remove('hidden');
    selectHelpTopic(activeHelpTopic);
    if (helpGuideLoaded || !helpModalBody) return;
    try {
      const [userGuideMd, strategyMd] = await Promise.all([
        fetchHelpMarkdown('user-guide.md'),
        fetchHelpMarkdown('bridge-strategy.md')
      ]);
      parseUserGuideIntoTopics(userGuideMd);
      helpTopicsHtml.strategy = renderBridgeStrategyMarkdown(strategyMd);
      helpGuideLoaded = true;
      selectHelpTopic(activeHelpTopic);
    } catch (err) {
      helpModalBody.innerHTML = `<p>Could not load guide (${escapeHelpHtml(err.message || err)}).</p>`;
    }
  }

  function closeHelpModal() {
    if (!helpModal) return;
    helpModal.classList.add('hidden');
  }

  if (openHelpBtn && openHelpBtn.tagName !== 'A') {
    openHelpBtn.addEventListener('click', () => openHelpModal());
  }
  if (helpTopicsNav) {
    helpTopicsNav.querySelectorAll('.help-topic-btn').forEach(btn => {
      btn.addEventListener('click', () => {
        if (btn.dataset.topic) {
          selectHelpTopic(btn.dataset.topic);
        }
      });
    });
  }
  if (closeHelpModalBtn) {
    closeHelpModalBtn.addEventListener('click', closeHelpModal);
  }
  if (helpModal) {
    helpModal.addEventListener('click', (e) => {
      if (e.target === helpModal) {
        closeHelpModal();
      }
    });
  }
  window.addEventListener('keydown', (e) => {
    if (e.key === 'Escape' && helpModal && !helpModal.classList.contains('hidden')) {
      closeHelpModal();
    }
  });
  const urlHelpParam = new URLSearchParams(window.location.search).get('help');
  if (window.location.hash === '#how-it-works') {
    openHelpModal('strategy');
  } else if (window.location.hash.startsWith('#help=')) {
    openHelpModal(window.location.hash.slice(6));
  } else if (window.location.hash === '#help' || urlHelpParam) {
    openHelpModal(urlHelpParam && helpTopicsHtml[urlHelpParam] !== undefined ? urlHelpParam : 'loading');
  }

  // Initial UI Render & Fetch Presets
  fetchPresets().then(() => {
    if (presetSelect && presetSelect.value) {
      loadPreset(presetSelect.value, true);
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
    "iters": 100,
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
    "iters": 100,
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
    "iters": 100,
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
    "iters": 100,
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
    "iters": 100,
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
    "iters": 100,
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
    "iters": 100,
    "north": "S AJ97; H A2; D A2; C A2;",
    "east": "",
    "south": "S KT86; H K3; D K3; C K3;",
    "west": "",
    "unknown_cards": "S Q5432 H 98765 D 98765 C 98765"
  },
  "hand_8_261014": {
    "name": "Hand 8 from 261014",
    "trump": "HEARTS",
    "leader": "WEST",
    "cur_trick": ["CJ"],
    "target": 9,
    "iters": 300,
    "north": "S Q; H KJT52; D J83; C KQ9;",
    "east": "",
    "south": "S KT9543; H A97; D K; C 75;",
    "west": "S -; H -; D -; C J;",
    "unknown_cards": "S AJ8762 H Q8643 D AQT976542 C T86"
  }
}

)rawjson";

inline const char* BRIDGE_STRATEGY_MD = R"rawmd(
# How the Solver Thinks About Bridge

This guide explains how the Bridge Line Finder works from a card-play
perspective. Standard computer bridge solvers tell you what happens when all 52
cards are face up. This solver answers the question declarer actually faces at
the table: **given only my hand, dummy, and the clues from the auction, what is
the single best plan to make my contract?**

---

## Why Double-Dummy Isn't Bridge

A traditional double-dummy solver (such as Deep Finesse or Bo Haglund's DDS)
solves one layout at a time with all four hands visible. That produces
superhuman plays that no real declarer can find:
- On Deal #1, where East holds a singleton Queen, double-dummy plays the Ace to
  drop it.
- On Deal #2, where East holds Queen-third, double-dummy leads to the Jack to
  finesse.
- If a defender holds a two-way guess, double-dummy always picks the right suit
  to attack.

At the table, you only see 26 cards—North and South—plus whatever you can infer
from the bidding and the cards played so far. You cannot play one way on Deal #1
and a different way on Deal #2 unless **something visible happens during the
play** (such as an honor appearing or a defender showing out of a suit) that
tells you which layout you are in.

Instead of solving each layout in isolation, this solver builds a **single
conditional plan**—a decision tree of tricks and visible branches—that makes
your contract across the largest possible share of hidden East/West layouts.

---

## Overview of a Solve

Every solve runs in six main stages:

```mermaid
flowchart LR
    A["1. Deal E/W Hands<br/>(Matching Bidding Clues)"] --> B["2. Find Contract-Making<br/>Plays on Each Deal"]
    B --> C["3. Group Plays into<br/>Bridge Decisions"]
    C --> D["4. Build One Playable<br/>Decision Tree"]
    D --> E["5. Adversarial<br/>Defense Pass"]
    E --> F["6. Summarize Winning<br/>& Losing Layouts"]
```

1. **Sample unseen hands**: Deal out hundreds of random East/West layouts that
   fit any bidding or play clues.
2. **Solve for the contract**: On each layout, find all plays that still make
   your trick target (including safety plays that give up overtricks).
3. **Generalize defender cards**: Group identical bridge situations together
   ("West follows low" vs. "West covers with an honor" vs. "West shows out").
4. **Build a legal decision tree**: Construct a declarer plan that only branches
   on cards seen at the table.
5. **Test against adversarial defense**: Re-score the top candidate plans
   against defenders who actively choose the toughest legal defense against your
   exact line.
6. **Explain the layouts**: Summarize in plain bridge terms which suit breaks
   and honor locations make or break the plan.

---

## 1. Simulating Possible East/West Layouts

The solver starts with the cards you can see (North and South, plus any cards
already played to earlier tricks) and deals the remaining unseen cards randomly
between East and West, typically 100 to 300 times.

Real bridge hands rarely treat every unseen layout as equally likely. If West
opened a weak two-bid or East overcalled 1NT, you know quite a bit about their
shapes and high cards. Before dealing, the solver lets you attach constraints to
East and West (implemented in [`hand_requirements.h`](../hand_requirements.h)):
- **High Card Points (HCP)**: e.g., East holds 12–14 HCP, or West holds at most
  7 HCP.
- **Suit lengths**: e.g., West has at least 5 spades, or East has 2–3 hearts.
- **Hand pattern**: e.g., a balanced hand, a void, or possession of a specific
  honor like the ♠K.

Every simulated deal is checked against those rules before the play search
begins, so the percentages reflect the auction rather than raw 52-card odds.

---

## 2. Aiming for the Contract, Not Overtricks

When a double-dummy engine analyzes a deal, its default mode asks: *"Which card
wins the maximum possible number of tricks?"* In rubber bridge or IMPs,
maximizing tricks on one layout can actually hurt your chance of making the
contract across all layouts.

Consider a classic safety play in trumps. Cashing the Ace first might hold you
to five trump tricks when the suit breaks 3-2, whereas finessing immediately
wins six trump tricks on a friendly 3-2 split—but goes down when an honor is
singleton offside. If the solver only kept plays that won the *maximum* tricks
on each deal, it would throw away the safety play on every 3-2 deal!

To prevent that, the solver asks DDS a different question on declarer's turn (in
[`interface_dds.cc`](../interface_dds.cc)): **"Which cards still make at least
the contract (`min_tricks`)?"**
- Any play that makes the contract stays alive as a candidate on that deal, even
  if a riskier peek-dependent play could win an overtrick.
- Once the contract is already locked up (`min_tricks <= 0`), the solver
  switches to maximizing the remaining tricks to keep the end-of-hand play
  clean.

---

## 3. How Defenders Play During the Initial Search (Step 1)

On any single deal, double-dummy analysis often reports that several defensive
cards tie. Suppose dummy leads a small card toward **A Q** in South, and East
(sitting in front of South) holds **K 2**. Double-dummy, East's **K** and **2**
tie: South wins both the **A** and the **Q** either way.

If East popped up with the **K** from **K 2** half the time, declarer would
never face a real finesse guess, and the decision tree would learn unnatural
habits based on defense no human plays at the table.

During the initial search (`ChooseDefenderPlay` in
[`interface_dds.cc`](../interface_dds.cc)), whenever multiple cards tie for best
double-dummy defense, defenders break ties using normal human carding rules:
- **Opening lead / 1st hand**: Defenders lead the bottom of touching honors
  (such as the Queen from **Q J 10**), avoid leading away from an unsupported
  honor into declarer's tenace (for instance, refusing to lead from **J x x**
  into **A Q**), and otherwise make a low lead.
- **2nd hand**: Defenders play **2nd hand low**, playing their lowest optimal
  spot card.
- **3rd hand**: Defenders play **3rd hand high** (the cheapest of touching high
  cards), unless partner led an honor (Jack or higher) that is currently winning
  the trick and they have a lower optimal card that avoids overtaking partner.
- **4th hand**: If declarer or dummy is currently winning the trick, 4th hand
  wins as cheaply as possible; if partner is already winning, 4th hand plays low
  unless forced to overtake.
- **Touching honors**: When a defender holds touching equals (like **K Q** or
  **Q J 10**), the solver randomizes uniformly among those touching honors using
  a deterministic seed per board. That stops declarer from inferring that
  "playing the King guarantees the defender does not hold the Queen."
- **Defensive noise**: A small configurable noise setting (default 10%)
  occasionally selects an alternative double-dummy-optimal play so declarer's
  line does not become brittle against slightly unusual but still sound defense.

---

## 4. Grouping Cards into Bridge Decisions

As the solver looks across hundreds of simulated deals, it cannot tell declarer
*"play the ♠2 on Deal #4 and the ♠Q on Deal #9."* Declarer's own plays must be
concrete cards chosen before seeing the next defender's card.

Defender cards, however, vary from deal to deal even in the exact same bridge
situation: on one deal West follows low with the **♠3**, on another with the
**♠5**, and on a third with the **♠8**. To a bridge player, all three mean the
same thing: **West followed with a small spade**.

The solver groups defender plays into bridge categories (`GeneralizeOppCard` and
`GeneralizedTrickFinder` in [`conditional_line.cc`](../conditional_line.cc)):
- **Specific honor** (`SK`, `SQ`, `HJ`): Used when covering, splitting, or
  dropping an honor changes declarer's play.
- **Small spot card** (`Sx`, `Hx`): Any spot card (`2` through `9`) in the suit.
- **Any honor** (`SX`): Any honor (`10` through `A`) in the suit.
- **Any card in the suit** (`S_`): Following suit when the rank does not matter.
- **Discard / show-out** (`R_`): Pitching another suit when void (which tells
  declarer the suit broke badly).
- **Any card at all** (`__`): When declarer's play is unconditional.

By combining declarer's exact card with these defender buckets, the solver turns
hundreds of raw deal records into clear instructions like:
> Lead **♠2** from South. If West plays a small spade (`Sx`), play **♠Q** from
> North; if West puts up the **♠K** (`SK`), win the **♠A** in North.

---

## 5. Building a Single Playable Decision Tree

The heart of the solver (`LineFinder::Solve` in
[`line_finder.cc`](../line_finder.cc)) searches for the decision tree
(`ConditionalLine`) that succeeds on the highest number of simulated layouts.

To keep the line 100% legal at the table, the tree is only allowed to branch on
**information visible to declarer**:
1. **A defender's card during a trick** (`TrickPlayCondition`): Declarer leads
   toward dummy, watches whether Left-Hand Opponent follows low, splits honors,
   or shows out, and then chooses dummy's play.
2. **Who won the trick** (`WinnerPlayCondition`): After a finesse or a hold-up
   play, one branch continues when North or South wins the trick, and another
   branch handles what to do when East or West wins and shifts.
3. **Whether declarer's top card in a suit is now high** (`IsCardGoodCondition`,
   `TryBranchOnGoodCards`): After playing one or two rounds of a suit, a bridge
   player checks whether an opposing honor dropped or the suit split evenly. For
   example, after cashing **♥A** and **♥K**, declarer can see whether dummy's
   **♥J** is now the highest heart left in the deck. If the **♥J** is good,
   declarer cashes it; if an opponent still holds the **♥Q**, declarer switches
   to a ruffing or side-suit plan.

---

## 6. The Adversarial Defense Pass (Step 2 Re-scoring)

When Step 1 searches for the tree that wins the most sample deals, it can
sometimes invent **jagged, overfitted lines of play** that exploit quirks in the
sample or in Step 1's standard carding rather than playing sound bridge.

### Why Step 1 Can Produce Jagged Lines

Suppose on the sample deals in Step 1, whenever East played the **10** on the
first round of a suit, East happened not to hold the **Queen** (for instance,
dropping the **10** from **10 x** or singleton **10**). A greedy tree search
might build a jagged rule:
- If East plays a small spot card, finesse.
- If East drops the **10**, refuse the finesse and play for the drop!

No real declarer should play that way. If declarer adopts a rule like *"don't
finesse when East drops the 10,"* a defender holding **Q 10 x** can simply
falsecard with the **10**—which costs nothing double-dummy—and bamboozle
declarer into rising with the Ace. Likewise, a line might look great in Step 1
only because a defender with two double-dummy-tied returns happened to pick the
passive return instead of the attacking shift that puts declarer to an early
guess.

The purpose of the **Adversarial Defense Pass** is to superhumanly punish flaws
and fragile branches in declarer's line of play, preventing jagged lines that a
human declarer shouldn't actually take.

### How Step 2 Adversarial Defense Works

Once Step 1 finishes, the solver takes the top candidate decision trees (up to
20 candidate plans) and runs a second, adversarial pass
(`RescoreTopLinesAdversarial` in [`line_finder.cc`](../line_finder.cc) and
`ChooseAdversarialDefenderCard` in
[`conditional_line.cc`](../conditional_line.cc)):

1. **List all double-dummy-optimal defensive cards**: At every defensive turn on
   a deal, DDS finds all cards that do not give away a trick double-dummy
   (`FindOptimalDefenderCards`). Defenders will never intentionally blow a trick
   just to trick declarer, but any cost-free falsecard or alternative switch is
   on the table.
2. **Check declarer's branch success rate for each card**: If two or more
   defensive cards tie double-dummy on that deal (such as playing the **10** vs.
   a small spot from **Q 10 x**, or shifting suits), the defender looks at
   declarer's decision tree and checks what fraction of deals declarer makes
   down each corresponding branch (`BranchSuccessProbability`).
3. **Include unmakeable deals in the branch odds**: Before scoring, the solver
   also traces deals that were unmakeable from the start or became unmakeable
   after an earlier play (`TraceUnmakeableBoard`). Including those losing
   layouts in each branch's denominator prevents a dangerous switch from looking
   like a "100% declarer branch" just because Step 1 had already dropped the
   layouts where that switch beat the contract.
4. **Steer declarer into the worst branch**: The defender plays the
   double-dummy-optimal card that **minimizes** declarer's success rate on that
   branch—dropping the **10** from **Q 10 x** if declarer's tree goes wrong over
   the **10**, or finding the killing shift.
5. **Re-rank declarer's plans**: Every candidate line is re-scored by how many
   deals it actually makes against this superhumanly tough defense, so clean,
   sound percentage lines beat jagged lines that rely on defenders cooperating.

---

## 7. Explaining When a Plan Wins or Fails

Knowing that a line works on 76% of simulated layouts is useful, but a bridge
player also wants to know *why* it works on those 76% and *what goes wrong* on
the other 24%.

After choosing the top line, the solver inspects the East/West hands on all
winning deals and all losing deals (`CrossHandDealSetSummarizer` in
[`cross_hand_summarizer.h`](../cross_hand_summarizer.h) and
[`unknown_cards_invariant.h`](../unknown_cards_invariant.h)) and extracts
human-readable rules:
- **Suit splits**: e.g., *"Spades break 3-2"* or *"Neither defender holds more
  than 4 clubs."*
- **Honor locations**: e.g., *"West holds the ♥K"* or *"East holds at least one
  diamond honor."*
- **Compound conditions**: e.g., *"Either hearts are 3-3, or the defender with
  4+ hearts also holds the ♠Q"*—the exact condition for a trump-and-spade
  squeeze or a drop-else-finesse line.

To keep the summary clean, the solver tests candidate rules against random
splits of the unseen cards (`FilterUninterestingInvariants`) and filters out
trivial statements that are true on almost every deal anyway. What remains is a
concise bridge explanation of the layouts your line beats and the layouts that
defeat it.

---

## Quick Reference: Bridge Concepts in the Code

| Bridge Concept | Where It Lives | Key Function / Class |
| :--- | :--- | :--- |
| Bidding clues (HCP, length, shape) | [`hand_requirements.h`](../hand_requirements.h) | `HandRequirements`, `HighCardRange`, `SuitLength` |
| Dealing unseen E/W hands | [`line_finder.h`](../line_finder.h), [`deal.cc`](../deal.cc) | `FuzzyBoardState::Randomize`, `DealMissingCards` |
| Solving for contract vs. overtricks | [`interface_dds.cc`](../interface_dds.cc) | `DDSSolveHelper` (`use_min_tricks_target`) |
| Human defensive leads & carding | [`interface_dds.cc`](../interface_dds.cc) | `ChooseDefenderPlay`, `RandomizeTouchingHonorRank` |
| Grouping spot cards, honors, show-outs | [`conditional_line.cc`](../conditional_line.cc) | `GeneralizeOppCard`, `GeneralizedTrickFinder` |
| Checking if a card is now high | [`conditional_line.cc`](../conditional_line.cc), [`line_finder.cc`](../line_finder.cc) | `IsCardGoodCondition`, `LineFinder::TryBranchOnGoodCards` |
| Adversarial defense re-scoring (Step 2) | [`line_finder.cc`](../line_finder.cc), [`conditional_line.cc`](../conditional_line.cc) | `RescoreTopLinesAdversarial`, `ChooseAdversarialDefenderCard` |
| Counting unmakeable layouts on branches | [`conditional_line.cc`](../conditional_line.cc) | `TraceUnmakeableBoard`, `BranchSuccessProbability` |
| Summarizing winning & losing layouts | [`cross_hand_summarizer.cc`](../cross_hand_summarizer.cc) | `CrossHandDealSetSummarizer::Summarize` |

)rawmd";

inline const char* USER_GUIDE_MD = R"rawmd(
# Bridge Deal Solver User Guide

Welcome to the **Bridge Deal Solver & Decision Tree Explorer**. This guide walks
through how to load deals from hand records or files, explore the interactive
decision tree, test your own lines of play, and compare competing strategies.

---

## 1. Loading Hands & Setting Up a Deal

You can start from a built-in classic problem, scan a tournament hand record
(PDF, photo, or PBN), import a text deal file, or type cards directly.

### Built-In Presets & Session History

Use the **Hand Selector** dropdown in the top header to load built-in deals
such as *Simple Finesse*, *4-Card Endplay*, *Squeeze Deal (NT)*, *9-Card
Hearts*, *Safety Play*, *Restricted Choice*, *Mollo 6S*, or *2-Way Finesse*.

Whenever you load a board from a PDF, photo, PBN, or text file during your
session, it is automatically added to the **Hand Selector** dropdown (for
example, `Hand 14 from club_game.pdf`) so you can switch back to it at any
time.

### The `Load` Button: PDFs, Photos, PBNs & Text Files

Click **Load** in the **Hand Text Inputs** header to import a deal from your
device:

![Scan Hand Record Modal](images/hand-record-scanner.png)

1. **PDF Hand Records (`.pdf`)**
   - Supports multi-page tournament hand record sheets (such as ACBL/Common
     Game PDFs).
   - If the PDF contains embedded PBN deal records, all boards are extracted
     instantly; the visual PDF page is also rendered so you can browse pages
     with **`◀ Page 1 / N ▶`** and click any detected board pill (**`#1`**,
     **`#2`**, ..., **`#16`**) or tap a board diagram directly on the page.
2. **Camera Photos & Screenshots (`.png`, `.jpg`, `.webp`, etc.)**
   - Runs **on-device optical character recognition (OCR)** directly in your
     browser.
   - Automatically detects all 4-hand compass diagrams in the image. Drag the
     crop handles around any board, use **`↻ Rotate 90°`** or **`🔍 Zoom
     Crop`**, and edit any scanned suit holding if needed before applying.
3. **Portable Bridge Notation Files (`.pbn`)**
   - Parses all `[Deal "..."]` records (along with `[Board "..."]`,
     `[Declarer "..."]`, and `[Contract "..."]` tags when present) and renders
     an interactive multi-board sheet in the scanner modal.
4. **Plain-Text Deal Files (`.txt`, `.deal`)**
   - Supports key-value files (`north = S AQ...`, `south = ...`,
     `unknown_cards = ...`, `trump = SPADES`, `leader = EAST`, `target = 10`,
     `iters = 100`, `declarer = S`, `cur_trick = ...`) as well as 4-line
     plain-text files (North, East, South, West on consecutive lines).

After loading a PDF, photo, or PBN, a **Crop Photo** button appears next to
**Load** so you can re-open the board sheet and pick another board or adjust
the settings without re-uploading the file.

### Scanner Modal: Declarer Rotation, Contract & Opening Lead

When you load a 4-hand board in the scanner modal, you can configure the table
context before clicking **Apply to Deal**:

- **Who is Declarer? (`N`, `E`, `S`, `W`)**: The solver always normalizes the
  table so **Declarer sits at North** and **Dummy sits at South**. Selecting
  who declared the hand at the table automatically rotates the four hands so
  Declarer becomes North and Dummy becomes South, pools the two defenders'
  cards into **Unknown Cards**, and saves the real East/West layout as
  **Actual Deal (`#0`)**. You can also rotate Declarer at any time from the
  main **`Declarer (→ N)`** bar.
- **Contract (`1`–`7`, `♣`/`♦`/`♥`/`♠`/`NT`)**: Sets the trump strain and
  automatically computes the required tricks (`6 + level`).
- **Opening Lead / Initial Play**: Type or tap cards played at the start of the
  deal (for example, the opening lead `DQ`, or the first trick `DQ D2 DA DK`
  followed by a shift `S7`). Completed tricks automatically adjust the target
  tricks and next leader, partial tricks pre-populate the current trick on the
  table, and any defender show-out automatically records a void constraint for
  that defender.

### Manual Deal Configuration & Constraints

- **Single-Dummy vs. Double-Dummy**:
  - For **single-dummy** analysis (realistic declarer play), put Declarer's
    hand in **N**, Dummy's hand in **S**, leave **E** and **W** empty (or only
    enter known cards), and list the remaining unseen cards in **Unknown Cards
    (Defender Pool)** under *Hand Requirements*. You can use specific ranks
    (`S KJT9`) or `x` for indistinguishable small spot cards (`S Kxxx`).
  - For **double-dummy** analysis (all 4 hands visible), fill in **N**, **E**,
    **S**, and **W** and leave **Unknown Cards** empty.
- **Center Table Controls**: Set **Trump** (`♠`, `♥`, `♦`, `♣`, `NT`),
  **Leader** (`N`, `E`, `S`, `W`), **Tricks** (target tricks for N/S), and
  **Rand deals** (number of random East/West layouts to sample).
- **Hand Requirements (Fuzzy Search Constraints)**: Expand *Hand Requirements*
  to restrict **West** or **East** by **HCP Range** (Min–Max) or minimum suit
  lengths when the auction gives you inferences about the defenders' hands.

---

## 2. Exploring the Decision Tree & Line Inspector

Clicking **⚡ Solve Deal** computes declarer's optimal conditional play tree and
displays it in the **Decision Tree** tab.

![Decision Tree & Line Inspector](images/decision-tree-inspector.png)

### Reading Nodes, Colors & Edge Conditions

Each card in the tree represents a trick or final outcome:

- **Blue Nodes (`Play`)**: A trick played along the line (for example,
  `Trick 1: SA, S4, S7, SJ`). Shows how many sampled deals reach and make along
  this branch. When some deals reaching a node go on to fail, the node
  background splits proportionally between blue and red and displays both
  `Makes:` and `Fails:` counts.
- **Green Leaf Nodes (`Makes`)**: The contract succeeds on all deals reaching
  this leaf (`N/S Won: X`).
- **Red Leaf Nodes (`Fails`)**: The defense defeats the line on the deals
  reaching this leaf.
- **Edge Condition Badges**: Arrows between tricks show *why* the play branches
  —such as `if E plays SQ`, `when W wins SK`, or `when E discards`.
- **Card Wildcard Notation**: When the specific spot card doesn't change the
  strategy, the tree groups cards using shorthand:
  - `Sx`: Any low spot card (`2`–`9`) in spades.
  - `SX`: Any honor (`T`, `J`, `Q`, `K`, `A`) in spades.
  - `S_`: Any spade of any rank.
  - `R_`: Any discard or ruff in another suit.

### What Happens When You Click Around the Tree

- **Clicking Any Node**:
  1. Expands the path from the root to the clicked node and continues down its
     primary continuation to a leaf, while collapsing other branches to keep
     the view clean.
  2. Smoothly centers the clicked node in the tree viewport.
  3. Opens the **Line Inspector** side panel on the right with the complete
     trick-by-trick sequence and highlights the clicked trick.
  4. Updates the **Deal Configuration** SVG diagram on the left to show the
     exact board state at that trick and opens the **Propose Trick** bar.
- **Panning & Zooming**: Drag the tree background to pan, scroll/pinch or use
  **`+`** / **`-`** / **`Reset View`** to zoom, and use **`Expand All`** or
  **`Collapse All`** to open or fold the entire tree. Drag the vertical divider
  between the left and right panels to resize them.

### Using the Line Inspector & Diagram Sync

The **Line Inspector** lists every trick along the currently inspected path in
fixed **N / E / S / W** columns:

- The **led card** has a highlighted border, and the **winning card** is badged
  in green (if won by N/S) or red/amber (if won by E/W).
- **Clicking a Trick Row (or `#K` badge)** in the Inspector selects that trick:
  - Earlier tricks' cards are shown **dimmed and struck through** on the left
    SVG deal diagram so you can see exactly which cards remain at Trick `#K`.
  - Clicking the selected trick row a second time clears the selection and
    restores the full starting hands.

### Tracing a Specific Hand (`Hand #`)

At the top of the Line Inspector, enter a deal number in **`Hand #`** (`1..N`,
or `0` when an Actual Deal from a hand record is loaded) and click **Trace**:

- The exact path taken by that deal through the decision tree is highlighted in
  **glowing cyan**.
- A **Traced Hand Banner** at the top of the Inspector displays West's and
  East's exact 4-suit holdings and whether the deal **MAKES** or **FAILS**.
- Every generalized wildcard in the trick list is replaced with the **actual
  cards** played on that specific deal, and the left SVG diagram displays East
  and West's concrete hands.
- Click **Clear** to exit trace mode and return to the aggregate tree view.

---

## 3. Trying & Comparing Alternative Lines

Bridge players constantly ask *"What if I cash the Ace first?"* or *"What if I
play clubs before drawing the last trump?"* The solver lets you test any line of
play against the exact same random deals and compare them head-to-head.

### Cycling Through Equivalent Optimal Lines

When two or more different lines of play tie for the highest success rate, the
**Multiple Equivalent Lines** bar appears above the tree. Use the **`◀`** /
**`▶`** buttons or the dropdown menu to switch between them.

### Proposing an Alternative Line (`Propose Trick #K`)

You can branch off and force your own play at any trick:

1. **Select a Trick**: Click a node in the Decision Tree or click any trick row
   `#K` in the **Line Inspector**.
2. **Pick Cards on the Deal Diagram**:
   - The **Propose Trick #K** bar appears in the left panel showing whose turn
     it is to play (`N to play (0/4)`, `E to play (1/4)`, etc.).
   - Legal cards for the active player glow **bright yellow** on the SVG deal
     diagram (for defenders in a single-dummy solve, click cards directly in the
     **UNKNOWN CARDS** box).
   - You can click **1 card** (for example, forcing declarer's lead to the trick
     and letting the solver handle the defenders' replies) or up to **all 4
     cards** of the trick.
3. **Compute the Continuation**: Click **Compute new line** (or use **Undo** to
   step back one card, or **Reset** to return to the original solve).
   - The solver replays Tricks `1..K-1`, forces your proposed card(s) at Trick
     `#K`, and solves the remainder of the deal across the **exact same sampled
     deals** (`reuse_deals`).
   - If a defender shows out on a proposed trick, the solver automatically
     constrains that defender to be void in the led suit for the continuation.
   - Your new line is added to the line selector dropdown as
     **`Alt line 1: Trick #K ...`** with its overall success percentage, so you
     can switch freely between the **Original line** and every **Alt line** you
     have explored.

### Comparing Lines Head-to-Head (`Compare Lines` Tab)

Once you have multiple lines (either multiple optimal lines from the initial
solve or one or more proposed **Alt lines**), click the **Compare Lines** tab:

1. **Select Lines**: Check the boxes next to **2 or more lines** in the
   *Computed Lines* list.
2. **Stacked Bar Breakdown**: For each pair of selected lines (**Line A vs. Line
   B**), a color-coded bar chart breaks all sampled deals into four buckets:
   - **Both Make (Green)**: Deals where either line succeeds.
   - **Only Line A Makes (Blue)**: Deals where Line A succeeds and Line B fails.
   - **Only Line B Makes (Amber)**: Deals where Line B succeeds and Line A
     fails.
   - **Both Fail (Slate)**: Unmakeable layouts where neither line succeeds.
3. **Inspect Disagreement Deals**: Below the chart, side-by-side columns list
   the specific sample deals where **Only Line A Makes** vs. **Only Line B
   Makes**. Click any deal card to immediately switch to that line and trace
   that exact hand in the **Decision Tree**!

![Compare Lines Tab](images/propose-and-compare-lines.png)

---

## 4. Distribution Summaries, Sample Hands & Actual Deal (`#0`)

Above the main tabs, the **Result Summary Cards** show **Tricks Won**, **Optimal
Lead** (or **Card Led**), **Success Rate**, **Failure Rate**, and—when loaded
from a 4-hand record—**Actual (`#0`)**.

### Filtering by Success, Failure, or Actual Deal (`#0`)

The **Success Rate**, **Failure Rate**, and **Actual (`#0`)** stat cards are
interactive toggle buttons:

- Click **Success Rate** to inspect the layouts where declarer's line **makes**
  the contract.
- Click **Failure Rate** to inspect the layouts where the defense **beats** the
  contract.
- Click **Actual (`#0`)** (when present) to isolate and trace the real table
  layout from your loaded hand record.

### The `Distribution Summary` Tab

The **Distribution Summary** tab translates the raw set of making (or failing)
deals into bridge table takeaways:

- **Invariant Rules**: Automatically discovered rules that hold true across the
  selected deals—such as *"East holds the King of Spades"*, *"Hearts split
  3-2"*, or *"West has at most 1 diamond"*.
- **Suit Length Bounds**: Minimum and maximum suit lengths for East, West, and
  their combined holding.
- **Honor Card Possession Stats**: The exact percentage of deals in which East,
  West, both, or neither holds each key honor (`A`, `K`, `Q`, `J`, `T`).

![Distribution Summary Tab](images/distribution-summary.png)

### The `Sample Hands` Tab

The **Sample Hands** tab displays miniature 4-hand compass diagrams for the
sampled deals (`#1..#N`, plus `#0 Actual` when available) matching your active
filter (**Covered Deals**, **Non-Covered Deals**, or **Actual Deal `#0`**):

- Each card shows the board number (`#1`, `#2`, ...) and full 4-hand layout.
- **Click any sample deal card** to jump straight to the **Decision Tree** tab
  with that hand traced from Trick 1 to the end of play.

)rawmd";

} // namespace web_assets
