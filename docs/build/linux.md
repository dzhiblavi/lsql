# Build on Linux

Install Git, Make, and Docker. Start Docker, then clone the repository:

```sh
git clone https://github.com/dzhiblavi/lsql.git
cd lsql
```

For x86-64:

```sh
make build-docker-linux-builder
make PROFILE=amd64-linux.docker build-docker-linux-Release
./output/lsql-Release-linux-amd64 --version
```

For ARM64:

```sh
make build-docker-linux-builder
make PROFILE=armv8-linux.docker build-docker-linux-Release
./output/lsql-Release-linux-armv8 --version
```

Both builds run the tests.
