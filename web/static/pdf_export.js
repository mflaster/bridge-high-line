// =========================================================================
// Standalone Multi-Page PDF Analysis Report Generator
// =========================================================================

const saveAnalysisBtn = document.getElementById('saveAnalysisBtn');

const PDF_PAGE_W = 1700;
const PDF_PAGE_H = 2200;
const PDF_PT_W = 612;
const PDF_PT_H = 792;

const PDF_SUIT_COLORS = {
  S: '#60a5fa',
  H: '#f87171',
  D: '#fb923c',
  C: '#4ade80',
  NT: '#e2e8f0'
};

const PDF_SUIT_SYMBOLS = {
  S: '♠',
  H: '♥',
  D: '♦',
  C: '♣',
  SPADES: '♠',
  HEARTS: '♥',
  DIAMONDS: '♦',
  CLUBS: '♣',
  NOTRUMP: 'NT',
  NT: 'NT'
};

function updateSaveAnalysisButtonState() {
  if (!saveAnalysisBtn) return;
  const hasData = Boolean(
    currentSolveData &&
    currentSolveData.tree &&
    Object.keys(currentSolveData.tree).length > 0
  );
  saveAnalysisBtn.disabled = !hasData;
}

function pdfRoundRect(ctx, x, y, w, h, r) {
  const rad = Math.min(r, w / 2, h / 2);
  ctx.beginPath();
  ctx.moveTo(x + rad, y);
  ctx.lineTo(x + w - rad, y);
  ctx.quadraticCurveTo(x + w, y, x + w, y + rad);
  ctx.lineTo(x + w, y + h - rad);
  ctx.quadraticCurveTo(x + w, y + h, x + w - rad, y + h);
  ctx.lineTo(x + rad, y + h);
  ctx.quadraticCurveTo(x, y + h, x, y + h - rad);
  ctx.lineTo(x, y + rad);
  ctx.quadraticCurveTo(x, y, x + rad, y);
  ctx.closePath();
}

function pdfFillStrokeCard(ctx, x, y, w, h, r, fillStyle, strokeStyle, lw = 2) {
  pdfRoundRect(ctx, x, y, w, h, r);
  if (fillStyle) {
    ctx.fillStyle = fillStyle;
    ctx.fill();
  }
  if (strokeStyle) {
    ctx.lineWidth = lw;
    ctx.strokeStyle = strokeStyle;
    ctx.stroke();
  }
}

function pdfWrapTextLines(ctx, text, maxWidth) {
  const words = String(text || '').split(/\s+/).filter(Boolean);
  if (words.length === 0) return [''];
  const lines = [];
  let cur = words[0];
  for (let i = 1; i < words.length; i++) {
    const test = `${cur} ${words[i]}`;
    if (ctx.measureText(test).width <= maxWidth) {
      cur = test;
    } else {
      lines.push(cur);
      cur = words[i];
    }
  }
  lines.push(cur);
  return lines;
}

function pdfTruncateToWidth(ctx, text, maxWidth) {
  const s = String(text || '');
  if (ctx.measureText(s).width <= maxWidth) return s;
  let lo = 0;
  let hi = s.length;
  while (lo < hi) {
    const mid = Math.ceil((lo + hi) / 2);
    const cand = s.slice(0, mid) + '...';
    if (ctx.measureText(cand).width <= maxWidth) {
      lo = mid;
    } else {
      hi = mid - 1;
    }
  }
  return s.slice(0, Math.max(1, lo)) + '...';
}

function createPdfPageCanvas() {
  const canvas = document.createElement('canvas');
  canvas.width = PDF_PAGE_W;
  canvas.height = PDF_PAGE_H;
  const ctx = canvas.getContext('2d');
  ctx.fillStyle = '#0b1120';
  ctx.fillRect(0, 0, PDF_PAGE_W, PDF_PAGE_H);
  return { canvas, ctx };
}

function drawPdfPageHeader(ctx, titleText, subText, rightMetaLines) {
  pdfFillStrokeCard(
    ctx, 48, 36, PDF_PAGE_W - 96, 104, 14, '#111c33', '#283856', 2
  );
  ctx.fillStyle = '#60a5fa';
  ctx.font = 'bold 34px Inter, system-ui, -apple-system, sans-serif';
  ctx.textAlign = 'left';
  ctx.textBaseline = 'middle';
  ctx.fillText('♠', 74, 72);

  ctx.fillStyle = '#f8fafc';
  ctx.font = 'bold 28px Inter, system-ui, -apple-system, sans-serif';
  ctx.fillText(
    pdfTruncateToWidth(ctx, titleText, 1020),
    112,
    71
  );

  ctx.fillStyle = '#94a3b8';
  ctx.font = '500 19px Inter, system-ui, -apple-system, sans-serif';
  ctx.fillText(
    pdfTruncateToWidth(ctx, subText, 1050),
    112,
    110
  );

  if (Array.isArray(rightMetaLines) && rightMetaLines.length > 0) {
    ctx.textAlign = 'right';
    ctx.font = '600 18px Inter, system-ui, -apple-system, sans-serif';
    ctx.fillStyle = '#cbd5e1';
    rightMetaLines.forEach((line, idx) => {
      ctx.fillText(line, PDF_PAGE_W - 72, 68 + idx * 28);
    });
    ctx.textAlign = 'left';
  }
}

function drawPdfPageFooter(ctx, pageIdx, totalPages, handLabel) {
  const y = PDF_PAGE_H - 36;
  ctx.strokeStyle = '#1e293b';
  ctx.lineWidth = 1.5;
  ctx.beginPath();
  ctx.moveTo(48, y - 22);
  ctx.lineTo(PDF_PAGE_W - 48, y - 22);
  ctx.stroke();

  ctx.font = '500 16px Inter, system-ui, -apple-system, sans-serif';
  ctx.fillStyle = '#64748b';
  ctx.textAlign = 'left';
  ctx.textBaseline = 'middle';
  ctx.fillText(
    `Bridge Deal Solver — ${handLabel || 'Analysis Report'}`,
    52,
    y
  );

  ctx.textAlign = 'right';
  ctx.fillText(`Page ${pageIdx} of ${totalPages}`, PDF_PAGE_W - 52, y);
  ctx.textAlign = 'left';
}

function drawSuitHoldingRow(
  ctx, x, y, suitKey, holdingStr, maxW, fontSize = 21
) {
  const sym = PDF_SUIT_SYMBOLS[suitKey] || suitKey;
  const col = PDF_SUIT_COLORS[suitKey] || '#e2e8f0';
  ctx.font = `bold ${fontSize}px Inter, system-ui, sans-serif`;
  ctx.fillStyle = col;
  ctx.textAlign = 'left';
  ctx.textBaseline = 'middle';
  ctx.fillText(sym, x, y);

  ctx.font = `600 ${fontSize}px "JetBrains Mono", monospace, sans-serif`;
  const cleanStr = (holdingStr && holdingStr.trim()) ? holdingStr.trim() : '-';
  ctx.fillStyle = cleanStr === '-' ? '#64748b' : '#f1f5f9';
  ctx.fillText(
    pdfTruncateToWidth(ctx, cleanStr, maxW - (fontSize + 8)),
    x + fontSize + 6,
    y
  );
}

function drawHandBoxOnPdf(ctx, x, y, w, h, title, handStr, isUnknown = false) {
  const bg = isUnknown ? '#132a20' : '#0f241a';
  const border = isUnknown ? '#34d399' : '#2d5e46';
  pdfFillStrokeCard(ctx, x, y, w, h, 12, bg, border, 2);

  ctx.font = 'bold 17px Inter, system-ui, sans-serif';
  ctx.fillStyle = isUnknown ? '#6ee7b7' : '#94a3b8';
  ctx.textAlign = 'left';
  ctx.textBaseline = 'middle';
  ctx.fillText(title, x + 16, y + 24);

  const suits = parseHandToSuits(handStr || '');
  const suitOrder = ['S', 'H', 'D', 'C'];
  const startY = y + 54;
  const rowGap = (h - 62) / 3.8;
  suitOrder.forEach((sKey, idx) => {
    drawSuitHoldingRow(
      ctx,
      x + 16,
      startY + idx * rowGap,
      sKey,
      suits[sKey] || '-',
      w - 28,
      20
    );
  });
}

