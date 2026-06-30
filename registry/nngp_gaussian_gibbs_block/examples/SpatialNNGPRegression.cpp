// Copyright (C) 2026 AI4BayesCode contributors.
// Licensed under the GNU General Public License v3.0 or later
// (GPL-3.0-or-later). See COPYING / LICENSE at the repo root.
//
// SpatialNNGPRegression.cpp -- frontend-independent (plain C++, NO Rcpp/pybind)
// worked demo for nngp_gaussian_gibbs_block.
//
// MODEL
//   y_i | beta, w_i, tau^2 ~ Normal(x_i' beta + w_i, tau^2)
//   w                      ~ NNGP(exponential covariance sigma^2 exp(-phi d))
//   beta ~ Normal(0, sigma_beta^2 I),  tau^2, sigma^2 ~ Inverse-Gamma,  phi ~ Uniform
//
// BLOCK DECOMPOSITION
//   composite_block{ nngp_gaussian_gibbs_block "nngp" }  -- ONE self-contained block
//   samples (beta, tau^2, sigma^2, phi, w) jointly. The composite feeds it the data
//   inputs (y, X, coords) as declared dependencies each sweep.
//
// WHY THIS BLOCK (SelectWhen): large-scale spatial regression with Gaussian-process
//   random effects on point-referenced data -- the NNGP replaces the dense n x n GP
//   covariance with a sparse nearest-neighbor approximation.
//
// WHAT THE DEMO SHOWS (tuned to DISCRIMINATE)
//   1. Parameter recovery: posterior means of (beta, tau^2, sigma^2, phi) vs truth.
//   2. Posterior-predictive check (MANDATORY): Bayesian p-value for the residual SD
//      statistic, which must be well-calibrated (not extreme).
//   3. Held-out KRIGING: predict y at unobserved sites by NNGP kriging and beat a
//      non-spatial OLS baseline (which ignores w) on test RMSE -- the spatial field
//      is the value this block adds, so the baseline must visibly lose.
//
// DEFAULTS (override in main): n_train=200, n_test=60, p=2, m=10; truth
//   beta=(1.0, 2.0), tau2=0.2, sigma2=1.0, phi=6.0 (effective range ~0.5 on [0,1]^2);
//   warmup=600, samples=1500; seed=20260624.

#include "nngp_gaussian_gibbs_block.hpp"
#include "AI4BayesCode/composite_block.hpp"

#include <armadillo>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <random>
#include <vector>

using AI4BayesCode::nngp_gaussian_gibbs_block;
using AI4BayesCode::composite_block;

namespace {

double euclid(const arma::mat& C, arma::uword i, arma::uword j) {
    return arma::norm(C.row(i) - C.row(j), 2);
}

// m nearest TRAINING sites (rows of Ctr) to a query point q, by brute force.
arma::uvec nearest_train(const arma::rowvec& q, const arma::mat& Ctr, std::size_t m) {
    const arma::uword ntr = Ctr.n_rows;
    std::vector<std::pair<double, arma::uword>> d(ntr);
    for (arma::uword j = 0; j < ntr; ++j) d[j] = {arma::norm(Ctr.row(j) - q, 2), j};
    const std::size_t mm = std::min<std::size_t>(m, ntr);
    std::partial_sort(d.begin(), d.begin() + mm, d.end());
    arma::uvec nb(mm);
    for (std::size_t j = 0; j < mm; ++j) nb[j] = d[j].second;
    return nb;
}

} // namespace

