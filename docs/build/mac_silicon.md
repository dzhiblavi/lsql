# Build on macOS (Apple Silicon)

Install Xcode Command Line Tools (or Xcode) and [Homebrew](https://brew.sh/).

```sh
xcode-select --install
```

Clone the repository and build:

```sh
git clone https://github.com/dzhiblavi/lsql.git
cd lsql
brew bundle install --file=build/macos/Brewfile --no-upgrade
make PROFILE=armv8-macos BUILD_TYPE=Release build
./output/lsql-Release --version
```

Run the tests:

```sh
make PROFILE=armv8-macos BUILD_TYPE=Release test
```
