/*================================================================================
 *  AI4BayesCode (blocks_local) -- GPL-3.0-or-later. Copyright (C) 2026 contributors.
 *
 *  StochasticVolatility.cpp -- worked example for particle_gibbs_block.
 *  ------------------------------------------------------------------------------
 *  The canonical use case: recover the LATENT LOG-VOLATILITY PATH of a univariate
 *  stochastic-volatility model from the returns y_{1:T}, using one
 *  particle_gibbs_block (conditional SMC + ancestor sampling / PGAS).
 *
 *      h_1            ~ N(mu, sigma_eta^2 / (1 - phi^2))      stationary init
 *      h_t | h_{t-1}  ~ N(mu + phi*(h_{t-1}-mu), sigma_eta^2) log-vol AR(1)
 *      y_t | h_t      ~ N(0, exp(h_t))                        returns
 *
 *  The four model callbacks below ARE the whole interface: m_theta (init_sample),
 *  f_theta (transition_sample), g_theta (obs_logweight), and -- because PGAS is on
 *  -- log f_theta (transition_logpdf). Each reads theta = (mu, phi, sigma_eta) from
 *  the block_context, so in a full model the dynamics parameters are sampled by
 *  SIBLING blocks and updated in ctx every sweep. Here we FIX theta at the truth to
 *  isolate the path-recovery demonstration; swapping in a sibling theta-sampler is
 *  the only change needed for a full Gibbs sampler.
 *
 *  Build (from repo root, after delivery to blocks_local/):
 *    c++ -std=c++17 -O2 -Wno-unused-parameter \
 *      -I include -I include/mcmclib -I include/mcmclib/BaseMatrixOps/include \
 *      -I include/eigen \
 *      -I /Library/Frameworks/R.framework/Versions/Current/Resources/library/RcppArmadillo/include \
 *      -I blocks_local/particle_gibbs_block \
 *      -DMCMC_ENABLE_ARMA_WRAPPERS -DARMA_DONT_USE_WRAPPER \
 *      -o /tmp/sv_demo \
 *      blocks_local/particle_gibbs_block/examples/StochasticVolatility.cpp \
 *      -framework Accelerate
 *================================================================================*/

#include "particle_gibbs_block.hpp"

#include <cmath>
#include <cstdio>
#include <random>

using namespace AI4BayesCode;

static const double LOG2PI = std::log(2.0 * std::acos(-1.0));

