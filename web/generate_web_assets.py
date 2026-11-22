#!/usr/bin/env python3
import os
import re
import subprocess
import sys


def read_utf8_bytes(path):
  with open(path, "rb") as f:
    data = f.read()
  data.decode("utf-8")
  if 0 in data:
    raise ValueError(f"Asset contains embedded NUL byte: {path}")
  return data


def format_byte(byte_val):
  if byte_val < 0x80:
    return f"0x{byte_val:02x}"
  return f"static_cast<char>(0x{byte_val:02x})"


def format_asset_definition(name, data):
  if not data:
    array_def = f"inline constexpr char {name}[] = {{0}};"
  else:
    tokens = [format_byte(b) for b in data] + ["0"]
    lines = []
    for i in range(0, len(tokens), 16):
      lines.append("    " + ", ".join(tokens[i : i + 16]))
    body = ",\n".join(lines)
    array_def = f"inline constexpr char {name}[] = {{\n{body}\n}};"
  view_def = (
      f"inline constexpr std::string_view {name}_VIEW(\n"
      f"    {name}, sizeof({name}) - 1);"
  )
  return f"{array_def}\n{view_def}"


def resolve_app_version(repo_dir):
  try:
    out = subprocess.check_output(
        ["git", "rev-parse", "--short=8", "HEAD"],
        cwd=repo_dir,
        stderr=subprocess.DEVNULL,
        text=True,
    ).strip()
    if re.fullmatch(r"[0-9a-fA-F]{7,40}", out):
      return out.lower()
  except (OSError, subprocess.SubprocessError):
    pass
  return "dev"


def main():
  script_dir = os.path.dirname(os.path.abspath(__file__))
  repo_dir = os.path.dirname(script_dir)
  static_dir = os.path.join(script_dir, "static")
  app_version = resolve_app_version(repo_dir)

  app_version_bytes = app_version.encode("utf-8")
  if 0 in app_version_bytes:
    raise ValueError("Resolved app_version contains embedded NUL byte")

  assets = [
      ("INDEX_HTML", os.path.join(static_dir, "index.html")),
      ("HELP_HTML", os.path.join(static_dir, "help.html")),
      ("STYLES_CSS", os.path.join(static_dir, "styles.css")),
      ("APP_JS", os.path.join(static_dir, "app.js")),
      ("SOLVER_API_JS", os.path.join(static_dir, "solver_api.js")),
      ("DEAL_SVG_JS", os.path.join(static_dir, "deal_svg.js")),
      ("ANALYSIS_TABS_JS", os.path.join(static_dir, "analysis_tabs.js")),
      ("PHOTO_SCANNER_JS", os.path.join(static_dir, "photo_scanner.js")),
      ("DECISION_TREE_JS", os.path.join(static_dir, "decision_tree.js")),
      (
          "TELEMETRY_AND_HELP_JS",
          os.path.join(static_dir, "telemetry_and_help.js"),
      ),
      ("PDF_EXPORT_JS", os.path.join(static_dir, "pdf_export.js")),
      ("PRESETS_JSON", os.path.join(script_dir, "presets.json")),
      (
          "BRIDGE_STRATEGY_MD",
          os.path.join(script_dir, "..", "docs", "bridge-strategy.md"),
      ),
      (
          "USER_GUIDE_MD",
          os.path.join(script_dir, "..", "docs", "user-guide.md"),
      ),
  ]
  out_path = (
      sys.argv[1]
      if len(sys.argv) > 1
      else os.path.join(script_dir, "web_assets.h")
  )

  rendered_assets = []
  for name, path in assets:
    data = read_utf8_bytes(path)
    if name == "TELEMETRY_AND_HELP_JS":
      data = data.replace(b"__BRIDGE_HIGH_LINE_VERSION__", app_version_bytes)
    rendered_assets.append(format_asset_definition(name, data))
  definitions = "\n\n".join(rendered_assets)

  header_content = (
      "#pragma once\n\n"
      "#include <string_view>\n\n"
      "namespace web_assets {\n\n"
      f"{definitions}\n\n"
      "}  // namespace web_assets\n"
  )

  with open(out_path, "w", encoding="utf-8", newline="\n") as f:
    f.write(header_content)

  print(f"Successfully generated {out_path}")


if __name__ == "__main__":
  main()
