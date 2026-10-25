# Solver Heuristics and Special Cases Reference

This document provides a complete technical inventory of the heuristics,
tie-breakers, pruning rules, and special-case mechanisms across the solver
pipeline (`interface_dds.cc`, `conditional_line.cc`, and `line_finder.cc`).

---

## 1. End-to-End Pipeline Overview

`LineFinder::Solve()` finds a human-readable decision tree (`ConditionalLine`)
that maximizes the probability of making at least `min_tricks` across $N$
sampled deals:

1. **Step 0 — Per-Board Double-Dummy Play Extraction (`interface_dds.cc`)**:
   For each sampled deal and trick state, queries DDS (`PopulateDeal` +
   `SolveBoard`, with exact fast paths for 1-trick endings and single-group
   follow-suit plays) to find all contract-making declarer cards and a single
   deterministic (or noise-diversified) defender card, pruning dominated
   declarer plays within a trick.
2. **Step 1 — Recursive Tree Search (`line_finder.cc` + `conditional_line.cc`)**:
   Generalizes opponent card patterns (`GetGeneralizedTricks`), groups
   distinguishable tricks into branching decision candidates (`GroupTricks`),
   recursively solves continuation subtrees (`SolveHelperUncached`,
   `GetLinesFromGroups`, `GetLinesFromTrick`), optionally inserts pre-trick
   state conditions (`TryBranchOnGoodCards`), and retains up to `top_k`
   candidate trees per level across a coverage vs. tree-complexity Pareto
   frontier.
3. **Step 2 — Adversarial Rescoring & Post-Processing (`line_finder.cc` +
   `conditional_line.cc`)**:
   Re-evaluates the Step-1 candidate trees against an adversarial defender
   (`RescoreTopLinesAdversarial` / `ChooseAdversarialDefenderCard`) who can
   falsecard or switch leads among DDS-optimal plays to steer declarer into
   failing branches, then prunes empty branches (`PruneEmptyBranches`) and
   collapses redundant state conditions (`CollapseRedundantNodes`).

---

## 2. Deep Dive: How `ExtraBranches()` Works and What It Affects

### Definition (`ConditionalLine::ExtraBranches()`, `conditional_line.h:248–256`)

```cpp
size_t ExtraBranches() const {
  size_t extra = (conditions_.size() > 1) ? (conditions_.size() - 1) : 0;
  for (const auto &[_cond, child] : conditions_) {
    if (child) {
      extra += child->ExtraBranches();
    }
  }
  return extra;
}
```

- Every node in a `ConditionalLine` tree with $k$ outgoing `conditions_`
  contributes $\max(0, k - 1)$ to `ExtraBranches()`.
- A completely linear line of play (1 trick pattern per level, 1 winner outcome
  per trick, no state splits) has `ExtraBranches() == 0`.
- Each additional branch—whether from:
  - a multi-trick group in `GetLinesFromGroups` (`TrickPlayCondition`),
  - a trick with multiple winning seats in `GetLinesFromTrick`
    (`WinnerPlayCondition`), or
  - a 2-way state split in `TryBranchOnGoodCards` (`IsCardGoodCondition` or
    `PlayerDiscardedCondition`)
  increments `ExtraBranches()` by 1.
- Therefore, `1 + line.ExtraBranches()` equals the total number of terminal
  leaf paths in the decision tree.

### The 5 Places `ExtraBranches()` Is Used

#### 1. Tie-Breaking Equal-Coverage Lines (`is_better_meta` & Step-2 Sort)
- **Step 1 (`GetLinesFromGroups`, `line_finder.cc`)**:
  When two candidates cover the same number of boards (`covered`),
  `is_better_meta` prefers:
  1. Fewer immediate branches at the current trick (`num_conds` ASC),
  2. Fewer total subtree extra branches (`extra_branches` ASC),
  3. Drawing opponent trumps (`draws_trump` DESC).
- **Step 2 (`RescoreTopLinesAdversarial`, `line_finder.cc`)**:
  After adversarial rescoring, top-level lines are ranked by:
  1. Step-2 adversarial coverage (`CoveredFinalBoards()` DESC),
  2. Immediate Trick-1 branches (`conditions().size()` ASC),
  3. Total subtree extra branches (`ExtraBranches()` ASC),
  4. Step-1 coverage (`step1_covered_boards()` DESC),
  5. Trick-1 trump drawing (`LineDrawsTrumpAtTrick1` DESC).

#### 2. Admitting Simpler Trees Inside the `top_k` Competitive Window
- **Where**: `competitive_window_floor` and `add_top_candidate` in
  `GetLinesFromGroups` (`line_finder.cc`).
- **Why**: In Step 1, defenders follow deterministic seat conventions (e.g.,
  2nd-hand low), so branching on an opponent honor (such as `(CT)` vs `(Cx)`)
  can artificially inflate Step-1 board coverage by "peeking" at singleton
  honors. In Step 2, an adversarial defender holding `CT9x` will falsecard `CT`
  to steer declarer into the wrong branch, collapsing the split line's
  coverage.
