# Build

The main build entrypoint in the repository is `Makefile`. It uses Conan and
CMake presets generated under `target/<profile>/<build-type>`.

```sh
make build
make test
```

Useful variables:

```sh
BUILD_TYPE=Debug        # Debug, Release, RelWithDebInfo
PROFILE=armv8-macos    # see conan/*.profile
ASAN=ON                # also TSAN=ON, UBSAN=ON
TESTS=ON
```

Successful builds copy binaries to `output/`.

## Common Targets

```sh
make deps
make configure
make build
make test
make check-format
make apply-format
make check-tidy
```

## Profiles

Conan profiles live in `conan/`.

```text
armv8-macos.profile
amd64-linux.profile
armv8-linux.docker.profile
amd64-linux.docker.profile
```

## Docker Build Helpers

The Makefile also contains helpers for Linux builds through the Ubuntu 20.04
builder image:

```sh
make build-docker-linux-builder
make build-docker-linux-RelWithDebInfo
```
