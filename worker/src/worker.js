// AI4BayesCode-hub block-submission Worker.
//
// Receives a zipped block bundle from the Block upload page, commits it to
// submissions/<block>/<block>.zip in the hub repo, and opens a review issue.
// The GitHub token lives only here (set with `wrangler secret put GITHUB_TOKEN`),
// never in the static site.

const OWNER = 'zjg540066169';
const REPO = 'AI4BayesCode-hub';
const BRANCH = 'main';
const MAX_BYTES = 5 * 1024 * 1024; // 5 MB cap on an uploaded bundle
const LABEL = 'block-submission';
const GH = 'https://api.github.com';

const CORS = {
  'Access-Control-Allow-Origin': '*',
  'Access-Control-Allow-Methods': 'POST, OPTIONS',
  'Access-Control-Allow-Headers': 'Content-Type',
};

function json(obj, status) {
  return new Response(JSON.stringify(obj), {
    status: status || 200,
    headers: { 'Content-Type': 'application/json', ...CORS },
  });
}

function gh(path, env, init) {
  init = init || {};
  return fetch(GH + path, {
    method: init.method || 'GET',
    body: init.body,
    headers: {
      'Authorization': 'Bearer ' + env.GITHUB_TOKEN,
      'Accept': 'application/vnd.github+json',
      'User-Agent': 'ai4bayescode-hub-submit',
      'X-GitHub-Api-Version': '2022-11-28',
    },
  });
}

function toBase64(bytes) {
  let bin = '';
  const chunk = 0x8000;
  for (let i = 0; i < bytes.length; i += chunk) {
    bin += String.fromCharCode.apply(null, bytes.subarray(i, i + chunk));
  }
  return btoa(bin);
}

export default {
  async fetch(request, env) {
    if (request.method === 'OPTIONS') return new Response(null, { headers: CORS });
    if (request.method !== 'POST') return json({ error: 'POST only.' }, 405);
    if (!env.GITHUB_TOKEN) return json({ error: 'Server is not configured (no token).' }, 500);

    let form;
    try { form = await request.formData(); }
    catch (e) { return json({ error: 'Expected multipart form data.' }, 400); }

    const name = (form.get('name') || '').toString().trim();
    const meta = (form.get('meta') || '').toString();
    const file = form.get('file');

    if (!/^[a-z0-9_]+_block$/.test(name)) {
      return json({ error: 'Invalid block name. Expected snake_case ending in _block.' }, 400);
    }
    if (!file || typeof file.arrayBuffer !== 'function') {
      return json({ error: 'Missing zip file.' }, 400);
    }

    const bytes = new Uint8Array(await file.arrayBuffer());
    if (bytes.length === 0) return json({ error: 'Empty file.' }, 400);
    if (bytes.length > MAX_BYTES) return json({ error: 'File too large (max 5 MB).' }, 400);
    if (!(bytes[0] === 0x50 && bytes[1] === 0x4b)) return json({ error: 'That is not a .zip file.' }, 400);

    const path = 'submissions/' + name + '/' + name + '.zip';
    const fileUrl = 'https://github.com/' + OWNER + '/' + REPO + '/blob/' + BRANCH + '/' + path;

    // If the same name was submitted before, fetch its sha so the PUT updates it.
    let sha;
    const getRes = await gh('/repos/' + OWNER + '/' + REPO + '/contents/' + path + '?ref=' + BRANCH, env);
    if (getRes.ok) { try { sha = (await getRes.json()).sha; } catch (e) {} }

    const putRes = await gh('/repos/' + OWNER + '/' + REPO + '/contents/' + path, env, {
      method: 'PUT',
      body: JSON.stringify({
        message: 'Block submission: ' + name,
        content: toBase64(bytes),
        branch: BRANCH,
        ...(sha ? { sha } : {}),
      }),
    });
    if (!putRes.ok) {
      return json({ error: 'Could not commit the bundle.', detail: await putRes.text() }, 502);
    }

    // Best-effort: make sure the label exists (ignored if it already does).
    await gh('/repos/' + OWNER + '/' + REPO + '/labels', env, {
      method: 'POST',
      body: JSON.stringify({ name: LABEL, color: '4e4ea8' }),
    });

    const body =
      'Block submission: **' + name + '**\n\n' +
      (meta ? meta + '\n\n' : '') +
      'Bundle committed to [`' + path + '`](' + fileUrl + ').\n\n' +
      '_Submitted automatically from the Block upload page._';

    const issueRes = await gh('/repos/' + OWNER + '/' + REPO + '/issues', env, {
      method: 'POST',
      body: JSON.stringify({ title: 'Block submission: ' + name, body, labels: [LABEL] }),
    });
    if (!issueRes.ok) {
      return json({ error: 'Bundle committed, but opening the issue failed.', detail: await issueRes.text(), fileUrl }, 502);
    }
    const issue = await issueRes.json();
    return json({ ok: true, issueUrl: issue.html_url, issueNumber: issue.number, fileUrl });
  },
};
