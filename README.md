# AI4BayesCode Hub

Website and community hub for **AI4BayesCode** — a header-only C++ library of
composable, stateful block-wise MCMC samplers.

- **Library (core):** https://github.com/zjg540066169/AI4BayesCode
- **Live site:** https://zjg540066169.github.io/AI4BayesCode-hub/

## What lives here

- `index.html`, `blocks.html`, `api.html`, `teams.html` — the static website
  (served by GitHub Pages from the repository root).
- `assets/` — shared stylesheet, logo, fonts.
- *(future)* `registry/` — community-contributed ("downloaded") blocks, kept
  here so they stay isolated from the vetted core library (trust / license /
  distribution boundary; see the core repo's `contrib.md`).

The website's block catalogue separates **core** blocks (shipped with the
library) from **community** blocks (contributed here). User-submitted blocks
are reviewed before they are merged and published.

## Local preview

No build step — it is plain static HTML/CSS. Open `index.html` in a browser,
or serve the folder:

```bash
python3 -m http.server 8000   # then open http://localhost:8000
```

## License

GPL-3.0-or-later, matching the AI4BayesCode library.
