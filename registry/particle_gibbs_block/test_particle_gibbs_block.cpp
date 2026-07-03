/*================================================================================
 *  AI4BayesCode (blocks_local) -- GPL-3.0-or-later. Copyright (C) 2026 contributors.
 *
 *  test_particle_gibbs_block.cpp -- ground-truth library test for the
 *  particle_gibbs_block (conditional SMC / PGAS latent-path sampler).
 *
 *  REGIME LADDER (validate.md §1)
 *    T0  sanity      : flat likelihood (obs_logweight == 0) => stationary law is
 *                      the PRIOR; recover AR(1) prior marginal mean/var.
 *    T1a parity      : linear-Gaussian SSM => exact posterior marginals from the
 *                      Kalman filter + RTS smoother; empirical mean/var of the
 *                      sampled path match within MC SE (batch-means, see below).
 *    T2  recovery    : stochastic-volatility model (the worked example); fix
 *                      theta at truth, recover the latent log-vol path (90% CI
 *                      coverage + RMSE beats the prior-mean guess).
 *    T3  cross-chain : two different-seed, over-dispersed chains on an LGSSM;
 *                      cross-chain rank-normalized R-hat < 1.01 on every marginal.
 *    T4  stress      : long, highly-persistent path -- plain PG (no ancestor
 *                      sampling) DECISIVELY fails to mix the early states while
 *                      PGAS stays < 1.02 (the block's signature hard regime).
 *
 *  SE basis for the parity bars (T0/T1a): the sampler emits an AUTOCORRELATED
 *  chain, so the iid SE sqrt(Var/M) is invalid (too small). We use a BATCH-MEANS
 *  SE (B batches) which absorbs autocorrelation, and a FAMILY-WISE z<6 bar over
 *  the T marginals (per-entry false-reject ~1e-9; the multiplicity-aware bar of
 *  validate.md §1). The variance parity centers on the EXACT smoother mean, so a
 *  mean bias also inflates the variance z (joint mean+var test).
 *
 *  Build (from repo root):
 *    c++ -std=c++17 -O2 -Wno-unused-parameter \
 *      -I include -I include/mcmclib -I include/mcmclib/BaseMatrixOps/include \
 *      -I include/eigen \
 *      -I /Library/Frameworks/R.framework/Versions/Current/Resources/library/RcppArmadillo/include \
 *      -I /tmp/AI4BayesCode_staging/particle_gibbs_block \
 *      -DMCMC_ENABLE_ARMA_WRAPPERS -DARMA_DONT_USE_WRAPPER \
 *      -o /tmp/test_particle_gibbs_block \
 *      /tmp/AI4BayesCode_staging/particle_gibbs_block/test_particle_gibbs_block.cpp \
 *      -framework Accelerate
 *================================================================================*/

#include "particle_gibbs_block.hpp"

#include <cmath>
#include <cstdio>
#include <numeric>
#include <random>
#include <vector>

using namespace AI4BayesCode;

static const double PI    = std::acos(-1.0);
static const double LOG2PI = std::log(2.0 * PI);

static int g_failures = 0;
static void check(bool ok, const char* regime, const char* what, double val,
                  double bar) {
    std::printf("  [%-4s] %-44s = %12.6f  (bar %.4f)  %s\n",
                regime, what, val, bar, ok ? "PASS" : "FAIL");
    if (!ok) ++g_failures;
}

// ---- helpers ----------------------------------------------------------------

