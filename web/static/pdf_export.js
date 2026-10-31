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
  const ctx = new PdfVectorContext(PDF_PAGE_W, PDF_PAGE_H, PDF_PT_W, PDF_PT_H);
  ctx.fillStyle = '#0b1120';
  ctx.fillRect(0, 0, PDF_PAGE_W, PDF_PAGE_H);
  return { canvas: ctx, ctx };
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
  const evalTotal = (
    currentSolveData &&
    typeof currentSolveData.eval_total_boards === 'number' &&
    currentSolveData.eval_total_boards > 0
  ) ? currentSolveData.eval_total_boards : 0;

  const centerRows = [
    ['Trump:', `${PDF_SUIT_SYMBOLS[trumpVal] || ''} ${trumpVal}`],
    ['Leader:', leaderVal],
    ['Target:', `${targetVal} tricks`],
    [evalTotal > 0 ? 'Solve:' : 'Deals:', `${itersVal}`]
  ];
  if (evalTotal > 0) {
    centerRows.push(['Test:', evalTotal.toLocaleString('en-US')]);
  }
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
  const counts = (typeof getHeadlineSolveCounts === 'function')
    ? getHeadlineSolveCounts(data)
    : {
        total: data.total_boards || 1,
        success: data.success_boards !== undefined
          ? data.success_boards
          : (data.covered_boards || 0),
        failure: data.failure_boards !== undefined
          ? data.failure_boards
          : Math.max(
              0,
              (data.total_boards || 1) -
                (data.success_boards !== undefined
                  ? data.success_boards
                  : (data.covered_boards || 0))
            )
      };
  const total = Math.max(1, counts.total || 1);
  const success = counts.success || 0;
  const failure = counts.failure || 0;
  const succPct = ((success / total) * 100).toFixed(1);
  const failPct = ((failure / total) * 100).toFixed(1);
  const succRatio = `${success.toLocaleString()}/${total.toLocaleString()}`;
  const failRatio = `${failure.toLocaleString()}/${total.toLocaleString()}`;
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
  const rawLeadVal = (initCur && initCur.length > 0)
    ? initCur.join(' ')
    : ((rawStatLead && rawStatLead !== '-')
        ? rawStatLead
        : (data.best_lead || '-'));
  const leadVal = (typeof formatCardTokensForFrontend === 'function')
    ? formatCardTokensForFrontend(rawLeadVal)
    : rawLeadVal;

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
  ctx.font = 'bold 28px Inter, system-ui, sans-serif';
  ctx.fillStyle = '#ecfdf5';
  ctx.fillText(`${succPct}% (${succRatio})`, cX1 + 20, rY2 + 74);

  // 4. Failure Rate
  pdfFillStrokeCard(ctx, cX2, rY2, cardW, cardH, 12, '#450a0a', '#ef4444', 2);
  ctx.font = '600 16px Inter, system-ui, sans-serif';
  ctx.fillStyle = '#fecaca';
  ctx.fillText('FAILURE RATE', cX2 + 20, rY2 + 30);
  ctx.font = 'bold 28px Inter, system-ui, sans-serif';
  ctx.fillStyle = '#fef2f2';
  ctx.fillText(`${failPct}% (${failRatio})`, cX2 + 20, rY2 + 74);

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
      const rawSeqStr = (
        data.sequence || data.best_lead || 'Optimal double-dummy line'
      );
      const seqStr = (typeof formatCardTokensForFrontend === 'function')
        ? formatCardTokensForFrontend(rawSeqStr)
        : rawSeqStr;
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
    `(?:when|if)\\s+${seatPat}\\s+plays\\s+([A-Z0-9_<]+)`,
    'i'
  );
  const discardRegex = new RegExp(
    `(?:when|if)\\s+${seatPat}\\s+discards`,
    'i'
  );

  const initCur = (typeof getActiveInitialCurTrick === 'function')
    ? getActiveInitialCurTrick()
    : [];

  let pendingConditions = [];

  path.forEach((pathNode, stepIdx) => {
    if (pathNode.condition) {
      let cText = pathNode.condition;
      if (cText.startsWith('after ')) cText = cText.substring(6);
      if (typeof formatCardTokensForFrontend === 'function') {
        cText = formatCardTokensForFrontend(cText);
      }
      pathConditions.push({
        trickNum: trickRows.length + 1,
        condition: cText,
        node: pathNode
      });
      pendingConditions.push({
        text: cText,
        parent: pathNode._parent || null
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
      const cleanTok = tok.replace(/[\(\)]/g, '').trim();
      const isHonorWild = cleanTok.length >= 2 &&
        (cleanTok.slice(1) === 'X' || cleanTok.slice(1) === 'H');
      const effectiveTok = (
        trickRows.length === 0 &&
        i < initCur.length &&
        (cleanTok.includes('_') || cleanTok.includes('x') || isHonorWild)
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
          const isHonorWild = existingRaw.length >= 2 &&
            (existingRaw.slice(1) === 'X' || existingRaw.slice(1) === 'H');
          const isWildcardSlot = !existingRaw ||
            existingRaw.includes('_') ||
            existingRaw.includes('x') ||
            existingRaw.includes('<') ||
            isHonorWild;
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
    const rowConditions = pendingConditions.slice();
    const condBeforeTrick = rowConditions.map(c => c.text).join(' → ');
    pendingConditions = [];

    trickRows.push({
      trickNum: trickRows.length + 1,
      leadSeatIdx: currentLeader,
      leadSeatName,
      cards: parsedCards,
      winningCardIdx,
      winnerSeat: winnerCard ? winnerCard.seatName : '',
      condition: condBeforeTrick,
      conditions: rowConditions,
      node: pathNode
    });

    if (winnerCard) {
      currentLeader = winnerCard.seatIdx;
    }
  });

  const trailingConditions = pendingConditions.slice();
  return { path, trickRows, pathConditions, trailingConditions };
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
  const nodeWidth = 196;
  const colGap = 18;
  const ellipsisWidth = 84;
  const maxVisibleVariants = 3;

  const isExpandedInSnapshot = (node) => {
    if (!node || !node.children || node.children.length === 0) return false;
    return pathNodeSet.has(node);
  };

  const getNodeBoardCount = (n) => {
    if (!n) return 0;
    if (n.failure) {
      return n.failure_boards !== undefined
        ? n.failure_boards
        : (Array.isArray(n.boards) ? n.boards.length : 0);
    }
    return n.success_boards !== undefined
      ? n.success_boards
      : (n.covered_boards || (Array.isArray(n.boards) ? n.boards.length : 0));
  };

  // Build one row per level along the active path:
  // - Column 0 (leftmost) is ALWAYS the primary (active path) node.
  // - Columns 1..3 to the right are the sibling variant nodes in the same row.
  // - If there are > 3 variants, Column 4 is an ellipsis ("...") summary card.
  const rows = [];
  rows.push({
    activeNode: rootNode,
    variants: [],
    ellipsis: null
  });

  let curr = rootNode;
  while (curr && isExpandedInSnapshot(curr)) {
    const rawChildren = curr.children || [];
    const activeIdx = rawChildren.findIndex(c => pathNodeSet.has(c));
    const activeChild = activeIdx >= 0
      ? rawChildren[activeIdx]
      : rawChildren[0];
    const allVariants = rawChildren
      .filter(c => c !== activeChild)
      .slice()
      .sort((a, b) => getNodeBoardCount(b) - getNodeBoardCount(a));

    const visibleVariants = allVariants.slice(0, maxVisibleVariants);
    const omittedVariants = allVariants.slice(maxVisibleVariants);
    let ellipsis = null;
    if (omittedVariants.length > 0) {
      const omittedDeals = omittedVariants.reduce(
        (sum, n) => sum + getNodeBoardCount(n),
        0
      );
      ellipsis = {
        omittedCount: omittedVariants.length,
        omittedDeals
      };
    }

    rows.push({
      activeNode: activeChild,
      variants: visibleVariants,
      ellipsis
    });
    curr = activeChild;
  }

  const L = rows.length;
  const availH = Math.max(400, (innerH || 1888) - 20);

  // Size nodeHeight and vertical gap so up to 17 rows fit at 1:1 scale
  const nodeHeight = (L >= 17) ? 66 : 70;
  const defaultGapY = (L >= 17) ? 45 : 48;
  const gapY = (L > 1)
    ? Math.min(
        54,
        Math.max(38, Math.floor((availH - L * nodeHeight) / (L - 1)))
      )
    : defaultGapY;

  let curY = 0;
  for (let r = 0; r < L; r++) {
    const row = rows[r];
    if (row.activeNode) {
      row.activeNode._pdfDepth = r;
      row.activeNode._pdfX = 0;
      row.activeNode._pdfY = curY;
    }
    row.variants.forEach((vNode, vIdx) => {
      vNode._pdfDepth = r;
      vNode._pdfX = (vIdx + 1) * (nodeWidth + colGap);
      vNode._pdfY = curY;
    });
    if (row.ellipsis) {
      row.ellipsis.x =
        (maxVisibleVariants + 1) * (nodeWidth + colGap) - 2;
      row.ellipsis.y = curY;
      row.ellipsis.w = ellipsisWidth;
      row.ellipsis.h = nodeHeight;
    }
    if (r < L - 1) {
      curY += nodeHeight + gapY;
    } else {
      curY += nodeHeight;
    }
  }

  const fixedGridW =
    (maxVisibleVariants + 1) * nodeWidth +
    maxVisibleVariants * colGap +
    16 +
    ellipsisWidth;
  const treeH = curY;

  return {
    rows,
    nodeWidth,
    nodeHeight,
    treeW: fixedGridW,
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
    rows,
    nodeWidth,
    nodeHeight,
    treeW,
    treeH
  } = layoutTreeForPdfSnapshot(rootNode, pathNodeSet, innerW, innerH);

  const scaleX = (innerW - 12) / treeW;
  const scaleY = (innerH - 16) / treeH;
  const scale = Math.min(1.02, Math.max(0.5, Math.min(scaleX, scaleY)));

  // Left-align and top-align so Column 0 is always anchored in the first column
  const offsetX = innerX + 10;
  const offsetY = innerY + 10;

  ctx.save();
  ctx.beginPath();
  ctx.rect(innerX, innerY, innerW, innerH);
  ctx.clip();
  ctx.translate(offsetX, offsetY);
  ctx.scale(scale, scale);

  // Pass 1: Draw Links (Primary vertical link + Variant curves + Ellipsis)
  const pillQueue = [];
  for (let r = 0; r < rows.length - 1; r++) {
    const parent = rows[r].activeNode;
    const nextRow = rows[r + 1];
    if (!parent || !nextRow) continue;

    const startX = parent._pdfX + nodeWidth / 2;
    const startY = parent._pdfY + nodeHeight;

    // 1. Links to visible variants (drawn first so active cyan link is on top)
    nextRow.variants.forEach((vNode, vIdx) => {
      const endX = vNode._pdfX + nodeWidth / 2;
      const endY = vNode._pdfY;
      const gapH = endY - startY;

      ctx.beginPath();
      ctx.moveTo(startX, startY);
      ctx.bezierCurveTo(
        startX,
        startY + gapH * 0.45,
        endX,
        startY + gapH * 0.22,
        endX,
        endY
      );
      ctx.lineWidth = 2.0;
      ctx.strokeStyle = vNode.failure ? '#b91c1c' : '#4a6572';
      ctx.stroke();

      if (vNode.condition) {
        pillQueue.push({
          child: vNode,
          colIdx: vIdx + 1,
          endX,
          endY,
          onPath: false
        });
      }
    });

    // 2. Dashed link to ellipsis card if present
    if (nextRow.ellipsis) {
      const el = nextRow.ellipsis;
      const endX = el.x + el.w / 2;
      const endY = el.y;
      const gapH = endY - startY;

      ctx.save();
      ctx.setLineDash([6, 5]);
      ctx.beginPath();
      ctx.moveTo(startX, startY);
      ctx.bezierCurveTo(
        startX,
        startY + gapH * 0.45,
        endX,
        startY + gapH * 0.22,
        endX,
        endY
      );
      ctx.lineWidth = 1.8;
      ctx.strokeStyle = '#64748b';
      ctx.stroke();
      ctx.restore();
    }

    // 3. Primary vertical link in Column 0
    if (nextRow.activeNode) {
      const activeChild = nextRow.activeNode;
      const endX = activeChild._pdfX + nodeWidth / 2;
      const endY = activeChild._pdfY;

      ctx.beginPath();
      ctx.moveTo(startX, startY);
      ctx.lineTo(endX, endY);
      ctx.lineWidth = 3.8;
      ctx.strokeStyle = '#22d3ee';
      ctx.stroke();

      if (activeChild.condition) {
        pillQueue.push({
          child: activeChild,
          colIdx: 0,
          endX,
          endY,
          onPath: true
        });
      }
    }
  }

  // Pass 2: Draw Condition Pills on top of links
  pillQueue.forEach(({ child, colIdx, endX, endY, onPath }) => {
    let condText = child.condition;
    if (condText.startsWith('after ')) condText = condText.substring(6);
    if (typeof formatCardTokensForFrontend === 'function') {
      condText = formatCardTokensForFrontend(condText);
    }
    ctx.font = '600 12px Inter, system-ui, sans-serif';
    const textW = ctx.measureText(condText).width;
    const labelW = Math.max(58, textW + 14);
    const labelH = 20;

    // Alternate vertical band between even and odd columns so pills in
    // adjacent columns never overlap even when labels are long.
    const pillY = (colIdx % 2 === 0) ? (endY - 13) : (endY - 33);
    const pillX = Math.max(
      labelW / 2 + 2,
      Math.min(treeW - labelW / 2 - 2, endX)
    );

    pdfFillStrokeCard(
      ctx,
      pillX - labelW / 2,
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
    ctx.fillText(condText, pillX, pillY);
    ctx.textAlign = 'left';
  });

  // Pass 3: Draw Node Cards & Ellipsis Cards
  const drawSingleNodeCard = (node) => {
    if (!node) return;
    const nx = node._pdfX;
    const ny = node._pdfY;
    const isTarget = (node === targetNode);
    const onPath = pathNodeSet.has(node);

    const { dealFrac, failFrac } = getNodeFillProportions(node);
    const greyColor = '#1e293b';
    const successColor = '#064e3b';
    const failColor = '#450a0a';

    ctx.save();
    pdfRoundRect(ctx, nx, ny, nodeWidth, nodeHeight, 9);
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

    let strokeCol = '#3b82f6';
    if (node.type === 'condition') strokeCol = '#a855f7';
    if (node.type === 'outcome') {
      strokeCol = node.failure ? '#ef4444' : '#10b981';
    }
    if (onPath) strokeCol = '#22d3ee';
    if (isTarget) strokeCol = '#facc15';

    pdfRoundRect(ctx, nx, ny, nodeWidth, nodeHeight, 9);
    ctx.lineWidth = isTarget ? 3.8 : (onPath ? 2.8 : 1.8);
    ctx.strokeStyle = strokeCol;
    ctx.stroke();

    const y1 = Math.round(nodeHeight * 0.26);
    const y2 = Math.round(nodeHeight * 0.54);
    const y3 = Math.round(nodeHeight * 0.80);

    let rawTitle = node.label || node.trick || 'Node';
    if (rawTitle.startsWith('after ')) rawTitle = rawTitle.substring(6);
    if (typeof formatCardTokensForFrontend === 'function') {
      rawTitle = formatCardTokensForFrontend(rawTitle);
    }
    ctx.font = 'bold 13px Inter, system-ui, sans-serif';
    ctx.fillStyle = '#f8fafc';
    ctx.textAlign = 'left';
    ctx.textBaseline = 'middle';
    ctx.fillText(
      pdfTruncateToWidth(ctx, rawTitle, nodeWidth - 18),
      nx + 9,
      ny + y1
    );

    const { primary, secondary } = formatNodeCardDealLines(node);
    ctx.font = '600 12px Inter, system-ui, sans-serif';
    ctx.fillStyle = '#e2e8f0';
    ctx.fillText(primary, nx + 9, ny + y2);

    if (node.type === 'outcome' && node.ns_tricks !== undefined) {
      ctx.font = 'bold 12.5px Inter, system-ui, sans-serif';
      ctx.fillStyle = node.failure ? '#fca5a5' : '#6ee7b7';
      ctx.fillText(`N/S Won: ${node.ns_tricks}`, nx + 9, ny + y3);
    } else if (secondary) {
      ctx.font = '600 12px Inter, system-ui, sans-serif';
      ctx.fillStyle = '#fca5a5';
      ctx.fillText(secondary, nx + 9, ny + y3);
    }
  };

  rows.forEach((row) => {
    if (row.activeNode) drawSingleNodeCard(row.activeNode);
    row.variants.forEach(drawSingleNodeCard);
    if (row.ellipsis) {
      const el = row.ellipsis;
      pdfFillStrokeCard(
        ctx, el.x, el.y, el.w, el.h, 9, '#1e293b', '#64748b', 1.6
      );
      ctx.textAlign = 'center';
      ctx.textBaseline = 'middle';
      ctx.font = 'bold 20px Inter, system-ui, sans-serif';
      ctx.fillStyle = '#e2e8f0';
      ctx.fillText('...', el.x + el.w / 2, el.y + Math.round(el.h * 0.30));

      ctx.font = 'bold 12px Inter, system-ui, sans-serif';
      ctx.fillStyle = '#94a3b8';
      const moreLbl = `+${el.omittedCount} more`;
      ctx.fillText(moreLbl, el.x + el.w / 2, el.y + Math.round(el.h * 0.58));

      ctx.font = '500 11px Inter, system-ui, sans-serif';
      ctx.fillStyle = '#cbd5e1';
      const dealsLbl =
        `(${el.omittedDeals} deal${el.omittedDeals !== 1 ? 's' : ''})`;
      ctx.fillText(dealsLbl, el.x + el.w / 2, el.y + Math.round(el.h * 0.82));
      ctx.textAlign = 'left';
    }
  });

  ctx.restore();
}

function drawLineInspectorSnapshotOnPdf(
  ctx, x, y, w, h, targetNode, inspectorData, branch1ConditionSet = null
) {
  pdfFillStrokeCard(ctx, x, y, w, h, 16, '#111c33', '#283856', 2);

  ctx.font = 'bold 20px Inter, system-ui, sans-serif';
  ctx.fillStyle = '#f8fafc';
  ctx.textAlign = 'left';
  ctx.textBaseline = 'middle';
  ctx.fillText('Line Inspector (Trick-by-Trick Play)', x + 22, y + 32);

  const { trickRows, trailingConditions = [] } = inspectorData;
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

  const drawConditionBanner = (condItem) => {
    const condStr = typeof condItem === 'string'
      ? condItem
      : (condItem && condItem.text ? condItem.text : '');
    if (!condStr) return;
    const parentNode = (condItem && typeof condItem === 'object')
      ? condItem.parent
      : null;

    let b1CondStr = null;
    if (branch1ConditionSet instanceof Map && parentNode) {
      b1CondStr = branch1ConditionSet.get(parentNode) || null;
    }
    const isSameAsBranch1 = !branch1ConditionSet || (b1CondStr === condStr) ||
      (branch1ConditionSet instanceof Set && branch1ConditionSet.has(condStr));

    if (isSameAsBranch1) {
      pdfFillStrokeCard(
        ctx, x + 22, curY, w - 44, 28, 6, '#172554', '#3b82f6', 1
      );
      ctx.font = '600 14px Inter, system-ui, sans-serif';
      ctx.fillStyle = '#93c5fd';
      ctx.fillText(
        pdfTruncateToWidth(ctx, `↳ Condition: ${condStr}`, w - 68),
        x + 34,
        curY + 14
      );
      curY += 34;
      return;
    }

    const toks = condStr.trim().split(/\s+/);
    const b1Toks = b1CondStr ? b1CondStr.trim().split(/\s+/) : [];
    let commonPre = 0;
    while (
      commonPre < toks.length &&
      commonPre < b1Toks.length &&
      toks[commonPre] === b1Toks[commonPre]
    ) {
      commonPre++;
    }
    let commonSuf = 0;
    while (
      commonSuf < (toks.length - commonPre) &&
      commonSuf < (b1Toks.length - commonPre) &&
      toks[toks.length - 1 - commonSuf] ===
        b1Toks[b1Toks.length - 1 - commonSuf]
    ) {
      commonSuf++;
    }

    if (commonPre >= 2 && commonPre + commonSuf < toks.length) {
      pdfFillStrokeCard(
        ctx, x + 22, curY, w - 44, 28, 6, '#172554', '#3b82f6', 1
      );
      const prefixTxt = `↳ Condition: ${toks.slice(0, commonPre).join(' ')} `;
      const diffTxt = toks.slice(commonPre, toks.length - commonSuf).join(' ');
      const suffixTxt = commonSuf > 0
        ? ` ${toks.slice(toks.length - commonSuf).join(' ')}`
        : '';

      const tx = x + 34;
      const ty = curY + 14;
      ctx.font = '600 14px Inter, system-ui, sans-serif';
      ctx.fillStyle = '#93c5fd';
      ctx.fillText(prefixTxt, tx, ty);
      const preW = ctx.measureText(prefixTxt).width;

      ctx.font = 'bold 14px Inter, system-ui, sans-serif';
      const diffW = ctx.measureText(diffTxt).width;
      const pillX = tx + preW + 2;
      const pillW = diffW + 10;
      pdfFillStrokeCard(
        ctx, pillX, curY + 3, pillW, 22, 4, '#fde047', '#eab308', 1.2
      );
      ctx.fillStyle = '#000000';
      ctx.fillText(diffTxt, pillX + 5, ty);

      if (suffixTxt) {
        ctx.font = '600 14px Inter, system-ui, sans-serif';
        ctx.fillStyle = '#93c5fd';
        ctx.fillText(suffixTxt, pillX + pillW + 2, ty);
      }
      curY += 34;
      return;
    }

    pdfFillStrokeCard(
      ctx, x + 22, curY, w - 44, 28, 6, '#fde047', '#eab308', 1.8
    );
    ctx.font = 'bold 14px Inter, system-ui, sans-serif';
    ctx.fillStyle = '#000000';
    ctx.fillText(
      pdfTruncateToWidth(ctx, `↳ Condition: ${condStr}`, w - 68),
      x + 34,
      curY + 14
    );
    curY += 34;
  };

  trickRows.forEach((tr) => {
    const condList = (Array.isArray(tr.conditions) && tr.conditions.length > 0)
      ? tr.conditions
      : (tr.condition ? [tr.condition] : []);
    condList.forEach(drawConditionBanner);

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

  trailingConditions.forEach(drawConditionBanner);

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
// Pure-JS Native Vector & Text PDF 1.4 Renderer (Base-14 Type1 Fonts)
// =========================================================================

// Standard Adobe AFM glyph widths (per 1000 units) for ASCII 32..126
const PDF_HELV_WIDTHS = [
  278, 278, 355, 556, 556, 889, 667, 191, 333, 333, 389, 584, 278, 333, 278,
  278, 556, 556, 556, 556, 556, 556, 556, 556, 556, 556, 278, 278, 584, 584,
  584, 556, 1015, 667, 667, 722, 722, 667, 611, 778, 722, 278, 500, 667, 556,
  833, 722, 778, 667, 778, 722, 667, 611, 722, 667, 944, 667, 667, 611, 278,
  278, 278, 469, 556, 333, 556, 556, 500, 556, 556, 278, 556, 556, 222, 222,
  500, 222, 833, 556, 556, 556, 556, 333, 500, 278, 556, 500, 722, 500, 500,
  500, 334, 260, 334, 584
];

const PDF_HELV_BOLD_WIDTHS = [
  278, 333, 474, 556, 556, 889, 722, 238, 333, 333, 389, 584, 278, 333, 278,
  278, 556, 556, 556, 556, 556, 556, 556, 556, 556, 556, 333, 333, 584, 584,
  584, 611, 975, 722, 722, 722, 722, 667, 611, 778, 722, 278, 556, 722, 611,
  833, 722, 778, 667, 778, 722, 667, 611, 722, 667, 944, 667, 667, 611, 333,
  278, 333, 584, 556, 333, 556, 611, 556, 611, 556, 333, 611, 611, 278, 278,
  556, 278, 889, 611, 611, 611, 611, 389, 556, 333, 611, 556, 778, 556, 556,
  500, 389, 280, 389, 584
];

function pdfFmt(n) {
  const v = Number(n) || 0;
  if (Math.abs(v) < 1e-4) return '0';
  const s = v.toFixed(2);
  return s.replace(/\.?0+$/, '');
}

function pdfColorToRgb(colorStr) {
  const s = String(colorStr || '#000000').trim();
  if (s.startsWith('#')) {
    let hex = s.slice(1);
    if (hex.length === 3) {
      hex = hex[0] + hex[0] + hex[1] + hex[1] + hex[2] + hex[2];
    }
    const r = parseInt(hex.slice(0, 2), 16) / 255;
    const g = parseInt(hex.slice(2, 4), 16) / 255;
    const b = parseInt(hex.slice(4, 6), 16) / 255;
    return `${pdfFmt(r)} ${pdfFmt(g)} ${pdfFmt(b)}`;
  }
  const m = s.match(/rgba?\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)/i);
  if (m) {
    const r = Number(m[1]) / 255;
    const g = Number(m[2]) / 255;
    const b = Number(m[3]) / 255;
    return `${pdfFmt(r)} ${pdfFmt(g)} ${pdfFmt(b)}`;
  }
  return '1 1 1';
}

function parsePdfFontSpec(fontStr) {
  const s = String(fontStr || '16px sans-serif');
  const m = s.match(/(\d+(?:\.\d+)?)px/i);
  const sizePx = m ? parseFloat(m[1]) : 16;
  const isBold = /\b(bold|[789]00)\b/i.test(s);
  const isItalic = /\b(italic|oblique)\b/i.test(s);
  let fontKey = 'F1';
  if (isBold) {
    fontKey = 'F2';
  } else if (isItalic) {
    fontKey = 'F3';
  }
  return { sizePx, fontKey, isMono: false, isBold };
}

function escapePdfAsciiChar(ch) {
  if (ch === '\\') return '\\\\';
  if (ch === '(') return '\\(';
  if (ch === ')') return '\\)';
  return ch;
}

// Splits text into runs of Base-14 fonts (/F1../F5 for text, /F6 for /Symbol
// suit/arrow glyphs, /F7 for /ZapfDingbats star) and computes exact AFM width.
function tokenizePdfTextRuns(text, fontSpec) {
  const str = String(text === undefined || text === null ? '' : text);
  const { fontKey: baseFontKey, isMono, isBold, sizePx } = fontSpec;
  const widthTable = isBold ? PDF_HELV_BOLD_WIDTHS : PDF_HELV_WIDTHS;
  const runs = [];
  let totalUnits = 0;

  const appendRun = (fKey, pdfLit, units) => {
    totalUnits += units;
    if (runs.length > 0 && runs[runs.length - 1].fontKey === fKey) {
      runs[runs.length - 1].pdfLiteral += pdfLit;
    } else {
      runs.push({ fontKey: fKey, pdfLiteral: pdfLit });
    }
  };

  for (const ch of str) {
    if (ch === '♠') {
      appendRun('F6', '\\252', 750);
    } else if (ch === '♥') {
      appendRun('F6', '\\251', 750);
    } else if (ch === '♦') {
      appendRun('F6', '\\250', 750);
    } else if (ch === '♣') {
      appendRun('F6', '\\247', 750);
    } else if (ch === '→' || ch === '↳') {
      appendRun('F6', '\\256', 987);
    } else if (ch === '★') {
      appendRun('F7', 'H', 790);
    } else if (ch === '•') {
      appendRun(baseFontKey, '\\225', isMono ? 600 : 350);
    } else if (ch === '–') {
      appendRun(baseFontKey, '\\226', isMono ? 600 : 556);
    } else if (ch === '—') {
      appendRun(baseFontKey, '\\227', isMono ? 600 : 1000);
    } else if (ch === '·') {
      appendRun(baseFontKey, '\\267', isMono ? 600 : 278);
    } else {
      const code = ch.charCodeAt(0);
      if (code >= 32 && code <= 126) {
        const w = isMono ? 600 : (widthTable[code - 32] || 556);
        appendRun(baseFontKey, escapePdfAsciiChar(ch), w);
      } else if (code >= 160 && code <= 255) {
        const oct = '\\' + code.toString(8).padStart(3, '0');
        appendRun(baseFontKey, oct, isMono ? 600 : 556);
      } else {
        appendRun(baseFontKey, '-', isMono ? 600 : 333);
      }
    }
  }

  return {
    runs,
    widthPx: (totalUnits / 1000) * sizePx
  };
}

class PdfVectorContext {
  constructor(pageW, pageH, ptW, ptH) {
    this.width = pageW;
    this.height = pageH;
    this._ptW = ptW;
    this._ptH = ptH;
    this._scalePt = ptW / pageW;
    this._ops = [];
    this._pathCmds = [];
    this._curX = 0;
    this._curY = 0;
    this._tx = 0;
    this._ty = 0;
    this._sx = 1;
    this._sy = 1;
    this._lineDash = [];
    this._stateStack = [];

    this.fillStyle = '#000000';
    this.strokeStyle = '#000000';
    this.lineWidth = 1;
    this.font = '16px Inter, sans-serif';
    this.textAlign = 'left';
    this.textBaseline = 'alphabetic';
  }

  _px(x) {
    return (this._tx + x * this._sx) * this._scalePt;
  }

  _py(y) {
    return this._ptH - (this._ty + y * this._sy) * this._scalePt;
  }

  save() {
    this._stateStack.push({
      tx: this._tx,
      ty: this._ty,
      sx: this._sx,
      sy: this._sy,
      lineDash: this._lineDash.slice(),
      fillStyle: this.fillStyle,
      strokeStyle: this.strokeStyle,
      lineWidth: this.lineWidth,
      font: this.font,
      textAlign: this.textAlign,
      textBaseline: this.textBaseline
    });
    this._ops.push('q');
  }

  restore() {
    const st = this._stateStack.pop();
    if (st) {
      this._tx = st.tx;
      this._ty = st.ty;
      this._sx = st.sx;
      this._sy = st.sy;
      this._lineDash = st.lineDash;
      this.fillStyle = st.fillStyle;
      this.strokeStyle = st.strokeStyle;
      this.lineWidth = st.lineWidth;
      this.font = st.font;
      this.textAlign = st.textAlign;
      this.textBaseline = st.textBaseline;
    }
    this._ops.push('Q');
  }

  translate(dx, dy) {
    this._tx += dx * this._sx;
    this._ty += dy * this._sy;
  }

  scale(sx, sy) {
    this._sx *= sx;
    this._sy *= sy;
  }

  setLineDash(segments) {
    this._lineDash = Array.isArray(segments) ? segments.slice() : [];
  }

  beginPath() {
    this._pathCmds = [];
  }

  moveTo(x, y) {
    this._curX = x;
    this._curY = y;
    this._pathCmds.push(`${pdfFmt(this._px(x))} ${pdfFmt(this._py(y))} m`);
  }

  lineTo(x, y) {
    this._curX = x;
    this._curY = y;
    this._pathCmds.push(`${pdfFmt(this._px(x))} ${pdfFmt(this._py(y))} l`);
  }

  quadraticCurveTo(cpx, cpy, x, y) {
    const x0 = this._curX;
    const y0 = this._curY;
    const cp1x = x0 + (2 / 3) * (cpx - x0);
    const cp1y = y0 + (2 / 3) * (cpy - y0);
    const cp2x = x + (2 / 3) * (cpx - x);
    const cp2y = y + (2 / 3) * (cpy - y);
    this.bezierCurveTo(cp1x, cp1y, cp2x, cp2y, x, y);
  }

  bezierCurveTo(cp1x, cp1y, cp2x, cp2y, x, y) {
    this._curX = x;
    this._curY = y;
    this._pathCmds.push(
      `${pdfFmt(this._px(cp1x))} ${pdfFmt(this._py(cp1y))} ` +
      `${pdfFmt(this._px(cp2x))} ${pdfFmt(this._py(cp2y))} ` +
      `${pdfFmt(this._px(x))} ${pdfFmt(this._py(y))} c`
    );
  }

  rect(x, y, w, h) {
    this.moveTo(x, y);
    this.lineTo(x + w, y);
    this.lineTo(x + w, y + h);
    this.lineTo(x, y + h);
    this.closePath();
  }

  arc(cx, cy, r) {
    const k = r * 0.552284749831;
    this.moveTo(cx + r, cy);
    this.bezierCurveTo(cx + r, cy + k, cx + k, cy + r, cx, cy + r);
    this.bezierCurveTo(cx - k, cy + r, cx - r, cy + k, cx - r, cy);
    this.bezierCurveTo(cx - r, cy - k, cx - k, cy - r, cx, cy - r);
    this.bezierCurveTo(cx + k, cy - r, cx + r, cy - k, cx + r, cy);
    this.closePath();
  }

  closePath() {
    this._pathCmds.push('h');
  }

  fill() {
    if (this._pathCmds.length === 0) return;
    const rgb = pdfColorToRgb(this.fillStyle);
    this._ops.push(`${rgb} rg ${this._pathCmds.join(' ')} f`);
  }

  stroke() {
    if (this._pathCmds.length === 0) return;
    const rgb = pdfColorToRgb(this.strokeStyle);
    const lw = Math.max(0.2, this.lineWidth * this._sx * this._scalePt);
    let dashOp = '[] 0 d';
    if (this._lineDash && this._lineDash.length > 0) {
      const dStr = this._lineDash
        .map(d => pdfFmt(d * this._sx * this._scalePt))
        .join(' ');
      dashOp = `[${dStr}] 0 d`;
    }
    this._ops.push(
      `${rgb} RG ${pdfFmt(lw)} w 1 J 1 j ${dashOp} ` +
      `${this._pathCmds.join(' ')} S`
    );
  }

  clip() {
    if (this._pathCmds.length === 0) return;
    this._ops.push(`${this._pathCmds.join(' ')} W n`);
  }

  fillRect(x, y, w, h) {
    const rgb = pdfColorToRgb(this.fillStyle);
    const rx = this._px(x);
    const ry = this._py(y + h);
    const rw = w * this._sx * this._scalePt;
    const rh = h * this._sy * this._scalePt;
    this._ops.push(
      `${rgb} rg ${pdfFmt(rx)} ${pdfFmt(ry)} ${pdfFmt(rw)} ${pdfFmt(rh)} re f`
    );
  }

  measureText(text) {
    const fontSpec = parsePdfFontSpec(this.font);
    const { widthPx } = tokenizePdfTextRuns(text, fontSpec);
    return { width: widthPx };
  }

  fillText(text, x, y) {
    const fontSpec = parsePdfFontSpec(this.font);
    const { runs, widthPx } = tokenizePdfTextRuns(text, fontSpec);
    if (runs.length === 0) return;

    let startX = x;
    if (this.textAlign === 'center') {
      startX = x - widthPx / 2;
    } else if (this.textAlign === 'right' || this.textAlign === 'end') {
      startX = x - widthPx;
    }

    let baseY = y;
    if (this.textBaseline === 'middle') {
      baseY = y + fontSpec.sizePx * 0.34;
    } else if (
      this.textBaseline === 'top' || this.textBaseline === 'hanging'
    ) {
      baseY = y + fontSpec.sizePx * 0.78;
    } else if (
      this.textBaseline === 'bottom' || this.textBaseline === 'ideographic'
    ) {
      baseY = y - fontSpec.sizePx * 0.15;
    }

    const txPt = this._px(startX);
    const tyPt = this._py(baseY);
    const fontPt = fontSpec.sizePx * this._sx * this._scalePt;
    const rgb = pdfColorToRgb(this.fillStyle);

    const tjOps = runs
      .map(r => `/${r.fontKey} ${pdfFmt(fontPt)} Tf (${r.pdfLiteral}) Tj`)
      .join(' ');
    this._ops.push(
      `BT ${rgb} rg ${pdfFmt(txPt)} ${pdfFmt(tyPt)} Td ${tjOps} ET`
    );
  }

  toPdfStream() {
    return this._ops.join('\n') + '\n';
  }
}

function buildPdfFromPageCanvases(pages) {
  const enc = new TextEncoder();
  const chunks = [];
  let offset = 0;

  const pushStr = (s) => {
    const b = enc.encode(s);
    chunks.push(b);
    offset += b.length;
  };

  const numPages = pages.length;
  // Objects:
  // 1: Catalog
  // 2: Pages
  // 3..9: 7 Base-14 Type1 Font objects (/F1../F7)
  // 10 + i*2: Page object i
  // 11 + i*2: Content stream object i
  const firstPageObjId = 10;
  const totalObjs = 9 + numPages * 2;
  const xrefOffsets = new Array(totalObjs + 1).fill(0);

  pushStr('%PDF-1.4\n%\xFF\xFF\xFF\xFF\n');

  // Object 1: Catalog
  xrefOffsets[1] = offset;
  pushStr('1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n');

  // Object 2: Pages
  const kidsRefs = [];
  for (let i = 0; i < numPages; i++) {
    kidsRefs.push(`${firstPageObjId + i * 2} 0 R`);
  }
  xrefOffsets[2] = offset;
  pushStr(
    `2 0 obj\n<< /Type /Pages /Kids [ ${kidsRefs.join(' ')} ] ` +
    `/Count ${numPages} >>\nendobj\n`
  );

  // Objects 3..9: Base-14 Type1 Fonts
  const fontDefs = [
    [3, 'Helvetica', true],
    [4, 'Helvetica-Bold', true],
    [5, 'Helvetica-Oblique', true],
    [6, 'Courier', true],
    [7, 'Courier-Bold', true],
    [8, 'Symbol', false],
    [9, 'ZapfDingbats', false]
  ];
  fontDefs.forEach(([objId, baseFont, useWinAnsi]) => {
    xrefOffsets[objId] = offset;
    const encPart = useWinAnsi ? ' /Encoding /WinAnsiEncoding' : '';
    pushStr(
      `${objId} 0 obj\n` +
      `<< /Type /Font /Subtype /Type1 /BaseFont /${baseFont}${encPart} >>\n` +
      `endobj\n`
    );
  });

  const fontResDict =
    '/Font << /F1 3 0 R /F2 4 0 R /F3 5 0 R /F4 6 0 R ' +
    '/F5 7 0 R /F6 8 0 R /F7 9 0 R >>';

  for (let i = 0; i < numPages; i++) {
    const pageCtx = pages[i];
    const streamStr = (pageCtx && typeof pageCtx.toPdfStream === 'function')
      ? pageCtx.toPdfStream()
      : '';
    const streamBytes = enc.encode(streamStr);
    const pageObjId = firstPageObjId + i * 2;
    const contentObjId = firstPageObjId + i * 2 + 1;

    xrefOffsets[pageObjId] = offset;
    pushStr(
      `${pageObjId} 0 obj\n` +
      `<< /Type /Page /Parent 2 0 R /MediaBox [0 0 ${PDF_PT_W} ${PDF_PT_H}] ` +
      `/Resources << ${fontResDict} >> ` +
      `/Contents ${contentObjId} 0 R >>\nendobj\n`
    );

    xrefOffsets[contentObjId] = offset;
    pushStr(
      `${contentObjId} 0 obj\n<< /Length ${streamBytes.length} >>\nstream\n`
    );
    chunks.push(streamBytes);
    offset += streamBytes.length;
    pushStr('endstream\nendobj\n');
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
  const firstInspectorData = snapshots.length > 0
    ? computeInspectorDataForNode(snapshots[0].node)
    : null;
  const branch1CondByParent = new Map();
  if (firstInspectorData && Array.isArray(firstInspectorData.path)) {
    firstInspectorData.path.forEach((n) => {
      if (n && n._parent && n.condition) {
        let cTxt = n.condition.startsWith('after ')
          ? n.condition.substring(6)
          : n.condition;
        if (typeof formatCardTokensForFrontend === 'function') {
          cTxt = formatCardTokensForFrontend(cTxt);
        }
        branch1CondByParent.set(n._parent, cTxt);
      }
    });
  }

  snapshots.forEach((snap, idx) => {
    const p = createPdfPageCanvas();
    const inspectorData = (idx === 0 && firstInspectorData)
      ? firstInspectorData
      : computeInspectorDataForNode(snap.node);
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
      inspectorData,
      idx > 0 ? branch1CondByParent : null
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
