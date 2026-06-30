// Auto-generated core block catalogue. Regenerate from the library, do not hand-edit.
window.CORE_BLOCKS = [
  {
    "name": "bart_block",
    "kind": "sampling",
    "title": "Bayesian Additive Regression Trees mean function",
    "summary": "Draws the nonparametric regression mean \\(f\\) in \\(y_i \\sim \\mathcal{N}(f(x_i), \\sigma^2)\\) using a Bayesian sum-of-trees model.",
    "description": "Fits a flexible nonparametric mean function with Bayesian Additive Regression Trees (BART), modeling \\(y_i \\sim \\mathcal{N}(f(x_i), \\sigma^2)\\) where \\(f(x) = \\sum_{t=1}^{T} g_t(x)\\) is a sum of regression trees. Each update is one Bayesian backfitting sweep: every tree is regrown by Metropolis-Hastings moves against its partial residual and its leaf values are refreshed under the BART regularization prior, which keeps individual trees weak so the ensemble captures nonlinear effects and interactions without manual basis or knot choices. The residual scale \\(\\sigma\\) is updated by a companion variance step, and an optional sparse Dirichlet split prior (DART) shrinks unhelpful predictors toward zero and yields a variable-importance score in high-dimensional settings. References Chipman, George and McCulloch (2010) and Linero (2018).",
    "when_to_use": "Reach for it when you want a flexible, automatic regression surface for a continuous outcome, \\(y = f(x) + \\epsilon\\), with nonlinearities and interactions you do not want to specify by hand.",
    "example": "BartNoise.cpp",
    "group": "Tree-ensemble priors"
  },
  {
    "name": "beta_gibbs_block",
    "kind": "sampling",
    "title": "Exact Beta-conjugate Gibbs draw for probabilities",
    "summary": "Draws a scalar probability parameter exactly from its Beta full conditional in one Gibbs step.",
    "description": "Samples a probability or mixing proportion \\(\\pi \\in (0,1)\\) whose full conditional is exactly a Beta distribution, using an exact closed-form Gibbs update rather than gradient-based sampling. It targets conjugate Beta-Binomial and Beta-Bernoulli structures: with a \\(\\pi \\sim \\mathrm{Beta}(a,b)\\) prior and Bernoulli inclusion indicators \\(\\gamma_j \\sim \\mathrm{Bernoulli}(\\pi)\\), the posterior is \\(\\pi \\mid \\gamma \\sim \\mathrm{Beta}\\!\\big(a + \\sum_j \\gamma_j,\\, b + p - \\sum_j \\gamma_j\\big)\\); the analogous Beta-Binomial case gives \\(p \\mid y \\sim \\mathrm{Beta}(a+y,\\, b+n-y)\\). Because the conditional depends only on a single sufficient statistic, the draw is exact and avoids the warmup and tuning waste of running NUTS on a tight one-dimensional posterior. See Gelman et al. (2013), Bayesian Data Analysis.",
    "when_to_use": "Reach for it when a scalar probability or spike-and-slab mixing proportion has a conjugate Beta prior and an exactly Beta full conditional, so it can be sampled in closed form instead of with NUTS.",
    "example": "SpikeSlabRJMCMC.cpp",
    "group": "Continuous-conjugate Gibbs"
  },
  {
    "name": "binary_gibbs_block",
    "kind": "sampling",
    "title": "Gibbs sampler for binary indicators",
    "summary": "Exact Gibbs updates for vector-valued binary parameters \\(z \\in \\{0,1\\}\\), such as the inclusion indicators in a spike-and-slab variable-selection model.",
    "description": "Samples a vector of binary (0/1) parameters \\(z = (z_1, \\dots, z_p)\\) by drawing each one directly from its full conditional Bernoulli distribution, a closed-form Gibbs step that requires no tuning, gradients, or accept-reject. The canonical use is Stochastic Search Variable Selection, where \\(z_j\\) flags whether predictor \\(j\\) is active and the conditional inclusion probability balances the prior inclusion rate against how well coefficient \\(\\beta_j\\) fits the slab versus the spike, \\(P(z_j = 1 \\mid \\cdot) = 1 / (1 + \\exp(-\\eta_j))\\). It targets the classical point-mass-free spike-and-slab where both the spike and the slab are Gaussian, \\(\\beta_j \\mid z_j \\sim z_j\\,\\mathcal{N}(0,\\tau^2) + (1-z_j)\\,\\mathcal{N}(0,\\tau_0^2)\\), and more generally any model with latent Bernoulli indicators. References George and McCulloch (1993).",
    "when_to_use": "Reach for it when your model has 0/1 indicators to sample, most commonly spike-and-slab Bayesian variable selection with a continuous (Gaussian-spike) prior, rather than a Dirac point-mass spike at \\(\\beta_j = 0\\).",
    "example": "",
    "group": "Discrete-latent Gibbs"
  },
  {
    "name": "categorical_gibbs_block",
    "kind": "sampling",
    "title": "Categorical latent label Gibbs sampler",
    "summary": "Draws per-observation discrete class labels \\(z_i \\in \\{1, \\dots, K\\}\\) by exact Gibbs sampling from their full conditional.",
    "description": "Samples latent categorical indicators with \\(K > 2\\) categories, one label \\(z_i \\in \\{1, \\dots, K\\}\\) per observation, from the closed-form full conditional \\(p(z_i = k \\mid \\text{rest}) \\propto \\exp(\\eta_{ik})\\) using a numerically stable softmax over the supplied conditional log-probabilities. Because discrete parameters cannot be handled by gradient-based samplers, this Gibbs step is the standard way to impute mixture-component memberships, latent-class indicators, regime/state labels, and topic assignments alongside the continuous parameters they depend on (mixture weights, component means and variances, regression coefficients). The update is exact and requires no tuning or warmup, drawing an independent categorical for each observation each sweep. Valid when the labels are conditionally independent across observations given the continuous parameters, as in finite mixtures, latent-class analysis, and zero-inflated models. For exchangeable components, impose an identifiability constraint (for example ordered means or a mass constraint on \\(\\pi\\)) to avoid label switching.",
    "when_to_use": "Reach for it when your model has a discrete per-observation class label \\(z_i \\in \\{1, \\dots, K\\}\\) that is conditionally independent across observations, as in finite mixtures, latent-class analysis, or discrete-choice models, and you want to impute those labels alongside the continuous parameters.",
    "example": "DPGaussianMixture_DerivedAlpha.cpp",
    "group": "Discrete-latent Gibbs"
  },
  {
    "name": "celerite_gp_block",
    "kind": "sampling",
    "title": "Fast 1-D time-series Gaussian Process",
    "summary": "Fits a fast one-dimensional Gaussian-process model for evenly or unevenly sampled time series, evaluating the marginal likelihood and posterior predictions for kernels built from damped and quasi-periodic terms.",
    "description": "Fits a Bayesian one-dimensional Gaussian-process regression for a time series \\(y(t)\\), placing a GP prior on the latent signal with a Gaussian observation likelihood \\(y_i \\sim \\mathcal{N}(f(t_i), \\sigma^2)\\). The covariance is a celerite kernel, a sum of real-exponential and quasi-periodic terms \\(k(\\Delta t) = \\sum_j a_j \\exp(-c_j \\Delta t)\\cos(d_j \\Delta t)\\), which captures smooth trends together with damped oscillations. Because the latent function is integrated out analytically, the block delivers the marginal likelihood \\(p(y \\mid \\text{kernel parameters})\\) for sampling the amplitude, length-scale, period, and noise hyperparameters (for example by slice sampling), plus the closed-form posterior mean and variance at new times. References Foreman-Mackey et al. (2017).",
    "when_to_use": "Reach for it when modeling a single long time series (astronomical, financial, or climate) with smooth trend and oscillatory structure, especially for thousands of observations where a generic Gaussian process is too slow.",
    "example": "GPTimeSeries.cpp",
    "group": "Continuous-conjugate Gibbs"
  },
  {
    "name": "composite_block",
    "kind": "sampling",
    "title": "Gibbs sweep over a multi-parameter model",
    "summary": "Assembles a full Gibbs sampler for a multi-parameter Bayesian model by sweeping over its parameter groups in a fixed order, updating each from its conditional posterior.",
    "description": "Builds a deterministic systematic-scan Gibbs sampler for a joint posterior \\(\\pi(\\theta_1, \\ldots, \\theta_K \\mid y)\\) by cycling once through each parameter group \\(\\theta_k\\) and drawing it from its full conditional \\(\\pi(\\theta_k \\mid \\theta_{-k}, y)\\). Each group can be updated by whatever method suits its conditional, for example a NUTS update for a regression coefficient \\(\\beta\\) and a conjugate or slice update for a variance \\(\\sigma^2\\), and the sweep shares the latest value of every parameter across the model so that downstream conditionals always see the current state. This is the standard composition strategy for hierarchical and multi-block Bayesian models, and groups may themselves be sub-models so that nested hierarchies are handled by the same fixed-scan logic. References Geman and Geman (1984) and Gelfand and Smith (1990).",
    "when_to_use": "Reach for it whenever your model has several parameters or parameter groups that are easiest to update one at a time from their full conditionals, and you want them assembled into a single coherent Gibbs sampler.",
    "example": "BetaBernoulli.cpp",
    "group": "Tools and helpers"
  },
  {
    "name": "dirichlet_gibbs_block",
    "kind": "sampling",
    "title": "Exact Dirichlet Gibbs sampler for simplex weights",
    "summary": "Draws a probability vector \\(\\theta\\) on the simplex exactly from its Dirichlet full conditional in conjugate Dirichlet-Categorical and Dirichlet-Multinomial models.",
    "description": "Samples a length-\\(K\\) probability vector \\(\\theta = (\\theta_1, \\dots, \\theta_K)\\) on the simplex whose full conditional is exactly Dirichlet, \\(\\theta \\mid \\text{rest} \\sim \\mathrm{Dirichlet}(\\alpha^{\\mathrm{post}})\\), as arises whenever a Dirichlet prior \\(\\theta \\sim \\mathrm{Dirichlet}(\\alpha)\\) meets categorical or multinomial counts and the posterior concentration is \\(\\alpha^{\\mathrm{post}}_k = \\alpha_k + n_k\\). It produces independent exact draws by the gamma-normalization method, \\(g_k \\sim \\mathrm{Gamma}(\\alpha^{\\mathrm{post}}_k, 1)\\) and \\(\\theta_k = g_k / \\sum_j g_j\\), with no warmup, tuning, or gradients, making it the fast conjugate choice for mixing weights in finite mixtures, topic proportions and word distributions in latent Dirichlet allocation, and the initial-state and transition rows of a Bayesian hidden Markov model. For non-conjugate targets on the simplex (e.g. a logistic link or a smooth prior on log-ratios), use a simplex-constrained Hamiltonian sampler instead.",
    "when_to_use": "Reach for it whenever a probability vector on the simplex has a clean conjugate Dirichlet full conditional, such as mixture weights, LDA topic and word proportions, or HMM transition and initial-state distributions.",
    "example": "FiniteGaussianMixture.cpp",
    "group": "Continuous-conjugate Gibbs"
  },
  {
    "name": "elliptical_slice_sampling_block",
    "kind": "sampling",
    "title": "Elliptical Slice Sampling for Latent Gaussian Models",
    "summary": "Draws the latent field of a Gaussian-prior model with any non-Gaussian likelihood using Elliptical Slice Sampling.",
    "description": "Samples the latent vector \\(f\\) of a latent Gaussian model, where \\(f \\sim \\mathcal{N}(0, \\Sigma)\\) carries a Gaussian-process or Markov-random-field prior and the observations follow an arbitrary, possibly non-Gaussian likelihood \\(p(y \\mid f)\\). Updates use Elliptical Slice Sampling, which proposes \\(f' = f\\cos\\theta + \\nu\\sin\\theta\\) with \\(\\nu \\sim \\mathcal{N}(0,\\Sigma)\\) and shrinks the slice angle \\(\\theta\\) until acceptance, requiring no gradients and no step-size tuning while coping gracefully with strong prior cross-correlation in \\(\\Sigma\\). Typical targets include Gaussian-process classification with a Bernoulli-logit likelihood, GP regression under Student-t or Poisson noise, and CAR / ICAR / GMRF spatial and temporal smoothers with non-Gaussian observations. References Murray, Adams and MacKay (2010) and Rue and Held (2005).",
    "when_to_use": "Reach for it when your model places a Gaussian prior on a latent field \\(f \\sim \\mathcal{N}(0,\\Sigma)\\) and the likelihood is non-Gaussian (logit, Poisson, Student-t); for purely Gaussian observations, marginalize \\(f\\) and sample only the covariance hyperparameters instead.",
    "example": "GPClassification.cpp",
    "group": "Generic transition kernels"
  },
  {
    "name": "full_rank_gaussian_vi_block",
    "kind": "sampling",
    "title": "Full-Rank Gaussian Variational Inference",
    "summary": "Approximates a block of correlated continuous parameters with a full-covariance Gaussian, fitted by variational inference.",
    "description": "Approximates the posterior over a block of continuous parameters \\(\\eta\\) by a full-covariance Gaussian \\(q(\\eta) = \\mathcal{N}(\\eta; \\mu, \\Sigma)\\) on the unconstrained scale, fitted by automatic differentiation variational inference (ADVI). Unlike a mean-field approximation, it estimates the entire correlation matrix \\(\\Sigma\\) among coordinates, so it recovers honest marginal variances when parameters are strongly dependent (collinear regression coefficients, hierarchical scale-by-coefficient funnels, neural-network output weights). The fit maximizes the evidence lower bound using reparameterization-gradient stochastic optimization, and works directly from your model's log-density and gradient without a hand-coded Jacobian. References Kucukelbir et al. (2017).",
    "when_to_use": "Reach for it when you want a fast Gaussian approximation to a small block (roughly \\(K \\le 50\\)) of tightly correlated continuous parameters whose dependence a mean-field approximation would wash out, underestimating the marginal variance.",
    "example": "",
    "group": "Variational inference"
  },
  {
    "name": "gamma_gibbs_block",
    "kind": "sampling",
    "title": "Conjugate Gamma draw for a positive scalar",
    "summary": "Draws an exact Gibbs update for a positive scalar parameter whose full conditional is a Gamma distribution.",
    "description": "Samples a single positive scalar parameter, such as a precision or a concentration parameter, from its exact Gamma full conditional posterior using a closed-form conjugate Gibbs step, \\(x \\mid \\cdot \\sim \\mathrm{Gamma}(\\text{shape}, \\text{rate})\\). A canonical use is the Dirichlet process concentration \\(\\alpha\\) under a truncated stick-breaking prior \\(\\alpha \\sim \\mathrm{Gamma}(a, b)\\), whose conditional is \\(\\alpha \\mid V_1, \\dots, V_{T-1} \\sim \\mathrm{Gamma}\\big(a + T - 1,\\; b - \\sum_k \\log(1 - V_k)\\big)\\), giving exact independent draws with no warm-up and no autocorrelation. The shape-and-rate parameterization matches R's rgamma, JAGS, NIMBLE, and Stan. Reach for this in place of gradient-based sampling whenever the target precision or concentration has an exact Gamma conjugate posterior (Escobar and West, 1995).",
    "when_to_use": "Use it to update any single positive parameter, such as a Gamma-prior precision or a Dirichlet process concentration, when its conditional posterior is exactly a Gamma distribution.",
    "example": "HDPGaussianMixture.cpp",
    "group": "Continuous-conjugate Gibbs"
  },
  {
    "name": "genbart_block",
    "kind": "sampling",
    "title": "Generalized BART tree-ensemble regression for any likelihood",
    "summary": "Samples a Bayesian additive-tree regression function for non-Gaussian responses such as binary, count, survival, and overdispersed outcomes.",
    "description": "Fits generalized Bayesian Additive Regression Trees (BART), modeling an unknown regression function \\(r(x) = \\sum_{t=1}^{T} g(x; \\mathcal{T}_t)\\) on the linear-predictor scale as a sum of \\(T\\) regression trees, so the response link is whatever the chosen likelihood specifies, for example \\(y_i \\sim \\mathrm{Bernoulli}(\\mathrm{sigmoid}(r(x_i)))\\) for binary data or \\(y_i \\sim \\mathrm{Poisson}(\\exp(r(x_i)))\\) for counts. The forest is updated by reversible-jump MCMC with Laplace-approximated birth, death, and change moves, which accepts non-Gaussian likelihoods directly without data augmentation. Leaf magnitudes follow an adaptive half-Cauchy prior with scale \\(\\propto 1/\\sqrt{T}\\), and an optional Dirichlet sparsity prior on the split-variable probabilities lets the ensemble select relevant covariates. Shipped likelihoods cover Normal, Bernoulli-logistic, Poisson, negative binomial, heteroscedastic Gaussian, accelerated-failure-time survival, Beta, gamma, and beta-binomial responses. References Linero (2022) and, for the sparsity prior, Linero (2018).",
    "when_to_use": "Reach for it when you want a flexible nonparametric regression surface that captures nonlinearities and interactions automatically, but your outcome is not Gaussian, for instance binary, count, overdispersed, or censored survival data.",
    "example": "GBartMultinomial.cpp",
    "group": "Tree-ensemble priors"
  },
  {
    "name": "gmrf_precision_block",
    "kind": "sampling",
    "title": "Sparse Gaussian Markov random field sampler",
    "summary": "Draws a high-dimensional Gaussian latent field defined by a sparse precision matrix directly from its full conditional.",
    "description": "Generates exact draws of a Gaussian Markov random field \\(x \\sim \\mathcal{N}(Q^{-1} b, Q^{-1})\\) whose distribution is specified in canonical form \\(\\pi(x) \\propto \\exp\\{-\\tfrac12 x^\\top Q x + b^\\top x\\}\\), where \\(Q\\) is a large sparse symmetric positive-(semi)definite precision matrix. Each iteration produces an independent conditional draw via the sparse-Cholesky algorithm of Rue (2001), so it replaces gradient-based updates for the spatial or temporal latent field inside a hierarchical Gibbs scheme. It supports intrinsic GMRFs through an optional sum-to-zero constraint and a small ridge regularisation, covering random-walk \\(\\mathrm{RW1}\\)/\\(\\mathrm{RW2}\\) smoothing priors, ICAR and BYM2 spatial effects, and lattice Gaussian-process approximations. References Rue (2001).",
    "when_to_use": "Reach for it when your model has a high-dimensional Gaussian latent field with a sparse precision matrix, such as a spatial, areal, or smoothing-spline random effect, and you want exact conditional draws rather than tuning a generic gradient sampler.",
    "example": "GMRFPrior.cpp",
    "group": "Bayesian graphical models"
  },
  {
    "name": "gmrf_whitened_ess_block",
    "kind": "sampling",
    "title": "Spatial GMRF effects with non-Gaussian observations",
    "summary": "Samples a Gaussian Markov random field latent field when the observed data follow a non-Gaussian likelihood such as Poisson or Bernoulli.",
    "description": "Draws a latent spatial or temporal field \\(x\\) that has a Gaussian Markov random field prior \\(\\pi(x) \\propto \\exp(-\\tfrac12 x^\\top Q x)\\) with a sparse precision matrix \\(Q\\) (for example an intrinsic conditional autoregressive or BYM2 structure, optionally constrained to \\(\\sum_i x_i = 0\\)), when the observations are tied to the field through an arbitrary non-Gaussian likelihood \\(L(y \\mid x)\\) such as \\(y_i \\sim \\mathrm{Poisson}(e^{\\alpha + x_i})\\), Bernoulli, negative binomial, Student-\\(t\\), or a log-Gaussian Cox process. Because the resulting full conditional is no longer Gaussian, the field is updated by Elliptical Slice Sampling, whose acceptance behavior is invariant to how sharply the likelihood concentrates the posterior, giving reliable mixing even for highly informative data. Pair it with samplers for the smooth hyperparameters (intercept, spatial precision) to obtain a full posterior. References Murray, Adams and MacKay (2010) for the slice sampler and Rue (2001) for fast sparse-precision field draws.",
    "when_to_use": "Reach for it when you have a latent Gaussian spatial or temporal field with a sparse precision (CAR/ICAR/BYM2) but your data are counts, binary outcomes, or otherwise non-Gaussian, so a direct conjugate Gaussian update does not apply.",
    "example": "",
    "group": "Bayesian graphical models"
  },
  {
    "name": "hmm_block",
    "kind": "sampling",
    "group": "Bayesian graphical models",
    "title": "Forward-filter backward-sample for hidden Markov models",
    "summary": "Draws the entire latent state sequence \\(z_{1:T}\\) of a hidden Markov model jointly in one exact step.",
    "description": "Samples the full latent state path \\(z_{1:T}\\) of a finite-state hidden Markov model with \\(K\\) states, drawing the whole sequence jointly from its exact conditional with the forward-filter backward-sample (FFBS) algorithm. A forward pass accumulates the filtered state probabilities and a backward pass samples each state given the next, which mixes far better than updating one state at a time. It pairs with conjugate updates for the transition probabilities and the per-state emission parameters. References Fruhwirth-Schnatter (2006).",
    "when_to_use": "Reach for it when your model has a discrete latent state that evolves through time with Markov dependence, such as a regime-switching or segmentation model, and you want to sample the whole state path at once.",
    "example": "HMMGaussian2State.cpp"
  },
  {
    "name": "inv_gamma_gibbs_block",
    "kind": "sampling",
    "title": "Inverse-Gamma Gibbs update for a variance parameter",
    "summary": "Draws an exact conjugate update for a scalar variance or scale parameter whose full conditional posterior is Inverse-Gamma.",
    "description": "Performs an exact, closed-form Gibbs update for a single positive scale or variance parameter whose full conditional posterior is Inverse-Gamma, the conjugate update that arises in Normal-InverseGamma models such as the error variance \\(\\sigma^2\\) in Gaussian regression, \\(\\sigma^2 \\sim \\mathrm{InvGamma}(a, b)\\) with conditional \\(\\sigma^2 \\mid \\cdot \\sim \\mathrm{InvGamma}\\!\\left(a + \\tfrac{N}{2},\\, b + \\tfrac{1}{2}\\sum_i (y_i - x_i^\\top\\beta)^2\\right)\\). Because the posterior is known analytically, each iteration samples a true independent draw rather than a gradient-based approximation, making it faster and more stable than a general-purpose sampler for this parameter. The shape-rate parameterization matches R's \\(\\texttt{rgamma(shape, rate)}\\) and JAGS/NIMBLE/Stan conventions. Note that an Inverse-Gamma prior on a variance is discouraged as a default in favor of weakly-informative scale priors (Gelman 2006); reach for this block when the conjugate prior is genuinely informative and justified.",
    "when_to_use": "Use when a model has a scalar variance or scale parameter with a conjugate Inverse-Gamma prior, so its full conditional is exactly Inverse-Gamma and can be drawn directly instead of through an adaptive sampler.",
    "example": "",
    "group": "Continuous-conjugate Gibbs"
  },
  {
    "name": "ising_cluster_block",
    "kind": "sampling",
    "title": "Swendsen-Wang cluster sampler for Ising and Potts models",
    "summary": "Draws the labels \\(x\\) of a ferromagnetic Ising or Potts model on an arbitrary graph using Swendsen-Wang cluster moves.",
    "description": "Samples the discrete state vector \\(x \\in \\{0,1,\\ldots,Q-1\\}^n\\) of a ferromagnetic Markov random field on a user-supplied undirected graph, with target \\(\\pi(x) \\propto \\exp\\big(\\beta \\sum_{i \\sim j} \\mathbb{I}[x_i = x_j]\\big)\\), where \\(Q=2\\) gives the Ising model and \\(Q \\ge 3\\) the Potts model. Sampling uses the Swendsen-Wang algorithm, which augments the model with edge bonds drawn as \\(u_e \\sim \\text{Bernoulli}(1 - e^{-\\beta})\\) on like-labelled edges, forms connected clusters, and assigns each cluster a fresh uniform label, giving large coordinated updates that mix far faster than single-site Gibbs near the critical coupling. The interaction strength \\(\\beta > 0\\) may be held fixed or sampled jointly with another block in a hierarchical model. References Swendsen and Wang (1987) and Higdon (1998).",
    "when_to_use": "Reach for it when you need to sample a strongly-coupled discrete Ising or Potts field, such as a spatial smoothing prior over image pixels or graph nodes, where per-site Gibbs mixes too slowly.",
    "example": "IsingPrior.cpp",
    "group": "Bayesian graphical models"
  },
  {
    "name": "joint_nuts_block",
    "kind": "sampling",
    "title": "Joint NUTS sampler for coupled continuous parameters",
    "summary": "Samples several tightly-coupled continuous parameters jointly from their posterior using the No-U-Turn Sampler (NUTS), a self-tuning Hamiltonian Monte Carlo method.",
    "description": "Draws two or more continuous parameter blocks together from a single joint posterior \\(\\pi(\\theta_1, \\theta_2, \\ldots \\mid y)\\) using the No-U-Turn Sampler (NUTS), an adaptive form of Hamiltonian Monte Carlo that tunes its step size and trajectory length automatically. By exploring all coupled parameters along one Hamiltonian trajectory, it captures cross-parameter dependence that one-at-a-time updates cannot, which is essential when parameters are correlated through the mean structure, for example the intercept and slopes \\(\\beta\\) in \\(y \\sim \\mathcal{N}(\\alpha + X\\beta, \\sigma^2)\\), the person and item effects in an IRT model \\(y \\sim \\mathrm{Bernoulli}(\\mathrm{logit}^{-1}(\\theta_i - b_j))\\), or fixed and random effects \\(\\beta\\) and \\(u\\) in a hierarchical linear model \\(y \\sim \\mathcal{N}(X\\beta + Zu, \\sigma^2)\\), \\(u \\sim \\mathcal{N}(0, \\tau^2)\\). Each parameter may carry its own constraint (positive, bounded, ordered, simplex, correlation or covariance matrix, and others), and the sampler targets any smooth user-specified log-posterior. References Hoffman and Gelman (2014).",
    "when_to_use": "Reach for it when two or more continuous parameters are strongly correlated in the likelihood, so that updating them separately would mix slowly and you want one joint, well-tuned NUTS update over all of them at once.",
    "example": "BSplineRegression.cpp",
    "group": "Generic transition kernels"
  },
  {
    "name": "lda_collapsed_gibbs_block",
    "kind": "sampling",
    "title": "Latent Dirichlet Allocation by collapsed Gibbs sampling",
    "summary": "Fits Latent Dirichlet Allocation, jointly inferring per-token topic labels, per-document topic proportions, and per-topic word distributions.",
    "description": "Fits a Latent Dirichlet Allocation topic model for a corpus of documents over a fixed vocabulary, assigning each word token a latent topic \\(z_n \\in \\{1,\\dots,K\\}\\), with per-document topic proportions \\(\\theta_d \\sim \\mathrm{Dirichlet}(\\alpha)\\) and per-topic word distributions \\(\\phi_k \\sim \\mathrm{Dirichlet}(\\beta)\\), where the number of topics \\(K\\) and the Dirichlet hyperparameters \\(\\alpha, \\beta\\) are fixed in advance. Sampling uses the collapsed Gibbs scheme of Griffiths and Steyvers (2004), which integrates out \\(\\theta\\) and \\(\\phi\\) analytically and updates each token from its full conditional \\(P(z_n = k \\mid z_{-n}, w) \\propto (n_{d,k}^{-n} + \\alpha_k)\\,(n_{k,w_n}^{-n} + \\beta_{w_n}) / (n_{k,\\cdot}^{-n} + \\sum_v \\beta_v)\\); this marginalization removes the strong coupling among \\((z, \\theta, \\phi)\\) and yields far better mixing than updating them separately. After each sweep over the tokens, \\(\\theta\\) and \\(\\phi\\) are drawn from their Dirichlet conjugate posteriors, so every iteration returns the topic labels together with both sets of probability vectors. References Griffiths and Steyvers (2004).",
    "when_to_use": "Reach for it when you want to discover topics in a document-term corpus and need posterior samples of token topic assignments, document topic mixtures \\(\\theta_d\\), and topic word distributions \\(\\phi_k\\) under a fixed number of topics.",
    "example": "LdaCollapsedGibbs.cpp",
    "group": "Discrete-latent Gibbs"
  },
  {
    "name": "mean_field_categorical_vi_block",
    "kind": "sampling",
    "title": "Mean-Field Variational Inference for Categorical Latents",
    "summary": "Approximates the posterior over discrete categorical latent variables by mean-field variational inference, returning per-variable category probabilities.",
    "description": "Fits a mean-field variational approximation to the joint posterior of a collection of discrete latent variables \\(z_i \\in \\{0, \\dots, K_i - 1\\}\\), each with its own number of categories \\(K_i\\). The approximating distribution factorises as a product of independent categoricals, \\(q(z) = \\prod_i \\mathrm{Categorical}(z_i; \\phi_i)\\) with \\(\\phi_i\\) on the simplex, and the category probabilities are fit by maximising the evidence lower bound (ELBO) against any user-specified joint log-density \\(\\log \\tilde{p}(z)\\), generalising coordinate-ascent variational inference (CAVI) to arbitrary, non-factorising targets such as Ising and Potts models. The fit yields fast, deterministic estimates of the marginal category probabilities, at the known cost of underestimating posterior variance for strongly coupled latents. References Bishop (2006).",
    "when_to_use": "Reach for it when you need quick, deterministic posterior estimates of several coupled discrete labels and per-site Gibbs sampling would mix too slowly, and you can tolerate mean-field variance underestimation.",
    "example": "CategoricalIsingChainVI.cpp",
    "group": "Variational inference"
  },
  {
    "name": "mean_field_gaussian_vi_block",
    "kind": "sampling",
    "title": "Mean-Field Gaussian Variational Inference",
    "summary": "Fast approximate Bayesian inference that fits a fully factorized Gaussian approximation to a continuous posterior by maximizing the evidence lower bound.",
    "description": "Approximates the posterior of a vector of continuous parameters \\(\\theta\\) by automatic differentiation variational inference (ADVI), transforming \\(\\theta\\) to an unconstrained scale \\(\\eta\\) and fitting a mean-field Gaussian family \\(q(\\eta) = \\prod_i \\mathcal{N}(\\eta_i; \\mu_i, \\sigma_i^2)\\). The variational means and standard deviations \\((\\mu_i, \\sigma_i)\\) are learned by maximizing the evidence lower bound \\(\\mathrm{ELBO}(q) = \\mathbb{E}_q[\\log p(\\eta, y)] - \\mathbb{E}_q[\\log q(\\eta)]\\) with a robust adaptive stochastic-gradient scheme (averaged Adam with Polyak-Ruppert iterate averaging), targeting any differentiable log posterior \\(\\log p(\\theta \\mid y)\\) you can specify. It returns the variational mean \\(\\mu\\) as a point estimate and can also draw independent samples \\(\\theta = T(\\mu + \\sigma \\odot \\varepsilon)\\), \\(\\varepsilon \\sim \\mathcal{N}(0, I)\\). Because coordinates are treated as independent, this approximation is well calibrated for the posterior mean but systematically underestimates marginal variance and posterior correlations. References Kucukelbir et al. (2017) and Welandawe et al. (2022).",
    "when_to_use": "Reach for it when you want fast, deterministic approximate inference for a moderate-to-high-dimensional continuous posterior, or when symmetries fragment the posterior into modes that MCMC cannot mix between and a single well-fit mode is acceptable, but prefer NUTS when you need honest credible intervals or faithful posterior correlations.",
    "example": "",
    "group": "Variational inference"
  },
  {
    "name": "niw_cluster_gibbs_block",
    "kind": "sampling",
    "title": "Full-covariance Gaussian cluster parameters",
    "summary": "Draws each cluster's mean and full covariance matrix from a conjugate Normal-Inverse-Wishart posterior in a Gaussian mixture model.",
    "description": "Samples the per-cluster parameters of a multivariate Gaussian mixture, drawing the mean vector and full covariance matrix \\((\\mu_k, \\Sigma_k)\\) jointly for each of \\(K\\) clusters via Gibbs sampling under a conjugate Normal-Inverse-Wishart prior, \\(\\Sigma_k \\sim \\mathcal{IW}(\\Psi_0, \\nu_0)\\) and \\(\\mu_k \\mid \\Sigma_k \\sim \\mathcal{N}(\\mu_0, \\Sigma_k/\\kappa_0)\\). Because it estimates the full \\(\\Sigma_k\\) rather than a diagonal one, it captures within-cluster correlation and recovers the true elliptical shape and tilt of each cluster, not just an axis-aligned spread. Clusters with no assigned observations are refreshed directly from the prior, following the Ishwaran and James (2001) convention for truncated mixtures. References Murphy (2007) for the conjugate updates.",
    "when_to_use": "Reach for it when fitting a Gaussian mixture or clustering model whose clusters have correlated, non-spherical shapes and you want each cluster's full covariance rather than a diagonal approximation.",
    "example": "HDPGaussianMixture.cpp",
    "group": "Continuous-conjugate Gibbs"
  },
  {
    "name": "normal_gamma_cluster_gibbs_block",
    "kind": "sampling",
    "title": "Normal-Gamma cluster parameters for Gaussian mixtures",
    "summary": "Draws the per-cluster mean and precision of every component in a diagonal-Gaussian mixture from its exact Normal-Gamma posterior.",
    "description": "Performs the conjugate Gibbs update for the emission parameters \\((\\mu_k, \\lambda_k)\\) of a truncated diagonal-Gaussian mixture, where each cluster has likelihood \\(y_i \\sim \\mathcal{N}(\\mu_{z_i}, \\mathrm{diag}(1/\\lambda_{z_i}))\\) under a per-dimension Normal-Gamma prior \\(\\lambda_d \\sim \\mathrm{Gamma}(a_0, b_0)\\), \\(\\mu_d \\mid \\lambda_d \\sim \\mathcal{N}(\\mu_{0,d}, 1/(\\kappa_0 \\lambda_d))\\). Given the current cluster assignments, it refreshes all \\(K\\) components in one sweep: populated clusters are sampled from their data-driven posterior and empty clusters directly from the prior, exactly as prescribed for truncated stick-breaking mixtures. Because the update is closed-form it is several times faster and mixes better than gradient-based samplers on the many flat, empty-cluster directions. References Murphy (2007) and Ishwaran and James (2001).",
    "when_to_use": "Reach for it when fitting a Bayesian nonparametric Gaussian mixture (Dirichlet-process or Pitman-Yor stick-breaking) with diagonal covariance and you want exact, fast updates of each component's mean and precision.",
    "example": "DPGaussianMixture.cpp",
    "group": "Continuous-conjugate Gibbs"
  },
  {
    "name": "nuts_block",
    "kind": "sampling",
    "title": "Hamiltonian No-U-Turn sampler for continuous parameters",
    "summary": "Draws a continuous parameter \\(\\theta\\) from its posterior with the No-U-Turn Sampler, a gradient-based Hamiltonian Monte Carlo method.",
    "description": "Samples a real-valued scalar or vector parameter \\(\\theta\\) from an arbitrary continuous posterior \\(\\pi(\\theta) \\propto p(\\text{data} \\mid \\theta)\\,p(\\theta)\\) using the No-U-Turn Sampler (NUTS), a self-tuning form of Hamiltonian Monte Carlo that follows the gradient of the log-posterior to propose efficient, low-autocorrelation moves. Step size is tuned automatically by dual-averaging adaptation, so no hand-tuning is required, and the adaptation state carries forward when this draw is interleaved with Gibbs updates of other parameters. Constrained parameters are handled by sampling on a transformed unconstrained scale, covering strictly positive scales such as \\(\\sigma\\) and \\(\\tau\\), probabilities in \\((0,1)\\), bounded and ordered values, simplex vectors, and correlation matrices. This is the workhorse for non-conjugate continuous parameters where no closed-form full conditional exists. References Hoffman and Gelman (2014).",
    "when_to_use": "Reach for it whenever a continuous parameter has a smooth but non-conjugate posterior with no closed-form update, for example a regression coefficient \\(\\beta\\), a scale \\(\\sigma\\) or \\(\\tau\\) under a Jeffreys prior, or a probability under a non-conjugate likelihood.",
    "example": "ARDLasso.cpp",
    "group": "Generic transition kernels"
  },
  {
    "name": "order_mcmc_block",
    "kind": "sampling",
    "title": "Bayesian network structure learning via order MCMC",
    "summary": "Learns the structure of a Bayesian network from discrete data by sampling variable orderings and the directed acyclic graphs they imply.",
    "description": "Performs Bayesian structure learning for a directed acyclic graph over \\(n\\) discrete variables by sampling from the posterior over total orderings \\(\\prec\\), where \\(p(\\prec \\mid D) \\propto p(\\prec)\\sum_{G \\,\\vdash\\, \\prec} p(D \\mid G)\\,p(G)\\). Because each variable's parents must come from its predecessors in \\(\\prec\\), the sum over graphs factorizes, so the marginal likelihood is computed in closed form using the BDeu score for discrete data, which integrates out the conditional-probability parameters under Dirichlet priors. Orderings are explored by Metropolis-Hastings with a mixture of any-pair and adjacent-position swaps, and at each step a graph is drawn by sampling each variable's parent set from its conditional posterior, yielding Bayesian model averaging over graphs. Supports either a uniform prior on graphs or the fan-in-penalizing prior \\(p(G) \\propto \\prod_j 1/\\binom{p-1}{|\\mathrm{Pa}_j|}\\), which give different posteriors. References Friedman and Koller (2003) and Heckerman, Geiger and Chickering (1995).",
    "when_to_use": "Reach for it when you want to infer the directed acyclic graph relating a set of discrete variables and quantify structural uncertainty, for moderate problems (up to roughly \\(n=64\\) variables), where gradient-based or per-graph samplers cannot navigate the combinatorial space.",
    "example": "OrderMCMCBN.cpp",
    "group": "Bayesian graphical models"
  },
  {
    "name": "pg_logistic_block",
    "kind": "sampling",
    "title": "Bayesian logistic regression coefficient sampler",
    "summary": "Samples the coefficient vector of a Bayesian logistic regression with a Gaussian prior by exact Gibbs sampling.",
    "description": "Fits Bayesian binary logistic regression, \\(y_i \\sim \\mathrm{Bernoulli}(\\sigma(x_i^\\top \\beta))\\) with \\(\\sigma\\) the logistic link, under a Gaussian prior \\(\\beta \\sim \\mathcal{N}(b_0, B_0)\\) on the regression coefficients. Estimation uses Polya-Gamma data augmentation, which introduces latent variables that render the conditional distribution of \\(\\beta\\) exactly Gaussian, so the sampler draws \\(\\beta\\) and the latent variables in closed-form Gibbs steps with no tuning. This exact-Gibbs scheme typically mixes far faster than gradient-based samplers for logistic models with a moderate number of predictors. References Polson, Scott and Windle (2013).",
    "when_to_use": "Reach for it when you want posterior draws of the coefficients in a Bayesian logistic regression with a fixed design matrix and a Gaussian prior, especially when you prefer a tuning-free, fast-mixing Gibbs sampler over gradient-based methods.",
    "example": "LogisticRegression.cpp",
    "group": "Data-augmentation Gibbs"
  },
  {
    "name": "poisson_multinomial_aug_block",
    "kind": "sampling",
    "group": "Data-augmentation Gibbs",
    "title": "Poisson augmentation for multinomial models",
    "summary": "Recasts a multinomial model as independent Poisson counts so category log-rates can be sampled with simple conjugate or Gaussian updates.",
    "description": "Samples the parameters of a multinomial or categorical-count model with the Poisson trick, which represents the multinomial counts as independent Poisson variables conditioned on their total. This turns a coupled multinomial likelihood into separate pieces for each category log-rate \\(\\log \\phi_k\\), so each can be updated with a conjugate Gamma or a Gaussian step instead of one coupled move. It is convenient for multinomial-logistic and categorical tree-ensemble models where the softmax over categories would otherwise be awkward to sample.",
    "when_to_use": "Reach for it when fitting a multinomial or categorical-outcome model and you want to decouple the categories into independent Poisson pieces for easier conjugate updates.",
    "example": "GBartMultinomial.cpp"
  },
  {
    "name": "probit_aug_block",
    "kind": "sampling",
    "title": "Albert-Chib latent variable for probit models",
    "summary": "Draws the truncated-normal latent variable that turns a binary probit likelihood into a Gaussian one, so any continuous-response model can fit binary data.",
    "description": "Implements the Albert-Chib data-augmentation Gibbs step for Bayesian probit models, where a binary outcome follows \\(y_i \\sim \\mathrm{Bernoulli}(\\Phi(\\mu_i))\\) with \\(\\Phi\\) the standard normal CDF and \\(\\mu_i\\) a linear predictor, latent function, or other mean structure. At each iteration it samples the latent score \\(z_i \\mid y_i, \\mu_i \\sim \\mathcal{N}(\\mu_i, 1)\\) truncated to the positive half-line when \\(y_i = 1\\) and the negative half-line when \\(y_i = 0\\), drawn exactly by Robert's (1995) accept-reject scheme. Because the latent \\(z_i\\) acts as a Gaussian working response, pairing this step with any Normal-likelihood mean model (linear regression, BART, Gaussian processes, hierarchical structures, or shrinkage priors such as the horseshoe) recovers the full Bayesian probit sampler. The variance is fixed at \\(1\\) by probit identifiability. References Albert and Chib (1993).",
    "when_to_use": "Reach for it when your outcome is binary and you want a probit-link model whose mean structure is fit by a Gaussian-response sampler, for example probit regression, probit BART, or hierarchical and shrinkage probit GLMs.",
    "example": "ProbitRegression.cpp",
    "group": "Data-augmentation Gibbs"
  },
  {
    "name": "rjmcmc_block",
    "kind": "sampling",
    "title": "Reversible-Jump MCMC for Spike-and-Slab Variable Selection",
    "summary": "Samples a Dirac spike-and-slab model with reversible-jump MCMC, jointly inferring which coefficients are nonzero and their values.",
    "description": "Performs Bayesian variable selection under the Dirac spike-and-slab prior, where each coefficient is either exactly zero or drawn from a continuous slab, \\(\\gamma_j \\sim \\mathrm{Bernoulli}(\\pi)\\), \\(\\beta_j \\mid \\gamma_j = 0 = 0\\), and \\(\\beta_j \\mid \\gamma_j = 1 \\sim \\mathcal{N}(0, \\tau^2)\\). Because turning a coefficient on or off changes the dimension of the model, the sampler uses reversible-jump MCMC with birth and death proposals to move correctly across these mixed atomic-and-continuous spaces, while active coefficients are refreshed by a closed-form Gibbs update of \\(\\beta_j \\mid \\gamma_j = 1\\). The result is a joint posterior over the inclusion indicators \\(\\gamma\\) and the coefficient values \\(\\beta\\), giving per-variable inclusion probabilities and model-averaged effect estimates in one run. References Green (1995) for the reversible-jump construction and Ishwaran and Rao (2005) for the variance-scaled slab.",
    "when_to_use": "Reach for it when you want Bayesian variable selection that decides which predictors enter the model rather than merely shrinking them, for example sparse regression with a true point mass at zero, change-point insertion, or birth and death of a finite unknown number of mixture components.",
    "example": "SpikeSlabRJMCMC.cpp",
    "group": "Trans-dimensional MH"
  },
  {
    "name": "softbart_block",
    "kind": "sampling",
    "title": "Soft BART smooth tree-ensemble regression",
    "summary": "Fits a flexible nonparametric regression \\(y = f(x) + \\epsilon\\) using a Soft BART ensemble of smoothed regression trees.",
    "description": "Fits Bayesian nonparametric regression \\(y_i \\sim \\mathcal{N}(f(x_i), \\sigma^2)\\), where the unknown mean surface \\(f\\) is given a Soft BART prior, a sum-of-trees ensemble whose splits are smooth logistic activations with a learned bandwidth \\(\\tau\\) rather than hard cutpoints, yielding differentiable predictions that adapt well to smooth response surfaces. Inference proceeds by Bayesian backfitting MCMC, updating one tree at a time against the partial residuals along with the ensemble's leaf parameters. An optional Dirichlet sparsity prior on the splitting probabilities (DART) lets the model select relevant predictors when many inputs are irrelevant. References Linero and Yang (2018).",
    "when_to_use": "Reach for this when you want a flexible, automatic regression of a continuous outcome on many predictors and you expect the underlying mean surface to be smooth, or when you want built-in variable selection through the sparsity prior.",
    "example": "SoftBartNoise.cpp",
    "group": "Tree-ensemble priors"
  },
  {
    "name": "split_merge_block",
    "kind": "sampling",
    "title": "Split-merge moves for mixture cluster labels",
    "summary": "A Metropolis-Hastings update that splits one cluster into two or merges two clusters into one in a single move, to speed up mixing of the cluster-membership labels in a Bayesian mixture model.",
    "description": "Accelerates partition sampling in finite or truncated Bayesian mixture models, where each observation carries a latent cluster label \\(z_i \\in \\{1, \\dots, K\\}\\) with prior \\(z_i \\sim \\mathrm{Categorical}(\\pi)\\) and Gaussian component likelihood \\(y_i \\mid z_i \\sim \\mathcal{N}(\\mu_{z_i}, \\Sigma_{z_i})\\). Ordinary one-observation-at-a-time Gibbs updates of \\(z\\) move slowly because relabelling a whole cluster requires many low-probability single flips, so the chain gets stuck in local modes. This block adds a Jain-Neal split-merge proposal that reassigns an entire cluster's worth of points at once, splitting a cluster into two or merging two into one, with the proposal built from a short restricted Gibbs sweep over the affected observations and accepted or rejected by the exact Metropolis-Hastings ratio. It is meant to run alongside the usual per-observation Gibbs step, holding the mixing weights \\(\\pi\\) and component parameters fixed during each split-merge move, and typically improves mixing on hard clustering problems by an order of magnitude. References Jain and Neal (2004).",
    "when_to_use": "Reach for it when fitting a Gaussian mixture or Dirichlet-process / truncated stick-breaking mixture and the per-observation Gibbs sampler mixes poorly over the number and composition of clusters.",
    "example": "",
    "group": "Trans-dimensional MH"
  },
  {
    "name": "stick_breaking_block",
    "kind": "sampling",
    "title": "Stick-breaking mixture weights (DP, Pitman-Yor)",
    "summary": "Draws the truncated stick-breaking simplex of mixture weights \\(\\pi\\) used by Dirichlet-process and Pitman-Yor nonparametric mixtures.",
    "description": "Samples the mixing weights \\(\\pi = (\\pi_1, \\ldots, \\pi_{K})\\) of a Bayesian nonparametric mixture under the Ishwaran and James (2001) truncated stick-breaking representation, where independent Beta sticks \\(V_k \\sim \\mathrm{Beta}(a_k, b_k)\\) are combined as \\(\\pi_k = V_k \\prod_{j<k}(1 - V_j)\\) to form weights that sum to one. The full-conditional given the current cluster counts is exactly a product of Beta densities, so the weights are drawn in closed form by Gibbs sampling with no tuning. The same machinery covers the Dirichlet process (Sethuraman 1994), with \\(a_k = 1 + n_k\\) and \\(b_k = \\alpha + \\sum_{j>k} n_j\\), and the Pitman-Yor process (Pitman and Yor 1997) by adjusting the stick parameters for the discount. References Ishwaran and James (2001).",
    "when_to_use": "Reach for it when you fit a Dirichlet-process, Pitman-Yor, or hierarchical-Dirichlet mixture and need to update the cluster weights \\(\\pi\\), letting the data choose how many components are active up to a chosen truncation level.",
    "example": "DPGaussianMixture.cpp",
    "group": "Discrete-latent Gibbs"
  },
  {
    "name": "structured_categorical_vi_block",
    "kind": "sampling",
    "title": "Structured mean-field VI for discrete latents",
    "summary": "Fits a clique-structured variational approximation to discrete latent variables, capturing dependence within user-chosen groups while factorising across them.",
    "description": "Approximates the posterior over discrete latent variables \\(z_1, \\dots, z_n\\) (each taking one of \\(K_i\\) categories) using structured mean-field variational inference. Given a partition of the variables into cliques \\(\\{C_1, \\dots, C_M\\}\\), the variational family is \\(q(z) = \\prod_C q_C(z_C)\\), where each \\(q_C\\) is a joint Categorical over its clique's combined state space, so correlation within a clique is modelled exactly while only inter-clique coupling is factorised away. The block maximises the evidence lower bound by analytic gradient ascent through an anchored softmax parameterisation, marginalising over the remaining cliques either by exact enumeration or Monte Carlo. This is the right tool when the discrete posterior has strong local dependence but weak long-range coupling, such as coupled hidden Markov models or Potts and Markov random fields. References Saul and Jordan (1996).",
    "when_to_use": "Reach for it when you have discrete latent variables with strong dependence inside known local groups but weak coupling between groups, and a fully factorised mean field is too crude.",
    "example": "StructuredPottsVI.cpp",
    "group": "Variational inference"
  },
  {
    "name": "univariate_slice_sampling_block",
    "kind": "sampling",
    "title": "Univariate Slice Sampler for Scalar Parameters",
    "summary": "Draws a single continuous scalar parameter from its posterior using Neal's univariate slice sampling.",
    "description": "Samples one continuous scalar parameter \\(\\theta\\) from a target posterior \\(\\pi(\\theta) \\propto L(\\theta)\\,p(\\theta)\\) using the univariate slice sampling algorithm with stepping-out and shrinkage. The method introduces an auxiliary height \\(y \\sim \\mathcal{U}(0, \\log \\pi(\\theta))\\) and resamples \\(\\theta\\) uniformly within the slice \\(\\{\\theta : \\log \\pi(\\theta) > y\\}\\), so it needs only pointwise evaluations of the log-posterior and no gradient. This makes it suited to targets where the log-density is non-differentiable or is a black-box marginal likelihood whose derivative is impractical to obtain. References Neal (2003).",
    "when_to_use": "Reach for it to update a single continuous scalar parameter (for example a positive variance or a Gaussian-process hyperparameter such as amplitude, length-scale, or noise) when its log-posterior has no usable gradient and a gradient-based sampler is therefore unavailable.",
    "example": "GPTimeSeries.cpp",
    "group": "Generic transition kernels"
  },
  {
    "name": "vi_block",
    "kind": "sampling",
    "group": "Tools and helpers",
    "title": "Base interface for variational-inference blocks",
    "summary": "The shared interface the variational blocks build on, fitting a parametric approximation \\(q_\\lambda(\\theta)\\) to a posterior by optimization rather than sampling.",
    "description": "Defines the common variational-inference interface used by the mean-field and full-rank blocks. Instead of drawing samples, a variational block fits a parametric family \\(q_\\lambda(\\theta)\\) to the posterior by maximizing the evidence lower bound (ELBO), trading exactness for speed on large problems. The concrete mean-field Gaussian, full-rank Gaussian, and categorical blocks specialize it.",
    "when_to_use": "",
    "example": ""
  },
  {
    "name": "autodiff_wrap",
    "kind": "utility",
    "title": "Automatic Gradients for Log-Density Functions",
    "summary": "It lets a model be described by its log-density alone, then automatically supplies the matching gradients and parameter-range bookkeeping that gradient-based samplers need.",
    "description": "This helper takes a plain formula for how likely each set of parameter values is and works out, on its own, how that likelihood changes as the parameters move (its slope, or gradient \\( \\nabla \\log p \\)). It also handles the math of keeping parameters inside their valid ranges, so the model author never has to write any gradient or change-of-variable formulas by hand.",
    "when_to_use": "",
    "example": "",
    "group": "Tools and helpers"
  },
  {
    "name": "backend_neutral",
    "kind": "utility",
    "title": "Backend-Neutral Helpers for R and Python",
    "summary": "Lets a sampler's shared code report errors and compute common math functions the same way whether it runs from R or from Python.",
    "description": "This helper gives the library a single set of routines, for raising errors and for a few standard math functions like \\( \\log \\Gamma(x) \\) and the digamma function, that behave correctly no matter whether the sampler is being used inside R or inside Python. That way the same shared code works in both environments without being rewritten for each one.",
    "when_to_use": "",
    "example": "",
    "group": "Tools and helpers"
  },
  {
    "name": "bde_scorer",
    "kind": "utility",
    "title": "Bayesian Network Structure Scorer",
    "summary": "It rates how well a proposed network of cause-and-effect links explains an observed dataset.",
    "description": "This helper takes a dataset of categorical observations and a candidate set of \"parent\" relationships for a variable, then returns a single number measuring how well that arrangement fits the data while gently favoring simpler structures. The library uses these scores to search for the most plausible web of dependencies among variables.",
    "when_to_use": "",
    "example": "",
    "group": "Tools and helpers"
  },
  {
    "name": "block_sampler",
    "kind": "utility",
    "title": "Block Sampler: the shared building-block contract",
    "summary": "It defines the common rulebook that every parameter-updating building block in the library must follow so they can be snapped together into one larger sampler.",
    "description": "This helper sets out the shared interface that each self-contained piece of a model agrees to, covering how a piece receives the inputs it needs, takes one update step, reports its current value, and can be started from a chosen value. Because every block speaks this same language and keeps to itself, the library can freely combine many pieces into a single coordinated sampler and optionally record the full history of values they produce.",
    "when_to_use": "",
    "example": "",
    "group": "Tools and helpers"
  },
  {
    "name": "bnp_utils",
    "kind": "utility",
    "title": "Building blocks for clustering models",
    "summary": "A small toolkit of shared math helpers that let the library's clustering models decide how data points group together.",
    "description": "It provides the common bookkeeping for \"how many groups and who belongs where\" calculations, such as counting members per group and weighing the chance that a point joins an existing group versus starts a brand-new one. These pieces are reused by the library's mixture and clustering samplers so each one does not have to re-derive the same formulas.",
    "when_to_use": "",
    "example": "",
    "group": "Tools and helpers"
  },
  {
    "name": "celerite_marginal_likelihood",
    "kind": "utility",
    "title": "Fast likelihood score for time-series GP models",
    "summary": "A quick scoring helper that tells you how well a set of time-series model settings fits your data, so other parts of the sampler can compare alternative settings.",
    "description": "It takes a single time series and a candidate set of model settings and returns one number, \\( \\log p(y \\mid \\text{settings}) \\), measuring how well those settings explain the data. Because it computes this from scratch each time without disturbing anything else, other components can safely use it to test proposed settings, run model-comparison sweeps, or sanity-check results.",
    "when_to_use": "",
    "example": "",
    "group": "Tools and helpers"
  },
  {
    "name": "constraints",
    "kind": "utility",
    "title": "Constraints: safe parameter range transforms",
    "summary": "It lets the library safely handle parameters that must stay in a limited range, such as a value that can only be positive.",
    "description": "Many model parameters are only allowed certain values, like a standard deviation that must stay above zero or proportions that must add up to one. This helper handles the fiddly, error-prone bookkeeping of converting between those restricted values and the free numbers the sampler works with, so the rest of the model can be written simply and correctly.",
    "when_to_use": "",
    "example": "",
    "group": "Tools and helpers"
  },
  {
    "name": "ode_rk45",
    "kind": "utility",
    "title": "Adaptive ODE solver for mechanistic models",
    "summary": "A built-in solver that traces how a system changes over time, letting models of real-world dynamics (like drug levels or disease spread) plug into the library.",
    "description": "This helper follows a set of change-over-time equations forward in time and reports the system's state at the moments you ask for, automatically adjusting how big a step it takes to stay accurate. It lets the library handle mechanistic models such as pharmacokinetics, epidemics, and ecology, where the quantity of interest \\( y(t) \\) evolves according to a known rule.",
    "when_to_use": "",
    "example": "",
    "group": "Tools and helpers"
  },
  {
    "name": "pybind_casters",
    "kind": "utility",
    "title": "Translates results between C++ and Python",
    "summary": "It quietly converts the library's vectors, matrices, and result tables into the array and dictionary formats Python expects, and back again.",
    "description": "This helper is the translator that sits at the boundary between the fast C++ engine and Python, so numbers, tables, and summaries pass across cleanly in each direction. It also handles a layout mismatch behind the scenes (the two sides store table rows and columns in opposite order), so the values always come out arranged the way a Python user would expect.",
    "when_to_use": "",
    "example": "",
    "group": "Tools and helpers"
  },
  {
    "name": "rcpp_wrap",
    "kind": "utility",
    "title": "Hand sampler results back to R",
    "summary": "It teaches the library how to translate its internal result bundles into ordinary R objects so they show up cleanly in your R session.",
    "description": "When a sampler finishes, this helper converts its packaged outputs (current parameter values, full run histories, and a few internal bookkeeping summaries) into familiar R named lists of numbers and tables. It only switches on when the code is built for R, so the same library still works untouched in a Python build.",
    "when_to_use": "",
    "example": "",
    "group": "Tools and helpers"
  },
  {
    "name": "rjmcmc_custom_bijection",
    "kind": "utility",
    "title": "Custom Bijection with Auto-Computed Jacobian",
    "summary": "It lets users plug a custom nonlinear coordinate transform into reversible-jump moves without ever hand-writing the correction term it requires.",
    "description": "When a reversible-jump sampler adds or removes a parameter, the move needs a stretch-and-squeeze correction factor (the Jacobian \\( |d\\beta/du| \\)) to stay valid. This helper lets the user supply only their own forward map and its inverse, and computes that correction automatically so it is always correct.",
    "when_to_use": "",
    "example": "",
    "group": "Tools and helpers"
  },
  {
    "name": "rjmcmc_transforms",
    "kind": "utility",
    "title": "Ready-made coordinate maps for RJMCMC moves",
    "summary": "A small menu of reusable coordinate transforms that automatically handle the bookkeeping needed when a reversible-jump sampler moves between parameter spaces.",
    "description": "This helper offers a handful of standard ways to reshape coordinates (leave them unchanged, stretch them, or stretch-and-shift them) for use in reversible-jump moves. Each one quietly computes the volume-change factor \\( |\\det J| \\) it implies, so users never have to work out that correction by hand.",
    "when_to_use": "",
    "example": "",
    "group": "Tools and helpers"
  },
  {
    "name": "score_cache",
    "kind": "utility",
    "title": "Score Cache for Network Structure Search",
    "summary": "It precomputes and stores the quality scores of candidate connections in a network so the sampler can look them up instantly instead of recalculating them.",
    "description": "When learning which variables influence which others, the library needs to repeatedly judge how well different sets of \"parents\" explain each variable. This helper does that scoring once up front, keeps only the most promising options, and serves the stored answers on demand so the rest of the search runs far faster.",
    "when_to_use": "",
    "example": "",
    "group": "Tools and helpers"
  },
  {
    "name": "shared_data",
    "kind": "utility",
    "title": "Shared Data: the model's central memory",
    "summary": "It is the shared notebook that holds every value in a model and keeps track of how the pieces depend on one another.",
    "description": "Shared Data is one central place that stores all of a model's values, the fixed inputs, the quantities being estimated, and the figures computed from them, under simple names. It also records how each piece depends on the others, so that when one value updates the related quantities are refreshed correctly and the whole model stays consistent.",
    "when_to_use": "",
    "example": "",
    "group": "Tools and helpers"
  },
  {
    "name": "types",
    "kind": "utility",
    "title": "Shared result types for sampler outputs",
    "summary": "It defines a small set of neutral, named containers that every sampler uses to hand its results back to R and Python.",
    "description": "This helper provides standard labeled bundles for sampler output, one holding named parameter values and another holding named tables of draws (rows are samples, columns are quantities). Because results always come back in these shared shapes, the same sampler can deliver its answers cleanly to both R and Python without any special translation.",
    "when_to_use": "",
    "example": "",
    "group": "Tools and helpers"
  },
  {
    "name": "vi_optimizer",
    "kind": "utility",
    "title": "Engine That Tunes Variational Approximations",
    "summary": "It supplies the shared optimization machinery that variational inference blocks use to fit and check a fast approximate posterior.",
    "description": "This helper gives the library's variational inference blocks a common toolkit for steadily improving an approximate answer, smoothing out the noise in each update, and judging how close the approximation has gotten. It also includes a quality score \\( \\hat{k} \\) that flags when the approximation can no longer be trusted.",
    "when_to_use": "",
    "example": "",
    "group": "Tools and helpers"
  }
];
