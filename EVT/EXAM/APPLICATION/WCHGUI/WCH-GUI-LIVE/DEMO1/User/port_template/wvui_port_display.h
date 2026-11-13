#ifndef WVUI_PORT_DISPLAY_H
#define WVUI_PORT_DISPLAY_H



#include <stdint.h>
#include "wvui_port.h"


int lcd_init(void);
void lcd_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void lcd_write_pixels(const uint16_t *pixels, uint32_t count);
void lcd_wait_done(void);
void lcd_end_transaction(void);


void wvui_port_display_configure(uint8_t async);

const wvui_port_t *wvui_port_display(void);

#endif /* WVUI_PORT_DISPLAY_H */
