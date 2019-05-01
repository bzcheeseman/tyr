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

#include "FileHelper.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

bool tyr_serialize_to_file(const char *filename, serializer_fn s,
                           void *tyr_struct_ptr) {

  FILE *file = fopen(filename, "wb");
  if (file == NULL) {
    printf("Open failed with error code %d, aborting\n", errno);
    return false;
  }

  uint8_t *serialized = s(tyr_struct_ptr);
  if (serialized == NULL) {
    printf("Serializing struct failed, aborting\n");
    return false;
  }

  // First 64 bits are the length of the whole buffer
  uint64_t serialized_len = *((uint64_t *)serialized);

  size_t items_written = fwrite(serialized, sizeof(uint8_t),
                                serialized_len / sizeof(uint8_t), file);
  if (items_written != serialized_len / sizeof(uint8_t)) {
    printf("Write to file failed with error %d, aborting\n", errno);
    return false;
  }

  free(serialized);
  fclose(file);
  return true;
}

void *tyr_deserialize_from_file(const char *filename, deserializer_fn d) {
  FILE *file = fopen(filename, "rb");
  if (file == NULL) {
    printf("Opening file %s failed, aborting deserialization\n", filename);
    return NULL;
  }

  uint64_t serialized_len = 0;
  // Read in the serialized size
  fread(&serialized_len, sizeof(uint64_t), 1, file);
  // Reset to the beginning
  fseek(file, 0, SEEK_SET);

  uint8_t *serialized_struct = (uint8_t *)malloc(serialized_len);
  fread(serialized_struct, serialized_len, 1, file);

  void *deserialized = d(serialized_struct);
  if (deserialized == NULL) {
    printf("Deserializing struct failed, aborting\n");
    return NULL;
  }

  fclose(file);
  // We've copied over the serialized struct so we can get rid of the memory
  // now.
  free(serialized_struct);

  return deserialized;
}
