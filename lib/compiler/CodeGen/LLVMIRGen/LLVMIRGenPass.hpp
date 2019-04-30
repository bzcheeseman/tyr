//
// Created by Aman LaChapelle on 2019-01-27.
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

#ifndef TYR_CODEGENVISITOR_HPP
#define TYR_CODEGENVISITOR_HPP

#include "Module.hpp"
#include "Pass.hpp"

#include <llvm/IR/IRBuilder.h>

#include <string>

namespace llvm {
class Module;
class Value;
class Argument;
} // namespace llvm

namespace tyr {
namespace ir {
class Struct;
class Field;
} // namespace ir

namespace pass {
class LLVMIRGenPass : public ir::Pass {
public:
  LLVMIRGenPass(llvm::Module *Parent, llvm::StringMap<std::string> Builtins);

  std::string getName() override;
  bool runOnStruct(const ir::Struct &s) override;
  bool runOnField(const ir::Field &f) override;

private:
  llvm::Value *getFieldAllocSize(const ir::Field *f, llvm::Value *Struct,
                                 llvm::IRBuilder<> &builder) const;

  bool initField(const ir::Field *f, llvm::Value *Struct, llvm::Argument *Arg,
                 llvm::IRBuilder<> &builder);
  bool destroyField(const ir::Field *f, llvm::Value *Struct,
                    llvm::IRBuilder<> &builder);

  bool getGetter(const ir::Field *f) const;
  bool getItemGetter(const ir::Field *f) const;
  bool getSetter(const ir::Field *f) const;
  bool getItemSetter(const ir::Field *f) const;

  std::string getSerializerName(const ir::Field *f) const;
  std::string getDeserializerName(const ir::Field *f) const;

  bool getSerializer(const ir::Field *f) const;
  bool getDeserializer(const ir::Field *f) const;

  uint64_t getStructAllocSize(const ir::Struct *s);
  bool getConstructor(const ir::Struct *s);
  bool getDestructor(const ir::Struct *s);
  bool getSerializer(const ir::Struct *s);
  bool getDeserializer(const ir::Struct *s);

private:
  llvm::Module *m_parent_ = nullptr;
  const llvm::StringMap<std::string> m_builtin_names_;
};

ir::Pass::Ptr createLLVMIRGenPass(tyr::Module &Parent);

} // namespace pass
} // namespace tyr

#endif // TYR_CODEGENVISITOR_HPP
