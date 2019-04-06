//
// Created by Aman LaChapelle on 2019-03-21.
//
// tyr
// Copyright (c) 2019 Aman LaChapelle
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

#ifndef TYR_MODULE_HPP
#define TYR_MODULE_HPP

#include <string>
#include <unordered_map>

#include "IR.hpp"
#include "PassManager.hpp"

namespace llvm {
class Module;
class TargetMachine;
class ExecutionEngine;
} // namespace llvm

namespace tyr {
class Module {
public:
  explicit Module(const std::string &ModuleName, llvm::LLVMContext &Ctx);
  virtual ~Module();

  void setTargetTriple(const std::string &TargetTriple);
  void setSourceFileName(const std::string &SourceFilename);

  ir::Struct *getOrCreateStruct(const std::string &name);
  const std::unordered_map<std::string, ir::Struct *> &getStructs() const;
  bool linkRuntimeModules(const std::string &Directory, uint32_t options);

  bool visit(ir::Pass &p);

  llvm::Module *getModule();

  llvm::Type *parseType(llvm::StringRef FieldType, bool IsRepeated);

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const Module &m);

private:
  llvm::LLVMContext &m_ctx_;
  std::unique_ptr<llvm::Module> m_parent_;

  std::unordered_map<std::string, ir::Struct *> m_module_structs_;
};

llvm::ExecutionEngine *getExecutionEngine(llvm::Module *Parent);

namespace rt {
bool isFileEnabled(uint32_t options);
bool isB64Enabled(uint32_t options);
} // namespace rt
} // namespace tyr

#endif // TYR_MODULE_HPP
