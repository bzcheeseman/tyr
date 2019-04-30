//
// Created by Aman LaChapelle on 2019-02-13.
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

#include "CCodegenPass.hpp"

#include "IR.hpp"
#include "Module.hpp"

#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/raw_ostream.h>

namespace {
llvm::raw_ostream &operator<<(llvm::raw_ostream &out, const llvm::Type *Ty) {
  if (Ty->isIntegerTy()) {
    uint32_t bitWidth = Ty->getIntegerBitWidth();
    if (bitWidth == 1) {
      out << "bool ";
    } else if (bitWidth <= 8) {
      out << "uint8_t ";
    } else if (bitWidth <= 16) {
      out << "uint16_t ";
    } else if (bitWidth <= 32) {
      out << "uint32_t ";
    } else if (bitWidth <= 64) {
      out << "uint64_t ";
    } else {
      return out;
    }
  } else if (Ty->isFloatingPointTy()) {
    if (Ty->isFloatTy()) {
      out << "float ";
    } else if (Ty->isDoubleTy()) {
      out << "double ";
    }
  } else if (Ty->isPointerTy() && Ty->getPointerElementType()->isStructTy()) {
    out << Ty->getPointerElementType()->getStructName() << "_t *";
  } else if (Ty->isPointerTy()) {
    return out << Ty->getPointerElementType() << "*";
  }

  return out;
}
} // namespace

tyr::pass::CCodegenPass::CCodegenPass(const llvm::StringRef OutputDir,
                                      uint32_t RTOptions)
    : m_output_dir_(OutputDir), m_rt_options_(RTOptions) {}

std::string tyr::pass::CCodegenPass::getName() { return "CCodegenPass"; }

bool tyr::pass::CCodegenPass::runOnModule(tyr::Module &m) {
  using ::operator<<; // unclear why this using-declaration is necessary, but OK

  // Set up the file we're writing to
  llvm::SmallVector<char, 100> path{m_output_dir_.begin(), m_output_dir_.end()};
  llvm::sys::path::append(path, m.getModule()->getName() + ".h");
  llvm::sys::fs::make_absolute(path);

  const std::string Filename{path.begin(), path.end()};
  std::error_code EC;
  llvm::raw_fd_ostream out(Filename, EC, llvm::sys::fs::F_None);

  out << "#ifndef TYR_AUTOGENERATED_MODULE_" << m.getModule()->getName().upper()
      << "_H\n";
  out << "#define TYR_AUTOGENERATED_MODULE_" << m.getModule()->getName().upper()
      << "_H\n";
  out << "/*\n"
         "  "
         "====================================================================="
         "=====\n"
         "  ============= Autogenerated by the tyr compiler, DO NOT EDIT "
         "=============\n"
         "  "
         "====================================================================="
         "=====\n"
         "*/\n\n";

  // extern C guard
  out << "#ifdef __cplusplus\n"
         "extern \"C\" {\n"
         "#endif // __cplusplus\n\n";

  out << "#include <stdbool.h>\n"
         "#include <stdint.h>\n";

  if (rt::isFileEnabled(m_rt_options_)) {
    // Link FileHelper
    out << "#include <tyr/rt/FileHelper.h>\n";
  }

  if (rt::isB64Enabled(m_rt_options_)) {
    // Link Base64
    out << "#include <tyr/rt/Base64.h>\n";
  }

  out << "\n";

  // Iterate over the structs and create the typedefs
  for (const auto &s : m.getStructs()) {
    out << "typedef struct " << s.first() << " " << s.first() << "_t;\n";
  }

  out << "\n\n";

  for (const auto &s : m.getStructs()) {
    const std::string PtrName = std::string(s.first()) + "_t *";

    llvm::SmallVector<ir::Field *, 8> ConstructorFields;
    for (auto &f : s.second->getFields()) {
      if (f->isCount) {
        continue;
      }
      out << "bool get_" << s.first() << "_" << f->name << "(" << PtrName
          << "struct_ptr, " << f->type->getPointerTo(0) << f->name << ");\n";

      if (f->isMutable) {
        if (f->isRepeated) {
          out << "bool set_" << s.first() << "_" << f->name << "(" << PtrName
              << "struct_ptr, " << f->type << f->name << ", uint64_t "
              << f->name << "_count);\n";
        } else {
          out << "bool set_" << s.first() << "_" << f->name << "(" << PtrName
              << "struct_ptr, " << f->type << f->name << ");\n";
        }
      }

      if (f->isRepeated) {
        out << "bool get_" << s.first() << "_" << f->name << "_item(" << PtrName
            << "struct_ptr, uint64_t idx, " << f->type << f->name
            << "_item);\n";
        if (f->isMutable) {
          out << "bool set_" << s.first() << "_" << f->name << "_item("
              << PtrName << "struct_ptr, uint64_t idx, "
              << f->type->getPointerElementType() << f->name << "_item);\n";
        }
        out << "bool get_" << s.first() << "_" << f->name << "_count("
            << PtrName << "struct_ptr, uint64_t *count);\n";
      }

      if (!f->isMutable) {
        ConstructorFields.push_back(f.get());
      }
    }

    // Constructor
    out << PtrName << " create_" << s.first() << "(";
    if (!ConstructorFields.empty()) {
      for (auto cf = ConstructorFields.begin(),
                end = ConstructorFields.end() - 1;
           cf != end; ++cf) {
        if ((*cf)->isRepeated) {
          out << "uint64_t " << (*cf)->name << "_count, ";
        }

        out << (*cf)->type << (*cf)->name << ", ";
      }
      auto lastField = ConstructorFields.rbegin();
      if ((*lastField)->isRepeated) {
        out << "uint64_t " << (*lastField)->name << "_count, ";
      }
      out << (*lastField)->type << (*lastField)->name;
    }
    out << ");\n";

    // Destructor
    out << "void destroy_" << s.first() << "(" << PtrName << "struct_ptr);\n";

    out << "typedef void *" << s.first() << "_ptr;\n";
    // Serializer
    out << "uint8_t *serialize_" << s.first() << "(" << s.first()
        << "_ptr struct_ptr);\n";

    // Deserializer
    out << s.first() << "_ptr deserialize_" << s.first()
        << "(uint8_t *serialized_struct);\n\n";
  }

  out << "#ifdef __cplusplus\n";
  out << "}\n";
  out << "#endif // __cplusplus\n\n";

  out << "#endif // TYR_AUTOGENERATED_MODULE_"
      << m.getModule()->getName().upper() << "_H\n";

  out.flush();
  out.close();

  return true;
}

tyr::ir::Pass::Ptr
tyr::pass::createCCodegenPass(const llvm::StringRef OutputDir,
                              uint32_t RTOptions) {
  return llvm::make_unique<tyr::pass::CCodegenPass>(OutputDir, RTOptions);
}
