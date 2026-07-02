library(AI4BayesCode)
ai4bayescode_install_block("nngp_gaussian_gibbs_block")   # download + install from the hub
ai4bayescode_example("SpatialNNGPRegression")             # compile + load the bundled example class

# small synthetic spatial dataset on [0,1]^2 (matches the C++ data-gen, simplified)
set.seed(1)
n <- 200L; p <- 2L
beta_true <- c(1.0, 2.0); tau2 <- 0.2; sigma2 <- 1.0; phi <- 6.0
coords <- matrix(runif(n * 2), n, 2)          # n x 2 site coordinates
X <- cbind(1, rnorm(n))                        # n x p design (intercept + 1 covariate)
D <- as.matrix(dist(coords))
Cov <- sigma2 * exp(-phi * D); diag(Cov) <- diag(Cov) + 1e-8
w <- as.numeric(t(chol(Cov)) %*% rnorm(n))     # true spatial field
y <- as.numeric(X %*% beta_true + w + sqrt(tau2) * rnorm(n))
# constructor wants column-major flat X and coords
Xf <- as.numeric(X); coordsf <- as.numeric(coords)

# run several chains and check convergence (the usual workflow)
run <- AI4BayesCode_run_chains(
  function(s) new(SpatialNNGPRegression, y, Xf, coordsf,
                  n = n, p = p, coord_dim = 2L, m = 10L,
                  phi_lower = 0.5, phi_upper = 30.0,
                  a_tau = 2.0, b_tau = 1.0, a_sigma = 2.0, b_sigma = 1.0,
                  seed = s, keep_history = TRUE), n_chains = 4, n_burn = 5000, n_keep = 5000)
AI4BayesCode_rhat_summary(run)            # cross-chain R-hat and ESS
ai4b_diagnose(run$histories[[1]])         # trace, ACF, density for one chain

# optional: stateful usage, drive a single chain yourself
m <- new(SpatialNNGPRegression, y, Xf, coordsf,
         n = n, p = p, coord_dim = 2L, m = 10L,
         phi_lower = 0.5, phi_upper = 30.0,
         a_tau = 2.0, b_tau = 1.0, a_sigma = 2.0, b_sigma = 1.0,
         seed = 1L, keep_history = TRUE)
m$step(5000L)
hist <- m$get_history()
