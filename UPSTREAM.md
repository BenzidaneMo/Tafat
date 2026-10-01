# Tracking upstream Veyon

Tafat is a fork of [Veyon](https://github.com/veyon/veyon) that keeps
Veyon's full git history. Upstream releases are merged regularly to pick up
security fixes, bug fixes and new Qt support.

Current base: **Veyon v4.11.3**.

## Rules that keep merges cheap

  * Rebrand only what users and administrators see (product name, binaries,
    icons, installer, service, paths, URLs). Internal identifiers such as
    `VeyonCore`, `VEYON_*` macros, class and file names, feature UIDs and the
    `veyon_*.ts` translation file names stay unchanged.
  * Put new features in new plugins under `plugins/` instead of changing core
    files.
  * Keep all Veyon copyright headers and `COPYING`.

## Merging a new upstream release

```sh
git remote add upstream https://github.com/veyon/veyon.git   # once
git fetch --no-tags upstream refs/tags/vX.Y.Z:refs/tags/vX.Y.Z
git checkout -b merge-upstream-vX.Y.Z
git merge vX.Y.Z
git submodule update --init --recursive
```

Then resolve conflicts, rebuild, run the tests and update the "Current base"
line above. Never rebase or squash upstream history.
