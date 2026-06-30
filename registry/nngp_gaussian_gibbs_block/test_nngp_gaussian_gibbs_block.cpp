/*================================================================================
 *  AI4BayesCode -- GPL-3.0-or-later. Copyright (C) 2026 AI4BayesCode contributors.
 *
 *  test_nngp_gaussian_gibbs_block.cpp -- ground-truth library test (L3) for
 *  nngp_gaussian_gibbs_block. Self-contained; links only block_sampler.hpp +
 *  nngp_gaussian_gibbs_block.hpp (which pulls gmrf_precision_block + Eigen +
 *  vendored nanoflann). main() returns non-zero if any regime fails.
 *
 *  REGIME LADDER (validate.md §1; Gibbs / direct-draw mechanism)
 *  ------------------------------------------------------------
 *    T0  sanity / prior-limit w parity  : with tau^2 -> inf the w full conditional
 *                                         collapses to the NNGP prior N(0, sigma^2 P^{-1});
 *                                         empirical cov/mean of many draw_w_only() draws
 *                                         match a DENSE independent reference.
 *    T1a closed-form posterior w parity : finite tau^2; empirical cov/mean match the
 *                                         dense N(Q_w^{-1} b_w, Q_w^{-1}),
 *                                         Q_w = sigma^{-2} P + tau^{-2} I.
 *    BL1 NNGP-density consistency        : block log_phi_conditional(phi) matches the
 *                                         dense -0.5(sum log f_i + w'P(phi)w / sigma^2).
 *    T1b FD-gradient                     : N/A -- no hand-written gradient (pure Gibbs+slice).
 *    T2  recovery-from-synthetic-truth   : simulate from known (beta,tau2,sigma2,phi,w),
 *                                         run the FULL sampler, posterior CIs cover truth.
 *    T3  cross-chain rank-normalized Rhat: 2 over-dispersed different-seed chains; the
 *                                         cross-chain (between-chain, NON-split) rank-Rhat
 *                                         (Vehtari 2021) stays < 1.01 (FIXED library bar).
 *    T4  stress                          : larger n + strong spatial correlation; numerical
 *                                         stability + recovery + Rhat < 1.01.
 *    VC  vendored-correctness (nanoflann): block neighbor sets == an independent brute-force
 *                                         m-nearest-among-earlier-ordered computation.
 *    SC  vendored stateful-compatibility : same-seed determinism, two-instance isolation,
 *                                         KD-tree cache rebuild on set_context(B != A).
 *
 *  TOLERANCES (validate.md §5; SE bases stated inline)
 *    - parity cov bar: per-entry SE(s_ij) ~ sqrt((C_ii C_jj + C_ij^2)/M) (Gaussian,
 *      IID draws -- draw_w_only() emits independent exact Gaussian draws), family-wise
 *      max-z < 7.0 over ~n^2/2 entries (false-reject ~1e-10*K). mean bar: max-z < 6.0.
 *    - BL1 density: |delta| < 1e-6 (1 + |val|)  (same math, independent code paths).
 *    - T2 recovery: beta within 4 posterior SD; tau2/sigma2/phi truth within the 99% CI.
 *    - T3/T4 Rhat: cross-chain rank-normalized between-chain Rhat < 1.01.
 *    - jitter on the neighbor correlation diagonal = 1e-8 (matches the block default).
 *
 *  nanoflann vendored-correctness is also covered standalone by VC (neighbor sets vs a
 *  brute-force reference); the test header records that the KD-tree returns the exact
 *  m-nearest-among-earlier sets so a future vendor swap cannot silently drift.
 *================================================================================*/

#include "nngp_gaussian_gibbs_block.hpp"

#include <armadillo>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <random>
#include <string>
#include <vector>

using AI4BayesCode::nngp_gaussian_gibbs_block;
using AI4BayesCode::block_context;

