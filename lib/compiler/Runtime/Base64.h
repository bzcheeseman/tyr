//
// Created by Aman LaChapelle on 2019-03-24.
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


#ifndef TYR_BASE64_H
#define TYR_BASE64_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stdint.h>

typedef uint8_t *(*serializer_fn)(void *);
typedef void *(*deserializer_fn)(uint8_t *);

uint8_t *tyr_serialize_to_base64(serializer_fn s, void *tyr_struct_ptr);

void *tyr_deserialize_from_base64(deserializer_fn d, const uint8_t *b64_serialized_object);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif //TYR_BASE64_H
