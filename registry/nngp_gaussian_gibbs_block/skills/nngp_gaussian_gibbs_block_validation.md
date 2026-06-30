---
name: nngp_gaussian_gibbs_block_validation
description: Block-specific silent-failure checks (BL1-BL4) for nngp_gaussian_gibbs_block; loaded only at VALIDATE / re-validation / audit.
license: GPL-3.0-or-later
---

# nngp_gaussian_gibbs_block — block-specific validation (BL1–BL4)

These checks catch silent-correctness bugs UNIQUE to this block — failures that compile, run,
and give clean R-hat / ESS / LOO yet sample the wrong posterior. They are exercised in
`test_nngp_gaussian_gibbs_block.cpp` (runnable) or by code review (static). Generic checks
(Jacobian discipline, RNG separation, dense metric) are cited in `skills/nngp_gaussian_gibbs_block.md`,
not restated. Core #15/#16/#17 (Gibbs parity / justification / no-inline-sampler) also apply.

---

## BL1 — NNGP precision assembly matches the Vecchia definition  (RUNNABLE)

- **Trigger:** the block assembles the sparse precision P = (I−B)' F⁻¹ (I−B) from the
  per-location kriging weights b_i and conditional variances f_i, and reuses it for the w-draw
  (Q_w = σ⁻²P + τ⁻²I), the σ² update (σ²|· ~ IG(·, b_σ + ½ w'Pw)), and the collapsed φ slice.
- **Why (silent):** a wrong b_i/f_i solve, a wrong outer-product sign, or a mis-summed sparse
  entry yields a P that is still symmetric PD, so the sparse Cholesky succeeds, the chain
  converges (clean R-hat/ESS), and the posterior is WRONG. No runtime diagnostic catches it.
- **What to look for:** the per-conditional product log p(w) = Σ log N(w_i; b_i'w_{N(i)}, σ²f_i)
  (the Vecchia DEFINITION) must equal the quadratic-form value −½[n log(2πσ²) + Σ log f_i +
  σ⁻² w'Pw] for random w and several φ. `test_*`: the `density_consistency` regime checks
  `log_phi_conditional(φ)` against an INDEPENDENT dense P built from the neighbor sets
  (max rel|Δ| < 1e-6).
- **Fix:** correct the m×m neighbor solve (`refresh_BF_`), the f_i = 1 − c'b_i conditional
  variance, or the `assemble_Qw_bw_` triplet accumulation (`add_values=true` to SUM duplicate
  (row,col) locations) until the two log-densities agree.

## BL2 — exact ordered-prefix neighbor sets  (RUNNABLE)

- **Trigger:** the NNGP requires N(i) = the m nearest neighbors of location i among the
  EARLIER-ordered locations only; the block builds these with the vendored nanoflann dynamic
  KD-tree, adding points in NNGP order and querying before each add.
- **Why (silent):** if the search returns the m nearest among ALL points (not the earlier
  prefix), or mishandles the ordering/index mapping, the directed Vecchia graph is wrong → a
  different (still valid-looking) sparse GP approximation → wrong posterior with clean
  diagnostics. The nanoflann dynamic adaptor also auto-adds the whole cloud at construction if
  `kdtree_get_point_count()` is nonzero — leaving it at 0 then growing the live count per add is
  what restricts each query to the prefix.
- **What to look for:** `neighbors()` must equal an independent brute-force m-nearest-among-
  earlier computation under the SAME coordinate-sort ordering. `test_*`: the
  `vendored_correctness` (VC) regime asserts all neighbor sets match brute force (as sets).
- **Fix:** in `rebuild_structure_`, ensure the adaptor `count` starts at 0 (skip the
  constructor auto-add), is bumped to k+1 before `addPoints(k,k)`, the cloud is indexed in
  ORDERED positions, and returned positions are mapped back to original indices via `perm`.

## BL3 — partial-collapse (σ², φ) sweep order  (STATIC)

- **Trigger:** the (σ², φ) block update is a PARTIAL COLLAPSE: φ is slice-sampled on the
  σ²-MARGINALIZED conditional (`log_phi_collapsed_`), then σ² is drawn conjugately given the
  new φ. This is a valid block draw of (σ²,φ)|w ONLY if the collapsed φ step precedes the
  conditional σ² draw.
- **Why (silent):** reversing them (draw σ²|φ,w first, then slice φ on the collapsed target)
  breaks the Van Dyk–Park (2008) ordering requirement for partial collapse → an invalid
  sampler whose stationary law is not the target, yet it still converges to *something* with
  clean diagnostics.
- **What to look for (grep `step()`):** the call order must be
  `slice_update_phi_` (collapsed) → `update_sigma2_` → … . The φ slice MUST use
  `log_phi_collapsed_`, NOT the conditional `log_phi_cond_` (the latter is exposed only for the
  BL1 density test). `grep -nE "slice_update_phi_|update_sigma2_|log_phi_collapsed_|log_phi_cond_" <Block>.hpp`.
- **Fix:** keep `slice_update_phi_` (using `log_phi_collapsed_`) immediately before
  `update_sigma2_` in the sweep; never swap them or repoint the slice at `log_phi_cond_`.

## BL4 — vendored nanoflann stateful-compatibility  (RUNNABLE)

- **Trigger:** the block vendors an external kernel (nanoflann) and caches a data-derived
  KD-tree / neighbor structure built from the coordinates (`vendor.md` §3.3).
- **Why (silent):** a stale cache after `set_context` swaps the coordinates, cross-instance
  leakage from any hidden global/`static` state, or nondeterminism would silently use the wrong
  neighbor structure. (nanoflann has no internal RNG and no mutable global state — verified by a
  static grep — so determinism is automatic; the live risk is the cache.)
- **What to look for:** `set_context(coords A) → step → set_context(coords B≠A) → step` must use
  B's neighbor structure (dirty-flag rebuild), two interleaved instances must not interfere, and
  same-seed runs must be identical. `test_*`: the `stateful_compat` (SC) regime checks
  determinism + two-instance isolation + cache rebuild; plus a static grep of `vendor/nanoflann/
  nanoflann.hpp` for `rand(`/`srand`/`mt19937`/`thread_local`/non-const `static` mutable state.
- **Fix:** set `dirty_structure_` in `set_context` when the coordinates change; rebuild the
  KD-tree + neighbor sets lazily on the next `step`/`prepare`; copy coordinates into block-owned
  storage (never hold a pointer into `block_context`/`shared_data`).