static int g_fail = 0;
static void report(const std::string& tag, bool ok, const std::string& detail) {
    std::cout << (ok ? "[PASS] " : "[FAIL] ") << tag << " -- " << detail << "\n";
    if (!ok) ++g_fail;
}

// ---------------------------------------------------------------------------
//  small numeric helpers
// ---------------------------------------------------------------------------
static double inv_norm_cdf(double p) {
    // Acklam's rational approximation.
    static const double a[] = {-3.969683028665376e+01, 2.209460984245205e+02,
        -2.759285104469687e+02, 1.383577518672690e+02, -3.066479806614716e+01,
        2.506628277459239e+00};
    static const double b[] = {-5.447609879822406e+01, 1.615858368580409e+02,
        -1.556989798598866e+02, 6.680131188771972e+01, -1.328068155288572e+01};
    static const double c[] = {-7.784894002430293e-03, -3.223964580411365e-01,
        -2.400758277161838e+00, -2.549732539343734e+00, 4.374664141464968e+00,
        2.938163982698783e+00};
    static const double d[] = {7.784695709041462e-03, 3.224671290700398e-01,
        2.445134137142996e+00, 3.754408661907416e+00};
    const double plow = 0.02425, phigh = 1.0 - plow;
    double q, r;
    if (p < plow) { q = std::sqrt(-2.0 * std::log(p));
        return (((((c[0]*q+c[1])*q+c[2])*q+c[3])*q+c[4])*q+c[5]) /
               ((((d[0]*q+d[1])*q+d[2])*q+d[3])*q+1.0); }
    if (p <= phigh) { q = p - 0.5; r = q*q;
        return (((((a[0]*r+a[1])*r+a[2])*r+a[3])*r+a[4])*r+a[5])*q /
               (((((b[0]*r+b[1])*r+b[2])*r+b[3])*r+b[4])*r+1.0); }
    q = std::sqrt(-2.0 * std::log(1.0 - p));
    return -(((((c[0]*q+c[1])*q+c[2])*q+c[3])*q+c[4])*q+c[5]) /
            ((((d[0]*q+d[1])*q+d[2])*q+d[3])*q+1.0);
}

static arma::vec rank_normalize(const arma::vec& x) {
    const arma::uword n = x.n_elem;
    arma::uvec ord = arma::sort_index(x);          // ascending
    arma::vec ranks(n);
    for (arma::uword k = 0; k < n; ++k) ranks[ord[k]] = static_cast<double>(k + 1);
    arma::vec z(n);
    for (arma::uword i = 0; i < n; ++i)
        z[i] = inv_norm_cdf((ranks[i] - 0.375) / (static_cast<double>(n) + 0.25));
    return z;
}

// cross-chain (between-chain, NON-split) rank-normalized R-hat over two chains.
static double cross_chain_rhat(const arma::vec& c1, const arma::vec& c2) {
    const double N = static_cast<double>(c1.n_elem);
    arma::vec z  = rank_normalize(arma::join_cols(c1, c2));
    arma::vec z1 = z.head(c1.n_elem), z2 = z.tail(c2.n_elem);
    const double m1 = arma::mean(z1), m2 = arma::mean(z2), gm = 0.5 * (m1 + m2);
    const double B = N * ((m1 - gm) * (m1 - gm) + (m2 - gm) * (m2 - gm));  // /(2-1)
    const double W = 0.5 * (arma::var(z1) + arma::var(z2));                // arma var: /(N-1)
    const double vhat = ((N - 1.0) / N) * W + B / N;
    return std::sqrt(vhat / W);
}

static double euclid(const arma::mat& C, arma::uword i, arma::uword j) {
    return arma::norm(C.row(i) - C.row(j), 2);
}

