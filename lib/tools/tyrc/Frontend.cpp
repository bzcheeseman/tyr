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

#include "CodeGen.hpp"
#include "Parser.hpp"

#include <fstream>

#include <llvm/ADT/Triple.h>

namespace {

#ifndef TYR_RT_BITCODE_FILE
#error "Need the tyr runtime lib file"
#endif

using namespace llvm;

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
                                cl::init(""), cl::cat(tyrCompilerOptions), cl::Hidden);

cl::opt<tyr::UseLang>
    BindLang("bind-lang",
             cl::desc("Set the langauge in which to emit bindings:"),
             cl::values(clEnumValN(tyr::kUseLangC, "c",
                                   "Generate C bindings (header file)"),
                        clEnumValN(tyr::kUseLangPython, "python",
                                   "Generate Python bindings (.py file)")),
             cl::cat(tyrCompilerOptions));

cl::opt<std::string> Target("target-triple", cl::desc("The target triple"),
                            cl::value_desc("triple"),
                            cl::init(llvm::sys::getDefaultTargetTriple()),
                            cl::cat(tyrCompilerOptions));

cl::opt<std::string> CPU("mcpu", cl::desc("Specific CPU (e.g. skylake)"),
                         cl::init("generic"), cl::cat(tyrCompilerOptions));

cl::opt<std::string> Features(
    "mattr",
    cl::desc("Use +feature to enable a feature, or -feature to disable"),
    cl::init(""));

cl::opt<bool> EmitLLVM("emit-llvm", cl::desc("Emit LLVM bytecode"),
                       cl::init(false));

cl::opt<bool> EmitText("emit-text", cl::desc("Emit LLVM IR as text. Ignored if not used with emit-llvm."), cl::init(false));

cl::opt<bool> LinkRuntime("link-rt", cl::desc("Link the tyr runtime into the executable."), cl::init(false));

const int COULD_NOT_OPEN_FILE = -1;
const int PARSING_FAILED = -2;
const int CODEGEN_FAILED = -3;
const int RT_LINKING_FAILED = -4;

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
    std::string MN = ModuleName.getValue();
  } else {
    MN = FN.substr(0, dotPos);
  }

  // init the generator
  tyr::CodeGen generator{MN, Target.getValue(), CPU.getValue(),
                         Features.getValue()};

  // read the file
  std::ifstream in_file{FN};
  if (!in_file.is_open()) {
    llvm::errs() << "Could not open file " << FN << " for input\n";
    return COULD_NOT_OPEN_FILE;
  }

  // parse the file
  tyr::Parser parser{generator};
  if (!parser.parseFile(in_file)) {
    llvm::errs() << "Error occurred parsing file " << FN << "\n";
    return PARSING_FAILED;
  }

  // Link the runtime
  if (LinkRuntime.getValue()) {
    if (!generator.linkOutsideModule(TYR_RT_BITCODE_FILE)) {
      llvm::errs() << "Error occurred linking the runtime\n";
      return RT_LINKING_FAILED;
    }
  }

  // emit the struct code
  if (!generator.emitStructForUse(BindLang.getValue(), EmitLLVM.getValue(), EmitText.getValue(), LinkRuntime.getValue(),
                                  OutputDir.getValue())) {
    llvm::errs() << "Error occurred emitting struct for use\n";
    return CODEGEN_FAILED;
  }

  // should I compile into .so?
  bool ShouldCompileToDylib = (BindLang.getValue() == tyr::kUseLangPython);
  if (ShouldCompileToDylib) {

    if (EmitLLVM.getValue()) {
      system(("cd " + OutputDir.getValue() + " && clang -flto -shared " + MN +
              ".bc -o libtyr_" + MN + ".so")
                 .c_str());
    } else {
      system(("cd " + OutputDir.getValue() + " && gcc -flto -shared " + MN +
              ".o -o libtyr_" + MN + ".so")
                 .c_str());
    }
  }

  return 0;
}
