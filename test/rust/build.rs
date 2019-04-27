mod build_path_bindings;
use build_path_bindings::*;

fn main() {
    // Choose which one to use based on what flags you passed into tyr

    // If you passed -emit-llvm, use this one
    build_bindings_llvm();

    // Otherwise, use this one
    // build_bindings_object();
}