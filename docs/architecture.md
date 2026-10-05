# Bridge Solver & Line Finder Architecture Specification

This document details the software architecture, component topology, runtime data flow, and module interfaces for the **Bridge Solver & Line Finder** C++ library.

---

## 1. Top-Level Component Topology

The system is organized into four distinct architectural layers, ranging from core data primitives up to high-level analytical line-finding engines. The component topology diagram below highlights module boundaries, header dependencies, and call-graph relationships across the library.

```mermaid
flowchart TB
    subgraph AnalyticalEngines["Analytical Engines Layer"]
        LF["LineFinder<br/><code>line_finder.h</code>"]
        FBS["FuzzyBoardState<br/><code>line_finder.h</code>"]
        CL["ConditionalLine<br/><code>conditional_line.h</code>"]
        GTF["GeneralizedTrickFinder / GeneralizedDemiPlay<br/><code>conditional_line.h</code>"]
        PC["PlayCondition<br/>(WinnerPlayCondition, TrickPlayCondition)<br/><code>conditional_line.h</code>"]
        CHDS["CrossHandDealSetSummarizer<br/><code>cross_hand_summarizer.h</code>"]
    end

    subgraph PerfUtils["Performance Utilities Layer"]
        LFP["line_finder_perf<br/><code>perf/line_finder_perf.cc</code>"]
    end

    subgraph SolversLayer["Solvers & Strategy Layer"]
        OPT["OptimalPlayTree<br/><code>interface_dds.h</code>"]
        IDDS["DDS C++ Wrapper<br/>(DDSFindOptimalLines, DDSParallelSolve)<br/><code>interface_dds.h</code>"]
        MTCW["Solver Engine<br/>(MostTricksCanBeWon)<br/><code>solver.h</code>"]
        SC["SolverCache<br/><code>solver.h</code>"]
        EXTDDS["External Double Dummy Solver C Library<br/>(SolveBoardPBN)<br/><code>dll.h</code>"]
    end

    subgraph StateManagement["State Management Layer"]
        BS["BoardState<br/><code>board_state.h</code>"]
    end

    subgraph CorePrimitives["Core Data Primitives Layer"]
        CARD["Card Representation & Utilities<br/><code>card.h</code>"]
        HAND["Hand Bitmask Operations<br/><code>hand.h</code>"]
        HR["HandRequirements Hierarchy<br/><code>hand_requirements.h</code>"]
        DEAL["Deal Generation & File Loading<br/><code>deal.h</code>"]
    end

    %% Dependencies & Interaction Paths
    LFP --> LF

    LF --> FBS
    LF --> OPT
    LF --> IDDS
    LF --> SC
    LF --> GTF
    LF --> CL
    LF --> CHDS

    CHDS --> CL
    CHDS --> HAND
    CHDS --> CARD

    FBS --> HR
    FBS --> DEAL
    FBS --> BS

    CL --> PC
    CL --> GTF

    OPT --> BS
    OPT --> SC
    OPT --> IDDS

    IDDS --> BS
    IDDS --> SC
    IDDS --> EXTDDS

    MTCW --> BS
    MTCW --> SC
    MTCW --> CARD
    MTCW --> HAND

    BS --> CARD
    BS --> HAND

    DEAL --> BS
    DEAL --> HAND
    DEAL --> HR

    HR --> HAND
    HAND --> CARD
```

---

## 2. Line-Finding Evaluation Sequence Diagram

The runtime execution flow during line-finding searches involves stochastic deal sampling, requirement validation, Double Dummy Solver (DDS) invocation, play tree construction, and decision tree synthesis.

