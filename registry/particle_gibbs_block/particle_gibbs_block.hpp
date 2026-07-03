/*================================================================================
 *  AI4BayesCode  (block_design / blocks_local) -- a community-contributed block.
 *  Copyright (C) 2026 AI4BayesCode contributors.
 *  Licensed under the GNU General Public License v3.0 or later
 *  (GPL-3.0-or-later). See COPYING / LICENSE at the repo root.
 *================================================================================
 *
 *  particle_gibbs_block.hpp
 *  ------------------------
 *  Particle Gibbs (conditional SMC) sampler for the LATENT STATE PATH of a
 *  univariate, first-order state-space (continuous hidden Markov) model, with
 *  optional ancestor sampling (PGAS).
 *
 *  MODEL  (generic univariate state-space model; theta is sampled by siblings)
 *  =====
 *      x_1            ~ m_theta(.)                       initial state law
 *      x_t | x_{t-1}  ~ f_theta(. | x_{t-1})    t=2..T   transition (Markov)
 *      y_t | x_t      ~ g_theta(. | x_t)        t=1..T   emission
 *
 *  The block samples the latent path x_{1:T} (a length-T vector of CONTINUOUS
 *  states) from its full conditional p(x_{1:T} | y_{1:T}, theta). theta and the
 *  observations y_{1:T} are read from the block_context every sweep; theta is
 *  sampled by sibling blocks (e.g. a nuts_block / gibbs block for the dynamics
 *  parameters). All model structure is supplied through five callbacks (below);
 *  the block itself is model-agnostic.
 *
 *  TARGET GEOMETRY  (geometry.md / system_design §11 -- the correctness gate)
 *  ===============
 *      Support of the sampled object: R^T (a fixed-dimension, absolutely
 *      continuous latent path).  ==> system_design §11.1 case 1
 *      (fixed-dim absolutely continuous). NOT a §11.2 STOP case: the dimension
 *      is fixed, and the state is continuous (so this is NOT the finite-state
 *      §11.2(b) HMM handled by hmm_block). The §11.2(b)-style STRONG temporal
 *      Markov dependence is what makes a per-site or single-kernel update
 *      structurally wrong (see the Exception-4 justification below) and a
 *      structured sequential-Monte-Carlo move correct.
 *
 *  WORKED EXAMPLE -- stochastic volatility (see examples/StochasticVolatility.cpp)
 *  ==============
 *      h_t = mu + phi*(h_{t-1} - mu) + sigma_eta * eps_t,   eps_t ~ N(0,1)
 *      y_t ~ N(0, exp(h_t))
 *  i.e.  m_theta = N(mu, sigma_eta^2/(1-phi^2)),
 *        f_theta(.|h_{t-1}) = N(mu + phi*(h_{t-1}-mu), sigma_eta^2),
 *        g_theta(y_t|h_t)   = N(0, exp(h_t)).
 *
 *  ALGORITHM -- conditional SMC (bootstrap proposal) + optional PGAS
 *  =========
 *  One step() = one conditional-SMC sweep, conditioned on the current path
 *  x*_{1:T} (held in slot 0 of the particle system). Bootstrap proposal: the
 *  importance weight at each time is exactly the emission density g_theta.
 *
 *    t = 1:
 *      x_1^0 := x*_1                                        (reference, fixed)
 *      x_1^i ~ m_theta(.)                  i = 1..N-1       (init_sample)
 *      W_1^i prop g_theta(y_1 | x_1^i)     i = 0..N-1       (obs_logweight)
 *    t = 2..T:
 *      {a^i}_{i=1..N-1} ~ resample(W_{t-1})                (systematic/multinomial)
 *      x_t^i ~ f_theta(. | x_{t-1}^{a^i})  i = 1..N-1       (transition_sample)
 *      x_t^0 := x*_t                                        (reference, fixed)
 *      a^0 := 0                            (plain PG)        reference keeps ancestry
 *      a^0 ~ Cat_j( W_{t-1}^j * f_theta(x*_t | x_{t-1}^j) ) (PGAS, ancestor sampling)
 *                                                           (transition_logpdf)
 *      W_t^i prop g_theta(y_t | x_t^i)     i = 0..N-1
 *    end:
 *      J ~ Cat(W_T) ; trace the ancestral lineage of J ; x* <- that trajectory.
 *
 *  INVARIANCE (why this targets the correct posterior):
 *    Conditional SMC leaves p(x_{1:T}|y_{1:T},theta) invariant for ANY N >= 2
 *    -- Andrieu, Doucet & Holenstein, "Particle Markov chain Monte Carlo
 *    methods", JRSS-B 72(3):269-342, 2010 (Theorem 5). Ancestor sampling
 *    PRESERVES that invariance while breaking the path-degeneracy that cripples
 *    plain PG for large T -- Lindsten, Jordan & Schon, "Particle Gibbs with
 *    Ancestor Sampling", JMLR 15:2145-2184, 2014.
 *
 *  // JUSTIFICATION (Check #17): Exception 4 -- no blessed block fits because the
 *  // latent path is CONTINUOUS (so hmm_block's finite-state FFBS does not apply)
 *  // yet first-order Markov-coupled through a possibly non-linear / non-Gaussian
 *  // transition and emission (so no closed-form FFBS/Kalman conditional exists);
 *  // joint/single NUTS structurally inapplicable because a single kernel over the
 *  // whole length-T path must differentiate through the entire transition-emission
 *  // recursion -- impossible for non-differentiable / heavy-tailed emissions, and
 *  // ill-conditioned under strong sequential dependence for long T -- and NUTS
 *  // needs a hand-written T-dim gradient the model may not admit; custom scheme =
 *  // conditional SMC (Particle Gibbs) with optional ancestor sampling (PGAS), a
 *  // bootstrap particle filter conditioned on the current path; targets the
 *  // correct posterior because cSMC is invariant for any N>=2 (ADH 2010 Thm 5)
 *  // and ancestor sampling preserves that invariance (Lindsten et al. 2014).
 *
 *  WHAT THE USER SUPPLIES (particle_gibbs_block_config)
 *  ======================
 *    name, T, N, ancestor_sampling, resampling, obs_key, initial_path, and the
 *    callbacks init_sample / transition_sample / obs_logweight /
 *    transition_logpdf (the last REQUIRED iff ancestor_sampling). Each callback
 *    receives the block_context, so it can read theta (sibling-sampled) freely.
 *
 *  This is a GRADIENT-FREE block: no hand-written gradient, hence NO Check #12
 *  AD-twin and NO Check #5 Jacobian (there is no constraint transform -- the
 *  path lives unconstrained in R^T; any parameter constraints live in the
 *  sibling blocks that sample theta).
 *================================================================================*/

