# Build on macOS (Apple Silicon)

Install Xcode Command Line Tools and [Homebrew](https://brew.sh/).

```sh
xcode-select --install
```

From the repository root:

```sh
brew bundle install --file=build/macos/Brewfile --no-upgrade
make PROFILE=armv8-macos BUILD_TYPE=Release build
./output/lsql-Release --version
```

Run the tests:

```sh
make PROFILE=armv8-macos BUILD_TYPE=Release test
```
