# Self-hosted CI runner

Containerised, auto-registering GitHub Actions runner for the Jde OpcGateway
build. The image adds the clang++-22 / libc++ toolchain to
[`myoung34/github-runner`](https://github.com/myoung34/docker-github-actions-runner);
the 2 GB third-party dependency tree is **bind-mounted from the host**, not baked
in. Consumed by [`.github/workflows/ci.yml`](../workflows/ci.yml).

## Build the image

```bash
sudo docker build -t jde-ci-runner:latest .github/docker
```

The default base is `myoung34/github-runner:ubuntu-noble` (Ubuntu 24.04, glibc
2.39). The mounted host-built deps need glibc >= 2.38, so noble clears them;
`latest` (focal, 2.31) and jammy (2.35) do **not**. If a dep is later rebuilt
against a newer glibc and you hit `GLIBC_x.y not found`, bump the base to a newer
tag:

```bash
sudo docker build --build-arg BASE_IMAGE=myoung34/github-runner:<newer-tag> \
  -t jde-ci-runner:latest .github/docker
```

## Run the runner

Replace the placeholder `gha-runner` (registered from the bare image) with the
toolchain image. `ACCESS_TOKEN` is a PAT with `repo` scope (fine-grained: Actions
+ Administration read/write) so the container can fetch its own registration
tokens.

```bash
sudo docker rm -f gha-runner 2>/dev/null || true

sudo docker run -d --restart always --name gha-runner \
  -e REPO_URL="https://github.com/Jde-cpp/Public" \
  -e ACCESS_TOKEN="<PAT>" \
  -e RUNNER_NAME="jde-ci" \
  -e RUNNER_SCOPE="repo" \
  -e EPHEMERAL="1" \
  -e LABELS="linux,container,clang22" \
  -e REPO_DIR="/deps" \
  -v /home/duffyj/code/libs/install:/deps/install:ro \
  --tmpfs /mnt/ram:exec,size=8g \
  --security-opt seccomp=unconfined \
  --cap-add SYS_PTRACE \
  jde-ci-runner:latest
```

Why each non-obvious flag:

| Flag | Reason |
|------|--------|
| `-v .../install:/deps/install:ro` | Host-built Boost/protobuf/open62541/… — the presets read `$REPO_DIR/install/clang++/<Debug\|RelWithDebInfo>`. |
| `--tmpfs /mnt/ram:exec,size=8g` | Build output dir. `exec` because tmpfs is `noexec` by default and the build runs the binaries/`.so`s it produces. Wiped between jobs → clean builds. |
| `--security-opt seccomp=unconfined` | Docker's default seccomp profile blocks `io_uring_setup`; the fwk tests exercise io_uring. |
| `--cap-add SYS_PTRACE` | LeakSanitizer (debug preset builds with ASan/LSan) needs `ptrace`. |
| `EPHEMERAL=1` | Runner deregisters after each job; `--restart always` brings up a fresh registration. Reduces blast radius of a bad job. |

Follow registration with `sudo docker logs -f gha-runner` (look for
`Listening for Jobs`); the runner then appears under repo **Settings → Actions →
Runners**.

## Notes / trade-offs

- **Incremental builds:** the tmpfs `/mnt/ram` is wiped when the ephemeral
  container restarts, so every job is a clean build. To trade isolation for speed,
  swap the `--tmpfs` for a named volume (`-v jde-build:/mnt/ram`).
- **Public repo safety:** self-hosted + public repo means fork PRs must not run
  here — the workflow triggers on `push`/`workflow_dispatch` only. Also keep repo
  **Settings → Actions → General → Fork pull request workflows** at "require
  approval for all outside collaborators".
- **If mounted deps won't load** (ABI/glibc mismatch): rebuild them inside the
  image instead of mounting, using the `linux-clang-debug-repos` preset (needs the
  Boost source and UA-Nodeset too — heavier image, zero host coupling).
