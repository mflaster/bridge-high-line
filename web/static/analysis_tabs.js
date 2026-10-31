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
    const baseActiveIds =
      Array.isArray(baseData.active_board_ids) &&
      baseData.active_board_ids.length > 0
        ? baseData.active_board_ids
        : Array.from({ length: baseTotal }, (_, b) => b);
    const origLines = Array.isArray(baseData.lines) ? baseData.lines : [];
    const fmtFrontend = (s) => (typeof formatCardTokensForFrontend === 'function')
      ? formatCardTokensForFrontend(s)
      : s;
    if (origLines.length > 1) {
      origLines.forEach((lineObj, idx) => {
        let label = fmtFrontend(lineObj.label || `Line ${idx + 1}`);
        if (hasAlts) {
          label = label.replace(/^Line\s+(\d+):/i, 'Original line $1:');
        }
        const lineSucc = (lineObj && lineObj.covered_boards !== undefined)
          ? lineObj.covered_boards
          : baseSuccess;
        const hasLineEval = Boolean(
          lineObj &&
          typeof lineObj.eval_total_boards === 'number' &&
          lineObj.eval_total_boards > 0 &&
          typeof lineObj.eval_covered_boards === 'number'
        );
        const headlineCovered = hasLineEval
          ? lineObj.eval_covered_boards
          : lineSucc;
        const headlineTotal = hasLineEval
          ? lineObj.eval_total_boards
          : baseTotal;
        const pctPrefix = formatLineSuccessPctPrefix(
          headlineCovered,
          headlineTotal
        );
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
          headlineCovered: headlineCovered,
          headlineTotal: headlineTotal,
          coveredBoardIds: covIds || [],
          activeBoardIds: baseActiveIds,
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
        eval_total_boards: baseData.eval_total_boards,
        eval_covered_boards: baseData.eval_success_boards,
        actual_hand: baseData.actual_hand,
        distribution_summary: baseData.distribution_summary,
        failure_distribution_summary: baseData.failure_distribution_summary
      };
      const lineSucc =
        (singleLineObj && singleLineObj.covered_boards !== undefined)
          ? singleLineObj.covered_boards
          : baseSuccess;
      const hasLineEval = Boolean(
        singleLineObj &&
        typeof singleLineObj.eval_total_boards === 'number' &&
        singleLineObj.eval_total_boards > 0 &&
        typeof singleLineObj.eval_covered_boards === 'number'
      );
      const headlineCovered = hasLineEval
        ? singleLineObj.eval_covered_boards
        : lineSucc;
      const headlineTotal = hasLineEval
        ? singleLineObj.eval_total_boards
        : baseTotal;
      const pctPrefix = formatLineSuccessPctPrefix(
        headlineCovered,
        headlineTotal
      );
      const rawTitle = (singleLineObj && singleLineObj.label)
        ? fmtFrontend(singleLineObj.label).replace(
            /^Line\s+1:/i,
            'Original line:'
          )
        : 'Original line';
      let covIds = Array.isArray(singleLineObj && singleLineObj.covered_board_ids)
        ? singleLineObj.covered_board_ids
        : (Array.isArray(baseData.covered_board_ids)
            ? baseData.covered_board_ids
            : null);
      if (!covIds && singleLineObj && singleLineObj.tree) {
        covIds = Array.from(extractCoveredBoardIdsFromTree(singleLineObj.tree));
      }
      entries.push({
        kind: 'original',
        rawTitle: rawTitle,
        label: `${pctPrefix}Original line`,
        coveredBoards: lineSucc,
        totalBoards: baseTotal,
        headlineCovered: headlineCovered,
        headlineTotal: headlineTotal,
        coveredBoardIds: covIds || [],
        activeBoardIds: baseActiveIds,
        lineObj: singleLineObj,
        baseData: baseData
      });
    }
  }

  if (hasAlts) {
    const fmtFrontend = (s) =>
      (typeof formatCardTokensForFrontend === 'function')
        ? formatCardTokensForFrontend(s)
        : s;
    proposedAltLines.forEach((altEntry) => {
      const altData = altEntry.data || {};
      const altTotal = altData.total_boards || 1;
      const altSuccess = altData.success_boards !== undefined
        ? altData.success_boards
        : (altData.covered_boards || 0);
      const hasAltEval = Boolean(
        typeof altData.eval_total_boards === 'number' &&
        altData.eval_total_boards > 0 &&
        typeof altData.eval_success_boards === 'number'
      );
      const headlineCovered = hasAltEval
        ? altData.eval_success_boards
        : altSuccess;
      const headlineTotal = hasAltEval ? altData.eval_total_boards : altTotal;
      const pctPrefix = formatLineSuccessPctPrefix(
        headlineCovered,
        headlineTotal
      );
      const altOrigLines = Array.isArray(altData.lines) ? altData.lines : [];
      let covIds = Array.isArray(altData.covered_board_ids)
        ? altData.covered_board_ids
        : (altOrigLines.length > 0 &&
            Array.isArray(altOrigLines[0].covered_board_ids)
              ? altOrigLines[0].covered_board_ids
              : null);
      if (!covIds && altData.tree) {
        covIds = Array.from(extractCoveredBoardIdsFromTree(altData.tree));
      }
      const altActiveIds =
        Array.isArray(altData.active_board_ids) &&
        altData.active_board_ids.length > 0
          ? altData.active_board_ids
          : Array.from({ length: altTotal }, (_, b) => b);
      const formattedAltLabel = fmtFrontend(altEntry.label);
      entries.push({
        kind: 'alt',
        rawTitle: formattedAltLabel,
        label: `${pctPrefix}${formattedAltLabel}`,
        coveredBoards: altSuccess,
        totalBoards: altTotal,
        headlineCovered: headlineCovered,
        headlineTotal: headlineTotal,
        coveredBoardIds: covIds || [],
        activeBoardIds: altActiveIds,
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

  const tableDeal = (typeof getTableCompassDealHands === 'function')
    ? getTableCompassDealHands(deal)
    : (deal || { N: '', E: '', S: '', W: '' });

  const rowNorth = document.createElement('div');
  rowNorth.className = 'sample-deal-row-center';
  rowNorth.appendChild(createSampleHandBox('NORTH', tableDeal.N));
  card.appendChild(rowNorth);

  const rowWE = document.createElement('div');
  rowWE.className = 'sample-deal-row-we';
  rowWE.appendChild(createSampleHandBox('WEST', tableDeal.W));
  rowWE.appendChild(createSampleHandBox('EAST', tableDeal.E));
  card.appendChild(rowWE);

  const rowSouth = document.createElement('div');
  rowSouth.className = 'sample-deal-row-center';
  rowSouth.appendChild(createSampleHandBox('SOUTH', tableDeal.S));
  card.appendChild(rowSouth);

  return card;
}

function renderComparePairSection(entryA, idxA, entryB, idxB, allEntries) {
  const activeA = Array.isArray(entryA.activeBoardIds) &&
      entryA.activeBoardIds.length > 0
    ? entryA.activeBoardIds
    : Array.from(
        { length: Math.max(1, entryA.totalBoards || 1) },
        (_, b) => b
      );
  const activeB = Array.isArray(entryB.activeBoardIds) &&
      entryB.activeBoardIds.length > 0
    ? entryB.activeBoardIds
    : Array.from(
        { length: Math.max(1, entryB.totalBoards || 1) },
        (_, b) => b
      );
  const activeSetB = new Set(activeB);
  let compareBoards = activeA.filter(b => activeSetB.has(b));
  if (compareBoards.length === 0) {
    const unionSet = new Set([...activeA, ...activeB]);
    compareBoards = Array.from(unionSet).sort((x, y) => x - y);
  }
  const total = Math.max(1, compareBoards.length);
  const setA = new Set(entryA.coveredBoardIds || []);
  const setB = new Set(entryB.coveredBoardIds || []);

  const bothPass = [];
  const onlyA = []; // Succeeds in A, fails in B
  const onlyB = []; // Fails in A, succeeds in B
  const bothFail = [];

  for (const b of compareBoards) {
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

  // Prune out-of-range selected indices and auto-select 2 lines if <2 selected.
  const hasAlts =
    Array.isArray(proposedAltLines) && proposedAltLines.length > 0;
  Array.from(selectedCompareLineIndices).forEach(idx => {
    if (idx < 0 || idx >= entries.length) {
      selectedCompareLineIndices.delete(idx);
    }
  });
  if (selectedCompareLineIndices.size === 0 && entries.length >= 2) {
    if (!hasAlts && entries.length <= 3) {
      for (let k = 0; k < entries.length; k++) {
        selectedCompareLineIndices.add(k);
      }
    } else {
      selectedCompareLineIndices.add(0);
      selectedCompareLineIndices.add(hasAlts ? entries.length - 1 : 1);
    }
  } else if (selectedCompareLineIndices.size === 0 && entries.length === 1) {
    selectedCompareLineIndices.add(0);
  } else if (selectedCompareLineIndices.size === 1 && entries.length === 2) {
    // Auto-select both when a second line (e.g. an Alt line) is computed.
    selectedCompareLineIndices.add(0);
    selectedCompareLineIndices.add(1);
  }

  if (compareLinesHint) {
    compareLinesHint.textContent = entries.length < 2
      ? 'Only 1 line computed so far — propose a card on the deal diagram ' +
        'and click "Compute new line" to compare!'
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

      const total = entry.headlineTotal || entry.totalBoards || 1;
      const succ = entry.headlineCovered !== undefined
        ? entry.headlineCovered
        : (entry.coveredBoards !== undefined ? entry.coveredBoards : 0);
      const pct = ((succ / total) * 100).toFixed(1);
      const pctBadge = document.createElement('span');
      pctBadge.className = 'compare-line-pct-badge';
      pctBadge.textContent =
        `${succ.toLocaleString()}/${total.toLocaleString()} (${pct}%)`;
      if (entry.lineObj &&
          typeof entry.lineObj.eval_total_boards === 'number' &&
          entry.lineObj.eval_total_boards > 0 &&
          entry.totalBoards > 0) {
        pctBadge.title =
          `Solve sample: ${entry.coveredBoards}/${entry.totalBoards}`;
      }
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
    const sortedSelected =
      Array.from(selectedCompareLineIndices).sort((a, b) => a - b);
    if (sortedSelected.length < 2) {
      const promptEl = document.createElement('div');
      promptEl.className = 'summary-empty';
      promptEl.textContent = entries.length < 2
        ? 'Compute at least 2 lines to compare their succeeding and failing ' +
          'deals.'
        : 'Select at least 2 lines above to view the comparison chart and ' +
          'differing deals.';
      compareLinesResults.appendChild(promptEl);
    } else {
      for (let i = 0; i < sortedSelected.length; i++) {
        for (let j = i + 1; j < sortedSelected.length; j++) {
          const idxA = sortedSelected[i];
          const idxB = sortedSelected[j];
          compareLinesResults.appendChild(
            renderComparePairSection(
              entries[idxA],
              idxA,
              entries[idxB],
              idxB,
              entries
            )
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
  const hasAlts =
    Array.isArray(proposedAltLines) && proposedAltLines.length > 0;

  if (entries.length > 1) {
    entries.forEach((entry, idx) => {
      const opt = document.createElement('option');
      opt.value = String(idx);
      opt.textContent = entry.label;
      equivalentLineSelect.appendChild(opt);
    });
    const activeIdx =
      (selectedEntryIndex !== null &&
        selectedEntryIndex >= 0 &&
        selectedEntryIndex < entries.length)
        ? selectedEntryIndex
        : 0;
    equivalentLineSelect.value = String(activeIdx);
    if (currentSolveData) {
      currentSolveData.active_line_index = activeIdx;
    }
    const firstCov = entries[0].headlineCovered;
    const firstTot = entries[0].headlineTotal;
    const allEqualScore = entries.every(
      e => e.headlineCovered === firstCov && e.headlineTotal === firstTot
    );
    const hasEval = Boolean(
      entries[0].lineObj &&
      typeof entries[0].lineObj.eval_total_boards === 'number' &&
      entries[0].lineObj.eval_total_boards > 0
    );
    const baseSet = new Set(entries[0].coveredBoardIds || []);
    const allSameCoveredSet =
      allEqualScore && !hasEval && entries.every(e => {
        const ids = e.coveredBoardIds || [];
        return ids.length === baseSet.size && ids.every(b => baseSet.has(b));
      });
    if (equivalentLinesBadge) {
      if (hasAlts) {
        equivalentLinesBadge.textContent =
          `${entries.length} Lines (Original + ${proposedAltLines.length} Alt)`;
      } else if (allSameCoveredSet) {
        equivalentLinesBadge.textContent = `${entries.length} Equivalent Lines`;
      } else {
        equivalentLinesBadge.textContent = `${entries.length} Finalist Lines`;
      }
    }
    if (equivalentLinesHint) {
      if (hasAlts) {
        equivalentLinesHint.textContent =
          'Switch between original and proposed lines:';
      } else if (hasEval) {
        equivalentLinesHint.textContent =
          `Ranked on ${firstTot.toLocaleString('en-US')} deals` +
          ' — switch to inspect:';
      } else if (allSameCoveredSet) {
        equivalentLinesHint.textContent =
          'Choose an equally optimal line of play:';
      } else {
        equivalentLinesHint.textContent =
          'Top distinct lines of play — switch to inspect:';
      }
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
      if (typeof lineObj.eval_total_boards === 'number' &&
          lineObj.eval_total_boards > 0 &&
          typeof lineObj.eval_covered_boards === 'number') {
        baseCopy.eval_total_boards = lineObj.eval_total_boards;
        baseCopy.eval_success_boards = lineObj.eval_covered_boards;
        baseCopy.eval_failure_boards = Math.max(
          0,
          lineObj.eval_total_boards - lineObj.eval_covered_boards
        );
      } else {
        delete baseCopy.eval_total_boards;
        delete baseCopy.eval_success_boards;
        delete baseCopy.eval_failure_boards;
      }
    }
    currentSolveData = baseCopy;
    currentSolveData.active_line_index = idx;
    hasComputedProposedLine =
      Array.isArray(proposedAltLines) && proposedAltLines.length > 0;
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
  const counts = (typeof getHeadlineSolveCounts === 'function')
    ? getHeadlineSolveCounts(currentSolveData)
    : {
        total: currentSolveData.total_boards || 1,
        success: currentSolveData.success_boards !== undefined
          ? currentSolveData.success_boards
          : (currentSolveData.covered_boards || 0),
        failure: currentSolveData.failure_boards !== undefined
          ? currentSolveData.failure_boards
          : Math.max(
              0,
              (currentSolveData.total_boards || 1) -
                (currentSolveData.success_boards !== undefined
                  ? currentSolveData.success_boards
                  : (currentSolveData.covered_boards || 0))
            )
      };
  const fmtRatio = (typeof formatBoardRatioText === 'function')
    ? formatBoardRatioText
    : (c, t) => `${c}/${t} (${((c / Math.max(1, t)) * 100).toFixed(1)}%)`;
  if (statSuccess) {
    statSuccess.textContent = fmtRatio(counts.success, counts.total);
  }
  if (statFailure) {
    statFailure.textContent = fmtRatio(counts.failure, counts.total);
  }

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


function renderDistributionSummary(dist) {
  if (!dist) {
    clearDistributionSummary();
    return;
  }
  const rules = dist.invariant_rules || [];
  const bounds = dist.suit_bounds || {};
  const honors = dist.honor_stats || {};
  const rotMap = (typeof getRotationSeatMap === 'function')
    ? getRotationSeatMap()
    : { lhoName: 'East', rhoName: 'West', lhoSeat: 'E', rhoSeat: 'W' };

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
        const formattedRule = (typeof formatInvariantRuleForRotation === 'function')
          ? formatInvariantRuleForRotation(r)
          : r.replace(
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
    const boundsH1Header =
      document.getElementById('summaryBoundsHand1Header') ||
      summaryBoundsTable.querySelector('thead th:nth-child(2)');
    const boundsH2Header =
      document.getElementById('summaryBoundsHand2Header') ||
      summaryBoundsTable.querySelector('thead th:nth-child(3)');
    if (boundsH1Header) {
      boundsH1Header.textContent = `Hand 1 (${rotMap.lhoName}) Range`;
    }
    if (boundsH2Header) {
      boundsH2Header.textContent = `Hand 2 (${rotMap.rhoName}) Range`;
    }
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
    const honorH1Header =
      document.getElementById('summaryHonorHand1Header') ||
      summaryHonorTable.querySelector('thead th:nth-child(3)');
    const honorH2Header =
      document.getElementById('summaryHonorHand2Header') ||
      summaryHonorTable.querySelector('thead th:nth-child(4)');
    if (honorH1Header) {
      honorH1Header.textContent = `Hand 1 (${rotMap.lhoName}) %`;
    }
    if (honorH2Header) {
      honorH2Header.textContent = `Hand 2 (${rotMap.rhoName}) %`;
    }
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

      const tableDeal = (typeof getTableCompassDealHands === 'function')
        ? getTableCompassDealHands(deal)
        : (deal || { N: '', E: '', S: '', W: '' });

      const rowNorth = document.createElement('div');
      rowNorth.className = 'sample-deal-row-center';
      rowNorth.appendChild(createSampleHandBox('NORTH', tableDeal.N));
      card.appendChild(rowNorth);

      const rowWE = document.createElement('div');
      rowWE.className = 'sample-deal-row-we';
      rowWE.appendChild(createSampleHandBox('WEST', tableDeal.W));
      rowWE.appendChild(createSampleHandBox('EAST', tableDeal.E));
      card.appendChild(rowWE);

      const rowSouth = document.createElement('div');
      rowSouth.className = 'sample-deal-row-center';
      rowSouth.appendChild(createSampleHandBox('SOUTH', tableDeal.S));
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
