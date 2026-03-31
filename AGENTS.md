# Agent Instructions

- Repo-local agent metadata lives in `REPO_META.yaml`.
- Use `REPO_META.yaml` as the local source of truth for build entrypoints, owned paths, and allowed automated change classes.
- `develop` is the integration branch for normal work.
- `master` is release-only.
- Prioritize IO correctness, source semantics, backend selection behavior, dependency safety, and platform compatibility.
