# Reusing a local C fix

For the easy direct-compare method, start at [the C catalogue](C-BUGS.md#open-a-new-pr-directly-from-the-existing-c-branch). This page covers the alternatives and follow-up details.

This guide is for another Angband contributor who chooses to propose a correction. We are not making an upstream C submission. The PR, commit, branch and patch remain usable even when our local proposal is closed unmerged.

Example: [local C proposal #1](https://github.com/diegoami/neo-angband-tracker/pull/1), commit `540f621960e674d9fde227a175eeb1d27cc02132`, base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`. It changes only `src/borg/borg-log.c`. GitHub identifies our fork's network source as angband/angband, although its immediate parent/default branch is Neo. Choose the C topic branch `pr/c-borg-map-dump-cancel`.

GitHub requires head/source write access or the applicable organization membership for a public PR from that branch. If the direct comparison does not offer creation for your account, use your own fork/branch below. [GitHub permissions reference](https://docs.github.com/en/pull-requests/how-tos/create-pull-requests/creating-a-pull-request-from-a-fork).

## Copy into your own Angband fork

Recheck current C and open/closed reports first: the correction may already exist. Start a clean topic branch from official C master in a clone of your own fork:

```sh
git clone https://github.com/YOUR_ACCOUNT/angband.git angband-fix
cd angband-fix
git remote add upstream https://github.com/angband/angband.git
git fetch upstream master
git switch -c fix/borg-map-dump upstream/master
```

Choose one copy method. Cherry-picking preserves the author and `-x` records the original commit:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git refs/pull/1/head
git cherry-pick -x 540f621960e674d9fde227a175eeb1d27cc02132
git show --stat --oneline HEAD
```

Or download the immutable commit patch and apply it with commit metadata:

```sh
curl --fail --location \
  https://github.com/diegoami/neo-angband-tracker/commit/540f621960e674d9fde227a175eeb1d27cc02132.patch \
  --output B94-map-dump-cancel.patch
git am --3way B94-map-dump-cancel.patch
```

The commit URL is pinned; a PR-wide `.patch` can change when its branch changes. If application conflicts or is empty, inspect the current implementation before continuing. The normal abort commands are `git cherry-pick --abort` and `git am --abort`.

Build/test against the current C version using [Angband's compilation guide](https://github.com/angband/angband/blob/master/docs/hacking/compiling.rst), then rerun the [native scenario and controls](audit/c-reports/B94/README.md). Our [copy receipt](audit/c-reports/B94/reuse-receipt.json) verifies both methods reproduce the proposed source tree at the recorded base; it does not verify an arbitrary future upstream revision.

When satisfied, push your branch and create your own upstream PR:

```sh
git push -u origin fix/borg-map-dump
gh pr create --repo angband/angband --base master \
  --head YOUR_ACCOUNT:fix/borg-map-dump \
  --title 'Borg: return when map dumping has no file' \
  --body-file proposal.md
```

`proposal.md` should identify the observed effect, original commit/local proposal, source/testing pins and actual tests you ran. Preserve credit to the original author and discovery source. Link `diegoami/neo-angband-tracker#13` (formerly Mandos94) as a reference; use `Fixes` only for the actual issue you intend to close in the correct repository. There is no automatic PR transfer between repositories.

## Patch/report-only proposals

Some older proposals, such as [Mandos #88](https://github.com/diegoami/mandos/pull/88), contain a nested `patches/upstream-c/*.patch` and a report. For those, extract the actual C patch, inspect it, use `git apply --check` and then `git apply`, and create your own attributed commit. Cherry-picking the wrapper commit would add the report/patch file instead of applying its C hunks. That lighting proposal still lacks demonstrated practical impact.

## Make the upstream result discoverable

Please include the catalogue ID, original local proposal and internal tracker in the new upstream report/PR. Add the official `angband/angband#N` reference to our tracker when known. Our read-only watches then flag thread/head/source changes for review. A merge triggers adoption with our changes, rebuild and verification before a tracker becomes `fixed`; applied backports are removed only when their own consumer has adopted the replacement.

References: [Git cherry-pick](https://git-scm.com/docs/git-cherry-pick), [Git format-patch/git-am](https://git-scm.com/docs/git-format-patch), [GitHub fork PR permissions and steps](https://docs.github.com/en/pull-requests/how-tos/create-pull-requests/creating-a-pull-request-from-a-fork), [comparing forks](https://docs.github.com/en/pull-requests/how-tos/commit-changes/comparing-commits).
