#!/usr/bin/env python3
"""Applies subset-deal table-reuse optimization to DDS v2.9.0 sources.

In DDS's SolveBoard, SetDealTables(thrp) + TransTableL::Init(handLookup)
initializes lookup tables (thrp->rel and transTable->aggr) for all 8192
possible 13-rank bitmasks against the remaining cards in the deal. When
exploring mid-trick plays (2nd, 3rd, and 4th cards of a trick) or subsequent
tricks of the same deal, the remaining cards in all four hands are a strict
subset of the cards for which thrp->rel and transTable->aggr were built. Since
those tables are indexed by rank bitmask, removing already-played cards never
invalidates entries for any subset of the remaining cards.

This script patches Memory.h and SolverIF.cpp so each DDS thread tracks the
card bitmask (tablesSuit) for which SetDealTables was last computed, and skips
recomputing SetDealTables when the new deal's remaining cards are a subset of
tablesSuit.
"""

import pathlib
import sys


def patch_memory_h(memory_h: pathlib.Path) -> None:
  text = memory_h.read_text(encoding="utf-8")
  if "dealTablesValid" in text:
    return
  nl = "\r\n" if "\r\n" in text else "\n"
  target = "  unsigned short int suit[DDS_HANDS][DDS_SUITS];"
  replacement = (
      "  unsigned short int suit[DDS_HANDS][DDS_SUITS];" + nl +
      "  bool dealTablesValid = false;" + nl +
      "  unsigned short int tablesSuit[DDS_HANDS][DDS_SUITS];"
  )
  if target not in text:
    raise RuntimeError(f"Could not find target line in {memory_h}")
  memory_h.write_text(text.replace(target, replacement, 1), encoding="utf-8")


def patch_solver_if_cpp(solver_if: pathlib.Path) -> None:
  text = solver_if.read_text(encoding="utf-8")
  if "dealTablesValid" in text:
    return
  nl = "\r\n" if "\r\n" in text else "\n"
  normalized = text.replace("\r\n", "\n")
  target = """  bool newDeal = false;
  bool newTrump = false;
  unsigned diffDeal = 0;
  unsigned aggDeal = 0;
  bool similarDeal;
  int cardCount = 0;
  int ind, forb, noMoves;

  for (int h = 0; h < DDS_HANDS; h++)
  {
    for (int s = 0; s < DDS_SUITS; s++)
    {
      unsigned int c = dl.remainCards[h][s] >> 2;

      cardCount += counttable[c];
      diffDeal += (c ^ (thrp->suit[h][s]));
      aggDeal += c;

      if (thrp->suit[h][s] != c)
      {
        thrp->suit[h][s] = static_cast<unsigned short>(c);
        newDeal = true;
      }
    }
  }"""

  replacement = """  bool newDeal = false;
  bool newTrump = false;
  bool subsetOfTables = thrp->dealTablesValid;
  unsigned diffDeal = 0;
  unsigned aggDeal = 0;
  bool similarDeal;
  int cardCount = 0;
  int ind, forb, noMoves;

  for (int h = 0; h < DDS_HANDS; h++)
  {
    for (int s = 0; s < DDS_SUITS; s++)
    {
      unsigned int c = dl.remainCards[h][s] >> 2;

      cardCount += counttable[c];
      diffDeal += (c ^ (thrp->suit[h][s]));
      aggDeal += c;

      if (c & ~static_cast<unsigned int>(thrp->tablesSuit[h][s]))
        subsetOfTables = false;

      if (thrp->suit[h][s] != c)
      {
        thrp->suit[h][s] = static_cast<unsigned short>(c);
        newDeal = true;
      }
    }
  }"""

  target2 = """  if (newDeal)
  {
    SetDeal(thrp);
    SetDealTables(thrp);
  }"""

  replacement2 = """  if (newDeal)
  {
    SetDeal(thrp);
    if (!subsetOfTables)
    {
      SetDealTables(thrp);
      thrp->dealTablesValid = true;
      for (int h = 0; h < DDS_HANDS; h++)
        for (int s = 0; s < DDS_SUITS; s++)
          thrp->tablesSuit[h][s] = thrp->suit[h][s];
    }
  }"""

  if target not in normalized or target2 not in normalized:
    raise RuntimeError(f"Could not find target block in {solver_if}")

  patched = normalized.replace(target, replacement, 1).replace(
      target2, replacement2, 1
  )
  if nl == "\r\n":
    patched = patched.replace("\n", "\r\n")
  solver_if.write_text(patched, encoding="utf-8")


def patch_system_cpp(system_cpp: pathlib.Path) -> None:
  text = system_cpp.read_text(encoding="utf-8")
  if "__EMSCRIPTEN__" in text:
    return
  nl = "\r\n" if "\r\n" in text else "\n"
  target = "  (void) System::GetCores(ncores);"
  replacement = (
      "  (void) System::GetCores(ncores);" + nl + nl +
      "#ifdef __EMSCRIPTEN__" + nl +
      "  kilobytesFree = 512ULL * 1024ULL;" + nl +
      "  ncores = 1;" + nl +
      "  return;" + nl +
      "#endif"
  )
  if target not in text:
    raise RuntimeError(f"Could not find target line in {system_cpp}")
  system_cpp.write_text(text.replace(target, replacement, 1), encoding="utf-8")


def main() -> int:
  if len(sys.argv) != 2:
    print(f"Usage: {sys.argv[0]} <dds_source_dir>", file=sys.stderr)
    return 1
  dds_dir = pathlib.Path(sys.argv[1])
  patch_memory_h(dds_dir / "src" / "Memory.h")
  patch_solver_if_cpp(dds_dir / "src" / "SolverIF.cpp")
  patch_system_cpp(dds_dir / "src" / "System.cpp")
  return 0


if __name__ == "__main__":
  sys.exit(main())

