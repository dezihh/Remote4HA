#ifndef REPORT_MAP_H
#define REPORT_MAP_H

#include <stdint.h>

// Hier definierst du die HID Report Map
const uint8_t reportMap[] = {
    // Keyboard Report
    0x05, 0x01,                // Usage Page (Generic Desktop)
    0x09, 0x06,                // Usage (Keyboard)
    0xA1, 0x01,                // Collection (Application)
    0x85, 0x01,                // Report ID (1) - Keyboard Report
    0x05, 0x07,                // Usage Page (Key Codes)
    0x19, 0xE0,                // Usage Minimum (Left Control)
    0x29, 0xE7,                // Usage Maximum (Right Control)
    0x15, 0x00,                // Logical Minimum (0)
    0x25, 0x01,                // Logical Maximum (1)
    0x75, 0x01,                // Report Size (1)
    0x95, 0x08,                // Report Count (8)
    0x81, 0x02,                // Input (Data, Variable, Absolute) - Modifier byte
    0x95, 0x01,                // Report Count (1)
    0x75, 0x08,                // Report Size (8)
    0x81, 0x01,                // Input (Constant) - Reserved byte
    0x95, 0x05,                // Report Count (5)
    0x75, 0x01,                // Report Size (1)
    0x05, 0x08,                // Usage Page (LEDs)
    0x19, 0x01,                // Usage Minimum (Num Lock)
    0x29, 0x05,                // Usage Maximum (Kana)
    0x91, 0x02,                // Output (Data, Variable, Absolute) - LED report
    0x95, 0x01,                // Report Count (1)
    0x75, 0x03,                // Report Size (3)
    0x91, 0x01,                // Output (Constant) - LED report padding
    0x95, 0x06,                // Report Count (6)
    0x75, 0x08,                // Report Size (8)
    0x15, 0x00,                // Logical Minimum (0)
    0x25, 0xFF,                // Logical Maximum (101)
    0x05, 0x07,                // Usage Page (Key Codes)
    0x19, 0x00,                // Usage Minimum (0)
    0x29, 0xFF,                // Usage Maximum (255)
    0x81, 0x00,                // Input (Data, Array) - Key array (6 keys)
    0xC0,                      // End Collection

    // Consumer Control Report
    0x05, 0x0C,                // Usage Page (Consumer Devices)
    0x09, 0x01,                // Usage (Consumer Control)
    0xA1, 0x01,                // Collection (Application)
    0x85, 0x02,                // Report ID (2) - Consumer Control Report
    0x75, 0x10,                // Report Size (16)
    0x95, 0x01,                // Report Count (1)
    0x15, 0x00,                // Logical Minimum (0)
    0x26, 0xFF, 0x03,          // Logical Maximum (1023)
    0x19, 0x00,                // Usage Minimum (0)
    0x2A, 0xFF, 0x03,          // Usage Maximum (1023)
    0x81, 0x00,                // Input (Data, Array)
    0xC0                       // End Collection
};

#endif  // REPORT_MAP_H