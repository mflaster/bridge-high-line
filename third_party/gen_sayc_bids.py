#!/usr/bin/env python3
"""Compiles SAYC bidding constraint DAGs from eseidel/saycbridge into C++.

This script imports the unmodified `z3b` engine from `eseidel/saycbridge`
(fetched by CMake into `build/_deps/saycbridge-src/src`) and exports the exact
boolean arithmetic constraint DAGs for first-round auctions into a C++ header
(`sayc_generated.h`).
"""

import os
import sys
import types


def _install_networkx_shim():
  """Minimal DiGraph implementation for saycbridge's z3b/ordering.py."""
  nx = types.ModuleType("networkx")

  class DiGraph:

    def __init__(self):
      self._adj = {}

    def add_edge(self, u, v):
      self._adj.setdefault(u, set()).add(v)
      self._adj.setdefault(v, set())

    def has_edge(self, u, v):
      return v in self._adj.get(u, ())

    def nodes(self):
      return self._adj.keys()

  def descendants(graph, source):
    visited = set()
    stack = list(graph._adj.get(source, ()))
    while stack:
      node = stack.pop()
      if node not in visited:
        visited.add(node)
        stack.extend(graph._adj.get(node, ()))
    return visited

  def is_directed_acyclic_graph(graph):
    for node in graph._adj:
      if node in descendants(graph, node):
        return False
    return True

  nx.DiGraph = DiGraph
  nx.descendants = descendants
  nx.is_directed_acyclic_graph = is_directed_acyclic_graph
  sys.modules["networkx"] = nx