- **Mechanism**:
  - `competitive_window_floor` defines a coverage window below the best Step-1
    coverage seen so far:
    $$\text{slack} = \max\left(2, \lfloor \text{most\_boards\_covered} / 6 \rfloor\right), \quad \text{floor} = \max(1, \text{most\_boards\_covered} - \text{slack}).$$
  - Even when `top_candidates` already has `top_k` entries with higher Step-1
    coverage, `add_top_candidate` admits a lower-coverage candidate (provided
    `cov >= competitive_window_floor()`) if its `ExtraBranches()` is strictly
    less than `min_existing_extra` (or ties `min_existing_extra` with better
    metadata).

#### 3. Preventing Premature Search-Loop Termination (`top_k_prune_floor`)
- **Where**: `top_k_prune_floor` in `GetLinesFromGroups` (`line_finder.cc`).
- **Why**: Groups are evaluated in descending order of total Step-1 board
  count. Normally, once `top_candidates` is full with lowest retained coverage
  `full_cov`, the search raises its pruning floor to `full_cov` (or `full_cov +
  1`) and skips smaller groups. However, if all retained candidates have
  `ExtraBranches() > 0` (`min_eb > 0`), a slightly smaller 1-trick group might
  achieve coverage within `competitive_window_floor()` with
  `ExtraBranches() == 0`.
- **Mechanism**: When `top_k > 1`, `group_size == 1`, and `min_eb > 0`,
  `top_k_prune_floor` lowers the pruning threshold back down to
  `competitive_window_floor()`, allowing subsequent 1-trick groups to be
  searched for simpler trees.

#### 4. Constructing Simpler Composite Subtrees (`AssembleBranchLines`: `alt_line` and `min_eb_line`)
When a node has $m \ge 1$ branches (multiple `WinnerPlayCondition` outcomes in
`GetLinesFromTrick`, multiple tricks in a group in `GetLinesFromGroups`, or
true/false branches in `TryBranchOnGoodCards`), each branch $i \in \{0, \dots,
m-1\}$ returns up to `child_top_k` candidate subtrees sorted by coverage DESC.
The shared helper `AssembleBranchLines` (`line_finder.cc`) combines them into:
1. **Primary line (`base_line`)**: Picks `child_lines[i][0]` (highest coverage)
   for every branch $i$.
2. **Single-branch substitutions (`alt_line`)**: For each branch $i$ and each
   alternative `alt_idx > 0` where `NonStateExtraBranches(child_lines[i][0]) > 0`
   and `NonStateExtraBranches(alt) < NonStateExtraBranches(child_lines[i][0])`,
   builds a composite tree substituting `child_lines[i][alt_idx]` at branch $i$
   and keeping `[0]` elsewhere.
3. **All-minimum-branch composite (`min_eb_line`)**: Simultaneously picks the
   child with the minimum `NonStateExtraBranches` at *every* branch $i$. If at
   least `min_diff_branches` branches (`1` in `GetLinesFromTrick` /
   `GetLinesFromGroups`, `2` in `TryBranchOnGoodCards`) used a non-zero index,
   adds `min_eb_line` so the globally simplest combined subtree is always
   represented.

#### 5. Pareto-Frontier Deduplication and `min_extra` Slot Reservation (`FinalizeTopKLines`)
Before returning up to `top_k` candidate lines from `GetLinesFromTrick`,
`GetLinesFromGroups`, or `TryBranchOnGoodCards`, `FinalizeTopKLines` (and the
matching `TopCandidate` truncation in `add_top_candidate`) reduces the
candidate list as follows:
1. **Sort** by `(CoveredFinalBoards() DESC, ExtraBranches() ASC)`.
2. **Pareto filter**: Walk the sorted list tracking `seen_nseb`
   (`NonStateExtraBranches` values `> 0` already present at higher or equal
   coverage; multiple `nseb == 0` candidates are kept un-demoted). Any
   candidate whose positive `NonStateExtraBranches` was already seen earlier is
   partitioned to the back.
3. **Guarantee `min_extra` survival**: Find the candidate with the absolute
   minimum `NonStateExtraBranches` in the list. If its index is `>= top_k`,
   swap it into slot `[top_k - 1]` before resizing to `top_k`.

---

## 3. Step 0 Heuristics: Per-Board DDS & Play Pruning (`interface_dds.cc`)

### 3.1 Contract-Making Target vs. Overtricks (`DDSSolveHelper`)
- When declarer is on play and `min_tricks >= 1`, `SolveBoard` is called
  with `target = min_tricks` (`solutions = 2`) rather than `target = -1`
  (double-dummy maximum tricks), using binary `deal` bitmasks updated in place.
- **Rationale**: A safety play that guarantees `min_tricks` across almost all
  boards may sacrifice an overtrick on a board where a peek-dependent drop or
  finesse wins `min_tricks + 1`. Querying `target = min_tricks` keeps the
  safety play in the candidate set. Once `min_tricks <= 0` (contract already
  secured), `target = -1` is used to keep the play tree small.

