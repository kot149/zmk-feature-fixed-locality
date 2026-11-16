/* SPDX-License-Identifier: MIT */
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "../src/behavior_fixed_locality.c"

#define CAN_SEND (CONFIG_ZMK_SPLIT && CONFIG_ZMK_SPLIT_ROLE_CENTRAL && \
                  ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT > 0)

static struct fixed_locality_config config = {.behavior_dev = "delegate"};
static const struct device wrapper = {.config = &config};
static const struct device delegate;
static uint32_t expected_param1, expected_param2;
static int local_result, remote_result, metadata_result;
static struct {
    bool remote, pressed;
    uint8_t source;
} calls[16];
static size_t call_count;

static const struct behavior_parameter_value_metadata values[] = {
    {.type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE, .value = 42},
};
static const struct behavior_parameter_metadata_set sets[] = {
    {.param1_values = values, .param1_values_len = 1,
     .param2_values = values, .param2_values_len = 1},
};

const struct device *zmk_behavior_get_binding(const char *name) {
    if (strcmp(name, "wrapper") == 0) {
        return &wrapper;
    }
    assert(strcmp(name, "delegate") == 0);
    return &delegate;
}

static void record_call(struct zmk_behavior_binding *binding,
                        struct zmk_behavior_binding_event event, bool remote,
                        uint8_t source, bool pressed) {
    assert(strcmp(binding->behavior_dev, "delegate") == 0);
    assert(binding->param1 == expected_param1 && binding->param2 == expected_param2);
    assert(event.position == 17 && event.timestamp == 1234 && event.layer == 3);
    assert(call_count < ARRAY_SIZE(calls));
    calls[call_count].remote = remote;
    calls[call_count].source = source;
    calls[call_count++].pressed = pressed;
}

int behavior_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                    struct zmk_behavior_binding_event event) {
    record_call(binding, event, false, UINT8_MAX, true);
    return local_result;
}

int behavior_keymap_binding_released(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    record_call(binding, event, false, UINT8_MAX, false);
    return local_result;
}

int zmk_split_central_invoke_behavior(uint8_t source, struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event, bool pressed) {
    record_call(binding, event, true, source, pressed);
    return remote_result;
}

int behavior_get_parameter_metadata(const struct device *dev,
                                    struct behavior_parameter_metadata *metadata) {
    assert(dev == &delegate);
    if (metadata_result) {
        return metadata_result;
    }
    *metadata = (struct behavior_parameter_metadata){.sets = sets, .sets_len = 1};
    return 0;
}

static void clear_calls(void) {
    call_count = 0;
    local_result = remote_result = 0;
}

static int invoke(uint32_t locality, uint8_t source, bool pressed) {
    config.locality = locality;
    struct zmk_behavior_binding binding = {
        .behavior_dev = "wrapper", .param1 = expected_param1, .param2 = expected_param2,
    };
    struct zmk_behavior_binding_event event = {.layer = 3, .position = 17, .timestamp = 1234};
#if CONFIG_ZMK_SPLIT
    event.source = source;
#endif
    int result = pressed ? fixed_locality_api.binding_pressed(&binding, event)
                         : fixed_locality_api.binding_released(&binding, event);
    assert(strcmp(binding.behavior_dev, "wrapper") == 0);
    assert(binding.param1 == expected_param1 && binding.param2 == expected_param2);
    return result;
}

static void expect_local(bool pressed) {
    assert(call_count == 1 && !calls[0].remote && calls[0].pressed == pressed);
}

static void expect_global(bool pressed) {
#if CAN_SEND
    assert(call_count == ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + 1);
    for (size_t i = 0; i < ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT; i++) {
        assert(calls[i].remote && calls[i].source == i && calls[i].pressed == pressed);
    }
#else
    assert(call_count == 1);
#endif
    assert(!calls[call_count - 1].remote && calls[call_count - 1].pressed == pressed);
}

static void exercise_routing(bool pressed) {
    clear_calls();
    assert(invoke(FIXED_LOCALITY_CENTRAL, 1, pressed) == 0);
    expect_local(pressed);

    clear_calls();
    assert(invoke(FIXED_LOCALITY_EVENT_SOURCE, UINT8_MAX, pressed) == 0);
    expect_local(pressed);

    clear_calls();
#if CAN_SEND
    assert(invoke(FIXED_LOCALITY_EVENT_SOURCE, 1, pressed) == 0);
    assert(call_count == 1 && calls[0].remote && calls[0].source == 1);
    assert(calls[0].pressed == pressed);
#elif !CONFIG_ZMK_SPLIT
    assert(invoke(FIXED_LOCALITY_EVENT_SOURCE, 1, pressed) == 0);
    expect_local(pressed);
#else
    assert(invoke(FIXED_LOCALITY_EVENT_SOURCE, 1, pressed) == -ENOTSUP);
    assert(call_count == 0);
#endif

#if CAN_SEND
    for (uint32_t source = 0; source < ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT; source++) {
        clear_calls();
        assert(invoke(FIXED_LOCALITY_PERIPHERAL(source), UINT8_MAX, pressed) == 0);
        assert(call_count == 1 && calls[0].remote && calls[0].source == source);
        assert(calls[0].pressed == pressed);
    }
    clear_calls();
    assert(invoke(FIXED_LOCALITY_PERIPHERAL(ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT),
                  UINT8_MAX, pressed) == -EINVAL);
    assert(call_count == 0);
    assert(invoke(FIXED_LOCALITY_EVENT_SOURCE, ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT,
                  pressed) == -EINVAL);
    assert(call_count == 0);

    remote_result = -ENOTCONN;
    assert(invoke(FIXED_LOCALITY_PERIPHERAL(0), UINT8_MAX, pressed) == -ENOTCONN);
    assert(call_count == 1 && calls[0].remote && calls[0].pressed == pressed);
#else
    clear_calls();
    assert(invoke(FIXED_LOCALITY_PERIPHERAL(0), UINT8_MAX, pressed) == -ENOTSUP);
    assert(call_count == 0);
#endif

    clear_calls();
    assert(invoke(FIXED_LOCALITY_GLOBAL, 1, pressed) == 0);
    expect_global(pressed);

#if CAN_SEND
    clear_calls();
    remote_result = -ENOTCONN;
    assert(invoke(FIXED_LOCALITY_GLOBAL, 1, pressed) == -ENOTCONN);
    expect_global(pressed);
#endif

    clear_calls();
    local_result = -EIO;
    assert(invoke(FIXED_LOCALITY_GLOBAL, 1, pressed) == -EIO);
    expect_global(pressed);

    clear_calls();
    local_result = -EIO;
    assert(invoke(FIXED_LOCALITY_CENTRAL, 1, pressed) == -EIO);
    expect_local(pressed);
}

int main(void) {
    const uint32_t params[][2] = {{0, 0}, {42, 0}, {0xDEADBEEF, 0x12345678}};
    for (size_t i = 0; i < ARRAY_SIZE(params); i++) {
        expected_param1 = params[i][0];
        expected_param2 = params[i][1];
        exercise_routing(true);
        exercise_routing(false);
    }

    struct behavior_parameter_metadata metadata;
    assert(fixed_locality_api.get_parameter_metadata(&wrapper, &metadata) == 0);
    assert(metadata.sets_len == 1);
    assert(metadata.sets[0].param1_values[0].value == 42);
    assert(metadata.sets[0].param2_values[0].value == 42);
    metadata_result = -ENOTSUP;
    assert(fixed_locality_api.get_parameter_metadata(&wrapper, &metadata) == -ENOTSUP);
    puts("PASS: all localities, transparent arguments, press/release, errors and metadata");
}
