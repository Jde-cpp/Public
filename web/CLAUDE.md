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
| `web/<lib>/control/src/{lib,public-api.ts,styles,assets}` | `my-workspace/projects/jde-<lib>/src/…` | symlink |
| `web/opc/site/**` (except `assets`) | `my-workspace/src/**` | **hard link** (`web/opc/scripts/setup.sh`) |
| `web/opc/site/assets` | `my-workspace/src/assets` | symlink (`setup.sh`) |
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

## Help content

The `/help` section renders markdown at runtime. Each library keeps its own under `web/<lib>/control/src/assets/help/*.md`
and the site keeps `overview.md`/`about.md` under `web/opc/site/assets/help/`. `create-workspace.sh` adds one angular.json
`assets` entry per library that has an assets dir (`projects/jde-<lib>/src/assets` → `assets/jde-<lib>`) and `setup.sh` one
for the site (`src/assets` → `assets/site`), so a file is served at `assets/<jde-lib|site>/help/<file>.md`. Topics are
described by `HelpTopic` constants each library exports (`spaHelpTopics`, `frameworkHelpTopics`, …) and registered in
`app.config.ts` under the `HELP_TOPICS` multi token, in display order; a topic's `routes` patterns are what the navbar's `?`
button matches the current url against. `marked` renders the page and must only ever be imported dynamically
(`await import('marked')`): the libraries are not lazy — `app.config.ts` imports every barrel — so a static import anywhere
would hoist it into the initial bundle. `{{version}}` in the markdown is the `JDE_VERSION` constant setup.sh passes through
the builder's `define` option, read via `IEnvironment.get('version')`.

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

- **`ng test` runs Vitest**, not Karma (`@angular/build:unit-test`). It runs every project; `ng test <lib>` runs one. Library
  specs live next to the code in `web/<lib>/control/src/…` and are picked up through the symlink — but only because each
  library's test target is given `buildTarget: my-workspace:build` (by `create-workspace.sh`), which is how the unit-test
  builder inherits `preserveSymlinks`. Without it esbuild realpaths every spec out of the workspace and the target silently
  runs nothing. A library with no spec files errors with "No tests found" and fails bare `ng test`, so keep at least one.
- `ng build <lib>` works, but only in dependency order — a library whose dependencies are not yet
  in `dist/` fails with `Cannot find module 'jde-spa'`. Build `jde-spa` first, or just build the
  application (`ng build my-workspace`), which compiles every library from the symlinked sources.