### 3.2 Declarer Equivalence Group Canonicalization & Intra-Trick Pruning (`DDSSolveHelper`, lines 755–1013)
Within a single trick on a single board, DDS returns equivalence groups
(`futp.rank[i]` and `futp.equals[i]`). To prevent combinatorial explosion from
equivalent spot cards or dominated plays:
1. **Representative rank within a DDS group (lines 756–766)**:
   - For spot cards (`rank < 10`), replaces `futp.rank[i]` with the *lowest*
     rank in the equivalence group (`LowestRankInGroup`).
   - For honors (`rank >= 10`), walks down any contiguous touching honor
     sequence (`r - 1 >= 10`) in `futp.equals[i]` to pick the lowest touching
     honor.
2. **Pruned higher spot lead (`level == 0`, lines 854–858)**:
   If declarer has two optimal spot leads in the same suit (`r < 10` and
   `lowest_lead_rank[s] < 10`) and partner holds a card in `s` higher than `r`,
   prunes the higher spot lead `r`.
3. **Pruned dominated lead within same tier (`level == 0`, lines 859–876)**:
   If two optimal leads `r1 < r` in suit `s` are both spots or both honors,
   opponents hold no cards between `r1` and `r`, partner holds a card below
   `r1`, and `CanNeedUnblockForPartner` is false, prunes `r`.
4. **Cheapest discard (`level >= 1`, lines 889–891)**:
   When discarding (neither following `suit_led` nor ruffing), only the lowest
   optimal discard in each off-suit (`lowest_discard_rank[s]`) is kept.
5. **Cheapest losing follow (`level >= 1`, lines 892–905)**:
   When following suit below `cur_win_rank` (or on a ruffed trick), prunes
   cards higher than `lowest_losing_follow_rank` unless partner has more cards
   left in the suit than either opponent has higher cards AND
   `CanNeedUnblockForPartner` holds.
6. **Pruned honor underplay (`level >= 1`, lines 906–921)**:
   When an opponent is currently winning the trick, prunes underplaying an
   honor (`r >= 10`, `r < cur_win_rank`) if the hand also has an optimal
   winning card `r_win > cur_win_rank` with no opponent cards between `r` and
   `r_win` (unless needed to unblock for partner).
7. **Partner-lead consistency in 3rd seat (`level == 2`, lines 926–968)**:
   If partner led a higher card `lead_rank > partner_lead.lowest_lead_rank` at
   `level == 0` when a lower lead was also optimal:
   - Prunes ruffing partner's lead in 3rd seat (line 929).
   - Prunes discarding in 3rd seat when partner's higher lead is already beaten
     (line 932).
   - Prunes overtaking partner's higher lead in 3rd seat if opponents had no
     cards between `lowest_lead_rank` and `lead_rank` and partner was not
     unblocking (line 936).
   - Prunes ducking under partner's master lead when partner had a cheaper
     master lead of the same spot/honor tier and was not unblocking (line 953).
8. **Cheapest sure win in 3rd/4th seat & equivalent winning cards**:
   Prunes winning with a higher card when a cheaper card in the same suit also
   wins (and no opponent holds an intermediate card, or RHO cannot beat the
   3rd-seat card) unless `CanNeedUnblockForPartner` is true. For 3rd-seat ruffs,
   RHO is treated as unable to overruff only on public single-dummy grounds
   (`rho_known_cant_overruff`: no opponent trumps remain, RHO previously showed
   out of trumps, or LHO just failed to follow to `suit_led` while opponents
   still hold cards in `suit_led`), without peeking at RHO's private `suit_led`
   holding.
9. **No 4th-seat ruff of partner's Ace (lines 982–985)**:
   In 4th seat, if partner is already winning the trick with the Ace (`rank ==
   14`, not ruffed) and a discard is available in `futp`, prunes ruffing
   partner's Ace.

### 3.3 Step-1 Defender Card Selection & Exploration Noise (`ChooseDefenderPlay`, `interface_dds.cc:143–550`)
In Step 1, defenders play a single card per board/state chosen from DDS's
optimal moves according to bridge conventions:
1. **Void in `suit_led` (discarding or ruffing)**: Calls
   `BoardState::FindBestDiscard` and plays the lowest card in the chosen DDS
   equivalence group (`RandomizeTouchingHonorRank` randomizes across touching
   honors `>= T` seeded by `board_id`).
2. **On lead (`level == 0`, `EvaluateDefenderLead` / `ScoreDefenderLeadGroup`)**:
   Ranks each DDS-optimal equivalence group by lead naturalness (lower score =
   preferred):
   - **Tier 0**: Touching honors (`top_r >= 10` with `top_r - 1` in group or
     hand) when N/S do not hold a tenace in that suit.
   - **Tier 50**: Passive low spot when N/S do not hold a tenace.
   - **Tier 150**: Touching honors into an N/S tenace.
   - **Tier 250**: Spot card into an N/S tenace.
   - **Tier 350**: Unsupported honor (`>= J`) into an N/S tenace when defender
     holds a higher card than N/S.
   - **Tier 400**: Unsupported honor under a higher N/S honor/tenace.
   Within touching honors, leads the top of the sequence (`LeadRankInGroup`);
   otherwise leads lowest (`LowestRankInGroup`).
