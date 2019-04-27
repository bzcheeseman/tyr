//
// Created by Aman LaChapelle on 2018-12-12.
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

#ifndef TYR_FILEHELPER_H
#define TYR_FILEHELPER_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stdbool.h>
#include <stdint.h>

typedef uint8_t *(*serializer_fn)(void *);
typedef void *(*deserializer_fn)(uint8_t *);

/**
 * Serializes the tyr struct and stores it into a file. Does NOT free the memory
 * associated with the struct, if that is desired, the caller should call
 * destroy_<struct_name> on the struct pointer once this function terminates
 * successfully.
 *
 * @param filename The name of the file to write into
 * @param s The serializer function for the tyr struct
 * @param tyr_struct_ptr A pointer to the tyr struct
 * @return true on success, false on failure
 */
bool tyr_serialize_to_file(const char *filename, serializer_fn s,
                           void *tyr_struct_ptr);

/**
 * Deserializes a tyr struct from a file and returns it - allocating memory for
 * the struct with malloc. The caller is responsible for memory returned from
 * this function.
 *
 * @param filename The name of the file to read from.
 * @param d The deserializer function for the tyr struct
 * @return NULL on failure, the initialized struct on success
 */
void *tyr_deserialize_from_file(const char *filename, deserializer_fn d);

#ifdef __cplusplus
};
#endif // __cplusplus

#endif // TYR_FILEHELPER_H