int main() {
    // ---- ground-truth stochastic-volatility parameters --------------------
    const std::size_t T = 250;
    const double mu = -1.0, phi = 0.97, sigma_eta = 0.20;
    const double stat_sd = sigma_eta / std::sqrt(1.0 - phi * phi);

    // ---- simulate a returns series y_{1:T} with latent log-vol h_{1:T} -----
    std::mt19937_64 sim(20260626u);
    std::normal_distribution<double> nz(0.0, 1.0);
    arma::vec htrue(T), y(T);
    htrue[0] = mu + stat_sd * nz(sim);
    y[0]     = std::exp(0.5 * htrue[0]) * nz(sim);
    for (std::size_t t = 1; t < T; ++t) {
        htrue[t] = mu + phi * (htrue[t - 1] - mu) + sigma_eta * nz(sim);
        y[t]     = std::exp(0.5 * htrue[t]) * nz(sim);
    }

    // ---- theta lives in the context (a sibling block would sample it) ------
    block_context ctx;
    ctx["mu"]        = arma::vec{mu};
    ctx["phi"]       = arma::vec{phi};
    ctx["sigma_eta"] = arma::vec{sigma_eta};
    ctx["y"]         = y;

    // ---- configure the block: SV model via four callbacks ------------------
    particle_gibbs_block_config c;
    c.name = "h";
    c.T = T;
    c.N = 128;                    // particles; cSMC is invariant for any N >= 2
    c.ancestor_sampling = true;   // PGAS -- essential for phi this close to 1
    c.obs_key = "y";

    c.init_sample = [](const block_context& cx, std::mt19937_64& r) {
        const double m_ = cx.at("mu")[0], p_ = cx.at("phi")[0], s_ = cx.at("sigma_eta")[0];
        std::normal_distribution<double> z(0.0, 1.0);
        return m_ + (s_ / std::sqrt(1.0 - p_ * p_)) * z(r);
    };
    c.transition_sample = [](double hp, std::size_t, const block_context& cx,
                             std::mt19937_64& r) {
        const double m_ = cx.at("mu")[0], p_ = cx.at("phi")[0], s_ = cx.at("sigma_eta")[0];
        std::normal_distribution<double> z(0.0, 1.0);
        return m_ + p_ * (hp - m_) + s_ * z(r);
    };
    c.obs_logweight = [](double h, double yt, std::size_t, const block_context&) {
        // log N(y_t; 0, exp(h)) = -0.5*(log 2pi + h + y^2 * exp(-h))
        return -0.5 * (LOG2PI + h + yt * yt * std::exp(-h));
    };
    c.transition_logpdf = [](double hp, double ht, std::size_t, const block_context& cx) {
        const double m_ = cx.at("mu")[0], p_ = cx.at("phi")[0], s_ = cx.at("sigma_eta")[0];
        const double d = ht - (m_ + p_ * (hp - m_));
        return -0.5 * (LOG2PI + 2.0 * std::log(s_) + d * d / (s_ * s_));
    };

    // ---- run the sampler ---------------------------------------------------
    particle_gibbs_block blk(c);
    blk.set_context(ctx);
    blk.set_current(arma::vec(T, arma::fill::value(mu)));  // warm start at the mean

    std::mt19937_64 rng(2026u);
    const std::size_t burnin = 500, draws = 4000;
    for (std::size_t s = 0; s < burnin; ++s) blk.step(rng);

    arma::mat H(draws, T);
    for (std::size_t s = 0; s < draws; ++s) {
        blk.step(rng);
        H.row(s) = blk.current().t();
    }

    // ---- summarize recovery ------------------------------------------------
    arma::vec hbar = arma::mean(H, 0).t();
    double sse = 0.0, sse_prior = 0.0;
    std::size_t covered = 0;
    arma::vec lo(T), hi(T);
    for (std::size_t t = 0; t < T; ++t) {
        arma::vec col = arma::sort(H.col(t));
        lo[t] = col[(std::size_t)std::floor(0.05 * draws)];
        hi[t] = col[(std::size_t)std::floor(0.95 * draws)];
        if (htrue[t] >= lo[t] && htrue[t] <= hi[t]) ++covered;
        sse       += (hbar[t] - htrue[t]) * (hbar[t] - htrue[t]);
        sse_prior += (mu      - htrue[t]) * (mu      - htrue[t]);
    }
    const double rmse       = std::sqrt(sse / T);
    const double rmse_prior = std::sqrt(sse_prior / T);
    const double cover      = static_cast<double>(covered) / T;
    const double corr       = arma::as_scalar(arma::cor(hbar, htrue));

    std::printf("=== particle_gibbs_block -- stochastic-volatility demo ===\n");
    std::printf("T=%zu  N=%zu  PGAS=on   theta=(mu=%.2f, phi=%.2f, sigma_eta=%.2f)\n",
                T, c.N, mu, phi, sigma_eta);
    std::printf("draws=%zu (burnin=%zu)\n\n", draws, burnin);

    std::printf("  %4s  %9s  %9s  %18s\n", "t", "h_true", "post.mean", "90% CI");
    for (std::size_t t = 0; t < T; t += T / 10) {
        std::printf("  %4zu  %9.3f  %9.3f  [%7.3f, %7.3f]\n",
                    t, htrue[t], hbar[t], lo[t], hi[t]);
    }

    std::printf("\nposterior-mean RMSE      = %.3f   (prior-guess RMSE = %.3f)\n",
                rmse, rmse_prior);
    std::printf("90%% CI coverage          = %.3f\n", cover);
    std::printf("corr(post.mean, truth)   = %.3f\n", corr);
    std::printf("\nThe latent log-vol path is recovered well below the prior baseline;\n"
                "in a full model a sibling block would resample (mu, phi, sigma_eta)\n"
                "into the context between sweeps.\n");
    return 0;
}
