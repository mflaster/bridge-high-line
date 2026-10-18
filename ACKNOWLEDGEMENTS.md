# Acknowledgements & Third-Party Credits

This project builds upon and draws inspiration from several open-source bridge
engines and C++/web libraries. We gratefully acknowledge their authors and
contributors:

## Bridge Engines & Bidding Systems

### Double Dummy Solver (`dds`)
- **Authors**: Bo Haglund and Soren Hein
- **Repository**: [https://github.com/dds-bridge/dds](https://github.com/dds-bridge/dds)
- **License**: Apache License 2.0
- **Role in Project**: Powers all double-dummy trick evaluations, legal move
  analysis, and play-tree search (`interface_dds.{h,cc}`, `solver.{h,cc}`).

### SAYCBridge & Yarborough (`saycbridge` / `yarborough`)
- **Authors**: Eric Seidel, Adam Barth, and The SAYCBridge Authors
- **Repositories**:
  - [`eseidel/saycbridge`](https://github.com/eseidel/saycbridge) (BSD-3-Clause)
  - [`saycbridge/yarborough`](https://github.com/saycbridge/yarborough) (Apache License 2.0)
- **License**: BSD-3-Clause / Apache License 2.0
- **Role in Project**: Fetched via CMake `FetchContent` (`CMakeLists.txt`). At
  build time, `third_party/gen_sayc_bids.py` imports `saycbridge`'s `z3b`
  Standard American Yellow Card (SAYC) rules and priority hierarchy and compiles
  their exact constraint expressions (including negative-inference `Pass` and
  priority exclusions) into `sayc_generated.h`, which is evaluated by `WouldBid`
  (`hand_requirements.{h,cc}`) in both native C++ and WebAssembly builds.

```text
Copyright (c) 2013, The SAYCBridge Authors. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

  * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in
    the documentation and/or other materials provided with the
    distribution.
  * Neither the name of the SAYCBridge project nor the names of its
    contributors may be used to endorse or promote products derived
    from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
```

---

## C++ & Web Libraries

- **[`cpp-httplib`](https://github.com/yhirose/cpp-httplib)** by Yuji Hirose
  (MIT License) — Embedded single-header C++ HTTP server and client
  (`third_party/httplib.h`).
- **[JSON for Modern C++ (`nlohmann/json`)](https://github.com/nlohmann/json)**
  by Niels Lohmann (MIT License) — Single-header C++ JSON serialization and
  parsing (`third_party/nlohmann/json.hpp`).
- **[Boost C++ Libraries](https://github.com/boostorg/headers)** (Boost Software
  License 1.0) — Header libraries fetched via CMake `FetchContent`.
- **[Google Logging (`glog`)](https://github.com/google/glog)** (BSD-3-Clause
  License) — Structured C++ logging.
- **[GoogleTest (`googletest`)](https://github.com/google/googletest)**
  (BSD-3-Clause License) — C++ unit and integration test framework.
- **[PDF.js](https://github.com/mozilla/pdf.js)** by Mozilla (Apache License
  2.0) — Client-side PDF rendering for bridge hand records in the web UI.
