# CLAUDE.md — `web/`

Guidance for the Angular side of the repo. Angular/TypeScript **style** rules come from the
CLI-generated `opc/my-workspace/CLAUDE.md`; this file holds what is specific to *this* repo's
layout. Put durable frontend guidance here — this file is tracked, that one is not.

## `my-workspace` is generated output, not source

`web/opc/my-workspace/` is scaffolded by `web/framework/scripts/create-workspace.sh`
(`ng new … --ai-config=claude-code`) and every file in it is untracked — `.gitignore` carries
`**/my-workspace*/**`. It is recreated from scratch, so nothing authored directly inside it
survives. The tracked sources are linked in:

| tracked source | appears in the workspace as | link |
|---|---|---|
| `web/<lib>/control/src/{lib,public-api.ts,styles}` | `my-workspace/projects/jde-<lib>/src/…` | symlink |
| `web/opc/site/**` | `my-workspace/src/**` | **hard link** (`web/opc/scripts/setup.sh`) |
| `web/proto` | `my-workspace/proto`, mapped as the `jde-proto/*` tsconfig path | symlink |

Editing **through a symlink is fine** — the write lands in the tracked file under
`web/<lib>/control`, and `ng serve` picks it up without a restart.

Editing a **hard-linked** `my-workspace/src` file is not: any tool that replaces the file rather
than writing it in place (most editors, and the Edit tool) breaks the link, and the change
silently stops being tracked. Prefer editing `web/opc/site/…` directly; if you did edit the
workspace copy, re-link it with `ln -f <site-file> <workspace-file>` or re-run
`web/opc/scripts/setup.sh`.

`preserveSymlinks` is set in both `angular.json` and `tsconfig.json` and must stay — without it
tsc and esbuild resolve the library sources to their real paths outside the workspace, which
breaks compilation and the sass `node_modules` lookup.

## Libraries

Four libraries plus the `my-workspace` application. Dependencies run one way:

```
jde-spa  →  jde-framework  →  jde-access  →  jde-opc
```

with `jde-proto` (generated, `web/proto`) consumed by `jde-framework` and `jde-opc`.
`jde-proto` is a plain package, not an Angular library — it has no `dist`, is imported by bare
specifier (`jde-proto/App.FromServer`) so ng-packagr externalizes it, and must never be copied
back inside a library.

## Commands

Run `ng` from `web/opc/my-workspace`.

- **`ng test` runs Vitest**, not Karma (`@angular/build:unit-test`).
- `ng build <lib>` works, but only in dependency order — a library whose dependencies are not yet
  in `dist/` fails with `Cannot find module 'jde-spa'`. Build `jde-spa` first, or just build the
  application (`ng build my-workspace`), which compiles every library from the symlinked sources.
