# Decision Tree Simulation & Failure Indexing Architecture

This document describes the unified board-simulation and failure-indexing architecture across `ConditionalLine`, `LineFinder`, and the web tree serializer (`web/web_server.cc`).

---

## 1. Overview & Motivation

Previously, evaluating a `ConditionalLine` decision tree against a concrete `BoardState` required two separate code paths:
1. **`ResultsOnHandHelper` (`conditional_line.cc`)**: Recursively walked the tree to compute the number of declarer tricks won (and, during Step 2 adversarial rescoring, recorded `RescoreBoardTrace` leaf/trick mappings).
2. **`TraceOnBoardHelper` (`conditional_line.cc`)**: Independently re-walked the tree to produce a `BoardPlayTrace` (step-by-step `PlayedTrickStep` entries) for `RecordFailureTrace` in `web/web_server.cc`.

Having two separate tree-walking engines caused subtle behavioral divergences:
- `TraceOnBoard` re-ran non-adversarial DDS defender play (`FindBestDefenderCard`) without the Step 2 `DefenderCardsCache`, even when a `ConditionalLine` had already been adversarially rescored in `RescoreTopLinesAdversarial`.
- Mid-trick defender divergence handling differed between `ResultsOnHandHelper` (which only filtered candidate branches when `candidates.size() > 1`) and `TraceOnBoardHelper` (which filtered single-candidate tricks and could falsely mark makeable boards as diverging mid-trick, producing spurious `HAND FAILED FOR` nodes in the web UI).
- `RecordFailureTrace` in `web/web_server.cc` recursively walked the serialized JSON tree alongside the `ConditionalLine` pointers, duplicating tree-traversal logic a third time.

---

## 2. Unified Simulation Engine (`ConditionalLine::SimulateBoard`)

`ConditionalLine::SimulateBoard` (`conditional_line.h`, `conditional_line.cc`) is now the single source of truth for simulating a `ConditionalLine` on a concrete `BoardState`:

```cpp
BoardPlayTrace SimulateBoard(
    const BoardState &bs, int board_id = 0, bool adversarial = false,
    DefenderCardsCache *def_cache = nullptr) const;
```

### Key Responsibilities
1. **Condition & Candidate Resolution**:
   - Evaluates `IsCardGoodCondition` branches against the live `BoardState`.
   - Collects candidate action children (`!do_first().Empty()`) at each trick.
2. **Trick Execution (Pre-played and Live Positions)**:
   - Supports mid-trick starting states (`start_pos = copy.CardsPlayedThisTrick()`).
   - For declarer positions (`p % 2 == 0`), selects the most specific candidate (`SelectMostSpecificCandidate`) up to the current position, plays `ChooseValidCard`, and narrows `candidates`.
   - For defender positions (`p % 2 != 0`), selects the defender card via `ChooseAdversarialDefenderCard(copy, pos, candidates, def_cache)` when `adversarial` is true, or `FindBestDefenderCard(copy)` when `adversarial` is false.
   - Candidate filtering on defender cards (`FilterCandidatesByDefenderCard`) is performed when `!adversarial || candidates.size() > 1`:
     - In **adversarial** mode (`Step 2`), once a trick has been narrowed to a single candidate action node, declarer's play for the trick is fixed and the trick completes before checking `WinnerPlayCondition(winner)`.
     - In **non-adversarial** mode (`ResultsOnHand` / un-rescored `TraceOnBoard`), single-candidate tricks still verify pattern compatibility and record `unmatched_seat` / `unmatched_card` if a defender deviates from the line's pattern.
3. **Trace Recording & Continuation**:
   - Emits a `PlayedTrickStep` for each completed 4-card trick, including `node`, `trick_str`, `winner`, `winning_card`, `ns_tricks_so_far`, `diverged_after`, `unmatched_seat`, and `unmatched_card`.
   - Advances to the matching `WinnerPlayCondition(winner)` child or records `trace.leaf` when a terminal success leaf is reached.
   - If the board diverges from the tree before reaching a leaf, computes remaining double-dummy tricks via `FindOptimalTricks` and caps `final_ns_tricks` at `min_tricks_ - 1` when `min_tricks_ > 0` so unverified branches cannot be counted as covered by the line.

### Public Entry Points Built on `SimulateBoard`
- **`ResultsOnHand(const BoardState &bs)`**: Calls `SimulateBoard(bs, 0, /*adversarial=*/false, nullptr).final_ns_tricks`.
- **`ResultsOnHandAdversarial(const BoardState &bs, DefenderCardsCache *def_cache, RescoreBoardTrace *out_trace)`**: Calls `SimulateBoard(bs, 0, /*adversarial=*/true, def_cache)` and populates `out_trace` from `trace.steps` and `trace.leaf`.
- **`TraceOnBoard(const BoardState &bs, int board_id)`**: Returns the cached `BoardPlayTrace` from `board_traces_` when `board_id` was already simulated during `RescoreTopLinesAdversarial`; otherwise calls `SimulateBoard(bs, board_id, /*adversarial=*/has_rescored_boards_, nullptr)`.

---

## 3. Trace Caching During Step 2 Rescoring (`RescoreTopLinesAdversarial`)

During `RescoreTopLinesAdversarial` (`line_finder.cc`), each active board `b` is simulated once per candidate `ConditionalLine` using `SimulateBoard(boards[b], b, /*adversarial=*/true, &def_cache)`:

1. If `trace.final_ns_tricks >= min_tricks && trace.leaf != nullptr`, the board is marked covered (`board_line_ok[b_i][l_i] = 1`), its terminal leaf is credited in `leaf_boards[trace.leaf]`, and its per-node trick strings are recorded in `node_board_tricks`.
2. Every simulated `BoardPlayTrace` (both succeeding and failing boards) is stored on the `ConditionalLine` via `set_board_traces(std::move(line_traces))`.
3. Subsequent calls to `line.TraceOnBoard(boards[b], b)` (for example, when serializing failure branches in the web server) return the exact `BoardPlayTrace` produced during Step 2 adversarial rescoring in $O(1)$ lookup time without re-invoking DDS.

---

## 4. Indexed Failure-Node Injection (`RecordFailureTraceOnIndex`)

In `web/web_server.cc`, `SerializeLineTreeWithFailures` attaches red failure leaves (`is_failure: true`, `"HAND FAILED FOR: ..."`) for makeable boards that are not covered by a given `ConditionalLine`:

1. **`BuildLineNodeIndex`**: Walks the `ConditionalLine` tree and its parallel serialized JSON tree once to build a `LineNodeIndex` mapping `const ConditionalLine *` pointers to their corresponding JSON node (`node_to_json`), condition label (`node_to_cond`), and tree depth (`node_to_depth`).
2. **`RecordFailureTraceOnIndex`**: For each uncovered makeable board `b`:
   - Retrieves `BoardPlayTrace trace = line.TraceOnBoard(boards[b], b)`.
   - Finds the last `PlayedTrickStep` associated with a node in `LineNodeIndex`.
   - If the board diverged mid-trick (`unmatched_seat >= 0`), attaches a failure child under the diverging seat's card condition (`"if E/W plays <card>"` or `"if E/W leads <card>"`).
   - If the board completed the trick at `node` and diverged afterward (`diverged_after`), attaches a failure child under `"when <seat> wins the <card>"`.
   - Propagates the failing board ID up `failure_boards` along the ancestor chain so the web UI can highlight the failure path.
