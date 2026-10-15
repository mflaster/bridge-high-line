document.addEventListener('DOMContentLoaded', () => {
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
  const statusProgress = document.getElementById('statusProgress');
  const statusProgressFill = document.getElementById('statusProgressFill');
  const execTime = document.getElementById('execTime');
  const resultsSummary = document.getElementById('resultsSummary');
  const statTricks = document.getElementById('statTricks');
  const statLeadLabel = document.getElementById('statLeadLabel');
  const statLead = document.getElementById('statLead');
  const cardStatSuccess = document.getElementById('cardStatSuccess');
  const cardStatFailure = document.getElementById('cardStatFailure');
  const cardStatActual = document.getElementById('cardStatActual');
  const statSuccess = document.getElementById('statSuccess');
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
            req.resolve({
              ok: parsed.success !== false,
              json: async () => parsed
            });
          } catch (err) {
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
        if (wasmActiveRequest) {
          const req = wasmActiveRequest;
          wasmActiveRequest = null;
          req.resolve({
            ok: false,
            json: async () => ({
              success: false,
              error: msg.error || 'WebAssembly worker error'
            })
          });
        }
      }
    };
    wasmWorker.onerror = (err) => {
      if (wasmActiveRequest) {
        const req = wasmActiveRequest;
        wasmActiveRequest = null;
        req.resolve({
          ok: false,
          json: async () => ({
            success: false,
            error: 'Web Worker error: ' + (err && err.message ? err.message : 'failed to load solver_worker.js')
          })
        });
      }
    };
    return wasmWorker;
  }

  function configureWasmThreadsUI() {
    const numThreadsSelect = document.getElementById('numThreads');
    if (!numThreadsSelect) return;
    numThreadsSelect.value = '1';
    numThreadsSelect.disabled = true;
    numThreadsSelect.title = 'Running client-side in WebAssembly Web Worker (1 worker thread)';
  }

  async function ensureBackendMode() {
    if (backendMode) return backendMode;
    if (backendDetectPromise) return backendDetectPromise;
    backendDetectPromise = (async () => {
      if (window.BRIDGE_FORCE_WASM === true) {
        backendMode = 'wasm';
        configureWasmThreadsUI();
        initWasmWorker();
        return backendMode;
      }
      try {
        const res = await fetch('/api/health', { cache: 'no-store' });
        if (res.ok) {
          const data = await res.json();
          if (data && data.status === 'ok') {
            backendMode = 'server';
            return backendMode;
          }
        }
      } catch (_) {}
      backendMode = 'wasm';
      configureWasmThreadsUI();
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
        payloadJson: JSON.stringify(Object.assign({}, payload, { num_threads: 1 }))
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
          return;
        }
      }
    } catch (e) {
      // Fall back to default embedded PRESETS
    }
    ensureBackendMode();
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
  let currentDeclarerSeat = 'N';

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
    if (countHandCards(inputEast ? inputEast.value : '') > 0 ||
        countHandCards(inputWest ? inputWest.value : '') > 0) {
      return null;
    }
    const curUnk = inputUnknownCards ? inputUnknownCards.value.trim() : '';
    if (!curUnk) return null;
    const expectedUnk = combineHandsToUnknownString(loadedActualDeal.E, loadedActualDeal.W);
    const normCurUnk = combineHandsToUnknownString(curUnk, '');
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
  }

  function applyDeclarerRotation(seat) {
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

    const declHand = loadedBoardHands[order[idx]] || '';
    const lhoHand = loadedBoardHands[order[(idx + 1) % 4]] || '';
    const dummyHand = loadedBoardHands[order[(idx + 2) % 4]] || '';
    const rhoHand = loadedBoardHands[order[(idx + 3) % 4]] || '';

    loadedActualDeal = {
      N: declHand,
      E: lhoHand,
      S: dummyHand,
      W: rhoHand
    };

    presetCurTrick = [];
    clearPanels();
    inputNorth.value = declHand;
    inputSouth.value = dummyHand;
    inputEast.value = '';
    inputWest.value = '';
    if (inputUnknownCards) {
      inputUnknownCards.value = combineHandsToUnknownString(lhoHand, rhoHand);
    }

    updateDeclarerBarUI(seat);
    setStatus('idle', `Loaded board with ${order[idx]} as Declarer (rotated to North) and ${order[(idx + 2) % 4]} as Dummy (South).`);
    updateDealSvg();
  }

  document.querySelectorAll('.declarer-seat-btn').forEach(btn => {
    btn.addEventListener('click', () => {
      applyDeclarerRotation(btn.dataset.seat);
    });
  });

  function loadPreset(key) {
    const p = PRESETS[key];
    if (!p) return;

    loadedBoardHands = null;
    loadedActualDeal = null;
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
  }

  // Load Preset on Dropdown Selection
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

      const isPdf = (file.type === 'application/pdf') ||
                    /\.pdf$/i.test(file.name || '');
      if (isPdf) {
        openPhotoScanModalWithPdfFile(file);
        return;
      }

      const isImage = (file.type && file.type.startsWith('image/')) ||
                      /\.(png|jpe?g|webp|gif|bmp)$/i.test(file.name || '');
      if (isImage) {
        openPhotoScanModalWithFile(file);
        return;
      }

      const isPbnExt = /\.pbn$/i.test(file.name || '');
      const reader = new FileReader();
      reader.onload = (event) => {
        const text = event.target.result;
        if (isPbnExt || (text && text.includes('[Deal "'))) {
          const pbnBoards = parsePbnBoardsFromText(text);
          if (pbnBoards.length > 0) {
            openPhotoScanModalWithPbnBoards(pbnBoards);
            return;
          }
        }
        parseAndApplyDealText(text);
      };
      reader.readAsText(file);
    });
  }

  if (recropPhotoBtn) {
    recropPhotoBtn.addEventListener('click', () => {
      openPhotoScanModalExisting();
    });
  }

  function parseAndApplyDealText(text) {
    if (!text) return;
    loadedBoardHands = null;
    loadedActualDeal = null;
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
        return;
      }
    }
    updateDeclarerBarUI('N');
    updateDealSvg();
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

  let scanSourceCanvas = null; // Offscreen canvas holding current (possibly rotated) image
  let detectedBoardBoxes = []; // Array of { x, y, w, h, boardLabel?, pbnHands? } in scanSourceCanvas coords
  let activeDetectedBoardIdx = -1;
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
    for (const line of lines) {
      const bm = line.match(/^\[Board\s+"([^"]+)"\]/i);
      if (bm) curBoard = bm[1].trim();
      const dm = line.match(/^\[Deal\s+"([^"]+)"\]/i);
      if (dm) {
        const hands = parsePbnDealString(dm[1]);
        if (hands) {
          boards.push({
            board: curBoard || String(boards.length + 1),
            hands
          });
        }
        curBoard = null;
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
        pbnHands: b.hands
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

    if (photoScanModal) photoScanModal.classList.remove('hidden');
    if (recropPhotoBtn) recropPhotoBtn.classList.remove('hidden');
    updateDeclarerBarUI(currentDeclarerSeat);
    if (scanStatusBadge) {
      scanStatusBadge.className = 'scan-status-badge scan-status-pending';
      scanStatusBadge.textContent = 'Loading PDF hand record...';
    }

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
          if (scanStatusBadge) {
            scanStatusBadge.className = 'scan-status-badge scan-status-warn';
            scanStatusBadge.textContent = '⚠ Could not render PDF offline and no embedded PBN deals were found.';
          }
        }
      }
    } catch (err) {
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
    photoScanModal.classList.remove('hidden');
    updateDeclarerBarUI(currentDeclarerSeat);
    updatePdfPageBarUI();

    requestAnimationFrame(() => {
      if (resetDetection) {
        runFullPhotoBoardDetection();
      } else {
        fitCropPhotoView();
        renderCropCanvas();
      }
    });
  }

  function closePhotoScanModal() {
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
        pbnHands: pbnEntry ? pbnEntry.hands : null
      };
    });

    if (detectedBoardBoxes.length > 0) {
      activeDetectedBoardIdx = 0;
      cropBox = { ...detectedBoardBoxes[0] };
    } else {
      activeDetectedBoardIdx = -1;
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
        activeDetectedBoardIdx = idx;
        cropBox = { ...detectedBoardBoxes[idx] };
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

    const res = recognizeBoardFromImageData(imgData);
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
      inp.addEventListener('input', updateScanValidationBadge);
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
          activeDetectedBoardIdx = hitIdx;
          cropBox = { ...detectedBoardBoxes[hitIdx] };
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

  if (closePhotoModalBtn) closePhotoModalBtn.addEventListener('click', closePhotoScanModal);
  if (cancelPhotoModalBtn) cancelPhotoModalBtn.addEventListener('click', closePhotoScanModal);

  if (applyPhotoModalBtn) {
    applyPhotoModalBtn.addEventListener('click', () => {
      loadedBoardHands = {
        N: (scanInputN ? scanInputN.value : '').trim(),
        E: (scanInputE ? scanInputE.value : '').trim(),
        S: (scanInputS ? scanInputS.value : '').trim(),
        W: (scanInputW ? scanInputW.value : '').trim()
      };
      closePhotoScanModal();
      applyDeclarerRotation(currentDeclarerSeat);
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
        return;
      }

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
      } else {
        console.error(err);
        setStatus('error', 'Network or server error: ' + err.message);
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

      solveStoppedByUser = false;
      activeSolveAbortController = new AbortController();
      setStatus('running', `Computing new line from Trick #${ctx.trickNum}...`);
      computeNewLineBtn.disabled = true;
      solveBtn.disabled = true;
      const startTime = performance.now();

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
          return;
        }

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

        renderSolveTree();
        updateViewMode(activeViewMode);
      } catch (err) {
        isSolving = false;
        clearInterval(statusPollTimer);
        const elapsedSec = ((performance.now() - startTime) / 1000).toFixed(1);
        if (solveStoppedByUser || (err && err.name === 'AbortError')) {
          setStatus('idle', 'Solve stopped by user.', `${elapsedSec} s`);
        } else {
          console.error(err);
          setStatus('error', 'Network or server error: ' + err.message);
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
      if (fails === 0 && Array.isArray(node.boards) && node.boards.includes(-1)) {
        return { primary: 'Fails: Actual (#0)', secondary: '' };
      }
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

  // Sentry User Feedback & Error Reporting
  const SENTRY_DSN = 'https://4f773fa337d8e6b1e559790da9ee1fb1@o4512261310709760.ingest.us.sentry.io/4512261325324288';
  const SENTRY_ENVELOPE_URL = 'https://o4512261310709760.ingest.us.sentry.io/api/4512261325324288/envelope/?sentry_key=4f773fa337d8e6b1e559790da9ee1fb1&sentry_version=7&sentry_client=bridge-high-line-web%2F1.0.0';

  const openFeedbackBtn = document.getElementById('openFeedbackBtn');
  const feedbackModal = document.getElementById('feedbackModal');
  const closeFeedbackModalBtn = document.getElementById('closeFeedbackModalBtn');
  const cancelFeedbackBtn = document.getElementById('cancelFeedbackBtn');
  const feedbackForm = document.getElementById('feedbackForm');
  const feedbackNameInput = document.getElementById('feedbackNameInput');
  const feedbackEmailInput = document.getElementById('feedbackEmailInput');
  const feedbackMessageInput = document.getElementById('feedbackMessageInput');
  const feedbackIncludeDealCheckbox = document.getElementById('feedbackIncludeDealCheckbox');
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

  function collectFeedbackDealSnapshot() {
    const snap = {
      preset: presetSelect ? presetSelect.value : '',
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

  async function submitSentryFeedback({ name, email, message, includeDeal }) {
    const eventId = generateSentryEventId();
    const feedbackEventId = generateSentryEventId();
    const nowIso = new Date().toISOString();
    const dealSnapshot = includeDeal ? collectFeedbackDealSnapshot() : null;
    const environment = window.location.hostname.includes('github.io') ? 'github-pages' : 'local';

    const firstLine = message.split('\n')[0].trim().slice(0, 80) || 'Feedback';
    const dealSummaryText = dealSnapshot
      ? `\n\n--- Deal State ---\nPreset: ${dealSnapshot.preset}\nN: ${dealSnapshot.hands.N}\nE: ${dealSnapshot.hands.E}\nS: ${dealSnapshot.hands.S}\nW: ${dealSnapshot.hands.W}\nUnknown: ${dealSnapshot.unknown_cards || '-'}\nTrump: ${dealSnapshot.trump}, Leader: ${dealSnapshot.leader}, Target: ${dealSnapshot.min_tricks}, Rand deals: ${dealSnapshot.num_iters}`
      : '';

    const userObj = {
      name: name || 'Anonymous',
      ...(email ? { email } : {})
    };

    const eventPayload = {
      event_id: eventId,
      timestamp: nowIso,
      platform: 'javascript',
      level: 'info',
      logger: 'user.feedback',
      environment,
      message: {
        formatted: `[User Feedback] ${firstLine}\n\n${message}${dealSummaryText}`
      },
      fingerprint: ['user-feedback', eventId],
      user: userObj,
      tags: {
        feedback: 'true',
        preset: dealSnapshot ? (dealSnapshot.preset || 'custom') : 'none',
        trump: dealSnapshot ? (dealSnapshot.trump || '') : '',
        leader: dealSnapshot ? (dealSnapshot.leader || '') : '',
        backend_mode: backendMode || 'unknown'
      },
      contexts: dealSnapshot ? { deal: dealSnapshot } : {},
      extra: {
        feedback_message: message,
        deal_state: dealSnapshot,
        url: window.location.href,
        user_agent: navigator.userAgent
      }
    };

    const feedbackMessage = `${message}${dealSummaryText}`.slice(0, 4096);
    const feedbackPayload = {
      event_id: feedbackEventId,
      timestamp: nowIso,
      level: 'info',
      platform: 'javascript',
      environment,
      user: userObj,
      tags: {
        preset: dealSnapshot ? (dealSnapshot.preset || 'custom') : 'none',
        backend_mode: backendMode || 'unknown'
      },
      contexts: {
        feedback: {
          name: name || 'Anonymous',
          ...(email ? { contact_email: email } : {}),
          message: feedbackMessage,
          associated_event_id: eventId,
          url: window.location.href
        },
        ...(dealSnapshot ? { deal: dealSnapshot } : {})
      }
    };

    // Sentry envelope spec: `event`, `transaction`, and `feedback` items are
    // mutually exclusive (at most one per envelope, and the envelope header's
    // `event_id` must match the contained item's `event_id`). Combining both
    // in a single envelope causes Relay to reject the request with HTTP 413.
    const eventEnvelope = [
      JSON.stringify({ event_id: eventId, sent_at: nowIso, dsn: SENTRY_DSN }),
      JSON.stringify({ type: 'event', content_type: 'application/json' }),
      JSON.stringify(eventPayload)
    ].join('\n') + '\n';

    const feedbackEnvelope = [
      JSON.stringify({ event_id: feedbackEventId, sent_at: nowIso, dsn: SENTRY_DSN }),
      JSON.stringify({ type: 'feedback', content_type: 'application/json' }),
      JSON.stringify(feedbackPayload)
    ].join('\n') + '\n';

    const [eventRes, feedbackRes] = await Promise.all([
      fetch(SENTRY_ENVELOPE_URL, { method: 'POST', body: eventEnvelope }),
      fetch(SENTRY_ENVELOPE_URL, { method: 'POST', body: feedbackEnvelope })
    ]);

    if (!eventRes.ok && !feedbackRes.ok) {
      const errText = await eventRes.text().catch(() => '');
      throw new Error(
        `Sentry returned HTTP ${eventRes.status}${errText ? ': ' + errText.slice(0, 120) : ''}`
      );
    }
    return eventId;
  }

  function openFeedbackModal() {
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
        await submitSentryFeedback({ name, email, message, includeDeal });
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

  // Initial UI Render & Fetch Presets
  fetchPresets().then(() => {
    if (presetSelect && presetSelect.value) {
      loadPreset(presetSelect.value);
    } else {
      updateDealSvg();
    }
  });
});
