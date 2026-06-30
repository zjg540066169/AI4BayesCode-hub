// Auto-generated core block catalogue. Regenerate from the library, do not hand-edit.
window.CORE_BLOCKS = [
  {
    "name": "bart_block",
    "kind": "sampling",
    "title": "BART Tree-Ensemble Mean Block",
    "summary": "Samples a nonparametric Gaussian regression mean f(x) with a Bayesian Additive Regression Trees (BART) forest, one tree-ensemble sweep per step.",
    "description": "A block_sampler wrapping the vendored stdbart::bart_model kernel for the model y = f(x) + epsilon, where f is a sum-of-trees BART forest. Each step() performs exactly one tree-ensemble sweep, while the residual noise sigma is supplied by a sibling block via the configured sigma_key. It records forest, sigma, and variable-usage history for posterior prediction, caches the forest fit so current() stays O(N), and supports tree serialization for round-tripping state inside an outer Gibbs sampler. Optional flags enable DART sparse variable selection (Linero 2018) and a probit leaf-prior tau formula for Albert-Chib data augmentation.",
    "when_to_use": "Pick this block to learn an unknown Gaussian regression mean function f(x) nonparametrically with a BART forest when a separate block handles the noise sigma.",
    "example": "BartNoise.cpp"
  },
  {
    "name": "beta_gibbs_block",
    "kind": "sampling",
    "title": "Exact Beta Gibbs Draw for (0,1) Parameters",
    "summary": "A closed-form Gibbs leaf that draws a scalar (0,1) parameter exactly from its conjugate Beta full conditional.",
    "description": "This block draws a scalar parameter directly from a Beta distribution whose two shape parameters are computed from context by a user-supplied params_fn returning (alpha, beta). It samples exactly via the Gamma trick: draw x from Gamma(alpha, 1) and y from Gamma(beta, 1), then return x / (x + y), costing O(1) per step. It is built for conjugate cases such as the spike-and-slab mixing proportion, where gamma_j is Bernoulli(pi) and pi has a Beta(a, b) prior, giving pi given gamma equal to Beta(a + sum(gamma), b + p - sum(gamma)). It is a last-resort specialized block: the default for continuous parameters is a NUTS block, and misuse from a wrong params_fn derivation can silently produce a wrong posterior that still passes diagnostics.",
    "when_to_use": "Pick this block when a scalar (0,1) parameter has an exactly Beta full conditional, such as a spike-and-slab mixing proportion or a Beta-Binomial probability, where running NUTS on a tight 1-D scalar would be wasteful.",
    "example": "SpikeSlabRJMCMC.cpp"
  },
  {
    "name": "binary_gibbs_block",
    "kind": "sampling",
    "title": "Closed-form Gibbs for binary indicator vectors",
    "summary": "A closed-form Gibbs leaf that draws an entire vector of 0/1 binary parameters in one step from their conditional Bernoulli distribution.",
    "description": "This block samples vector-valued binary (0/1) parameters, typically the inclusion indicators of a spike-and-slab model or any other latent Bernoulli, where Stan and other gradient samplers cannot handle discrete parameters directly. The conditional distribution of the binary vector given everything else is a product of Bernoullis, so a single step draws the whole vector in closed form with no NUTS or gradient machinery. The per-element log-odds are supplied by a user-provided functor log_odds_fn(ctx) that reads the current state from the context, which is the only model-specific math the user writes. Sampling is done directly from Bernoulli with p = 1 / (1 + exp(-log_odds)) so it stays numerically stable and saturates cleanly to 0 or 1 for large magnitudes.",
    "when_to_use": "Pick this block for continuous spike-and-slab variable selection or plain binary indicators, but not for Dirac spike-and-slab where beta_j is exactly 0 when the indicator is 0 (use rjmcmc_block there instead).",
    "example": ""
  },
  {
    "name": "categorical_gibbs_block",
    "kind": "sampling",
    "title": "Closed-Form Gibbs for Categorical Latents",
    "summary": "Draws per-observation categorical latent labels z in {1..K} from their closed-form full conditional via a stable softmax.",
    "description": "This block is the K-ary generalization of binary_gibbs_block: it samples per-observation discrete latents z_i in {1, 2, ..., K} in closed form, with one label per observation. The user supplies a functor log_probs_fn(ctx) that returns an n_obs by n_categories matrix whose (i, k) entry is the log conditional probability of z_i = k up to an additive constant per row, and the block normalizes each row with a stable softmax and draws an independent categorical per observation. It exists because Stan cannot sample discrete parameters directly, so latent categorical variables in mixtures, latent-class, LDA, and regime-switching models must be handled by a separate Gibbs step. Validity requires the z_i to be conditionally independent across observations given the continuous parameters, so it is appropriate for mixture and latent-class models but not for Markov-structured latents such as HMM state sequences (use hmm_block there).",
    "when_to_use": "Pick this block to sample conditionally independent per-observation categorical labels z in {1..K}, such as mixture component indicators or latent-class assignments, alongside the continuous blocks of the model.",
    "example": "DPGaussianMixture.cpp"
  },
  {
    "name": "celerite_gp_block",
    "kind": "sampling",
    "title": "Fast O(N) 1-D Time-Series Gaussian Process",
    "summary": "A 1-D time-series Gaussian process block that computes O(N) Cholesky and marginal log-likelihood using the semi-separable celerite algorithm.",
    "description": "This block wraps the celerite CholeskySolver of Foreman-Mackey et al. 2017 to evaluate a 1-D Gaussian process whose kernel is a sum of real-exponential and quasi-periodic oscillatory terms, achieving O(N) Cholesky and log-determinant instead of the usual O(N^3). It integrates out the latent function f using Gaussian-likelihood conjugacy, so no explicit f is sampled. Instead it exposes the marginal log-likelihood p(y given kernel params) for sibling hyperparameter blocks to drive via MCMC, and supplies closed-form posterior mean and variance at prediction time. It handles 1-D input only, with multi-dimensional GPs deferred to the elliptical slice sampling block plus libgp kernels.",
    "when_to_use": "Pick this block for long 1-D time-series (for example N greater than 2000, such as astronomical, financial, or climate data) where a dense O(N^3) GP is too slow and the kernel is a sum of real-exponential plus quasi-periodic terms.",
    "example": "GPTimeSeries.cpp"
  },
  {
    "name": "composite_block",
    "kind": "sampling",
    "title": "Composite block: ordered Gibbs over child blocks",
    "summary": "Runs an ordered Gibbs sweep over a list of child blocks, sharing their current values through one pool.",
    "description": "The composite_block holds a fixed list of child blocks and a shared_data pool of current values. Each step() updates the children in a fixed Gibbs order, writing each child draw back into the pool so later children condition on it. It is itself a block_sampler, so a composite behaves exactly like a single block and composites can nest. This is the wrapper that assembles a full sampler out of leaf blocks.",
    "when_to_use": "It is assembled automatically whenever a model needs more than one block; you rarely construct it by hand.",
    "example": ""
  },
  {
    "name": "dirichlet_gibbs_block",
    "kind": "sampling",
    "title": "Exact Dirichlet Gibbs Leaf",
    "summary": "Draws a K-vector theta exactly from its Dirichlet full conditional via gamma normalization at each step.",
    "description": "A closed-form Gibbs leaf for the common case where theta's full conditional is exactly Dirichlet(alpha_post), as in Dirichlet-Categorical, Dirichlet-Multinomial, LDA topic models, and Bayesian naive Bayes. It samples directly using the standard trick: draw each g_k from Gamma(alpha_post[k], 1), then set theta[k] = g[k] / sum(g), giving an exact iid draw in O(K) per step with no warmup, adaptation, or gradients. The user supplies only an alpha_post_fn(ctx) returning the length-K posterior concentration vector, typically a prior alpha plus category counts read from the context. It composes inside a composite_block with nuts_block and other leaves exactly like any other block.",
    "when_to_use": "Pick this block when theta's full conditional is exactly Dirichlet; if extra non-Dirichlet factors couple in (a logistic link, a smooth prior on log-ratios, a simplex penalty), use simplex_nuts_block or nuts_block plus constraints::simplex::wrap instead.",
    "example": "FiniteGaussianMixture.cpp"
  },
  {
    "name": "elliptical_slice_sampling_block",
    "kind": "sampling",
    "title": "Elliptical Slice Sampling for Latent-Gaussian Models",
    "summary": "Performs one Murray-Adams-MacKay elliptical slice sampling update of a latent field f drawn from a zero-mean Gaussian prior under an arbitrary likelihood.",
    "description": "This block implements Elliptical Slice Sampling (Murray, Adams, MacKay 2010) for any model with a latent-Gaussian prior f ~ N(0, Sigma) and a user-supplied likelihood log p(y | f). It needs no gradients and no step-size tuning, drawing an auxiliary nu = L z from the prior Cholesky, defining a slice height, and shrinking an angular bracket until a proposal is accepted (acceptance is guaranteed). It handles arbitrary cross-correlation in Sigma, which step-size-based samplers like NUTS struggle with on Sigma-correlated posteriors. The block is library-agnostic: the lower-triangular Cholesky L of Sigma is supplied through shared_data as a column-major flat vector, and the wrapper is responsible for refreshing L when covariance hyperparameters change.",
    "when_to_use": "Pick this block to update a high-dimensional, strongly correlated latent Gaussian field (GP, CAR/ICAR/GMRF, or random-walk smoothing) under a non-conjugate likelihood when you want gradient-free, tuning-free sampling.",
    "example": "GPRegression.cpp"
  },
  {
    "name": "full_rank_gaussian_vi_block",
    "kind": "sampling",
    "title": "Full-Rank Gaussian Variational Inference Block",
    "summary": "Fits a full-covariance Gaussian variational approximation q(eta) = N(eta; mu, L L') on the unconstrained scale, capturing the full posterior correlation among coordinates.",
    "description": "This block maintains a full-rank Gaussian variational posterior q(eta) = N(eta; mu, L L') on the unconstrained scale, where L is a lower-triangular Cholesky factor with positive diagonal. By keeping every off-diagonal covariance term it captures the full correlation among the K coordinates, the standard remedy for mean-field VI underestimating marginal variance when the posterior is correlated. Variational parameters are mu, log of the diagonal of L, and the off-diagonal entries of L, for a total of K + K(K+1)/2 parameters, so cost grows quadratically in K. It uses the reparameterization gradient eta = mu + L eps with eps ~ N(0, I) and the same natural-scale log-density-plus-gradient lambda as nuts_block, with suggested caps of K up to about 50 auto-suggested and K above 500 rejected.",
    "when_to_use": "Pick this block when several continuous parameters are strongly correlated (for example regression with collinear predictors, a hierarchical scale-by-parameter funnel, or BNN output-layer weights) and a single joint variational block of merged dimension up to about 50 is needed so the cross-coordinate covariance is represented rather than collapsed by a mean-field factorization.",
    "example": ""
  },
  {
    "name": "gamma_gibbs_block",
    "kind": "sampling",
    "title": "Closed-form Gamma Gibbs leaf block",
    "summary": "Draws an exact Gibbs sample for a scalar positive parameter whose full conditional is a Gamma distribution.",
    "description": "This is a closed-form Gibbs leaf block for a single positive scalar parameter whose full conditional posterior is exactly Gamma in the shape-rate parameterization. It is the companion or dual of inv_gamma_gibbs_block, and it draws iid samples with no warmup and no autocorrelation. Typical uses include the Dirichlet process concentration alpha under truncated stick-breaking, where alpha given the sticks is Gamma(a + T - 1, b - sum of log(1 - V_k)), and a scalar precision lambda when a Normal-Gamma joint is integrated to its marginal Gamma. It is a statistically equivalent, exact replacement for running a one-dimensional NUTS step on log(alpha).",
    "when_to_use": "Pick this block when a positive scalar parameter has an exactly conjugate Gamma full conditional and you want an exact iid draw instead of NUTS on the log scale.",
    "example": "HDPGaussianMixture.cpp"
  },
  {
    "name": "genbart_block",
    "kind": "sampling",
    "title": "Generalized BART tree ensemble, any likelihood",
    "summary": "A generic Bayesian additive-regression-tree block that fits a tree ensemble for any plug-in likelihood via reversible-jump MCMC.",
    "description": "genbart_block runs one generalized-BART RJMCMC tree-ensemble sweep per step, implementing Linero 2022 (arXiv:2202.09924) with Laplace-approximated BIRTH, DEATH, and CHANGE tree proposals. It accepts any likelihood satisfying the three-method contract of log_f, score, and obs_info, and ships ten ready likelihoods (normal, logistic, Poisson, negative binomial, heteroscedastic, AFT, gamma_shape, beta, beta_binomial, plus custom). The tree-ensemble output r(x) is on the linear-predictor scale, so its interpretation depends on the attached likelihood. The block is Rcpp-free and backend-neutral, using Armadillo containers so it compiles under both the R and standalone or pybind11 backends.",
    "when_to_use": "Pick this block when you need a nonparametric BART tree ensemble for a response whose likelihood is non-Gaussian or custom, such as logistic, Poisson, or count and survival models.",
    "example": "GBartLogistic.cpp"
  },
  {
    "name": "gmrf_precision_block",
    "kind": "sampling",
    "title": "Sparse-Precision Gaussian Markov Random Field Sampler",
    "summary": "It draws exact samples from a Gaussian Markov random field whose density is proportional to exp(-1/2 x' Q x + b' x), where Q is a sparse precision matrix.",
    "description": "This block is a direct (Gibbs-style) sampler for Gaussian Markov random fields specified in canonical form by a sparse, symmetric, positive-(semi)definite precision matrix Q, following Rue 2001. Each sweep builds Q via a user functor, factorizes it with a sparse Cholesky decomposition (Eigen SimplicialLLT with AMD reordering), and produces an exact draw from x ~ N(Q^{-1} b, Q^{-1}); the symbolic factorization is cached once while the numerical values are refactorized every step. An optional canonical vector b shifts the mean to mu = Q^{-1} b, and a sum-to-zero constraint can be enforced by post-sampling projection with a small ridge regularization on Q. It is a specialized, efficient alternative to running NUTS on high-dimensional Gaussian latents.",
    "when_to_use": "Pick this block for high-dimensional Gaussian latents with a sparse precision matrix, such as spatial smoothing, RW1/RW2 splines, ICAR/BYM2 disease mapping, or lattice GP approximations, where a direct conjugate draw beats NUTS.",
    "example": "GMRFPrior.cpp"
  },
  {
    "name": "gmrf_whitened_ess_block",
    "kind": "sampling",
    "title": "Elliptical slice sampling for non-Gaussian GMRF latents",
    "summary": "Samples a sparse-precision GMRF latent field under an arbitrary non-Gaussian observation likelihood using Murray 2010 elliptical slice sampling on the implicit prior.",
    "description": "This block targets a Gaussian Markov random field latent vector x whose full conditional is proportional to exp(-0.5 x' Q x + b' x) times an arbitrary observation likelihood L(y | x), where Q is a sparse PSD precision matrix that may be rank-deficient with an optional sum-to-zero constraint such as ICAR. It is the non-Gaussian companion to gmrf_precision_block, which only handles the conjugate Gaussian-conditional case. Each step refactorizes the sparse Cholesky of Q, draws a prior sample nu from N(0, Q inverse) via a Rue 2001 permuted backsolve, then runs an elliptical-slice shrink loop whose acceptance rate is independent of the likelihood scale. It supports Poisson, Bernoulli, Student-t, negative binomial, and log-Gaussian Cox likelihoods, with the sum-to-zero invariant preserved to machine precision.",
    "when_to_use": "Pick this block when the latent prior is a sparse-precision GMRF but the observation likelihood is non-Gaussian, so the full conditional is no longer Gaussian and a direct conjugate draw does not apply, such as a Poisson-ICAR or BYM2 spatial model.",
    "example": ""
  },
  {
    "name": "hmm_block",
    "kind": "sampling",
    "title": "Hidden Markov state sequence via FFBS",
    "summary": "Exact forward-filter backward-sample block that draws the latent state sequence of a finite-state Hidden Markov Model from its full conditional.",
    "description": "This block jointly samples the latent state sequence z_1:T in {0..K-1} of a finite-state HMM from p(z_1:T given y, A, pi, theta) using the standard log-space forward-filter backward-sample (FFBS) algorithm, with K and T fixed. Each sweep it reads the transition matrix A, initial distribution pi, and a user-supplied per-(t, k) emission log-density from context; those quantities are sampled by sibling blocks such as dirichlet_gibbs_block for A and pi and nuts_block or beta_gibbs_block for the emission parameters. It is a Gibbs-family block (Exception 1: discrete states that NUTS cannot target) costing O(T times K^2) per sweep and producing an exact conditional draw. A parity test checks its empirical marginals against analytical Baum-Welch smoothing.",
    "when_to_use": "Pick this block when your model has a Markov chain of discrete latent states z_1:T over a fixed finite state space and you want to sample the whole sequence exactly given the transition, initial, and emission parameters.",
    "example": "HMMGaussian2State.cpp"
  },
  {
    "name": "inv_gamma_gibbs_block",
    "kind": "sampling",
    "title": "Inverse-Gamma Gibbs Leaf Block",
    "summary": "Draws an exact iid Inverse-Gamma sample for a scalar positive parameter whose full conditional is Inverse-Gamma in shape-rate form.",
    "description": "A closed-form Gibbs leaf block for a single positive scalar parameter, such as a variance sigma^2 or scale tau^2, whose full conditional posterior is an Inverse-Gamma distribution in shape-rate parameterization. The user supplies a params_fn lambda that returns the (shape, rate) of the conditional from the current context, and the block draws an exact iid Inverse-Gamma value, avoiding the warmup cost of a nuts_block. It is intentionally library-only and strongly discouraged as a default, since the preferred prior for variance and scale parameters is a Jeffreys p(sigma) proportional to 1/sigma on a nuts_block rather than an Inverse-Gamma. Reserve it for informative Inverse-Gamma priors grounded in external knowledge or genuinely pathological conditionals where the Jeffreys plus half-Normal fallback fails to mix.",
    "when_to_use": "Pick this block only when the conditional is exactly Inverse-Gamma and you have a documented, informative Inverse-Gamma prior or a pathological conditional that the default Jeffreys nuts_block pattern cannot handle.",
    "example": ""
  },
  {
    "name": "ising_cluster_block",
    "kind": "sampling",
    "title": "Swendsen-Wang Cluster Sampler for Ising/Potts",
    "summary": "Samples a discrete Ising or Potts field on an undirected graph using one Swendsen-Wang cluster sweep.",
    "description": "This block draws x in {0, ..., Q-1}^n from the ferromagnetic Ising/Potts target pi(x) proportional to exp(beta times sum over edges of I[x_i = x_j]), with beta > 0 and no external field. Each step performs one full Swendsen-Wang sweep: bond augmentation that activates like-colored edges with probability 1 - exp(-beta), union-find cluster identification, then a fresh uniform recolor of each cluster over all Q states. It is the standard remedy for the catastrophic mixing of per-site Gibbs on strongly-coupled discrete Markov random fields near the critical beta. A make_2d_lattice_edges helper builds 4-NN or 8-NN edge lists for regular image lattices, and beta can be fixed or supplied jointly by a sibling block via a shared-data key.",
    "when_to_use": "Pick this block to sample a discrete MRF (Ising or Potts) with strong local dependence on a graph or image lattice, where single-site Gibbs would mix too slowly.",
    "example": "GMRFPrior.cpp"
  },
  {
    "name": "joint_nuts_block",
    "kind": "sampling",
    "title": "Joint NUTS over coupled continuous parameters",
    "summary": "A single NUTS block that owns multiple named sub-parameters and samples them jointly on one concatenated unconstrained vector.",
    "description": "joint_nuts_block packs two or more named continuous sub-parameters into a single concatenated unconstrained vector and runs one NUTS sampler over them, so a user-supplied joint log-density and gradient drive the whole update. It is the performance escape hatch for models whose likelihood tightly couples several parameters, such as IRT shift-invariance between theta and b, the additive linear mean alpha plus X beta, or fixed and random effects beta and u that share the mean structure. Splitting such parameters into separate per-parameter NUTS blocks makes the step size collapse and the NUTS tree max out, which this block avoids. It supports 15 per-slice constraint kinds (for example REAL, POSITIVE, SIMPLEX, COV_MATRIX) and splits the concatenated vector back into named outputs after each step.",
    "when_to_use": "Pick it when two or more continuous parameters are tightly coupled in the likelihood, so that sampling them in separate Gibbs-wise NUTS blocks would collapse the step size.",
    "example": "BSplineRegression.cpp"
  },
  {
    "name": "lda_collapsed_gibbs_block",
    "kind": "sampling",
    "title": "Collapsed Gibbs Sampler for LDA Topics",
    "summary": "Samples token-level topic assignments z for Latent Dirichlet Allocation using the Griffiths-Steyvers 2004 collapsed Gibbs update, then draws the document and topic simplex parameters from their Dirichlet posteriors.",
    "description": "This block is a specialized sampler for Latent Dirichlet Allocation with fixed K and fixed Dirichlet hyperparameters alpha and beta. It samples each token's discrete topic assignment z_n in {1..K} from its full conditional with theta and phi analytically integrated out: P(z_n = k | rest) is proportional to (n_dk + alpha_k) * (n_kw + beta_w) / (n_k + sum(beta)), maintained via incremental count tables at O(K) per token. After the z sweep it draws the per-document topic proportions theta and per-topic word distributions phi once from their Dirichlet conjugate posteriors via gamma-normalization, exposing all three (z, theta, phi) as named outputs. It replaces the naive composition of categorical and Dirichlet blocks, which is correct in the limit but mixes catastrophically due to strong coupling between z, theta, and phi.",
    "when_to_use": "Pick this block for any LDA-style model where token-level discrete topic assignments z are coupled to per-document and per-topic Dirichlet simplex parameters under fixed K and fixed hyperparameters.",
    "example": "LdaCollapsedGibbs.cpp"
  },
  {
    "name": "mean_field_categorical_vi_block",
    "kind": "sampling",
    "title": "Mean-Field VI for Categorical Latents",
    "summary": "Mean-field variational inference for discrete categorical latent variables z_i in {0,...,K_i - 1}.",
    "description": "This block fits a fully factorised mean-field variational family, q(z) = product over i of Categorical(z_i ; phi_i), to a user-supplied joint log-density over discrete latents. Each per-variable probability vector phi_i is parameterised by an unconstrained vector via an anchored softmax, and the ELBO gradient is computed analytically by summing over each focal variable's states with a shared Monte-Carlo set marginalising the rest (S = 16 draws by default). An exact-enumeration mode evaluates the gradient over the full joint state space (capped at 4096 states) for unit tests and small validation problems. After each step the composite writes a fresh per-variable integer index draw to shared data so MCMC sibling blocks see a new discrete sample each outer iteration.",
    "when_to_use": "Pick this block for discrete categorical latents, even with strong local dependence, when you want a deterministic VI approximation that converges cleanly and you can accept underestimated posterior variance.",
    "example": "CategoricalIsingChainVI.cpp"
  },
  {
    "name": "mean_field_gaussian_vi_block",
    "kind": "sampling",
    "title": "Mean-Field Gaussian Variational Inference Block",
    "summary": "Fits a diagonal Gaussian variational approximation to a parameter block by optimizing the ELBO with the RAABBVI optimizer.",
    "description": "This block maintains a fully factorized variational posterior q(eta), a product of independent normals N(eta_i; mu_i, sigma_i^2), on the unconstrained scale eta = unconstrain(theta), and optimizes the negative ELBO via the RAABBVI-lite procedure (averaged Adam plus Polyak-Ruppert iterate averaging plus geometric step-size decay plus a symmetric-KL termination rule). Each step() call runs one optimizer step, and afterward the block writes a fresh q-sample theta = constrain(eta_draw) to shared data so sibling blocks see a new draw each outer iteration, while get_current() returns the deterministic q-mean point estimate. It accepts the user's natural-scale log density and gradient through the same log_density_gradient_fn signature that nuts_block uses, so no new user-side infrastructure or hand-written Jacobian is required. It is one of the two primary v1 concrete subclasses of the abstract vi_block, alongside full_rank_gaussian_vi_block.",
    "when_to_use": "Pick this block when you want a fast approximate posterior for a parameter block whose coordinates are roughly independent, accepting the mean-field independence assumption and its known marginal-variance underestimation in exchange for speed.",
    "example": ""
  },
  {
    "name": "niw_cluster_gibbs_block",
    "kind": "sampling",
    "title": "Full-covariance NIW Gaussian cluster Gibbs",
    "summary": "Samples per-cluster mean and full covariance (mu_k, Sigma_k) jointly across K_trunc clusters under a conjugate Normal-Inverse-Wishart prior.",
    "description": "This closed-form Gibbs leaf draws each cluster's (mu_k, Sigma_k) from a conjugate Normal-Inverse-Wishart prior, using the textbook Murphy 2007 conjugate update for populated clusters and drawing empty clusters straight from the prior (Ishwaran and James 2001 convention). Inverse-Wishart draws use the Bartlett decomposition, which is numerically stable and avoids matrix inversion on the hot path beyond one inv_sympd of Psi_n per cluster per step. It is the full-covariance companion to the diagonal normal_gamma_cluster_gibbs_block, recovering off-diagonal correlation structure within clusters rather than just an axis-aligned envelope. The output sigma exposes covariance (not precision), so downstream consumers know which they are reading.",
    "when_to_use": "Pick this block for Gaussian mixture or clustering models where dimensions d are at least 2 and observations within a cluster have off-diagonal correlation, so a full covariance matters rather than a diagonal one.",
    "example": "HDPGaussianMixture.cpp"
  },
  {
    "name": "normal_gamma_cluster_gibbs_block",
    "kind": "sampling",
    "title": "Normal-Gamma Diagonal Gaussian Cluster Gibbs",
    "summary": "A closed-form Gibbs leaf that jointly samples per-cluster diagonal-Gaussian parameters (mu_k, lambda_k) across K_trunc clusters under conjugate Normal-Gamma priors.",
    "description": "This block performs the exact conjugate Normal-Gamma update for the per-cluster mean mu_k and precision lambda_k of a diagonal Gaussian, treating each dimension independently. In one O(K_trunc * d + N) sweep it draws populated clusters from the data-driven posterior and empty clusters from the prior, matching Ishwaran and James 2001. It is the cluster-emission block for truncated stick-breaking BNP mixtures such as DP and Pitman-Yor Gaussian mixtures, replacing NUTS because exact conjugate draws avoid getting stuck on the flat-prior posterior of empty clusters and give a 5 to 10x speedup. It outputs mu and lambda each of length K_trunc * d in cluster-major row order, where lambda holds precisions, not variances.",
    "when_to_use": "Pick this block as the cluster-emission step in a truncated stick-breaking Gaussian mixture when the dimensions inside each cluster are approximately independent (diagonal covariance); for off-diagonal within-cluster correlation use niw_cluster_gibbs_block instead.",
    "example": "DPGaussianMixture.cpp"
  },
  {
    "name": "nuts_block",
    "kind": "sampling",
    "title": "NUTS block with persistent adaptation",
    "summary": "Samples one continuous parameter with No-U-Turn HMC whose dual-averaging step-size adaptation persists across outer Gibbs sweeps.",
    "description": "nuts_block runs MCMClib's No-U-Turn sampler on a single continuous parameter, driven by a user-supplied log-density-and-gradient oracle (a plain std::function), so it makes no assumption about the autodiff backend and works with BridgeStan, a hand-written C++ lambda, or an AI-generated functor. Constrain and unconstrain transforms are injected as functions, letting the same block handle real, strictly-positive, interval, simplex, ordered, Cholesky-of-correlation, and unit-vector parameters via the constraints wrappers. Each call to step() runs NUTS transitions with use_persistent_adapt on, so the dual-averaging state (epsilon_bar, h, mu, adapt_iter) accumulates across calls and NUTS resumes from the exact step size it had reached after other blocks update in between. The mass matrix is fixed at construction in this version, though it can be reset between sweeps with set_precond_matrix.",
    "when_to_use": "Pick nuts_block for a single, genuinely scalar non-conjugate continuous parameter; for tightly-coupled continuous parameters prefer joint_nuts_block, which is the default.",
    "example": "ARDLasso.cpp"
  },
  {
    "name": "order_mcmc_block",
    "kind": "sampling",
    "title": "Order MCMC for Bayesian network structure",
    "summary": "Samples total orderings of discrete variables via Metropolis-Hastings to learn Bayesian-network structure with BDeu Bayesian model averaging over DAGs.",
    "description": "This block implements Friedman-Koller 2003 order MCMC for Bayesian-network structure learning, with target distribution P(order | D) proportional to P(D | order) times P(order), where P(order) is uniform over the n! orderings. The marginal likelihood factorizes across the n variables because each variable's candidate parent set is restricted to its predecessors in the order, scored with the Heckerman-Geiger-Chickering 1995 BDeu family. Orders are proposed by a user-specified mixture of symmetric any-pair swaps and adjacent swaps, and at each step a DAG is drawn by sampling each variable's parent set conditional on the current order. The v1.2 scope covers discrete data with per-column cardinalities, up to n = 64 variables, a configurable max-parents cap (default 5), and a selectable uniform-on-DAGs or fan-in-penalised structure prior.",
    "when_to_use": "Pick this block to learn the structure of a Bayesian network from discrete data, where the combinatorial space of DAGs is intractable for gradient-based or per-DAG Gibbs samplers and order MCMC reduces the search to permutations.",
    "example": "OrderMCMCBN.cpp"
  },
  {
    "name": "pg_logistic_block",
    "kind": "sampling",
    "title": "Polya-Gamma augmentation for logistic regression",
    "summary": "Samples logistic-regression coefficients in closed form using Polya-Gamma data augmentation.",
    "description": "This block uses the Polson, Scott and Windle (2013) Polya-Gamma scheme for Bernoulli (logistic) responses. It introduces auxiliary Polya-Gamma variables so the conditional for the coefficients becomes Gaussian, giving an exact conjugate Gibbs update with no tuning. It is the recommended way to sample a logistic regression linear predictor inside a larger model.",
    "when_to_use": "Pick this block for Bernoulli (logistic) regression coefficients, where Polya-Gamma augmentation gives an exact Gaussian conditional and avoids NUTS tuning.",
    "example": "LogisticRegression.cpp"
  },
  {
    "name": "poisson_multinomial_aug_block",
    "kind": "sampling",
    "title": "Poisson-Multinomial Gamma Augmentation for Logistic BART",
    "summary": "Augments multinomial logistic BART by sampling a per-observation latent gamma variable so the likelihood factorizes into independent Poisson trees, one per non-reference category.",
    "description": "This block implements the classical Poisson-multinomial gamma trick for multinomial or binary logistic BART via genBART, under the reference-category identified parameterization where category 0 is fixed as the reference. It draws a latent phi_i per observation from Gamma(n_i, 1 + sum over non-reference categories of exp(r_j(x_i))), which for single-observation data reduces to an exponential. Conditional on phi_i, the multinomial likelihood factorizes into C-1 conditionally independent Poisson-like likelihoods that each match genBART's poisson_lik with offset log(phi_i). The block writes log_phi and the per-category indicators u_j into shared data so downstream genbart_block children can run independent Poisson tree sweeps that jointly compose the multinomial logistic model.",
    "when_to_use": "Pick this block to fit multinomial logistic BART with three or more categories (or a binary case you want handled uniformly) by pairing it with one genbart_block per non-reference category reading the shared log_phi offset.",
    "example": "GBartMultinomial.cpp"
  },
  {
    "name": "probit_aug_block",
    "kind": "sampling",
    "title": "Albert-Chib probit latent z augmentation",
    "summary": "Draws the Albert-Chib latent variable z by a closed-form truncated-normal Gibbs step, turning any probit binary likelihood into a Gaussian one.",
    "description": "This block is the closed-form Gibbs leaf for the Albert and Chib (1993) data-augmentation latent z in any probit-link binary model, where y_i is Bernoulli with success probability Phi(mu_i + offset_i). It draws each z_i from N(mu_i + offset_i, 1) truncated to the positive half-line when y_i = 1 and the negative half-line when y_i = 0, using Robert's (1995) truncated-normal sampler. Because the draws are conditionally independent across observations, the whole vector is sampled in one pass with no inner loop. Composing it with any downstream Gaussian-likelihood block (such as nuts_block, bart_block, or a linear or GP mean) that reads z as its working response recovers the standard Bayesian probit sampler; sigma is fixed at 1.0 by probit identifiability.",
    "when_to_use": "Pick this block when you have a probit-link binary likelihood and want a downstream Gaussian block to handle the mean structure via a clean closed-form latent-z Gibbs step rather than inlining the truncated-normal draw.",
    "example": "ProbitRegression.cpp"
  },
  {
    "name": "rjmcmc_block",
    "kind": "sampling",
    "title": "Reversible-Jump MCMC for Dirac Spike-and-Slab",
    "summary": "Trans-dimensional birth-death sampler for Dirac spike-and-slab variable selection, where each coefficient is either exactly zero or drawn from a continuous slab.",
    "description": "This block runs reversible-jump MCMC (Green 1995) on a paired (gamma, beta) state, where gamma is a binary inclusion vector and beta[j] equals exactly 0 when gamma[j] = 0 (the dimension-changing Dirac spike-and-slab geometry that naive Gibbs and NUTS handle incorrectly). Each sweep visits coefficients in random order, optionally Gibbs-updates an active beta[j] via a continuous_update hook, then proposes a birth or death flip of gamma[j]. Birth proposals come in three families that all spare the user from hand-writing a Jacobian: an identity-coordinate default, library 1D transforms (identity, linear, affine), and custom templated bijections whose Jacobian is computed by runtime autodiff. State is stored as a fixed-size length-2p vector, so the dimension change appears only in the accept ratio, not in the history.",
    "when_to_use": "Pick this block for Dirac spike-and-slab variable selection, change-point insertion with prior-sampled values, or finite mixture-component birth-death, where a coefficient must be exactly zero when inactive rather than a continuous near-zero relaxation.",
    "example": "SpikeSlabRJMCMC.cpp"
  },
  {
    "name": "softbart_block",
    "kind": "sampling",
    "title": "Soft BART tree-ensemble regression sweep",
    "summary": "Runs one Soft BART tree-ensemble sweep per step to model a smooth regression function f(x) from data.",
    "description": "softbart_block is a block_sampler wrapper around the vendored Soft BART model of Linero and Yang (2018), fitting y = f(x) + epsilon with one tree-ensemble sweep per call to step(). Unlike hard BART, Soft BART replaces hard cutpoints with smooth logistic activation around a learned bandwidth tau, giving differentiable predictions that tend to outperform hard BART on smooth response surfaces. It exposes the same uniform six-method plug-in interface as bart_block and genbart_block (set_X, set_Y, set_data, set_offset, update_step, predict), and snapshots the forest, sigma, and variable counts into its history buffers each step. The kernel draws from its own seedable RNG stream, so the std::mt19937_64 argument passed to step() is ignored and the residual noise sigma is handled by a sibling block.",
    "when_to_use": "Pick this block when you need a flexible nonparametric regression mean f(x) and expect the true response surface to be smooth, where Soft BART's differentiable predictions outperform hard BART.",
    "example": "SoftBartNoise.cpp"
  },
  {
    "name": "split_merge_block",
    "kind": "sampling",
    "title": "Jain-Neal Split-Merge Cluster Partition Move",
    "summary": "A Metropolis-Hastings proposal that toggles a cluster allocation vector between merged and split partitions in a single step to accelerate mixing in mixture models.",
    "description": "This block implements the Jain-Neal 2004 split-merge MH proposal on a cluster-allocation vector z, in the truncated stick-breaking regime where pi is held fixed and the prior on z is product-of-categorical Cat(pi). Per-observation Gibbs is correct but slow to escape local modes, since single-i flips need O(n) low-probability moves to merge two clusters or split one; this proposal toggles the whole partition in one step, with acceptance computed via a restricted-Gibbs proposal density run for T scans over the affected observations. It supports two emission shapes, a diagonal Normal-Gamma path (lambda_key) and a full-covariance NIW path (sigma_key), exactly one of which must be set. Because it is an MH proposal rather than a conjugate draw, it is meant to co-compose with categorical_gibbs_block, which both drive the same shared allocation z.",
    "when_to_use": "Pick this block to accelerate mixing on cluster partitions in a truncated Dirichlet-process or finite Gaussian mixture when per-observation categorical Gibbs sweeps get stuck and cannot easily merge or split clusters.",
    "example": ""
  },
  {
    "name": "stick_breaking_block",
    "kind": "sampling",
    "title": "Truncated stick-breaking simplex sampler",
    "summary": "A closed-form Gibbs leaf that samples a truncated stick-breaking simplex pi by drawing independent Beta sticks.",
    "description": "This block samples a truncated stick-breaking-process simplex pi of length K_trunc by drawing independent Beta sticks V_k and forming pi_k = V_k times the product over j less than k of (1 - V_j), under the Ishwaran and James (2001) truncation. The user supplies the per-stick Beta parameters a_k and b_k as functor closures, so the same block can implement a Dirichlet Process, Pitman-Yor, HDP, or any custom stick-breaking weight scheme without the library knowing which process it is. Beta draws use the gamma-trick (X ~ Gamma(a_k, 1), Y ~ Gamma(b_k, 1), V_k = X / (X + Y)), which is numerically stable for any a_k, b_k greater than 0. Use dirichlet_gibbs_block instead when the conditional is exactly a single Dirichlet; the two are not interchangeable because the correlation structure differs.",
    "when_to_use": "Pick this block when the simplex conditional is a sequence of independent Betas with a stick-breaking product, as in truncated DP, Pitman-Yor, HDP, or kernel-stick-breaking mixtures.",
    "example": "DPGaussianMixture.cpp"
  },
  {
    "name": "structured_categorical_vi_block",
    "kind": "sampling",
    "title": "Structured Mean-Field VI for Discrete Latents",
    "summary": "Saul-Jordan 1996 partially-factorised mean-field VI that keeps joint correlation within user-defined cliques while factorising across them.",
    "description": "This block implements Saul-Jordan 1996 \"structured\" (partially-factorised) mean-field variational inference for discrete categorical latents. Given a user-supplied partition of the variables into cliques, the variational family is q(z) = prod_C q_C(z_C), where each q_C is a joint Categorical over the clique's joint state space, parameterised by an anchored softmax. It refines the fully-factorised mean field by preserving intra-clique correlation exactly while factorising across cliques, optimising an analytical clique-sum-over-state ELBO gradient via the RAABBVI optimizer with a PSIS-k-hat diagnostic. The clique marginalisation runs in exact enumeration mode (capped at a configurable joint-state limit) or in Monte Carlo mode (default); singleton cliques reduce to fully-factorised mean field and a single grand-clique gives exact inference.",
    "when_to_use": "Pick this block for discrete latents whose true posterior has strong intra-clique coupling but weak inter-clique coupling, where the fully-factorised mean field is too crude but full enumeration is too expensive.",
    "example": "StructuredPottsVI.cpp"
  },
  {
    "name": "univariate_slice_sampling_block",
    "kind": "sampling",
    "title": "Univariate Slice Sampling Block",
    "summary": "Samples a single scalar parameter via Neal 2003 univariate slice sampling using only a log-density, no gradient.",
    "description": "This block implements the univariate (1-D scalar) slice sampler of Neal 2003 (section 4.1) with stepping-out plus shrinkage, including a random step-out budget split for reversibility. The user supplies only a natural-scale log-density lambda; the sampler machinery is textbook and library-owned, and constraints are handled by an optional (constrain, unconstrain) pair so sampling runs on the unconstrained scale. Shrinkage guarantees acceptance within finite iterations because the bracket converges back to the current point. It carries the same AI-safety profile as nuts_block, requiring no derivation of conditional posteriors.",
    "when_to_use": "Pick this only for a single scalar parameter when NUTS cannot be used because the log-density is non-differentiable, is a black-box library call whose gradient is infeasible, or has a gradient that is prohibitively expensive relative to a plain log-density evaluation.",
    "example": "GPTimeSeries.cpp"
  },
  {
    "name": "vi_block",
    "kind": "sampling",
    "title": "Abstract base for variational-inference blocks",
    "summary": "An abstract base class that defines the variational-inference contract shared by all concrete VI blocks in the library.",
    "description": "vi_block is the abstract base for the VI family, deriving from block_sampler and reporting engine_kind VI so the composite router treats it differently from sampling blocks. Each step(rng) runs one RAABBVI optimizer step rather than drawing a sample, current() returns the deterministic q-mean point estimate, and the new current_sample(rng) method draws a fresh q-sample theta = constrain(eta) for eta drawn from q. A key hybrid-correctness invariant is enforced: when a VI child writes its value into shared data for an MCMC sibling, it must write a q-sample, never the q-mean, so the sibling integrates over q instead of conditioning on a point estimate. Users do not construct vi_block directly; two concrete subclasses ship in v1, mean_field_gaussian_vi_block and full_rank_gaussian_vi_block.",
    "when_to_use": "You never pick vi_block directly; choose one of its concrete subclasses when you want fast variational approximation of a parameter block, optionally mixed with MCMC siblings in a hybrid sampler.",
    "example": "CategoricalIsingChainVI.cpp"
  },
  {
    "name": "autodiff_wrap",
    "kind": "utility",
    "title": "Autodiff bridge for NUTS gradient oracles",
    "summary": "It turns a user-written natural-scale scalar log-density into the unconstrained value-plus-gradient oracle that nuts_block needs, with all transforms and Jacobians handled automatically by reverse-mode autodiff.",
    "description": "This internal header bridges a templated natural-scale log-density (written once to work with both plain doubles and autodiff var types) to the (theta_unc, grad) interface that nuts_block expects. The wrap maps unconstrained parameters back to the natural scale via each constraint's inverse transform (identity for real, exp for positive, and similar), adds the log absolute Jacobian of that transform, evaluates the user log-density, and asks autodiff for the gradient of the total with respect to the unconstrained vector. Because the chain rule flows through the differentiable transforms, the generated code never writes a gradient, a Jacobian, or any unconstrained-scale math by hand. Supported constraint kinds in V1 are real, positive, lower_bounded, upper_bounded, and interval, with simplex and other structured constraints reserved for a later V2 overload set.",
    "when_to_use": "",
    "example": ""
  },
  {
    "name": "backend_neutral",
    "kind": "utility",
    "title": "Backend-neutral helpers for shared sampler code",
    "summary": "Provides backend-agnostic error, warning, and special-function helpers so shared sampler code compiles unchanged under both the R (Rcpp) and Python (pybind11) backends.",
    "description": "This internal header supplies small helpers in the ai4b namespace so a sampler's shared C++ code does not have to name Rcpp or R symbols directly. It provides ai4b::stop and ai4b::warning, which forward to Rcpp::stop and Rcpp::warning under the R backend (preserving R error and warning conditions and message formatting) and otherwise throw std::runtime_error or print to stderr, which pybind11 maps to a Python exception. It also provides ai4b::lgammafn as a replacement for R::lgammafn and ai4b::digamma as a replacement for R::digamma, both implemented with standard C++ math and validated against R. The correct implementation is selected at compile time via backend macros, so the same shared region works under either backend while pulling in nothing backend-specific on its own.",
    "when_to_use": "",
    "example": ""
  },
  {
    "name": "bde_scorer",
    "kind": "utility",
    "title": "BDe Family Score for Bayesian Networks",
    "summary": "A closed-form kernel that computes the Bayesian Dirichlet (BDe/BDeu) family score used to evaluate parent sets in Bayesian network structure learning.",
    "description": "This internal utility header provides the bde_scorer class, which scores a node given a candidate parent set using the BDe family score from Heckerman, Geiger, and Chickering (1995, Eq. 28), with BDeu pseudocounts (Buntine 1991) as the default. It takes integer-encoded discrete data and per-variable cardinalities, and family_score(i, parents) returns the log marginal likelihood of the family plus a per-family uniform structure prior (Friedman-Koller 2003). Pseudocounts default to BDeu form, N'_ijk = alpha / (r_i * q_i) and N'_ij = alpha / q_i, with equivalent sample size alpha defaulting to 1. It is the supporting closed-form kernel behind order_mcmc_block and is unit-tested against the canonical 2-node Heckerman example.",
    "when_to_use": "",
    "example": ""
  },
  {
    "name": "block_sampler",
    "kind": "utility",
    "title": "Abstract base class for MCMC blocks",
    "summary": "Defines the abstract base class and design contract that every composable MCMC block in AI4BayesCode must satisfy.",
    "description": "block_sampler is the abstract base class for every block in the library: a self-contained, stateful MCMC updater for one block of parameters inside a larger Gibbs sampler. It fixes the interface through which blocks talk to the outside world, namely set_context to receive external inputs, step to run one closed MCMC update, and current and set_current to read or warm-start the block's value on the natural user-facing scale. It enforces a strict contract in which blocks may not reference, read, or call each other, so only the composite_block knows how blocks fit together via a fixed Gibbs order and shared data. It also specifies the optional history mode, where enabled blocks buffer every draw and get_history returns a backend-neutral named map of matrices that auto-wraps to an R list or a Python dict.",
    "when_to_use": "",
    "example": ""
  },
  {
    "name": "bnp_utils",
    "kind": "utility",
    "title": "Bayesian Nonparametric Utility Functions",
    "summary": "A header-only collection of Bayesian-nonparametric helper functions in namespace AI4BayesCode::bnp for CRP and Pitman-Yor priors, cluster counts, and DP concentration sampling.",
    "description": "This internal header provides short, pure helper functions used by stick_breaking_block, the truncated stick-breaking example wrappers such as DPGaussianMixture and PYGaussianMixture, and any user-written Neal Algorithm 2 or 8 composition. It supplies counts_from_z to histogram 1-indexed cluster assignments, crp_log_prior and py_log_prior for Chinese-restaurant-process and Pitman-Yor allocation log-priors, and crp_sample_new_assignment and py_sample_new_assignment to draw fresh cluster labels from those weights. It also includes sample_alpha_escobar_west, an Escobar and West (1995) auxiliary-variable Gibbs step that draws the DP concentration alpha conditioning only on the sufficient statistic of occupied-cluster count k and sample size n. It is header-only so the same functions can be called inside library blocks and inside user log_probs_fn and refresher lambdas without a separate compile and link step.",
    "when_to_use": "",
    "example": ""
  },
  {
    "name": "celerite_marginal_likelihood",
    "kind": "utility",
    "title": "Celerite Log-Marginal-Likelihood Helper",
    "summary": "A stateless function that returns log p(y given kernel params) for a 1-D time series under a celerite semi-separable GP kernel.",
    "description": "This internal header provides celerite_log_marginal, a pure-function wrapper around the celerite solver that returns the log-marginal-likelihood log p(y given kernel params) for a 1-D time series with semi-separable kernel structure (Foreman-Mackey et al. 2017). Unlike the stateful celerite_gp_block, which caches the log-density for the current shared_data state, this helper constructs a temporary CholeskySolver internally from raw inputs (times, response, real-term and complex-term kernel parameters, observation noise, and a jitter stabilizer) and discards it on return, so sibling hyperparameter blocks can evaluate the log-marginal at a proposed parameter value without mutating another block's solver. Per-call cost is O(N) in the number of points, making it cheap enough to call several times per step from a slice-sampler or Metropolis-Hastings lambda for N up to roughly 10,000. It returns negative infinity when the Cholesky factor is not positive-definite, and supports hyperparameter MCMC, log-evidence sweeps, and independent verification of celerite_gp_block.",
    "when_to_use": "",
    "example": ""
  },
  {
    "name": "constraints",
    "kind": "utility",
    "title": "Pre-tested constraint transforms for log-density lambdas",
    "summary": "Provides pre-tested unconstraining transforms that handle all the Jacobian arithmetic so AI-written log-density lambdas only need to supply a natural-scale density and its natural-scale gradient.",
    "description": "This internal header supplies a library of constraint transforms (real, positive, simplex, lower_bounded, upper_bounded, interval, ordered, cholesky_corr, and unit_vector) for use inside log-density functions that feed gradient-based samplers like NUTS. Each transform exposes a wrap helper that maps an unconstrained parameter vector to the natural scale, calls the user's inner lambda, adds the log absolute Jacobian determinant, and assembles the unconstrained-scale gradient via the chain rule plus the gradient of the log Jacobian. This isolates the error-prone steps (choosing the unconstraining transform, computing its log Jacobian, and applying the chain rule) so an AI-written block only writes a pure natural-scale likelihood-plus-prior and its natural-scale gradient. Zero-parameter constraints offer static constrain, unconstrain, and templated wrap functions, while parameterised ones take their bounds explicitly and provide factory helpers that return transform functions ready to assign to a NUTS block configuration.",
    "when_to_use": "",
    "example": ""
  },
  {
    "name": "ode_rk45",
    "kind": "utility",
    "title": "Adaptive ODE Integrator (Dormand-Prince 5/4)",
    "summary": "A header-only, dependency-free Dormand-Prince 5(4) adaptive Runge-Kutta solver that lets user log-density lambdas embed mechanistic ODE models inside a NUTS block.",
    "description": "This utility provides a stateless rk45 function that forward-integrates a non-stiff ODE system from initial state y0 over user-supplied output times, returning a matrix whose rows give the state at each time. It uses the Dormand-Prince 5(4) method with adaptive step-size control via a PI error controller, giving a 5th-order solution plus an embedded 4th-order error estimate. It exists so AI4BayesCode can support pharmacokinetic, epidemiological, and ecological models, serving as the analogue of Stan's integrate_ode_rk45. Scope is limited to non-stiff forward integration with no Jacobian or sensitivity support, so gradients for NUTS must come from finite differences or user-supplied analytical derivatives.",
    "when_to_use": "",
    "example": ""
  },
  {
    "name": "pybind_casters",
    "kind": "utility",
    "title": "pybind11 Type Casters for Python Bindings",
    "summary": "Provides pybind11 type casters that convert AI4BayesCode's neutral C++ types and Armadillo vectors and matrices to and from numpy arrays at the Python boundary.",
    "description": "This internal header registers pybind11 type casters so any .cpp that emits a PYBIND11_MODULE can return AI4BayesCode types such as state_map, history_map, dag_info, and adaptation_info to Python. It converts arma::vec to a 1-D numpy array and arma::mat to a 2-D numpy array, and maps state_map and history_map to Python dicts of numpy arrays. Because Armadillo matrices are column-major and numpy defaults to row-major, the matrix caster copies into the row-and-column order a user expects, costing one transposition on each boundary crossing. Include it before pybind11.h in the including .cpp, since this header pulls in pybind11 itself and registers the casters.",
    "when_to_use": "",
    "example": ""
  },
  {
    "name": "rcpp_wrap",
    "kind": "utility",
    "title": "Rcpp Conversion Wrappers for Neutral Types",
    "summary": "Provides Rcpp wrap and as specializations so the library's neutral C++ types convert to and from R objects.",
    "description": "This internal header supplies Rcpp::wrap and Rcpp::as specializations for the library's neutral types declared in types.hpp, including state_map, history_map, dag_info, and adaptation_info. It lets an RCPP_MODULE method that returns these types convert them to R objects such as named lists of numeric vectors and matrices, and convert an R named list back into a state_map. Include it from any .cpp that uses RCPP_MODULE and returns these types. The specializations are guarded behind Rcpp ifdefs, so the header stays safe to include in a Python build where nothing is emitted.",
    "when_to_use": "",
    "example": ""
  },
  {
    "name": "rjmcmc_custom_bijection",
    "kind": "utility",
    "title": "Custom RJMCMC Bijection With Autodiff Jacobian",
    "summary": "It lets rjmcmc_block use a user-supplied nonlinear 1D birth/death map whose Jacobian is computed automatically by autodiff rather than hand-written.",
    "description": "This internal header extends rjmcmc_block with a third proposal family: a custom 1D scalar bijection for non-linear monotone maps that the library's identity-coordinate and built-in linear/affine transforms cannot express. The user writes a single templated forward map beta = T(u) plus a non-templated analytic inverse u = T inverse of beta, and the framework instantiates the same forward at double for sampling and at autodiff::var to compute the Jacobian |dbeta/du| by reverse-mode autodiff, so no Jacobian formula is ever hand-written. A make_templated_bijection_1d wrapper exposes this as the existing transform_1d_base interface, requiring no changes to rjmcmc_block. It is limited to dimension-preserving 1D-to-1D maps (k_aux = k_new = 1) with an analytic inverse, and provides gen-time sanity probes for round-trip accuracy, Jacobian non-singularity, and the forward/reverse Jacobian inverse-pair invariant.",
    "when_to_use": "",
    "example": ""
  },
  {
    "name": "rjmcmc_transforms",
    "kind": "utility",
    "title": "RJMCMC Bijective Transforms With Auto Jacobians",
    "summary": "Provides bijective proposal transform classes that compute their own log-Jacobians for use in reversible-jump MCMC.",
    "description": "This header supplies a small menu of library-provided bijective transform classes (identity, linear, and affine maps of the form x maps to M x + b) for use with the rjmcmc_block dimension-changing proposals. Each transform class computes the absolute determinant of its Jacobian internally from the user-supplied matrix M, diagonal D, or affine pair (M, b), so users never hand-write Jacobian formulas; the forward map feeds birth proposals, the reverse map feeds death proposals, and the precomputed abs Jacobian enters the Metropolis-Hastings acceptance ratio. It also includes minimal supporting matrix utilities for cofactor, determinant, and inverse. The transform classes are ported, with namespace and C++17 modernization changes only, from the librjmcmc project under a GPL-compatible CeCILL-B license.",
    "when_to_use": "",
    "example": ""
  },
  {
    "name": "score_cache",
    "kind": "utility",
    "title": "Friedman-Koller Family-Score Cache",
    "summary": "A precomputed Bayesian-network family-score cache that supplies order-marginal log-scores and parent-set sampling to the order-MCMC kernel.",
    "description": "This internal utility implements the three-tier family-score heuristic of Friedman and Koller (2003, section 4.2): top-C candidate-parent prescreening per node, top-F family caching over parent sets up to size k, and gamma-pruning that drops families scoring more than gamma below the best per-node family. Built from a scorer and config, it caches each node's families as sorted (score, parent_mask) pairs so order queries can filter for order-consistency and accumulate via log-sum-exp. It exposes order_node_score and order_log_score for order-marginal log-scores and sample_parent_set to draw a node's parent set proportional to its score under a given order. It is the Tier C kernel backing the order_mcmc_block rather than a standalone sampler.",
    "when_to_use": "",
    "example": ""
  },
  {
    "name": "shared_data",
    "kind": "utility",
    "title": "Shared Data: Cross-Block State Container",
    "summary": "A composite-owned, string-keyed store of all cross-block state plus the dependency metadata a Gibbs sweep needs to keep derived quantities current.",
    "description": "shared_data_t is the single place in block_mcmc that knows how blocks depend on one another, holding fixed data, block parameters, and derived quantities in one string-keyed map of arma::vec values. It is owned by exactly one composite_block, which uses it during each Gibbs sweep to build a per-block context, write back updated values, and refresh derived keys. Three per-block tables (dependencies, invalidates, and refresher closures) together encode the model DAG, so deterministic quantities like residual or linear_predictor stay consistent after every block update. It also supports stochastic refreshers for posterior-predictive draws and is not thread-safe, so parallel chains should each own their own instance.",
    "when_to_use": "",
    "example": ""
  },
  {
    "name": "types",
    "kind": "utility",
    "title": "Backend-Neutral Result Types",
    "summary": "Defines the backend-neutral C++ result types that every public sampler accessor returns so both R and Python bindings can wrap them.",
    "description": "This internal header declares the neutral data types returned by every user-facing accessor on block_sampler and composite_block, keeping backend-specific types like Rcpp::List or py::dict out of library signatures. It provides state_map (named parameter vectors for context and per-step outputs), history_map (named matrices of stored draws and predict outputs), dag_info (the four DAG edge-layers plus replaceable data-input keys returned by get_dag), and adaptation_info (a NUTS step-size, mass-matrix, and dual-averaging snapshot for save and restore). Rcpp auto-wraps these to named R lists and numeric arrays, while pybind11 converts them through the casters in pybind_casters.hpp. The design rule is that any quantity crossing the public boundary must first be expressed with one of these types.",
    "when_to_use": "",
    "example": ""
  },
  {
    "name": "vi_optimizer",
    "kind": "utility",
    "title": "RAABBVI Optimizer Helpers for VI Blocks",
    "summary": "A header-only set of optimizer and diagnostic utilities that variational inference blocks compose to run robust black-box VI.",
    "description": "This internal utility header ports the Welandawe 2022 RAABBVI (\"Robust, Automated, and Accurate Black-box Variational Inference\") algorithm into reusable building blocks for the library's VI blocks. It provides a config struct of optimizer hyperparameters, per-block optimizer state, and an avgAdam update step with Polyak-Ruppert iterate averaging. It also supplies a closed-form symmetric KL between two mean-field Gaussian variational distributions and a Pareto-smoothed importance sampling k-hat diagnostic (Vehtari et al. 2024). The orchestration of the inner loop, geometric learning-rate decay, and SKL-based termination lives inside each concrete VI block that composes these helpers, not in this file.",
    "when_to_use": "",
    "example": ""
  }
];
