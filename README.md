# tyr
[![Build Status](https://travis-ci.org/bzcheeseman/tyr.svg?branch=master)](https://travis-ci.org/bzcheeseman/tyr)

tyr is a data structure compiler. It takes declarations like this
```
struct path {
  mutable repeated float x
  mutable repeated float y
  int32 idx
}
```
and compiles them into functions like this
```c
typedef struct path path_t;

bool get_path_idx(path_t *struct_ptr, uint32_t *idx);
bool get_path_x(path_t *struct_ptr, float **x);
bool set_path_x(path_t *struct_ptr, float *x, uint64_t x_count);
bool get_path_x_item(path_t *struct_ptr, uint64_t idx, float *x_item);
bool set_path_x_item(path_t *struct_ptr, uint64_t idx, float x_item);
bool get_path_x_count(path_t *struct_ptr, uint64_t *count);
bool get_path_y(path_t *struct_ptr, float **y);
bool set_path_y(path_t *struct_ptr, float *y, uint64_t y_count);
bool get_path_y_item(path_t *struct_ptr, uint64_t idx, float *y_item);
bool set_path_y_item(path_t *struct_ptr, uint64_t idx, float y_item);
bool get_path_y_count(path_t *struct_ptr, uint64_t *count);
path_t * create_path(uint32_t idx);
void destroy_path(path_t *struct_ptr);
typedef void *path_ptr;
uint8_t *serialize_path(path_ptr struct_ptr);
path_ptr deserialize_path(uint8_t *serialized_struct);
```
in the form of either an LLVM bitcode file or an object file. It also generates bindings 
for using the generated  object in one of the supported languages. Currently, we support 
C by generating a header file, and rust by generating a C header and then using the `bindgen`
library to generate the rust bindings. You can specify which language(s) to generate bindings for with 
the `-bind-lang` command line flag. Accepted values are `c` or `rust`. You can also generate bindings 
for multiple languages by passing multiple `bind-lang` flags.

Most tyr generated function returns `true` on success and `false` on error. The getters return by 
reference to accommodate this pattern. If the function returns a pointer, it will be NULL on failure.

## Usage
Use `tyr -help` to show all the available options. `tyr` uses an LLVM backend so all the LLVM-supported target triples
are supported. Examples for common cases follow.

### Native
```bash
tyr <filename> -bind-lang=c -base64 -out-dir=/path/to/out/dir
```

### Docker (build + run)
```bash
cd /path/to/tyr
docker build . -t tyrc -f docker/Dockerfile
export TT=$(llvm-config --host-target)  # You have to specify the target triple, because otherwise it'll be compiled for alpine linux (the dockerfile OS)
docker run -v $(pwd):/opt/local tyrc:latest bash -c "tyr /opt/local/<filename> -bind-lang=c -target-triple=${TT} -file-utils -out-dir=/opt/local"
```


## Why use tyr?

### tyr is *small*
tyr compiles to LLVM IR directly, or even just an object file. This means that its output
can be very small, depending on the size of your struct. It also doesn't use any external libraries 
to do any part of its work, in fact, the only non-generated functions it calls are `malloc`,
`realloc` and `free`; and even those can be overridden at runtime with command line flags.
This means that you can ship your tyr generated code to anywhere that has a version of
those 3 functions - so pretty much any Unix system at least.

### tyr includes a support runtime
tyr also ships with a small runtime of supporting libraries. These are designed to be lightweight
and POSIX compliant, so they should also run anywhere that has the C standard library. They are
disabled by default, but can be enabled by passing in flags corresponding to the desired runtime
libs. Currently we have:

 - Base64: enables Base64 encoding according to [this rfc](https://tools.ietf.org/html/rfc4648#section-5). Enabled by passing the `-base64` command line option.
 - File Helper: helper functions for writing to and reading from files. Enabled by passing the `-file-utils` command line option.
 
These functions are included directly in the tyr generated code, which means that if you install to a system location, you can just use them
without having to link any libraries except for the C standard library.
 
### tyr is configurable
tyr is highly configurable; inspired by LLVM, pretty much anything you want to do with the tyr IR can be done with a `Pass`. Every operation after the parser is 
a pass, and adding your own operation that acts on the tyr IR is very easy. See the various codegen passes as an example.

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
that can then be compiled into a dynamic/static library or just used
as a `.o` argument to `clang` or `gcc`.

## Requirements for building tyr
```
llvm == 8.0 (brew install llvm) <- also installs clang-format
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

## Roadmap (in no particular order)
- Non-C language support
- Compression runtime helper
- Crypto runtime helper
- Function serialization?
- Bit-accurate structs as packed ints
- Add map support
- Clean up examples/integration tests
