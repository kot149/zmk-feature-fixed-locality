/* SPDX-License-Identifier: MIT */
#include <errno.h>
#include <stdint.h>

#include <drivers/behavior.h>
#include <dt-bindings/zmk/fixed_locality.h>

#if IS_ENABLED(CONFIG_ZMK_SPLIT)
#include <zmk/events/position_state_changed.h>
#endif
#if IS_ENABLED(CONFIG_ZMK_SPLIT) && IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
#include <zmk/split/central.h>
#endif

struct fixed_locality_config {
    const char *behavior_dev;
    uint32_t locality;
};

static int invoke_locally(struct zmk_behavior_binding *binding,
                          struct zmk_behavior_binding_event event, bool pressed) {
    return pressed ? behavior_keymap_binding_pressed(binding, event)
                   : behavior_keymap_binding_released(binding, event);
}

static int invoke_binding(struct zmk_behavior_binding *binding,
                          struct zmk_behavior_binding_event event, bool pressed) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    const struct fixed_locality_config *cfg = dev->config;
    struct zmk_behavior_binding forwarded_binding = {
        .behavior_dev = cfg->behavior_dev,
        .param1 = binding->param1,
        .param2 = binding->param2,
    };
    uint32_t locality = cfg->locality;
    if (locality == FIXED_LOCALITY_EVENT_SOURCE) {
#if IS_ENABLED(CONFIG_ZMK_SPLIT)
        locality = event.source == ZMK_POSITION_STATE_CHANGE_SOURCE_LOCAL
                       ? FIXED_LOCALITY_CENTRAL
                       : event.source;
#else
        locality = FIXED_LOCALITY_CENTRAL;
#endif
    }
    if (locality == FIXED_LOCALITY_CENTRAL) {
        return invoke_locally(&forwarded_binding, event, pressed);
    }
    if (locality == FIXED_LOCALITY_GLOBAL) {
        int result = 0;
#if IS_ENABLED(CONFIG_ZMK_SPLIT) && IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL) && \
    ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT > 0
        for (int source = 0; source < ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT; source++) {
            int err = zmk_split_central_invoke_behavior(source, &forwarded_binding, event, pressed);
            if (err && !result) {
                result = err;
            }
        }
#endif
        int local_result = invoke_locally(&forwarded_binding, event, pressed);
        return result ? result : local_result;
    }
#if IS_ENABLED(CONFIG_ZMK_SPLIT) && IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL) && \
    ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT > 0
    if (locality >= ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT) {
        return -EINVAL;
    }
    return zmk_split_central_invoke_behavior((uint8_t)locality, &forwarded_binding, event, pressed);
#else
    return -ENOTSUP;
#endif
}

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    return invoke_binding(binding, event, true);
}

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    return invoke_binding(binding, event, false);
}

#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
static int get_parameter_metadata(const struct device *dev,
                                  struct behavior_parameter_metadata *metadata) {
    const struct fixed_locality_config *cfg = dev->config;
    return behavior_get_parameter_metadata(zmk_behavior_get_binding(cfg->behavior_dev), metadata);
}
#endif

static const struct behavior_driver_api fixed_locality_api = {
    .locality = BEHAVIOR_LOCALITY_CENTRAL,
    .binding_pressed = on_keymap_binding_pressed,
    .binding_released = on_keymap_binding_released,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .get_parameter_metadata = get_parameter_metadata,
#endif
};

#define FIXED_LOCALITY_DEFINE(node_id)                                                          \
    BUILD_ASSERT(DT_PROP_LEN(node_id, bindings) == 1, "Exactly one behavior is required");        \
    BUILD_ASSERT(DT_PROP(node_id, locality) < UINT8_MAX ||                                       \
                     DT_PROP(node_id, locality) == FIXED_LOCALITY_CENTRAL ||                     \
                     DT_PROP(node_id, locality) == FIXED_LOCALITY_EVENT_SOURCE ||                \
                     DT_PROP(node_id, locality) == FIXED_LOCALITY_GLOBAL,                        \
                 "Invalid fixed locality");                                                    \
    static const struct fixed_locality_config config_##node_id = {                             \
        .behavior_dev = DEVICE_DT_NAME(DT_PHANDLE(node_id, bindings)),                           \
        .locality = DT_PROP(node_id, locality),                                                 \
    };                                                                                        \
    BEHAVIOR_DT_DEFINE(node_id, NULL, NULL, NULL, &config_##node_id, POST_KERNEL,                 \
                       CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &fixed_locality_api);

DT_FOREACH_STATUS_OKAY(zmk_behavior_fixed_locality, FIXED_LOCALITY_DEFINE)
DT_FOREACH_STATUS_OKAY(zmk_behavior_fixed_locality_one_param, FIXED_LOCALITY_DEFINE)
DT_FOREACH_STATUS_OKAY(zmk_behavior_fixed_locality_two_param, FIXED_LOCALITY_DEFINE)