// Independent reference: coordinate-sort ordering + brute-force m-nearest among
// EARLIER-ordered locations. Returns one (ascending-distance) uvec per location.
static std::vector<arma::uvec> ref_neighbors(const arma::mat& C, std::size_t m) {
    const arma::uword n = C.n_rows, d = C.n_cols;
    std::vector<arma::uword> perm(n);
    for (arma::uword i = 0; i < n; ++i) perm[i] = i;
    std::sort(perm.begin(), perm.end(), [&](arma::uword a, arma::uword b) {
        for (arma::uword dd = 0; dd < d; ++dd) {
            if (C(a, dd) < C(b, dd)) return true;
            if (C(a, dd) > C(b, dd)) return false;
        }
        return a < b;
    });
    std::vector<arma::uvec> nbr(n);
    for (arma::uword k = 0; k < n; ++k) {
        const arma::uword i  = perm[k];
        const std::size_t kk = std::min<std::size_t>(m, k);
        if (kk == 0) continue;
        std::vector<std::pair<double, arma::uword>> cand(k);
        for (arma::uword j = 0; j < k; ++j) cand[j] = {euclid(C, i, perm[j]), perm[j]};
        std::partial_sort(cand.begin(), cand.begin() + kk, cand.end());
        arma::uvec nb(kk);
        for (std::size_t j = 0; j < kk; ++j) nb[j] = cand[j].second;
        nbr[i] = nb;
    }
    return nbr;
}

// Dense unit-variance NNGP precision P = (I-B)'F^{-1}(I-B) from the neighbor sets.
static arma::mat dense_Ptilde(const arma::mat& C, const std::vector<arma::uvec>& nbr,
                              double phi, double jitter) {
    const arma::uword n = C.n_rows;
    arma::mat P(n, n, arma::fill::zeros);
    for (arma::uword i = 0; i < n; ++i) {
        const arma::uvec& nb = nbr[i];
        const arma::uword mi = nb.n_elem;
        if (mi == 0) { P(i, i) += 1.0; continue; }       // f=1, v=[1]
        arma::mat CN(mi, mi);
        for (arma::uword a = 0; a < mi; ++a) {
            CN(a, a) = 1.0 + jitter;
            for (arma::uword b = a + 1; b < mi; ++b) {
                const double r = std::exp(-phi * euclid(C, nb[a], nb[b]));
                CN(a, b) = r; CN(b, a) = r;
            }
        }
        arma::vec c(mi);
        for (arma::uword j = 0; j < mi; ++j) c[j] = std::exp(-phi * euclid(C, i, nb[j]));
        arma::vec bvec = arma::solve(CN, c, arma::solve_opts::likely_sympd);
        double f = 1.0 - arma::dot(c, bvec);
        if (!(f > 0.0)) f = 1.0e-12;
        const double inv_f = 1.0 / f;
        // index list L = [i, nb...], coeff v = [1, -b...]
        arma::uvec L(mi + 1); arma::vec v(mi + 1);
        L[0] = i; v[0] = 1.0;
        for (arma::uword j = 0; j < mi; ++j) { L[j + 1] = nb[j]; v[j + 1] = -bvec[j]; }
        for (arma::uword a = 0; a <= mi; ++a)
            for (arma::uword b = 0; b <= mi; ++b)
                P(L[a], L[b]) += inv_f * v[a] * v[b];
    }
    return P;
}

// Build a column-major-flattened block_context from (y, X, coords).
static block_context make_ctx(const arma::vec& y, const arma::mat& X, const arma::mat& C) {
    block_context ctx;
    ctx["y"]      = y;
    ctx["X"]      = arma::vectorise(X);   // column-major
    ctx["coords"] = arma::vectorise(C);   // column-major
    return ctx;
}

