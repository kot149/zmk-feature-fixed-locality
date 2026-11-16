#pragma once
#include <drivers/behavior.h>

int zmk_split_central_invoke_behavior(uint8_t source, struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event, bool state);