function drawDealConfigurationPanelOnPdf(ctx, x, y, w, h) {
  pdfFillStrokeCard(ctx, x, y, w, h, 16, '#1a382b', '#2d5e46', 2.5);

  // Panel Header bar inside Deal Config
  ctx.font = 'bold 22px Inter, system-ui, sans-serif';
  ctx.fillStyle = '#ecfdf5';
  ctx.textAlign = 'left';
  ctx.textBaseline = 'middle';
  ctx.fillText('Deal Configuration', x + 24, y + 34);

  const endingBadgeEl = document.getElementById('dealEndingBadge');
  const badgeText = (endingBadgeEl && endingBadgeEl.textContent)
    ? endingBadgeEl.textContent.trim()
    : 'Full deal';
  ctx.font = '600 16px Inter, system-ui, sans-serif';
  const bw = ctx.measureText(badgeText).width + 24;
  pdfFillStrokeCard(
    ctx, x + w - bw - 22, y + 18, bw, 32, 16, '#0f241a', '#34d399', 1.5
  );
  ctx.fillStyle = '#6ee7b7';
  ctx.textAlign = 'center';
  ctx.fillText(badgeText, x + w - bw / 2 - 22, y + 34);
  ctx.textAlign = 'left';

  // Compass Rose (upper left)
  const cx = x + 115;
  const cy = y + 148;
  ctx.beginPath();
  ctx.arc(cx, cy, 30, 0, Math.PI * 2);
  ctx.fillStyle = '#0b1b13';
  ctx.fill();
  ctx.lineWidth = 2;
  ctx.strokeStyle = '#2d5e46';
  ctx.stroke();

  ctx.beginPath();
  ctx.arc(cx, cy, 4, 0, Math.PI * 2);
  ctx.fillStyle = '#6ee7b7';
  ctx.fill();

  ctx.font = 'bold 17px Inter, system-ui, sans-serif';
  ctx.fillStyle = '#a7f3d0';
  ctx.textAlign = 'center';
  ctx.textBaseline = 'middle';
  ctx.fillText('N', cx, cy - 42);
  ctx.fillText('S', cx, cy + 44);
  ctx.fillText('W', cx - 44, cy + 1);
  ctx.fillText('E', cx + 44, cy + 1);
  ctx.textAlign = 'left';

  const nStr = inputNorth ? inputNorth.value : '';
  const eStr = inputEast ? inputEast.value : '';
  const sStr = inputSouth ? inputSouth.value : '';
  const wStr = inputWest ? inputWest.value : '';
  const uStr = inputUnknownCards ? inputUnknownCards.value.trim() : '';

  const boxW = 246;
  const boxH = 184;
  const colLeft = x + 22;
  const colMid = x + Math.round((w - boxW) / 2);
  const colRight = x + w - boxW - 22;
  const rowTop = y + 64;
  const rowMid = y + 266;
  const rowBot = y + 468;

  // North Hand
  const hasDeclSeat = (
    typeof currentDeclarerSeat === 'string' && currentDeclarerSeat
  );
  const declSeat = hasDeclSeat ? currentDeclarerSeat : 'N';
  const nTitle = declSeat !== 'N'
    ? `NORTH (Decl: ${declSeat})`
    : 'NORTH (Declarer)';
  const sTitle = 'SOUTH (Dummy)';
  drawHandBoxOnPdf(ctx, colMid, rowTop, boxW, boxH, nTitle, nStr, false);

  // Unknown Cards (Upper Right)
  if (uStr) {
    drawHandBoxOnPdf(
      ctx, colRight, rowTop, boxW, boxH, 'UNKNOWN CARDS', uStr, true
    );
  }

  // West Hand
  drawHandBoxOnPdf(ctx, colLeft, rowMid, boxW, boxH, 'WEST', wStr, false);

  // Center Box (Trump / Leader / Tricks / Rand deals)
  pdfFillStrokeCard(
    ctx, colMid, rowMid, boxW, boxH, 12, '#0f241a', '#2d5e46', 2
  );
  const trumpVal = (currentSolveData && currentSolveData.trump)
    ? currentSolveData.trump
    : (trumpSelect ? trumpSelect.value : 'SPADES');
  const leaderVal = (currentSolveData && currentSolveData.leader)
    ? currentSolveData.leader
    : (leaderSelect ? leaderSelect.value : 'NORTH');
  const targetVal = targetTricks ? targetTricks.value : '-';
  const itersVal = (currentSolveData && currentSolveData.total_boards)
    ? String(currentSolveData.total_boards)
    : (numIters ? numIters.value : '100');

  const centerRows = [
    ['Trump:', `${PDF_SUIT_SYMBOLS[trumpVal] || ''} ${trumpVal}`],
    ['Leader:', leaderVal],
    ['Target:', `${targetVal} tricks`],
    ['Deals:', `${itersVal}`]
  ];
  const initCur = (typeof getActiveInitialCurTrick === 'function')
    ? getActiveInitialCurTrick()
    : [];
  if (initCur && initCur.length > 0) {
    centerRows.push(['Led:', initCur.join(' ')]);
  }
  const cRowGap = (boxH - 36) / centerRows.length;
  centerRows.forEach((pair, idx) => {
    const ry = rowMid + 26 + idx * cRowGap;
    ctx.font = '600 17px Inter, system-ui, sans-serif';
    ctx.fillStyle = '#94a3b8';
    ctx.textAlign = 'left';
    ctx.fillText(pair[0], colMid + 18, ry);

    ctx.font = 'bold 18px Inter, system-ui, sans-serif';
    ctx.fillStyle = '#f8fafc';
    ctx.textAlign = 'right';
    ctx.fillText(pair[1], colMid + boxW - 18, ry);
  });
  ctx.textAlign = 'left';

  // East Hand
  drawHandBoxOnPdf(ctx, colRight, rowMid, boxW, boxH, 'EAST', eStr, false);

  // South Hand
  drawHandBoxOnPdf(ctx, colMid, rowBot, boxW, boxH, sTitle, sStr, false);

  // Lower-left: Auction or Hand Requirements if present
  const hasAuction = Boolean(
    typeof importedAuctionData === 'object' &&
    importedAuctionData &&
    Array.isArray(importedAuctionData.calls) &&
    importedAuctionData.calls.length > 0
  );
  if (hasAuction) {
    pdfFillStrokeCard(
      ctx, colLeft, rowBot, boxW, boxH, 12, '#0f241a', '#2d5e46', 2
    );
    ctx.font = 'bold 16px Inter, system-ui, sans-serif';
    ctx.fillStyle = '#a7f3d0';
    ctx.fillText(
      `AUCTION (Dlr: ${importedAuctionData.dealer || 'N'})`,
      colLeft + 14,
      rowBot + 24
    );
    const seats = ['W', 'N', 'E', 'S'];
    seats.forEach((s, i) => {
      ctx.font = 'bold 15px Inter, system-ui, sans-serif';
      ctx.fillStyle = '#6ee7b7';
      ctx.fillText(s, colLeft + 20 + i * 54, rowBot + 50);
    });
    const dIdx = Math.max(0, seats.indexOf(importedAuctionData.dealer || 'N'));
    importedAuctionData.calls.slice(0, 16).forEach((call, idx) => {
      const slot = dIdx + idx;
      const colIdx = slot % 4;
      const rIdx = Math.floor(slot / 4);
      if (rIdx < 4) {
        ctx.font = '600 15px "JetBrains Mono", monospace';
        ctx.fillStyle = '#f1f5f9';
        ctx.fillText(
          String(call),
          colLeft + 18 + colIdx * 54,
          rowBot + 76 + rIdx * 26
        );
      }
    });
  }
}

function drawOverallStatsPanelOnPdf(ctx, x, y, w, h) {
  pdfFillStrokeCard(ctx, x, y, w, h, 16, '#111c33', '#283856', 2);

  ctx.font = 'bold 22px Inter, system-ui, sans-serif';
  ctx.fillStyle = '#f8fafc';
  ctx.textAlign = 'left';
  ctx.textBaseline = 'middle';
  ctx.fillText('Overall Solve Results', x + 24, y + 34);

  const data = currentSolveData || {};
  const total = data.total_boards || 1;
  const success = data.success_boards !== undefined
    ? data.success_boards
    : (data.covered_boards || 0);
  const failure = data.failure_boards !== undefined
    ? data.failure_boards
    : Math.max(0, total - success);
  const succPct = ((success / total) * 100).toFixed(1);
  const failPct = ((failure / total) * 100).toFixed(1);
  const tricksWon = data.tricks_won !== undefined
    ? String(data.tricks_won)
    : '-';
  const initCur = (typeof getActiveInitialCurTrick === 'function')
    ? getActiveInitialCurTrick()
    : [];
  const leadLabel = (initCur && initCur.length > 0)
    ? 'Card Led'
    : ((statLeadLabel && statLeadLabel.textContent)
        ? statLeadLabel.textContent
        : 'Optimal Lead');
  const rawStatLead = (statLead && statLead.textContent)
    ? statLead.textContent.trim()
    : '';
  const leadVal = (initCur && initCur.length > 0)
    ? initCur.join(' ')
    : ((rawStatLead && rawStatLead !== '-')
        ? rawStatLead
        : (data.best_lead || '-'));

  const cardW = Math.floor((w - 64) / 2);
  const cardH = 112;
  const cX1 = x + 24;
  const cX2 = x + 24 + cardW + 16;
  const rY1 = y + 64;
  const rY2 = rY1 + cardH + 16;

  // 1. Tricks Won
  pdfFillStrokeCard(ctx, cX1, rY1, cardW, cardH, 12, '#1e293b', '#334155', 2);
  ctx.font = '600 16px Inter, system-ui, sans-serif';
  ctx.fillStyle = '#94a3b8';
  ctx.fillText('TRICKS WON (N/S)', cX1 + 20, rY1 + 30);
  ctx.font = 'bold 34px Inter, system-ui, sans-serif';
  ctx.fillStyle = '#f8fafc';
  ctx.fillText(tricksWon, cX1 + 20, rY1 + 74);

  // 2. Optimal Lead
  pdfFillStrokeCard(ctx, cX2, rY1, cardW, cardH, 12, '#1e293b', '#334155', 2);
  ctx.font = '600 16px Inter, system-ui, sans-serif';
  ctx.fillStyle = '#94a3b8';
  ctx.fillText(leadLabel.toUpperCase(), cX2 + 20, rY1 + 30);
  ctx.font = 'bold 34px Inter, system-ui, sans-serif';
  ctx.fillStyle = '#38bdf8';
  ctx.fillText(leadVal, cX2 + 20, rY1 + 74);

  // 3. Success Rate
  pdfFillStrokeCard(ctx, cX1, rY2, cardW, cardH, 12, '#064e3b', '#10b981', 2);
  ctx.font = '600 16px Inter, system-ui, sans-serif';
  ctx.fillStyle = '#a7f3d0';
  ctx.fillText('SUCCESS RATE', cX1 + 20, rY2 + 30);
  ctx.font = 'bold 30px Inter, system-ui, sans-serif';
  ctx.fillStyle = '#ecfdf5';
  ctx.fillText(`${succPct}% (${success}/${total})`, cX1 + 20, rY2 + 74);

  // 4. Failure Rate
  pdfFillStrokeCard(ctx, cX2, rY2, cardW, cardH, 12, '#450a0a', '#ef4444', 2);
  ctx.font = '600 16px Inter, system-ui, sans-serif';
  ctx.fillStyle = '#fecaca';
  ctx.fillText('FAILURE RATE', cX2 + 20, rY2 + 30);
  ctx.font = 'bold 30px Inter, system-ui, sans-serif';
  ctx.fillStyle = '#fef2f2';
  ctx.fillText(`${failPct}% (${failure}/${total})`, cX2 + 20, rY2 + 74);

  let curY = rY2 + cardH + 22;

  // Actual Deal (#0) box if available
  if (data.actual_hand) {
    const ah = data.actual_hand;
    const makes = Boolean(ah.succeeded);
    const boxBg = makes ? '#06392c' : '#3b0d0d';
    const boxBorder = makes ? '#10b981' : '#f87171';
    const ahH = 148;
    pdfFillStrokeCard(ctx, cX1, curY, w - 48, ahH, 12, boxBg, boxBorder, 2);

    ctx.font = 'bold 18px Inter, system-ui, sans-serif';
    ctx.fillStyle = makes ? '#6ee7b7' : '#fca5a5';
    const tricksPart = ah.ns_tricks !== undefined
      ? ` (${ah.ns_tricks} tricks won by N/S)`
      : '';
    ctx.fillText(
      `Actual Deal (#0): ${makes ? 'MAKES' : 'FAILS'}${tricksPart}`,
      cX1 + 18,
      curY + 26
    );

    const seats = [
      ['E:', ah.E || ''],
      ['W:', ah.W || '']
    ];
    seats.forEach((pair, idx) => {
      const sy = curY + 62 + idx * 42;
      ctx.font = 'bold 17px Inter, system-ui, sans-serif';
      ctx.fillStyle = '#e2e8f0';
      ctx.fillText(pair[0], cX1 + 18, sy);
      const suits = parseHandToSuits(pair[1]);
      let sx = cX1 + 54;
      ['S', 'H', 'D', 'C'].forEach((sKey) => {
        drawSuitHoldingRow(ctx, sx, sy, sKey, suits[sKey] || '-', 150, 17);
        sx += 155;
      });
    });
    curY += ahH + 18;
  }

  // Equivalent / Alternative Lines list
  const selectableLines = (typeof buildSelectableLinesList === 'function')
    ? buildSelectableLinesList()
    : [];
  const activeIdx = (typeof data.active_line_index === 'number')
    ? data.active_line_index
    : 0;
  const remH = (y + h) - curY - 20;
  if (remH >= 90) {
    pdfFillStrokeCard(
      ctx, cX1, curY, w - 48, remH, 12, '#0f172a', '#334155', 1.5
    );
    ctx.font = 'bold 17px Inter, system-ui, sans-serif';
    ctx.fillStyle = '#cbd5e1';
    const headerTitle = selectableLines.length > 1
      ? `Evaluated Lines of Play (${selectableLines.length} lines — ★ = Active)`
      : 'Evaluated Line of Play';
    ctx.fillText(headerTitle, cX1 + 18, curY + 26);

    if (selectableLines.length > 0) {
      const maxRows = Math.max(1, Math.floor((remH - 46) / 30));
      selectableLines.slice(0, maxRows).forEach((item, idx) => {
        const ly = curY + 56 + idx * 30;
        const isCurrent = item.index === activeIdx;
        ctx.font = isCurrent
          ? 'bold 16px "JetBrains Mono", monospace'
          : '500 15px "JetBrains Mono", monospace';
        ctx.fillStyle = isCurrent ? '#38bdf8' : '#94a3b8';
        const prefix = isCurrent ? '★ ' : '  ';
        ctx.fillText(
          pdfTruncateToWidth(ctx, `${prefix}${item.label}`, w - 84),
          cX1 + 16,
          ly
        );
      });
    } else {
      ctx.font = '500 16px "JetBrains Mono", monospace';
      ctx.fillStyle = '#e2e8f0';
      const seqStr = (
        data.sequence || data.best_lead || 'Optimal double-dummy line'
      );
      ctx.fillText(
        pdfTruncateToWidth(ctx, seqStr, w - 84),
        cX1 + 18,
        curY + 58
      );
    }
  }
}

