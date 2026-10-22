#!/usr/bin/env python3
import os
import sys


def read_utf8(path):
  with open(path, "r", encoding="utf-8") as f:
    return f.read()


def main():
  script_dir = os.path.dirname(os.path.abspath(__file__))
  static_dir = os.path.join(script_dir, "static")

  html_content = read_utf8(os.path.join(static_dir, "index.html"))
  help_html_content = read_utf8(os.path.join(static_dir, "help.html"))
  css_content = read_utf8(os.path.join(static_dir, "styles.css"))
  app_js_content = read_utf8(os.path.join(static_dir, "app.js"))
  solver_api_js_content = read_utf8(os.path.join(static_dir, "solver_api.js"))
  deal_svg_js_content = read_utf8(os.path.join(static_dir, "deal_svg.js"))
  analysis_tabs_js_content = read_utf8(
      os.path.join(static_dir, "analysis_tabs.js")
  )
  photo_scanner_js_content = read_utf8(
      os.path.join(static_dir, "photo_scanner.js")
  )
  decision_tree_js_content = read_utf8(
      os.path.join(static_dir, "decision_tree.js")
  )
  telemetry_and_help_js_content = read_utf8(
      os.path.join(static_dir, "telemetry_and_help.js")
  )
  pdf_export_js_content = read_utf8(
      os.path.join(static_dir, "pdf_export.js")
  )
  presets_content = read_utf8(os.path.join(script_dir, "presets.json"))
  strategy_doc_content = read_utf8(
      os.path.join(script_dir, "..", "docs", "bridge-strategy.md")
  )
  user_guide_content = read_utf8(
      os.path.join(script_dir, "..", "docs", "user-guide.md")
  )
  out_path = (
      sys.argv[1]
      if len(sys.argv) > 1
      else os.path.join(script_dir, "web_assets.h")
  )

  header_content = f"""#pragma once

#include <string>

namespace web_assets {{

inline const char* INDEX_HTML = R"rawhtml(
{html_content}
)rawhtml";

inline const char* HELP_HTML = R"rawhtml(
{help_html_content}
)rawhtml";

inline const char* STYLES_CSS = R"rawcss(
{css_content}
)rawcss";

inline const char* APP_JS = R"rawjs(
{app_js_content}
)rawjs";

inline const char* SOLVER_API_JS = R"rawjs(
{solver_api_js_content}
)rawjs";

inline const char* DEAL_SVG_JS = R"rawjs(
{deal_svg_js_content}
)rawjs";

inline const char* ANALYSIS_TABS_JS = R"rawjs(
{analysis_tabs_js_content}
)rawjs";

inline const char* PHOTO_SCANNER_JS = R"rawjs(
{photo_scanner_js_content}
)rawjs";

inline const char* DECISION_TREE_JS = R"rawjs(
{decision_tree_js_content}
)rawjs";

inline const char* TELEMETRY_AND_HELP_JS = R"rawjs(
{telemetry_and_help_js_content}
)rawjs";

inline const char* PDF_EXPORT_JS = R"rawjs(
{pdf_export_js_content}
)rawjs";

inline const char* PRESETS_JSON = R"rawjson(
{presets_content}
)rawjson";

inline const char* BRIDGE_STRATEGY_MD = R"rawmd(
{strategy_doc_content}
)rawmd";

inline const char* USER_GUIDE_MD = R"rawmd(
{user_guide_content}
)rawmd";

}} // namespace web_assets
"""

  with open(out_path, "w", encoding="utf-8") as f:
    f.write(header_content)

  print(f"Successfully generated {out_path}")


if __name__ == "__main__":
  main()
