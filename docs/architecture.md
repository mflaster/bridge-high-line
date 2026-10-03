# Architecture Overview

The Bridge Solver is designed to analyze bridge hands and determine the optimal
line of play. This document outlines the core components and data flow of the
system.

## Core Responsibilities

The system is responsible for:

1. **Parsing Input:** Accepting raw bridge deal data and converting it into an
   internal board state representation.
2. **Analysis:** Exploring possible card plays using game-theoretic algorithms
   (e.g., minimax, alpha-beta pruning) to find optimal strategies.
3. **Output Generation:** Producing a set of recommended moves or structural
   analysis of the deal.

## Data Flow

### Primary Inputs

- **Bridge Layout / Deal State Data:** The solver takes in the complete state
  of a bridge deal. This includes the cards held by each player (North, South,
  East, West), the current contract, the trump suit, and the sequence of plays
  made so far.

### Processing

- The input data is processed by the internal `BoardState` and `Deal`
  components.
- The `Solver` engine iterates through possible moves, evaluating conditions
  using the `LineFinder` and `ConditionalLine` components.

### Primary Outputs

- **Optimal Moves / Structural Analysis:** The final output is an evaluation of
  the deal, detailing the optimal card to play next, the expected number of
  tricks the declarer can win, and any structural analysis of the hand's
  potential lines of play.
