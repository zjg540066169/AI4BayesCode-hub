---
name: nngp_gaussian_gibbs_block
description: Large-scale Gaussian spatial regression with a sparse Nearest-Neighbor Gaussian Process (NNGP) random effect; samples (beta, tau^2, sigma^2, phi, w) by conjugate Gibbs + a joint sigma^2/phi update.
license: GPL-3.0-or-later
---

# nngp_gaussian_gibbs_block

Self-contained sampler for **Bayesian Gaussian spatial regression with an NNGP latent
random effect** (Datta, Banerjee, Finley & Gelfand 2016; Finley et al. 2019). It replaces
the dense n×n Gaussian-process covariance with a sparse nearest-neighbor (Vecchia)
approximation, so it scales to large point-referenced datasets where a full GP is infeasible.

Model (β regression, w spatial effect, τ² nugget, σ² partial sill, φ decay):

```
y_i | beta, w_i, tau^2 ~ Normal(x_i' beta + w_i, tau^2)
w_i | w_{N(i)}         ~ Normal(b_i' w_{N(i)}, f_i)       # NNGP, exponential cov sigma^2 exp(-phi d)
beta ~ Normal(0, sigma_beta^2 I),  tau^2,sigma^2 ~ Inverse-Gamma,  phi ~ Uniform
```

## (a) Routing row

```
| **point-referenced spatial random effect w (NNGP / sparse Vecchia GP), Gaussian response** | **`nngp_gaussian_gibbs_block`** (latent NNGP, Datta et al. 2016 / Finley et al. 2019; conjugate Gibbs + joint sigma^2/phi) | **(none — natural-scale conjugate draws + bounded slice; no constraint transform)** |
```

## (b) WHEN to use

Large-scale spatial regression with Gaussian-process random effects on point-referenced
data. (Identical to the manifest `SelectWhen`.)

## (c) WHEN NOT to use

- **Areal / lattice / graph data with a sparse precision Q (ICAR, BYM2, RW1/RW2)** → use
  `gmrf_precision_block` (it samples a GMRF given Q directly). This block is for
  **point-referenced** locations where the precision is built from coordinates + a covariance
  kernel, not supplied as Q.
- **1-D / time-series GP** → use `celerite_gp_block` (O(n) 1-D GP). NNGP is for 2-D/3-D
  point-referenced fields.
- **Small n where a full GP is affordable** → a full-GP / `joint_nuts_block` formulation is
  exact; NNGP is an approximation justified by SCALE. Use this block when n is large enough
  that the dense covariance is the bottleneck.
- **Non-Gaussian response** (Poisson/binomial counts with a spatial effect) → this block
  bakes in the Gaussian likelihood and its conjugacy; a non-Gaussian spatial GLMM needs a
  different block (e.g. an ESS-on-NNGP-prior + user log-likelihood), not this one.

## (d) Geometry class

§11.1 **class 1 — fixed-dimension absolutely continuous** (w is a continuous Gaussian field
with sparse precision; β real, τ²/σ²/φ positive). NOT a §11.2 STOP class (the field is
continuous Gaussian, not discrete strongly-coupled). See `system_design §11` (`geometry.md`).

## (e) Config snippet (mirrors the staged `.hpp`)

```cpp
nngp_gaussian_gibbs_block::config cfg;
cfg.name        = "nngp";   // sub-outputs: nngp_{beta,tau2,sigma2,phi,w}
cfg.n           = n;        // # locations (REQUIRED)
cfg.p           = p;        // # regression coefficients (REQUIRED)
cfg.coord_dim   = 2;        // spatial dimension d
cfg.m           = 10;       // # nearest neighbors (Datta 2016: m~10-15 ~ full GP)
cfg.sigma_beta2 = 1e4;      // beta ~ N(0, sigma_beta2 I)
cfg.a_tau = 2; cfg.b_tau = 1;       // tau^2 ~ IG(shape, scale)  (proper; NOT IG(eps,eps))
cfg.a_sigma = 2; cfg.b_sigma = 1;   // sigma^2 ~ IG(shape, scale)
cfg.phi_lower = 0; cfg.phi_upper = 0;  // <=0 => auto-derive domain-scaled Uniform bounds
cfg.use_level_shift = true;         // PX intercept<->spatial-level recentering (auto-off if no intercept col)
cfg.x_key = "X"; cfg.y_key = "y"; cfg.coords_key = "coords";  // refreshable data inputs (column-major flat)
// Data inputs come via set_context; in a composite declare:
//   comp.data().declare_dependencies(cfg.name, {"y","X","coords"});
// Self-contained: pairs with no hyperparameter blocks (it samples beta/tau2/sigma2/phi/w itself).
```

Vendors **nanoflann** (BSD-2-Clause, `vendor/nanoflann/`) for the KD-tree neighbor search.

## (f) Example

`examples/SpatialNNGPRegression.cpp` — frontend-independent C++ `int main()`: simulate from a
true GP → fit → recover (β, τ², σ², φ) → posterior-predictive check → held-out **NNGP kriging
that beats a non-spatial OLS baseline**.

## Block-specific checks

This block has block-specific silent-failure checks (BL1–BL4): see the separate
`ValidationSkill` `skills/nngp_gaussian_gibbs_block_validation.md` (loaded only at
validation / audit, per the manifest `ValidationSkill:`).

## Shared conventions (cited, not restated)

- interface contract (6 R methods + readapt_NUTS): `system_design §1` (`interface.md`)
- constraint transforms / the 15 joint_constraint kinds: `constraints.md`
- block-selection / Exception taxonomy: `codegen_priors.md §2b`
- metric + warmup policy (N/A — this is a Gibbs/slice block, no NUTS metric): `system_design §13` (`families.md`)
- validator checks faced: `validator.md` (#15/#16/#17 Gibbs discipline, defined in `codegen_priors.md §2c–§2e`); #5/#12/#18/#20/#25 are silent (no hand-written gradient, no NUTS metric, no funnel)
- geometry legality gate: `system_design §11` (`geometry.md`)
- vendoring (stateful adaptation of nanoflann): `block_design_skills/vendor.md`