int main() {
    // ----------------------------------------------------------------- setup
    const arma::uword n_train = 200, n_test = 60, p = 2, m = 10;
    const arma::uword n = n_train + n_test;
    const arma::vec beta_true = {1.0, 2.0};
    const double tau2_true = 0.2, sigma2_true = 1.0, phi_true = 6.0;
    const std::size_t warmup = 600, samples = 1500;

    std::mt19937_64 rng(20260624ULL);
    std::normal_distribution<double> Nrm(0.0, 1.0);
    std::uniform_real_distribution<double> U(0.0, 1.0);

    // ------------------------------------------------- simulate from a TRUE GP
    arma::mat C(n, 2), X(n, p);
    for (arma::uword i = 0; i < n; ++i) { C(i,0)=U(rng); C(i,1)=U(rng);
        X(i,0)=1.0; for (arma::uword k=1;k<p;++k) X(i,k)=Nrm(rng); }
    arma::mat Cov(n, n);                                  // dense exponential GP cov
    for (arma::uword i = 0; i < n; ++i)
        for (arma::uword j = 0; j < n; ++j)
            Cov(i,j) = sigma2_true * std::exp(-phi_true * euclid(C, i, j));
    Cov.diag() += 1e-8;                                   // jitter for chol
    arma::mat L = arma::chol(Cov, "lower");
    arma::vec z(n); for (arma::uword i=0;i<n;++i) z[i]=Nrm(rng);
    arma::vec w_true = L * z;
    arma::vec y(n);
    for (arma::uword i = 0; i < n; ++i)
        y[i] = arma::dot(X.row(i), beta_true) + w_true[i] + std::sqrt(tau2_true) * Nrm(rng);

    // train / test split
    arma::mat  Ctr = C.rows(0, n_train-1),     Cte = C.rows(n_train, n-1);
    arma::mat  Xtr = X.rows(0, n_train-1),     Xte = X.rows(n_train, n-1);
    arma::vec  ytr = y.subvec(0, n_train-1),   yte = y.subvec(n_train, n-1);

    // ------------------------------------------------------- build + fit block
    nngp_gaussian_gibbs_block::config cfg;
    cfg.name = "nngp"; cfg.n = n_train; cfg.p = p; cfg.coord_dim = 2; cfg.m = m;
    cfg.phi_lower = 0.5; cfg.phi_upper = 30.0;            // explicit (skip auto-derive)
    cfg.a_tau = 2.0; cfg.b_tau = 1.0; cfg.a_sigma = 2.0; cfg.b_sigma = 1.0;

    composite_block comp("SpatialNNGPRegression");
    comp.data().set("y", ytr);
    comp.data().set("X", arma::vectorise(Xtr));          // column-major flat
    comp.data().set("coords", arma::vectorise(Ctr));
    comp.add_child(std::make_unique<nngp_gaussian_gibbs_block>(cfg));
    comp.data().declare_dependencies("nngp", {"y", "X", "coords"});

    for (std::size_t s = 0; s < warmup; ++s) comp.step(rng);
    comp.set_keep_history(true);
    for (std::size_t s = 0; s < samples; ++s) comp.step(rng);

    auto H = comp.get_history();
    const arma::mat& beta_h = H.at("nngp_beta");          // samples x p
    const arma::mat& tau_h  = H.at("nngp_tau2");
    const arma::mat& sig_h  = H.at("nngp_sigma2");
    const arma::mat& phi_h  = H.at("nngp_phi");
    const arma::mat& w_h    = H.at("nngp_w");             // samples x n_train

    arma::rowvec beta_pm = arma::mean(beta_h, 0);
    const double tau2_pm = arma::mean(tau_h.col(0));
    const double sig_pm  = arma::mean(sig_h.col(0));
    const double phi_pm  = arma::mean(phi_h.col(0));
    arma::rowvec w_pm    = arma::mean(w_h, 0);            // 1 x n_train

    // ----------------------------------------------------- (1) recovery report
    std::printf("=== SpatialNNGPRegression demo ===\n");
    std::printf("recovery (posterior mean vs truth):\n");
    std::printf("  beta0   %.3f  (truth %.3f)\n", beta_pm[0], beta_true[0]);
    std::printf("  beta1   %.3f  (truth %.3f)\n", beta_pm[1], beta_true[1]);
    std::printf("  tau2    %.3f  (truth %.3f)\n", tau2_pm, tau2_true);
    std::printf("  sigma2  %.3f  (truth %.3f)\n", sig_pm,  sigma2_true);
    std::printf("  phi     %.3f  (truth %.3f)\n", phi_pm,  phi_true);
    const bool ok_beta = std::abs(beta_pm[0]-beta_true[0]) < 0.5 &&
                         std::abs(beta_pm[1]-beta_true[1]) < 0.5;

    // ----------------------------------------- (2) posterior-predictive check
    // Bayesian p-value for T = sd(y): p = P(sd(y_rep) >= sd(y_obs)); ~0.5 if calibrated.
    const double T_obs = arma::stddev(ytr);
    std::size_t ge = 0;
    for (arma::uword s = 0; s < beta_h.n_rows; ++s) {
        arma::vec yrep(n_train);
        const double sd_s = std::sqrt(tau_h(s,0));
        for (arma::uword i = 0; i < n_train; ++i) {
            double mu = w_h(s,i);
            for (arma::uword k = 0; k < p; ++k) mu += Xtr(i,k) * beta_h(s,k);
            yrep[i] = mu + sd_s * Nrm(rng);
        }
        if (arma::stddev(yrep) >= T_obs) ++ge;
    }
    const double ppc_p = static_cast<double>(ge) / static_cast<double>(beta_h.n_rows);
    const bool ok_ppc = ppc_p > 0.05 && ppc_p < 0.95;
    std::printf("posterior-predictive check: Bayesian p-value(sd) = %.3f (calibrated 0.05..0.95)\n", ppc_p);

    // -------------------------------------------- (3) held-out NNGP kriging
    // Plug-in kriging mean at each test site: yhat = x0'beta_pm + b0' w_pm[N(s0)],
    //   b0 = C_N^{-1} c0 from the exponential correlation at phi_pm over the m nearest
    //   TRAINING sites. Compare to a non-spatial OLS baseline (ignores w).
    arma::vec yhat_nngp(n_test), yhat_ols(n_test);
    arma::vec beta_ols = arma::solve(Xtr, ytr);           // OLS on training
    for (arma::uword t = 0; t < n_test; ++t) {
        const arma::rowvec q = Cte.row(t);
        arma::uvec nb = nearest_train(q, Ctr, m);
        const arma::uword mm = nb.n_elem;
        arma::mat CN(mm, mm); arma::vec c0(mm);
        for (arma::uword a = 0; a < mm; ++a) {
            CN(a,a) = 1.0 + 1e-8;
            c0[a] = std::exp(-phi_pm * arma::norm(Ctr.row(nb[a]) - q, 2));
            for (arma::uword b = a+1; b < mm; ++b) {
                const double r = std::exp(-phi_pm * arma::norm(Ctr.row(nb[a]) - Ctr.row(nb[b]), 2));
                CN(a,b) = r; CN(b,a) = r;
            }
        }
        arma::vec b0 = arma::solve(CN, c0, arma::solve_opts::likely_sympd);
        double w0 = 0.0;
        for (arma::uword a = 0; a < mm; ++a) w0 += b0[a] * w_pm[nb[a]];
        yhat_nngp[t] = arma::dot(Xte.row(t), beta_pm) + w0;
        yhat_ols[t]  = arma::dot(Xte.row(t), beta_ols);
    }
    const double rmse_nngp = std::sqrt(arma::mean(arma::square(yhat_nngp - yte)));
    const double rmse_ols  = std::sqrt(arma::mean(arma::square(yhat_ols  - yte)));
    std::printf("held-out prediction RMSE:  NNGP kriging = %.3f   vs   OLS baseline = %.3f\n",
                rmse_nngp, rmse_ols);
    const bool ok_krige = rmse_nngp < rmse_ols;

    // ------------------------------------------------------------- verdict
    const bool ok = ok_beta && ok_ppc && ok_krige;
    std::printf("RESULT: %s  (recovery=%d, ppc=%d, kriging-beats-OLS=%d)\n",
                ok ? "PASS" : "FAIL", (int)ok_beta, (int)ok_ppc, (int)ok_krige);
    return ok ? 0 : 1;
}
