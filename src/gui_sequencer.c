#include <stdint.h>
#include <stdbool.h>

// 16-bit RGB565 colors for the FM-1 240x240 screen
#define COLOR_BG          0x0842  // Dark background
#define COLOR_EMPTY       0x1925  // Dark inactive step
#define COLOR_WHITE       0xFFFF  // Normal Trig -> WHITE
#define COLOR_LIGHT_BLUE  0x665F  // Accent -> LIGHT BLUE
#define COLOR_LIGHT_PURP  0xD4DF  // Fill -> LIGHT PURPLE
#define COLOR_RED         0xF9E6  // Both (Accent + Fill) -> RED
#define COLOR_CURSOR      0x07FF  // Playhead cyan border

typedef struct {
    uint16_t triggers; // 16 bits for 16 steps
    uint16_t accents;
    uint16_t fills;
} PatternTrack;

// Built-in LCD frame drawing hooks (self-contained for linking)
void lcd_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    (void)x; (void)y; (void)w; (void)h; (void)color;
}

void lcd_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    (void)x; (void)y; (void)w; (void)h; (void)color;
}

void draw_sequencer_lane(uint8_t track_idx, uint8_t cur_step, const PatternTrack *track) {
    uint16_t y = 46 + (track_idx * 17);
    uint8_t cell_w = 12;
    uint8_t cell_h = 13;

    for (uint8_t s = 0; s < 16; s++) {
        uint16_t x = 36 + (s * (cell_w + 1));
        bool trig   = (track->triggers & (1 << s)) != 0;
        bool accent = (track->accents  & (1 << s)) != 0;
        bool fill   = (track->fills    & (1 << s)) != 0;

        uint16_t fill_color;

        // Exact trigger color priority:
        if (accent && fill) {
            fill_color = COLOR_RED;        // BOTH -> RED
        } else if (fill) {
            fill_color = COLOR_LIGHT_PURP; // FILL -> LIGHT PURPLE
        } else if (accent) {
            fill_color = COLOR_LIGHT_BLUE; // ACCENT -> LIGHT BLUE
        } else if (trig) {
            fill_color = COLOR_WHITE;      // TRIG -> WHITE
        } else {
            fill_color = COLOR_EMPTY;      // Empty step
        }

        lcd_fill_rect(x, y, cell_w, cell_h, fill_color);

        // Highlight playhead step
        if (s == cur_step) {
            lcd_draw_rect(x - 1, y - 1, cell_w + 2, cell_h + 2, COLOR_CURSOR);
        }
    }
}
