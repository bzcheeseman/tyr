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

#ifndef TYR_BINDGENIMPL_HPP
#define TYR_BINDGENIMPL_HPP

#include <llvm/ADT/ArrayRef.h>
#include <llvm/Support/raw_ostream.h>
#include <ostream>

namespace llvm {
class Module;
class Function;
class Type;
class StructType;
} // namespace llvm

namespace tyr {
class Generator {
public:
  virtual ~Generator() = default;

  virtual void
  setupOpaqueTypes(llvm::ArrayRef<llvm::StructType *> struct_tys) = 0;
  virtual std::string getFunctionProto(const llvm::Function &f) = 0;
  virtual std::string convertType(const llvm::Type *t) = 0;
  virtual std::string getHeader() = 0;
  virtual std::string getTypeWrappers() = 0;
  virtual std::string getFooter() = 0;

  void setModuleName(std::string module_name);

protected:
  std::string m_module_name_;
};

class Binding {
public:
  void setGenerator(Generator *gen);
  void setModule(llvm::Module *m);

  std::string assembleBinding(bool funcsOnly = false) const;

private:
  Generator *m_generator_;
  llvm::Module *m_module_;
};

llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const Binding &b);
std::ostream &operator<<(std::ostream &os, const Binding &b);
} // namespace tyr

#endif // TYR_BINDGENIMPL_HPP
