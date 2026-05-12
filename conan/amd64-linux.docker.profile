[settings]
arch=x86_64
os=Linux
compiler=clang
compiler.version=20
compiler.cppstd=23
compiler.libcxx=libc++

[buildenv]
LDFLAGS=-L/usr/lib/llvm-20/lib -Wl,-rpath,/usr/lib/llvm-20/lib

[conf]
tools.build:compiler_executables={'c':'/usr/lib/llvm-20/bin/clang','cpp':'/usr/lib/llvm-20/bin/clang++'}
tools.build:exelinkflags=['-fuse-ld=/usr/lib/llvm-20/bin/ld.lld']