// Scalar Kalman filter + RTS smoother for the LGSSM
//   x_1 ~ N(0, s0^2);  x_t = phi x_{t-1} + N(0,q^2);  y_t = x_t + N(0,r^2).
// Returns smoothed marginal means ms[t] and variances Ps[t].
static void kalman_rts(const arma::vec& y, double phi, double q2, double r2,
                       double s0_2, arma::vec& ms, arma::vec& Ps) {
    const std::size_t T = y.n_elem;
    arma::vec mp(T), Pp(T), mf(T), Pf(T);
    mp[0] = 0.0; Pp[0] = s0_2;
    for (std::size_t t = 0; t < T; ++t) {
        const double S = Pp[t] + r2;
        const double K = Pp[t] / S;
        mf[t] = mp[t] + K * (y[t] - mp[t]);
        Pf[t] = (1.0 - K) * Pp[t];
        if (t + 1 < T) {
            mp[t + 1] = phi * mf[t];
            Pp[t + 1] = phi * phi * Pf[t] + q2;
        }
    }
    ms.set_size(T); Ps.set_size(T);
    ms[T - 1] = mf[T - 1]; Ps[T - 1] = Pf[T - 1];
    for (std::size_t tt = T - 1; tt-- > 0; ) {
        const double C = Pf[tt] * phi / Pp[tt + 1];
        ms[tt] = mf[tt] + C * (ms[tt + 1] - mp[tt + 1]);
        Ps[tt] = Pf[tt] + C * C * (Ps[tt + 1] - Pp[tt + 1]);
    }
}

// AR(1) prior marginal variance: V[0]=s0^2, V[t]=phi^2 V[t-1]+q2.
static double prior_marg_var(std::size_t t, double phi, double q2, double s0_2) {
    double V = s0_2;
    for (std::size_t k = 0; k < t; ++k) V = phi * phi * V + q2;
    return V;
}

// Batch-means estimate of E[x] and its SE (absorbs autocorrelation).
static void batch_means(const std::vector<double>& x, std::size_t B,
                        double& mean, double& se) {
    const std::size_t M = x.size();
    const std::size_t L = M / B;
    std::vector<double> bm(B, 0.0);
    for (std::size_t b = 0; b < B; ++b) {
        double s = 0.0;
        for (std::size_t i = 0; i < L; ++i) s += x[b * L + i];
        bm[b] = s / static_cast<double>(L);
    }
    mean = std::accumulate(bm.begin(), bm.end(), 0.0) / static_cast<double>(B);
    double v = 0.0;
    for (double m : bm) v += (m - mean) * (m - mean);
    v /= static_cast<double>(B - 1);          // variance of batch means
    se = std::sqrt(v / static_cast<double>(B));
}

// Acklam's inverse normal CDF (~1e-9).
static double inv_norm_cdf(double p) {
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
    const double pl = 0.02425;
    if (p < pl) {
        double qq = std::sqrt(-2.0 * std::log(p));
        return (((((c[0]*qq+c[1])*qq+c[2])*qq+c[3])*qq+c[4])*qq+c[5]) /
               ((((d[0]*qq+d[1])*qq+d[2])*qq+d[3])*qq+1.0);
    } else if (p <= 1.0 - pl) {
        double qq = p - 0.5, rr = qq * qq;
        return (((((a[0]*rr+a[1])*rr+a[2])*rr+a[3])*rr+a[4])*rr+a[5])*qq /
               (((((b[0]*rr+b[1])*rr+b[2])*rr+b[3])*rr+b[4])*rr+1.0);
    } else {
        double qq = std::sqrt(-2.0 * std::log(1.0 - p));
        return -(((((c[0]*qq+c[1])*qq+c[2])*qq+c[3])*qq+c[4])*qq+c[5]) /
                ((((d[0]*qq+d[1])*qq+d[2])*qq+d[3])*qq+1.0);
    }
}

