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

#include "Base64.h"

#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

// We use the encoding from here: https://tools.ietf.org/html/rfc4648#section-5
// so it's URL safe
static inline uint8_t b64_index_to_char(uint32_t index) {
  // 0-25 is capital letters, 26-51 is lowercase, 52-61 is numbers, 62 is '-'
  // and 63 is '_', and I'm using 64 as '='
  return (uint8_t)(
      index + ('A' * (index < 26)) + (('a' - 26) * (index >= 26 & index < 52)) +
      (('0' - 52) * (index >= 52 & index < 62)) + (('-' - 62) * (index == 62)) +
      (('_' - 63) * (index == 63)) + (('=' - 64) * (index == 64)));
}

static inline uint32_t b64_char_to_index(uint8_t c) {
  if ((c >= 'A') && (c <= 'Z')) {
    return (uint32_t)(c - 'A');
  } else if ((c >= 'a') && (c <= 'z')) {
    return (uint32_t)(c - 'a' + 26);
  } else if ((c >= '0') && (c <= '9')) {
    return (uint32_t)(c - '0' + 52);
  } else if (c == '-') {
    return 62;
  } else if (c == '_') {
    return 63;
  } else if (c == '=') {
    return 64;
  }

  return 0;
}

uint8_t *tyr_serialize_to_base64(serializer_fn s, void *tyr_struct_ptr) {
  const uint8_t *serialized = s(tyr_struct_ptr);
  if (serialized == NULL) {
    printf("Serializing struct failed, aborting\n");
    return NULL;
  }

  const uint8_t *serialized_ptr = serialized + sizeof(uint64_t);

  // First 64 bits are the length of the whole buffer (including the first 64
  // bits)
  uint64_t serialized_len = *((uint64_t *)serialized);

  const uint64_t num_pad = (3 - (serialized_len % 3)) % 3;
  const uint64_t packed_size = serialized_len / 3 + (serialized_len % 3 != 0);
  const uint64_t b64_len = packed_size * 4;

  uint8_t *b64_str = calloc(b64_len + sizeof(uint64_t), sizeof(uint8_t));

  // Set the beginning to be the total size of the buffer
  *(uint64_t *)b64_str = b64_len;

  // Now we have a byte string, so we need to b64 encode it
  // Pack each 3 8bit item into a 32bit int, then unpack it into 6 bit indices
  size_t str_iter = 0;
  size_t b64_iter = 0;
  uint8_t *b64_ptr = b64_str + sizeof(uint64_t);
  uint32_t packed = 0;
  for (size_t i = 0; i < packed_size; ++i) {
    packed = 0;
    // Parse the chars one at a time and pack them into a uint32_t
    for (int8_t j = 2; j >= 0; --j) {
      packed |=
          ((str_iter < serialized_len) * (uint32_t)(serialized_ptr[str_iter]))
          << (8 * j);
      ++str_iter;
    }

    for (int8_t k = 3; k >= 0; --k) {
      // If we've gotten into the padding bits then use a 64 value to indicate
      // to the converter to give us a padding character
      uint32_t masked = (b64_iter >= (b64_len - num_pad))
                            ? 64
                            : ((packed & (0b111111 << (k * 6))) >> (k * 6));
      b64_ptr[b64_iter] = b64_index_to_char(masked);
      ++b64_iter;
    }
  }

  return b64_str;
}

void *tyr_deserialize_from_base64(deserializer_fn d,
                                  const uint8_t *b64_serialized_object) {
  const uint64_t b64_len = *(uint64_t *)b64_serialized_object;
  if (b64_len % 4 != 0) {
    printf("String length is not a multiple of 4, b64 decoding failed\n");
    return NULL;
  }

  const uint64_t str_len = (b64_len / 4) * 3;

  uint8_t *decoded_serialized =
      malloc(str_len * sizeof(uint8_t) + sizeof(uint64_t));
  memset(decoded_serialized, 0, str_len * sizeof(uint8_t) + sizeof(uint64_t));
  uint8_t *decoded_serialized_ptr = decoded_serialized + sizeof(uint64_t);

  uint32_t packed = 0;
  uint64_t b64_iter = 0;
  const uint8_t *b64_ptr = b64_serialized_object + sizeof(uint64_t);
  uint64_t decoded_iter = 0;
  uint8_t num_pad = 0;
  for (size_t i = 0; i < b64_len; i += 4) {
    packed = 0;
    for (int8_t j = 3; j >= 0; --j) {
      uint32_t b64_index = b64_char_to_index(b64_ptr[b64_iter]);
      if (b64_index == 64) {
        ++num_pad;
        continue;
      }
      packed |= ((b64_index != 64) * b64_index)
                << (6 * j); // Do nothing if b64_index is 0 or 64
      ++b64_iter;
    }

    for (int8_t k = 2; k >= 0; --k) {
      decoded_serialized_ptr[decoded_iter] =
          (uint8_t)((packed & (0xff << (8 * k))) >> (8 * k));
      ++decoded_iter;
    }
  }

  const uint64_t real_str_len = str_len - num_pad;
  *(uint64_t *)decoded_serialized = real_str_len;

  void *deserialized = d(decoded_serialized);
  if (deserialized == NULL) {
    printf("Deserializing struct failed, aborting\n");
    return NULL;
  }

  free(decoded_serialized);

  return deserialized;
}
