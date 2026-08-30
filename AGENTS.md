# Agent Instructions

## Repository metadata and scope

- Repo-local agent metadata lives in `REPO_META.yaml`.
- Use `REPO_META.yaml` as the local source of truth for build entrypoints, owned paths, and allowed automated change classes.
- Stop and request explicit approval when work requires a forbidden change class or expands beyond the owned paths.

## Repository workflow

- `develop` is the integration branch for normal work.
- `master` is release-only.
- Create every normal feature or fix branch, and every associated worktree, from `develop`; normal pull requests target `develop`, never `master`.
- Make the smallest reviewable change that resolves the requested issue without widening scope.
- Validate with repo-local checks first. When a public frame contract or backend behavior changes, also validate affected downstream configurations.
- Before committing and again immediately before pushing any C or C++ change, run the full-tree `clang-format` check implemented by `.github/workflows/format.yml`. Do not commit or push if it fails.
- Pull requests produced by agents must state the validation performed and any downstream or cross-repository impact.
- During branch closure, synchronize local `develop` with `origin/develop`, then remove the merged feature branch locally and remotely.

## Review focus

- Prioritize IO correctness, source semantics, backend selection behavior, dependency safety, and platform compatibility.
- Review correctness and edge cases, backward compatibility, missing tests, API and ABI consistency, performance regressions, and build or release safety.
- For C++, review ownership and lifetime, thread and exception safety, const-correctness, unnecessary copies, and frame layout or stride assumptions.
- For capture changes, preserve timestamp and sequence semantics, capture lifecycle behavior, backend fallback behavior, and logging clarity.
- Avoid trivial style-only review comments and major rewrites unless they are clearly justified by the task.

## Documentation

- Keep `Readme.md` as the general-purpose project entrypoint. Put backend-specific setup, build, Docker, and troubleshooting details in `docs/` when they outgrow a concise overview.
- When editing documentation links, verify that relative links resolve and that external links are reachable. Prefer stable absolute GitHub links for cross-repository references.