#ifndef AI4BAYESCODE_PARTICLE_GIBBS_BLOCK_HPP
#define AI4BAYESCODE_PARTICLE_GIBBS_BLOCK_HPP

#include "AI4BayesCode/block_sampler.hpp"

#include <cmath>
#include <cstddef>
#include <functional>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace AI4BayesCode {
namespace pg_detail {

// Normalized weights from log-weights (numerically stable). If every entry is
// -inf (a fully-degenerate weight vector) returns the uniform distribution so
// the sweep cannot produce NaN.
inline arma::vec softmax(const arma::vec& logw) {
    const std::size_t n = logw.n_elem;
    const double m = logw.max();
    if (!std::isfinite(m)) {
        arma::vec u(n);
        u.fill(1.0 / static_cast<double>(n));
        return u;
    }
    arma::vec w = arma::exp(logw - m);
    const double s = arma::accu(w);
    return w / s;
}

// Systematic resampling: draw M stratified ancestor indices from normalized
// weights w (length N). Unbiased (E[#offspring_i] = M*w_i), low variance.
inline void systematic_resample(const arma::vec& w, std::size_t M,
                                std::mt19937_64& rng,
                                std::vector<std::size_t>& out) {
    out.resize(M);
    const std::size_t N = w.n_elem;
    std::uniform_real_distribution<double> unif(0.0, 1.0);
    const double u0 = unif(rng) / static_cast<double>(M);
    double cum = w[0];
    std::size_t i = 0;
    for (std::size_t mm = 0; mm < M; ++mm) {
        const double u = u0 + static_cast<double>(mm) / static_cast<double>(M);
        while (u > cum && i + 1 < N) { ++i; cum += w[i]; }
        out[mm] = i;
    }
}

// Multinomial resampling: M i.i.d. ancestor indices from w.
inline void multinomial_resample(const arma::vec& w, std::size_t M,
                                 std::mt19937_64& rng,
                                 std::vector<std::size_t>& out) {
    out.resize(M);
    const std::size_t N = w.n_elem;
    arma::vec cdf = arma::cumsum(w);
    std::uniform_real_distribution<double> unif(0.0, 1.0);
    for (std::size_t mm = 0; mm < M; ++mm) {
        const double u = unif(rng);
        std::size_t i = 0;
        while (u > cdf[i] && i + 1 < N) ++i;
        out[mm] = i;
    }
}

// Single categorical draw from normalized weights w.
inline std::size_t sample_categorical(const arma::vec& w,
                                      std::mt19937_64& rng) {
    std::uniform_real_distribution<double> unif(0.0, 1.0);
    const double u = unif(rng);
    double cum = 0.0;
    for (std::size_t i = 0; i < w.n_elem; ++i) {
        cum += w[i];
        if (u <= cum) return i;
    }
    return w.n_elem - 1;  // safety against fp round-off
}

} // namespace pg_detail

