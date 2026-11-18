## Agent skills

### Issue tracker

Issues and specs are tracked as GitHub Issues via the `gh` CLI. See `docs/agents/issue-tracker.md`.

### Triage labels

Default five-role vocabulary (needs-triage, needs-info, ready-for-agent, ready-for-human, wontfix). See `docs/agents/triage-labels.md`.

### Domain docs

Single-context: one `GLOSSARY.md` and `docs/adr/` at the repo root. See `docs/agents/domain.md`.

## Branches & Pull Requests
- Every ticket is built on its own branch off `main`, named `ticket/<issue-number>-<short-slug>` (e.g. `ticket/2-walking-skeleton`).
- Ticket work never goes directly to `main`: it lands through a pull request that references its issue (`Closes #<n>`), and the PR must be reviewed before it is merged.
- Merging a PR still needs explicit approval (see Publishing & Distribution Safety).

## Confirmation Protocol
For any destructive, irreversible, production-touching, or outside-project-directory action:
1. State the exact command/diff that will run.
2. State the scope/blast radius and the active project/environment.
3. Wait for explicit per-action confirmation — a prior general approval or task description does not count.

## Database & Production Safety
- NEVER run DELETE/DROP/TRUNCATE/UPDATE without a WHERE clause, or any migration/seed script, without showing the exact SQL and getting approval.
- NEVER target a production database unless explicitly named in the request. Treat names/IDs containing "prod"/"production"/"live" as production even if unlabeled.
- Default all DB work to local/dev/staging; ask if ambiguous.
- NEVER write/alter/delete data in a shared team database without showing a dry-run or diff first.

## Publishing & Distribution Safety
- NEVER push to main/master, tag a release, publish a package, force-push a shared branch, or merge a PR without explicit approval for that specific action.
- NEVER send data externally (webhooks, email, Slack, third-party APIs) as a side effect unless explicitly requested.
- NEVER commit, push, print, or log API keys, secrets, tokens, passwords, or credentials — in code, config, .env files, commit history, or chat output. Use .gitignore + env var references or a secret manager instead of hardcoding.
- Before any commit/push, scan the diff for likely secrets and stop if found rather than committing.
- NEVER include Claude/AI attribution, co-authorship tags, or generated-by notices in commit messages. Write as if authored solely by the developer.

## Filesystem Safety
- NEVER write outside the project directory (or designated working directory) without asking first.
- NEVER modify files outside the repo root — dotfiles, shell configs, system directories, unrelated projects.
- NEVER run rm -rf, bulk deletes, or recursive overwrites without listing what's affected and getting confirmation.
- NEVER modify .git internals, .env files, credentials files, or CI/CD configs without confirmation.
- Treat paths containing ~, /etc, /usr, /System, or other system paths as off-limits without confirmation.

<!-- BEGIN:nextjs-agent-rules -->

# This is NOT the Next.js you know

This version has breaking changes — APIs, conventions, and file structure may all differ from your training data. Read the relevant guide in `node_modules/next/dist/docs/` (resolved from this file's directory; in monorepos the `next` package may not be visible from the repo root) before writing any code. Heed deprecation notices.

This block is written and re-added by `next dev` — verify at `node_modules/next/dist/server/lib/generate-agent-files.js`. Removing it from a diff only re-creates the uncommitted change; committing it with your work keeps the tree clean.

<!-- END:nextjs-agent-rules -->
