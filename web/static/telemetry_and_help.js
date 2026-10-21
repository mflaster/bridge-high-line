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

function getActivePresetAndHandInfo() {
  const presetKey = presetSelect ? presetSelect.value : '';
  let presetName = '';
  if (presetKey && PRESETS && PRESETS[presetKey] &&
      PRESETS[presetKey].name) {
    presetName = PRESETS[presetKey].name;
  } else if (presetKey) {
    const customEntry = sessionCustomHands.find(h => h.key === presetKey);
    if (customEntry && customEntry.label) {
      presetName = customEntry.label;
    } else if (presetSelect && presetSelect.selectedIndex >= 0 &&
               presetSelect.options[presetSelect.selectedIndex]) {
      presetName =
        presetSelect.options[presetSelect.selectedIndex].textContent || '';
    }
  }
  let handLabel = '';
  if (loadedHandSource) {
    const fName =
      loadedHandSource.fileName || loadedHandSource.file_name || '';
    const bLbl =
      loadedHandSource.boardLabel || loadedHandSource.board_label || '';
    if (fName && bLbl) {
      handLabel = `Hand ${bLbl} from ${fName}`;
    } else if (fName || bLbl) {
      handLabel = fName || `Hand ${bLbl}`;
    }
  }
  if (!handLabel) {
    if (presetName && presetKey && presetName !== presetKey) {
      handLabel = `${presetName} (${presetKey})`;
    } else {
      handLabel = presetName || presetKey || 'Custom Deal';
    }
  }
  return {
    preset: presetKey,
    preset_name: presetName,
    hand_label: handLabel
  };
}

function collectFeedbackDealSnapshot() {
  const handInfo = getActivePresetAndHandInfo();
  const snap = {
    preset: handInfo.preset,
    preset_name: handInfo.preset_name,
    hand_label: handInfo.hand_label,
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
  if (importedBiddingReqs &&
      ((Array.isArray(importedBiddingReqs.E) && importedBiddingReqs.E.length > 0) ||
       (Array.isArray(importedBiddingReqs.W) && importedBiddingReqs.W.length > 0))) {
    snap.bidding_reqs = importedBiddingReqs;
  }
  if (currentSolveData) {
    snap.last_solve = {
      success: Boolean(currentSolveData.success),
      tricks_won: currentSolveData.tricks_won,
      best_lead: currentSolveData.best_lead || ''
    };
  }
  if (lastStatusErrorText) {
    snap.last_error = lastStatusErrorText;
  }
  if (lastSolveRequestInfo) {
    snap.last_solve_request = lastSolveRequestInfo;
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
            ...(dealSnapshot.preset_name
              ? { preset_name: dealSnapshot.preset_name }
              : {}),
            ...(dealSnapshot.hand_label
              ? { hand_label: dealSnapshot.hand_label }
              : {}),
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
  const evId = window.Sentry.captureException(errorObj, { tags, extra: extraData });
  if (typeof evId === 'string' && evId) {
    lastSentryErrorEventId = evId;
  }
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
  const fullFeedbackMessage =
    (dealSnapshot && dealSnapshot.hand_label &&
     !feedbackMessage.includes('Preset / Hand:'))
      ? `${feedbackMessage}\n\n[Preset / Hand: ${dealSnapshot.hand_label}]`
          .slice(0, 4096)
      : feedbackMessage;
  const feedbackPayload = {
    event_id: feedbackEventId,
    timestamp: nowIso,
    level: 'info',
    platform: 'javascript',
    environment,
    user: userObj,
    tags: {
      preset: dealSnapshot ? (dealSnapshot.preset || 'custom') : 'none',
      preset_name: dealSnapshot ? (dealSnapshot.preset_name || '') : '',
      hand_label: dealSnapshot ? (dealSnapshot.hand_label || '') : '',
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
        message: fullFeedbackMessage,
        url: window.location.href,
        ...(lastSentryErrorEventId ? { associated_event_id: lastSentryErrorEventId } : {})
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
    const nHand = inputNorth ? inputNorth.value.trim() : '';
    const eHand = inputEast ? inputEast.value.trim() : '';
    const sHand = inputSouth ? inputSouth.value.trim() : '';
    const wHand = inputWest ? inputWest.value.trim() : '';
    const uHand = inputUnknownCards ? inputUnknownCards.value.trim() : '';

    const handInfo = getActivePresetAndHandInfo();
    const solveHandLabel =
      (lastSolveRequestInfo && lastSolveRequestInfo.hand_label)
        ? lastSolveRequestInfo.hand_label
        : handInfo.hand_label;

    const lines = [
      '[Status Error Report]',
      `Preset / Hand: ${solveHandLabel}`,
      `Error: ${errorMsg}`,
      `Backend: ${backendVal}`,
      `Deal Context: Trump=${trumpVal}, Leader=${leaderVal}, Target=${targetVal}, Iterations=${itersVal}`,
      `Hands: N="${nHand}" | E="${eHand}" | S="${sHand}" | W="${wHand}"${uHand ? ` | U="${uHand}"` : ''}`
    ];
    if (lastSolveRequestInfo) {
      lines.push(`Last Solve Kind: ${lastSolveRequestInfo.solve_kind || 'main'}`);
      if (lastSolveRequestInfo.trick_num) {
        lines.push(
          `Proposed Trick #${lastSolveRequestInfo.trick_num}: ${(lastSolveRequestInfo.proposed_cards || []).join(' ')}`
        );
      }
      if (lastSolveRequestInfo.payload) {
        lines.push(`Request Payload: ${JSON.stringify(lastSolveRequestInfo.payload)}`);
      }
    }
    const reportText = lines.join('\n');

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
