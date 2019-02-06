# tyr
[![Build Status](https://travis-ci.org/bzcheeseman/tyr.svg?branch=master)](https://travis-ci.org/bzcheeseman/tyr)

tyr is a data structure compiler. It takes declarations like this
```
struct path {
  mutable repeated float x
  mutable repeated float y
  int64 idx
}
```
and compiles them into functions like this
```c
typedef void *path_ptr

path_ptr create_path(uint64_t); // immutable fields need to be passed into the constructor
bool get_x(path_ptr, float **);
bool set_x(path_ptr, float *, uint64_t);
bool get_x_count(path_ptr, uint64_t *);
bool get_y(path_ptr, float **);
bool set_y(path_ptr, float *, uint64_t);
bool get_y_count(path_ptr, uint64_t *);
bool get_idx(path_ptr, uint64_t *);
bool set_idx(path_ptr, uint64_t); // -> this one will always return false since the field is immutable
uint8_t *serialize_path(path_ptr, uint64_t *);
path_ptr deserialize_path(uint8_t *, uint64_t);
void destroy_path(path_ptr);
```
in the form of either an LLVM bitcode file or an object file. It also generates bindings 
for using the generated  object in one of the supported languages. Currently, we support 
`C` (by generating a header file) and `Python` via the `cffi` library.

Most tyr generated function returns `true` on success and `false` on error. The getters return by 
reference to accommodate this pattern. If the function returns a pointer, it will be NULL on failure.

## Why use tyr?
tyr compiles to LLVM IR directly, or even just an object file. This means that its output
can be very small, depending on the size of your struct - the example above produces a 1.7K 
object file and a 4.7K LLVM bitcode file. It also doesn't use any external libraries to do 
any part of its work, in fact, the only non-generated functions it calls are `malloc`, `realloc` 
and `free`. This means that you can ship your tyr generated code to anywhere that has the C 
standard library installed (so pretty much any Unix system).

tyr also ships with a small runtime of supporting libraries. These are designed to be lightweight
and POSIX compliant, so they should also run anywhere that has the C standard library. They are
disabled by default because tyr is designed to be lightweight and non-intrusive. They can be
enabled by passing the `-link-rt` flag to the tyr compiler, in which case it will compile the 
runtime functions directly into the tyr-generated code.

## How to contribute
Literally anywhere you see the need. The bullets in the Roadmap section are a great place
for the ambitious, but even just reading the code and asking questions to help me know what's not
clear so I can comment it better is helpful! Contributions in the form of 
documentation/logos/anything you can come up with are welcomed.

## Design
tyr is basically an LLVM frontend for a struct description language. It also does some high level 
code generation to make it easier to use.

## Compiler
tyr's compiler operates by reading and parsing struct declarations like the one above
and generating LLVM IR. It then passes that through a backend to produce an object file
that can then be compiled into a dynamic library (as in the Python case) or just used
as a `.o` argument to `clang` or `gcc`.

## Usage
Use `tyr -h` to show all the available options

### Native
```bash
tyr <filename> -bind-lang=[c/python] -out-dir=/path/to/out/dir
```

### Docker (build + run)
```bash
cd /path/to/tyr
docker build . -t tyrc -f docker/Dockerfile
export TT=$(llvm-config --host-target)
docker run -v $(pwd):/opt/local tyrc:latest bash -c "tyr /opt/local/<filename> -bind-lang=[c/python] -target-triple=${TT}"
```

## Requirements for building tyr
```
llvm == 7.0 (brew install llvm) <- also installs clang-format
```

## Requirements for using the Python bindings
```
cffi >= 1.11.5
numpy >= 1.10 (not exact, just use something recent-ish)
```

## Build (and install)
```
cd /path/to/tyr
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make check install
```

## Running the integration tests (C)
```bash
cd /path/to/tyr/build
make c_test && ./test/c_test
```

## Running the integration tests (Python)
```bash
cd /path/to/tyr/build
make python_test
```

## Running the integration tests (Cross language)
```bash
cd /path/to/tyr/build
make x_lang_test
```



## Roadmap (in no particular order)
- Function serialization?
- Add map support
- Code cleanup + refactoring (constant process)
- Use the new IR to generate the bindings