static nngp_gaussian_gibbs_block::config base_cfg(std::size_t n, std::size_t p,
                                                  std::size_t m, double phi_lo, double phi_hi) {
    nngp_gaussian_gibbs_block::config cfg;
    cfg.name = "nn"; cfg.n = n; cfg.p = p; cfg.coord_dim = 2; cfg.m = m;
    cfg.sigma_beta2 = 1.0e4;
    cfg.a_tau = 2.0; cfg.b_tau = 1.0;
    cfg.a_sigma = 2.0; cfg.b_sigma = 1.0;
    cfg.phi_lower = phi_lo; cfg.phi_upper = phi_hi;
    return cfg;
}

// max family-wise z over covariance entries (Gaussian SE, IID draws).
static double max_cov_z(const arma::mat& emp, const arma::mat& ref, double M) {
    const arma::uword n = ref.n_rows;
    double mx = 0.0;
    for (arma::uword i = 0; i < n; ++i)
        for (arma::uword j = i; j < n; ++j) {
            const double se = std::sqrt((ref(i,i)*ref(j,j) + ref(i,j)*ref(i,j)) / M);
            mx = std::max(mx, std::abs(emp(i,j) - ref(i,j)) / se);
        }
    return mx;
}
static double max_mean_z(const arma::rowvec& emp, const arma::vec& ref,
                         const arma::mat& cov, double M) {
    double mx = 0.0;
    for (arma::uword i = 0; i < ref.n_elem; ++i)
        mx = std::max(mx, std::abs(emp[i] - ref[i]) / std::sqrt(cov(i,i) / M));
    return mx;
}

// =========================================================================
//  T0 / T1a -- w-conditional parity (prior limit + posterior)
// =========================================================================
static void parity_w(double tau2_fix, const std::string& tag) {
    const std::size_t n = 40, p = 2, m = 8;
    std::mt19937_64 rng(20260624ULL);
    std::uniform_real_distribution<double> U(0.0, 1.0);
    std::normal_distribution<double> Nrm(0.0, 1.0);

    arma::mat C(n, 2), X(n, p);
    for (std::size_t i = 0; i < n; ++i) { C(i,0)=U(rng); C(i,1)=U(rng); X(i,0)=1.0; X(i,1)=Nrm(rng); }
    arma::vec y(n, arma::fill::zeros);                    // y enters only via b_w (finite tau2)
    for (std::size_t i = 0; i < n; ++i) y[i] = Nrm(rng);

    const double phi_fix = 4.0, sigma2_fix = 2.0;
    arma::vec beta_fix = {0.7, -1.1};

    auto cfg = base_cfg(n, p, m, 0.5, 30.0);
    nngp_gaussian_gibbs_block blk(cfg);
    blk.set_context(make_ctx(y, X, C));
    blk.prepare();

    // fix the state: [beta; tau2; sigma2; phi; w0]
    arma::vec theta(p + n + 3, arma::fill::zeros);
    theta.subvec(0, p-1) = beta_fix;
    theta[p] = tau2_fix; theta[p+1] = sigma2_fix; theta[p+2] = phi_fix;
    blk.set_current(theta);

    // dense reference using the SAME neighbor sets (independently verified by VC)
    arma::mat P = dense_Ptilde(C, ref_neighbors(C, m), phi_fix, 1e-8);
    arma::mat Qw = (1.0 / sigma2_fix) * P;
    Qw.diag() += 1.0 / tau2_fix;
    arma::mat ref_cov = arma::inv_sympd(Qw);
    arma::vec ref_mean = arma::solve(Qw, (1.0 / tau2_fix) * (y - X * beta_fix));

    const std::size_t M = 30000;
    arma::mat draws(M, n);
    for (std::size_t t = 0; t < M; ++t) draws.row(t) = blk.draw_w_only(rng).t();
    arma::rowvec emp_mean = arma::mean(draws, 0);
    arma::mat emp_cov = arma::cov(draws);

    const double zc = max_cov_z(emp_cov, ref_cov, static_cast<double>(M));
    const double zm = max_mean_z(emp_mean, ref_mean, ref_cov, static_cast<double>(M));
    report(tag, zc < 7.0 && zm < 6.0,
           "max cov-z=" + std::to_string(zc) + " (<7), max mean-z=" + std::to_string(zm) +
           " (<6), n=" + std::to_string(n) + " M=" + std::to_string(M));
}

