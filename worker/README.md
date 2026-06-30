# Block-submission Worker

A tiny Cloudflare Worker that powers one-click block submission on the Block
upload page. The static site cannot attach a file to a GitHub issue on its own,
so the page POSTs the zipped bundle here. The Worker holds a GitHub token (server
side only), commits the bundle to `submissions/<block>/<block>.zip`, opens a
review issue labeled `block-submission`, and returns the issue link.

## What it does per request

1. Reads the uploaded `.zip`, checks the name, size (max 5 MB), and zip header.
2. Commits it to `submissions/<block>/<block>.zip` on `main`.
3. Opens an issue titled `Block submission: <block>` linking that file.
4. Returns `{ issueUrl, issueNumber, fileUrl }` to the page.

## One-time setup

You need a free Cloudflare account and a GitHub token. The token never touches
the website.

1. **Create the GitHub token.** GitHub → Settings → Developer settings →
   Fine-grained personal access tokens → Generate new token.
   - Repository access: only `zjg540066169/AI4BayesCode-hub`.
   - Permissions: **Contents: Read and write**, **Issues: Read and write**.
   - Copy the token.

2. **Install wrangler and log in.**
   ```bash
   npm install -g wrangler
   cd worker
   wrangler login
   ```

3. **Store the token as a secret (not in any file).**
   ```bash
   wrangler secret put GITHUB_TOKEN
   # paste the fine-grained token when prompted
   ```

4. **Deploy.**
   ```bash
   wrangler deploy
   ```
   Wrangler prints the URL, e.g. `https://ai4bayescode-submit.<account>.workers.dev`.

5. **Point the site at it.** In `blocks.html`, set `SUBMIT_ENDPOINT` (top of the
   upload script) to that URL, then commit and push.

## Notes

- The `block-submission` label is created automatically on first use.
- Re-submitting the same block name updates its zip in place.
- To rotate the token: `wrangler secret put GITHUB_TOKEN` again, then redeploy.
- Submissions land in `submissions/` (pending). Accepted blocks move to
  `registry/`, which is what Contributed blocks displays.
