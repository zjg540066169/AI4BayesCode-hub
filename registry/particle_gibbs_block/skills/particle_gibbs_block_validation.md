---
name: particle_gibbs_block_validation
description: Block-specific (BL#) silent-failure checks for particle_gibbs_block — conditional-SMC reference-path preservation and PGAS ancestor-weight form. Loaded only at VALIDATE / audit.
---

# particle_gibbs_block — block-specific validation checks (`BL#`)

License: GPL-3.0-or-later.

These are the silent-failure modes UNIQUE to this block's conditional-SMC / PGAS mechanism:
they **compile, run, and pass casual R̂ / ESS**, yet sample the WRONG posterior. They are kept
out of the main `skills/particle_gibbs_block.md` (loaded on every selection / use) and pointed
to by the manifest `ValidationSkill:` field — load this file only during VALIDATE,
re-validation, or audit. Core checks `#1`–`#26` (`validator.md`) are all **silent by mechanism**
for this gradient-free, fixed-dim, constraint-free block; the two checks below are the whole
block-specific surface.

`ChecksApplicable: BL1, BL2`.

---

## BL1 — reference-path preservation (RUNNABLE)

**Trigger.** Every conditional-SMC sweep (the core of `step()`), `ancestor_sampling` either off
or on.

**Why (the silent bug).** Conditional SMC is invariant for any `N ≥ 2` ONLY because the current
path `x*` is retained as a distinguished reference particle (slot 0) at EVERY time `t` and can be
recovered by the final lineage trace (Andrieu-Doucet-Holenstein 2010, Thm 5). If the reference is
lost — slot 0 overwritten by a resampled offspring, the reference accidentally included in the
`N-1` resampled ancestors, or the lineage trace dropping it — the kernel **silently** stops
targeting `p(x_{1:T}|y,θ)`. It still emits plausible-looking paths, and on a short / weakly
dependent series the marginals can look fine, so **R̂ and ESS do not catch it**. This is THE
canonical cSMC implementation bug.

**What to look for.**
```cpp
// RIGHT — slot 0 is pinned to the reference at EVERY t; only N-1 NON-reference
// offspring are resampled; slot-0 ancestry is retained (plain PG) or PGAS-sampled.
X_(0, 0) = x_[0];
for (std::size_t i = 1; i < N; ++i) X_(i, 0) = cfg_.init_sample(context_, rng);
// ... per t:
systematic_resample(w, N - 1, rng, anc_off);   // N-1, NOT N
X_(0, t) = x_[t];                              // reference re-pinned every step
anc_(0, t) = ancestor_sampling ? sampled_index : 0;

// WRONG — resampling all N (reference can be discarded) and/or not re-pinning slot 0:
systematic_resample(w, N, rng, anc_off);       // BUG: reference may be resampled away
// (slot 0 left as a resampled particle => invariance lost, silently)
```
Runnable guard: the **`[BL1]` regime** in `test_particle_gibbs_block.cpp` builds a sweep in which
ONLY the reference value carries weight (plain PG, constant reference `REF`, callbacks that emit a
different value so every non-reference particle gets `~ -1e12` log-weight). The returned path must
then equal `x*` **exactly** for any seed; observed drift `= 0.000000`. A broken slot-0 pin makes
the output drift off `REF` and the assertion fails.

**Fix.** Pin `X_(0,t) = x_[t]` at every `t`; resample exactly `N-1` non-reference offspring; keep
slot-0 ancestry (`anc_(0,t)=0` for plain PG, the PGAS draw otherwise); trace the lineage of the
final index back through `anc_`.

---

## BL2 — PGAS ancestor-weight form (RUNNABLE via T1a + STATIC review)

**Trigger.** `ancestor_sampling == true` (the PGAS path only).

**Why (the silent bug).** The ancestor of the reference at time `t` must be drawn with weight
`∝ W_{t-1}^j · f_θ(x*_t | x_{t-1}^j)` — the **previous normalized weight** times the **forward
transition density to the reference state** (Lindsten-Jordan-Schon 2014). Three easy edits each
**silently** bias the stationary law while still mixing: (a) dropping the `W_{t-1}^j` factor
(using only the transition density); (b) reversing the transition arguments,
`f_θ(x_{t-1}^j | x*_t)`; (c) using the emission `g_θ` instead of the transition `f_θ`. None of
these throw; on a short, weakly-dependent series the chain can still look converged.

**What to look for.**
```cpp
// RIGHT — previous weight (in log) + forward transition density to the reference state x*_t:
am[j] = std::log(w[j]) + cfg_.transition_logpdf(X_(j, t-1), x_[t], t, context_);
//                       ^ from particle j at t-1  ^ to the reference at t   (forward direction)

// WRONG — missing the log W_{t-1}^j term:
am[j] = cfg_.transition_logpdf(X_(j, t-1), x_[t], t, context_);
// WRONG — reversed transition arguments (density of the reference -> particle):
am[j] = std::log(w[j]) + cfg_.transition_logpdf(x_[t], X_(j, t-1), t, context_);
```
Runtime backstop: the **`[T1a]` regime** runs the block with `ancestor_sampling = true` on a
linear-Gaussian SSM and checks the sampled marginals against the **exact Kalman/RTS smoother**.
Any of the wrong forms above shifts the stationary marginals off the exact smoother and blows the
family-wise z-score past the bar — so T1a is the runnable exercise of this check (`grep` the
`am[j] = ` line for the static review).

**Fix.** Use `log(W_{t-1}^j) + transition_logpdf(x_{t-1}^j, x*_t, …)` — previous weight in log
plus the FORWARD transition density evaluated AT the reference state.

---

## Shared conventions (cited, not restated)

- core validator check definitions `#1`–`#26`: `validator.md` (all silent for this block by mechanism)
- the by-mechanism applicability table + family-silence rule: `validate.md §2`
- geometry legality gate: `system_design §11` (`geometry.md`)
