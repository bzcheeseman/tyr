//
// Created by Aman LaChapelle on 2019-04-26.
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

#ifndef TYR_PASSES_HPP
#define TYR_PASSES_HPP

#include <llvm/ADT/StringRef.h>

namespace llvm {
class Pass;
}

namespace tyr {
llvm::Pass *createUnifyFnAttrsPass(llvm::StringRef cpu,
                                   llvm::StringRef features);
}

#endif // TYR_PASSES_HPP
