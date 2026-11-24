# Copy style for ai4bayescode.com

This file lists the writing rules for the website copy, including the Blocks catalog in
`assets/blocks-data.js`. The rules follow the JASA paper "AI4BayesCode: From Natural Language
Descriptions to Validated Modular Stateful Bayesian Samplers". Apply them when you edit a page.

The catalog was generated from the library and then edited by hand. If you regenerate it,
apply these rules to the new text. Otherwise the edits are lost.

## One concept, one term

Use the term in the table for each concept, word for word, on every page. Do not vary terms for
style, and do not use one word for two different concepts.

| Concept | Term | Do not use |
|---|---|---|
| The software | AI4BayesCode, then "the system" | |
| The AI component | "the AI agent" at first mention on a page, then "the agent". The Install page may use "the agent" right after "coding agent" | the AI, the generator |
| The host application | coding agent (Claude Code, Codex, Cursor) | |
| The language model | the LLM. Its setting is the "effort level" (argument `effort`) | "the model" for the LLM, "how hard it thinks" |
| What you write | "natural-language model description" at first mention, then "model description". "Describe your model in natural language" | plain-language model, in words |
| What the system parses | the parsed model. What you meant is "the intended model" | the model you meant |
| Library units | built-in block. The two kinds are sampling blocks and helpers ("Tools and helpers") | built-in sampler, core block, building block, sampling primitive |
| Parameter groups after decomposition | modular block | modular unit, parameter group |
| Pairing modular blocks with built-in blocks | block assignment | sampling plan, blocks selected |
| Blocks shared by users | contributed block, stored in the hub registry. A block you design is a "new block" | community block, user-contributed block |
| Designing a new block | block-design flow | design flow, guided design mode |
| Checks before code generation | pre-generation validation (label "Stage 1"), with two parts: prior completion and user verification | validation before generation, sign-off |
| Checks after code generation | post-generation validation (label "Stage 2"), a three-level pipeline: syntax validation, semantic validation, runtime validation | validation after generation, layer, three-layer |
| The whole framework | two-stage validation | |
| Runtime validation parts | smoke test, R-hat and effective sample size (ESS) diagnostics, posterior predictive checks | posterior check |
| Semantic check names | the names in the paper's supplement (for example "DAG consistency") | other names for the same check |
| After a failed check | the agent revises the code and validates it again, up to `max_attempts` attempts | repairs, retries, tries again, repair budget |
| The stateful idea | stateful sampler paradigm. A sampler that follows it is a "stateful sampler" | recursively stateful coding paradigm, coding paradigm |
| Keeping state | "retains its internal state", "continues sampling without reinitialization" | keeps its own state, starts over |
| What a block conditions on | conditioning quantities | the quantities around it, outside values |
| The outer procedure | larger MCMC procedure | larger MCMC, larger sampler |
| The three functions | stateful interface: `set_current()`, `step()`, `get_current()` | the three calls |
| API functions under "Diagnostics & DAG" | diagnostic functions. `include_path` and similar are "path functions" | helpers |
| Architecture | three-tier architecture: wrapper tier, block tier, kernel tier (labels Tier A, B, C) | wrapper layer |
| Where contributed blocks install | install locations ("local" and "download"). The argument `tier` keeps its name | tiers, in prose |
| Model graph | "directed acyclic graph (DAG)" at first use on a page. The graph `predict_at()` follows is the "prediction DAG". The figure in user verification is the "DAG of model dependencies" | prediction graph, dependency graph, model graph |
| The baseline | direct LLM generation | asking the AI to write a sampler from scratch |
| Run lengths | burn-in iterations, posterior draws (arguments `n_burn`, `n_keep`) | burn-in draws, kept draws |
| One call of `step()` | iteration | sweep, step (as a count) |
| The chain's present value | current draw | latest draw, current state (as a draw) |
| Draws kept with `keep_history` | recorded draws | stored draws, retained draws |
| The samplers in `examples/` | reference example (the paper's term) | bundled example, example sampler, built-in example |
| A sampler without the stateful interface | standalone sampler | standard sampler |
| The LLM credential | API key, from your LLM provider | provider key |
| Tutorial recordings | video | screencast |
| Agent instructions | the skill, skill files | AI skill |
| People | you (the reader), users (people in general), contributors (block authors), we (the maintainers only) | |

"model" always means the Bayesian model. "layer" is not used. The API page sorts the functions
into three classes: package functions, the stateful interface, and diagnostic functions.
"sweep" stays only for a pass inside one block's algorithm, such as a Gibbs or backfitting sweep.
"level" alone names a level of post-generation validation. The LLM setting is always "effort level".
In the catalog, "a standalone nuts_block" means a nuts_block used on its own, outside `joint_nuts_block`.
Spelling is American, including "catalog".

## Software is not a person

Software, data, models, priors and chains take operational verbs: parses, decomposes, assigns,
generates, writes, compiles, checks, updates, samples, draws, returns, reports, stores, reads,
calls, uses, requires, runs.

- They do not know, understand, decide, want, need, expect, remember, notice or find.
- They do not tell, explain, suggest, answer, help, guide, hand back, walk you through or let you.
- They do not live in, sit inside, carry, feed, hold onto, keep track of, try, struggle or get stuck.
- AI4BayesCode and the agent may "ask", because the agent asks questions in the session. A function,
  file or argument does not ask.
- Address the reader as "you".

Common replacements:

- "the sampler needs X" becomes "the sampler requires X".
- "lets you do Y" becomes "you can do Y".
- "the data decide how many clusters" becomes "the number of clusters is inferred from the data".
- "the chain gets stuck" becomes "the chain mixes poorly".

## Other rules

- No em dashes, no "--" used as a dash, no semicolons in prose. Code, CSS and LaTeX are exempt.
- No "rather than", except in two approved catalog sentences in `elliptical_slice_sampling_block`
  and `joint_nuts_block`.
- No "X, not Y" slogans. Use "instead of" only for a named technical alternative or a warning. Contrast
  only against a named baseline, such as direct LLM generation.
- No fillers or marketing words: just, simply, actually, really, seamlessly, powerful, magic, out of the
  box, under the hood. Keep "only" when it states scope.
- No idioms or analogies: reach for, drop into, heavy lifting, plays well with. Standard technical
  terms stay: warm start, burn-in, smoke test, from scratch, black-box.
- No fragments in running prose. Headings, labels, captions, table cells and buttons may be noun phrases.
- Start a sentence with "This" or "These" only when a noun follows.
- Expand ESS and DAG at their first use in running prose on each page.
- Hedge claims that can fail with can, may, typically or currently.
- Do not over-compress. Two or three short full sentences read better than one long stacked sentence.
  Keep concrete nouns and precise statistical vocabulary.
- Never change code, identifiers, argument names, file names or URLs for style. Comments in code
  examples follow the same rules as prose.

## Fixed text

- The `<title>` of `index.html` and the footer BibTeX use the exact paper title.
- The homepage stays high-level, with no equations.
- The four homepage bullets under the title stay in this order: natural language to sampler, systematic
  validation, stateful sampler paradigm, extensible system. The stateful bullet never names
  `set_current()`, `step()` or `get_current()`.
