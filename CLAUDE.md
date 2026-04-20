# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What This Is

Iscut (Is Semantic Cutter / 是语切词) is a Chinese word segmentation tool. It uses a Double-Array Trie for dictionary lookup, DAG + dynamic programming for optimal segmentation, and an EM algorithm for learning word frequencies from raw corpus data. Pure C++17, no external runtime dependencies.

## Build & Run

```bash
# Build C++ binary
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Run smoke tests (not wired into CMake; compile manually)
g++ -std=c++17 -Isrc src/test.cc src/cut.cc src/ustr.cc -o build/iscut-test && ./build/iscut-test

# Install Python package (requires pybind11; scikit-build-core drives the build when SKBUILD is set)
uv pip install .

# Interactive REPL
./build/iscut --dict dict.txt

# Pipe mode (stdin → stdout)
./build/iscut --dict dict.txt --pipe < input.txt > output.txt
```

CLI modes in `main.cc`: `--pipe`, `--segment` (longest-match via `Segmenter`), `--cut` (DAG+DP via `Cutter`), `--count` (frequency accumulation with optional dict whitelist), `--prune` (per-word deletion loss for EM pruning). With just `--dict` and no mode, it drops into a REPL.

## Training Pipeline

Full pipeline to retrain the dictionary from scratch:

1. **Build dictionary** (`cd dict && make`) — downloads Wikimedia title dumps, converts traditional→simplified, filters by length rules
2. **Fetch corpus** (`cd data && make`) — downloads Chinese Wikipedia/Wikinews from HuggingFace, converts to simplified, splits into sentences
3. **Train** (`cd scripts && make`) — character frequency counting, dict filtering, EM training with pruning

Training requires `datasets`, `huggingface_hub[cli]`, `opencc` Python packages. Control vocab size and EM sub-iterations with `make VOCAB_SIZE=100000 SUB_ITERS=3`.

## Architecture

### Core C++ (`src/`)

- **`trie.h`** — `trie::DoubleArray<T>`: XOR-indexed double-array trie. Requires sorted input. Supports exact lookup (`GetUnit`) and common prefix search (`PrefixSearch`). Header-only.
- **`cut.h/cc`** — `cut::Cutter`: the main segmenter. `Cut()` runs the internal `PreTokenize` pre-splitter, then on Han runs builds a DAG via trie prefix search and runs backward DP to find the max log-probability path. Non-Han runs (Latin/digits/punct/space chunks) are passed through unchanged. `CutWithLoss` computes per-word deletion loss (rerun DP with the edge removed) for pruning.
- **`segment.h/cc`** — `cut::Segmenter`: forward longest-match segmenter (no frequencies). Used for EM cold start.
- **`ustr.h/cc`** — UTF-8 utilities: `CharLen`, `DecodeUTF8`, `IsHan`, `IsPunct`, plus `SplitByPunct` / `SplitByHan` helpers (retained in the header; `Cutter` itself uses its own file-local `PreTokenize` now, not these).
- **`count.h/cc`** — `cut::Counter`: word frequency accumulator.
- **`main.cc`** — CLI entry point (modes listed above).
- **`pip.cc`** — pybind11 wrapper exposing a single `iscut.Cutter(dict_path).cut(sentence)` to Python. Built only when `SKBUILD` is set (see `CMakeLists.txt`).
- **`test.cc`** — Smoke tests. Not wired into CMake; compile manually (see Build & Run).

### Pre-tokenization (inside `cut.cc`)

`Cutter::Cut` does not call `ustr::SplitByPunct` / `SplitByHan`. Instead it uses two static helpers local to `cut.cc`:

- **`Normalize`** — collapses runs of ASCII spaces to the sentencepiece-style `▁` (U+2581) marker; strips leading/trailing space.
- **`PreTokenize`** — after `Normalize`, classifies each codepoint as `SPACE | LETTER | DIGIT | HAN | PUNCT` (via the file-local `IsHanCP`, `IsDigitCP`, `IsWordChar`) and groups contiguous same-kind codepoints into chunks. `SPACE` and `PUNCT` chunks are emitted one codepoint at a time; `LETTER` runs additionally keep inner apostrophes (`don't`, `they'll`).

Each chunk is then classified by `IsHanChunk`: Han chunks go through `CutSegment` (DAG+DP) if a dict is loaded, or fall back to per-char split otherwise; non-Han chunks pass through verbatim. This is why the test `test_mixed_cut` expects `他是英国人Tom` → `他/是/英国人/Tom` and `test_punct` expects `你好，世界！` → `你好/，/世界/！`.

### DAG construction

`Cutter::DAG` calls `da_.PrefixSearch` at each byte offset to collect dictionary matches, and **always** adds a single-UTF-8-char fallback edge so unknown characters still yield a valid path. Unknown-word log-prob is `log(1/sum_)` where `sum_` is the total frequency mass (see `GetTrieValue`).

### Key design patterns

- **Dictionary format**: TSV with `word\tfrequency`. Single-character entries are included — they serve as the fallback path when no multi-char match wins.
- **EM loop** (`scripts/run_em.sh` via `scripts/Makefile`): cold start (longest match) → iterative cut/count → prune by per-word deletion loss → final EM convergence.

## Coding Style

- C++17, standard library only — no external dependencies for the core.
- 4-space indentation in C++ and Python.
- `PascalCase` for classes and public methods (`Cutter`, `Cut`, `PrefixSearch`), `snake_case` for Python functions, shell scripts, and Make targets (`filter_dict.py`, `count_chars`).
- Prefer small translation units and file-local helpers (`static` functions at the top of `cut.cc`) over new public abstractions.

## Commit Style

Short, imperative subjects. Make them specific enough to identify the touched area, e.g. `scripts: tighten dict filter`. Include sample CLI output when segmentation behavior changes.

## Dict files at root

- `dict.txt` — current default dictionary
- `dict.12w.txt`, `dict.24w.txt` — alternative sizes (12万/24万 words)
