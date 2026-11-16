#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define IS_ENABLED(value) (value)
#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))
#define DT_FOREACH_STATUS_OKAY(compat, fn)
#define ZMK_BEHAVIOR_OPAQUE 0

struct device {
    const void *config;
};
struct zmk_behavior_binding {
    const char *behavior_dev;
    uint32_t param1;
    uint32_t param2;
};
struct zmk_behavior_binding_event {
    int layer;
    uint32_t position;
    int64_t timestamp;
#if CONFIG_ZMK_SPLIT
    uint8_t source;
#endif
};
enum behavior_locality {
    BEHAVIOR_LOCALITY_CENTRAL,
    BEHAVIOR_LOCALITY_EVENT_SOURCE,
    BEHAVIOR_LOCALITY_GLOBAL,
};
struct behavior_parameter_value_metadata {
    const char *display_name;
    enum {
        BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        BEHAVIOR_PARAMETER_VALUE_TYPE_RANGE,
    } type;
    union {
        uint32_t value;
        struct { uint32_t min, max; } range;
    };
};
struct behavior_parameter_metadata_set {
    const struct behavior_parameter_value_metadata *param1_values;
    size_t param1_values_len;
    size_t param2_values_len;
    const struct behavior_parameter_value_metadata *param2_values;
};
struct behavior_parameter_metadata {
    const struct behavior_parameter_metadata_set *sets;
    size_t sets_len;
};
struct behavior_driver_api {
    enum behavior_locality locality;
    int (*binding_pressed)(struct zmk_behavior_binding *, struct zmk_behavior_binding_event);
    int (*binding_released)(struct zmk_behavior_binding *, struct zmk_behavior_binding_event);
    const struct behavior_parameter_metadata *parameter_metadata;
    int (*get_parameter_metadata)(const struct device *, struct behavior_parameter_metadata *);
};

const struct device *zmk_behavior_get_binding(const char *name);
int behavior_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                    struct zmk_behavior_binding_event event);
int behavior_keymap_binding_released(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event);
int behavior_get_parameter_metadata(const struct device *dev,
                                    struct behavior_parameter_metadata *metadata);