function drawInvariantsAndDistributionSectionOnPdf(ctx, x, y, w, h) {
  pdfFillStrokeCard(ctx, x, y, w, h, 16, '#111c33', '#283856', 2);

  ctx.font = 'bold 22px Inter, system-ui, sans-serif';
  ctx.fillStyle = '#f8fafc';
  ctx.textAlign = 'left';
  ctx.textBaseline = 'middle';
  ctx.fillText(
    'Invariant Rules & Defender Distribution Summary',
    x + 24,
    y + 34
  );

  const data = currentSolveData || {};
  const succDist = data.distribution_summary || null;
  const failDist = data.failure_distribution_summary || null;
  const succRules = (succDist && Array.isArray(succDist.invariant_rules))
    ? succDist.invariant_rules
    : [];
  const failRules = (failDist && Array.isArray(failDist.invariant_rules))
    ? failDist.invariant_rules
    : [];

  const colW = Math.floor((w - 64) / 2);
  const leftX = x + 24;
  const rightX = x + 24 + colW + 16;
  const rulesBoxY = y + 62;
  const rulesBoxH = 430;

  const drawRulesBox = (bx, by, bw, bh, title, rules, isSuccess) => {
    const bg = isSuccess ? '#0a261e' : '#2a1215';
    const border = isSuccess ? '#059669' : '#dc2626';
    const titleCol = isSuccess ? '#6ee7b7' : '#fca5a5';
    pdfFillStrokeCard(ctx, bx, by, bw, bh, 12, bg, border, 1.5);

    ctx.font = 'bold 18px Inter, system-ui, sans-serif';
    ctx.fillStyle = titleCol;
    ctx.fillText(`${title} (${rules.length})`, bx + 18, by + 28);

    if (rules.length === 0) {
      ctx.font = 'italic 16px Inter, system-ui, sans-serif';
      ctx.fillStyle = '#94a3b8';
      const emptyMsg = isSuccess
        ? 'No single-condition invariant rules for 100% of success deals.'
        : 'No single-condition invariant rules for 100% of failure deals.';
      ctx.fillText(
        emptyMsg,
        bx + 18,
        by + 66
      );
      return;
    }

    ctx.font = '500 16px Inter, system-ui, sans-serif';
    ctx.fillStyle = '#f1f5f9';
    let ry = by + 60;
    const maxY = by + bh - 18;
    for (let i = 0; i < rules.length; i++) {
      const fmtRule = String(rules[i])
        .replace(/\bhand 1\b/gi, 'Hand 1 (East)')
        .replace(/\bhand [23]\b/gi, 'Hand 2 (West)');
      const wrapped = pdfWrapTextLines(ctx, `• ${fmtRule}`, bw - 36);
      if (ry + wrapped.length * 24 > maxY) {
        ctx.fillStyle = '#94a3b8';
        ctx.fillText(
          `... +${rules.length - i} more invariant rule(s)`,
          bx + 18,
          ry
        );
        break;
      }
      wrapped.forEach((line, lIdx) => {
        ctx.fillText(lIdx === 0 ? line : `  ${line}`, bx + 18, ry);
        ry += 24;
      });
      ry += 4;
    }
  };

  drawRulesBox(
    leftX,
    rulesBoxY,
    colW,
    rulesBoxH,
    'Success Deals — Invariant Rules',
    succRules,
    true
  );
  drawRulesBox(
    rightX,
    rulesBoxY,
    colW,
    rulesBoxH,
    'Failure Deals — Invariant Rules',
    failRules,
    false
  );

  // Lower tables: Suit Length Bounds (left) & Honor Possession Stats (right)
  const tableY = rulesBoxY + rulesBoxH + 20;
  const tableH = (y + h) - tableY - 20;
  if (tableH < 180) return;

  // Left Table: Suit Length Bounds (Success Deals & Failure Deals)
  pdfFillStrokeCard(
    ctx, leftX, tableY, colW, tableH, 12, '#0f172a', '#334155', 1.5
  );
  ctx.font = 'bold 18px Inter, system-ui, sans-serif';
  ctx.fillStyle = '#e2e8f0';
  ctx.fillText('Defender Suit Length Bounds', leftX + 18, tableY + 28);

  const drawBoundsSubtable = (distObj, label, startY) => {
    ctx.font = 'bold 15px Inter, system-ui, sans-serif';
    ctx.fillStyle = '#94a3b8';
    ctx.fillText(label, leftX + 18, startY);
    ctx.fillText('East (Hand 1)', leftX + 230, startY);
    ctx.fillText('West (Hand 2)', leftX + 410, startY);
    ctx.fillText('Combined', leftX + 585, startY);

    const bounds = (distObj && distObj.suit_bounds) ? distObj.suit_bounds : {};
    const suits = ['Spades', 'Hearts', 'Diamonds', 'Clubs'];
    const suitCodes = ['S', 'H', 'D', 'C'];
    suits.forEach((sName, idx) => {
      const sy = startY + 28 + idx * 28;
      const sCode = suitCodes[idx];
      const b = bounds[sCode] || bounds[sName] || null;
      ctx.font = 'bold 16px Inter, system-ui, sans-serif';
      ctx.fillStyle = PDF_SUIT_COLORS[sCode];
      ctx.fillText(`${PDF_SUIT_SYMBOLS[sCode]} ${sName}`, leftX + 18, sy);

      ctx.font = '500 16px "JetBrains Mono", monospace';
      ctx.fillStyle = '#f1f5f9';
      if (b) {
        const min1 = b.min_hand1 !== undefined ? b.min_hand1 : b.hand1_min;
        const max1 = b.max_hand1 !== undefined ? b.max_hand1 : b.hand1_max;
        const min2 = b.min_hand2 !== undefined ? b.min_hand2 : b.hand2_min;
        const max2 = b.max_hand2 !== undefined ? b.max_hand2 : b.hand2_max;
        const minC = b.min_combined !== undefined
          ? b.min_combined
          : b.combined_min;
        const maxC = b.max_combined !== undefined
          ? b.max_combined
          : b.combined_max;
        ctx.fillText(`${min1} – ${max1}`, leftX + 230, sy);
        ctx.fillText(`${min2} – ${max2}`, leftX + 410, sy);
        ctx.fillText(`${minC} – ${maxC}`, leftX + 585, sy);
      } else {
        ctx.fillStyle = '#64748b';
        ctx.fillText('-', leftX + 230, sy);
        ctx.fillText('-', leftX + 410, sy);
        ctx.fillText('-', leftX + 585, sy);
      }
    });
  };

  drawBoundsSubtable(succDist, 'Success Deals:', tableY + 60);
  if (tableH >= 340) {
    drawBoundsSubtable(failDist, 'Failure Deals:', tableY + 210);
  }

  // Right Table: Honor Card Possession Stats (Success Deals)
  pdfFillStrokeCard(
    ctx, rightX, tableY, colW, tableH, 12, '#0f172a', '#334155', 1.5
  );
  ctx.font = 'bold 18px Inter, system-ui, sans-serif';
  ctx.fillStyle = '#e2e8f0';
  ctx.fillText(
    'Honor Card Possession (Success Deals)',
    rightX + 18,
    tableY + 28
  );

  ctx.font = 'bold 15px Inter, system-ui, sans-serif';
  ctx.fillStyle = '#94a3b8';
  const hdrY = tableY + 60;
  ctx.fillText('Card', rightX + 18, hdrY);
  ctx.fillText('East %', rightX + 190, hdrY);
  ctx.fillText('West %', rightX + 330, hdrY);
  ctx.fillText('Both %', rightX + 470, hdrY);
  ctx.fillText('Neither %', rightX + 600, hdrY);

  const honors = (succDist && succDist.honor_stats) ? succDist.honor_stats : {};
  const distTotal = (succDist && succDist.total_deals)
    ? succDist.total_deals
    : 1;
  const honorRows = [];
  const suitMeta = [
    ['Spades', 'S'],
    ['Hearts', 'H'],
    ['Diamonds', 'D'],
    ['Clubs', 'C']
  ];
  suitMeta.forEach(([sName, sCode]) => {
    const sHonors = honors[sCode] || honors[sName];
    if (!sHonors) return;
    Object.keys(sHonors).forEach((rankKey) => {
      const st = sHonors[rankKey];
      if (!st) return;
      const h1Pct = st.hand1_count !== undefined
        ? (st.hand1_count / distTotal) * 100
        : (st.hand1_pct || 0);
      const h2Pct = st.hand2_count !== undefined
        ? (st.hand2_count / distTotal) * 100
        : (st.hand2_pct || 0);
      const bothPct = st.both_count !== undefined
        ? (st.both_count / distTotal) * 100
        : (st.both_pct || 0);
      const neitherPct = st.neither_count !== undefined
        ? (st.neither_count / distTotal) * 100
        : (st.neither_pct || 0);
      if (neitherPct < 99.95 || h1Pct > 0 || h2Pct > 0) {
        honorRows.push({
          sCode,
          label: `${PDF_SUIT_SYMBOLS[sCode]}${rankKey}`,
          h1: `${h1Pct.toFixed(1)}%`,
          h2: `${h2Pct.toFixed(1)}%`,
          both: `${bothPct.toFixed(1)}%`,
          neither: `${neitherPct.toFixed(1)}%`
        });
      }
    });
  });

  if (honorRows.length === 0) {
    ctx.font = 'italic 16px Inter, system-ui, sans-serif';
    ctx.fillStyle = '#64748b';
    ctx.fillText(
      'No variable defender honors in pool.',
      rightX + 18,
      hdrY + 36
    );
  } else {
    const maxHonorRows = Math.max(1, Math.floor((tableH - 86) / 26));
    honorRows.slice(0, maxHonorRows).forEach((hr, idx) => {
      const hy = hdrY + 28 + idx * 26;
      ctx.font = 'bold 15px Inter, system-ui, sans-serif';
      ctx.fillStyle = PDF_SUIT_COLORS[hr.sCode];
      ctx.fillText(hr.label, rightX + 18, hy);

      ctx.font = '500 15px "JetBrains Mono", monospace';
      ctx.fillStyle = '#f1f5f9';
      ctx.fillText(hr.h1, rightX + 190, hy);
      ctx.fillText(hr.h2, rightX + 330, hy);
      ctx.fillText(hr.both, rightX + 470, hy);
      ctx.fillText(hr.neither, rightX + 600, hy);
    });
  }
}