```mermaid
sequenceDiagram
    autonumber
    actor Client as Client / Application
    participant LF as LineFinder
    participant FBS as FuzzyBoardState
    participant DEAL as Deal Engine
    participant HR as HandRequirements
    participant BS as BoardState
    participant OPTree as OptimalPlayTree
    participant IDDS as DDS Interface
    participant SC as SolverCache
    participant DDS as DDS C Library
    participant GTF as GeneralizedTrickFinder
    participant CL as ConditionalLine
    participant CHDS as CrossHandDealSetSummarizer

    Client->>LF: Solve(min_tricks, hands, unknown_cards, leader, trump, cur_trick)
    activate LF

    loop Iterative Board Generation (1..num_iters)
        LF->>FBS: Randomize(deal_missing_cards)
        activate FBS
        FBS->>DEAL: DealMissingCards(known_hands, reqs)
        activate DEAL
        DEAL->>HR: MeetsRequirement(hand)
        activate HR
        HR-->>DEAL: true / false
        deactivate HR
        DEAL-->>FBS: Sampled hands
        deactivate DEAL
        FBS->>BS: SetHands(sampled_hands)
        activate BS
        BS-->>FBS: Initialized BoardState
        deactivate BS
        FBS-->>LF: BoardState instance
        deactivate FBS
    end

    loop Search & Play Tree Construction
        LF->>OPTree: FindOptimalPlayTree(board_state, min_tricks, cache)
        activate OPTree
        OPTree->>SC: Check saved_optimal_plays
        activate SC
        SC-->>OPTree: Cache hit or miss
        deactivate SC

        alt Cache Miss
            OPTree->>IDDS: DDSFindOptimalLines(board_state, min_tricks, cache)
            activate IDDS
            IDDS->>BS: PopulatePBNDeal(bs, &dealPBN)
            activate BS
            BS-->>IDDS: Formatted PBN deal
            deactivate BS
            IDDS->>DDS: SolveBoardPBN(dealPBN, target, solutions, mode)
            activate DDS
            DDS-->>IDDS: Solve results & card plays
            deactivate DDS
            IDDS->>SC: Store in SolverCache
            IDDS-->>OPTree: OptimalLines
            deactivate IDDS
        end
        OPTree-->>LF: OptimalPlayTree
        deactivate OPTree
    end

    LF->>GTF: RecordOptimalTrick(leader, trick, board_id)
    activate GTF
    GTF->>GTF: Aggregate tricks across boards
    GTF-->>LF: GeneralizedTricksVector
    deactivate GTF

    LF->>CL: GroupTricks & recursively build ConditionalLine
    activate CL
    CL->>CL: Instantiate PlayCondition branches (Winner/Trick)
    CL-->>LF: Root ConditionalLine
    deactivate CL

    loop Cross-Hand Deal Set Summarization
        LF->>CHDS: Summarize(covered_deals, "hand 1", "hand 3")
        activate CHDS
        CHDS->>CHDS: Extract joint suit bounds & honor stats
        CHDS-->>LF: CrossHandSummaryReport
        deactivate CHDS
        LF->>CL: set_summary(report)
        activate CL
        CL-->>LF: Updated ConditionalLine
        deactivate CL
    end

    LF-->>Client: std::vector<ConditionalLine>
    deactivate LF
```

---

## 3. Module Overview Tables

The system modules are categorized below by their primary architectural layer, header file location, responsibility, and public interface routines.

### 3.1 Analytical Engines Layer

| Module Name | Header File | Architectural Responsibility | Key Public Interface Routines |
| :--- | :--- | :--- | :--- |
| **LineFinder** | `line_finder.h` | High-level search engine for discovering optimal probabilistic lines of play and opening leads across randomized deals. | - `Solve()`: Evaluates and generates conditional lines of play.<br/>- `FindBestLead()`: Identifies candidate opening leads.<br/>- `FindTricksWon()`: Computes trick distribution statistics. |
| **FuzzyBoardState** | `line_finder.h` | Represents partially specified bridge deals with unknown card locations and hand shape/hcp constraints. | - `SetHands()`: Sets known hand holdings.<br/>- `SetRequirements()`: Applies hand constraints.<br/>- `Randomize()`: Generates concrete deal instances. |
| **ConditionalLine** | `conditional_line.h` | Decision tree structure representing branching lines of play conditioned on opponent actions or trick outcomes. | - `AddCondition()`: Adds a conditional branch.<br/>- `SetAction()`: Sets the initial generalized trick action.<br/>- `Describe()`: Generates human-readable line output. |
| **GeneralizedTrickFinder & DemiPlay** | `conditional_line.h` | Aggregates and simplifies specific card plays into generalized suit/rank trick structures across board samples. | - `RecordOptimalTrick()`: Records trick results.<br/>- `GetGeneralizedTricks()`: Extracts top generalized tricks.<br/>- `GroupTricks()`: Groups compatible tricks. |
| **PlayCondition** | `conditional_line.h` | Base class and implementations (`WinnerPlayCondition`, `TrickPlayCondition`) for evaluating decision tree branch conditions. | - `Describe()`: Formats condition description string. |
| **CrossHandDealSetSummarizer** | `cross_hand_summarizer.h` | Analyzes sets of paired deals across covered board states to compute joint suit distribution bounds and honor card possession statistics. | - `Summarize()`: Generates `CrossHandSummaryReport` for paired deal sets.<br/>- `JointSuitRuleExtractor`: Extracts suit length bounds across deals.<br/>- `HonorPossessionClassifier`: Computes honor card distribution statistics. |

### 3.2 Solvers & Strategy Layer

