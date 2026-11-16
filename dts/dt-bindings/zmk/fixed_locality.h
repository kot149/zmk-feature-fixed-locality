/* SPDX-License-Identifier: MIT */
#pragma once

/* Peripheral source IDs are 0..254; 255 is ZMK's local-source sentinel. */
#define FIXED_LOCALITY_PERIPHERAL(id) (id)
#define FIXED_LOCALITY_CENTRAL 256
#define FIXED_LOCALITY_EVENT_SOURCE 257
#define FIXED_LOCALITY_GLOBAL 258
