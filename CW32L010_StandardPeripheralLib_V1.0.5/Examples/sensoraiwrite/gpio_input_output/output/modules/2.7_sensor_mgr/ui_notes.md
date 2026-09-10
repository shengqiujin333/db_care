# UI/HMI Notes: Phase 2.7 — sensor_mgr

> The sensor_mgr module has **no display or HMI responsibilities**.

## 1. UI Context Summary

Per `meta/ui/UI_CONTEXT.md`:

> No LCD/serial-screen UI image assets were found under `inputs/ui/`, `inputs/hmi/`, `inputs/screens/`, `inputs/display/`, or `inputs/product/`.

This product is a **battery-powered temperature/humidity sensor** with 433 MHz FSK RF transmission. It has no LCD display, no serial screen, and no local user interface.

## 2. User-Facing Output

User-facing output is handled entirely through:

| Channel | Medium | Managed by | Visible to |
|---------|--------|------------|------------|
| RF transmission | 433 MHz FSK | rf_mgr (2.8) + rf_twi_driver (2.4) | Remote receiver |
| Debug console | UART1 (J3 connector) | uart_console (2.3) | Developer (via USB-serial) |

The sensor_mgr only produces a text line for the debug console (compile-time toggleable):

```
[T=25.4C H=62.3%]
```

This is a plain text line with no rendering, no page management, and no UI framework.

## 3. Firmware UI Responsibilities

The sensor_mgr has zero UI responsibilities:

- ❌ No LCD/OLED initialization
- ❌ No display buffer management
- ❌ No page/navigation state machine
- ❌ No widget rendering
- ❌ No serial-screen command generation
- ❌ No icon/bitmap handling
- ❌ No touch/button input handling

## 4. Related UI Modules

For any future display addition, the following modules would need to be consulted:

| Module | Role |
|--------|------|
| schedule (2.8) | Main cycle control, decides when to update display |
| power_manager (2.5) | Power gating for display, wake from display interaction |
| i2c_driver (2.1) | If display uses I2C (e.g., OLED SSD1306) |

However, **no such display exists in the current design**.
