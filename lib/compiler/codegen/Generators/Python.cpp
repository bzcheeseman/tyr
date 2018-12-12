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

#include "Python.hpp"

#include <llvm/IR/Function.h>

void tyr::Python::setupOpaqueTypes(
    llvm::ArrayRef<llvm::StructType *> StructTys) {
  for (auto &ty : StructTys) {
    std::string TyName = ty->getName();

    for (int i = 0; i < TyName.size() - 1; ++i) {
      if (TyName[i] == '_') {
        TyName[i + 1] += 'A' - 'a';
      }
    }
    TyName[0] += 'A' - 'a';

    std::copy_if(TyName.begin(), TyName.end(),
                 std::back_inserter(m_struct_wrappers_[ty->getName()]),
                 [](char c) { return c != '_'; });
  }
}

std::string tyr::Python::getFunctionProto(const llvm::Function &f) {
  std::string out{};
  llvm::raw_string_ostream rso(out);

  // Get the return type
  llvm::Type *RetTy = f.getReturnType();

  // Get the argument types
  std::vector<llvm::Type *> Args{};
  for (const auto &arg : f.args()) {
    Args.push_back(arg.getType());
  }

  bool HasInout = (f.getName().find("get") != llvm::StringRef::npos) ||
                  (f.getName().find("serialize") != llvm::StringRef::npos);

  // Function signature
  rso << "def " << f.getName() << "(";
  uint32_t i = 0;
  if (!Args.empty()) {
    for (auto arg_iter = Args.begin(), end = --Args.end(); arg_iter != end;
         ++arg_iter) {
      if (!(*arg_iter)->isPointerTy() || !HasInout ||
          arg_iter == Args.begin()) {
        rso << "in" << i << ": ";
        rso << convertType(*arg_iter);
        rso << ", ";
        ++i;
      }
    }
    if (!(*Args.rbegin())->isPointerTy() || !HasInout) {
      rso << "in" << i << ": ";
      rso << convertType(*Args.rbegin());
    }
  }
  rso << "):\n";

  if (Args.size() > 1) {
    std::string ffi_defs{};

    for (int i = 1; i < Args.size(); ++i) {
      if (Args[i]->isPointerTy() && HasInout) {
        ffi_defs += "\targ" + std::to_string(i);
        ffi_defs += " = ffi.new(\"" + getCType(Args[i]) + "\")\n";
      }
    }

    rso << ffi_defs;
  }

  // Now get the function body
  // If the args are empty then it's a constructor
  if (Args.empty()) {
    rso << "\t"
        << "retval = lib." << f.getName() << "()\n";
  } else {
    rso << "\t"
        << "retval = lib." << f.getName() << "(";
    rso << "in0";

    if (Args.size() > 1) {
      rso << ", ";
      for (int i = 1; i < Args.size() - 1; ++i) {
        if (Args[i]->isPointerTy() && HasInout) {
          rso << "arg" << i << ", ";
        } else {
          rso << "in" << i << ", ";
        }
      }
      if ((*Args.rbegin())->isPointerTy() && HasInout) {
        rso << "arg" << Args.size() - 1;
      } else {
        rso << "in" << Args.size() - 1;
      }
    }
    rso << ")\n";
  }

  rso << "\treturn retval";

  if (Args.size() > 1) {
    for (int i = 1; i < Args.size() - 1; ++i) {
      if (Args[i]->isPointerTy() && HasInout) {
        rso << ", arg" + std::to_string(i) << "[0]";
      }
    }
    if ((*Args.rbegin())->isPointerTy() && HasInout) {
      rso << ", arg" << Args.size() - 1 << "[0]";
    }
  }

  rso << "\n\n\n";

  return out;
}

std::string tyr::Python::convertType(const llvm::Type *t) {
  switch (t->getTypeID()) {
  case llvm::Type::VoidTyID:
    return "None";
  case llvm::Type::FloatTyID:
  case llvm::Type::DoubleTyID:
    return "float";
  case llvm::Type::IntegerTyID: {
    switch (t->getIntegerBitWidth()) {
    case 1:
      return "bool";
    default:
      return "int";
    }
  }
  case llvm::Type::PointerTyID: {
    if (t->getPointerElementType()->isStructTy()) {
      return m_struct_wrappers_.at(t->getPointerElementType()->getStructName());
    }
    return "List[" + convertType(t->getPointerElementType()) + "]";
  }
  default:
    return "";
  }
}