// Cross-chain rank-normalized R-hat (Vehtari 2021, classical BETWEEN-chain form
// over two FULL chains -- NOT split). A,B equal length M.
static double cross_chain_rank_rhat(const std::vector<double>& A,
                                    const std::vector<double>& B) {
    const std::size_t M = A.size();
    const std::size_t n = 2 * M;
    std::vector<double> pooled(A);
    pooled.insert(pooled.end(), B.begin(), B.end());

    std::vector<std::size_t> idx(n);
    std::iota(idx.begin(), idx.end(), 0);
    std::sort(idx.begin(), idx.end(),
              [&](std::size_t i, std::size_t j) { return pooled[i] < pooled[j]; });
    std::vector<double> rank(n);
    std::size_t i = 0;
    while (i < n) {
        std::size_t j = i;
        while (j + 1 < n && pooled[idx[j + 1]] == pooled[idx[i]]) ++j;
        const double avg = 0.5 * (static_cast<double>(i + 1) +
                                  static_cast<double>(j + 1));  // 1-based avg rank
        for (std::size_t k = i; k <= j; ++k) rank[idx[k]] = avg;
        i = j + 1;
    }
    std::vector<double> z(n);
    for (std::size_t t = 0; t < n; ++t)
        z[t] = inv_norm_cdf((rank[t] - 0.375) / (static_cast<double>(n) + 0.25));

    auto mean_of = [&](std::size_t lo, std::size_t hi) {
        double s = 0.0; for (std::size_t t = lo; t < hi; ++t) s += z[t];
        return s / static_cast<double>(hi - lo);
    };
    const double mA = mean_of(0, M), mB = mean_of(M, n);
    const double grand = 0.5 * (mA + mB);
    auto var_of = [&](std::size_t lo, std::size_t hi, double mu) {
        double s = 0.0; for (std::size_t t = lo; t < hi; ++t) s += (z[t]-mu)*(z[t]-mu);
        return s / static_cast<double>((hi - lo) - 1);
    };
    const double W = 0.5 * (var_of(0, M, mA) + var_of(M, n, mB));
    if (W <= 0.0) return 1.0;
    const double Bvar = static_cast<double>(M) *
                        ((mA - grand) * (mA - grand) + (mB - grand) * (mB - grand));
    const double varhat = (static_cast<double>(M - 1) / M) * W + (1.0 / M) * Bvar;
    return std::sqrt(varhat / W);
}

// Drive one chain: fixed context, burn in, then collect M draws into (M x T).
static arma::mat run_chain(particle_gibbs_block& blk, const block_context& ctx,
                           std::size_t burnin, std::size_t M, std::uint64_t seed,
                           const arma::vec& init_path) {
    std::mt19937_64 rng(seed);
    blk.set_context(ctx);
    blk.set_current(init_path);
    arma::mat draws(M, blk.dim());
    for (std::size_t s = 0; s < burnin; ++s) blk.step(rng);
    for (std::size_t s = 0; s < M; ++s) {
        blk.step(rng);
        draws.row(s) = blk.current().t();
    }
    return draws;
}

// ---- LGSSM callbacks (read phi,q,r,s0 from ctx) ----------------------------

static particle_gibbs_block_config lgssm_cfg(std::size_t T, std::size_t N,
                                             bool ancestor, bool flat_lik) {
    particle_gibbs_block_config c;
    c.name = "x"; c.T = T; c.N = N; c.ancestor_sampling = ancestor; c.obs_key = "y";
    c.init_sample = [](const block_context& ctx, std::mt19937_64& r) {
        const double s0 = ctx.at("s0")[0];
        std::normal_distribution<double> z(0.0, 1.0);
        return s0 * z(r);
    };
    c.transition_sample = [](double xp, std::size_t, const block_context& ctx,
                             std::mt19937_64& r) {
        const double phi = ctx.at("phi")[0], q = ctx.at("q")[0];
        std::normal_distribution<double> z(0.0, 1.0);
        return phi * xp + q * z(r);
    };
    if (flat_lik) {
        c.obs_logweight = [](double, double, std::size_t, const block_context&) {
            return 0.0;  // flat likelihood: stationary law == prior
        };
    } else {
        c.obs_logweight = [](double x, double yt, std::size_t,
                             const block_context& ctx) {
            const double r = ctx.at("r")[0];
            const double d = yt - x;
            return -0.5 * (LOG2PI + 2.0 * std::log(r) + d * d / (r * r));
        };
    }
    c.transition_logpdf = [](double xp, double xt, std::size_t,
                             const block_context& ctx) {
        const double phi = ctx.at("phi")[0], q = ctx.at("q")[0];
        const double d = xt - phi * xp;
        return -0.5 * (LOG2PI + 2.0 * std::log(q) + d * d / (q * q));
    };
    return c;
}

