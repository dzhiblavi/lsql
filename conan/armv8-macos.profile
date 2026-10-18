{% set llvm_prefix = subprocess.check_output(['brew', '--prefix', 'llvm@20'], text=True).strip() %}

[settings]
arch=armv8
os=Macos
compiler=clang
compiler.version=20
compiler.cppstd=23
compiler.libcxx=libc++

[buildenv]
CC={{ llvm_prefix }}/bin/clang
CXX={{ llvm_prefix }}/bin/clang++

[conf]
tools.build:compiler_executables={"c": "{{ llvm_prefix }}/bin/clang", "cpp": "{{ llvm_prefix }}/bin/clang++"}
