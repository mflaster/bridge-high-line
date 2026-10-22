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
let lastSolveRequestInfo = null;
let lastSentryErrorEventId = null;
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
    loadedFirstRoundCalls: Array.isArray(loadedFirstRoundCalls)
      ? loadedFirstRoundCalls.map(c => Object.assign({}, c))
      : [],
    importedBiddingReqs: {
      E: (importedBiddingReqs && Array.isArray(importedBiddingReqs.E))
        ? importedBiddingReqs.E.map(r => Object.assign({}, r))
        : [],
      W: (importedBiddingReqs && Array.isArray(importedBiddingReqs.W))
        ? importedBiddingReqs.W.map(r => Object.assign({}, r))
        : []
    },
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
  loadedFirstRoundCalls = Array.isArray(s.loadedFirstRoundCalls)
    ? s.loadedFirstRoundCalls.map(c => Object.assign({}, c))
    : [];
  importedBiddingReqs = {
    E: (s.importedBiddingReqs && Array.isArray(s.importedBiddingReqs.E))
      ? s.importedBiddingReqs.E.map(r => Object.assign({}, r))
      : [],
    W: (s.importedBiddingReqs && Array.isArray(s.importedBiddingReqs.W))
      ? s.importedBiddingReqs.W.map(r => Object.assign({}, r))
      : []
  };
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
  if (typeof updateSaveAnalysisButtonState === 'function') {
    updateSaveAnalysisButtonState();
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
  },
  hand_6_261014: {
    name: "Hand 6 from 261014",
    trump: 'SPADES',
    leader: 'EAST',
    cur_trick: ['HA'],
    target: 11,
    iters: 200,
    north: 'S AJ8765; H J85; D -; C KJ86;',
    east: 'S -; H AKQ; D -; C -;',
    south: 'S KT9; H 93; D AQ853; C AQ9;',
    west: 'S -; H 642; D -; C -;',
    unknown_cards: 'S Q432 H T7 D KJT97642 C T75432',
    requirements: {
      E: {
        would_bid: [{ bid: 'X', after: 'P 1S' }]
      },
      W: {
        would_bid: [{ bid: 'P', after: '' }]
      }
    },
    actual_deal: {
      N: 'S AJ8765; H J85; D -; C KJ86;',
      E: 'S Q3; H AKQ; D KJ96; C 7543;',
      S: 'S KT9; H 93; D AQ853; C AQ9;',
      W: 'S 42; H T7642; D T742; C T2;'
    }
  },
  '7s_1021': {
    name: '7S 1021',
    trump: 'SPADES',
    leader: 'EAST',
    cur_trick: ['S9'],
    target: 13,
    iters: 100,
    north: 'S AK87; H AK6; D 8; C AK642;',
    east: 'S 9; H -; D -; C -;',
    south: 'S QJ64; H J95; D AKQT; C J7;',
    west: '',
    unknown_cards: 'S T532 H QT87432 D J9765432 C QT9853',
    actual_deal: {
      N: 'S AK87; H AK6; D 8; C AK642;',
      E: 'S T9; H T84; D J765; C QT93;',
      S: 'S QJ64; H J95; D AKQT; C J7;',
      W: 'S 532; H Q732; D 9432; C 85;'
    }
  }
};


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
  if (typeof updateSaveAnalysisButtonState === 'function') {
    updateSaveAnalysisButtonState();
  }

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
let loadedFirstRoundCalls = []; // Stores [{ seat, bid, after }, ...] in original compass seats
let importedBiddingReqs = { E: [], W: [] }; // Rotated defender would_bid requirements: [{ bid, after }, ...]
let importedSuitVoids = { E: new Set(), W: new Set() }; // Voids revealed during imported initial play
let currentDeclarerSeat = 'N';
let modalDealerSeat = 'N';
let modalContractLevel = 4;
let modalContractStrain = 'SPADES';