// ============================================================================
int main() {
    std::printf("=== particle_gibbs_block library test (T0-T4 ladder) ===\n");

    // -- T0: flat likelihood => prior recovery (AR(1) marginals) -------------
    {
        const std::size_t T = 6, N = 48;
        const double phi = 0.7, q = 0.5, s0 = 1.0;
        block_context ctx;
        ctx["phi"] = arma::vec{phi}; ctx["q"] = arma::vec{q};
        ctx["s0"] = arma::vec{s0};   ctx["y"]  = arma::zeros<arma::vec>(T);
        particle_gibbs_block blk(lgssm_cfg(T, N, /*ancestor=*/true, /*flat=*/true));
        const std::size_t M = 60000, B = 60, burn = 1000;
        arma::mat dr = run_chain(blk, ctx, burn, M, /*seed=*/11u, arma::zeros<arma::vec>(T));
        double zmean_max = 0.0, zvar_max = 0.0;
        for (std::size_t t = 0; t < T; ++t) {
            const double Vt = prior_marg_var(t, phi, q * q, s0 * s0);
            std::vector<double> col(M), sq(M);
            for (std::size_t s = 0; s < M; ++s) { col[s] = dr(s, t); sq[s] = col[s]*col[s]; }
            double m, sem, v, sev;
            batch_means(col, B, m, sem);
            batch_means(sq,  B, v, sev);   // E[x^2] = Var (prior mean 0)
            zmean_max = std::max(zmean_max, std::fabs(m - 0.0) / sem);
            zvar_max  = std::max(zvar_max,  std::fabs(v - Vt)  / sev);
        }
        std::printf("[T0] flat-likelihood prior recovery (T=%zu,N=%zu,M=%zu):\n", T, N, M);
        check(zmean_max < 6.0, "T0", "max |mean-0| z (prior)",     zmean_max, 6.0);
        check(zvar_max  < 6.0, "T0", "max |var-prior| z",          zvar_max,  6.0);
    }

    // -- T1a: linear-Gaussian SSM parity vs Kalman/RTS smoother --------------
    {
        const std::size_t T = 8, N = 64;
        const double phi = 0.8, q = 0.4, r = 0.5, s0 = 1.0;
        std::mt19937_64 sim(2024u);
        std::normal_distribution<double> nz(0.0, 1.0);
        arma::vec xtrue(T), y(T);
        xtrue[0] = s0 * nz(sim); y[0] = xtrue[0] + r * nz(sim);
        for (std::size_t t = 1; t < T; ++t) {
            xtrue[t] = phi * xtrue[t-1] + q * nz(sim);
            y[t]     = xtrue[t] + r * nz(sim);
        }
        arma::vec ms, Ps;
        kalman_rts(y, phi, q*q, r*r, s0*s0, ms, Ps);

        block_context ctx;
        ctx["phi"] = arma::vec{phi}; ctx["q"] = arma::vec{q};
        ctx["r"]   = arma::vec{r};   ctx["s0"] = arma::vec{s0}; ctx["y"] = y;
        particle_gibbs_block blk(lgssm_cfg(T, N, /*ancestor=*/true, /*flat=*/false));
        const std::size_t M = 80000, B = 80, burn = 2000;
        arma::mat dr = run_chain(blk, ctx, burn, M, /*seed=*/7u, arma::zeros<arma::vec>(T));
        double zmean_max = 0.0, zvar_max = 0.0;
        for (std::size_t t = 0; t < T; ++t) {
            std::vector<double> col(M), sq(M);
            for (std::size_t s = 0; s < M; ++s) {
                col[s] = dr(s, t);
                sq[s]  = (col[s] - ms[t]) * (col[s] - ms[t]);  // center on exact mean
            }
            double m, sem, v, sev;
            batch_means(col, B, m, sem);
            batch_means(sq,  B, v, sev);
            zmean_max = std::max(zmean_max, std::fabs(m - ms[t]) / sem);
            zvar_max  = std::max(zvar_max,  std::fabs(v - Ps[t]) / sev);
        }
        std::printf("[T1a] LGSSM parity vs Kalman/RTS (T=%zu,N=%zu,M=%zu):\n", T, N, M);
        check(zmean_max < 6.0, "T1a", "max |E[x_t]-smoother mean| z", zmean_max, 6.0);
        check(zvar_max  < 6.0, "T1a", "max |Var x_t-smoother var| z", zvar_max,  6.0);
    }

    // -- T2: stochastic-volatility latent log-vol path recovery --------------
    {
        const std::size_t T = 200, N = 128;
        const double mu = -1.0, phi = 0.95, se = 0.25;
        const double stat_sd = se / std::sqrt(1.0 - phi * phi);
        std::mt19937_64 sim(909u);
        std::normal_distribution<double> nz(0.0, 1.0);
        arma::vec htrue(T), y(T);
        htrue[0] = mu + stat_sd * nz(sim);
        y[0]     = std::exp(0.5 * htrue[0]) * nz(sim);
        for (std::size_t t = 1; t < T; ++t) {
            htrue[t] = mu + phi * (htrue[t-1] - mu) + se * nz(sim);
            y[t]     = std::exp(0.5 * htrue[t]) * nz(sim);
        }
        block_context ctx;
        ctx["mu"] = arma::vec{mu}; ctx["phi"] = arma::vec{phi};
        ctx["sigma_eta"] = arma::vec{se}; ctx["y"] = y;

        particle_gibbs_block_config c;
        c.name = "h"; c.T = T; c.N = N; c.ancestor_sampling = true; c.obs_key = "y";
        c.init_sample = [](const block_context& cx, std::mt19937_64& rr) {
            const double m_=cx.at("mu")[0], p_=cx.at("phi")[0], s_=cx.at("sigma_eta")[0];
            std::normal_distribution<double> z(0.0, 1.0);
            return m_ + (s_ / std::sqrt(1.0 - p_*p_)) * z(rr);
        };
        c.transition_sample = [](double hp, std::size_t, const block_context& cx,
                                 std::mt19937_64& rr) {
            const double m_=cx.at("mu")[0], p_=cx.at("phi")[0], s_=cx.at("sigma_eta")[0];
            std::normal_distribution<double> z(0.0, 1.0);
            return m_ + p_ * (hp - m_) + s_ * z(rr);
        };
        c.obs_logweight = [](double h, double yt, std::size_t, const block_context&) {
            return -0.5 * (LOG2PI + h + yt * yt * std::exp(-h));
        };
        c.transition_logpdf = [](double hp, double ht, std::size_t,
                                 const block_context& cx) {
            const double m_=cx.at("mu")[0], p_=cx.at("phi")[0], s_=cx.at("sigma_eta")[0];
            const double d = ht - (m_ + p_ * (hp - m_));
            return -0.5 * (LOG2PI + 2.0 * std::log(s_) + d * d / (s_ * s_));
        };
        particle_gibbs_block blk(c);
        const std::size_t M = 3000, burn = 600;
        arma::mat dr = run_chain(blk, ctx, burn, M, /*seed=*/33u,
                                 arma::vec(T, arma::fill::value(mu)));
        arma::vec hbar = arma::mean(dr, 0).t();          // posterior mean path
        double sse = 0.0, sse_prior = 0.0;
        std::size_t covered = 0;
        for (std::size_t t = 0; t < T; ++t) {
            arma::vec col = dr.col(t);
            arma::vec sorted = arma::sort(col);
            const double lo = sorted[(std::size_t)std::floor(0.05 * M)];
            const double hi = sorted[(std::size_t)std::floor(0.95 * M)];
            if (htrue[t] >= lo && htrue[t] <= hi) ++covered;
            sse       += (hbar[t]   - htrue[t]) * (hbar[t]   - htrue[t]);
            sse_prior += (mu        - htrue[t]) * (mu        - htrue[t]);
        }
        const double rmse = std::sqrt(sse / T);
        const double rmse_prior = std::sqrt(sse_prior / T);
        const double cover = static_cast<double>(covered) / T;
        const double corr = arma::as_scalar(arma::cor(hbar, htrue));
        std::printf("[T2] SV path recovery (T=%zu,N=%zu,M=%zu; stat_sd=%.3f, prior RMSE=%.3f):\n",
                    T, N, M, stat_sd, rmse_prior);
        check(cover >= 0.80 && cover <= 0.99, "T2", "90% CI coverage", cover, 0.90);
        check(rmse < 0.90 * rmse_prior, "T2", "posterior-mean RMSE (vs 0.9*prior)",
              rmse, 0.90 * rmse_prior);
        check(corr > 0.40, "T2", "corr(post-mean path, truth)", corr, 0.40);
    }

    // -- T3: cross-chain rank-normalized R-hat on an LGSSM -------------------
    {
        const std::size_t T = 30, N = 64;
        const double phi = 0.85, q = 0.4, r = 0.5, s0 = 1.0;
        std::mt19937_64 sim(515u);
        std::normal_distribution<double> nz(0.0, 1.0);
        arma::vec xtrue(T), y(T);
        xtrue[0] = s0 * nz(sim); y[0] = xtrue[0] + r * nz(sim);
        for (std::size_t t = 1; t < T; ++t) {
            xtrue[t] = phi * xtrue[t-1] + q * nz(sim);
            y[t]     = xtrue[t] + r * nz(sim);
        }
        block_context ctx;
        ctx["phi"]=arma::vec{phi}; ctx["q"]=arma::vec{q};
        ctx["r"]=arma::vec{r}; ctx["s0"]=arma::vec{s0}; ctx["y"]=y;
        particle_gibbs_block bA(lgssm_cfg(T, N, true, false));
        particle_gibbs_block bB(lgssm_cfg(T, N, true, false));
        const std::size_t M = 4000, burn = 1000;
        arma::mat A = run_chain(bA, ctx, burn, M, 101u, arma::vec(T, arma::fill::value( 5.0)));
        arma::mat Bm= run_chain(bB, ctx, burn, M, 202u, arma::vec(T, arma::fill::value(-5.0)));
        double rhat_max = 0.0;
        for (std::size_t t = 0; t < T; ++t) {
            std::vector<double> ca(M), cb(M);
            for (std::size_t s = 0; s < M; ++s) { ca[s]=A(s,t); cb[s]=Bm(s,t); }
            rhat_max = std::max(rhat_max, cross_chain_rank_rhat(ca, cb));
        }
        std::printf("[T3] cross-chain rank R-hat (T=%zu,N=%zu,M=%zu,inits +/-5):\n", T, N, M);
        check(rhat_max < 1.01, "T3", "max cross-chain rank R-hat", rhat_max, 1.01);
    }

    // -- T4: stress -- plain PG fails where PGAS mixes -----------------------
    {
        const std::size_t T = 90, N = 16;
        const double phi = 0.99, q = 0.12, r = 0.30, s0 = 1.0;
        std::mt19937_64 sim(777u);
        std::normal_distribution<double> nz(0.0, 1.0);
        arma::vec xtrue(T), y(T);
        xtrue[0] = s0 * nz(sim); y[0] = xtrue[0] + r * nz(sim);
        for (std::size_t t = 1; t < T; ++t) {
            xtrue[t] = phi * xtrue[t-1] + q * nz(sim);
            y[t]     = xtrue[t] + r * nz(sim);
        }
        block_context ctx;
        ctx["phi"]=arma::vec{phi}; ctx["q"]=arma::vec{q};
        ctx["r"]=arma::vec{r}; ctx["s0"]=arma::vec{s0}; ctx["y"]=y;
        const std::size_t M = 4000, burn = 1000;

        auto run_pair = [&](bool ancestor) {
            particle_gibbs_block ba(lgssm_cfg(T, N, ancestor, false));
            particle_gibbs_block bb(lgssm_cfg(T, N, ancestor, false));
            arma::mat A  = run_chain(ba, ctx, burn, M, 303u, arma::vec(T, arma::fill::value( 5.0)));
            arma::mat Bm = run_chain(bb, ctx, burn, M, 404u, arma::vec(T, arma::fill::value(-5.0)));
            double rmax = 0.0;
            for (std::size_t t = 0; t < T; ++t) {
                std::vector<double> ca(M), cb(M);
                for (std::size_t s = 0; s < M; ++s) { ca[s]=A(s,t); cb[s]=Bm(s,t); }
                rmax = std::max(rmax, cross_chain_rank_rhat(ca, cb));
            }
            return rmax;
        };
        const double rhat_pg   = run_pair(false);  // plain PG
        const double rhat_pgas = run_pair(true);   // PGAS
        std::printf("[T4] persistent stress (T=%zu,N=%zu,phi=%.2f): PG vs PGAS:\n", T, N, phi);
        check(rhat_pg   > 1.05, "T4", "plain-PG max R-hat (want BROKEN)", rhat_pg,   1.05);
        check(rhat_pgas < 1.02, "T4", "PGAS max R-hat (want mixed)",      rhat_pgas, 1.02);
    }

    // -- BL1: reference-path preservation (deterministic) --------------------
    // The conditioned path x* MUST survive intact in slot 0 at EVERY t and be
    // recoverable by the lineage trace -- the canonical silent cSMC bug (lose
    // the reference => lose invariance, while short-T R-hat can still look ok).
    // Construct a sweep where ONLY the reference has weight: plain PG so the
    // slot-0 lineage is deterministic, a constant reference value REF, model
    // callbacks that emit a DIFFERENT value (so non-reference particles get
    // ~zero weight). The returned path must then equal x* EXACTLY, for any seed.
    {
        const std::size_t T = 25, N = 8;
        const double REF = 7.0;
        particle_gibbs_block_config c;
        c.name = "x"; c.T = T; c.N = N; c.ancestor_sampling = false; c.obs_key = "y";
        c.init_sample       = [](const block_context&, std::mt19937_64&) { return 0.0; };
        c.transition_sample = [](double, std::size_t, const block_context&,
                                 std::mt19937_64&) { return 0.0; };
        c.obs_logweight     = [REF](double x, double, std::size_t,
                                    const block_context&) {
            return (std::fabs(x - REF) < 1e-9) ? 0.0 : -1e12;  // only x*=REF survives
        };
        particle_gibbs_block blk(c);
        block_context ctx; ctx["y"] = arma::zeros<arma::vec>(T);
        blk.set_context(ctx);
        double drift_max = 0.0;
        for (std::uint64_t seed = 1; seed <= 5; ++seed) {
            blk.set_current(arma::vec(T, arma::fill::value(REF)));
            std::mt19937_64 rng(seed);
            for (int s = 0; s < 4; ++s) blk.step(rng);
            const arma::vec& out = blk.current();
            for (std::size_t t = 0; t < T; ++t)
                drift_max = std::max(drift_max, std::fabs(out[t] - REF));
        }
        std::printf("[BL1] reference-path preservation (plain PG, T=%zu,N=%zu):\n", T, N);
        check(drift_max < 1e-9, "BL1", "max |x_out - reference|", drift_max, 1e-9);
    }

    std::printf("=== %s (%d failure%s) ===\n",
                g_failures == 0 ? "ALL REGIMES PASS" : "REGIME FAILURE(S)",
                g_failures, g_failures == 1 ? "" : "s");
    return g_failures == 0 ? 0 : 1;
}
