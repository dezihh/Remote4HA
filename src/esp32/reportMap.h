// reportMap.h
#ifndef REPORT_MAP_H
#define REPORT_MAP_H

#include <stdint.h>

// Hier definierst du die HID Report Map
const uint8_t reportMap[] = {
    0x05, 0x01,                    // Usage Page (Generic Desktop)
    0x09, 0x06,                    // Usage (Keyboard)
    0xA1, 0x01,                    // Collection (Application)
    0x85, 0x01,                    // Report ID (1) - Keyboard Report
    // Modifier byte
    0x75, 0x01,                    // Report Size (1)
    0x95, 0x08,                    // Report Count (8)
    0x05, 0x07,                    // Usage Page (Key Codes)
    0x19, 0xE0,                    // Usage Minimum (224)
    0x29, 0xE7,                    // Usage Maximum (231)
    0x15, 0x00,                    // Logical Minimum (0)
    0x25, 0x01,                    // Logical Maximum (1)
    0x81, 0x02,                    // Input (Data, Variable, Absolute)
    // Reserved byte
    0x95, 0x01,                    // Report Count (1)
    0x75, 0x08,                    // Report Size (8)
    0x81, 0x03,                    // Input (Constant, Variable)
    // Key arrays (6 bytes)
    0x95, 0x06,                    // Report Count (6)
    0x75, 0x08,                    // Report Size (8)
    0x15, 0x00,                    // Logical Minimum (0)
    0x25, 0xFF,                    // Logical Maximum (255)
    0x05, 0x07,                    // Usage Page (Key Codes)
    0x19, 0x00,                    // Usage Minimum (0)
    0x29, 0xFF,                    // Usage Maximum (255)
    0x81, 0x00,                    // Input (Data, Array)
    0xC0,                          // End Collection

    // Consumer Control Report
    0x05, 0x0C,                    // Usage Page (Consumer Devices)
    0x09, 0x01,                    // Usage (Consumer Control)
    0xA1, 0x01,                    // Collection (Application)
    0x85, 0x02,                    // Report ID (2) - Consumer Control Report
    0x75, 0x10,                    // Report Size (16)
    0x95, 0x01,                    // Report Count (1)
    0x15, 0x00,                    // Logical Minimum (0)
    0x26, 0xFF, 0x03,              // Logical Maximum (1023)
    0x19, 0x00,                    // Usage Minimum (0)
    0x2A, 0xFF, 0x03,              // Usage Maximum (1023)
    0x81, 0x00,                    // Input (Data, Array)
    0xC0                           // End Collection
};


#endif  // REPORT_MAP_H