function computeDefenderBiddingReqsFromFirstRound(callsList, lhoSeat, rhoSeat) {
  const out = { E: [], W: [] };
  if (!Array.isArray(callsList)) return out;
  callsList.forEach(c => {
    if (!c || !c.seat || !c.bid) return;
    const entry = { bid: c.bid, after: c.after || '' };
    if (c.seat === lhoSeat) {
      out.E.push(entry);
    } else if (c.seat === rhoSeat) {
      out.W.push(entry);
    }
  });
  return out;
}

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

function getRotationSeatMap(seatOverride = null) {
  const seatNames = { N: 'North', E: 'East', S: 'South', W: 'West' };
  const order = ['N', 'E', 'S', 'W'];
  const rawSeat = seatOverride ||
    (currentSolveData && currentSolveData.declarer_seat) ||
    currentDeclarerSeat ||
    'N';
  const idx = Math.max(0, order.indexOf(rawSeat));
  const declSeat = order[idx];
  const lhoSeat = order[(idx + 1) % 4];
  const dummySeat = order[(idx + 2) % 4];
  const rhoSeat = order[(idx + 3) % 4];
  return {
    declSeat,
    lhoSeat,
    dummySeat,
    rhoSeat,
    declName: seatNames[declSeat],
    lhoName: seatNames[lhoSeat],
    dummyName: seatNames[dummySeat],
    rhoName: seatNames[rhoSeat],
    solverToTable: {
      N: declSeat,
      E: lhoSeat,
      S: dummySeat,
      W: rhoSeat,
      'hand 1': lhoSeat,
      'hand 2': rhoSeat,
      'hand 3': rhoSeat
    },
    tableToSolver: {
      [declSeat]: 'N',
      [lhoSeat]: 'E',
      [dummySeat]: 'S',
      [rhoSeat]: 'W'
    }
  };
}

function getTableCompassDealHands(deal, seatOverride = null) {
  if (!deal) return { N: '', E: '', S: '', W: '' };
  const map = getRotationSeatMap(seatOverride);
  return {
    [map.declSeat]: deal.N || '',
    [map.lhoSeat]: deal.E || '',
    [map.dummySeat]: deal.S || '',
    [map.rhoSeat]: deal.W || ''
  };
}

function formatInvariantRuleForRotation(rule, seatOverride = null) {
  if (!rule || typeof rule !== 'string') return '';
  const rotMap = getRotationSeatMap(seatOverride);
  return rule.replace(
    /\b(hand 1|hand [23])\b|\b(E|W)(?=\s+(?:has the|has|length in)\b)/gi,
    (match, handToken, compassToken) => {
      if (handToken) {
        return /^hand 1$/i.test(handToken)
          ? `Hand 1 (${rotMap.lhoName})`
          : `Hand 2 (${rotMap.rhoName})`;
      }
      if (compassToken === 'E') return rotMap.lhoSeat;
      if (compassToken === 'W') return rotMap.rhoSeat;
      return match;
    }
  );
}

