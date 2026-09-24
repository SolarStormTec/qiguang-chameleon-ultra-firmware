#ifndef SKY_EVENT_H
#define SKY_EVENT_H


#include <stdint.h>
#include <stdbool.h>

#define SKY_EVT_MASK_SLOT_CHANGED   0x0001u
#define SKY_EVT_MASK_TAG_READ       0x0002u
#define SKY_EVT_MASK_BATTERY        0x0004u
#define SKY_EVT_MASK_POWER          0x0008u
#define SKY_EVT_MASK_ALL            (SKY_EVT_MASK_SLOT_CHANGED | SKY_EVT_MASK_TAG_READ | \
                                     SKY_EVT_MASK_BATTERY | SKY_EVT_MASK_POWER)

#define SKY_POWER_FLAG_VBUS       0x01u
#define SKY_POWER_FLAG_FULL       0x02u
#define SKY_POWER_FLAG_UNSAMPLED  0x04u

#define SKY_SLOT_CHANGE_BY_APP       0x00
#define SKY_SLOT_CHANGE_BY_BUTTON    0x01
#define SKY_SLOT_CHANGE_BY_FIRMWARE  0x02
#define SKY_SLOT_CHANGE_BY_AUTO      0x03

void sky_event_set_mask(uint16_t mask);
uint16_t sky_event_get_mask(void);

void sky_event_on_disconnect(void);

void sky_event_note_slot_changed(uint8_t slot, uint8_t reason);
void sky_event_note_battery(uint16_t millivolt, uint8_t percent);
void sky_event_note_power(uint16_t millivolt, uint8_t percent, uint8_t flags);

void sky_event_process(void);

#endif
