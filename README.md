# zmk-feature-fixed-locality

Invoke a ZMK behavior at a fixed locality, passing its arguments through unchanged.

## Installation

Add the project to your `west.yml`:

```yaml
manifest:
  remotes:
    - name: zmkfirmware
      url-base: https://github.com/zmkfirmware
    - name: kot149
      url-base: https://github.com/kot149
  projects:
    - name: zmk-feature-fixed-locality
      remot: kot149
      revision: main
```

## Usage

For an existing one-argument behavior `&my_behavior`, define a wrapper:

```dts
#include <dt-bindings/zmk/fixed_locality.h>

/ {
    behaviors {
        fixed: fixed {
            compatible = "zmk,behavior-fixed-locality-one-param";
            #binding-cells = <1>;
            locality = <FIXED_LOCALITY_CENTRAL>;
            bindings = <&my_behavior>;
        };
    };
};
```

Set `bindings` to the wrapped behavior's phandle without arguments. Match `compatible` and `#binding-cells` to its argument count:

| Compatible | `#binding-cells` |
| --- | --- |
| `zmk,behavior-fixed-locality` | 0 |
| `zmk,behavior-fixed-locality-one-param` | 1 |
| `zmk,behavior-fixed-locality-two-param` | 2 |

Set `locality` to `FIXED_LOCALITY_CENTRAL`, `FIXED_LOCALITY_EVENT_SOURCE`, `FIXED_LOCALITY_GLOBAL`, or `FIXED_LOCALITY_PERIPHERAL(n)`. This overrides the wrapped behavior's locality.

Peripheral IDs and the device acting as central may change after re-pairing or switching split transports.

## Use cases

### Reset a keyless dongle

When a keyless dongle is the split central, trigger its reset or bootloader from the keyboard:

```dts
#include <behaviors/reset.dtsi>
#include <dt-bindings/zmk/fixed_locality.h>

/ {
    behaviors {
        dongle_reset: dongle_reset {
            compatible = "zmk,behavior-fixed-locality";
            #binding-cells = <0>;
            locality = <FIXED_LOCALITY_CENTRAL>;
            bindings = <&sys_reset>;
        };

        dongle_boot: dongle_boot {
            compatible = "zmk,behavior-fixed-locality";
            #binding-cells = <0>;
            locality = <FIXED_LOCALITY_CENTRAL>;
            bindings = <&bootloader>;
        };
    };
};
```

### Control peripheral power

```dts
#include <dt-bindings/zmk/fixed_locality.h>

/ {
    behaviors {
        left_power: left_power {
            compatible = "zmk,behavior-fixed-locality-one-param";
            #binding-cells = <1>;
            locality = <FIXED_LOCALITY_PERIPHERAL(0)>;
            bindings = <&ext_power>;
        };
    };
};
```

## Tests

```bash
python3 tests/run.py
```
