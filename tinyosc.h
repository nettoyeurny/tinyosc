/*
 * Copyright 2013 Google Inc. All Rights Reserved.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy
 * of the License at
 * 
 * http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 */

#ifndef __TINYOSC_H__
#define __TINYOSC_H__

#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>

typedef struct {
  int32_t size;
  char *data;
} osc_packet;

int osc_pack_message(osc_packet *packet, int capacity,
    const char *address, const char *types, ...);

int osc_unpack_message(const osc_packet *packet,
    const char *address, const char *types, ...);

int osc_is_bundle(const osc_packet *packet);

int osc_make_bundle(osc_packet *bundle, int capacity, uint64_t time);

int osc_time_from_bundle(const osc_packet *bundle, uint64_t *time);

int osc_add_packet_to_bundle(
    osc_packet *bundle, int capacity, const osc_packet *packet);

int osc_next_packet_from_bundle(const osc_packet *bundle, osc_packet *current);

#endif