| Module Name | Header File | Architectural Responsibility | Key Public Interface Routines |
| :--- | :--- | :--- | :--- |
| **OptimalPlayTree** | `interface_dds.h` | Recursive game tree representation of optimal double-dummy card plays and trick continuations. | - `FindOptimalPlayTree()`: Factory method to construct play tree.<br/>- `Deepen()`: Expands tree depth.<br/>- `PlayTrick()`: Advances tree state. |
| **DDS C++ Interface** | `interface_dds.h` | Higher-level bridge solver interface wrapping low-level C-DDS calls, parallel solving, and PBN conversion. | - `DDSFindOptimalLines()`: Finds winning lines for target tricks.<br/>- `DDSParallelSolve()`: Batch solves multiple board states.<br/>- `PopulatePBNDeal()`: Converts board state to PBN deal. |
| **Solver** | `solver.h` | Native search solver routines for determining maximum trick yield and optimal line sequences. | - `MostTricksCanBeWon()`: Calculates max tricks for a board state.<br/>- `PrintLine()` / `PrintLines()`: Formats optimal plays. |
| **SolverCache** | `solver.h` | Hash-map based memoization cache for solver outcomes, optimal play sets, and DDS lookups. | - `saved_results`: Cached trick counts.<br/>- `saved_optimal_plays`: Cached play lines.<br/>- `dds_size()`: Returns total cached entries. |

### 3.3 State Management Layer

| Module Name | Header File | Architectural Responsibility | Key Public Interface Routines |
| :--- | :--- | :--- | :--- |
| **BoardState** | `board_state.h` | Central state machine representing a bridge board, including hands, played cards, current trick, trump, and leader. | - `PlayCard()`: Advances state by playing a card.<br/>- `ValidCardsToPlay()`: Computes legal moves.<br/>- `TrickWinner()`: Determines current trick winner.<br/>- `GetPBNCards()` / `ConvertHandToPBN()`: Formats board for PBN solvers.<br/>- `Finished()`: Checks if board is completed. |

### 3.4 Core Data Primitives Layer

| Module Name | Header File | Architectural Responsibility | Key Public Interface Routines |
| :--- | :--- | :--- | :--- |
| **Card** | `card.h` | Represents 64-bit card bitmasks (`CardType`), suit enums (`Suit`), player directions (`PlayerDirection`), and formatting. | - `GetSuitedCard()`: Retrieves card bitmask.<br/>- `ParseStringCard()` / `ParseSuit()`: String parsing.<br/>- `PrintCard()` / `DecodeCard()`: Card string representation. |
| **Hand** | `hand.h` | Bitmanipulation routines for evaluating 64-bit hand masks, card extraction, HCP calculation, and suit distributions. | - `CardsInHand()` / `CardsInHandInSuit()`: Unpacks card vectors.<br/>- `ParseHand()` / `MakeHandFromCards()`: Hand creation.<br/>- `GetHighCardPoints()`: Calculates HCP.<br/>- `GetSuitDistribution()`: Computes suit counts. |
| **HandRequirements** | `hand_requirements.h` | Polymorphic constraint system (`SuitAtLeast`, `SuitLength`, `BalancedHand`, `AntiRequirement`, `HighCardRange`, `HandHasCard`, `AtLeastAces`) for hand filtering. | - `MeetsRequirement()`: Evaluates whether a hand satisfies specific shape, HCP, or card constraints. |
| **Deal** | `deal.h` | Random deal generator, conditional hand dealer, and key-value file configuration parser. | - `LoadDealFromKeyValueFile()`: Parses plain-text deal files.<br/>- `DealHands()` / `DealTwoHands()`: Random card allocation.<br/>- `DealMissingCards()` / `DealTwoRequiredHands()`: Constraint-satisfied dealing. |

### 3.5 Performance Utilities Layer

| Module Name | Header File | Architectural Responsibility | Key Public Interface Routines |
| :--- | :--- | :--- | :--- |
| **line_finder_perf** | `perf/line_finder_perf.cc` | Standalone performance benchmark binary evaluating `LineFinder::Solve` search scaling and timing metrics across deal ending sizes (6 to 13 cards per hand). | - `main()`: Executes benchmark trials across deal ending sizes.<br/>- `GetHandSuitDistribution()`: Computes suit length profiles for deal setup. |

---

## 4. Architectural Design Principles

1. **Separation of Concerns**:
   - High-level decision tree search (`LineFinder`) is completely decoupled from double-dummy evaluation engines (`DDS C++ Wrapper` & `Solver Engine`).
   - Board mutation (`BoardState`) enforces standard bridge rules (following suit, trick evaluation) independently of search algorithms.

2. **Stochastic Sampling & Generalization**:
   - In bridge with incomplete information, `LineFinder` uses `FuzzyBoardState` to sample concrete deals consistent with player constraints (`HandRequirements`).
   - Specific trick lines generated across sample boards are normalized and clustered using `GeneralizedTrickFinder` and `GeneralizedDemiPlay` into actionable `ConditionalLine` decision trees.

3. **Performance Optimization via Memoization**:
   - `SolverCache` eliminates redundant double-dummy calculations across identical remaining card holdings, dramatically speeding up multi-deal sampling.