/// Resampling scheme used inside the conditional-SMC sweep.
enum class pg_resampling_t {
    systematic,    ///< stratified, low-variance (default)
    multinomial    ///< i.i.d. multinomial
};

struct particle_gibbs_block_config {
    /// Data key under which the sampled latent path x_{1:T} is published.
    std::string name = "x";

    /// Sequence length (number of time points).
    std::size_t T = 0;

    /// Number of particles. cSMC is invariant for any N >= 2; larger N mixes
    /// better at O(N*T) cost per sweep.
    std::size_t N = 64;

    /// Ancestor sampling (PGAS). ON by default -- preserves invariance and
    /// dramatically improves mixing for long, strongly-dependent paths.
    /// When true, `transition_logpdf` is REQUIRED.
    bool ancestor_sampling = true;

    /// Resampling scheme for the non-reference offspring.
    pg_resampling_t resampling = pg_resampling_t::systematic;

    /// block_context key holding the length-T observation vector y_{1:T}.
    std::string obs_key = "y";

    /// Optional length-T warm-start path. If empty, the reference path is
    /// initialized to zeros (any valid path is fine -- cSMC is invariant to it).
    arma::vec initial_path;

    // ---- model callbacks (all read theta from ctx as needed) ---------------

    /// Draw x_1 ~ m_theta(.).
    std::function<double(const block_context& ctx,
                         std::mt19937_64& rng)> init_sample;

    /// Draw x_t ~ f_theta(. | x_prev). `t` is the (0-based) time index, for
    /// time-varying dynamics.
    std::function<double(double x_prev, std::size_t t,
                         const block_context& ctx,
                         std::mt19937_64& rng)> transition_sample;

    /// Bootstrap importance weight: log g_theta(y_t | x_t). The block fetches
    /// y_t from ctx[obs_key] and passes it in; ctx carries theta.
    std::function<double(double x_t, double y_t, std::size_t t,
                         const block_context& ctx)> obs_logweight;

    /// log f_theta(x_t | x_prev). REQUIRED iff ancestor_sampling == true
    /// (used only for the PGAS ancestor weights); ignored otherwise.
    std::function<double(double x_prev, double x_t, std::size_t t,
                         const block_context& ctx)> transition_logpdf;
};

class particle_gibbs_block : public block_sampler {
public:
    explicit particle_gibbs_block(particle_gibbs_block_config cfg)
        : cfg_(std::move(cfg))
    {
        if (cfg_.T == 0)
            throw std::invalid_argument(
                "particle_gibbs_block: T (sequence length) must be > 0");
        if (cfg_.N < 2)
            throw std::invalid_argument(
                "particle_gibbs_block: N (particle count) must be >= 2");
        if (!cfg_.init_sample)
            throw std::invalid_argument(
                "particle_gibbs_block: init_sample callback is required");
        if (!cfg_.transition_sample)
            throw std::invalid_argument(
                "particle_gibbs_block: transition_sample callback is required");
        if (!cfg_.obs_logweight)
            throw std::invalid_argument(
                "particle_gibbs_block: obs_logweight callback is required");
        if (cfg_.ancestor_sampling && !cfg_.transition_logpdf)
            throw std::invalid_argument(
                "particle_gibbs_block: transition_logpdf is required when "
                "ancestor_sampling is enabled (PGAS)");

        x_.set_size(cfg_.T);
        if (cfg_.initial_path.n_elem == cfg_.T) {
            x_ = cfg_.initial_path;
        } else if (cfg_.initial_path.n_elem == 0) {
            x_.zeros();
        } else {
            throw std::invalid_argument(
                "particle_gibbs_block: initial_path length must be T or 0");
        }

        // Pre-size the particle workspace (reused across sweeps; no per-step
        // reallocation). X(i,t) = state of particle i at time t; anc(i,t) =
        // ancestor index (at t-1) of particle i; column 0 of anc is unused.
        X_.set_size(cfg_.N, cfg_.T);
        anc_.set_size(cfg_.N, cfg_.T);
    }

