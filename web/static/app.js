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
  const statLead = document.getElementById('statLead');
  const cardStatSuccess = document.getElementById('cardStatSuccess');
  const cardStatFailure = document.getElementById('cardStatFailure');
  const statSuccess = document.getElementById('statSuccess');
  const statFailure = document.getElementById('statFailure');
  const equivalentLinesBar = document.getElementById('equivalentLinesBar');
  const equivalentLinesBadge = document.getElementById('equivalentLinesBadge');
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
  const panelDecisionTree = document.getElementById('panelDecisionTree');
  const panelDistributionSummary = document.getElementById('panelDistributionSummary');
  const panelSampleHands = document.getElementById('panelSampleHands');
  const ruleCounterBadge = document.getElementById('ruleCounterBadge');
  const sampleHandsCounterBadge = document.getElementById('sampleHandsCounterBadge');
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

    if (panelDecisionTree) panelDecisionTree.classList.toggle('hidden', tabName !== 'tree');
    if (panelDistributionSummary) panelDistributionSummary.classList.toggle('hidden', tabName !== 'summary');
    if (panelSampleHands) panelSampleHands.classList.toggle('hidden', tabName !== 'sample_hands');
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
        const pct = Math.round((success / total) * 100);
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
        const pct = Math.round((failure / total) * 100);
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

  function populateEquivalentLinesControls(data) {
    if (!equivalentLinesBar || !equivalentLineSelect) return;
    equivalentLineSelect.innerHTML = '';
    const lines = (data && Array.isArray(data.lines)) ? data.lines : [];
    if (lines.length > 1) {
      lines.forEach((lineObj, idx) => {
        const opt = document.createElement('option');
        opt.value = String(idx);
        opt.textContent = lineObj.label || `Line ${idx + 1}`;
        equivalentLineSelect.appendChild(opt);
      });
      equivalentLineSelect.value = '0';
      if (equivalentLinesBadge) {
        equivalentLinesBadge.textContent = `${lines.length} Equivalent Lines`;
      }
      equivalentLinesBar.classList.remove('hidden');
    } else {
      equivalentLinesBar.classList.add('hidden');
    }
  }

  function selectEquivalentLine(rawIndex) {
    if (!currentSolveData || !Array.isArray(currentSolveData.lines) ||
        currentSolveData.lines.length === 0) {
      return;
    }
    const count = currentSolveData.lines.length;
    const idx = ((parseInt(rawIndex, 10) || 0) % count + count) % count;
    const lineObj = currentSolveData.lines[idx];
    if (!lineObj) return;

    currentSolveData.active_line_index = idx;
    currentSolveData.tree = JSON.parse(JSON.stringify(lineObj.tree || {}));
    currentSolveData.best_lead = lineObj.best_lead || '-';
    currentSolveData.distribution_summary = lineObj.distribution_summary || null;
    currentSolveData.failure_distribution_summary =
      lineObj.failure_distribution_summary || null;

    const total = currentSolveData.total_boards || 1;
    if (Array.isArray(lineObj.covered_board_ids) &&
        Array.isArray(currentSolveData.all_hands)) {
      const coveredSet = new Set(lineObj.covered_board_ids);
      const sampleHands = [];
      const failureSampleHands = [];
      currentSolveData.all_hands.forEach(handEntry => {
        const isCov = coveredSet.has(handEntry.board_id);
        handEntry.succeeded = isCov;
        if (isCov && sampleHands.length < 100) {
          sampleHands.push(handEntry);
        } else if (!isCov && failureSampleHands.length < 100) {
          failureSampleHands.push(handEntry);
        }
      });
      currentSolveData.sample_hands = sampleHands;
      currentSolveData.failure_sample_hands = failureSampleHands;
      currentSolveData.success_boards = coveredSet.size;
      currentSolveData.covered_boards = coveredSet.size;
      currentSolveData.failure_boards = Math.max(0, total - coveredSet.size);
    }

    if (statLead) {
      statLead.textContent = currentSolveData.best_lead || '-';
    }
    const success = currentSolveData.success_boards !== undefined
      ? currentSolveData.success_boards
      : (currentSolveData.covered_boards || 0);
    const failure = currentSolveData.failure_boards !== undefined
      ? currentSolveData.failure_boards
      : Math.max(0, total - success);
    const successPct = Math.round((success / total) * 100);
    const failurePct = Math.round((failure / total) * 100);
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
  let originalSolveSnapshot = null; // Saved original solve data & inputs before any proposed line
  let hasComputedProposedLine = false;
  let activeProposalContext = null; // { trickNum, selectedIdx, leaderIdx, nsTricksBefore, playedStates, prefixTrickRows, proposedCards: [] }

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

  function updateProposeLineBarUI() {
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
        computeNewLineBtn.disabled = (proposedCards.length === 0);
      }
      if (undoProposedCardBtn) {
        undoProposedCardBtn.disabled = (proposedCards.length === 0);
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
      const hasProposedCards = Boolean(activeProposalContext && activeProposalContext.proposedCards.length > 0);
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

    const seatPlayState = (activeTrickCardStates && seatKey) ? activeTrickCardStates[seatKey] : null;
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
      if (!isUnknown && leadSuit) {
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

    const nTurn = (nextSeatIdx === 0);
    const eTurnKnown = (nextSeatIdx === 1 && eKnown);
    const sTurn = (nextSeatIdx === 2);
    const wTurnKnown = (nextSeatIdx === 3 && wKnown);
    const uTurn = ((nextSeatIdx === 1 && !eKnown && uHasCards) ||
                   (nextSeatIdx === 3 && !wKnown && uHasCards));

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
        uTurn ? { isTurn: true, leadSuit, targetSeatIdx: nextSeatIdx } : null);
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

    clearSampleHands('No sample hands available yet. Click "Solve Deal" to generate and inspect deals.');
    if (sampleHandsModeHeader) sampleHandsModeHeader.classList.add('hidden');
    if (sampleHandsModeBadge) {
      sampleHandsModeBadge.className = 'mode-badge mode-success';
      sampleHandsModeBadge.textContent = 'Covered Deals';
    }
    if (sampleHandsModeSub) sampleHandsModeSub.textContent = '';

    activateTab('tree');
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

      currentSolveData = data;
      currentSolveData.active_line_index = 0;
      hasComputedProposedLine = false;
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
          numIters: numIters.value
        }
      };
      updateProposeLineBarUI();

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
      if (traceHandInput) {
        traceHandInput.placeholder = `1..${total}`;
      }

      populateEquivalentLinesControls(data);

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
      if (!activeProposalContext || activeProposalContext.proposedCards.length === 0) return;
      activeProposalContext.proposedCards.pop();
      applyProposalStateToSvg();
    });
  }

  if (resetLineBtn) {
    resetLineBtn.addEventListener('click', () => {
      if (!hasComputedProposedLine && activeProposalContext && activeProposalContext.proposedCards.length > 0) {
        activeProposalContext.proposedCards = [];
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
      }

      hasComputedProposedLine = false;
      activeProposalContext = null;
      activeTrickCardStates = null;
      clearTracedHandStateOnly();
      if (traceHandInput) traceHandInput.value = '';

      const data = JSON.parse(JSON.stringify(snap.data));
      currentSolveData = data;
      currentSolveData.active_line_index = 0;
      updateProposeLineBarUI();
      populateEquivalentLinesControls(data);

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
      if (traceHandInput) {
        traceHandInput.placeholder = `1..${total}`;
      }

      setStatus('success', 'Restored original solved line.');
      renderSolveTree();
      updateViewMode(activeViewMode);
    });
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
        initial_ns_tricks: nsTricksBeforeSolve
      };

      const remUnknownStr = formatSuitsToHandStr(remainingBeforeTrickK.U, true);
      if (remUnknownStr) {
        payload.unknown_cards = remUnknownStr;
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
        }

        hasComputedProposedLine = true;
        activeProposalContext = null;
        currentSolveData = data;
        currentSolveData.active_line_index = 0;
        clearTracedHandStateOnly();
        if (traceHandInput) {
          traceHandInput.value = '';
          traceHandInput.placeholder = `1..${total}`;
        }
        updateProposeLineBarUI();
        populateEquivalentLinesControls(data);

        setStatus('success', `Computed new line from Trick #${ctx.trickNum}!`, timeStr);
        execTime.textContent = timeStr;

        resultsSummary.classList.remove('hidden');
        statTricks.textContent = data.tricks_won !== undefined ? data.tricks_won : '-';
        statLead.textContent = data.best_lead || '-';

        const successPct = Math.round((success / total) * 100);
        const failurePct = Math.round((failure / total) * 100);
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
    if (!sampleHandsGrid) return;
    const cards = sampleHandsGrid.querySelectorAll('.sample-deal-card');
    cards.forEach(card => {
      const bId = card.dataset.boardId !== undefined ? parseInt(card.dataset.boardId, 10) : -1;
      card.classList.toggle('traced-deal-card', activeTracedBoardId !== null && bId === activeTracedBoardId);
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
    let leaf = targetNode;
    while (leaf.children && leaf.children.length > 0) {
      leaf._collapsed = false;
      let chosenIdx = 0;
      if (activeTracedNodeSet) {
        const tracedIdx = leaf.children.findIndex(c => activeTracedNodeSet.has(c));
        if (tracedIdx !== -1) {
          chosenIdx = tracedIdx;
        }
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
    const withPct = (n) => total ? `${n} (${Math.round((n / total) * 100)}%)` : `${n}`;
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
      const eHasCards = hasTracedEW || countHandCards(inputEast ? inputEast.value : '') > 0;
      const wHasCards = hasTracedEW || countHandCards(inputWest ? inputWest.value : '') > 0;
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
        if ((seat === 'E' && !eHasCards && uHasCards) ||
            (seat === 'W' && !wHasCards && uHasCards)) {
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
      activeProposalContext = {
        trickNum: selectedTrickRow.trickNum,
        selectedIdx: selectedIdx,
        leaderIdx: selectedTrickRow.leadSeatIdx,
        nsTricksBefore: nsTricksBefore,
        playedStates: clonePlayStates(states),
        prefixTrickRows: prefixTrickRows,
        proposedCards: []
      };

      activeTrickCardStates = states;
      updateProposeLineBarUI();
      if (rowElements) {
        rowElements.forEach((el, idx) => {
          el.classList.toggle('trick-row-selected', idx === selectedIdx);
          el.classList.toggle('trick-row-past', idx < selectedIdx);
        });
      }
      updateDealSvg();
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

