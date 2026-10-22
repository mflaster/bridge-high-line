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
  const maxHandNum = Math.max(
    totalBoards,
    Array.isArray(currentSolveData.all_hands)
      ? currentSolveData.all_hands.length
      : 0
  );
  const minHandNum = currentSolveData.actual_hand ? 0 : 1;
  if (!cleaned || isNaN(handNum) || handNum < minHandNum || handNum > maxHandNum) {
    setTraceHandError(`Enter a hand number between ${minHandNum} and ${maxHandNum}.`);
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

function getNodeFillProportions(node) {
  if (!node) return { dealFrac: 1, failFrac: 0 };
  const total = (node.total_boards !== undefined && node.total_boards > 0)
    ? node.total_boards
    : (currentSolveData && currentSolveData.total_boards > 0 ? currentSolveData.total_boards : 0);
  if (node.failure) {
    const fails = Math.max(
      0,
      node.failure_boards !== undefined ? node.failure_boards : (node.covered_boards || 0)
    );
    const rawDealFrac = total > 0 ? Math.min(1, Math.max(0, fails / total)) : 1;
    const dealFrac = 0.10 + 0.90 * rawDealFrac;
    return { dealFrac, failFrac: 1 };
  }
  const wins = Math.max(
    0,
    node.success_boards !== undefined ? node.success_boards : (node.covered_boards || 0)
  );
  const fails = Math.max(0, node.failure_boards || 0);
  const reaching = wins + fails;
  const rawDealFrac = total > 0 ? Math.min(1, Math.max(0, reaching / total)) : 1;
  const dealFrac = 0.10 + 0.90 * rawDealFrac;
  const failFrac = (reaching > 0 && fails > 0)
    ? Math.min(1, Math.max(0, fails / reaching))
    : 0;
  return { dealFrac, failFrac };
}

function getNodeFailFraction(node) {
  return getNodeFillProportions(node).failFrac;
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

    const { dealFrac, failFrac } = getNodeFillProportions(node);
    const greyColor = '#1e293b';
    const successColor = '#064e3b';
    const failColor = '#450a0a';
    if (dealFrac <= 0) {
      rect.style.fill = greyColor;
    } else if (dealFrac >= 1 && failFrac <= 0) {
      rect.style.fill = successColor;
    } else if (dealFrac >= 1 && failFrac >= 1) {
      rect.style.fill = failColor;
    } else {
      const patId = `node-fill-pat-${failGradCounter++}`;
      const pat = document.createElementNS('http://www.w3.org/2000/svg', 'pattern');
      pat.setAttribute('id', patId);
      pat.setAttribute('patternUnits', 'objectBoundingBox');
      pat.setAttribute('width', '1');
      pat.setAttribute('height', '1');

      const bgRect = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
      bgRect.setAttribute('x', '0');
      bgRect.setAttribute('y', '0');
      bgRect.setAttribute('width', nodeWidth);
      bgRect.setAttribute('height', nodeHeight);
      bgRect.setAttribute('fill', greyColor);
      pat.appendChild(bgRect);

      const activeHeight = dealFrac * nodeHeight;
      const activeY = nodeHeight - activeHeight;
      const successWidth = (1 - failFrac) * nodeWidth;
      const failWidth = nodeWidth - successWidth;

      if (successWidth > 0) {
        const winRect = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
        winRect.setAttribute('x', '0');
        winRect.setAttribute('y', activeY.toFixed(2));
        winRect.setAttribute('width', successWidth.toFixed(2));
        winRect.setAttribute('height', activeHeight.toFixed(2));
        winRect.setAttribute('fill', successColor);
        pat.appendChild(winRect);
      }

      if (failWidth > 0) {
        const loseRect = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
        loseRect.setAttribute('x', successWidth.toFixed(2));
        loseRect.setAttribute('y', activeY.toFixed(2));
        loseRect.setAttribute('width', failWidth.toFixed(2));
        loseRect.setAttribute('height', activeHeight.toFixed(2));
        loseRect.setAttribute('fill', failColor);
        pat.appendChild(loseRect);
      }

      treeDefs.appendChild(pat);
      rect.style.fill = `url(#${patId})`;
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
          const existingRaw = (parsedCards[foundIdx].raw || '').replace(/[\(\)]/g, '');
          const isWildcardSlot = !existingRaw ||
            existingRaw.includes('_') ||
            existingRaw.includes('x') ||
            existingRaw.includes('X');
          if (isWildcardSlot || existingRaw.toUpperCase() === explicitWinnerCard.toUpperCase()) {
            // Update the played card for this seat if specified in the branch condition
            const updatedCard = parseSingleCardToken(explicitWinnerCard, leadSuit, trumpCode);
            updatedCard.seatIdx = explicitWinnerSeat;
            updatedCard.seatName = getSeatName(explicitWinnerSeat);
            parsedCards[foundIdx] = updatedCard;
          }
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
      const rotMap = (typeof getRotationSeatMap === 'function')
        ? getRotationSeatMap()
        : { declSeat: 'N', lhoSeat: 'E', dummySeat: 'S', rhoSeat: 'W' };
      const tableDeal = (typeof getTableCompassDealHands === 'function')
        ? getTableCompassDealHands(activeTracedDeal)
        : activeTracedDeal;
      const defSeats = (rotMap.declSeat === 'E' || rotMap.declSeat === 'W')
        ? [rotMap.lhoSeat, rotMap.rhoSeat]
        : ['W', 'E'];
      holdingsHtml = `
        <div class="traced-hand-holdings">
          <div><strong>${defSeats[0]}:</strong> ${formatCompactHand(tableDeal[defSeats[0]])}</div>
          <div><strong>${defSeats[1]}:</strong> ${formatCompactHand(tableDeal[defSeats[1]])}</div>
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
        // If the generalized card was unsuited ('__') or wildcard ('x'/'_'/'<H'), use the concrete card from the board
        if (!origCard.suit || origCard.rankStr === '__' || origCard.rankStr === '_' ||
            origCard.rankStr === 'x' || origCard.rankStr === 'X' ||
            (origCard.rankStr && origCard.rankStr.startsWith('<'))) {
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
                                 entry.rankStr === '_' || entry.rankStr === '__' ||
                                 (entry.rankStr && entry.rankStr.startsWith('<')));
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
        winnerSeatIdx: (tr.cards[tr.winningCardIdx] ? tr.cards[tr.winningCardIdx].seatIdx : tr.leadSeatIdx),
        boardToTrick: (tr.node && tr.node.board_to_trick)
          ? Object.assign({}, tr.node.board_to_trick)
          : null
      });
    }

    const selectedTrickRow = trickRows[selectedIdx];
    const playedBeforeSelected = clonePlayStates(states);
    const currentResolvedCards = resolveActualCardsForTrick(selectedTrickRow);
    currentResolvedCards.forEach((c, idx) => recordCard(c, 'current', idx === 0));
    const selectedTrickStates = clonePlayStates(states);

    const collectSubtreeBoardIds = (rootNode) => {
      const set = new Set();
      const walk = (n) => {
        if (!n) return;
        if (Array.isArray(n.boards)) {
          n.boards.forEach(b => {
            if (typeof b === 'number' && b >= 0) set.add(b);
          });
        }
        if (Array.isArray(n.children)) {
          n.children.forEach(walk);
        }
      };
      walk(rootNode);
      return Array.from(set).sort((a, b) => a - b);
    };

    const trickNode = selectedTrickRow.node;
    let startOfTrickNode = trickNode;
    if (trickNode && trickNode._parent &&
        !extractTrickString(trickNode._parent) &&
        trickNode.condition &&
        trickNode.condition === trickNode.trick) {
      startOfTrickNode = trickNode._parent;
    }
    const branchBeforeTrickIds = collectSubtreeBoardIds(startOfTrickNode);

    let inTrickNode = trickNode;
    const trickPathIdx = path.indexOf(trickNode);
    if (trickPathIdx !== -1 && trickPathIdx + 1 < path.length) {
      const nextPathNode = path[trickPathIdx + 1];
      if (nextPathNode && nextPathNode.condition &&
          (/(?:when|if)\s+([0-3]|north|east|south|west|n|e|s|w)\s+(?:plays|discards)/i.test(nextPathNode.condition) ||
           /(?:when|if)\s+([0-3]|north|east|south|west|n|e|s|w)\s+wins/i.test(nextPathNode.condition))) {
        inTrickNode = nextPathNode;
      }
    }
    const branchInTrickIds = collectSubtreeBoardIds(inTrickNode);

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
      playedStates: playedBeforeSelected,
      selectedTrickStates: selectedTrickStates,
      prefixTrickRows: prefixTrickRows,
      branchBeforeTrickIds: branchBeforeTrickIds,
      branchInTrickIds: branchInTrickIds,
      branchTrickCards: currentResolvedCards.map(c => Object.assign({}, c)),
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
        const safeRankHtml = String(c.rankStr || '').replace(/</g, '&lt;');
        const innerHtml = c.suitSym
          ? `<span class="suit-symbol">${c.suitSym}</span><span class="card-rank">${safeRankHtml}</span>`
          : `<span class="card-rank">${safeRankHtml}</span>`;
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
  const regex = /\([^\)]+\)|[S|H|D|C][<A|K|Q|J|T|10|9|8|7|6|5|4|3|2|X|x|_]+|[A|K|Q|J|T|10|9|8|7|6|5|4|3|2]+[S|H|D|C]|\S+/gi;
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
        !/^declarer/i.test(trimmed) &&
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