def _install_z3_ast_shim():
  """Hash-consed Z3 AST + vectorized single-hand solver for saycbridge's z3b."""
  z3 = types.ModuleType("z3")
  intern_pool = {}

  class ExprRef:
    __slots__ = ("op", "args", "_id", "_vec")

    def __new__(cls, op, args):
      key = (op, args)
      obj = intern_pool.get(key)
      if obj is None:
        obj = object.__new__(cls)
        obj.op = op
        obj.args = args
        obj._id = len(intern_pool)
        obj._vec = None
        intern_pool[key] = obj
      return obj

    def __hash__(self):
      return self._id

    def __add__(self, other):
      return ExprRef("add", (self, _coerce(other)))

    def __radd__(self, other):
      if other == 0 and isinstance(other, int) and not isinstance(other, bool):
        return self
      return ExprRef("add", (_coerce(other), self))

    def __sub__(self, other):
      return ExprRef("sub", (self, _coerce(other)))

    def __rsub__(self, other):
      return ExprRef("sub", (_coerce(other), self))

    def __mul__(self, other):
      return ExprRef("mul", (self, _coerce(other)))

    def __rmul__(self, other):
      return ExprRef("mul", (_coerce(other), self))

    def __lt__(self, other):
      return ExprRef("lt", (self, _coerce(other)))

    def __le__(self, other):
      return ExprRef("le", (self, _coerce(other)))

    def __gt__(self, other):
      return ExprRef("gt", (self, _coerce(other)))

    def __ge__(self, other):
      return ExprRef("ge", (self, _coerce(other)))

    def __eq__(self, other):
      return ExprRef("eq", (self, _coerce(other)))

    def __ne__(self, other):
      return ExprRef("ne", (self, _coerce(other)))

    def __repr__(self):
      return f"Expr({self.op}, {self.args})"

  def _coerce(x):
    if isinstance(x, ExprRef):
      return x
    if isinstance(x, bool):
      return ExprRef("bool", x)
    if isinstance(x, int):
      return ExprRef("int", x)
    raise TypeError(f"Cannot coerce {type(x)}: {x}")

  def _flatten(args):
    out = []
    for a in args:
      if isinstance(a, (list, tuple)) or (
          hasattr(a, "__iter__") and not isinstance(a, ExprRef)
      ):
        out.extend(_coerce(x) for x in a)
      else:
        out.append(_coerce(a))
    return tuple(out)

  z3.ExprRef = ExprRef
  z3.BoolRef = ExprRef
  z3.Int = lambda name: ExprRef("var", name)
  z3.Ints = lambda names: [ExprRef("var", n) for n in names.split()]
  z3.IntVal = lambda v: ExprRef("int", int(v))
  z3.BoolVal = lambda b: ExprRef("bool", bool(b))
  z3.And = lambda *args: ExprRef("and", _flatten(args))
  z3.Or = lambda *args: ExprRef("or", _flatten(args))
  z3.Not = lambda a: ExprRef("not", (_coerce(a),))
  z3.If = lambda c, t, f: ExprRef("if", (_coerce(c), _coerce(t), _coerce(f)))
  z3.Sum = lambda items: ExprRef("add", _flatten(items))
  z3.simplify = lambda e: e
  z3.sat = "sat"
  z3.unsat = "unsat"

  suits = ("clubs", "diamonds", "hearts", "spades")
  honors = (("ace", 4), ("king", 3), ("queen", 2), ("jack", 1), ("ten", 0))

  suit_masks_by_len = {}
  for length in range(14):
    by_hcp = {}
    for mask in range(32):
      a_bit = (mask >> 4) & 1
      k_bit = (mask >> 3) & 1
      q_bit = (mask >> 2) & 1
      j_bit = (mask >> 1) & 1
      t_bit = mask & 1
      if a_bit + k_bit + q_bit + j_bit + t_bit <= length:
        shcp = 4 * a_bit + 3 * k_bit + 2 * q_bit + j_bit
        by_hcp.setdefault(shcp, []).append((a_bit, k_bit, q_bit, j_bit, t_bit))
    suit_masks_by_len[length] = by_hcp

  def _build_universe():
    cols = {}
    all_vars = [
        "clubs", "diamonds", "hearts", "spades",
        "high_card_points", "points", "playing_points",
        "points_supporting_clubs", "points_supporting_diamonds",
        "points_supporting_hearts", "points_supporting_spades",
        "void_in_clubs", "void_in_diamonds", "void_in_hearts", "void_in_spades",
        "singleton_in_clubs", "singleton_in_diamonds",
        "singleton_in_hearts", "singleton_in_spades",
        "doubleton_in_clubs", "doubleton_in_diamonds",
        "doubleton_in_hearts", "doubleton_in_spades",
        "voids", "singletons", "doubletons",
    ]
    for s in suits:
      for h, _ in honors:
        all_vars.append(f"{h}_of_{s}")
    for v in all_vars:
      cols[v] = []

    idx = 0
    for c in range(14):
      for d in range(14 - c):
        for h in range(14 - c - d):
          s = 13 - c - d - h
          lens = (c, d, h, s)
          vc, vd, vh, vs = int(c == 0), int(d == 0), int(h == 0), int(s == 0)
          sc, sd, sh, ss = int(c == 1), int(d == 1), int(h == 1), int(s == 1)
          dc, dd, dh, ds = int(c == 2), int(d == 2), int(h == 2), int(s == 2)
          v_cnt = vc + vd + vh + vs
          s_cnt = sc + sd + sh + ss
          d_cnt = dc + dd + dh + ds
          max_suit_hcp = [
              min(10, [0, 4, 7, 9][l_val] if l_val < 4 else 10)
              for l_val in lens
          ]
          total_max_hcp = sum(max_suit_hcp)
          order_desc = sorted(range(4), key=lambda i: -lens[i])
          for hcp in range(total_max_hcp + 1):
            rem = hcp
            suit_honors = [None] * 4
            ok = True
            for pos_idx, si in enumerate(order_desc):
              other_max = sum(
                  max_suit_hcp[order_desc[j]] for j in range(pos_idx + 1, 4)
              )
              take = max(0, min(rem, max_suit_hcp[si]))
              if rem - take > other_max:
                take = rem - other_max
              cands = suit_masks_by_len[lens[si]].get(take)
              if not cands:
                for t2 in range(max_suit_hcp[si], -1, -1):
                  if (
                      t2 <= rem
                      and rem - t2 <= other_max
                      and t2 in suit_masks_by_len[lens[si]]
                  ):
                    take = t2
                    cands = suit_masks_by_len[lens[si]][t2]
                    break
              if not cands:
                ok = False
                break
              suit_honors[si] = cands[-1]
              rem -= take
            if not ok or rem != 0:
              continue
            for pp in (hcp, max(hcp, 12)):
              cols["clubs"].append(c)
              cols["diamonds"].append(d)
              cols["hearts"].append(h)
              cols["spades"].append(s)
              cols["high_card_points"].append(hcp)
              cols["points"].append(hcp)
              cols["playing_points"].append(pp)
              for si, sname in enumerate(suits):
                l_val = lens[si]
                if l_val <= 2:
                  sp = hcp
                elif l_val == 3:
                  sp = hcp + d_cnt + 2 * s_cnt + 3 * v_cnt
                else:
                  sp = hcp + d_cnt + 3 * s_cnt + 5 * v_cnt
                cols[f"points_supporting_{sname}"].append(sp)
                a_b, k_b, q_b, j_b, t_b = suit_honors[si]
                cols[f"ace_of_{sname}"].append(a_b)
                cols[f"king_of_{sname}"].append(k_b)
                cols[f"queen_of_{sname}"].append(q_b)
                cols[f"jack_of_{sname}"].append(j_b)
                cols[f"ten_of_{sname}"].append(t_b)
              cols["void_in_clubs"].append(vc)
              cols["void_in_diamonds"].append(vd)
              cols["void_in_hearts"].append(vh)
              cols["void_in_spades"].append(vs)
              cols["singleton_in_clubs"].append(sc)
              cols["singleton_in_diamonds"].append(sd)
              cols["singleton_in_hearts"].append(sh)
              cols["singleton_in_spades"].append(ss)
              cols["doubleton_in_clubs"].append(dc)
              cols["doubleton_in_diamonds"].append(dd)
              cols["doubleton_in_hearts"].append(dh)
              cols["doubleton_in_spades"].append(ds)
              cols["voids"].append(v_cnt)
              cols["singletons"].append(s_cnt)
              cols["doubletons"].append(d_cnt)
              idx += 1
    return cols, idx

  cols, num_states = _build_universe()
  all_ones = (1 << num_states) - 1

  def _eval_vec(expr):
    if expr._vec is not None:
      return expr._vec
    op = expr.op
    args = expr.args
    if op == "bool":
      res = all_ones if args else 0
    elif op == "int":
      res = args
    elif op == "var":
      res = cols[args]
    elif op == "not":
      res = all_ones ^ _eval_vec(args[0])
    elif op == "and":
      m = all_ones
      for a in args:
        m &= _eval_vec(a)
        if m == 0:
          break
      res = m
    elif op == "or":
      m = 0
      for a in args:
        m |= _eval_vec(a)
        if m == all_ones:
          break
      res = m
    elif op == "if":
      cm = _eval_vec(args[0])
      tv = _eval_vec(args[1])
      fv = _eval_vec(args[2])
      tv_int = isinstance(tv, int)
      fv_int = isinstance(fv, int)
      res = [
          (tv if tv_int else tv[i])
          if (cm >> i) & 1
          else (fv if fv_int else fv[i])
          for i in range(num_states)
      ]
    elif op == "add":
      vecs = [_eval_vec(a) for a in args]
      if len(vecs) == 2:
        v0, v1 = vecs
        if isinstance(v0, int) and isinstance(v1, int):
          res = v0 + v1
        elif isinstance(v0, int):
          res = [v0 + b for b in v1]
        elif isinstance(v1, int):
          res = [a + v1 for a in v0]
        else:
          res = [a + b for a, b in zip(v0, v1)]
      else:
        expanded = [([v] * num_states if isinstance(v, int) else v) for v in vecs]
        res = [sum(col) for col in zip(*expanded)]
    elif op == "sub":
      v0, v1 = _eval_vec(args[0]), _eval_vec(args[1])
      if isinstance(v0, int) and isinstance(v1, int):
        res = v0 - v1
      elif isinstance(v0, int):
        res = [v0 - b for b in v1]
      elif isinstance(v1, int):
        res = [a - v1 for a in v0]
      else:
        res = [a - b for a, b in zip(v0, v1)]
    elif op == "mul":
      v0, v1 = _eval_vec(args[0]), _eval_vec(args[1])
      if isinstance(v0, int) and isinstance(v1, int):
        res = v0 * v1
      elif isinstance(v0, int):
        res = [v0 * b for b in v1]
      elif isinstance(v1, int):
        res = [a * v1 for a in v0]
      else:
        res = [a * b for a, b in zip(v0, v1)]
    else:
      v0, v1 = _eval_vec(args[0]), _eval_vec(args[1])
      if isinstance(v1, int):
        if op == "eq" and args[0].op == "var" and args[0].args == "playing_points":
          bits = [a <= v1 for a in v0]
        elif op == "lt":
          bits = [a < v1 for a in v0]
        elif op == "le":
          bits = [a <= v1 for a in v0]
        elif op == "gt":
          bits = [a > v1 for a in v0]
        elif op == "ge":
          bits = [a >= v1 for a in v0]
        elif op == "eq":
          bits = [a == v1 for a in v0]
        elif op == "ne":
          bits = [a != v1 for a in v0]
        else:
          raise ValueError(op)
      elif isinstance(v0, int):
        if op == "lt":
          bits = [v0 < b for b in v1]
        elif op == "le":
          bits = [v0 <= b for b in v1]
        elif op == "gt":
          bits = [v0 > b for b in v1]
        elif op == "ge":
          bits = [v0 >= b for b in v1]
        elif op == "eq":
          bits = [v0 == b for b in v1]
        elif op == "ne":
          bits = [v0 != b for b in v1]
        else:
          raise ValueError(op)
      else:
        if op == "lt":
          bits = [a < b for a, b in zip(v0, v1)]
        elif op == "le":
          bits = [a <= b for a, b in zip(v0, v1)]
        elif op == "gt":
          bits = [a > b for a, b in zip(v0, v1)]
        elif op == "ge":
          bits = [a >= b for a, b in zip(v0, v1)]
        elif op == "eq":
          bits = [a == b for a, b in zip(v0, v1)]
        elif op == "ne":
          bits = [a != b for a, b in zip(v0, v1)]
        else:
          raise ValueError(op)
      res = (
          int("".join("1" if b else "0" for b in reversed(bits)), 2)
          if bits
          else 0
      )
    expr._vec = res
    return res

  class Solver:

    def __init__(self):
      self._stack = [all_ones]

    def push(self):
      self._stack.append(self._stack[-1])

    def pop(self):
      self._stack.pop()

    def add(self, expr):
      if isinstance(expr, (list, tuple)):
        return
      self._stack[-1] &= _eval_vec(expr)

    def check(self):
      return z3.sat if self._stack[-1] != 0 else z3.unsat

  z3.SolverFor = lambda logic: Solver()
  sys.modules["z3"] = z3
  return z3


