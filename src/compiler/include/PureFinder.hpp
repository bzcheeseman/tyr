//
// Created by Aman LaChapelle on 7/16/18.
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


#ifndef TYR_FINDPURE_HPP
#define TYR_FINDPURE_HPP


#include <string>
#include <map>

namespace llvm {
  class Function;
}

namespace tyr {
  namespace compile {
    class PureFinder {

      void AddFunctionIfPure(llvm::Function *f);

    private:
      // map call context (route) to function
      std::map<std::string, llvm::Function *> m_found_;
    };
  }
}


#endif //TYR_FINDPURE_HPP