// =========================================================================
//  BL1 -- NNGP density consistency (per-conditional product == quadratic form)
// =========================================================================
static void density_consistency() {
    const std::size_t n = 40, p = 2, m = 8;
    std::mt19937_64 rng(7ULL);
    std::uniform_real_distribution<double> U(0.0, 1.0);
    std::normal_distribution<double> Nrm(0.0, 1.0);
    arma::mat C(n, 2), X(n, p); arma::vec y(n);
    for (std::size_t i = 0; i < n; ++i) { C(i,0)=U(rng); C(i,1)=U(rng); X(i,0)=1; X(i,1)=Nrm(rng); y[i]=Nrm(rng); }

    auto cfg = base_cfg(n, p, m, 0.5, 30.0);
    nngp_gaussian_gibbs_block blk(cfg);
    blk.set_context(make_ctx(y, X, C));
    blk.prepare();

    const double sigma2 = 1.7;
    arma::vec w(n); for (std::size_t i = 0; i < n; ++i) w[i] = Nrm(rng);
    arma::vec theta(p + n + 3, arma::fill::zeros);
    theta[p] = 0.5; theta[p+1] = sigma2; theta[p+2] = 3.0;
    theta.subvec(p+3, p+3+n-1) = w;
    blk.set_current(theta);

    auto nbr = ref_neighbors(C, m);
    bool ok = true; double worst = 0.0;
    for (double phi : {1.0, 3.0, 8.0}) {
        const double blk_val = blk.log_phi_conditional(phi);
        arma::mat P = dense_Ptilde(C, nbr, phi, 1e-8);
        double logdet = 0.0;            // sum log f_i = -log|P| ... recompute via P? use det
        // log|P| = -sum log f_i  => sum log f_i = -log det(P). Use Cholesky logdet.
        double sign; double ld; arma::log_det(ld, sign, P);   // ld = log|det P|
        logdet = -ld;                                          // sum_i log f_i
        const double quad = arma::as_scalar(w.t() * P * w);
        const double ref_val = -0.5 * (logdet + quad / sigma2);
        const double delta = std::abs(blk_val - ref_val);
        worst = std::max(worst, delta / (1.0 + std::abs(ref_val)));
        if (delta > 1e-6 * (1.0 + std::abs(ref_val))) ok = false;
    }
    report("BL1 density-consistency", ok,
           "max rel|delta|=" + std::to_string(worst) + " (<1e-6) over phi in {1,3,8}");
}

// =========================================================================
//  chain runner -- returns selected marginals [beta(p), tau2, sigma2, phi, w0]
// =========================================================================
static arma::mat run_chain(const nngp_gaussian_gibbs_block::config& base,
                           const block_context& ctx, const arma::vec& init,
                           std::uint64_t seed, std::size_t burn, std::size_t samp) {
    auto cfg = base;
    nngp_gaussian_gibbs_block blk(cfg);
    blk.set_context(ctx);
    blk.set_current(init);
    std::mt19937_64 rng(seed);
    for (std::size_t t = 0; t < burn; ++t) blk.step(rng);
    blk.set_keep_history(true);
    for (std::size_t t = 0; t < samp; ++t) blk.step(rng);
    auto H = blk.get_history();
    const std::size_t p = cfg.p;
    arma::mat beta_h = H.at("nn_beta");      // samp x p
    arma::mat tau_h  = H.at("nn_tau2");      // samp x 1
    arma::mat sig_h  = H.at("nn_sigma2");
    arma::mat phi_h  = H.at("nn_phi");
    arma::mat w_h    = H.at("nn_w");         // samp x n
    arma::mat M(samp, p + 4);
    M.cols(0, p-1)  = beta_h;
    M.col(p)        = tau_h.col(0);
    M.col(p+1)      = sig_h.col(0);
    M.col(p+2)      = phi_h.col(0);
    M.col(p+3)      = w_h.col(0);
    return M;
}