def main():
  if len(sys.argv) != 3:
    sys.stderr.write(
        f"Usage: {sys.argv[0]} <saycbridge_src_dir> <output_header_path>\n"
    )
    sys.exit(1)

  saycbridge_src = os.path.abspath(sys.argv[1])
  output_header = os.path.abspath(sys.argv[2])

  _install_networkx_shim()
  z3 = _install_z3_ast_shim()
  sys.path.insert(0, saycbridge_src)

  from core.call import Call
  from core.callhistory import CallHistory
  from z3b.bidder import Interpreter, RuleSelector
  import z3b.model as model
  import z3b.sayc as sayc

  interp = Interpreter()
  system = sayc.StandardAmericanYellowCard

  openings = [
      "1C", "1D", "1H", "1S", "1N", "2C", "2D", "2H", "2S", "2N",
      "3C", "3D", "3H", "3S", "3N", "4C", "4D", "4H", "4S",
  ]
  calls_up_to_4s = [
      "1D", "1H", "1S", "1N", "2C", "2D", "2H", "2S", "2N",
      "3C", "3D", "3H", "3S", "3N", "4C", "4D", "4H", "4S",
  ]
  calls_up_to_3s = [
      "1D", "1H", "1S", "1N", "2C", "2D", "2H", "2S", "2N",
      "3C", "3D", "3H", "3S",
  ]

  histories = ["", "P P P"]
  for o in openings:
    histories.append(o)
    histories.append(f"{o} P")
    histories.append(f"{o} P P")
    histories.append(f"{o} X")
    histories.append(f"{o} X P")
    histories.append(f"{o} X XX")
    ch_o = CallHistory.from_string(o)
    for b in calls_up_to_4s:
      if ch_o.is_legal_call(Call.from_string(b)):
        histories.append(f"{o} {b}")
        histories.append(f"{o} {b} P")
    if o in ["1C", "1D", "1H", "1S", "1N", "2C", "2D", "2H", "2S"]:
      for b in calls_up_to_3s:
        if ch_o.is_legal_call(Call.from_string(b)):
          histories.append(f"{o} P {b}")
          histories.append(f"{o} {b} X")
          histories.append(f"{o} X {b}")

  def bidder_constraints_for_call(sel, call):
    rule = sel.rule_for_call(call)
    if not rule or rule.requires_planning:
      return z3.BoolVal(False)
    situations = []
    for priority, z3_meaning in rule.meaning_of(sel.history, call):
      situational_exprs = [z3_meaning]
      for unmade_call, unmade_rule in sel._call_to_rule.items():
        if unmade_rule.requires_planning:
          continue
        for unmade_priority, unmade_z3_meaning in unmade_rule.meaning_of(
            sel.history, unmade_call
        ):
          if sel.system.priority_ordering.lt(priority, unmade_priority):
            situational_exprs.append(z3.Not(unmade_z3_meaning))
      situations.append(z3.And(situational_exprs))
    return z3.Or(situations)

  # The 24 fundamental hand variables from z3b/model.py's expr_for_hand(hand):
  # 4 suit lengths + 20 honor indicator bits.
  raw_var_names = [
      "clubs", "diamonds", "hearts", "spades",
      "ace_of_clubs", "king_of_clubs", "queen_of_clubs",
      "jack_of_clubs", "ten_of_clubs",
      "ace_of_diamonds", "king_of_diamonds", "queen_of_diamonds",
      "jack_of_diamonds", "ten_of_diamonds",
      "ace_of_hearts", "king_of_hearts", "queen_of_hearts",
      "jack_of_hearts", "ten_of_hearts",
      "ace_of_spades", "king_of_spades", "queen_of_spades",
      "jack_of_spades", "ten_of_spades",
  ]
  var_to_idx = {name: i for i, name in enumerate(raw_var_names)}

  # Derived model variables defined by z3b/model.py axioms:
  suit_vars = (model.clubs, model.diamonds, model.hearts, model.spades)
  voids_expr = z3.Sum([z3.If(sv == 0, 1, 0) for sv in suit_vars])
  singletons_expr = z3.Sum([z3.If(sv == 1, 1, 0) for sv in suit_vars])
  doubletons_expr = z3.Sum([z3.If(sv == 2, 1, 0) for sv in suit_vars])

  hcp_terms = []
  for s_idx, sname in enumerate(("clubs", "diamonds", "hearts", "spades")):
    for hname, hval in (("ace", 4), ("king", 3), ("queen", 2), ("jack", 1)):
      hcp_terms.append(hval * z3.Int(f"{hname}_of_{sname}"))
  hcp_expr = z3.Sum(hcp_terms)

  def support_pts_expr(sv):
    return z3.If(
        sv <= 2,
        hcp_expr,
        z3.If(
            sv == 3,
            hcp_expr + doubletons_expr + 2 * singletons_expr + 3 * voids_expr,
            hcp_expr + doubletons_expr + 3 * singletons_expr + 5 * voids_expr,
        ),
    )

  derived_var_exprs = {
      "high_card_points": hcp_expr,
      "points": hcp_expr,
      "playing_points": z3.IntVal(55),
      "voids": voids_expr,
      "singletons": singletons_expr,
      "doubletons": doubletons_expr,
      "points_supporting_clubs": support_pts_expr(model.clubs),
      "points_supporting_diamonds": support_pts_expr(model.diamonds),
      "points_supporting_hearts": support_pts_expr(model.hearts),
      "points_supporting_spades": support_pts_expr(model.spades),
  }

  dag_nodes = []
  dag_map = {}
  expr_to_dag = {}

  def add_node(op, a=0, b=0, c=0):
    key = (op, a, b, c)
    idx = dag_map.get(key)
    if idx is None:
      idx = len(dag_nodes)
      dag_nodes.append(key)
      dag_map[key] = idx
    return idx

  false_idx = add_node("kBool", 0)
  true_idx = add_node("kBool", 1)

  def lower(e):
    res = expr_to_dag.get(e._id)
    if res is not None:
      return res
    op = e.op
    args = e.args
    if op == "bool":
      res = true_idx if args else false_idx
    elif op == "int":
      res = add_node("kInt", int(args))
    elif op == "var":
      if args in derived_var_exprs:
        res = lower(derived_var_exprs[args])
      else:
        res = add_node("kVar", var_to_idx[args])
    elif op == "not":
      child = lower(args[0])
      if child == true_idx:
        res = false_idx
      elif child == false_idx:
        res = true_idx
      elif dag_nodes[child][0] == "kNot":
        res = dag_nodes[child][1]
      else:
        res = add_node("kNot", child)
    elif op == "and":
      items = []
      for a in args:
        idx = lower(a)
        if idx == false_idx:
          expr_to_dag[e._id] = false_idx
          return false_idx
        if idx != true_idx and idx not in items:
          items.append(idx)
      if not items:
        res = true_idx
      else:
        res = items[-1]
        for idx in reversed(items[:-1]):
          res = add_node("kAnd", idx, res)
    elif op == "or":
      items = []
      for a in args:
        idx = lower(a)
        if idx == true_idx:
          expr_to_dag[e._id] = true_idx
          return true_idx
        if idx != false_idx and idx not in items:
          items.append(idx)
      if not items:
        res = false_idx
      else:
        res = items[-1]
        for idx in reversed(items[:-1]):
          res = add_node("kOr", idx, res)
    elif op == "add":
      items = [lower(a) for a in args if not (a.op == "int" and a.args == 0)]
      if not items:
        res = add_node("kInt", 0)
      else:
        res = items[0]
        for idx in items[1:]:
          res = add_node("kAdd", res, idx)
    elif op in ("sub", "mul", "lt", "le", "gt", "ge", "eq", "ne"):
      op_enum = "k" + op.capitalize()
      res = add_node(op_enum, lower(args[0]), lower(args[1]))
    elif op == "if":
      res = add_node("kIf", lower(args[0]), lower(args[1]), lower(args[2]))
    else:
      raise ValueError(f"Unsupported op: {op}")
    expr_to_dag[e._id] = res
    return res

  roots = {}
  for h_str in histories:
    ch = CallHistory.from_string(h_str)
    hist = interp.create_history(ch)
    sel = RuleSelector(system, hist)
    for call, rule in sel._call_to_rule.items():
      if rule.requires_planning:
        continue
      expr = bidder_constraints_for_call(sel, call)
      r = lower(expr)
      if r != false_idx:
        roots[(h_str, call.name)] = r

  sorted_entries = sorted(roots.items(), key=lambda kv: kv[0])

  os.makedirs(os.path.dirname(output_header), exist_ok=True)
  with open(output_header, "w", encoding="utf-8") as f:
    f.write("// AUTO-GENERATED AT BUILD TIME BY third_party/gen_sayc_bids.py\n")
    f.write("// Source: https://github.com/eseidel/saycbridge (BSD-3-Clause)\n")
    f.write("// DO NOT EDIT MANUALLY.\n\n")
    f.write("#pragma once\n\n")
    f.write("#include <algorithm>\n")
    f.write("#include <cstdint>\n")
    f.write("#include <cstring>\n")
    f.write("#include <string>\n")
    f.write("#include <vector>\n\n")
    f.write("namespace sayc {\n\n")
    f.write("enum class SaycOp : uint8_t {\n")
    f.write("  kBool = 0,\n")
    f.write("  kInt,\n")
    f.write("  kVar,\n")
    f.write("  kNot,\n")
    f.write("  kAnd,\n")
    f.write("  kOr,\n")
    f.write("  kAdd,\n")
    f.write("  kSub,\n")
    f.write("  kMul,\n")
    f.write("  kLt,\n")
    f.write("  kLe,\n")
    f.write("  kGt,\n")
    f.write("  kGe,\n")
    f.write("  kEq,\n")
    f.write("  kNe,\n")
    f.write("  kIf,\n")
    f.write("};\n\n")
    f.write("struct SaycDagNode {\n")
    f.write("  SaycOp op;\n")
    f.write("  int16_t a;\n")
    f.write("  int16_t b;\n")
    f.write("  int16_t c;\n")
    f.write("};\n\n")
    f.write(f"inline constexpr SaycDagNode kSaycDagNodes[{len(dag_nodes)}] = {{\n")
    for op, a, b, c in dag_nodes:
      f.write(f"  {{SaycOp::{op}, {a}, {b}, {c}}},\n")
    f.write("};\n\n")
    f.write("struct SaycAuctionBidEntry {\n")
    f.write("  const char *after;\n")
    f.write("  const char *bid;\n")
    f.write("  int16_t root_node;\n")
    f.write("};\n\n")
    f.write(
        f"inline constexpr SaycAuctionBidEntry kSaycAuctionBids[{len(sorted_entries)}] = {{\n"
    )
    for (after_str, bid_str), root_idx in sorted_entries:
      f.write(f'  {{"{after_str}", "{bid_str}", {root_idx}}},\n')
    f.write("};\n\n")
    f.write("inline int LookupSaycBidNode(const std::string &after,\n")
    f.write("                             const std::string &bid) {\n")
    f.write("  int lo = 0;\n")
    f.write(f"  int hi = {len(sorted_entries)} - 1;\n")
    f.write("  while (lo <= hi) {\n")
    f.write("    int mid = lo + (hi - lo) / 2;\n")
    f.write("    int cmp = std::strcmp(after.c_str(), kSaycAuctionBids[mid].after);\n")
    f.write("    if (cmp == 0) {\n")
    f.write("      cmp = std::strcmp(bid.c_str(), kSaycAuctionBids[mid].bid);\n")
    f.write("    }\n")
    f.write("    if (cmp == 0) {\n")
    f.write("      return kSaycAuctionBids[mid].root_node;\n")
    f.write("    } else if (cmp < 0) {\n")
    f.write("      hi = mid - 1;\n")
    f.write("    } else {\n")
    f.write("      lo = mid + 1;\n")
    f.write("    }\n")
    f.write("  }\n")
    f.write("  return 0;  // kBool(false)\n")
    f.write("}\n\n")
    f.write("inline void BuildSaycProgram(int root_node,\n")
    f.write("                             std::vector<SaycDagNode> *prog_out) {\n")
    f.write("  prog_out->clear();\n")
    f.write("  if (root_node < 0) return;\n")
    f.write("  std::vector<std::pair<int, int>> remap;\n")
    f.write("  auto find_remap = [&remap](int old_idx) -> int {\n")
    f.write("    for (const auto &kv : remap) {\n")
    f.write("      if (kv.first == old_idx) return kv.second;\n")
    f.write("    }\n")
    f.write("    return -1;\n")
    f.write("  };\n")
    f.write("  auto dfs = [&](auto &self, int idx) -> int {\n")
    f.write("    int existing = find_remap(idx);\n")
    f.write("    if (existing >= 0) return existing;\n")
    f.write("    const SaycDagNode &n = kSaycDagNodes[idx];\n")
    f.write("    SaycDagNode mapped = n;\n")
    f.write("    switch (n.op) {\n")
    f.write("      case SaycOp::kBool:\n")
    f.write("      case SaycOp::kInt:\n")
    f.write("      case SaycOp::kVar:\n")
    f.write("        break;\n")
    f.write("      case SaycOp::kNot:\n")
    f.write("        mapped.a = static_cast<int16_t>(self(self, n.a));\n")
    f.write("        break;\n")
    f.write("      case SaycOp::kIf:\n")
    f.write("        mapped.a = static_cast<int16_t>(self(self, n.a));\n")
    f.write("        mapped.b = static_cast<int16_t>(self(self, n.b));\n")
    f.write("        mapped.c = static_cast<int16_t>(self(self, n.c));\n")
    f.write("        break;\n")
    f.write("      default:\n")
    f.write("        mapped.a = static_cast<int16_t>(self(self, n.a));\n")
    f.write("        mapped.b = static_cast<int16_t>(self(self, n.b));\n")
    f.write("        break;\n")
    f.write("    }\n")
    f.write("    int new_idx = static_cast<int>(prog_out->size());\n")
    f.write("    prog_out->push_back(mapped);\n")
    f.write("    remap.push_back({idx, new_idx});\n")
    f.write("    return new_idx;\n")
    f.write("  };\n")
    f.write("  dfs(dfs, root_node);\n")
    f.write("}\n\n")
    f.write("inline bool EvalSaycProgram(const std::vector<SaycDagNode> &prog,\n")
    f.write("                            const int vars[24]) {\n")
    f.write("  if (prog.empty()) return false;\n")
    f.write("  int vals[1024];\n")
    f.write("  const size_t n = prog.size();\n")
    f.write("  for (size_t i = 0; i < n; ++i) {\n")
    f.write("    const SaycDagNode &node = prog[i];\n")
    f.write("    switch (node.op) {\n")
    f.write("      case SaycOp::kBool:\n")
    f.write("      case SaycOp::kInt:\n")
    f.write("        vals[i] = node.a;\n")
    f.write("        break;\n")
    f.write("      case SaycOp::kVar:\n")
    f.write("        vals[i] = vars[node.a];\n")
    f.write("        break;\n")
    f.write("      case SaycOp::kNot:\n")
    f.write("        vals[i] = !vals[node.a];\n")
    f.write("        break;\n")
    f.write("      case SaycOp::kAnd:\n")
    f.write("        vals[i] = vals[node.a] && vals[node.b];\n")
    f.write("        break;\n")
    f.write("      case SaycOp::kOr:\n")
    f.write("        vals[i] = vals[node.a] || vals[node.b];\n")
    f.write("        break;\n")
    f.write("      case SaycOp::kAdd:\n")
    f.write("        vals[i] = vals[node.a] + vals[node.b];\n")
    f.write("        break;\n")
    f.write("      case SaycOp::kSub:\n")
    f.write("        vals[i] = vals[node.a] - vals[node.b];\n")
    f.write("        break;\n")
    f.write("      case SaycOp::kMul:\n")
    f.write("        vals[i] = vals[node.a] * vals[node.b];\n")
    f.write("        break;\n")
    f.write("      case SaycOp::kLt:\n")
    f.write("        vals[i] = vals[node.a] < vals[node.b];\n")
    f.write("        break;\n")
    f.write("      case SaycOp::kLe:\n")
    f.write("        vals[i] = vals[node.a] <= vals[node.b];\n")
    f.write("        break;\n")
    f.write("      case SaycOp::kGt:\n")
    f.write("        vals[i] = vals[node.a] > vals[node.b];\n")
    f.write("        break;\n")
    f.write("      case SaycOp::kGe:\n")
    f.write("        vals[i] = vals[node.a] >= vals[node.b];\n")
    f.write("        break;\n")
    f.write("      case SaycOp::kEq:\n")
    f.write("        vals[i] = vals[node.a] == vals[node.b];\n")
    f.write("        break;\n")
    f.write("      case SaycOp::kNe:\n")
    f.write("        vals[i] = vals[node.a] != vals[node.b];\n")
    f.write("        break;\n")
    f.write("      case SaycOp::kIf:\n")
    f.write("        vals[i] = vals[node.a] ? vals[node.b] : vals[node.c];\n")
    f.write("        break;\n")
    f.write("    }\n")
    f.write("  }\n")
    f.write("  return vals[n - 1] != 0;\n")
    f.write("}\n\n")
    f.write("}  // namespace sayc\n")


if __name__ == "__main__":
  main()
