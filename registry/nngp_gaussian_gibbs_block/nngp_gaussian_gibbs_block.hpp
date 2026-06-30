/*================================================================================
 *  AI4BayesCode: stateful modular MCMC for composable Gibbs samplers
 *  Copyright (C) 2026 AI4BayesCode contributors.
 *  Licensed under the GNU General Public License v3.0 or later
 *  (GPL-3.0-or-later). See COPYING / LICENSE at the repo root.
 *
 *  Vendored third-party code under vendor/ keeps its own upstream license:
 *    - nanoflann (BSD 2-Clause) -- vendor/nanoflann/ (KD-tree neighbor search).
 *================================================================================
 *
 *  nngp_gaussian_gibbs_block.hpp -- full Bayesian Gaussian spatial regression with
 *                                   a Nearest-Neighbor Gaussian Process (NNGP)
 *                                   latent spatial random effect. Samples
 *                                   (beta, tau^2, sigma^2, phi, w) in ONE
 *                                   self-contained block.
 *
 *  MODEL  (Datta, Banerjee, Finley & Gelfand 2016, JASA <doi:10.1080/01621459.2015.1044091>;
 *          Finley, Datta, Cook, Morton, Andersen & Banerjee 2019, JCGS
 *          <doi:10.1080/10618600.2018.1537924>)
 *  =====
 *      y_i | beta, w_i, tau^2  ~  Normal( x_i' beta + w_i , tau^2 ),   i = 1..n
 *      w_i | w_{N(i)}          ~  Normal( b_i' w_{N(i)} , f_i )        (NNGP)
 *      beta                    ~  Normal( 0 , sigma_beta^2 I_p )
 *      tau^2                   ~  Inverse-Gamma( a_tau   , b_tau   )   (shape, scale)
 *      sigma^2                 ~  Inverse-Gamma( a_sigma , b_sigma )   (shape, scale)
 *      phi                     ~  Uniform( phi_lower , phi_upper )
 *
 *  The NNGP replaces the dense GP prior w ~ N(0, sigma^2 R(phi)) by the sparse
 *  Vecchia product prod_i p(w_i | w_{N(i)}), where N(i) is the set of up to m
 *  nearest neighbors of location i among the EARLIER-ordered locations. With the
 *  exponential correlation r(d) = exp(-phi d) and, per location i,
 *      C_N = r(D_{N(i),N(i)}),   c = r(d_{i,N(i)}),
 *      b_i = C_N^{-1} c,         f_i = 1 - c' b_i,            (unit-variance form)
 *  the induced prior precision is sigma^{-2} P with P = (I - B)' F^{-1} (I - B),
 *  B strictly "lower-triangular" (in the ordering) with rows b_i, F = diag(f_i).
 *  |I - B| = 1, so log|P| = -sum_i log f_i and w' P w = sum_i (w_i - b_i'w_{N(i)})^2 / f_i.
 *
 *  TARGET GEOMETRY (system_design.md §11.1, class 1: fixed-dim absolutely
 *  continuous). State (beta in R^p, tau^2>0, sigma^2>0, phi in (a,b), w in R^n) is
 *  continuous and fixed-dimension. The spatial field w is a CONTINUOUS Gaussian
 *  field with sparse precision (a Vecchia GMRF) -- NOT a discrete strongly-coupled
 *  field, so neither §11.2 STOP class applies; exact Gaussian conditionals make
 *  Gibbs correct and efficient (cf. gmrf_precision_block).
 *
 *  ALGORITHM (per sweep; all updates are exact full conditionals except phi)
 *  ========================================================================
 *    (phi,    : JOINT (collapsed) block update of the confounded range/variance.
 *     sigma^2)  phi is slice-sampled (Neal 2003) within (phi_lower, phi_upper) on the
 *              sigma^2-MARGINALIZED conditional
 *                g(phi) = -0.5 log|F(phi)| - (n/2 + a_sigma) log(b_sigma + 0.5 w'P(phi)w);
 *              then sigma^2 | phi, w is drawn from its conjugate
 *                IG( a_sigma + n/2 , b_sigma + 0.5 w'P w ).
 *              Together this is an exact block draw of (sigma^2, phi) | w; integrating
 *              sigma^2 out of the phi step breaks the sigma^2 <-> phi (microergodicity)
 *              ridge that stalls componentwise mixing under strong correlation. No gradient.
 *    w       : exact block Gaussian draw N(Q_w^{-1} b_w, Q_w^{-1}),
 *              Q_w = sigma^{-2} P + tau^{-2} I, b_w = tau^{-2}(y - X beta),
 *              delegated to a COMPOSED child gmrf_precision_block (sparse
 *              Cholesky, Rue 2001) -- the blessed "compose a child, do not
 *              hand-roll" pattern (design.md Stage 2.1).
 *    tau^2   : conjugate IG( a_tau + n/2 , b_tau + 0.5 ||y - X beta - w||^2 ).
 *    beta    : conjugate Gaussian, precision X'X/tau^2 + I/sigma_beta^2.
 *    +PX     : an exact, target-preserving parameter-expansion level-shift recenters
 *              the intercept against the spatial level each sweep (beta0 -= alpha,
 *              w += alpha) to break their confounding -- the dominant mixing
 *              bottleneck for componentwise spatial Gibbs (see level_shift_; requires
 *              an intercept column, auto-disabled otherwise).
 *
 *  The neighbor sets N(i) and all fixed pairwise distances are built ONCE from
 *  the coordinates (KD-tree via vendored nanoflann; coordinate-sort ordering) and
 *  rebuilt only when the coordinates change (vendor.md §3.3 cache rule). Only the
 *  correlation VALUES (b_i, f_i) are recomputed when phi moves.
 *
 *  CONSTRAINTS / JACOBIANS (design.md Stage 3): every parameter is sampled on its
 *  NATURAL scale -- conjugate draws (IG draws are >0 by construction; the Gaussian
 *  draws are unconstrained), and a bounded slice for phi. There is NO unconstrained
 *  transform, NO constraint Jacobian (those are NUTS-only) and NO hand-written
 *  gradient, so validator Check #5 and Check #12 (AD-twin) do not apply.
 *
 *  SCALE PRIORS (system_design.md §11.6): tau^2 and sigma^2 use Inverse-Gamma --
 *  the deliberate conjugacy choice standard in NNGP/spNNGP -- with PROPER,
 *  weakly-informative defaults (shape 2, data-scaled scale), NOT the prohibited
 *  IG(eps,eps) noninformative default.
 *
 *  JUSTIFICATION (Check #17): Exception 4 (codegen_priors §2b) -- AI-authored
 *  custom block. No blessed block fits (the library has no NNGP / Vecchia-GP
 *  primitive; gmrf_* blocks target areal/lattice precision, celerite is 1-D). A
 *  joint/single NUTS sampler is structurally inappropriate for the n-dimensional
 *  conjugate-Gaussian field w (and the IG/Gaussian variance & coefficient
 *  conditionals), which exact Gibbs handles far more efficiently. The custom
 *  scheme is conjugate Gibbs (sparse-Cholesky w-draw + IG tau^2 + Gaussian beta) with
 *  a JOINT (sigma^2, phi) block update -- a sigma^2-marginalized slice on phi then the
 *  conjugate sigma^2 draw -- for the confounded range/variance. It targets the
 *  correct posterior because every update is the exact full conditional of the
 *  stated model (the sparse-Cholesky w-draw reuses the validated
 *  gmrf_precision_block kernel). Conjugate Gibbs usages: Exception 1 (closed-form
 *  conjugate), see per-update notes below.
 *
 *  I/O CONTRACT (block_sampler / Tier B)
 *  =====================================
 *    INPUTS (refreshable, read from block_context in set_context):
 *      x_key      (default "X")      : design matrix, column-major flat, length n*p
 *      y_key      (default "y")      : response, length n
 *      coords_key (default "coords") : coordinates, column-major flat, length n*d
 *    OUTPUTS (current_named_outputs / get_history): one entry per sub-parameter
 *      "<name>_beta"   (R^p), "<name>_tau2" (>0), "<name>_sigma2" (>0),
 *      "<name>_phi"    (in (phi_lower,phi_upper)), "<name>_w" (R^n)
 *    current()/set_current() use the concatenated natural-scale vector
 *      [ beta (p) ; tau2 (1) ; sigma2 (1) ; phi (1) ; w (n) ]  (dim = p+n+3).
 *    TUNABLE SLOTS (config defaults; adjustable post-construction via the
 *    fine-grained C++ setters): m, sigma_beta2, a_tau/b_tau, a_sigma/b_sigma,
 *    phi bounds, slice width. phi bounds + slice width auto-derive from the
 *    spatial domain when left at their <=0 sentinels.
 *
 *  NOTE on prediction: NNGP kriging at new sites (find the new location's m
 *  nearest training neighbors, draw w_new ~ N(b_0' w_{N(0)}, sigma^2 f_0), then
 *  y_new ~ N(x_new'beta + w_new, tau^2)) is a Tier-A / example concern and is
 *  demonstrated in the shipped C++ example, NOT baked into this Tier-B block --
 *  matching the gmrf_precision_block convention (Tier-B blocks implement the
 *  sampler contract, not predict_at).
 *
 *  ENGINE FAMILY: engine_kind() = MCMC (Gibbs + slice). supports_readapt() =
 *  false (no NUTS kernel; slice has no tunable metric).
 *================================================================================*/