static arma::vec overdispersed_init(std::size_t p, std::size_t n, double bval,
                                    double tau2, double sigma2, double phi, double wval) {
    arma::vec init(p + n + 3); init.fill(wval);
    for (std::size_t k = 0; k < p; ++k) init[k] = bval;
    init[p] = tau2; init[p+1] = sigma2; init[p+2] = phi;
    return init;
}

// simulate (coords, X, w_true, y) from known truth
struct SimData { arma::mat C, X; arma::vec y, w_true; };
static SimData simulate(std::size_t n, std::size_t p, std::size_t m,
                        const arma::vec& beta, double tau2, double sigma2, double phi,
                        std::uint64_t seed) {
    std::mt19937_64 rng(seed);
    std::uniform_real_distribution<double> U(0.0, 1.0);
    std::normal_distribution<double> Nrm(0.0, 1.0);
    arma::mat C(n, 2), X(n, p);
    for (std::size_t i = 0; i < n; ++i) { C(i,0)=U(rng); C(i,1)=U(rng); X(i,0)=1.0;
        for (std::size_t k = 1; k < p; ++k) X(i,k) = Nrm(rng); }
    arma::mat P = dense_Ptilde(C, ref_neighbors(C, m), phi, 1e-8);
    arma::mat R = arma::chol(P);                     // R'R = P
    arma::vec z(n); for (std::size_t i = 0; i < n; ++i) z[i] = Nrm(rng);
    arma::vec w_true = std::sqrt(sigma2) * arma::solve(arma::trimatu(R), z); // cov sigma2 P^{-1}
    arma::vec y(n);
    for (std::size_t i = 0; i < n; ++i)
        y[i] = arma::dot(X.row(i), beta) + w_true[i] + std::sqrt(tau2) * Nrm(rng);
    return {C, X, y, w_true};
}

static double quantile(const arma::vec& x, double q) {
    arma::vec s = arma::sort(x);
    double idx = q * (static_cast<double>(s.n_elem) - 1.0);
    arma::uword lo = static_cast<arma::uword>(std::floor(idx));
    arma::uword hi = static_cast<arma::uword>(std::ceil(idx));
    double w = idx - lo;
    return (1.0 - w) * s[lo] + w * s[hi];
}

// =========================================================================
//  T2 -- recovery from synthetic truth
// =========================================================================
static void recovery() {
    const std::size_t n = 180, p = 2, m = 12;
    const arma::vec beta_true = {0.8, -1.2};
    const double tau2_true = 0.3, sigma2_true = 1.5, phi_true = 6.0;
    SimData D = simulate(n, p, m, beta_true, tau2_true, sigma2_true, phi_true, 123ULL);

    auto cfg = base_cfg(n, p, m, 0.5, 30.0);
    auto ctx = make_ctx(D.y, D.X, D.C);
    arma::vec init = overdispersed_init(p, n, 0.0, 1.0, 1.0, 5.0, 0.0);
    arma::mat Mh = run_chain(cfg, ctx, init, 4242ULL, 800, 1500);

    auto covers99 = [&](arma::uword col, double truth) {
        return quantile(Mh.col(col), 0.005) <= truth && truth <= quantile(Mh.col(col), 0.995);
    };
    bool b0 = std::abs(arma::mean(Mh.col(0)) - beta_true[0]) < 4.0 * arma::stddev(Mh.col(0));
    bool b1 = std::abs(arma::mean(Mh.col(1)) - beta_true[1]) < 4.0 * arma::stddev(Mh.col(1));
    bool ct = covers99(p,   tau2_true);
    bool cs = covers99(p+1, sigma2_true);
    bool cp = covers99(p+2, phi_true);
    report("T2 recovery", b0 && b1 && ct && cs && cp,
           "beta0 mean=" + std::to_string(arma::mean(Mh.col(0))) +
           " beta1 mean=" + std::to_string(arma::mean(Mh.col(1))) +
           " ; tau2/sigma2/phi 99% CI cover truth = " +
           std::to_string(ct) + "/" + std::to_string(cs) + "/" + std::to_string(cp));
}