// =========================================================================
// Snapshot Selection & Path / Inspector Computation
// =========================================================================

function computeInspectorDataForNode(targetNode) {
  const path = [];
  let curr = targetNode;
  while (curr) {
    path.unshift(curr);
    curr = curr._parent;
  }

  let initialLeader = getSeatIndex(leaderSelect ? leaderSelect.value : 'NORTH');
  if (currentSolveData && currentSolveData.leader) {
    initialLeader = getSeatIndex(currentSolveData.leader);
  }
  const trumpCode = getTrumpCode();
  let currentLeader = initialLeader;
  const trickRows = [];
  const pathConditions = [];

  const seatPat = '([0-3]|north|east|south|west|n|e|s|w)';
  const winRegex = new RegExp(
    `(?:when|if)\\s+${seatPat}\\s+wins` +
    `(?:\\s+(?:the\\s+)?([A-Z0-9_,\\(\\)\\s]+))?`,
    'i'
  );
  const playRegex = new RegExp(
    `(?:when|if)\\s+${seatPat}\\s+plays\\s+([A-Z0-9_]+)`,
    'i'
  );
  const discardRegex = new RegExp(
    `(?:when|if)\\s+${seatPat}\\s+discards`,
    'i'
  );

  const initCur = (typeof getActiveInitialCurTrick === 'function')
    ? getActiveInitialCurTrick()
    : [];

  path.forEach((pathNode, stepIdx) => {
    if (pathNode.condition) {
      let cText = pathNode.condition;
      if (cText.startsWith('after ')) cText = cText.substring(6);
      pathConditions.push({
        trickNum: trickRows.length + 1,
        condition: cText,
        node: pathNode
      });
    }

    const rawTrick = extractTrickString(pathNode);
    if (!rawTrick) return;

    const tokens = tokenizeTrick(rawTrick);
    if (tokens.length === 0) return;

    if (typeof pathNode.leader === 'number' &&
        pathNode.leader >= 0 &&
        pathNode.leader <= 3) {
      currentLeader = pathNode.leader;
    }

    const parsedCards = [];
    let leadSuit = null;
    tokens.forEach((tok, i) => {
      const effectiveTok = (
        trickRows.length === 0 &&
        i < initCur.length &&
        (tok.includes('_') || tok.includes('x') || tok.includes('X'))
      )
        ? initCur[i]
        : tok;
      const cardObj = parseSingleCardToken(effectiveTok, leadSuit, trumpCode);
      if (i === 0) leadSuit = cardObj.suit;
      cardObj.seatIdx = (currentLeader + i) % 4;
      cardObj.seatName = getSeatName(cardObj.seatIdx);
      parsedCards.push(cardObj);
    });

    let explicitWinnerSeat = null;
    let explicitWinnerCard = null;
    let explicitPlayedSeat = null;
    let explicitPlayedCard = null;

    if (stepIdx + 1 < path.length) {
      const nextNode = path[stepIdx + 1];
      if (nextNode) {
        if (typeof nextNode.branch_winner === 'number' &&
            nextNode.branch_winner >= 0 &&
            nextNode.branch_winner <= 3) {
          explicitWinnerSeat = nextNode.branch_winner;
        }
        if (nextNode.condition) {
          const cond = nextNode.condition;
          const mWin = cond.match(winRegex);
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
          const mPlay = cond.match(playRegex);
          if (mPlay) {
            explicitPlayedSeat = getSeatIndex(mPlay[1]);
            explicitPlayedCard = mPlay[2].trim();
          } else {
            const mDiscard = cond.match(discardRegex);
            if (mDiscard) {
              explicitPlayedSeat = getSeatIndex(mDiscard[1]);
              explicitPlayedCard = 'R_';
            }
          }
        }
      }
    }

    if (explicitWinnerSeat === null &&
        typeof pathNode.winner === 'number' &&
        pathNode.winner >= 0 &&
        pathNode.winner <= 3) {
      explicitWinnerSeat = pathNode.winner;
    }

    if (explicitPlayedSeat !== null && explicitPlayedCard) {
      const playedIdx = parsedCards.findIndex(
        c => c.seatIdx === explicitPlayedSeat
      );
      if (playedIdx !== -1) {
        const updatedCard = parseSingleCardToken(
          explicitPlayedCard, leadSuit, trumpCode
        );
        updatedCard.seatIdx = explicitPlayedSeat;
        updatedCard.seatName = getSeatName(explicitPlayedSeat);
        parsedCards[playedIdx] = updatedCard;
      }
    }

    let winningCardIdx = -1;
    if (explicitWinnerSeat !== null) {
      const foundIdx = parsedCards.findIndex(
        c => c.seatIdx === explicitWinnerSeat
      );
      if (foundIdx !== -1) {
        winningCardIdx = foundIdx;
        if (explicitWinnerCard) {
          const existingRaw = (parsedCards[foundIdx].raw || '').replace(
            /[\(\)]/g, ''
          );
          const isWildcardSlot = !existingRaw ||
            existingRaw.includes('_') ||
            existingRaw.includes('x') ||
            existingRaw.includes('X');
          if (isWildcardSlot ||
              existingRaw.toUpperCase() === explicitWinnerCard.toUpperCase()) {
            const updatedCard = parseSingleCardToken(
              explicitWinnerCard, leadSuit, trumpCode
            );
            updatedCard.seatIdx = explicitWinnerSeat;
            updatedCard.seatName = getSeatName(explicitWinnerSeat);
            parsedCards[foundIdx] = updatedCard;
          }
        }
      }
    }

    if (winningCardIdx === -1) {
      let bestIdx = 0;
      let bestIsTrump = (
        trumpCode !== 'NT' && parsedCards[0].suit === trumpCode
      );
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
    let condBeforeTrick = '';
    if (pathNode.condition) {
      condBeforeTrick = pathNode.condition.startsWith('after ')
        ? pathNode.condition.substring(6)
        : pathNode.condition;
    }

    trickRows.push({
      trickNum: trickRows.length + 1,
      leadSeatIdx: currentLeader,
      leadSeatName,
      cards: parsedCards,
      winningCardIdx,
      winnerSeat: winnerCard ? winnerCard.seatName : '',
      condition: condBeforeTrick,
      node: pathNode
    });

    if (winnerCard) {
      currentLeader = winnerCard.seatIdx;
    }
  });

  return { path, trickRows, pathConditions };
}

function selectSnapshotNodesForPdf(rootNode) {
  if (!rootNode) return [];
  attachParentPointers(rootNode, null);

  const successLeaves = [];
  const failureLeaves = [];

  const walk = (n) => {
    if (!n) return;
    if (!n.children || n.children.length === 0) {
      const boardLen = Array.isArray(n.boards) ? n.boards.length : 0;
      if (n.failure) {
        const cnt = n.failure_boards !== undefined
          ? n.failure_boards
          : boardLen;
        if (cnt > 0 || (Array.isArray(n.boards) && n.boards.includes(-1))) {
          failureLeaves.push({ node: n, count: cnt });
        }
      } else {
        const cnt = n.success_boards !== undefined
          ? n.success_boards
          : (n.covered_boards || boardLen);
        if (cnt > 0 || (Array.isArray(n.boards) && n.boards.includes(-1))) {
          successLeaves.push({ node: n, count: cnt });
        }
      }
      return;
    }
    n.children.forEach(walk);
  };
  walk(rootNode);

  successLeaves.sort((a, b) => b.count - a.count);
  failureLeaves.sort((a, b) => b.count - a.count);

  const snapshots = [];
  const usedNodes = new Set();

  // If user currently has a non-root node selected in the UI, include it first
  if (currentCenteredNode && currentCenteredNode !== rootNode) {
    usedNodes.add(currentCenteredNode);
    const isFail = Boolean(currentCenteredNode.failure);
    const cnt = isFail
      ? (currentCenteredNode.failure_boards || 0)
      : (currentCenteredNode.success_boards !== undefined
          ? currentCenteredNode.success_boards
          : (currentCenteredNode.covered_boards || 0));
    snapshots.push({
      node: currentCenteredNode,
      kind: isFail ? 'failure' : 'success',
      titlePrefix: 'Currently Selected Branch in UI',
      count: cnt
    });
  }

  // Up to 5 top successful leaf branches
  let succRank = 0;
  for (const item of successLeaves) {
    if (succRank >= 5) break;
    succRank++;
    if (usedNodes.has(item.node)) continue;
    usedNodes.add(item.node);
    const rankTitle = succRank === 1
      ? 'Most Common Successful Branch (#1)'
      : `Successful Branch #${succRank}`;
    snapshots.push({
      node: item.node,
      kind: 'success',
      titlePrefix: rankTitle,
      count: item.count
    });
  }

  // Up to 3 top failure leaf branches
  let failRank = 0;
  for (const item of failureLeaves) {
    if (failRank >= 3) break;
    failRank++;
    if (usedNodes.has(item.node)) continue;
    usedNodes.add(item.node);
    const rankTitle = failRank === 1
      ? 'Most Common Failure Branch (#1)'
      : `Failure Branch #${failRank}`;
    snapshots.push({
      node: item.node,
      kind: 'failure',
      titlePrefix: rankTitle,
      count: item.count
    });
  }

  if (snapshots.length === 0) {
    snapshots.push({
      node: rootNode,
      kind: 'success',
      titlePrefix: 'Decision Tree Overview',
      count: rootNode.success_boards || rootNode.covered_boards || 0
    });
  }

  return snapshots;
}

// =========================================================================
// Offscreen Decision Tree + Line Inspector Snapshot Renderer
// =========================================================================

function layoutTreeForPdfSnapshot(rootNode, pathNodeSet, innerW, innerH) {
  const nodeWidth = 204;
  const nodeHeight = 88;

  const isExpandedInSnapshot = (node) => {
    if (!node || !node.children || node.children.length === 0) return false;
    return pathNodeSet.has(node);
  };

  // Clear temporary layout properties recursively
  const clearPdfProps = (n) => {
    if (!n) return;
    delete n._pdfChildren;
    delete n._pdfHorizontalChild;
    delete n._pdfTierOffset;
    if (n.children) n.children.forEach(clearPdfProps);
  };
  clearPdfProps(rootNode);

  // Step 1: Build raw topological levels along the active expanded path
  const rawLevels = [];
  let branchStepCount = 0;

  const buildRawLevels = (startNode) => {
    rawLevels.push({
      nodes: [startNode],
      activeSlot: 0
    });
    let curr = startNode;
    while (curr && isExpandedInSnapshot(curr)) {
      const rawChildren = curr.children || [];
      const activeOrigIdx = rawChildren.findIndex(c => pathNodeSet.has(c));
      let ordered = rawChildren.slice();
      let activeSlot = 0;

      if (activeOrigIdx >= 0 && rawChildren.length >= 2) {
        const activeChild = rawChildren[activeOrigIdx];
        const others = rawChildren.filter((_, idx) => idx !== activeOrigIdx);
        const targetSlot = (branchStepCount % 2 === 0)
          ? Math.floor((rawChildren.length - 1) / 2)
          : Math.ceil((rawChildren.length - 1) / 2);
        branchStepCount++;
        ordered = [
          ...others.slice(0, targetSlot),
          activeChild,
          ...others.slice(targetSlot)
        ];
        activeSlot = targetSlot;
      } else if (activeOrigIdx >= 0) {
        activeSlot = activeOrigIdx;
      }

      curr._pdfChildren = ordered;
      rawLevels.push({
        nodes: ordered,
        activeSlot
      });
      curr = ordered[activeSlot] || null;
    }
  };

  buildRawLevels(rootNode);

  // Step 2: Compact consecutive unconditional single-child levels (d >= 1)
  // into horizontal serpentine chain rows of up to 3 nodes so long forced
  // sequences flow across the width of the panel instead of making a tall
  // skinny pole.
  const rows = [];
  let chainDir = 1; // 1 = left-to-right, -1 = right-to-left
  let d = 0;
  while (d < rawLevels.length) {
    const lv = rawLevels[d];
    const canStartChain =
      d >= 1 &&
      lv.nodes.length === 1 &&
      !lv.nodes[0].condition &&
      d + 1 < rawLevels.length &&
      rawLevels[d + 1].nodes.length === 1 &&
      !rawLevels[d + 1].nodes[0].condition;

    if (canStartChain) {
      const chainNodes = [lv.nodes[0]];
      let look = d + 1;
      while (
        look < rawLevels.length &&
        chainNodes.length < 3 &&
        rawLevels[look].nodes.length === 1 &&
        !rawLevels[look].nodes[0].condition
      ) {
        const prevNode = chainNodes[chainNodes.length - 1];
        const nextNode = rawLevels[look].nodes[0];
        prevNode._pdfHorizontalChild = nextNode;
        chainNodes.push(nextNode);
        look++;
      }

      const displayNodes = (chainDir === 1)
        ? chainNodes.slice()
        : chainNodes.slice().reverse();
      const entryNode = chainNodes[0];
      const exitNode = chainNodes[chainNodes.length - 1];
      rows.push({
        kind: 'chain',
        nodes: displayNodes,
        entrySlot: displayNodes.indexOf(entryNode),
        activeSlot: displayNodes.indexOf(exitNode),
        chainDir
      });
      chainDir = -chainDir;
      d = look;
    } else if (lv.nodes.length >= 5) {
      // Split 5+ child fan-outs into 2 staggered tiers:
      // Outer 2 branches in upper tier, inner (k - 2) branches in lower tier
      const all = lv.nodes;
      const k = all.length;
      const activeNode = all[lv.activeSlot];
      let outerLeft = all[0];
      let outerRight = all[k - 1];
      let innerNodes = all.slice(1, k - 1);
      if (outerLeft === activeNode) {
        outerLeft = all[1];
        innerNodes = [all[0], ...all.slice(2, k - 1)];
      } else if (outerRight === activeNode) {
        outerRight = all[k - 2];
        innerNodes = [...all.slice(1, k - 2), all[k - 1]];
      }
      rows.push({
        kind: 'split_fanout',
        nodes: all,
        outerLeft,
        outerRight,
        innerNodes,
        entrySlot: lv.activeSlot,
        activeSlot: lv.activeSlot
      });
      d++;
    } else {
      rows.push({
        kind: 'normal',
        nodes: lv.nodes,
        entrySlot: lv.activeSlot,
        activeSlot: lv.activeSlot
      });
      d++;
    }
  }

  const R = rows.length;
  const relX = [];
  const rowW = [];

  for (let r = 0; r < R; r++) {
    const row = rows[r];
    if (row.kind === 'chain') {
      const k = row.nodes.length;
      const gapX = 38;
      const coords = [];
      for (let i = 0; i < k; i++) {
        coords.push(i * (nodeWidth + gapX));
      }
      relX.push(coords);
      rowW.push(k * nodeWidth + Math.max(0, k - 1) * gapX);
    } else if (row.kind === 'split_fanout') {
      const innerK = row.innerNodes.length;
      const gapX = 20;
      const totalSpanW =
        Math.max(innerK, 4) * nodeWidth +
        Math.max(3, innerK - 1) * gapX;
      const innerTotalW =
        innerK * nodeWidth + Math.max(0, innerK - 1) * gapX;
      const innerStartX = (totalSpanW - innerTotalW) / 2;

      const coords = new Array(row.nodes.length).fill(0);
      row.nodes.forEach((n, idx) => {
        if (n === row.outerLeft) {
          coords[idx] = 0;
          n._pdfTierOffset = 0;
        } else if (n === row.outerRight) {
          coords[idx] = totalSpanW - nodeWidth;
          n._pdfTierOffset = 0;
        } else {
          const inIdx = row.innerNodes.indexOf(n);
          coords[idx] = innerStartX + inIdx * (nodeWidth + gapX);
          n._pdfTierOffset = nodeHeight + 58;
        }
      });
      relX.push(coords);
      rowW.push(totalSpanW);
    } else {
      const k = row.nodes.length;
      const gapX = (k >= 4) ? 18 : 24;
      const coords = [];
      for (let i = 0; i < k; i++) {
        coords.push(i * (nodeWidth + gapX));
      }
      relX.push(coords);
      rowW.push(k * nodeWidth + Math.max(0, k - 1) * gapX);
    }
  }

  const refW = Math.max(...rowW, nodeWidth * 3 + 48);
  const rowLeftX = new Array(R).fill(0);
  const clamp = (val, lo, hi) => Math.max(lo, Math.min(hi, val));

  // Initial top-down horizontal placement
  rowLeftX[0] = (refW - rowW[0]) / 2;
  for (let r = 1; r < R; r++) {
    const prevSlot = rows[r - 1].activeSlot;
    const parentCenterX =
      rowLeftX[r - 1] + relX[r - 1][prevSlot] + nodeWidth / 2;
    const row = rows[r];
    let desiredLeft;
    if (row.kind === 'chain') {
      // Align entry node of chain near parentCenterX while keeping row centered
      const entryRelCenter = relX[r][row.entrySlot] + nodeWidth / 2;
      const idealFromParent = parentCenterX - entryRelCenter;
      const idealCentered = (refW - rowW[r]) / 2;
      desiredLeft = 0.55 * idealFromParent + 0.45 * idealCentered;
    } else if (row.nodes.length === 1) {
      desiredLeft = parentCenterX - nodeWidth / 2;
    } else {
      const rowAnchor = 0.65 * parentCenterX + 0.35 * (refW / 2);
      desiredLeft = rowAnchor - rowW[r] / 2;
    }
    rowLeftX[r] = clamp(desiredLeft, 0, Math.max(0, refW - rowW[r]));
  }

  // Smooth 1-node normal rows between their parent above and child below
  for (let iter = 0; iter < 4; iter++) {
    for (let r = R - 2; r >= 0; r--) {
      if (rows[r].kind !== 'normal' || rows[r].nodes.length !== 1) continue;
      const aboveCenterX = (r > 0)
        ? (
          rowLeftX[r - 1] +
          relX[r - 1][rows[r - 1].activeSlot] +
          nodeWidth / 2
        )
        : (
          rowLeftX[r + 1] +
          relX[r + 1][rows[r + 1].entrySlot] +
          nodeWidth / 2
        );
      const nextEntryX =
        rowLeftX[r + 1] + relX[r + 1][rows[r + 1].entrySlot] + nodeWidth / 2;
      const nextRowCenterX = rowLeftX[r + 1] + rowW[r + 1] / 2;
      const belowCenterX = (rows[r + 1].nodes.length === 1)
        ? nextEntryX
        : (0.5 * nextRowCenterX + 0.5 * nextEntryX);
      const targetCenter = 0.5 * (aboveCenterX + belowCenterX);
      rowLeftX[r] = clamp(
        targetCenter - nodeWidth / 2,
        0,
        Math.max(0, refW - rowW[r])
      );
    }
  }

  // Assign X coordinates and normalize minX to 0
  let minX = Infinity;
  let maxX = -Infinity;
  for (let r = 0; r < R; r++) {
    const nodes = rows[r].nodes;
    for (let i = 0; i < nodes.length; i++) {
      const n = nodes[i];
      n._pdfDepth = r;
      n._pdfX = rowLeftX[r] + relX[r][i];
      if (n._pdfX < minX) minX = n._pdfX;
      if (n._pdfX + nodeWidth > maxX) maxX = n._pdfX + nodeWidth;
    }
  }
  if (Number.isFinite(minX) && minX !== 0) {
    for (let r = 0; r < R; r++) {
      for (const n of rows[r].nodes) {
        n._pdfX -= minX;
      }
    }
    maxX -= minX;
    minX = 0;
  }

  const treeW = Math.max(nodeWidth, maxX - minX);

  // Compute row heights and dynamic vertical gaps to fill the panel height
  const rowHeights = rows.map(row => (
    row.kind === 'split_fanout' ? (nodeHeight * 2 + 58) : nodeHeight
  ));
  const baseGaps = [];
  for (let r = 0; r < R - 1; r++) {
    const nextNodes = rows[r + 1].nodes;
    const hasCond = nextNodes.some(n => Boolean(n.condition));
    baseGaps.push(
      (rows[r + 1].kind !== 'split_fanout' && !hasCond) ? 34 : 66
    );
  }
  const totalNodeH = rowHeights.reduce((a, b) => a + b, 0);
  const minTreeH = totalNodeH + baseGaps.reduce((a, b) => a + b, 0);

  const availW = Math.max(300, (innerW || 960) - 20);
  const availH = Math.max(400, (innerH || 1880) - 24);
  const scaleX = availW / treeW;
  const targetScale = Math.min(1.55, Math.max(0.55, scaleX));
  const availTreeH = availH / targetScale;

  const finalGaps = baseGaps.slice();
  if (R > 1) {
    if (availTreeH > minTreeH) {
      const extraPerGap = Math.min(58, (availTreeH - minTreeH) / (R - 1));
      for (let r = 0; r < R - 1; r++) {
        finalGaps[r] += extraPerGap;
      }
    } else if (availTreeH < minTreeH) {
      const ratio = Math.max(
        0.82,
        (availTreeH - totalNodeH) / Math.max(1, minTreeH - totalNodeH)
      );
      for (let r = 0; r < R - 1; r++) {
        finalGaps[r] = Math.max(24, baseGaps[r] * ratio);
      }
    }
  }

  let curY = 0;
  for (let r = 0; r < R; r++) {
    for (const n of rows[r].nodes) {
      n._pdfY = curY + (n._pdfTierOffset || 0);
    }
    if (r < R - 1) {
      curY += rowHeights[r] + finalGaps[r];
    } else {
      curY += rowHeights[r];
    }
  }
  const treeH = curY;

  return {
    nodeWidth,
    nodeHeight,
    treeW,
    treeH,
    isExpandedInSnapshot
  };
}

function drawDecisionTreeSnapshotOnPdf(
  ctx, x, y, w, h, rootNode, targetNode, pathNodeSet
) {
  pdfFillStrokeCard(ctx, x, y, w, h, 16, '#0f172a', '#283856', 2);

  // Panel Header
  ctx.font = 'bold 20px Inter, system-ui, sans-serif';
  ctx.fillStyle = '#f8fafc';
  ctx.textAlign = 'left';
  ctx.textBaseline = 'middle';
  ctx.fillText('Decision Tree Panel (Active Path Expanded)', x + 22, y + 32);

  // Legend in upper right of tree box
  const legendItems = [
    ['#1e293b', 'Unreached'],
    ['#064e3b', 'Makes'],
    ['#450a0a', 'Fails']
  ];
  let lx = x + w - 314;
  legendItems.forEach(([col, lbl]) => {
    pdfFillStrokeCard(ctx, lx, y + 21, 18, 18, 4, col, '#475569', 1);
    ctx.font = '500 14px Inter, system-ui, sans-serif';
    ctx.fillStyle = '#94a3b8';
    ctx.fillText(lbl, lx + 24, y + 31);
    lx += ctx.measureText(lbl).width + 42;
  });

  const innerX = x + 16;
  const innerY = y + 56;
  const innerW = w - 32;
  const innerH = h - 72;

  const {
    nodeWidth,
    nodeHeight,
    treeW,
    treeH,
    isExpandedInSnapshot
  } = layoutTreeForPdfSnapshot(rootNode, pathNodeSet, innerW, innerH);

  const scaleX = (innerW - 16) / treeW;
  const scaleY = (innerH - 16) / treeH;
  const scale = Math.min(1.55, Math.max(0.5, Math.min(scaleX, scaleY)));

  const offsetX = innerX + Math.max(8, (innerW - treeW * scale) / 2);
  const offsetY = innerY + Math.max(8, (innerH - treeH * scale) / 2);

  ctx.save();
  ctx.beginPath();
  ctx.rect(innerX, innerY, innerW, innerH);
  ctx.clip();
  ctx.translate(offsetX, offsetY);
  ctx.scale(scale, scale);

  const bezierPoint = (t, p0, p1, p2, p3) => {
    const u = 1 - t;
    return (
      u * u * u * p0 +
      3 * u * u * t * p1 +
      3 * u * t * t * p2 +
      t * t * t * p3
    );
  };

  // Pass 1: Draw Links & Edge Condition Labels
  const drawLinks = (node) => {
    if (!isExpandedInSnapshot(node)) return;
    const children = node._pdfChildren || node.children || [];
    const k = children.length;
    children.forEach((child, idx) => {
      const onPath = pathNodeSet.has(node) && pathNodeSet.has(child);

      // Horizontal link within a compacted unconditional chain row
      if (node._pdfHorizontalChild === child) {
        const goRight = child._pdfX >= node._pdfX;
        const startX = goRight ? (node._pdfX + nodeWidth) : node._pdfX;
        const endX = goRight ? child._pdfX : (child._pdfX + nodeWidth);
        const linkY = node._pdfY + nodeHeight / 2;

        ctx.beginPath();
        ctx.moveTo(startX, linkY);
        ctx.lineTo(endX, linkY);
        ctx.lineWidth = 3.6;
        ctx.strokeStyle = '#22d3ee';
        ctx.stroke();

        // Arrowhead at endX
        const dir = goRight ? 1 : -1;
        ctx.beginPath();
        ctx.moveTo(endX, linkY);
        ctx.lineTo(endX - dir * 10, linkY - 6);
        ctx.lineTo(endX - dir * 10, linkY + 6);
        ctx.closePath();
        ctx.fillStyle = '#22d3ee';
        ctx.fill();

        drawLinks(child);
        return;
      }

      const startX = node._pdfX + nodeWidth / 2;
      const startY = node._pdfY + nodeHeight;
      const endX = child._pdfX + nodeWidth / 2;
      const endY = child._pdfY;
      const midY = (startY + endY) / 2;

      ctx.beginPath();
      ctx.moveTo(startX, startY);
      ctx.bezierCurveTo(startX, midY, endX, midY, endX, endY);
      ctx.lineWidth = onPath ? 3.6 : 2.0;
      ctx.strokeStyle = onPath
        ? '#22d3ee'
        : (child.failure ? '#b91c1c' : '#4a6572');
      ctx.stroke();

      if (child.condition) {
        let condText = child.condition;
        if (condText.startsWith('after ')) condText = condText.substring(6);
        ctx.font = '600 12.5px Inter, system-ui, sans-serif';
        const textW = ctx.measureText(condText).width;
        const labelW = Math.max(60, textW + 16);
        const labelH = 22;

        // Place even/odd sibling condition pills in strictly disjoint vertical
        // bands directly above their target child card so adjacent pills and
        // upper-tier split_fanout cards never overlap.
        let pillY;
        let tForX;
        if (k === 1) {
          pillY = (startY + endY) / 2;
          tForX = 0.5;
        } else if (child._pdfTierOffset && child._pdfTierOffset > 0) {
          pillY = (idx % 2 === 0) ? (endY - 15) : (endY - 40);
          tForX = (idx % 2 === 0) ? 0.90 : 0.78;
        } else {
          pillY = (idx % 2 === 0) ? (endY - 15) : (endY - 40);
          tForX = (idx % 2 === 0) ? 0.78 : 0.46;
        }
        const pillX = bezierPoint(tForX, startX, startX, endX, endX);
        const clampedPillX = Math.max(
          labelW / 2 + 2,
          Math.min(treeW - labelW / 2 - 2, pillX)
        );

        pdfFillStrokeCard(
          ctx,
          clampedPillX - labelW / 2,
          pillY - labelH / 2,
          labelW,
          labelH,
          5,
          onPath ? '#083344' : (child.failure ? '#2a1215' : '#0f172a'),
          onPath ? '#22d3ee' : (child.failure ? '#b91c1c' : '#334155'),
          onPath ? 1.8 : 1.2
        );
        ctx.fillStyle = onPath
          ? '#67e8f9'
          : (child.failure ? '#fca5a5' : '#cbd5e1');
        ctx.textAlign = 'center';
        ctx.textBaseline = 'middle';
        ctx.fillText(condText, clampedPillX, pillY);
        ctx.textAlign = 'left';
      }

      drawLinks(child);
    });
  };

  // Pass 2: Draw Node Cards
  const drawNodes = (node) => {
    const nx = node._pdfX;
    const ny = node._pdfY;
    const isTarget = (node === targetNode);
    const onPath = pathNodeSet.has(node);

    const { dealFrac, failFrac } = getNodeFillProportions(node);
    const greyColor = '#1e293b';
    const successColor = '#064e3b';
    const failColor = '#450a0a';

    // Clip to rounded node card to paint proportional fill bars
    ctx.save();
    pdfRoundRect(ctx, nx, ny, nodeWidth, nodeHeight, 10);
    ctx.clip();

    ctx.fillStyle = greyColor;
    ctx.fillRect(nx, ny, nodeWidth, nodeHeight);

    if (dealFrac > 0) {
      const activeH = dealFrac * nodeHeight;
      const activeY = ny + nodeHeight - activeH;
      const succW = (1 - failFrac) * nodeWidth;
      const failW = nodeWidth - succW;
      if (succW > 0) {
        ctx.fillStyle = successColor;
        ctx.fillRect(nx, activeY, succW, activeH);
      }
      if (failW > 0) {
        ctx.fillStyle = failColor;
        ctx.fillRect(nx + succW, activeY, failW, activeH);
      }
    }
    ctx.restore();

    // Node border
    let strokeCol = '#3b82f6';
    if (node.type === 'condition') strokeCol = '#a855f7';
    if (node.type === 'outcome') {
      strokeCol = node.failure ? '#ef4444' : '#10b981';
    }
    if (onPath) strokeCol = '#22d3ee';
    if (isTarget) strokeCol = '#facc15';

    pdfRoundRect(ctx, nx, ny, nodeWidth, nodeHeight, 10);
    ctx.lineWidth = isTarget ? 4.0 : (onPath ? 3.0 : 1.8);
    ctx.strokeStyle = strokeCol;
    ctx.stroke();

    // Node text lines
    let rawTitle = node.label || node.trick || 'Node';
    if (rawTitle.startsWith('after ')) rawTitle = rawTitle.substring(6);
    ctx.font = 'bold 14.5px "JetBrains Mono", Inter, sans-serif';
    ctx.fillStyle = '#f8fafc';
    ctx.textAlign = 'left';
    ctx.textBaseline = 'middle';
    ctx.fillText(
      pdfTruncateToWidth(ctx, rawTitle, nodeWidth - 20),
      nx + 10,
      ny + 22
    );

    const { primary, secondary } = formatNodeCardDealLines(node);
    ctx.font = '600 13px Inter, system-ui, sans-serif';
    ctx.fillStyle = '#e2e8f0';
    ctx.fillText(primary, nx + 10, ny + 46);

    if (node.type === 'outcome' && node.ns_tricks !== undefined) {
      ctx.font = 'bold 13.5px Inter, system-ui, sans-serif';
      ctx.fillStyle = node.failure ? '#fca5a5' : '#6ee7b7';
      ctx.fillText(`N/S Won: ${node.ns_tricks}`, nx + 10, ny + 68);
    } else if (secondary) {
      ctx.font = '600 13px Inter, system-ui, sans-serif';
      ctx.fillStyle = '#fca5a5';
      ctx.fillText(secondary, nx + 10, ny + 68);
    }

    if (isExpandedInSnapshot(node)) {
      const children = node._pdfChildren || node.children || [];
      children.forEach(drawNodes);
    }
  };

  drawLinks(rootNode);
  drawNodes(rootNode);
  ctx.restore();
}

function drawLineInspectorSnapshotOnPdf(
  ctx, x, y, w, h, targetNode, inspectorData
) {
  pdfFillStrokeCard(ctx, x, y, w, h, 16, '#111c33', '#283856', 2);

  ctx.font = 'bold 20px Inter, system-ui, sans-serif';
  ctx.fillStyle = '#f8fafc';
  ctx.textAlign = 'left';
  ctx.textBaseline = 'middle';
  ctx.fillText('Line Inspector (Trick-by-Trick Play)', x + 22, y + 32);

  const { trickRows } = inspectorData;
  const isFail = Boolean(targetNode && targetNode.failure);
  const total = (targetNode && targetNode.total_boards) ||
    (currentSolveData && currentSolveData.total_boards) || 1;
  const branchCount = isFail
    ? (targetNode.failure_boards || 0)
    : (targetNode.success_boards !== undefined
        ? targetNode.success_boards
        : (targetNode.covered_boards || 0));
  const pct = ((branchCount / total) * 100).toFixed(1);

  // Node Summary Banner inside Inspector
  const bannerY = y + 56;
  const bannerH = 84;
  pdfFillStrokeCard(
    ctx,
    x + 18,
    bannerY,
    w - 36,
    bannerH,
    10,
    isFail ? '#3b0d0d' : '#06392c',
    isFail ? '#ef4444' : '#10b981',
    1.8
  );

  ctx.font = 'bold 17px Inter, system-ui, sans-serif';
  ctx.fillStyle = isFail ? '#fca5a5' : '#6ee7b7';
  const failTricksTxt = targetNode.ns_tricks !== undefined
    ? `${targetNode.ns_tricks} tricks`
    : 'Down';
  const makeTricksTxt = targetNode.ns_tricks !== undefined
    ? `${targetNode.ns_tricks} tricks`
    : 'Success';
  const statusStr = isFail
    ? `OUTCOME: CONTRACT FAILS (${failTricksTxt})`
    : `OUTCOME: CONTRACT MAKES (${makeTricksTxt})`;
  ctx.fillText(statusStr, x + 34, bannerY + 28);

  ctx.font = '500 16px Inter, system-ui, sans-serif';
  ctx.fillStyle = '#f1f5f9';
  const covTxt =
    `Branch Coverage: ${branchCount} / ${total} deals (${pct}%) · ` +
    `${trickRows.length} Trick(s)`;
  ctx.fillText(
    covTxt,
    x + 34,
    bannerY + 58
  );

  // Trick Table Column Headers
  let curY = bannerY + bannerH + 22;
  pdfFillStrokeCard(ctx, x + 18, curY, w - 36, 38, 8, '#1e293b', '#334155', 1);
  ctx.font = 'bold 15px Inter, system-ui, sans-serif';
  ctx.fillStyle = '#94a3b8';
  ctx.textAlign = 'center';
  const colTrickX = x + 48;
  const colLeadX = x + 98;
  const chipAreaStart = x + 136;
  const chipAreaSpan = w - 208;
  const seatStep = chipAreaSpan / 4;
  const seatCols = [
    chipAreaStart + seatStep * 0.5, // N
    chipAreaStart + seatStep * 1.5, // E
    chipAreaStart + seatStep * 2.5, // S
    chipAreaStart + seatStep * 3.5  // W
  ];
  const chipW = Math.min(94, Math.floor(seatStep - 10));
  const colWinX = x + w - 44;
  ctx.fillText('#', colTrickX, curY + 19);
  ctx.fillText('Lead', colLeadX, curY + 19);
  ctx.fillStyle = '#60a5fa';
  ctx.fillText('N', seatCols[0], curY + 19);
  ctx.fillStyle = '#cbd5e1';
  ctx.fillText('E', seatCols[1], curY + 19);
  ctx.fillStyle = '#60a5fa';
  ctx.fillText('S', seatCols[2], curY + 19);
  ctx.fillStyle = '#cbd5e1';
  ctx.fillText('W', seatCols[3], curY + 19);
  ctx.fillStyle = '#94a3b8';
  ctx.fillText('Win', colWinX, curY + 19);
  ctx.textAlign = 'left';

  curY += 46;

  trickRows.forEach((tr) => {
    // Draw branch condition banner above trick if present
    if (tr.condition) {
      pdfFillStrokeCard(
        ctx, x + 22, curY, w - 44, 28, 6, '#172554', '#3b82f6', 1
      );
      ctx.font = '600 14px Inter, system-ui, sans-serif';
      ctx.fillStyle = '#93c5fd';
      ctx.fillText(
        pdfTruncateToWidth(ctx, `↳ Condition: ${tr.condition}`, w - 68),
        x + 34,
        curY + 14
      );
      curY += 34;
    }

    const rowH = 44;
    pdfFillStrokeCard(
      ctx, x + 18, curY, w - 36, rowH, 8, '#0f172a', '#1e293b', 1.2
    );

    // Trick #
    ctx.font = 'bold 16px "JetBrains Mono", monospace';
    ctx.fillStyle = '#e2e8f0';
    ctx.textAlign = 'center';
    ctx.fillText(`#${tr.trickNum}`, colTrickX, curY + rowH / 2);

    // Leader seat
    ctx.font = '600 15px Inter, system-ui, sans-serif';
    ctx.fillStyle = '#94a3b8';
    ctx.fillText(`(${tr.leadSeatName})`, colLeadX, curY + rowH / 2);

    // 4 seat card chips (N=0, E=1, S=2, W=3)
    const isWinEW = (tr.winnerSeat === 'E' || tr.winnerSeat === 'W');
    [0, 1, 2, 3].forEach((seatIdx) => {
      const playIdx = tr.cards.findIndex(c => c.seatIdx === seatIdx);
      if (playIdx === -1) return;
      const c = tr.cards[playIdx];
      const isWinner = (playIdx === tr.winningCardIdx);
      const isLead = (seatIdx === tr.leadSeatIdx);
      const cx = seatCols[seatIdx];
      const chipH = 32;

      let chipBg = '#1e293b';
      let chipBorder = isLead ? '#38bdf8' : '#334155';
      if (isWinner) {
        chipBg = isWinEW ? '#450a0a' : '#064e3b';
        chipBorder = isWinEW ? '#ef4444' : '#10b981';
      }
      pdfFillStrokeCard(
        ctx,
        cx - chipW / 2,
        curY + (rowH - chipH) / 2,
        chipW,
        chipH,
        16,
        chipBg,
        chipBorder,
        isWinner || isLead ? 2 : 1
      );

      const sCol = (c.suit && PDF_SUIT_COLORS[c.suit])
        ? PDF_SUIT_COLORS[c.suit]
        : '#cbd5e1';
      const sym = c.suitSym || '';
      const rankTxt = String(c.rankStr || '');
      const starTxt = isWinner ? '★' : '';
      ctx.font = 'bold 15px "JetBrains Mono", monospace';
      ctx.fillStyle = sCol;
      ctx.fillText(
        `${sym}${rankTxt}${starTxt}`,
        cx,
        curY + rowH / 2
      );
    });

    // Winner column
    ctx.font = 'bold 16px Inter, system-ui, sans-serif';
    ctx.fillStyle = isWinEW ? '#fca5a5' : '#6ee7b7';
    ctx.fillText(tr.winnerSeat || '-', colWinX, curY + rowH / 2);
    ctx.textAlign = 'left';

    curY += rowH + 8;
  });

  // Representative Sample Deal(s) on this branch
  const boardIds = (targetNode && Array.isArray(targetNode.boards))
    ? targetNode.boards.filter(b => typeof b === 'number')
    : [];
  const remH = (y + h) - curY - 18;
  if (boardIds.length > 0 && remH >= 150) {
    curY += 8;
    ctx.font = 'bold 17px Inter, system-ui, sans-serif';
    ctx.fillStyle = '#cbd5e1';
    ctx.fillText(
      `Sample Deal(s) on This Branch (${boardIds.length} total)`,
      x + 22,
      curY + 14
    );
    curY += 32;

    const suitColW = Math.floor((w - 118) / 4);
    const maxDeals = Math.min(2, Math.floor((y + h - curY - 16) / 145));
    for (let i = 0; i < Math.min(boardIds.length, maxDeals); i++) {
      const bId = boardIds[i];
      const dealObj = (typeof findDealByBoardId === 'function')
        ? findDealByBoardId(bId)
        : null;
      const cardH = 136;
      pdfFillStrokeCard(
        ctx, x + 18, curY, w - 36, cardH, 10, '#0f172a', '#334155', 1.5
      );

      const dealTitle = bId === -1
        ? 'Actual Deal (#0)'
        : `Sample Hand #${bId + 1}`;
      ctx.font = 'bold 16px Inter, system-ui, sans-serif';
      ctx.fillStyle = '#38bdf8';
      ctx.fillText(dealTitle, x + 32, curY + 24);

      if (dealObj) {
        const seatList = [
          ['East:', dealObj.E || ''],
          ['West:', dealObj.W || '']
        ];
        seatList.forEach(([sLbl, sHand], sIdx) => {
          const sy = curY + 56 + sIdx * 32;
          ctx.font = 'bold 15px Inter, system-ui, sans-serif';
          ctx.fillStyle = '#94a3b8';
          ctx.fillText(sLbl, x + 32, sy);
          const suits = parseHandToSuits(sHand);
          let sx = x + 90;
          ['S', 'H', 'D', 'C'].forEach((sKey) => {
            drawSuitHoldingRow(
              ctx, sx, sy, sKey, suits[sKey] || '-', suitColW - 4, 15
            );
            sx += suitColW;
          });
        });
      }

      // Concrete tricks played on this board
      const concreteTricks = [];
      const bKey = String(bId);
      trickRows.forEach((tr) => {
        if (tr.node && tr.node.board_to_trick && tr.node.board_to_trick[bKey]) {
          const btStr = tr.node.board_to_trick[bKey];
          concreteTricks.push(`#${tr.trickNum}: ${btStr}`);
        }
      });
      if (concreteTricks.length > 0) {
        ctx.font = '500 13.5px "JetBrains Mono", monospace';
        ctx.fillStyle = '#cbd5e1';
        ctx.fillText(
          pdfTruncateToWidth(ctx, concreteTricks.join('  '), w - 68),
          x + 32,
          curY + 116
        );
      }

      curY += cardH + 10;
    }
  }
}

// =========================================================================
// Pure-JS Multi-Page PDF 1.4 Binary Encoder (DCTDecode / JPEG Streams)
// =========================================================================

function dataUrlToUint8Array(dataUrl) {
  const commaIdx = dataUrl.indexOf(',');
  const b64 = commaIdx >= 0 ? dataUrl.slice(commaIdx + 1) : dataUrl;
  const binStr = atob(b64);
  const bytes = new Uint8Array(binStr.length);
  for (let i = 0; i < binStr.length; i++) {
    bytes[i] = binStr.charCodeAt(i) & 0xff;
  }
  return bytes;
}

function buildPdfFromPageCanvases(canvases) {
  const enc = new TextEncoder();
  const chunks = [];
  let offset = 0;

  const pushStr = (s) => {
    const b = enc.encode(s);
    chunks.push(b);
    offset += b.length;
  };
  const pushBytes = (b) => {
    chunks.push(b);
    offset += b.length;
  };

  const numPages = canvases.length;
  const totalObjs = 2 + numPages * 3;
  const xrefOffsets = new Array(totalObjs + 1).fill(0);

  // PDF Header (with high-bit comment bytes for binary stream compatibility)
  pushStr('%PDF-1.4\n%\xFF\xFF\xFF\xFF\n');

  // Object 1: Catalog
  xrefOffsets[1] = offset;
  pushStr('1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n');

  // Object 2: Pages
  const kidsRefs = [];
  for (let i = 0; i < numPages; i++) {
    kidsRefs.push(`${3 + i * 3} 0 R`);
  }
  xrefOffsets[2] = offset;
  pushStr(
    `2 0 obj\n<< /Type /Pages /Kids [ ${kidsRefs.join(' ')} ] ` +
    `/Count ${numPages} >>\nendobj\n`
  );

  for (let i = 0; i < numPages; i++) {
    const canvas = canvases[i];
    const jpegBytes = dataUrlToUint8Array(
      canvas.toDataURL('image/jpeg', 0.92)
    );
    const pageObjId = 3 + i * 3;
    const contentObjId = 4 + i * 3;
    const imgObjId = 5 + i * 3;

    // Page object
    xrefOffsets[pageObjId] = offset;
    pushStr(
      `${pageObjId} 0 obj\n` +
      `<< /Type /Page /Parent 2 0 R /MediaBox [0 0 ${PDF_PT_W} ${PDF_PT_H}] ` +
      `/Resources << /XObject << /Im0 ${imgObjId} 0 R >> >> ` +
      `/Contents ${contentObjId} 0 R >>\nendobj\n`
    );

    // Content stream object
    const streamCmd = `q ${PDF_PT_W} 0 0 ${PDF_PT_H} 0 0 cm /Im0 Do Q\n`;
    xrefOffsets[contentObjId] = offset;
    pushStr(
      `${contentObjId} 0 obj\n<< /Length ${streamCmd.length} >>\nstream\n` +
      `${streamCmd}endstream\nendobj\n`
    );

    // Image XObject
    xrefOffsets[imgObjId] = offset;
    pushStr(
      `${imgObjId} 0 obj\n` +
      `<< /Type /XObject /Subtype /Image ` +
      `/Width ${canvas.width} /Height ${canvas.height} ` +
      `/ColorSpace /DeviceRGB /BitsPerComponent 8 /Filter /DCTDecode ` +
      `/Length ${jpegBytes.length} >>\nstream\n`
    );
    pushBytes(jpegBytes);
    pushStr('\nendstream\nendobj\n');
  }

  const xrefStart = offset;
  pushStr(`xref\n0 ${totalObjs + 1}\n`);
  pushStr('0000000000 65535 f \n');
  for (let objId = 1; objId <= totalObjs; objId++) {
    const offStr = String(xrefOffsets[objId]).padStart(10, '0');
    pushStr(`${offStr} 00000 n \n`);
  }

  pushStr(
    `trailer\n<< /Size ${totalObjs + 1} /Root 1 0 R >>\n` +
    `startxref\n${xrefStart}\n%%EOF\n`
  );

  const out = new Uint8Array(offset);
  let pos = 0;
  for (const c of chunks) {
    out.set(c, pos);
    pos += c.length;
  }
  return out;
}

// =========================================================================
// Main Entry Point: Generate & Download Standalone Analysis PDF
// =========================================================================

function generateAnalysisPdfPages() {
  if (!currentSolveData || !currentTreeData) return [];

  const handInfo = (typeof getActivePresetAndHandInfo === 'function')
    ? getActivePresetAndHandInfo()
    : { hand_label: 'Bridge Deal', preset: 'custom' };
  const handLabel = handInfo.hand_label || 'Bridge Deal';
  const nowStr = new Date().toLocaleString();

  const trumpVal = (currentSolveData && currentSolveData.trump)
    ? currentSolveData.trump
    : (trumpSelect ? trumpSelect.value : 'SPADES');
  const leaderVal = (currentSolveData && currentSolveData.leader)
    ? currentSolveData.leader
    : (leaderSelect ? leaderSelect.value : 'NORTH');
  const targetVal = targetTricks ? targetTricks.value : '-';
  const totalDeals = currentSolveData.total_boards || 1;

  const subHeader =
    `Trump: ${PDF_SUIT_SYMBOLS[trumpVal] || ''} ${trumpVal} · ` +
    `Leader: ${leaderVal} · Target: ${targetVal} tricks · ` +
    `Simulated Deals: ${totalDeals}`;

  const pages = [];

  // Page 1: Deal Configuration (Upper Left) + Overall Results (Upper Right) +
  // Invariant Rules & Distribution Tables (Lower Section)
  const p1 = createPdfPageCanvas();
  drawPdfPageHeader(
    p1.ctx,
    `Bridge Deal Analysis — ${handLabel}`,
    subHeader,
    [nowStr, 'Page 1: Deal & Summary']
  );
  drawDealConfigurationPanelOnPdf(p1.ctx, 48, 160, 786, 684);
  drawOverallStatsPanelOnPdf(p1.ctx, 854, 160, 798, 684);
  drawInvariantsAndDistributionSectionOnPdf(
    p1.ctx, 48, 864, PDF_PAGE_W - 96, 1256
  );
  pages.push(p1);

  // Pages 2..N: Decision Tree Panel + Line Inspector Snapshots
  const snapshots = selectSnapshotNodesForPdf(currentTreeData);
  snapshots.forEach((snap, idx) => {
    const p = createPdfPageCanvas();
    const inspectorData = computeInspectorDataForNode(snap.node);
    const pathNodeSet = new Set(inspectorData.path);

    const condList = inspectorData.pathConditions
      .map(pc => `[${pc.condition}]`)
      .join(' → ');
    const snapSub = condList
      ? `Path Conditions: ${condList}`
      : 'Unconditional main path from root';

    const pct = ((snap.count / totalDeals) * 100).toFixed(1);
    const snapStatus = snap.kind === 'failure' ? 'FAILS' : 'MAKES';
    drawPdfPageHeader(
      p.ctx,
      `Snapshot ${idx + 1}/${snapshots.length}: ${snap.titlePrefix}`,
      snapSub,
      [
        `${snapStatus}: ${snap.count}/${totalDeals} (${pct}%)`,
        handLabel
      ]
    );

    // Left: Decision Tree Panel (width 988 px)
    drawDecisionTreeSnapshotOnPdf(
      p.ctx,
      36,
      160,
      988,
      1960,
      currentTreeData,
      snap.node,
      pathNodeSet
    );

    // Right: Line Inspector Panel (width 624 px)
    drawLineInspectorSnapshotOnPdf(
      p.ctx,
      1040,
      160,
      624,
      1960,
      snap.node,
      inspectorData
    );

    pages.push(p);
  });

  // Stamp footer with total page count on every page
  const totalPages = pages.length;
  pages.forEach((p, idx) => {
    drawPdfPageFooter(p.ctx, idx + 1, totalPages, handLabel);
  });

  return pages.map(p => p.canvas);
}

async function saveAnalysisAsPdf() {
  if (!currentSolveData || !currentTreeData) return;
  const origLabel = saveAnalysisBtn ? saveAnalysisBtn.textContent : '';
  if (saveAnalysisBtn) {
    saveAnalysisBtn.disabled = true;
    saveAnalysisBtn.textContent = '⏳ Saving...';
  }
  try {
    const canvases = generateAnalysisPdfPages();
    if (canvases.length === 0) return;
    const pdfBytes = buildPdfFromPageCanvases(canvases);
    const blob = new Blob([pdfBytes], { type: 'application/pdf' });
    const url = URL.createObjectURL(blob);

    const handInfo = (typeof getActivePresetAndHandInfo === 'function')
      ? getActivePresetAndHandInfo()
      : { preset: 'deal' };
    const slug = String(handInfo.preset || handInfo.hand_label || 'deal')
      .toLowerCase()
      .replace(/[^a-z0-9_-]+/g, '-')
      .replace(/^-+|-+$/g, '') || 'deal';

    const a = document.createElement('a');
    a.href = url;
    a.download = `bridge-analysis-${slug}.pdf`;
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    setTimeout(() => URL.revokeObjectURL(url), 5000);

    if (typeof emitSentryMetricCount === 'function') {
      emitSentryMetricCount('analysis_saved_pdf', 1, {
        pages: String(canvases.length)
      });
    }
  } finally {
    if (saveAnalysisBtn) {
      saveAnalysisBtn.textContent = origLabel || '📄 Save Analysis';
      updateSaveAnalysisButtonState();
    }
  }
}

if (saveAnalysisBtn) {
  saveAnalysisBtn.addEventListener('click', () => {
    saveAnalysisAsPdf();
  });
}

if (typeof window !== 'undefined') {
  window.saveAnalysisAsPdf = saveAnalysisAsPdf;
  window.generateAnalysisPdfPages = generateAnalysisPdfPages;
  window.buildPdfFromPageCanvases = buildPdfFromPageCanvases;
  window.updateSaveAnalysisButtonState = updateSaveAnalysisButtonState;
}
