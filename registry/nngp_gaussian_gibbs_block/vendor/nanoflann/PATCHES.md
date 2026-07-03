# Vendored: nanoflann

- **Upstream:** nanoflann (header-only C++ KD-tree library), https://github.com/jlblancoc/nanoflann
- **Version:** master snapshot dated 2026-06-09 (file `nanoflann.hpp`).
- **License:** BSD 2-Clause (see `COPYING`, copied verbatim). GPL-3-compatible.
- **Used for:** nearest-neighbor (KD-tree) search to build the NNGP neighbor sets
  N(i) (each location's m nearest among the *earlier*-ordered locations) and, in
  the example, to find the m nearest training locations to a new prediction site.

## Patches

**No upstream source modified.** `nanoflann.hpp` is byte-for-byte identical to
upstream; its BSD license header is intact. The library is adapted purely via a
thin wrapper (`coords_adaptor`, a nanoflann dataset adaptor over the block's
`arma::mat` of coordinates) defined inside
`nngp_gaussian_gibbs_block.hpp` — see vendor.md §1 (wrap, don't rewrite).

## Stateful-contract adaptation (vendor.md §2–§3)

- **State audit:** nanoflann's only state is the data-derived KD-tree cache built
  on the coordinates. It has **no internal RNG** (pure nearest-neighbor search,
  not sampling — same-seed determinism is automatic), no global/`static` mutable
  sampling state, and no hidden handles.
- **Cache rebuild (§3.3):** the KD-tree is rebuilt inside `set_context`/the first
  `step` whenever the coordinates change (a dirty flag). The block copies the
  coordinates into block-owned storage; it never retains a pointer into
  `shared_data`/`block_context`. The KD-tree index itself is a transient local in
  the neighbor-build routine (not a long-lived member), so there is no index
  lifetime coupling to the dataset.