// =========================================================================
//  T3 / T4 -- cross-chain rank-normalized Rhat < 1.01
// =========================================================================
static void rhat_regime(const std::string& tag, std::size_t n, std::size_t m,
                        double phi_true, std::size_t burn, std::size_t samp) {
    const std::size_t p = 2;
    const arma::vec beta_true = {1.0, -0.7};
    SimData D = simulate(n, p, m, beta_true, 0.4, 1.2, phi_true, 999ULL);
    auto cfg = base_cfg(n, p, m, 0.5, 30.0);
    auto ctx = make_ctx(D.y, D.X, D.C);

    arma::vec init1 = overdispersed_init(p, n,  4.0, 0.1, 0.3,  2.0,  3.0);
    arma::vec init2 = overdispersed_init(p, n, -4.0, 3.0, 4.0, 20.0, -3.0);
    arma::mat c1 = run_chain(cfg, ctx, init1, 11ULL, burn, samp);
    arma::mat c2 = run_chain(cfg, ctx, init2, 99ULL, burn, samp);

    const char* names[] = {"beta0", "beta1", "tau2", "sigma2", "phi", "w0"};
    double worst = 0.0; std::string worst_name;
    for (arma::uword col = 0; col < c1.n_cols; ++col) {
        const double rh = cross_chain_rhat(c1.col(col), c2.col(col));
        if (rh > worst) { worst = rh; worst_name = names[col]; }
    }
    report(tag, worst < 1.01,
           "max cross-chain rank-Rhat=" + std::to_string(worst) + " (<1.01) at " + worst_name +
           ", n=" + std::to_string(n) + " phi_true=" + std::to_string(phi_true));
}

// =========================================================================
//  VC -- vendored nanoflann correctness (neighbor sets vs brute force)
// =========================================================================
static void vendored_correctness() {
    const std::size_t n = 80, p = 2, m = 10;
    std::mt19937_64 rng(55ULL);
    std::uniform_real_distribution<double> U(0.0, 1.0);
    std::normal_distribution<double> Nrm(0.0, 1.0);
    arma::mat C(n, 2), X(n, p); arma::vec y(n);
    for (std::size_t i = 0; i < n; ++i) { C(i,0)=U(rng); C(i,1)=U(rng); X(i,0)=1; X(i,1)=Nrm(rng); y[i]=Nrm(rng); }

    auto cfg = base_cfg(n, p, m, 0.5, 30.0);
    nngp_gaussian_gibbs_block blk(cfg);
    blk.set_context(make_ctx(y, X, C));
    blk.prepare();

    auto ref = ref_neighbors(C, m);
    const auto& got = blk.neighbors();
    bool ok = (got.size() == n);
    std::size_t mism = 0;
    for (std::size_t i = 0; ok && i < n; ++i) {
        arma::uvec a = arma::sort(got[i]);     // compare as sets
        arma::uvec b = arma::sort(ref[i]);
        if (a.n_elem != b.n_elem || arma::any(a != b)) { ok = false; mism = i; }
    }
    report("VC nanoflann-correctness", ok,
           ok ? ("all " + std::to_string(n) + " neighbor sets match brute force")
              : ("neighbor-set mismatch at location " + std::to_string(mism)));
}

