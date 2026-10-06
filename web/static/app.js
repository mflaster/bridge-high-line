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
      target: 3,
      iters: 15,
      north: 'S AQ; H A; D K; C -;',
      east: 'S K9; H K; D A; C -;',
      south: 'S J8; H 2; D 2; C -;',
      west: 'S T7; H Q; D Q; C -;',
      unknown_cards: ''
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

          if (child.condition) {
            const midX = (startX + endX) / 2;
            const labelGroup = document.createElementNS('http://www.w3.org/2000/svg', 'g');
            labelGroup.setAttribute('class', 'tree-edge-label-group');

            const condText = child.condition;
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
      title.textContent = truncateText(node.label || node.trick || 'Node', 40);
      g.appendChild(title);

      // Detail Text
      const detail = document.createElementNS('http://www.w3.org/2000/svg', 'text');
      detail.setAttribute('x', '12');
      detail.setAttribute('y', '48');
      detail.setAttribute('class', 'node-text-detail');
      let detailText = '';
      if (node.type === 'outcome' && node.ns_tricks !== undefined) {
        detailText = `N/S Won: ${node.ns_tricks} | `;
      }
      if (node.success_boards !== undefined && node.total_boards !== undefined) {
        const pct = Math.round((node.success_boards / node.total_boards) * 100);
        detailText += `Success: ${node.success_boards}/${node.total_boards} (${pct}%)`;
      } else if (node.covered_boards !== undefined) {
        detailText += `Success: ${node.covered_boards}`;
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
    const nsInfo = node.ns_tricks !== undefined ? ` | N/S Tricks Won: ${node.ns_tricks}` : '';
    nodeDetailContent.textContent = `ID: ${node.id} | Type: ${node.type} | Label: "${node.label || ''}" | Trick: "${node.trick || ''}"${nsInfo} | ${boardsInfo}`;
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