function updateDeclarerBarUI(seat) {
  const rotMap = getRotationSeatMap(seat);
  const { declSeat, lhoSeat, dummySeat, rhoSeat } = rotMap;
  currentDeclarerSeat = declSeat;
  if (currentSolveData) {
    currentSolveData.declarer_seat = declSeat;
  }

  document.querySelectorAll('.declarer-seat-btn').forEach(btn => {
    btn.classList.toggle('active', btn.dataset.seat === declSeat);
  });
  document.querySelectorAll('.modal-declarer-btn').forEach(btn => {
    btn.classList.toggle('active', btn.dataset.seat === declSeat);
  });

  const declarerMappingHint = document.getElementById('declarerMappingHint');
  if (declarerMappingHint) {
    declarerMappingHint.textContent = (declSeat === 'N')
      ? 'N=Decl · E=LHO · S=Dummy · W=RHO'
      : `N=${declSeat}(Decl) · E=${lhoSeat}(LHO) · S=${dummySeat}(Dummy) · W=${rhoSeat}(RHO)`;
  }

  const modalDeclarerHint = document.getElementById('modalDeclarerHint');
  if (modalDeclarerHint) {
    modalDeclarerHint.textContent =
      `N=${declSeat}(Decl) · E=${lhoSeat}(LHO) · S=${dummySeat}(Dummy) · W=${rhoSeat}(RHO)`;
  }

  const roleMap = {
    [declSeat]: { text: 'Declarer (→ N)', cls: 'scan-role-tag role-declarer' },
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

  if (typeof updateModalBiddingUI === 'function') {
    updateModalBiddingUI();
  }
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
    } else if ((seat === 'N' || seat === 'S') &&
               countHandCards(nVal) > 0 && countHandCards(sVal) > 0 &&
               countHandCards(eVal) === 0 && countHandCards(wVal) === 0) {
      const prevSeat = currentDeclarerSeat || 'N';
      if ((prevSeat === 'N' || prevSeat === 'S') && seat !== prevSeat) {
        // Flip N and S on single-dummy preset when toggling between N and S
        inputNorth.value = sVal;
        inputSouth.value = nVal;
        updateDeclarerBarUI(seat);
        clearPanels();
        updateDealSvg();
      } else {
        updateDeclarerBarUI(seat);
      }
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

  if (fromModal && typeof parseModalFirstRoundCalls === 'function') {
    const parsedBids = parseModalFirstRoundCalls();
    loadedFirstRoundCalls = (!parsedBids.error && Array.isArray(parsedBids.calls))
      ? parsedBids.calls
      : [];
  }
  importedBiddingReqs = computeDefenderBiddingReqsFromFirstRound(
    loadedFirstRoundCalls,
    lhoSeat,
    rhoSeat
  );

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
    const bidDesc = (importedBiddingReqs.E.length > 0 || importedBiddingReqs.W.length > 0)
      ? ` [Bidding: ${loadedFirstRoundCalls.map(c => `${c.seat}:${c.bid}`).join(' ')}]`
      : '';
    setStatus('idle', `Loaded ${contractDesc}${playDesc}${bidDesc}. Declarer rotated to North, Dummy (${dummySeat}) to South.`);
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
  loadedActualDeal = p.actual_deal ? Object.assign({}, p.actual_deal) : null;
  loadedFirstRoundCalls = [];
  importedBiddingReqs = { E: [], W: [] };
  if (p.requirements && typeof p.requirements === 'object') {
    ['E', 'W'].forEach(seatKey => {
      const sr = p.requirements[seatKey];
      if (sr && Array.isArray(sr.would_bid)) {
        importedBiddingReqs[seatKey] = sr.would_bid.map(r => ({
          bid: r.bid || '',
          after: r.after || ''
        }));
      }
    });
  }
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
  loadedFirstRoundCalls = [];
  importedBiddingReqs = { E: [], W: [] };
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

  // Include hand requirements if entered or if voids/bidding were revealed
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
    const bidReqs = importedBiddingReqs && importedBiddingReqs[seatKey];
    if (Array.isArray(bidReqs) && bidReqs.length > 0) {
      if (!reqs[seatKey]) reqs[seatKey] = {};
      reqs[seatKey].would_bid = bidReqs.map(r => ({
        bid: r.bid || '',
        after: r.after || ''
      }));
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
    const solveHandInfo = getActivePresetAndHandInfo();
    lastSolveRequestInfo = {
      solve_kind: 'main',
      preset: solveHandInfo.preset,
      preset_name: solveHandInfo.preset_name,
      hand_label: solveHandInfo.hand_label,
      payload: JSON.parse(JSON.stringify(payload))
    };
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
      const errMsg = data.error || `HTTP ${response.status || 'error'}`;
      setStatus('error', 'Solver failed: ' + errMsg, `${elapsed} ms`);
      reportRuntimeErrorToSentry(new Error('Solver failed: ' + errMsg), {
        source: 'solve_error',
        solve_kind: 'main',
        http_status: response.status || 400,
        solve_payload: lastSolveRequestInfo ? lastSolveRequestInfo.payload : payload
      });
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
    data.declarer_seat = currentDeclarerSeat || 'N';
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
        declarerSeat: currentDeclarerSeat || 'N',
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
      reportRuntimeErrorToSentry(err, {
        source: 'solve_network_or_server_error',
        solve_kind: 'main',
        solve_payload: lastSolveRequestInfo ? lastSolveRequestInfo.payload : payload
      });
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

function splicePrefixTricksOntoTree(
  prefixTricks,
  continuationTree,
  totalBoards,
  successBoards,
  failureBoards,
  activeBoardIds
) {
  if (!prefixTricks || prefixTricks.length === 0) {
    return continuationTree;
  }

  let root = null;
  let prevNode = null;
  const activeIds =
    Array.isArray(activeBoardIds) && activeBoardIds.length > 0
      ? activeBoardIds.filter(b => typeof b === 'number' && b >= 0)
      : Array.from({ length: totalBoards }, (_, i) => i);
  const allBoards = [-1, ...activeIds];
  const boardIdStr = activeIds.length > 0 ? String(activeIds[0]) : null;

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
    if (!pt.isProposedTrick && pt.boardToTrick &&
        typeof pt.boardToTrick === 'object') {
      activeIds.forEach(bid => {
        const k = String(bid);
        if (pt.boardToTrick[k]) {
          node.board_to_trick[k] = pt.boardToTrick[k];
        }
      });
    }
    if (boardIdStr !== null && !node.board_to_trick[boardIdStr]) {
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
      const maxHand = Math.max(
        total,
        Array.isArray(data.all_hands) ? data.all_hands.length : 0
      );
      traceHandInput.placeholder = data.actual_hand ? `0..${maxHand}` : `1..${maxHand}`;
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
        winnerSeatIdx: winnerSeatIdx,
        isProposedTrick: true
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
    const replayBoardTricksPayload = prefixTricksToSplice.map(pt =>
      (!pt.isProposedTrick && pt.boardToTrick &&
       typeof pt.boardToTrick === 'object')
        ? pt.boardToTrick
        : {}
    );
    const hasAnyBoardTrickMap = replayBoardTricksPayload.some(
      m => Object.keys(m).length > 0
    );

    const parsedNumIters = isNaN(parseInt(numIters.value, 10))
      ? 100
      : parseInt(numIters.value, 10);
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
      num_iters: parsedNumIters,
      num_threads: isNaN(parsedThreads) ? 16 : parsedThreads,
      cur_trick: curTrickForSolve,
      start_trick_num: startTrickNumForSolve,
      initial_ns_tricks: nsTricksBeforeSolve,
      reuse_deals: true,
      replay_tricks: replayTricksPayload
    };
    if (hasAnyBoardTrickMap) {
      payload.replay_board_tricks = replayBoardTricksPayload;
    }

    const defIdx = (ctx.leaderIdx % 2 !== 0) ? 0 : 1;
    let keptDefenderBranchInTrick = false;
    if (Array.isArray(ctx.branchTrickCards) &&
        proposedCards.length > defIdx &&
        ctx.branchTrickCards.length > defIdx) {
      keptDefenderBranchInTrick = true;
      for (let j = 0; j <= defIdx; j++) {
        const pCard = proposedCards[j];
        const bCard = ctx.branchTrickCards[j];
        if (!pCard || !bCard) {
          keptDefenderBranchInTrick = false;
          break;
        }
        const pRank = (pCard.clickedToken || pCard.rankStr || '');
        const bRank = (bCard.rankStr || '');
        const normP = (pRank === '10' ? 'T' : pRank).toUpperCase();
        const normB = (bRank === '10' ? 'T' : bRank).toUpperCase();
        if (pCard.suit !== bCard.suit || normP !== normB) {
          keptDefenderBranchInTrick = false;
          break;
        }
      }
    }
    const candidateBranchIds =
      (keptDefenderBranchInTrick &&
       Array.isArray(ctx.branchInTrickIds) &&
       ctx.branchInTrickIds.length > 0)
        ? ctx.branchInTrickIds
        : ctx.branchBeforeTrickIds;
    const baseTotalDeals =
      (originalSolveSnapshot && originalSolveSnapshot.data &&
       originalSolveSnapshot.data.total_boards)
        ? originalSolveSnapshot.data.total_boards
        : parsedNumIters;
    if (Array.isArray(candidateBranchIds) &&
        candidateBranchIds.length > 0 &&
        candidateBranchIds.length < baseTotalDeals) {
      payload.branch_board_ids = candidateBranchIds;
    }

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
      const bidReqs = importedBiddingReqs && importedBiddingReqs[seatKey];
      if (Array.isArray(bidReqs) && bidReqs.length > 0) {
        if (!reqs[seatKey]) reqs[seatKey] = {};
        reqs[seatKey].would_bid = bidReqs.map(r => ({
          bid: r.bid || '',
          after: r.after || ''
        }));
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
      const solveHandInfo = getActivePresetAndHandInfo();
      lastSolveRequestInfo = {
        solve_kind: 'proposed_line',
        preset: solveHandInfo.preset,
        preset_name: solveHandInfo.preset_name,
        hand_label: solveHandInfo.hand_label,
        trick_num: ctx.trickNum,
        proposed_cards: proposedCards.map(
          c => `${c.seatName}:${c.suit || ''}${c.rankStr || c.rank || ''}`
        ),
        payload: JSON.parse(JSON.stringify(payload))
      };
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
        const errMsg = data.error || `HTTP ${response.status || 'error'}`;
        setStatus('error', 'Solver failed: ' + errMsg, `${elapsed} ms`);
        reportRuntimeErrorToSentry(new Error('Solver failed: ' + errMsg), {
          source: 'solve_error',
          solve_kind: 'proposed_line',
          http_status: response.status || 400,
          solve_payload: lastSolveRequestInfo ? lastSolveRequestInfo.payload : payload
        });
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
          failure,
          data.active_board_ids
        );
        if (Array.isArray(data.lines)) {
          data.lines.forEach(lineObj => {
            if (lineObj && lineObj.tree) {
              lineObj.tree = splicePrefixTricksOntoTree(
                prefixTricksToSplice.map(r => Object.assign({}, r)),
                lineObj.tree,
                total,
                success,
                failure,
                data.active_board_ids
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
      data.declarer_seat =
        (originalSolveSnapshot && originalSolveSnapshot.inputs && originalSolveSnapshot.inputs.declarerSeat) ||
        (currentSolveData && currentSolveData.declarer_seat) ||
        currentDeclarerSeat ||
        'N';

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
        const maxHand = Math.max(
          total,
          Array.isArray(data.all_hands) ? data.all_hands.length : 0
        );
        traceHandInput.placeholder = data.actual_hand ? `0..${maxHand}` : `1..${maxHand}`;
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
        reportRuntimeErrorToSentry(err, {
          source: 'solve_network_or_server_error',
          solve_kind: 'proposed_line',
          solve_payload: lastSolveRequestInfo ? lastSolveRequestInfo.payload : payload
        });
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


document.addEventListener('DOMContentLoaded', () => {
  if (typeof window !== 'undefined') {
    window.setStatus = setStatus;
    window.openFeedbackModal = openFeedbackModal;
    window.submitSentryFeedback = submitSentryFeedback;
    window.initWasmWorker = initWasmWorker;
  }

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
