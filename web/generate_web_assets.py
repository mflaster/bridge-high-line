#!/usr/bin/env python3
import os
import sys

def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    static_dir = os.path.join(script_dir, "static")
    
    html_path = os.path.join(static_dir, "index.html")
    css_path = os.path.join(static_dir, "styles.css")
    js_path = os.path.join(static_dir, "app.js")
    presets_path = os.path.join(script_dir, "presets.json")
    out_path = os.path.join(script_dir, "web_assets.h")

    with open(html_path, "r", encoding="utf-8") as f:
        html_content = f.read()

    with open(css_path, "r", encoding="utf-8") as f:
        css_content = f.read()

    with open(js_path, "r", encoding="utf-8") as f:
        js_content = f.read()

    with open(presets_path, "r", encoding="utf-8") as f:
        presets_content = f.read()

    header_content = f"""#pragma once

#include <string>

namespace web_assets {{

inline const char* INDEX_HTML = R"rawhtml(
{html_content}
)rawhtml";

inline const char* STYLES_CSS = R"rawcss(
{css_content}
)rawcss";

inline const char* APP_JS = R"rawjs(
{js_content}
)rawjs";

inline const char* PRESETS_JSON = R"rawjson(
{presets_content}
)rawjson";

}} // namespace web_assets
"""

    with open(out_path, "w", encoding="utf-8") as f:
        f.write(header_content)

    print(f"Successfully generated {out_path}")

if __name__ == "__main__":
    main()