std::string tyr::Python::getHeader() {
  std::string out{};
  out += "\"\"\"\n"
         "====================================================================="
         "=====\n"
         "============= Autogenerated by the tyr compiler, DO NOT EDIT "
         "=============\n"
         "====================================================================="
         "=====\n"
         "\"\"\"\n";

  out += "from typing import List\n"
         "import os\n"
         "import numpy as np\n"
         "from cffi import FFI\n"
         "ffi = FFI()\n"
         "ffi.cdef(\"\"\"\n" +
         m_c_header_ +
         "\"\"\")\n"
         "lib = ffi.dlopen(os.getenv(\"TYR_COMPILED_SO_PATH\", "
         "os.path.join(os.path.dirname(os.path.realpath(__file__)), \"libtyr_" +
         m_module_name_ +
         ".so\")))\n"
         "\n\n";

  return out;
}

std::string tyr::Python::getFooter() {
  std::string out{};

  // This is needed because the cffi can't translate between pointers that are
  // just pointers and pointers that are for arrays

  out += "def ptr_to_array(ptr: ffi.CData, size: int):\n"
         "\tif ffi.typeof(ptr) == ffi.typeof(\"float *\"):\n"
         "\t\treturn np.frombuffer(ffi.buffer(ptr, size*4), np.float32)\n"
         "\telif ffi.typeof(ptr) == ffi.typeof(\"double *\"):\n"
         "\t\treturn np.frombuffer(ffi.buffer(ptr, size*8), np.float64)\n"
         "\telif ffi.typeof(ptr) == ffi.typeof(\"bool *\"):\n"
         "\t\treturn np.frombuffer(ffi.buffer(ptr, size), np.bool_)\n"
         "\telif ffi.typeof(ptr) == ffi.typeof(\"uint8_t *\"):\n"
         "\t\treturn np.frombuffer(ffi.buffer(ptr, size), np.uint8)\n"
         "\telif ffi.typeof(ptr) == ffi.typeof(\"uint16_t *\"):\n"
         "\t\treturn np.frombuffer(ffi.buffer(ptr, size*2), np.uint16)\n"
         "\telif ffi.typeof(ptr) == ffi.typeof(\"uint32_t *\"):\n"
         "\t\treturn np.frombuffer(ffi.buffer(ptr, size*4), np.uint32)\n"
         "\telif ffi.typeof(ptr) == ffi.typeof(\"uint64_t *\"):\n"
         "\t\treturn np.frombuffer(ffi.buffer(ptr, size*8), np.uint64)\n";

  return out;
}

void tyr::Python::setCHeader(std::string c_header) {
  m_c_header_ = std::move(c_header);
}

std::string tyr::Python::getTypeWrappers() {
  std::string out{};

  for (auto &entry : m_struct_wrappers_) {
    out += "class " + entry.second + ":\n";
    out += "\tdef __init__(self, data: ffi.CData):\n";
    out += "\t\tself.data = data\n\n\n";
  }

  return out;
}

std::string tyr::Python::getCType(const llvm::Type *t) {
  switch (t->getTypeID()) {
  case llvm::Type::VoidTyID:
    return "void";
  case llvm::Type::FloatTyID:
    return "float";
  case llvm::Type::DoubleTyID:
    return "double";
  case llvm::Type::IntegerTyID: {
    switch (t->getIntegerBitWidth()) {
    case 1:
      return "bool";
    case 8:
      return "uint8_t";
    case 16:
      return "uint16_t";
    case 32:
      return "uint32_t";
    case 64:
      return "uint64_t";
    default:
      return "";
    }
  }
  case llvm::Type::PointerTyID: {
    return getCType(t->getPointerElementType()) + " *";
  }
  default:
    return "";
  }
}