#ifndef AI4BAYESCODE_NNGP_GAUSSIAN_GIBBS_BLOCK_HPP
#define AI4BAYESCODE_NNGP_GAUSSIAN_GIBBS_BLOCK_HPP

#include "AI4BayesCode/block_sampler.hpp"
#include "AI4BayesCode/gmrf_precision_block.hpp"  // child sparse-Cholesky draw + Eigen + arma_to_eigen_sparse

#include "vendor/nanoflann/nanoflann.hpp"           // vendored (BSD 2-Clause): KD-tree neighbor search
                                                    // bundle-relative path; bundle root is on -I (vendor.md §4)

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace AI4BayesCode {

/**
 * @brief Full Bayesian Gaussian spatial regression with an NNGP latent random
 *        effect, sampling (beta, tau^2, sigma^2, phi, w) by conjugate Gibbs +
 *        a slice step for phi.
 *
 * @see file header for the model, geometry class, algorithm and Exception-4
 *      justification.
 */
class nngp_gaussian_gibbs_block : public block_sampler {
public:
    struct config {
        /// Unique block name. Sub-outputs are written under "<name>_{beta,tau2,
        /// sigma2,phi,w}".
        std::string name = "nngp_gaussian_gibbs";

        /// Number of observations / locations n (REQUIRED, > 0).
        std::size_t n = 0;
        /// Number of regression coefficients p (REQUIRED, > 0).
        std::size_t p = 0;
        /// Spatial coordinate dimension d (default 2).
        std::size_t coord_dim = 2;
        /// Number of nearest neighbors m in the NNGP (default 10; Datta 2016
        /// finds m ~ 10-15 ~ full GP).
        std::size_t m = 10;

        // ---- priors (all tunable) ----
        /// beta ~ Normal(0, sigma_beta2 * I_p). Default near-flat.
        double sigma_beta2 = 1.0e4;
        /// tau^2 ~ Inverse-Gamma(a_tau, b_tau) (SHAPE, SCALE). Proper default.
        double a_tau = 2.0;
        double b_tau = 1.0;
        /// sigma^2 ~ Inverse-Gamma(a_sigma, b_sigma) (SHAPE, SCALE). Proper default.
        double a_sigma = 2.0;
        double b_sigma = 1.0;
        /// phi ~ Uniform(phi_lower, phi_upper). If either is <= 0 (sentinel),
        /// BOTH are auto-derived from the spatial domain: phi_lower = 3/diag,
        /// phi_upper = 300/diag (effective range from the full domain down to 1%
        /// of it), diag = bounding-box diagonal of the coordinates.
        double phi_lower = 0.0;
        double phi_upper = 0.0;
        /// Slice-sampler stepping-out width on phi. <= 0 (sentinel) => auto
        /// (phi_upper - phi_lower)/10.
        double slice_width = 0.0;
        /// Jitter added to each neighbor correlation matrix diagonal for
        /// numerical stability of the m-by-m solve (default 1e-8).
        double cnn_jitter = 1.0e-8;
        /// Parameter-expansion level-shift each sweep: draw an auxiliary intercept
        /// <-> spatial-level translation from its exact Gaussian conditional to break
        /// the intercept/spatial-level confounding that otherwise slows componentwise
        /// Gibbs (target-preserving; Gelfand-Sahu-Carlin 1995 / Liu-Wu 1999). Default
        /// true; auto-disabled if the first design column is not an all-ones intercept.
        bool use_level_shift = true;

        // ---- initial values (natural scale) ----
        arma::vec initial_beta;        ///< length p; empty => zeros.
        double    initial_tau2  = 1.0; ///< > 0.
        double    initial_sigma2 = 1.0;///< > 0.
        double    initial_phi   = 0.0; ///< <= 0 => geometric mean of phi bounds.
        arma::vec initial_w;           ///< length n; empty => zeros.

        // ---- block_context keys for the refreshable inputs ----
        std::string x_key      = "X";
        std::string y_key      = "y";
        std::string coords_key = "coords";

        /// nanoflann KD-tree max leaf size.
        std::size_t kd_leaf_max = 10;
        /// Record every draw into history when true.
        bool keep_history = false;
    };

    explicit nngp_gaussian_gibbs_block(config cfg)
        : cfg_(std::move(cfg)),
          n_(cfg_.n), p_(cfg_.p), d_(cfg_.coord_dim), m_(cfg_.m),
          sigma_beta2_(cfg_.sigma_beta2),
          a_tau_(cfg_.a_tau), b_tau_(cfg_.b_tau),
          a_sigma_(cfg_.a_sigma), b_sigma_(cfg_.b_sigma),
          phi_lower_(cfg_.phi_lower), phi_upper_(cfg_.phi_upper),
          slice_width_(cfg_.slice_width), cnn_jitter_(cfg_.cnn_jitter)
    {
        if (cfg_.name.empty())
            throw std::invalid_argument("nngp_gaussian_gibbs_block: name must be non-empty");
        if (n_ == 0) throw std::invalid_argument("nngp_gaussian_gibbs_block: n must be > 0");
        if (p_ == 0) throw std::invalid_argument("nngp_gaussian_gibbs_block: p must be > 0");
        if (d_ == 0) throw std::invalid_argument("nngp_gaussian_gibbs_block: coord_dim must be > 0");
        if (m_ == 0) throw std::invalid_argument("nngp_gaussian_gibbs_block: m must be > 0");
        if (!(sigma_beta2_ > 0.0))
            throw std::invalid_argument("nngp_gaussian_gibbs_block: sigma_beta2 must be > 0");
        if (!(a_tau_ > 0.0 && b_tau_ > 0.0))
            throw std::invalid_argument("nngp_gaussian_gibbs_block: tau^2 IG (a_tau,b_tau) must be > 0");
        if (!(a_sigma_ > 0.0 && b_sigma_ > 0.0))
            throw std::invalid_argument("nngp_gaussian_gibbs_block: sigma^2 IG (a_sigma,b_sigma) must be > 0");
        if (!(cfg_.initial_tau2 > 0.0))
            throw std::invalid_argument("nngp_gaussian_gibbs_block: initial_tau2 must be > 0");
        if (!(cfg_.initial_sigma2 > 0.0))
            throw std::invalid_argument("nngp_gaussian_gibbs_block: initial_sigma2 must be > 0");
        if (phi_lower_ > 0.0 && phi_upper_ > 0.0 && !(phi_lower_ < phi_upper_))
            throw std::invalid_argument("nngp_gaussian_gibbs_block: need phi_lower < phi_upper");
        if (m_ >= n_)
            throw std::invalid_argument("nngp_gaussian_gibbs_block: m must be < n");

        // ---- state init ----
        beta_ = cfg_.initial_beta.is_empty() ? arma::vec(p_, arma::fill::zeros)
                                             : cfg_.initial_beta;
        if (beta_.n_elem != p_)
            throw std::invalid_argument("nngp_gaussian_gibbs_block: initial_beta length must equal p");
        tau2_   = cfg_.initial_tau2;
        sigma2_ = cfg_.initial_sigma2;
        phi_    = cfg_.initial_phi;  // may be <= 0 sentinel; finalized in rebuild_structure_
        w_ = cfg_.initial_w.is_empty() ? arma::vec(n_, arma::fill::zeros) : cfg_.initial_w;
        if (w_.n_elem != n_)
            throw std::invalid_argument("nngp_gaussian_gibbs_block: initial_w length must equal n");

        // ---- composed child: exact sparse-Cholesky Gaussian draw of w ----
        gmrf_precision_block_config wc;
        wc.name = cfg_.name + "_w_kernel";
        wc.n    = n_;
        wc.Q_fn = [this](const block_context&) -> arma::sp_mat { return this->Qw_arma_; };
        wc.b_fn = [this](const block_context&) -> arma::vec    { return this->bw_arma_; };
        wc.initial_x = w_;
        child_w_ = std::make_unique<gmrf_precision_block>(std::move(wc));

        keep_history_ = cfg_.keep_history;
        sync_current_();
    }

    // The child's Q_fn/b_fn capture `this`; copying/moving would dangle them.
    nngp_gaussian_gibbs_block(const nngp_gaussian_gibbs_block&)            = delete;
    nngp_gaussian_gibbs_block& operator=(const nngp_gaussian_gibbs_block&) = delete;
    nngp_gaussian_gibbs_block(nngp_gaussian_gibbs_block&&)                 = delete;
    nngp_gaussian_gibbs_block& operator=(nngp_gaussian_gibbs_block&&)      = delete;

    // ---- block_sampler contract -----------------------------------------

    void set_context(const block_context& ctx) override {
        const bool first = !have_data_;

        auto require = [&](const std::string& k) -> const arma::vec& {
            auto it = ctx.find(k);
            if (it == ctx.end())
                throw std::runtime_error("nngp_gaussian_gibbs_block '" + cfg_.name +
                                         "': set_context missing key '" + k + "'");
            return it->second;
        };

        // coords: required on first call; refreshable afterwards.
        if (first || ctx.find(cfg_.coords_key) != ctx.end()) {
            const arma::vec& cf = require(cfg_.coords_key);
            if (cf.n_elem != n_ * d_)
                throw std::runtime_error("nngp_gaussian_gibbs_block '" + cfg_.name +
                                         "': coords length must be n*coord_dim");
            arma::mat new_coords(cf.memptr(), n_, d_);  // column-major reshape (copy)
            if (first || !arma::approx_equal(new_coords, coords_, "absdiff", 0.0)) {
                coords_ = new_coords;
                dirty_structure_ = true;
            }
        }
        // y, X: required on first call; refreshable afterwards.
        if (first || ctx.find(cfg_.y_key) != ctx.end()) {
            const arma::vec& yv = require(cfg_.y_key);
            if (yv.n_elem != n_)
                throw std::runtime_error("nngp_gaussian_gibbs_block '" + cfg_.name +
                                         "': y length must be n");
            y_ = yv;
        }
        if (first || ctx.find(cfg_.x_key) != ctx.end()) {
            const arma::vec& xv = require(cfg_.x_key);
            if (xv.n_elem != n_ * p_)
                throw std::runtime_error("nngp_gaussian_gibbs_block '" + cfg_.name +
                                         "': X length must be n*p");
            X_   = arma::mat(xv.memptr(), n_, p_);  // column-major reshape (copy)
            XtX_ = X_.t() * X_;
            has_intercept_ = true;                  // for the level-shift: is col 0 all ones?
            for (std::size_t i = 0; i < n_; ++i)
                if (std::abs(X_(i, 0) - 1.0) > 1e-8) { has_intercept_ = false; break; }
        }
        have_data_ = true;
    }

    void step(std::mt19937_64& rng) override {
        if (!have_data_)
            throw std::runtime_error("nngp_gaussian_gibbs_block '" + cfg_.name +
                                     "': step() before set_context() provided data");
        if (dirty_structure_) { rebuild_structure_(); dirty_structure_ = false; }

        slice_update_phi_(rng);   // phi | w, sigma^2  (leaves BF cache at phi_)
        update_sigma2_(rng);      // sigma^2 | w, phi
        update_w_(rng);           // w | sigma^2, phi, tau^2, beta  (child draw)
        update_tau2_(rng);        // tau^2 | beta, w
        update_beta_(rng);        // beta | tau^2, w
        level_shift_(rng);        // PX intercept <-> spatial-level recentering (mixing)

        sync_current_();
        if (keep_history_) history_buf_.push_back(current_);
    }

    const arma::vec& current() const override { return current_; }

    void set_current(const arma::vec& theta) override {
        if (theta.n_elem != dim())
            throw std::invalid_argument("nngp_gaussian_gibbs_block '" + cfg_.name +
                                        "': set_current length must equal p+n+3");
        beta_   = theta.subvec(0, p_ - 1);
        tau2_   = theta[p_];
        sigma2_ = theta[p_ + 1];
        phi_    = theta[p_ + 2];
        w_      = theta.subvec(p_ + 3, p_ + 3 + n_ - 1);
        if (!(tau2_ > 0.0) || !(sigma2_ > 0.0))
            throw std::invalid_argument("nngp_gaussian_gibbs_block: set_current tau2,sigma2 must be > 0");
        child_w_->set_current(w_);
        cached_phi_ = std::numeric_limits<double>::quiet_NaN();  // force BF refresh
        sync_current_();
    }

    const std::string& name() const noexcept override { return cfg_.name; }
    std::size_t dim() const noexcept override { return p_ + n_ + 3; }

    state_map current_named_outputs() const override {
        state_map out;
        out.emplace(beta_key(),   beta_);
        out.emplace(tau2_key(),   arma::vec{tau2_});
        out.emplace(sigma2_key(), arma::vec{sigma2_});
        out.emplace(phi_key(),    arma::vec{phi_});
        out.emplace(w_key(),      w_);
        return out;
    }

    history_map get_history() const override {
        history_map out;
        const std::size_t nd = history_buf_.empty() ? 1 : history_buf_.size();
        auto emit = [&](std::size_t start, std::size_t width, const std::string& key) {
            arma::mat M(nd, width);
            if (history_buf_.empty()) {
                for (std::size_t j = 0; j < width; ++j) M(0, j) = current_[start + j];
            } else {
                for (std::size_t i = 0; i < nd; ++i)
                    for (std::size_t j = 0; j < width; ++j) M(i, j) = history_buf_[i][start + j];
            }
            out.emplace(key, std::move(M));
        };
        emit(0,        p_, beta_key());
        emit(p_,       1,  tau2_key());
        emit(p_ + 1,   1,  sigma2_key());
        emit(p_ + 2,   1,  phi_key());
        emit(p_ + 3,   n_, w_key());
        return out;
    }
    std::size_t history_size() const noexcept override {
        return history_buf_.empty() ? 1 : history_buf_.size();
    }
    void clear_history() override { history_buf_.clear(); }
    void set_keep_history(bool keep) override { keep_history_ = keep; }

    engine_kind_t engine_kind() const noexcept override { return engine_kind_t::MCMC; }
    // supports_readapt() defaults to false (no NUTS kernel).

    // ---- fine-grained tunable setters (Tier B) --------------------------
    void set_m(std::size_t) {
        throw std::invalid_argument("nngp_gaussian_gibbs_block::set_m: m is a "
            "structural constant fixed at construction; build a new block to change it");
    }
    void set_sigma_beta2(double v) {
        if (!(v > 0.0)) throw std::invalid_argument("set_sigma_beta2: must be > 0");
        sigma_beta2_ = v;
    }
    void set_tau_prior(double a, double b) {
        if (!(a > 0.0 && b > 0.0)) throw std::invalid_argument("set_tau_prior: a,b must be > 0");
        a_tau_ = a; b_tau_ = b;
    }
    void set_sigma_prior(double a, double b) {
        if (!(a > 0.0 && b > 0.0)) throw std::invalid_argument("set_sigma_prior: a,b must be > 0");
        a_sigma_ = a; b_sigma_ = b;
    }
    void set_phi_bounds(double lo, double hi) {
        if (!(lo > 0.0 && lo < hi)) throw std::invalid_argument("set_phi_bounds: need 0 < lo < hi");
        phi_lower_ = lo; phi_upper_ = hi;
        if (slice_width_ <= 0.0) slice_width_ = (phi_upper_ - phi_lower_) / 10.0;
        phi_ = std::min(std::max(phi_, phi_lower_), phi_upper_);
        cached_phi_ = std::numeric_limits<double>::quiet_NaN();
    }

    /// shared_data / history keys for the five sub-outputs.
    std::string beta_key()   const { return cfg_.name + "_beta"; }
    std::string tau2_key()   const { return cfg_.name + "_tau2"; }
    std::string sigma2_key() const { return cfg_.name + "_sigma2"; }
    std::string phi_key()    const { return cfg_.name + "_phi"; }
    std::string w_key()      const { return cfg_.name + "_w"; }

    // ---- read-only accessors (current natural-scale slices) -------------
    const arma::vec& beta()  const noexcept { return beta_; }
    double           tau2()  const noexcept { return tau2_; }
    double           sigma2()const noexcept { return sigma2_; }
    double           phi()   const noexcept { return phi_; }
    const arma::vec& w()     const noexcept { return w_; }
    double           phi_lower() const noexcept { return phi_lower_; }
    double           phi_upper() const noexcept { return phi_upper_; }

    /// EXPOSED FOR TESTING: the non-normalized log full conditional of phi given
    /// the current (w, sigma^2). Mutates the internal (b_i, f_i) cache to @p phi.
    double log_phi_conditional(double phi) { return log_phi_cond_(phi); }

    /// EXPOSED FOR TESTING: force the NNGP structure (neighbors + distances) to be
    /// built now (it is otherwise built lazily on the first step / draw).
    void prepare() {
        if (!have_data_)
            throw std::runtime_error("nngp_gaussian_gibbs_block: prepare before set_context");
        if (dirty_structure_) { rebuild_structure_(); dirty_structure_ = false; }
    }

    /// EXPOSED FOR TESTING: the neighbor index sets N(i) (each location's NNGP
    /// parents). Empty until the structure is built (call prepare()/step() first).
    const std::vector<arma::uvec>& neighbors() const noexcept { return nbr_; }

    /// EXPOSED FOR TESTING: draw ONLY the spatial field w from its exact Gaussian
    /// full conditional at the CURRENT (beta, tau^2, sigma^2, phi), leaving the
    /// other parameters untouched. Used by the T0 (prior-limit) / T1a (posterior)
    /// parity regimes.
    const arma::vec& draw_w_only(std::mt19937_64& rng) {
        if (!have_data_)
            throw std::runtime_error("nngp_gaussian_gibbs_block: draw_w_only before set_context");
        if (dirty_structure_) { rebuild_structure_(); dirty_structure_ = false; }
        refresh_BF_(phi_);
        update_w_(rng);
        sync_current_();
        return w_;
    }

private:
    // ===================================================================
    //  nanoflann dataset adaptor over the block's coordinate matrix.
    //  live_count_ gates the dynamic-tree constructor's auto-add (we set it
    //  to 0 so points are added incrementally in NNGP order).
    // ===================================================================
    struct coords_adaptor {
        const arma::mat*                pts  = nullptr;  // n x d original coordinates
        const std::vector<arma::uword>* perm = nullptr;  // ordered position -> original index
        std::size_t                     count = 0;       // ordered positions currently present
        inline std::size_t kdtree_get_point_count() const { return count; }
        inline double kdtree_get_pt(const std::size_t ord_idx, const std::size_t dim) const {
            return (*pts)((*perm)[ord_idx], dim);
        }
        template <class BBOX> bool kdtree_get_bbox(BBOX&) const { return false; }
    };

    // ---- one-time NNGP structure build (KD-tree neighbors + distances) ----
    void rebuild_structure_() {
        finalize_phi_domain_();   // auto-derive phi bounds / init from coords

        // (1) coordinate-sort ordering (lexicographic over coordinate columns).
        std::vector<arma::uword> perm(n_);
        for (arma::uword i = 0; i < n_; ++i) perm[i] = i;
        std::sort(perm.begin(), perm.end(), [this](arma::uword a, arma::uword b) {
            for (std::size_t dd = 0; dd < d_; ++dd) {
                if (coords_(a, dd) < coords_(b, dd)) return true;
                if (coords_(a, dd) > coords_(b, dd)) return false;
            }
            return a < b;
        });

        // (2) ordered-prefix neighbor search via the nanoflann dynamic KD-tree. The
        //     cloud is indexed in ORDERED positions (position k = location perm[k]);
        //     the live count grows as positions are added, so each query sees only
        //     the EARLIER-ordered points -- exactly N(i). count starts at 0 so the
        //     constructor's auto-add is skipped; positions are then added one at a
        //     time, with count bumped to k+1 before adding position k (nanoflann's
        //     divideTree asserts every stored index < kdtree_get_point_count()).
        coords_adaptor adaptor{ &coords_, &perm, 0 };
        using kdtree_t = nanoflann::KDTreeSingleIndexDynamicAdaptor<
            nanoflann::L2_Simple_Adaptor<double, coords_adaptor>, coords_adaptor, -1>;
        kdtree_t index(static_cast<int>(d_), adaptor,
                       nanoflann::KDTreeSingleIndexAdaptorParams(cfg_.kd_leaf_max), n_);

        nbr_.assign(n_, arma::uvec());
        std::vector<double> q(d_);
        for (arma::uword k = 0; k < n_; ++k) {
            adaptor.count = static_cast<std::size_t>(k) + 1;        // ordered position k readable
            const arma::uword i  = perm[k];
            const std::size_t kk = std::min<std::size_t>(m_, static_cast<std::size_t>(k));
            if (kk > 0) {
                for (std::size_t dd = 0; dd < d_; ++dd) q[dd] = coords_(i, dd);
                std::vector<std::size_t> pos(kk);     // ordered positions of the neighbors
                std::vector<double>      dist2(kk);
                nanoflann::KNNResultSet<double> rs(kk);
                rs.init(pos.data(), dist2.data());
                index.findNeighbors(rs, q.data());
                arma::uvec nb(kk);
                for (std::size_t j = 0; j < kk; ++j) nb[j] = perm[pos[j]];  // -> original index
                nbr_[i] = nb;
            }
            index.addPoints(static_cast<std::uint32_t>(k), static_cast<std::uint32_t>(k));
        }

        // (3) fixed pairwise distance caches (phi-independent).
        Dnn_.assign(n_, arma::mat());
        dni_.assign(n_, arma::vec());
        for (std::size_t i = 0; i < n_; ++i) {
            const arma::uvec& nb = nbr_[i];
            const std::size_t mi = nb.n_elem;
            if (mi == 0) continue;
            dni_[i].set_size(mi);
            for (std::size_t j = 0; j < mi; ++j) dni_[i][j] = dist_(i, nb[j]);
            Dnn_[i].set_size(mi, mi);
            for (std::size_t a = 0; a < mi; ++a) {
                Dnn_[i](a, a) = 0.0;
                for (std::size_t b = a + 1; b < mi; ++b) {
                    const double dd = dist_(nb[a], nb[b]);
                    Dnn_[i](a, b) = dd;
                    Dnn_[i](b, a) = dd;
                }
            }
        }

        btil_.assign(n_, arma::vec());
        ftil_.assign(n_, 1.0);
        cached_phi_ = std::numeric_limits<double>::quiet_NaN();  // force refresh
    }

    double dist_(std::size_t i, std::size_t j) const {
        double s = 0.0;
        for (std::size_t dd = 0; dd < d_; ++dd) {
            const double diff = coords_(i, dd) - coords_(j, dd);
            s += diff * diff;
        }
        return std::sqrt(s);
    }

    // Auto-derive phi bounds / slice width / init from the spatial domain when
    // the user left them at their <=0 sentinels.
    void finalize_phi_domain_() {
        if (phi_lower_ <= 0.0 || phi_upper_ <= 0.0) {
            double diag2 = 0.0;
            for (std::size_t dd = 0; dd < d_; ++dd) {
                const double lo = coords_.col(dd).min();
                const double hi = coords_.col(dd).max();
                diag2 += (hi - lo) * (hi - lo);
            }
            const double diag = std::sqrt(diag2);
            const double safe = (diag > 0.0) ? diag : 1.0;
            // effective range exp(-phi*r)=0.05 => phi ~ 3/r; range from full
            // domain (phi_lower) down to 1% of the domain (phi_upper).
            phi_lower_ = 3.0 / safe;
            phi_upper_ = 300.0 / safe;
        }
        if (slice_width_ <= 0.0) slice_width_ = (phi_upper_ - phi_lower_) / 10.0;
        if (!(phi_ > 0.0))
            phi_ = std::sqrt(phi_lower_ * phi_upper_);  // geometric mean
        phi_ = std::min(std::max(phi_, phi_lower_), phi_upper_);
    }

    // ---- (b_i, f_i) cache: recompute the unit-variance NNGP weights at phi ----
    void refresh_BF_(double phi) {
        if (phi == cached_phi_) return;  // exact hit (set after each compute)
        for (std::size_t i = 0; i < n_; ++i) {
            const arma::uvec& nb = nbr_[i];
            const std::size_t mi = nb.n_elem;
            if (mi == 0) { ftil_[i] = 1.0; btil_[i].reset(); continue; }
            arma::mat C_N = arma::exp(-phi * Dnn_[i]);       // m x m correlation (diag = 1)
            C_N.diag() += cnn_jitter_;                        // numerical stability
            arma::vec c  = arma::exp(-phi * dni_[i]);         // m correlation to i
            arma::vec b  = arma::solve(C_N, c, arma::solve_opts::likely_sympd);
            double f = 1.0 - arma::dot(c, b);
            if (!(f > 0.0)) f = 1.0e-12;                      // guard tiny/negative
            btil_[i] = std::move(b);
            ftil_[i] = f;
        }
        cached_phi_ = phi;
    }

    // sum_i log f_i  and  w' P w = sum_i (w_i - b_i' w_{N(i)})^2 / f_i, at the
    // CURRENTLY cached phi. Uses the supplied field values.
    void quad_logdet_(const arma::vec& wv, double& quad, double& logdet) const {
        quad = 0.0; logdet = 0.0;
        for (std::size_t i = 0; i < n_; ++i) {
            const arma::uvec& nb = nbr_[i];
            double e = wv[i];
            if (nb.n_elem > 0) e -= arma::dot(btil_[i], wv.elem(nb));
            quad   += (e * e) / ftil_[i];
            logdet += std::log(ftil_[i]);
        }
    }

    double log_phi_cond_(double phi) {
        if (phi <= phi_lower_ || phi >= phi_upper_)
            return -std::numeric_limits<double>::infinity();
        refresh_BF_(phi);
        double quad, logdet;
        quad_logdet_(w_, quad, logdet);
        return -0.5 * (logdet + quad / sigma2_);
    }

    // sigma^2-MARGINALIZED (collapsed) log full conditional of phi. Integrating the
    // IG(a_sigma,b_sigma) prior out of N(w; 0, sigma^2 P(phi)^{-1}) gives
    //   g(phi) = -0.5 log|F(phi)| - (n/2 + a_sigma) log(b_sigma + 0.5 w'P(phi)w),
    // with log|F| = sum_i log f_i = logdet and w'Pw = quad. This is the phi half of
    // the joint (sigma^2, phi) block update (sigma^2 | phi, w follows conjugately),
    // which breaks the sigma^2 <-> phi confounding under strong correlation.
    double log_phi_collapsed_(double phi) {
        if (phi <= phi_lower_ || phi >= phi_upper_)
            return -std::numeric_limits<double>::infinity();
        refresh_BF_(phi);
        double quad, logdet;
        quad_logdet_(w_, quad, logdet);
        return -0.5 * logdet
               - (0.5 * static_cast<double>(n_) + a_sigma_)
                 * std::log(b_sigma_ + 0.5 * quad);
    }

    // ---- per-parameter updates ------------------------------------------

    // JUSTIFICATION (Check #16): Exception 1 -- phi half of the joint (sigma^2, phi)
    // block update. phi is slice-sampled (Neal 2003) on the sigma^2-MARGINALIZED
    // (collapsed) conditional log_phi_collapsed_; the conjugate sigma^2 | phi, w draw
    // (update_sigma2_, called next in the sweep) completes the exact block draw of
    // (sigma^2, phi) | w. Collapsing sigma^2 out of the phi step breaks the
    // sigma^2 <-> phi microergodicity ridge. Tuning-free; not an inline sampler.
    void slice_update_phi_(std::mt19937_64& rng) {
        std::uniform_real_distribution<double> U(0.0, 1.0);
        const double g0   = log_phi_collapsed_(phi_);
        const double logy = g0 + std::log(U(rng) + std::numeric_limits<double>::min());

        // stepping out (clamped to the prior support)
        double L = phi_ - slice_width_ * U(rng);
        double R = L + slice_width_;
        L = std::max(L, phi_lower_);
        R = std::min(R, phi_upper_);
        int guard = 0;
        while (L > phi_lower_ && log_phi_collapsed_(L) > logy && guard++ < 64)
            L = std::max(phi_lower_, L - slice_width_);
        guard = 0;
        while (R < phi_upper_ && log_phi_collapsed_(R) > logy && guard++ < 64)
            R = std::min(phi_upper_, R + slice_width_);

        // shrinkage
        double phi_new = phi_;
        for (int it = 0; it < 200; ++it) {
            phi_new = L + U(rng) * (R - L);
            if (log_phi_collapsed_(phi_new) > logy) break;
            if (phi_new < phi_) L = phi_new; else R = phi_new;
        }
        phi_ = phi_new;
        refresh_BF_(phi_);   // guarantee the BF cache matches the accepted phi
    }

    // JUSTIFICATION (Check #16): Exception 1 -- sigma^2 has a closed-form
    // conjugate Inverse-Gamma full conditional under the Gaussian NNGP prior.
    void update_sigma2_(std::mt19937_64& rng) {
        double quad, logdet;
        quad_logdet_(w_, quad, logdet);
        const double a_post = a_sigma_ + 0.5 * static_cast<double>(n_);
        const double b_post = b_sigma_ + 0.5 * quad;
        sigma2_ = draw_inv_gamma_(a_post, b_post, rng);
    }

    // JUSTIFICATION (Check #16): Exception 1 -- w has a closed-form Gaussian full
    // conditional; the exact draw is delegated to the validated, composed child
    // gmrf_precision_block (sparse Cholesky, Rue 2001).
    void update_w_(std::mt19937_64& rng) {
        assemble_Qw_bw_();
        child_w_->step(rng);
        w_ = child_w_->current();
    }

    // JUSTIFICATION (Check #16): Exception 1 -- tau^2 has a closed-form conjugate
    // Inverse-Gamma full conditional under the Gaussian likelihood.
    void update_tau2_(std::mt19937_64& rng) {
        const arma::vec resid = y_ - X_ * beta_ - w_;
        const double ssr = arma::dot(resid, resid);
        const double a_post = a_tau_ + 0.5 * static_cast<double>(n_);
        const double b_post = b_tau_ + 0.5 * ssr;
        tau2_ = draw_inv_gamma_(a_post, b_post, rng);
    }

    // JUSTIFICATION (Check #16): Exception 1 -- beta has a closed-form conjugate
    // Gaussian full conditional under the Gaussian likelihood + Gaussian prior.
    void update_beta_(std::mt19937_64& rng) {
        arma::mat Lambda = (1.0 / tau2_) * XtX_;
        Lambda.diag() += 1.0 / sigma_beta2_;                       // + I/sigma_beta^2
        const arma::vec rhs = (1.0 / tau2_) * (X_.t() * (y_ - w_));
        const arma::mat R = arma::chol(Lambda);                    // upper: R'R = Lambda
        const arma::vec mean = arma::solve(arma::trimatu(R),
                                arma::solve(arma::trimatl(R.t()), rhs));
        arma::vec z(p_);
        std::normal_distribution<double> nd(0.0, 1.0);
        for (std::size_t k = 0; k < p_; ++k) z[k] = nd(rng);
        beta_ = mean + arma::solve(arma::trimatu(R), z);           // cov = Lambda^{-1}
    }

    // JUSTIFICATION (Check #16): Exception 1 -- parameter-expansion level-shift. An
    // auxiliary location alpha is drawn from its EXACT Gaussian full conditional and
    // applied as beta0 -= alpha, w += alpha. The likelihood (fitted values X beta + w)
    // is invariant under this shift, so it is a target-PRESERVING move that only breaks
    // the intercept <-> spatial-level confounding slowing componentwise Gibbs
    // (Gelfand-Sahu-Carlin 1995 / Liu-Wu 1999). With the unit-variance NNGP precision P
    // and 1 = ones: prec(alpha) = 1/sigma_beta^2 + sigma^{-2} 1'P1,
    //               mean(alpha) = (beta0/sigma_beta^2 - sigma^{-2} 1'Pw) / prec,
    // where 1'P1 = sum_i s_i^2/f_i, 1'Pw = sum_i s_i r_i/f_i, s_i = 1 - sum(b_i),
    // r_i = w_i - b_i' w_{N(i)} (no dense matrix needed). Requires an intercept column.
    void level_shift_(std::mt19937_64& rng) {
        if (!cfg_.use_level_shift || !has_intercept_) return;
        double A = 0.0, Bsum = 0.0;   // A = 1'P1, Bsum = 1'Pw  (unit-variance P)
        for (std::size_t i = 0; i < n_; ++i) {
            const arma::uvec& nb = nbr_[i];
            double s = 1.0, r = w_[i];
            if (nb.n_elem > 0) { s -= arma::accu(btil_[i]); r -= arma::dot(btil_[i], w_.elem(nb)); }
            const double inv_f = 1.0 / ftil_[i];
            A    += s * s * inv_f;
            Bsum += s * r * inv_f;
        }
        const double inv_sb = 1.0 / sigma_beta2_;
        const double inv_s2 = 1.0 / sigma2_;
        const double prec   = inv_sb + inv_s2 * A;
        const double mu     = (beta_[0] * inv_sb - inv_s2 * Bsum) / prec;
        std::normal_distribution<double> nd(0.0, 1.0);
        const double alpha = mu + nd(rng) / std::sqrt(prec);
        beta_[0] -= alpha;
        w_       += alpha;
    }

    // Q_w = sigma^{-2} P + tau^{-2} I,  P = sum_i (1/f_i) v_i v_i',
    //   v_i = (+1 at i; -b_i at N(i));   b_w = tau^{-2}(y - X beta).
    void assemble_Qw_bw_() {
        std::size_t nnz = 0;
        for (std::size_t i = 0; i < n_; ++i) {
            const std::size_t mi = nbr_[i].n_elem + 1;  // include i itself
            nnz += mi * mi;
        }
        arma::umat locs(2, nnz);
        arma::vec  vals(nnz);
        std::size_t t = 0;
        for (std::size_t i = 0; i < n_; ++i) {
            const arma::uvec& nb = nbr_[i];
            const std::size_t mi = nb.n_elem;
            const double inv_f = 1.0 / ftil_[i];
            // index list L = [i, nb...]; coefficient v = [1, -b_i...]
            std::vector<arma::uword> L(mi + 1);
            std::vector<double>      v(mi + 1);
            L[0] = static_cast<arma::uword>(i); v[0] = 1.0;
            for (std::size_t j = 0; j < mi; ++j) { L[j + 1] = nb[j]; v[j + 1] = -btil_[i][j]; }
            for (std::size_t a = 0; a <= mi; ++a)
                for (std::size_t b = 0; b <= mi; ++b) {
                    locs(0, t) = L[a];
                    locs(1, t) = L[b];
                    vals[t]    = inv_f * v[a] * v[b];
                    ++t;
                }
        }
        // P: add_values=true SUMS duplicate (row,col) locations (the outer-product
        //    accumulation overlaps across rows; the default ctor would throw on them).
        arma::sp_mat P(true, locs, vals, n_, n_);
        Qw_arma_ = (1.0 / sigma2_) * P;
        Qw_arma_ += (1.0 / tau2_) * arma::speye<arma::sp_mat>(n_, n_);
        bw_arma_ = (1.0 / tau2_) * (y_ - X_ * beta_);
    }

    static double draw_inv_gamma_(double a, double b, std::mt19937_64& rng) {
        // X ~ IG(a,b)  <=>  1/X ~ Gamma(shape a, rate b) = Gamma(shape a, scale 1/b).
        std::gamma_distribution<double> g(a, 1.0 / b);
        double x = g(rng);
        if (!(x > 0.0)) x = std::numeric_limits<double>::min();
        return 1.0 / x;
    }

    void sync_current_() {
        current_.set_size(p_ + n_ + 3);
        current_.subvec(0, p_ - 1) = beta_;
        current_[p_]     = tau2_;
        current_[p_ + 1] = sigma2_;
        current_[p_ + 2] = phi_;
        current_.subvec(p_ + 3, p_ + 3 + n_ - 1) = w_;
    }

    // ---- members --------------------------------------------------------
    config cfg_;
    std::size_t n_, p_, d_, m_;
    double sigma_beta2_, a_tau_, b_tau_, a_sigma_, b_sigma_;
    double phi_lower_, phi_upper_, slice_width_, cnn_jitter_;

    // state
    arma::vec beta_;
    double    tau2_   = 1.0;
    double    sigma2_ = 1.0;
    double    phi_    = 0.0;
    arma::vec w_;
    arma::vec current_;

    // data (block-owned copies)
    arma::vec y_;
    arma::mat X_;
    arma::mat coords_;
    arma::mat XtX_;
    bool have_data_       = false;
    bool dirty_structure_ = true;
    bool has_intercept_   = false;  // first design column is all-ones (enables level-shift)

    // NNGP structure (fixed; rebuilt on coords change)
    std::vector<arma::uvec> nbr_;   // neighbor indices per location
    std::vector<arma::mat>  Dnn_;   // pairwise distances within each neighbor set
    std::vector<arma::vec>  dni_;   // distances from each location to its neighbors

    // (b_i, f_i) cache at cached_phi_
    std::vector<arma::vec>  btil_;
    std::vector<double>     ftil_;
    double cached_phi_ = std::numeric_limits<double>::quiet_NaN();

    // composed child + its per-sweep inputs
    std::unique_ptr<gmrf_precision_block> child_w_;
    arma::sp_mat Qw_arma_;
    arma::vec    bw_arma_;

    // history
    std::vector<arma::vec> history_buf_;
};

} // namespace AI4BayesCode

#endif // AI4BAYESCODE_NNGP_GAUSSIAN_GIBBS_BLOCK_HPP
