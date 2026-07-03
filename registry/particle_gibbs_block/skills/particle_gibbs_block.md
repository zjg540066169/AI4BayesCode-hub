---
name: particle_gibbs_block
description: Conditional-SMC / Particle-Gibbs (PGAS) sampler for the continuous latent state path of a univariate first-order state-space model with nonlinear / non-Gaussian transition or emission.
---

# particle_gibbs_block

Samples the **continuous latent state path** `x_{1:T}` of a univariate, first-order
state-space (continuous hidden-Markov) model from its full conditional
`p(x_{1:T} | y_{1:T}, theta)`, using **conditional SMC** (Particle Gibbs) with optional
**ancestor sampling (PGAS)**. Model structure enters through four callbacks; `theta` and the
observations `y_{1:T}` are read from the `block_context` each sweep, so the dynamics
parameters are sampled by **sibling blocks**. Gradient-free.

License: GPL-3.0-or-later.

## Routing row (for `block_catalogue.md`)

| parameter kind | block type | constraint wrap |
|---|---|---|
| **continuous latent state path `x_{1:T}` of a univariate first-order state-space model — nonlinear / non-Gaussian transition or emission (no closed-form FFBS/Kalman conditional)** | **`particle_gibbs_block`** (conditional SMC + PGAS; Andrieu-Doucet-Holenstein 2010, Lindsten-Jordan-Schon 2014) | **(none — the path is unconstrained ℝ^T; any parameter constraints live in the sibling θ-blocks)** |

## WHEN to use

Univariate **continuous** latent state-space path with a **nonlinear / non-Gaussian**
transition or emission (no closed-form smoother). *(This sentence is the manifest
`SelectWhen`; the two are kept identical by the identity rule.)* The canonical case is a
stochastic-volatility log-vol path (`examples/StochasticVolatility.cpp`): a Gaussian AR(1)
latent law with a non-Gaussian `N(0, exp(h_t))` emission, where no Kalman conditional exists.

## WHEN NOT to use

- **Finite / discrete latent state** (a categorical-state HMM) ⇒ use **`hmm_block`** (exact
  forward-filter backward-sample over the finite state space). This block is for **continuous**
  states only — its bootstrap-particle machinery does not apply to a discrete chain.
- **Fully linear-Gaussian SSM** where you want the latent path ⇒ the exact **Kalman / FFBS
  simulation smoother** draws are cheaper and exact. This block still targets the correct
  posterior there (verified in the test's T1a parity regime) but is overkill unless a
  nonlinearity, a non-Gaussian emission, or an intractable conditional removes the closed form.
- **Differentiable transition AND emission, short `T`, already running joint NUTS** ⇒ a single
  NUTS kernel over the whole path is viable. Prefer this block when the emission is
  non-differentiable / heavy-tailed, or the path is long and strongly persistent — where a
  joint gradient kernel conditions badly and **plain** PG degenerates (turn ancestor sampling
  ON; see the test's T4 PG-vs-PGAS contrast).

## Geometry class

Target = the **fixed-dimension, absolutely-continuous** latent path `x_{1:T} ∈ ℝ^T` ⇒
`system_design §11.1` case 1 (fixed-dim ℝ^d). It is **not** a `§11.2` case: the dimension is
fixed and the state is continuous, so this is neither the finite-state HMM (`§11.2(b)`) nor a
trans-dimensional target. See `geometry.md` for the legality gate — not restated here. The
block is gradient-free, so it faces **no** Check #5 (Jacobian) and **no** Check #12 (AD-twin):
the path is unconstrained, and there is no hand-written gradient.

## Config-struct snippet

```cpp
particle_gibbs_block_config cfg;
cfg.name              = "x";          // data key the sampled latent path x_{1:T} is published under
cfg.T                 = T;            // sequence length (number of time points)
cfg.N                 = 64;           // particles; cSMC is invariant for any N >= 2 (larger N mixes better, O(N*T)/sweep)
cfg.ancestor_sampling = true;         // PGAS ON (default): preserves invariance, breaks path degeneracy for long/persistent paths
cfg.resampling        = pg_resampling_t::systematic;   // stratified low-variance (default); or pg_resampling_t::multinomial
cfg.obs_key           = "y";          // block_context key holding the length-T observation vector y_{1:T}
cfg.initial_path      = {};           // OPTIONAL length-T warm start; empty => zeros (cSMC is invariant to the seed path)

// --- the four model callbacks (each reads theta from ctx; theta is sampled by SIBLING blocks) ---
cfg.init_sample       = [](const block_context& ctx, std::mt19937_64& rng) -> double { /* x_1 ~ m_theta(.) */ };
cfg.transition_sample = [](double x_prev, std::size_t t, const block_context& ctx,
                           std::mt19937_64& rng) -> double { /* x_t ~ f_theta(.|x_prev) */ };
cfg.obs_logweight     = [](double x_t, double y_t, std::size_t t,
                           const block_context& ctx) -> double { /* log g_theta(y_t|x_t) */ };
cfg.transition_logpdf = [](double x_prev, double x_t, std::size_t t,
                           const block_context& ctx) -> double { /* log f_theta(x_t|x_prev) */ };
                           // ^ REQUIRED iff ancestor_sampling == true (PGAS ancestor weights); the
                           //   constructor throws if it is missing while ancestor_sampling is on.

particle_gibbs_block blk(cfg);
// typical composite: pair with sibling blocks that sample theta (e.g. (mu, phi, sigma_eta) for SV)
// — a nuts_block / gibbs block — and write theta back into the block_context each sweep.
```

## Block-specific checks

Block-specific silent-failure checks (`BL1` reference-path preservation, `BL2` PGAS
ancestor-weight form) live in the separate validation skill — see the manifest
`ValidationSkill:` (`skills/particle_gibbs_block_validation.md`). They are needed only at
VALIDATE / audit, not on selection or use.

## Example

`examples/StochasticVolatility.cpp` — simulate an SV returns series, recover the latent
log-volatility path, and print a recovery summary (posterior mean vs truth, 90% CI coverage,
RMSE vs the prior-mean baseline).

## Shared conventions (cited, not restated)

- interface contract (the `block_sampler` methods): `system_design §1` (`interface.md`)
- constraint transforms / the 15 `joint_constraint` kinds: `constraints.md` (this block uses **none**)
- block-selection / Exception taxonomy (this block = Exception 4, AI-authored custom block): `codegen_priors.md §2b`
- metric + warmup policy: `system_design §13` (`families.md`) — N/A (gradient-free, no metric)
- validator checks this block faces: `validator.md` (#1–#26 — none triggered by mechanism; gradient-free, fixed-dim, no constraint transform); block-local `BL1`/`BL2` in the `ValidationSkill`
- geometry legality gate: `system_design §11` (`geometry.md`)
