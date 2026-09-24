#ifndef SKY_CYCLE_H
#define SKY_CYCLE_H

#include <stdint.h>
#include <stdbool.h>


#define SKY_CYCLE_AWAKE_MS 30000u
#define SKY_CYCLE_IDLE_MS  60000u

void sky_cycle_note_read(uint8_t slot);
void sky_cycle_note_field_detected(void);
void sky_cycle_note_field_lost(void);
void sky_cycle_note_sense_end(void);
void sky_cycle_note_command(void);
void sky_cycle_note_host_command(uint16_t cmd);
void sky_cycle_note_slot_taken_over(void);
void sky_cycle_note_link_down(void);

void sky_cycle_toggle(void);
void sky_cycle_note_short_press(char which);

void sky_cycle_process(void);

uint8_t sky_cycle_led_color(uint8_t slot);
void sky_cycle_apply_idle_light(void);
void sky_cycle_apply_light_preserving(void);

void sky_cycle_init(void);
void sky_cycle_save(void);
bool sky_cycle_is_enabled(void);

#endif
