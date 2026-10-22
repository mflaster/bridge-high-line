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

function layoutTreeForPdfSnapshot(rootNode, pathNodeSet) {
  const nodeWidth = 164;
  const nodeHeight = 78;
  const siblingGap = 18;
  const verticalPad = 14;

  const isExpandedInSnapshot = (node) => {
    if (!node || !node.children || node.children.length === 0) return false;
    return pathNodeSet.has(node);
  };

  const getEdgeLabelWidth = (child) => {
    if (!child || !child.condition) return 0;
    let condText = child.condition;
    if (condText.startsWith('after ')) condText = condText.substring(6);
    return Math.max(54, condText.length * 7 + 12);
  };

  const shiftSubtree = (node, dx) => {
    if (!node || dx === 0) return;
    node._pdfX += dx;
    if (node._pdfBoxes) {
      for (const b of node._pdfBoxes) {
        b.xMin += dx;
        b.xMax += dx;
      }
    }
    if (isExpandedInSnapshot(node)) {
      node.children.forEach(c => shiftSubtree(c, dx));
    }
  };

  const layoutNode = (node, depth = 0, y = 0) => {
    node._pdfDepth = depth;
    node._pdfY = y;
    if (!isExpandedInSnapshot(node)) {
      node._pdfX = 0;
      node._pdfBoxes = [{
        yMin: y,
        yMax: y + nodeHeight,
        xMin: 0,
        xMax: nodeWidth
      }];
      return;
    }

    const gap = (node.children.length === 1) ? 30 : 58;
    const nextY = y + nodeHeight + gap;
    node.children.forEach(child => layoutNode(child, depth + 1, nextY));

    const combinedBoxes = [...node.children[0]._pdfBoxes];
    for (let i = 1; i < node.children.length; i++) {
      const prevChild = node.children[i - 1];
      const currChild = node.children[i];
      let minShift =
        (prevChild._pdfX + nodeWidth + siblingGap) - currChild._pdfX;

      const wPrev = getEdgeLabelWidth(prevChild);
      const wCurr = getEdgeLabelWidth(currChild);
      if (wPrev > 0 && wCurr > 0) {
        const labelShift =
          (prevChild._pdfX + wPrev + wCurr + 14) - currChild._pdfX;
        if (labelShift > minShift) minShift = labelShift;
      }

      for (const bCurr of currChild._pdfBoxes) {
        for (const bPrev of combinedBoxes) {
          if (bCurr.yMin < bPrev.yMax + verticalPad &&
              bCurr.yMax + verticalPad > bPrev.yMin) {
            const reqShift = (bPrev.xMax + siblingGap) - bCurr.xMin;
            if (reqShift > minShift) minShift = reqShift;
          }
        }
      }

      shiftSubtree(currChild, minShift);
      combinedBoxes.push(...currChild._pdfBoxes);
    }

    const firstChild = node.children[0];
    const lastChild = node.children[node.children.length - 1];
    node._pdfX = (firstChild._pdfX + lastChild._pdfX) / 2;

    const startX = node._pdfX + nodeWidth / 2;
    const startY = y + nodeHeight;
    const midY = (startY + nextY) / 2;
    for (const child of node.children) {
      const lw = getEdgeLabelWidth(child);
      if (lw > 0) {
        const endX = child._pdfX + nodeWidth / 2;
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
      xMin: node._pdfX,
      xMax: node._pdfX + nodeWidth
    });
    node._pdfBoxes = combinedBoxes;
  };

  layoutNode(rootNode);

  let minX = Infinity;
  let maxX = -Infinity;
  let minY = Infinity;
  let maxY = -Infinity;
  for (const b of (rootNode._pdfBoxes || [])) {
    if (b.xMin < minX) minX = b.xMin;
    if (b.xMax > maxX) maxX = b.xMax;
    if (b.yMin < minY) minY = b.yMin;
    if (b.yMax > maxY) maxY = b.yMax;
  }
  if (Number.isFinite(minX) && minX !== 0) {
    shiftSubtree(rootNode, -minX);
    maxX -= minX;
    minX = 0;
  }

  return {
    nodeWidth,
    nodeHeight,
    treeW: Math.max(nodeWidth, maxX - minX),
    treeH: Math.max(nodeHeight, maxY - minY),
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

  const {
    nodeWidth,
    nodeHeight,
    treeW,
    treeH,
    isExpandedInSnapshot
  } = layoutTreeForPdfSnapshot(rootNode, pathNodeSet);

  const innerX = x + 20;
  const innerY = y + 58;
  const innerW = w - 40;
  const innerH = h - 78;

  const scaleX = innerW / (treeW + 24);
  const scaleY = innerH / (treeH + 24);
  const scale = Math.min(1.35, Math.max(0.38, Math.min(scaleX, scaleY)));

  const offsetX = innerX + Math.max(12, (innerW - treeW * scale) / 2);
  const offsetY = innerY + Math.max(12, (innerH - treeH * scale) / 2);

  ctx.save();
  ctx.beginPath();
  ctx.rect(innerX, innerY, innerW, innerH);
  ctx.clip();
  ctx.translate(offsetX, offsetY);
  ctx.scale(scale, scale);

  // Pass 1: Draw Links & Edge Condition Labels
  const drawLinks = (node) => {
    if (!isExpandedInSnapshot(node)) return;
    node.children.forEach((child) => {
      const onPath = pathNodeSet.has(node) && pathNodeSet.has(child);
      const startX = node._pdfX + nodeWidth / 2;
      const startY = node._pdfY + nodeHeight;
      const endX = child._pdfX + nodeWidth / 2;
      const endY = child._pdfY;
      const midY = (startY + endY) / 2;

      ctx.beginPath();
      ctx.moveTo(startX, startY);
      ctx.bezierCurveTo(startX, midY, endX, midY, endX, endY);
      ctx.lineWidth = onPath ? 3.2 : 1.8;
      ctx.strokeStyle = onPath
        ? '#22d3ee'
        : (child.failure ? '#b91c1c' : '#4a6572');
      ctx.stroke();

      if (child.condition) {
        let condText = child.condition;
        if (condText.startsWith('after ')) condText = condText.substring(6);
        const labelW = Math.max(54, condText.length * 7 + 12);
        const labelH = 18;
        const midX = (startX + endX) / 2;

        pdfFillStrokeCard(
          ctx,
          midX - labelW / 2,
          midY - labelH / 2,
          labelW,
          labelH,
          4,
          onPath ? '#083344' : (child.failure ? '#2a1215' : '#0f172a'),
          onPath ? '#22d3ee' : (child.failure ? '#b91c1c' : '#334155'),
          onPath ? 1.8 : 1.2
        );
        ctx.font = '600 11px Inter, system-ui, sans-serif';
        ctx.fillStyle = onPath
          ? '#67e8f9'
          : (child.failure ? '#fca5a5' : '#cbd5e1');
        ctx.textAlign = 'center';
        ctx.textBaseline = 'middle';
        ctx.fillText(condText, midX, midY);
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
    pdfRoundRect(ctx, nx, ny, nodeWidth, nodeHeight, 8);
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

    pdfRoundRect(ctx, nx, ny, nodeWidth, nodeHeight, 8);
    ctx.lineWidth = isTarget ? 3.5 : (onPath ? 2.5 : 1.6);
    ctx.strokeStyle = strokeCol;
    ctx.stroke();

    // Node text lines
    let rawTitle = node.label || node.trick || 'Node';
    if (rawTitle.startsWith('after ')) rawTitle = rawTitle.substring(6);
    ctx.font = 'bold 12.5px "JetBrains Mono", Inter, sans-serif';
    ctx.fillStyle = '#f8fafc';
    ctx.textAlign = 'left';
    ctx.textBaseline = 'middle';
    ctx.fillText(truncateText(rawTitle, 24), nx + 8, ny + 20);

    const { primary, secondary } = formatNodeCardDealLines(node);
    ctx.font = '500 11.5px Inter, system-ui, sans-serif';
    ctx.fillStyle = '#cbd5e1';
    ctx.fillText(primary, nx + 8, ny + 41);

    if (node.type === 'outcome' && node.ns_tricks !== undefined) {
      ctx.font = 'bold 12px Inter, system-ui, sans-serif';
      ctx.fillStyle = node.failure ? '#fca5a5' : '#6ee7b7';
      ctx.fillText(`N/S Won: ${node.ns_tricks}`, nx + 8, ny + 61);
    } else if (secondary) {
      ctx.font = '500 11.5px Inter, system-ui, sans-serif';
      ctx.fillStyle = '#fca5a5';
      ctx.fillText(secondary, nx + 8, ny + 61);
    }

    if (isExpandedInSnapshot(node)) {
      node.children.forEach(drawNodes);
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
  const colTrickX = x + 52;
  const colLeadX = x + 108;
  const seatCols = [
    x + 196, // N
    x + 306, // E
    x + 416, // S
    x + 526  // W
  ];
  const colWinX = x + w - 48;
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
      const chipW = 94;
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
          let sx = x + 92;
          ['S', 'H', 'D', 'C'].forEach((sKey) => {
            drawSuitHoldingRow(ctx, sx, sy, sKey, suits[sKey] || '-', 122, 15);
            sx += 126;
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

    // Left: Decision Tree Panel (width 920 px)
    drawDecisionTreeSnapshotOnPdf(
      p.ctx,
      48,
      160,
      920,
      1960,
      currentTreeData,
      snap.node,
      pathNodeSet
    );

    // Right: Line Inspector Panel (width 664 px)
    drawLineInspectorSnapshotOnPdf(
      p.ctx,
      988,
      160,
      664,
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