    void set_context(const block_context& ctx) override {
        context_ = ctx;  // copy: the block keeps no pointer into ctx
    }

    void step(std::mt19937_64& rng) override {
        const std::size_t T = cfg_.T;
        const std::size_t N = cfg_.N;

        const arma::vec& y = context_.at(cfg_.obs_key);
        if (y.n_elem != T)
            throw std::runtime_error(
                "particle_gibbs_block '" + cfg_.name + "': observation key '" +
                cfg_.obs_key + "' has length " + std::to_string(y.n_elem) +
                ", expected T = " + std::to_string(T));

        arma::vec logw(N), w(N);

        // ---- t = 0 -------------------------------------------------------
        X_(0, 0) = x_[0];  // conditioned reference particle (slot 0)
        for (std::size_t i = 1; i < N; ++i)
            X_(i, 0) = cfg_.init_sample(context_, rng);
        for (std::size_t i = 0; i < N; ++i)
            logw[i] = cfg_.obs_logweight(X_(i, 0), y[0], 0, context_);
        w = pg_detail::softmax(logw);

        std::vector<std::size_t> anc_off;  // N-1 resampled non-reference ancestors

        // ---- t = 1 .. T-1 ------------------------------------------------
        for (std::size_t t = 1; t < T; ++t) {
            // resample ancestors for the N-1 non-reference slots from W_{t-1}
            if (cfg_.resampling == pg_resampling_t::systematic)
                pg_detail::systematic_resample(w, N - 1, rng, anc_off);
            else
                pg_detail::multinomial_resample(w, N - 1, rng, anc_off);

            for (std::size_t i = 1; i < N; ++i) {
                const std::size_t parent = anc_off[i - 1];
                anc_(i, t) = parent;
                X_(i, t) = cfg_.transition_sample(X_(parent, t - 1), t,
                                                  context_, rng);
            }

            // conditioned reference particle (slot 0)
            X_(0, t) = x_[t];
            if (cfg_.ancestor_sampling) {
                // PGAS ancestor weights: log W_{t-1}^j + log f(x*_t | x_{t-1}^j)
                arma::vec am(N);
                for (std::size_t j = 0; j < N; ++j) {
                    const double lwj =
                        (w[j] > 0.0) ? std::log(w[j])
                                     : -std::numeric_limits<double>::infinity();
                    am[j] = lwj + cfg_.transition_logpdf(X_(j, t - 1), x_[t],
                                                         t, context_);
                }
                arma::vec aw = pg_detail::softmax(am);
                anc_(0, t) = pg_detail::sample_categorical(aw, rng);
            } else {
                anc_(0, t) = 0;  // plain PG: reference keeps its own ancestry
            }

            // bootstrap weights at t (emission density only)
            for (std::size_t i = 0; i < N; ++i)
                logw[i] = cfg_.obs_logweight(X_(i, t), y[t], t, context_);
            w = pg_detail::softmax(logw);
        }

        // ---- draw a final index and trace its ancestral lineage ----------
        std::size_t b = pg_detail::sample_categorical(w, rng);
        for (std::size_t tt = T; tt-- > 0; ) {
            x_[tt] = X_(b, tt);
            if (tt > 0) b = static_cast<std::size_t>(anc_(b, tt));
        }

        if (keep_history_) history_buf_.push_back(x_);
    }

    const arma::vec& current() const override { return x_; }

    void set_current(const arma::vec& path) override {
        if (path.n_elem != cfg_.T)
            throw std::invalid_argument(
                "particle_gibbs_block '" + cfg_.name +
                "': set_current length must equal T");
        x_ = path;
    }

    const std::string& name() const noexcept override { return cfg_.name; }
    std::size_t dim() const noexcept override { return cfg_.T; }

    state_map current_named_outputs() const override {
        state_map out;
        out.emplace(cfg_.name, x_);
        return out;
    }

    history_map get_history() const override {
        return detail::make_history_map(cfg_.name, history_buf_, x_);
    }

    std::size_t history_size() const noexcept override {
        return history_buf_.empty() ? 1 : history_buf_.size();
    }
    void clear_history() override { history_buf_.clear(); }

private:
    particle_gibbs_block_config cfg_;
    arma::vec                   x_;          // current latent path, length T
    block_context               context_;    // copied each set_context
    std::vector<arma::vec>      history_buf_;

    arma::mat                   X_;           // N x T particle states (workspace)
    arma::umat                  anc_;         // N x T ancestor indices (workspace)
};

} // namespace AI4BayesCode

#endif // AI4BAYESCODE_PARTICLE_GIBBS_BLOCK_HPP
