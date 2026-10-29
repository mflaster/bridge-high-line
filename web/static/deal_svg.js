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
    const rawLead = (data && data.best_lead) ? data.best_lead : '-';
    statLead.textContent = (typeof formatCardTokensForFrontend === 'function')
      ? formatCardTokensForFrontend(rawLead)
      : rawLead;
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
          const safeRank = String(c.rankStr || '').replace(/</g, '&lt;');
          pill.innerHTML = `${c.seatName}: <span class="${c.suitClass}">${c.suitSym}</span>${safeRank}`;
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
  const minProposedCards = activeProposalContext.initialCurCount || 0;
  if (activeProposalContext.proposedCards.length <= minProposedCards &&
      activeProposalContext.selectedTrickStates) {
    activeTrickCardStates = clonePlayStates(activeProposalContext.selectedTrickStates);
  } else {
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
  }
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
  } else if (normPat === 'X' || normPat === 'H') {
    if (actualRankToken === 'X' || actualRankToken === 'H' || 'AKQJT'.includes(normActual)) {
      return true;
    }
  } else if (normPat && normPat.startsWith('<') && normPat.length === 2) {
    const boundVal = rankOrderValue(normPat.charAt(1));
    const actVal = rankOrderValue(actualRankToken);
    if (boundVal > 0 && actVal > 0 && actVal < boundVal) {
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
    'A': 14, 'K': 13, 'Q': 12, 'J': 11, 'T': 10, 'H': 10, 'X': 10,
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
  // 2. If pattern is wildcard ('x', 'H'/'X', '<H', or '_'), match lowest valid card from the right
  if (normPat === 'x' || normPat === 'X' || normPat === 'H' || normPat === '_' || normPat.startsWith('<')) {
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
  // 3. If pattern is specific rank (e.g., 'K') and hand has 'x'/'H' wildcard
  for (let i = 0; i < rankTokens.length; i++) {
    if (usedMask[i]) continue;
    if (rankTokens[i] === 'x' || rankTokens[i] === 'X' || rankTokens[i] === 'H') {
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

  const minProposedCards = activeProposalContext ? (activeProposalContext.initialCurCount || 0) : 0;
  const isShowingSelectedTrickPreview = Boolean(
    activeProposalContext &&
    activeProposalContext.proposedCards.length <= minProposedCards &&
    activeProposalContext.selectedTrickStates
  );

  // Determine which suits are legal if this box is the active turn to play
  let legalSuits = null;
  if (playabilityInfo && playabilityInfo.isTurn) {
    const leadSuit = playabilityInfo.leadSuit;
    if (playabilityInfo.forceLeadSuit && leadSuit) {
      legalSuits = new Set([leadSuit]);
    } else if (!isUnknown && leadSuit) {
      const hasUnplayedLeadSuit = (suitTokenLists[leadSuit] || []).some(
        it => it.state === 'normal' || (isShowingSelectedTrickPreview && it.state === 'current')
      );
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
          (item.state === 'normal' ||
           (isShowingSelectedTrickPreview && item.state === 'current' && !item.addedFromUnknown))
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
    const st = (activeProposalContext && activeProposalContext.playedStates)
      ? activeProposalContext.playedStates[seatKey]
      : (activeTrickCardStates ? activeTrickCardStates[seatKey] : null);
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

  renderSvgAuction();
  updateDealBadge();
}

function appendSvgCallSpans(textEl, callCode) {
  const norm = normalizeCallToken(callCode) || callCode || '';
  if (norm === 'P') {
    textEl.textContent = 'P';
    return;
  }
  if (norm === 'X' || norm === 'XX') {
    textEl.textContent = norm;
    return;
  }
  if (norm.length >= 2 && norm[0] >= '1' && norm[0] <= '7') {
    const lvl = norm[0];
    const strain = norm.slice(1);
    const suitMap = {
      S: { sym: '♠', cls: 'card-suit-s' },
      H: { sym: '♥', cls: 'card-suit-h' },
      D: { sym: '♦', cls: 'card-suit-d' },
      C: { sym: '♣', cls: 'card-suit-c' }
    };
    if (suitMap[strain]) {
      const lvlSpan = document.createElementNS('http://www.w3.org/2000/svg', 'tspan');
      lvlSpan.textContent = lvl;
      textEl.appendChild(lvlSpan);
      const symSpan = document.createElementNS('http://www.w3.org/2000/svg', 'tspan');
      symSpan.setAttribute('class', `card-suit-label ${suitMap[strain].cls}`);
      symSpan.setAttribute('font-size', '15');
      symSpan.textContent = suitMap[strain].sym;
      textEl.appendChild(symSpan);
      return;
    }
  }
  textEl.textContent = norm;
}

function renderSvgAuction() {
  const auctionGroup = document.getElementById('svgAuctionHand');
  const cardsAuctionGroup = document.getElementById('cardsAuctionGroup');
  const titleAuction = document.getElementById('titleAuction');
  if (!auctionGroup || !cardsAuctionGroup) return;

  cardsAuctionGroup.innerHTML = '';

  const order = ['N', 'E', 'S', 'W'];
  let hasBiddingReqs = false;
  ['E', 'W'].forEach(seatKey => {
    if (importedBiddingReqs && Array.isArray(importedBiddingReqs[seatKey]) &&
        importedBiddingReqs[seatKey].length > 0) {
      hasBiddingReqs = true;
    }
  });

  let dealerIdx = -1;
  let callsByTurn = [];

  if (Array.isArray(loadedFirstRoundCalls) && loadedFirstRoundCalls.length > 0) {
    const declIdx = Math.max(0, order.indexOf(currentDeclarerSeat || 'N'));
    const firstOrigIdx = Math.max(0, order.indexOf(loadedFirstRoundCalls[0].seat || 'N'));
    dealerIdx = (firstOrigIdx - declIdx + 4) % 4;
    callsByTurn = loadedFirstRoundCalls.slice(0, 4).map((c, i) => {
      const seatIdx = (dealerIdx + i) % 4;
      const seat = order[seatIdx];
      const normBid = normalizeCallToken(c.bid) || c.bid;
      return { seat, seatIdx, bid: normBid };
    });
  } else if (hasBiddingReqs) {
    // Reconstruct the implied first-round auction from importedBiddingReqs
    let bestSeq = null;
    let bestDealerIdx = 0;
    ['E', 'W'].forEach(seatKey => {
      const seatIdx = order.indexOf(seatKey);
      const reqs = (importedBiddingReqs && importedBiddingReqs[seatKey]) || [];
      reqs.forEach(r => {
        if (!r || !r.bid) return;
        const afterToks = (r.after || '').trim().split(/\s+/).filter(Boolean);
        const seq = afterToks.concat([r.bid]);
        const dIdx = (seatIdx - afterToks.length + 40) % 4;
        if (!bestSeq || seq.length > bestSeq.length) {
          bestSeq = seq;
          bestDealerIdx = dIdx;
        }
      });
    });
    if (bestSeq && bestSeq.length > 0) {
      dealerIdx = bestDealerIdx;
      callsByTurn = bestSeq.slice(0, 4).map((rawBid, i) => {
        const seatIdx = (dealerIdx + i) % 4;
        const seat = order[seatIdx];
        const normBid = normalizeCallToken(rawBid) || rawBid;
        return { seat, seatIdx, bid: normBid };
      });
    }
  }

  if (callsByTurn.length === 0 || dealerIdx < 0) {
    auctionGroup.classList.add('hidden');
    return;
  }

  auctionGroup.classList.remove('hidden');
  if (titleAuction) {
    titleAuction.textContent = 'AUCTION';
  }

  const colX = [16, 39, 62, 85];

  // 4-column header row starting from Dealer
  for (let i = 0; i < 4; i++) {
    const seatIdx = (dealerIdx + i) % 4;
    const seat = order[seatIdx];
    const hdr = document.createElementNS('http://www.w3.org/2000/svg', 'text');
    hdr.setAttribute('x', String(colX[i]));
    hdr.setAttribute('y', '35');
    hdr.setAttribute('text-anchor', 'middle');
    hdr.setAttribute('font-size', '11');
    hdr.setAttribute('font-weight', '700');
    hdr.setAttribute('fill', '#94a3b8');
    hdr.textContent = seat;
    cardsAuctionGroup.appendChild(hdr);
  }

  const divider = document.createElementNS('http://www.w3.org/2000/svg', 'line');
  divider.setAttribute('x1', '7');
  divider.setAttribute('y1', '42');
  divider.setAttribute('x2', '95');
  divider.setAttribute('y2', '42');
  divider.setAttribute('stroke', '#2a5a43');
  divider.setAttribute('stroke-width', '1');
  cardsAuctionGroup.appendChild(divider);

  // First-round call cells
  for (let i = 0; i < 4; i++) {
    const callEntry = callsByTurn[i];
    const cell = document.createElementNS('http://www.w3.org/2000/svg', 'text');
    cell.setAttribute('x', String(colX[i]));
    cell.setAttribute('y', '61');
    cell.setAttribute('text-anchor', 'middle');
    cell.setAttribute('font-size', '12.5');
    cell.setAttribute('font-weight', '700');
    cell.setAttribute('fill', callEntry ? '#f8fafc' : '#64748b');
    if (callEntry) {
      appendSvgCallSpans(cell, callEntry.bid);
    } else {
      cell.textContent = '—';
    }
    cardsAuctionGroup.appendChild(cell);
  }
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

