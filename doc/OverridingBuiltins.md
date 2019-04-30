## Overriding Builtins

The tyr compiler uses 3 functions from the C standard library to function. It uses
`malloc`, `realloc` and `free`. There is no reason, however, that it need be those
particular three functions. It is therefore possible to override which functions are
used for allocating, re-allocating, and free-ing memory. There are two steps to
doing so.

Let's do an example. Let's say I want to use `my_alloc`, `my_realloc` and `my_free`.
I have a file called `path.tyr`, and I want to bind it to a simple C program.

First, you must pass in the appropriate flags to the tyr compiler. You will
want to invoke tyr like this: 
```bash
tyr path.tyr -malloc-name=my_alloc -realloc-name=my_realloc -free-name=my_free -out-dir=./out
```

Then you will need to make sure you link the resulting file with a library that contains
definitions for `my_alloc`, `my_realloc` and `my_free`. With this in hand, all you need
to do is link the resulting library against your final object, and everything should work.

An example of this lives in the `test` directory of this project. Inside of `test/override`
is a toy example of custom allocator functions. This gets compiled to a library that
is linked to the integration test executable, as you can see in `test/CMakeLists.txt`. Then
the integration tests are run against that 'custom' allocator set for the `c_override_test`
target.