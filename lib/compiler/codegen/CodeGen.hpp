//
// Created by Aman LaChapelle on 11/20/18.
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

#ifndef TYR_IRGEN_HPP
#define TYR_IRGEN_HPP

#include <map>
#include <string>

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

#include "StructGen.hpp"

namespace llvm {
class Module;
class TargetMachine;
} // namespace llvm

namespace tyr {
enum UseLang {
  kUseLangC,
  kUseLangPython,
};

class CodeGen {
public:
  CodeGen(const std::string &ModuleName, const std::string &TargetTriple,
          const std::string &CPU, const std::string &Features);

  // struct something {
  bool newStruct(const std::string &Name, bool IsPacked);

  // mutable repeated int8 bytes
  bool addField(const std::string &StructName, bool IsMutable, bool IsRepeated,
                std::string FieldType, std::string FieldName);

  // }
  bool finalizeStruct(const std::string &StructName);

  bool emitStructForUse(UseLang bind, bool EmitLLVM, bool EmitText, bool LinkRT,
                        const std::string &OutputDir);

  bool linkOutsideModule(const std::string &filename);

private:
  bool emitLLVM(const std::string &filename, bool EmitText);
  bool emitObjectCode(const std::string &filename);
  bool emitBindings(const std::string &filename, UseLang bind, bool linkRT);

  llvm::Type *parseType(std::string FieldType, bool IsRepeated);

private:
  llvm::LLVMContext m_ctx_;
  std::unique_ptr<llvm::Module> m_parent_;
  llvm::TargetMachine *m_target_machine_;

  std::map<std::string, StructGen> m_module_structs_;
};
} // namespace tyr

#endif // TYR_IRGEN_HPP
