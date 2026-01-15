#include "mouse.h"
#include "usb.h"
#include "time.h"
#include <stdint.h>

__xdata __at(XADDR_MOUSE_SCROLL_DIRECTION) int8_t scroll_direction = 0;
__xdata __at(XADDR_MOUSE_SCROLL_AT_TIME) uint16_t scroll_at_time = 0;

int8_t vx_q4 = 0;
int8_t vy_q4 = 0;
int8_t FRICTION_Q4 = 1;

int mouse_u = 0;
int mouse_d = 0;
int mouse_l = 0;
int mouse_r = 0;

void mouse_handle_key(uint16_t custom_code, uint8_t down) {
    if (custom_code < 8) {
        uint8_t buttons = USB_EP3I_read(0);
        buttons = (buttons & ~(1 << custom_code)) | (down << custom_code);
        return USB_EP3I_write(0, buttons);
    }

    switch (custom_code) {
    // case 8:  USB_EP3I_write(1, down * MOUSE_MOVE_SPEED);  break; // Right
    // case 9:  USB_EP3I_write(1, -down * MOUSE_MOVE_SPEED); break; // Left
    // case 10: USB_EP3I_write(2, down * MOUSE_MOVE_SPEED);  break; // Down
    // case 11: USB_EP3I_write(2, -down * MOUSE_MOVE_SPEED); break; // Up

    // case 8 : vx_q5 += down * 8; break; // Right
    // case 9 : vx_q5 -= -down * 8; break ; // Left
    // case 10: vy_q5 += down * 8; break; // Down
    // case 11: vy_q5 -= -down * 8; break; // Up

     case 8 : mouse_r = down; break; // Right
     case 9 : mouse_l = down; break; // Left
     case 10 : mouse_d = down; break; // Down
     case 11 : mouse_u = down; break; // Up
    
    case 12: // Wheel up
    case 13: // Wheel down
        scroll_direction = custom_code == 12 ? down : -down;
        scroll_at_time = get_timer();
        break;
    }
}

int8_t multiply_q4_sat(int8_t a_q4, int8_t b_q4) {
	int16_t product = (int16_t)a_q4 *  (int16_t)b_q4;
	int16_t res = ((product + (1 << 3)) >> 4);
	
	if (res > 127)  return 127;
    if (res < -128) return -128;
	return (int8_t) res;
}

void mouse_process() {
    if (scroll_direction != 0) {
        if (get_timer() >= scroll_at_time) {
            USB_EP3I_write(3, scroll_direction);
            USB_EP3I_send_now();
            USB_EP3I_write(3, 0);
            scroll_at_time = get_timer() + MOUSE_SCROLL_INTERVAL_MS;
        }
    } else {
        USB_EP3I_write(3, 0);
    }

    if (mouse_r==1) vx_q4 += 4;
	if (mouse_l==1) vx_q4 -= 4;
	if (mouse_d==1) vy_q4 += 4;
	if (mouse_u==1) vy_q4 -= 4;
	
	// Apply friction to movement
    vx_q4 = multiply_q4_sat(vx_q4, (1 << 4) - FRICTION_Q4); 
	vy_q4 = multiply_q4_sat(vy_q4, (1 << 4) - FRICTION_Q4);
	
	// Deadzone
	if (vx_q4 > -2 && vx_q4 < 2) vx_q4 = 0;
	if (vy_q4 > -2 && vy_q4 < 2) vy_q4 = 0;
	
    int8_t dx = vx_q4 / 16;
    int8_t dy = vy_q4 / 16;
    
    USB_EP3I_write(1,dx);
    USB_EP3I_write(2, dy);
    
    USB_EP3I_send_now();
}