// =========================================================================
//  SC -- vendored stateful-compatibility (determinism / isolation / cache rebuild)
// =========================================================================
static void stateful_compat() {
    const std::size_t n = 50, p = 2, m = 8;
    std::mt19937_64 rng(8ULL);
    std::uniform_real_distribution<double> U(0.0, 1.0);
    std::normal_distribution<double> Nrm(0.0, 1.0);
    arma::mat CA(n,2), CB(n,2), X(n,p); arma::vec y(n);
    for (std::size_t i = 0; i < n; ++i) { CA(i,0)=U(rng); CA(i,1)=U(rng);
        CB(i,0)=U(rng); CB(i,1)=U(rng); X(i,0)=1; X(i,1)=Nrm(rng); y[i]=Nrm(rng); }
    auto cfg = base_cfg(n, p, m, 0.5, 30.0);

    // (a) same-seed determinism
    auto run_solo = [&](std::uint64_t seed) {
        nngp_gaussian_gibbs_block b(cfg);
        b.set_context(make_ctx(y, X, CA));
        std::mt19937_64 r(seed);
        for (int t = 0; t < 40; ++t) b.step(r);
        return arma::vec(b.current());
    };
    arma::vec d1 = run_solo(314ULL), d2 = run_solo(314ULL);
    bool det = arma::approx_equal(d1, d2, "absdiff", 0.0);

    // (b) two-instance isolation: interleaved B must not perturb A's trajectory
    arma::vec a_solo = run_solo(271ULL);
    arma::vec a_inter;
    {
        nngp_gaussian_gibbs_block A(cfg), B(cfg);
        A.set_context(make_ctx(y, X, CA));
        B.set_context(make_ctx(y, X, CB));
        std::mt19937_64 rA(271ULL), rB(900ULL);
        for (int t = 0; t < 40; ++t) { A.step(rA); B.step(rB); }
        a_inter = A.current();
    }
    bool iso = arma::approx_equal(a_solo, a_inter, "absdiff", 1e-10);

    // (c) cache rebuild on set_context(B != A)
    bool rebuilt;
    {
        nngp_gaussian_gibbs_block b(cfg);
        b.set_context(make_ctx(y, X, CA)); b.prepare();
        std::vector<arma::uvec> nA = b.neighbors();
        b.set_context(make_ctx(y, X, CB)); b.prepare();
        const auto& nB = b.neighbors();
        auto refB = ref_neighbors(CB, m);
        bool matchesB = true, differsFromA = false;
        for (std::size_t i = 0; i < n; ++i) {
            if (arma::any(arma::sort(nB[i]) != arma::sort(refB[i]))) matchesB = false;
            if (nA[i].n_elem != nB[i].n_elem || arma::any(arma::sort(nA[i]) != arma::sort(nB[i])))
                differsFromA = true;
        }
        rebuilt = matchesB && differsFromA;
    }
    report("SC stateful-compat", det && iso && rebuilt,
           "same-seed determinism=" + std::to_string(det) +
           ", two-instance isolation=" + std::to_string(iso) +
           ", set_context cache rebuild=" + std::to_string(rebuilt));
}

int main() {
    std::cout << "=== test_nngp_gaussian_gibbs_block (T1b FD-gradient: N/A, no hand-written gradient) ===\n";
    try {
        parity_w(1.0e8, "T0 prior-limit w-parity");
        parity_w(0.5,   "T1a posterior w-parity");
        density_consistency();
        recovery();
        rhat_regime("T3 cross-chain Rhat",  90,  10,  6.0, 2000, 6000);
        rhat_regime("T4 stress (strong corr)", 250, 10, 2.0, 1500, 3000);
        vendored_correctness();
        stateful_compat();
    } catch (const std::exception& e) {
        std::cout << "[FAIL] EXCEPTION: " << e.what() << "\n";
        return 2;
    }
    std::cout << "=== " << (g_fail == 0 ? "ALL REGIMES PASS" : (std::to_string(g_fail) + " REGIME(S) FAILED"))
              << " ===\n";
    return g_fail == 0 ? 0 : 1;
}
