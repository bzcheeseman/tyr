//
// Created by Aman LaChapelle on 12/1/18.
//
// tyr
// Copyright (c) 2018 Aman LaChapelle
// Full license at tyr/LICENSE.txt
//

/*
    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
 */

#include <fstream>

#include <llvm/ADT/Triple.h>
#include <llvm/Support/Path.h>

#include <BindingCodeGen/CCodegenPass.hpp>
#include <BindingCodeGen/RustCodegenPass.hpp>
#include <LLVMCodegenPass/LLVMCodegenPass.hpp>
#include <LLVMIRGen/LLVMIRGenPass.hpp>
#include <Module.hpp>
#include <ObjectCodegenPass/ObjectCodegenPass.hpp>
#include <Parser.hpp>

namespace {

#ifndef TYR_RT_BITCODE_DIR
#error "Need the tyr runtime lib file"
#endif

using namespace llvm;

enum SupportedBindingLanguages {
  kSBLC,
  kSBLRust,
};

enum RuntimeOptions {
  kEnableFileHelper = 0,
  kEnableBase64 = 1,
};

cl::OptionCategory
    tyrCompilerOptions("tyr Compiler Options",
                       "These are the options specific to the tyr compiler");

cl::opt<std::string> FileName(cl::Positional, cl::Required,
                              cl::desc("<input file>"),
                              cl::cat(tyrCompilerOptions));

cl::opt<std::string> OutputDir("out-dir",
                               cl::desc("Where to put the generated files"),
                               cl::init(""), cl::cat(tyrCompilerOptions));

cl::opt<std::string> ModuleName("module-name",
                                cl::desc("Override the module name manually"),
                                cl::init(""), cl::cat(tyrCompilerOptions),
                                cl::Hidden);

cl::bits<SupportedBindingLanguages> BindLang(
    "bind-lang", cl::desc("Available binding languages:"),
    cl::values(clEnumValN(kSBLC, "c", "Generate C bindings (header file)"),
               clEnumValN(kSBLRust, "rust",
                          "Generate rust bindings (.h file and "
                          "build_*_bindings.rs file from that)")),
    cl::OneOrMore, cl::cat(tyrCompilerOptions));

cl::bits<RuntimeOptions>
    RuntimeOpts(cl::desc("Options for configuring the runtime linking:"),
                cl::values(clEnumValN(kEnableFileHelper, "file-utils",
                                      "Enable the file utilities"),
                           clEnumValN(kEnableBase64, "base64",
                                      "Enable the base64 utilities")),
                cl::ZeroOrMore, cl::cat(tyrCompilerOptions));

cl::OptionCategory
    tyrBuiltinOptions("tyr Compiler Builtin Options",
                      "These options specify what builtin/cstdlib functions "
                      "the compiler should use.");
cl::opt<std::string>
    MallocName("malloc-name",
               cl::desc("The name to use for the 'malloc' builtin"),
               cl::init("malloc"), cl::cat(tyrBuiltinOptions));
cl::opt<std::string>
    ReallocName("realloc-name",
                cl::desc("The name to use for the 'realloc' builtin"),
                cl::init("realloc"), cl::cat(tyrBuiltinOptions));
cl::opt<std::string>
    FreeName("free-name", cl::desc("The name to use for the 'free' builtin"),
             cl::init("free"), cl::cat(tyrBuiltinOptions));

cl::opt<std::string> Target("target-triple", cl::desc("The target triple"),
                            cl::value_desc("triple"),
                            cl::init(llvm::sys::getDefaultTargetTriple()),
                            cl::cat(tyrCompilerOptions));

cl::opt<std::string> CPU("mcpu", cl::desc("Specific CPU (e.g. skylake)"),
                         cl::init(""), cl::cat(tyrCompilerOptions));

cl::opt<std::string> Features(
    "mattr",
    cl::desc("Use +feature to enable a feature, or -feature to disable"),
    cl::init(""));

cl::opt<bool>
    EmitLLVM("emit-llvm",
             cl::desc("Emit LLVM bytecode. Also enables LTO and ThinLTO."),
             cl::init(false));

const int COULD_NOT_OPEN_FILE = -1;
const int PARSING_FAILED = -2;
const int CODEGEN_FAILED = -3;
const int RT_LINKING_FAILED = -4;
const int INVALID_ARGUMENT = -5;

} // namespace

int main(int argc, char *argv[]) {
  cl::ParseCommandLineOptions(argc, argv, "The tyr struct compiler",
                              &llvm::errs());

  // check the file name
  std::string FN = FileName.getValue();
  size_t dotPos = FN.rfind(".tyr");
  if (dotPos == std::string::npos) {
    llvm::errs() << "Invalid filename\n";
    return 1;
  }

  // parse the module name
  std::string MN;
  if (!ModuleName.getValue().empty()) {
    MN = ModuleName.getValue();
  } else {
    MN = llvm::sys::path::stem(FN);
  }

  // init the generator
  llvm::LLVMContext ctx;
  tyr::Module module{MN, ctx};

  // Reset the builtin names
  module.setBuiltinName("malloc", MallocName.getValue());
  module.setBuiltinName("realloc", ReallocName.getValue());
  module.setBuiltinName("free", FreeName.getValue());
  module.finalizeBuiltins();

  // read the file
  std::ifstream in_file{FN};
  if (!in_file.is_open()) {
    llvm::errs() << "Could not open file " << FN << " for reading\n";
    return COULD_NOT_OPEN_FILE;
  }

  // Set the source file name
  module.setSourceFileName(FN);

  // parse the file
  tyr::Parser parser{module};
  if (!parser.parseFile(in_file)) {
    llvm::errs() << "Error occurred parsing file " << FN << "\n";
    return PARSING_FAILED;
  }

  // Set the target triple
  module.setTargetTriple(Target.getValue());

  // Initialize the pass manager
  tyr::PassManager PM;
  PM.registerPass(tyr::pass::createLLVMIRGenPass(module));

  // Initialize the codegen
  if (EmitLLVM.getValue()) {
    PM.registerPass(tyr::pass::createLLVMCodegenPass(
        CPU.getValue(), Features.getValue(), OutputDir.getValue()));
  } else {
    PM.registerPass(tyr::pass::createObjectCodegenPass(
        CPU.getValue(), Features.getValue(), OutputDir.getValue()));
  }

  if (BindLang.isSet(kSBLC)) {
    // Initialize the C binding
    PM.registerPass(
        tyr::pass::createCCodegenPass(OutputDir, RuntimeOpts.getBits()));
  }

  if (BindLang.isSet(kSBLRust)) {
    // Initialize the Rust binding
    PM.registerPass(
        tyr::pass::createRustCodegenPass(OutputDir, RuntimeOpts.getBits()));
  }

  // Link the runtime
  if (!module.linkRuntimeModules(TYR_RT_BITCODE_DIR, RuntimeOpts.getBits())) {
    llvm::errs() << "Error occurred linking the runtime\n";
    return RT_LINKING_FAILED;
  }

  if (!PM.runOnModule(module)) {
    llvm::errs() << "Error occurred running passes over module\n";
    return CODEGEN_FAILED;
  }

  return 0;
}