3. **Following suit in 2nd, 3rd, 4th seat (`ChooseFollowingSuitPlay`)**:
   - **2nd seat (`level == 1`) or already ruffed**: 2nd-hand low
     (`PickLowestGroupPlay`).
   - **3rd seat (`level == 2`, `ChooseThirdHandFollow`)**:
     - When Dummy (`seat 2`) is in 4th seat and cannot ruff, 3rd hand sees all
       of 4th hand's remaining cards in `suit_led`: ignoring any Dummy cards
       above 3rd hand's highest optimal card (`effective_dummy_max`), if
       partner's card already beats `effective_dummy_max`, 3rd hand ducks with
       a lower card; otherwise 3rd hand plays the cheapest optimal card above
       `max(cur_win_rank, effective_dummy_max)` (so e.g. with `KJ9` over `S8`
       into Dummy's `A43`, 3rd hand plays `S9` to force out `SA`).
     - If partner's `J` or higher (`cur_win_rank >= 11`) is already winning,
       ducks with a lower card unless 4th hand holds an intermediate honor
       between `cur_win_rank` and 3rd hand's higher honor (and 2nd hand does
       not visibly hold it), in which case 3rd hand plays its higher honor
       (e.g., playing `SA` over partner's `SQ` when 4th hand holds `SK`).
     - Skips any candidate winning group that is a touching equal to partner's
       currently winning card (no unplayed card that 4th hand could hold from
       3rd hand's single-dummy view lies strictly between `cur_win_rank` and
       the candidate rank, e.g., refusing to play `ST` on partner's `S9`).
     - When multiple winning DDS groups exist and 3rd hand holds a tenace over
       2nd hand (every possible 4th-hand card between a cheaper winning group
       and the highest winning group is absent, e.g., 3rd hand holds `AQ` after
       2nd hand played low from `Kx`), finesses the cheaper winning group (`Q`)
       rather than playing the top group (`A`).
     - Otherwise plays the cheapest winning card from the highest-ranked
       winning DDS group (3rd-hand high, bottom of touching sequence), or
       lowest overall if unable to beat `cur_win_rank`.
   - **4th seat (`level == 3`, `ChooseFourthHandFollow`)**: Never overtakes
     partner's winning card; otherwise wins as cheaply as possible
     (`CheapestWinningRankInGroup`), or ducks with lowest card.
4. **Defender exploration noise (`ApplyDefenderNoise`, default
   `g_defender_noise_pct = 10%`)**:
   - Never adds noise when following suit in 2nd/3rd/4th seat.
   - On opening/trick lead (`level == 0`), if multiple suits have reasonable
     non-tenace-gift leads (`key < 350` and within `150` of `overall_best_key`),
     hashes `(board_id, played_cards, CurPlayer, level)` to distribute leads
     uniformly across all active suits so Step 1 sees all realistic defender
     suit shifts.
   - When discarding/ruffing (or single-suit lead with multiple optimal groups),
     switches to a random alternative optimal DDS group with probability
     `noise_pct`%.

---

## 4. Step 1 Heuristics: Generalization & Grouping (`conditional_line.cc`)

### 4.1 Opponent Card Pattern Generalization (`GeneralizeOppCard`, lines 582–629)
Each concrete opponent card (e.g., `S4`, `ST`, `SQ`) is generalized into a
hierarchy of candidate patterns (ordered by `PatternSpecificity`):
- Specific honor (`SA`, `SK`, `SQ`, `SJ`, `ST`) — specificity 9 (only emitted
  for honors, never raw spot cards `2..9`).
- Honor vs. spot (`SX` for `T..A`, `Sx` for `2..9`) — specificity 8.
- Upper-bounded in-suit pattern (`S<J`, `S<Q`, `S<K`, `S<A`) — specificity
  7, 6, 5, 4.
- Any in-suit follow/ruff (`S_`) — specificity 3.
- Any off-suit discard (`R_`) — specificity 2.
- Wildcard any card (`__`) — specificity 1.

#### Special rules for `<H` patterns (`GetGeneralizedPlays`, lines 673–738, & `GetGeneralizedTricks`, lines 1113–1136)
1. **Only generated for observed honors**: `S<H` (`H` in `{J, Q, K, A}`) is
   only emitted if honor `H` actually appeared as an in-suit opponent card on
   at least one board at that demi-trick (`honors_by_suit` mask).
2. **Must span both spots and honors**: Any `S<H` bucket whose matched boards
   do not include *both* at least one spot card (`2..9`, mask `0x00ff`) and at
   least one lower honor (`T..K`, mask `0x0f00`) is erased (lines 731–738 and
   1117–1136). Otherwise `S<H` would be identical to `Sx` or to a single honor.
3. **Never used on 4th seat when declarer leads**: In `GetGeneralizedTricks`
   (line 1113), `if (leader_ % 2 == 0 && d2.opp_card.size() == 3) continue;`
   suppresses `S<H` on 4th hand because declarer has already played both cards
   before 4th hand acts.

### 4.2 Grouping Tricks into Decision Branches (`GroupTricks`, lines 1485–1871)
`GroupTricks` forms groups of `GeneralizedTrick`s that cover disjoint sets of
Step-1 boards and can be distinguished by declarer at the table:
1. **When declarer leads (`leader % 2 == 0`, `AreCardsDistinguishable`, lines
   1541–1548)**:
   - Declarer's 1st-seat card (`my1`) must be identical (`same_my1 == true`).
   - Declarer's 3rd-seat card (`my2`) must be *different* (`same_my2 == false`).
     This prevents branching on 2nd-hand opponent cards when declarer plays the
     same 3rd-seat card regardless.
   - 2nd-hand opponent patterns (`demi1.opp_card`) must be non-overlapping
     (`!CardsAreRedundant` and not `__` vs `R_`).
2. **When a defender leads (`leader % 2 != 0`, lines 1550–1564)**:
   - Two tricks are distinguishable at 1st seat (`demi1.opp_card`) if their
     patterns are non-overlapping (or if one is more specific, e.g. `S_` vs
     `__`, and the less-specific trick's actual boards contain no cards matching
     the more-specific pattern via `TrickOppPatternMask`), *provided* that
     declarer's response (`my1` or `my2`) differs or the led suit differs.
   - If 1st-seat patterns are not distinguishable, requires `same_my1 == true`,
     `same_my2 == false`, and distinguishable 3rd-seat opponent patterns
     (`demi2.opp_card`).
3. **Redundancy keys & group subsumption (lines 1647–1853)**:
   - Tricks with identical `(my1, my2, board_bitmask)` share a `key_id`; a new
     group is only kept if it covers at least one `key_id` not yet included in
     any prior group (`any_new_groupings`) and is not component-wise subsumed
     (`group_subsumes`) by an existing group.

---

## 5. Step 1 Heuristics: Tree Search, Pruning & State Splits (`line_finder.cc`)

### 5.1 Dynamic `top_k` Schedule (`TopKForLevel`, lines 1230–1251)
Controls how many candidate `ConditionalLine`s are returned at each recursion
depth `level` (where `level == 0` is Trick 1):
- `level == 0`: `top_k = 10` (or `20` if `keep_all_top_k_` is set).
- `level == 1`: `top_k = 4` (if `active_boards <= 80`) or `2` (if `> 80`).
- `level == 2..4`: `top_k = 3` (if `active_boards <= 30`), `2` (if `31..80`),
  or `1` (if `> 80`).
- `level >= 5`: `top_k = 1`.
- Inside `GetLinesFromTrick` (lines 1368–1379), `child_top_k` is boosted to at
  least `4` (at `level <= 1`) or `3` (at `level == 2`, `active_boards <= 80`)
  so `GetLinesFromGroups` can assemble low-`ExtraBranches()` composite lines.

### 5.2 Null-Move (Pass-the-Lead) Pre-Pass & `IsUninformativeTrick` (`FindNullMoveLeads`, lines 1054–1214)
When the solver is at a trick boundary with `level <= 3`, `min_tricks >= 2`,
and `<= 9` cards remaining:
1. **Why `FindNullMoveLeads` exists**:
   Before committing to a 2-way state split (`if <card> is good` or
   `if <player> discarded on <suit>`), the solver checks what optimal tricks
   would be played if defenders were on lead right now (`cur_player % 2 == 1`)
   or what tricks are already in `gen_tricks` when declarer is on lead.
2. **`IsUninformativeTrick` (lines 1054–1095)**:
   A trick is classified as "uninformative" (meaning declarer can play it
   *before* deciding how to branch on a state condition, without losing any
   options or information) if:
   - It covers `>= 80%` of solvable boards, AND
   - Either:
     - **Case A (Declarer wins in 4th seat)**: Defender leads (`leader % 2 ==
       1`), declarer's 2nd-seat card is a low spot (`2..9`), and declarer's
       4th-seat card wins the trick on every board (so declarer sees both
       defenders' cards before playing 4th hand), OR
     - **Case B (Clearing the last opponent card in a suit)**: Declarer leads a
       non-trump suit (or any suit in NT) where at most 1 opponent card remains
       outstanding on every board, and declarer wins the trick on every board.
3. **Effect on `TryBranchOnGoodCards`**:
   - Suits with an uninformative trick (`uninformative_suits_mask`) are
     *excluded* from immediate `IsCardGoodCondition` and
     `PlayerDiscardedCondition` splitting at the current level, because
     declarer should cash/play the uninformative trick first and branch on the
     next level instead.
   - Candidate cards for `IsCardGoodCondition` are prioritized if they are
     led in the null-move tricks (`null_move_leads`), sorted by `(in_null_move
     DESC, rank ASC, suit ASC)`, and capped at `kMaxGoodCardCandidates = 6`.

### 5.3 Drawing Trumps Preference (`TrickDrawsTrump` / `GroupDrawsTrump`, lines 945–969)
- `TrickDrawsTrump(gt, trump)` returns `true` iff `trump != NOTRUMP`, declarer
  leads a trump (`gt.leader % 2 == 0 && gt.demi1.my_card[0] == trump`), and at
  least one opponent follows in trump (`gt.demi1.opp_card != "R_"` or
  `gt.demi2.opp_card != "R_"`). When both opponents discard (`R_`), cashing a
  high trump does not draw any opponent trumps and is not preferred.
- Used as a tie-breaker in:
  1. `SolveHelperUncached` group ordering (`line_finder.cc:3121`),
  2. `GetLinesFromGroups` candidate ranking (`is_better_meta`,
     `line_finder.cc:1859` and `top_k == 1` tie-breaking, line 2483),
  3. `RescoreTopLinesAdversarial` Step-2 tie-breaking (`line_finder.cc:773`).

### 5.4 Group & Trick Upper-Bound Pruning and `SubsetUpperBoundTightening` (`line_finder.cc:1985–2310, 3022–3123`)
1. **Disjoint-group upper bound (`SolveHelperUncached`, lines 3022–3062)**:
   Since tricks within a group cover disjoint board sets, `TotalPlayNums(group)`
   is an upper bound on how many boards the group can cover. Groups with
   `tot < best_possible` are pruned before `GetLinesFromGroups`.
2. **`SubsetUpperBoundTightening` (`kSubsetTightenMinTricks = 3`)**:
   At nodes where `min_tricks >= 3`, before recursing into `SolveHelper` for a
   winner outcome (in `GetLinesFromTrick`, lines 1336–1356) or before recursing
   on the tricks of a multi-trick group (in `GetLinesFromGroups`, lines
   2060–2135), the solver checks how many boards in each child subsetActually
   have `opt_tree->PlayTrick(t)->MiniMax() >= next_min_tricks`. Any board that
   cannot make `next_min_tricks` even double-dummy is subtracted from the
   upper bound immediately, tightening `max_possible` before any recursive tree
   construction.
3. **`saved_work_alts` memoization across groups (`GetLinesFromGroups`, lines
   1792, 2248–2473)**:
   Because the same `GeneralizedTrick` often appears in multiple groups from
   `GroupTricks`, `saved_work` caches the primary `ConditionalLine` for each
   `GeneralizedTrick` string, and `saved_work_alts` caches up to 3 lower-branch
   alternatives whenever a trick has multiple `WinnerPlayCondition` outcomes or
   multiple returned lines.

### 5.5 State-Condition Branching (`TryBranchOnGoodCards`)
When unsplit tricks do not cover all solvable boards (`best_covered < solvable_n`,
`lvl > 0`, declarer/dummy on lead, `state_depth < 4`), `TryBranchOnGoodCards`
tests 2-way state splits:
1. **`IsCardGoodCondition(card, true/false)`**:
   Checks whether the highest unplayed N/S card in `suit` is currently a master
   card. On the `is_good` branch, groups optimal `suit`-led cashing tricks via
   `GeneralizedTrickFinder` whose winning N/S card satisfies
   `IsCardGoodCondition(top_card, true)` (accepting equivalent non-touching spot
   cards once intermediate spots have been played).
2. **`PlayerDiscardedCondition(seat, prev_suit_led, true/false)`**:
   Checks whether `East` (`1`) or `West` (`3`) discarded on the explicit
   `prev_suit_led` of the preceding trick (avoiding transposed `PreviousTrick`
   cache aliasing), deduplicating symmetric 3-way splits when neither board has
   both defenders discarding.
3. **Upper-bound pruning & combination**:
   Uses `unsplit_ub` from `GetLinesFromGroups` to bound the `false` branch
   before solving, preserves `has_unsplit_play` thresholds on fail-low nodes,
   and combines `true`/`false` subtrees via `AssembleBranchLines` and
   `FinalizeTopKLines`.

---

## 6. Step 2 Heuristics: Adversarial Rescoring & Cleanup (`conditional_line.cc` & `line_finder.cc`)

### 6.1 Unmakeable Board Denominator Tracing (`TraceUnmakeableBoard`, `conditional_line.cc:2444–2706`)
- **Problem**: In Step 1, only boards that can make `min_tricks` double-dummy
  are recorded in `GeneralizedTrick` board sets. If an opponent lead `(C_)`
  succeeds on 5 makeable boards and fails on 1 unmakeable board (5/6 = 83.3%),
  while `(S_)` succeeds on 5 makeable boards and 0 unmakeable boards (5/5 =
  100%), looking only at makeable boards would make `(C_)` and `(S_)` appear
  tied at 100%.
- **Mechanism**: Before Step-2 rescoring, `TraceUnmakeableBoard` replays every
  board (both makeable and unmakeable) using the Step-1 defender policy
  (`ChooseStep1DefenderCard`) and increments `opp1_unmakeable_boards`,
  `opp2_unmakeable_boards`, and `winner_unmakeable_boards[w]` on each visited
  action node whenever the board was not already counted in Step 1's denominator
  for that node.

### 6.2 Adversarial Defender Card Selection (`ChooseAdversarialDefenderCard`, `conditional_line.cc`)
During Step-2 simulation (`SimulateBoard` with `adversarial = true`), whenever
a defender is on play:
1. **Candidate DDS-optimal cards (`FindOptimalDefenderCards`,
   `interface_dds.cc:579–630`)**:
   Queries DDS for all optimal equivalence groups (`futp.cards`) and returns:
   - The Step-1 heuristic card (`ChooseDefenderPlay`),
   - The seat-convention card (`RankForDefenderGroup`) for each other group,
   - The lowest card (`LowestRankInGroup`) in each group, AND
   - **Every honor (`T, J, Q, K, A`)** present in `futp.equals[i]` for each
     group. Including all honors in optimal equivalence groups allows the
     adversarial defender to falsecard an honor from holdings like `CT9x` or
     `CQJ` when doing so is double-dummy neutral.
2. **Three-tier continuation lookahead (`EvaluateTrickContinuation` &
   `ClassifyContinuationTier`)**:
   Whenever two or more DDS-optimal cards match valid candidate branches and
   either map to distinct branches (`pos < 3 && candidates.size() > 1`) or have
   different empirical success probabilities (`pos = 0..3`), each candidate
   continuation is simulated with **unclamped** N/S remaining double-dummy
   tricks (`clamp_missing_leaf = false`, memoized per
   `(node, played_cards, leader, ns_tricks)` in `SubtreeTierCache`) and ranked
   into three tiers (lower = preferred by the defender):
   - **Tier 0 — Genuine defeat** (enabled when responding mid-trick at
     `pos == 1` or `pos == 2` across explicit branch splits): Continuation does
     not reach a leaf (`leaf == nullptr`) and unclamped
     `final_ns_tricks < root_min_tricks` (or `root_min_tricks <= 0`). This
     defeats Step-1 "peek" splits where declarer branched on a singleton honor
     like `(CT)` or `(CQ)` and committed to a losing response on that branch,
     without creating underplay artifacts on unsplit `(S_) SQ` finesses.
   - **Tier 1 — Covered leaf**: Declarer makes the contract and reaches an
     existing leaf in the decision tree (`leaf != nullptr`).
   - **Tier 2 — Missing Step-1 branch**: Continuation exits the Step-1 tree
     (`leaf == nullptr`), e.g. via a downstream winner or shift branch that no
     Step-1 board needed to visit, while declarer still has
     `>= root_min_tricks` double-dummy. Ranking Tier 2 worse than Tier 1 across
     `pos = 0..3` prevents missing Step-1 branches from being mistaken for
     falsecard defeats or selected due to `0 / k` empirical probabilities.
3. **Empirical branch success probability tie-breaker
   (`BranchSuccessProbability`)**:
   Among DDS-optimal cards in the same tier, the defender picks the card that
   minimizes the matched branch's empirical Step-1 success probability:
   $$P(\text{success}) = \frac{\text{step1\_covered\_boards}}{\text{step1\_denom} + \text{unmakeable\_denom}}.$$

### 6.3 Single-Suit Lead and `<H` Pattern Relaxation (`DeepCloneChildren` & `CollapseUniformInSuitSplits`)
- **Single-suit lead against wildcard sibling (`DeepCloneChildren`, `conditional_line.cc`)**:
  When a defender is on lead (`leader % 2 != 0`) and a node has both a wildcard
  lead branch (`(__)`) and a single branch for a specific suit `S` (e.g., `(SX)`
  or `(DT)` with no other `S` or `D` sibling branch), `DeepCloneChildren` relaxes
  that suit branch's `demi1.opp_card` to `(S_)` (or `(D_)`). During simulation,
  `AreCandidateDeclarerCardsLegalForTrick` additionally filters out any `(__)`
  branch whose prescribed declarer cards would revoke (fail to follow `suit_led`
  when holding cards in `suit_led`).
- **Single-branch `<H` relaxation (`CollapseUniformInSuitSplits`, `conditional_line.cc`)**:
  When a node has a single `TrickPlayCondition` child on a declarer lead
  (`leader % 2 == 0`), relaxes any upper-bounded in-suit opponent pattern
  (`S<H`) on `opp1` or `opp2` to `S_`.

### 6.4 Post-Rescore Pruning and Pattern Relaxation (`PruneEmptyBranches` & `CollapseRedundantNodes`, `line_finder.cc`)
After Step-2 adversarial rescoring updates every node's `CoveredFinalBoards()`:
1. **`PruneEmptyBranches`**:
   - Removes any branch whose Step-2 `CoveredFinalBoards() == 0` (when the node
     still has at least one non-empty branch).
   - If a node is reduced to a single surviving `TrickPlayCondition` branch on
     a declarer lead (`leader % 2 == 0`), relaxes any specific in-suit opponent
     patterns (`Sx`, `SX`, `S<H`, or specific honor) on `opp1` and `opp2` to
     `S_`.
2. **`CollapseRedundantNodes`**:
   - If a state-condition node (`IsCardGoodCondition` or
     `PlayerDiscardedCondition`) has only 1 surviving non-empty child, replaces
     the condition node directly with that child.
   - If both children of a state-condition node prescribe the exact same
     subtree (`Describe()` strings match), merges their `CoveredFinalBoards()`
     and `board_to_trick` maps and replaces the condition node with the merged
     child.

---

## 7. Removed Legacy Special Cases

The following legacy mechanisms became dead or redundant after `GroupTricks`
prohibited same-declarer-play in-suit splits (`if (!same_my1 || same_my2)
return false;` in `AreCardsDistinguishable`) and after `top_k` candidate
tracking was introduced, and have been removed:

| # | Mechanism | Why It Was Redundant |
|---|---|---|
| **1** | **`uniform_buckets` pre-sort in `SolveHelperUncached`** | Re-sorted subsets of `groupings` by `(tot_a > tot_b, a.size() < b.size())` immediately before a full `std::stable_sort` over all of `groupings` by `(tot_a > tot_b, a.size() < b.size(), GroupDrawsTrump)`, which completely overwrote the pre-sort. |
| **2** | **`CountExtraUniformInSuitSplits` & `zero_insuit` tracking** | Penalized multi-branch groups with `leader % 2 == 0`, `same_my1`, and `same_my2` across in-suit opponent branches. Unreachable because `AreCardsDistinguishable` forbids `same_my1 && same_my2` when `leader % 2 == 0` (always returned `0`). |
| **3** | **`DeepCloneChildren` `leader % 2 == 0` sibling-transplant block** | Transplanted winner subtrees between sibling action branches with `leader % 2 == 0` and identical `(my1, my2)`. Unreachable because `GroupTricks` never produces such siblings. |
| **4** | **`SimulateBoard` non-adversarial `pos == 1` `same_pos2` fallback** | Appended general in-suit branches when `!adversarial && pos == 1 && candidates.size() > 1` and all `candidates` shared the same `pos == 2` (`my2`) card. Unreachable because `same_pos2` is always `false` when `candidates.size() > 1` at `pos == 1`. |
| **5** | **`CollapseUniformInSuitSplits` unused parameters & redundant `ResultsOnHandAdversarial` pass** | Simplified to `CollapseUniformInSuitSplits()` (no parameters) and removed the redundant pre-simulation `ParallelForDynamic` pass in `RescoreTopLinesAdversarial` that only populated `def_caches` for the unused parameter. |
| **6** | **Triplicated `ExtraBranches()` sort / Pareto-dedup / `min_extra` reservation** | Extracted shared `FinalizeTopKLines(&lines, limit)` helper used by `GetLinesFromTrick` and `TryBranchOnGoodCards`, and removed unused `LineFinder::GetLineFromTrick`. |
| **7** | **Parallel `top_k == 1` vs. `top_k > 1` paths in `GetLinesFromGroups` & `TryBranchOnGoodCards`** | Unified candidate tracking and state-split assembly so `top_candidates` and `FinalizeTopKLines` handle all `top_k >= 1`. |
| **8** | **Triplicated multi-branch `alt_line` + `min_eb_line` assembly & duplicated `SolveMemo` key packing** | Extracted `AssembleBranchLines` across `GetLinesFromTrick`, `GetLinesFromGroups`, and `TryBranchOnGoodCards`, and `SolveMemoKeyBuilder` across `GetLinesFromTrick` and `SolveHelper`. |
| **9** | **Duplicated `pre_pos` / `pos` loops and continuation helpers in `conditional_line.cc`** | Unified the `0..3` trick loop across `TraceUnmakeableBoardHelper` and `SimulateBoard`, and extracted shared `FilterCandidatesByDeclarerCard`, `KeepMostSpecificCandidates`, `AdvanceThroughStateConditions`, `MatchWinnerContinuation`, and `MakeDefenderStateKey` helpers. |
| **10** | **Dead `SolverCache` DDS maps, `#define CONSIDER_DISCARDS 1`, and unused helpers** | Removed unused `SolverCache::{dds_results, dds_lines, dds_size}`, `CONSIDER_EQUALS`, `GetBoardKey`, `CONSIDER_DISCARDS`, `AnyIntersection`, `IsDistinguishable`, `MakeGroupStartingWith`, `HasActualCardMatchingPattern`, `ExtractTrickLines`, `PlayTrick`, `ExtractWinningCardsForLeader`, and `PrintBoardToTrick`. |

