#include "vesa.h"
#include "font.h"
#include "../io/io.h"
#include "../lib/lib.h"
#include "../paging/paging.h"
#include "../system.h"
#include "../lib/alloc.h"

void switch_to_vga() {
    serial_write("Force switch to VGA!\n");
    vesa_available = 0;
    regs16_t regs;
    regs.ax = 0x0003;
    paging_disable();
    int32(0x10, &regs);
    paging_enable();

    uint16_t *terminal_buffer = (uint16_t *) 0xB8000;
    char *error = "This computer has no supported video drivers!";
    for (size_t i = 0; i < strlen(error); i++)
        terminal_buffer[i] = (uint16_t) error[i] | (uint16_t) 0x700;
    panic("No VESA!");
}

struct edid_data get_edid() {
    struct edid_data *edid = (struct edid_data *) kmalloc(sizeof(struct edid_data));

    regs16_t regs;
    regs.ax = 0x4F15;
    regs.bx = 0x01; // BL
    regs.es = get_segment(edid);
    regs.di = get_offset(edid);
    paging_disable();
    int32(0x10, &regs);
    paging_enable();

    kfree(edid);

    return *edid;
}

void vbe_set_mode(unsigned short mode) {
    vesa_available = 0;
    regs16_t regs;
    regs.ax = 0x4F02;
    regs.bx = mode;
    regs.bx |= 0x4000;
    paging_disable();
    int32(0x10, &regs);
    paging_enable();

    if (regs.ax != 0x004F) switch_to_vga();
    else vesa_available = 1;
}

uint16_t *vbe_get_modes() {
    vesa_available = 0;
    char *info_address = (char *) 0x7E00;
    strcpy(info_address, "VBE2");
    for (int i = 4; i < 512; i++) *(info_address + i) = 0;

    regs16_t regs;
    regs.ax = 0x4F00;
    regs.es = 0;
    regs.di = 0x7E00;
    paging_disable();
    int32(0x10, &regs);
    paging_enable();

    struct vbe_info *info = (struct vbe_info *) info_address;

    if (regs.ax != 0x004F || strcmp(info->signature, "VESA") != 0) {
        switch_to_vga();
        return ((void *) 0);
    }

    // Get number of modes
    uint16_t *mode_ptr = (uint16_t *) info->video_modes;
    int number_modes = 1;
    for (uint16_t *p = mode_ptr; *p != 0xFFFF; p++) number_modes++;

    uint16_t *video_modes = kmalloc(sizeof(uint16_t) * number_modes);
    for (int i = 0; i < number_modes; i++)
        video_modes[i] = mode_ptr[i]; // THIS FAILS

    for (int i = 0; i < 47; i++) {
        serial_write_hex(video_modes[i]);
        serial_write("\n");
    }

    return video_modes;
}

struct vbe_mode_info *vbe_get_mode_info(uint16_t mode) {
    regs16_t regs;
    regs.ax = 0x4F01;
    regs.cx = mode;
    regs.es = 0;
    regs.di = 0x7E00;
    paging_disable();
    int32(0x10, &regs);
    paging_enable();

    struct vbe_mode_info_all *mode_info = (struct vbe_mode_info_all *) 0x7E00;

    struct vbe_mode_info *mode_info_final = (struct vbe_mode_info *) kmalloc(sizeof(struct vbe_mode_info));
    mode_info_final->attributes = mode_info->attributes;
    mode_info_final->width = mode_info->width;
    mode_info_final->height = mode_info->height;
    mode_info_final->pitch = mode_info->pitch;
    mode_info_final->bpp = mode_info->bpp;
    mode_info_final->memory_model = mode_info->memory_model;
    mode_info_final->framebuffer = mode_info->framebuffer;
    mode_info_final->success = (uint16_t) regs.ax == 0x004F;

    return mode_info_final;
}

void set_optimal_resolution() {
    uint16_t *video_modes = vbe_get_modes();
    serial_write("\n\n");
    for (int i = 0; i < 47; i++) {
        serial_write_hex(video_modes[i]);
        serial_write("\n");
    }

    uint16_t highest = 0;

    for (uint16_t *mode = video_modes; *mode != 0xFFFF; mode++) {
        struct vbe_mode_info *mode_info = vbe_get_mode_info(*mode);
        serial_write_dec(mode_info->width);

        if ((mode_info->attributes & 0x90) != 0x90 || !mode_info->success ||
            (mode_info->memory_model != 4 && mode_info->memory_model != 6))
            continue;

        serial_write("Found mode: (");
        serial_write_hex(*mode);
        serial_write(") ");
        serial_write_dec(mode_info->width);
        serial_write("x");
        serial_write_dec(mode_info->height);
        serial_write("x");
        serial_write_dec(mode_info->bpp);
        serial_write("\n");

        if (mode_info->width >= vbe_width) {
            // (float) mode_info->width / (float) mode_info->height < 2.0 &&) {
            highest = *mode;
            vbe_width = mode_info->width;
            vbe_height = mode_info->height;
            vbe_pitch = mode_info->pitch;
            vbe_bpp = mode_info->bpp >> 3;
            fb = (unsigned char *) mode_info->framebuffer;
            kfree(mode_info);
        }
        kfree(mode_info);
    }
    kfree(video_modes);

    if (highest == 0) {
        struct vbe_mode_info *mode_info = vbe_get_mode_info(0x010e);
        highest = 0x010e;
        vbe_width = mode_info->width;
        vbe_height = mode_info->height;
        vbe_pitch = mode_info->pitch;
        vbe_bpp = mode_info->bpp >> 3;
        fb = (unsigned char *) mode_info->framebuffer;
        kfree(mode_info);
    }

    serial_write("Using mode: (");
    serial_write_hex(highest);
    serial_write(") ");
    serial_write_dec(vbe_width);
    serial_write("x");
    serial_write_dec(vbe_height);
    serial_write("x");
    serial_write_dec(vbe_bpp << 3);
    serial_write("\n");

    uint32_t fb_psize = vbe_width * vbe_height * vbe_bpp;
    for (uint32_t z = 0; z < fb_psize; z += 4096)
        paging_map((uint32_t) fb + z, (uint32_t) fb + z, PT_PRESENT | PT_RW | PT_USED);

    vbe_set_mode(highest);
}

uint16_t terminal_x = 1;
uint16_t terminal_y = 1;
uint32_t terminal_color = 0xFFFFFF;

// char text[1024] = {0};

void vesa_clear() {
    for (int i = 0; i < vbe_width * vbe_height * vbe_bpp; i++) {
        fb[i] = 0;
        fb[i + 1] = 0;
        fb[i + 2] = 0;
    }
}

void vesa_set_pixel(uint16_t x, uint16_t y, uint32_t color) {
    unsigned pos = x * vbe_bpp + y * vbe_pitch;
    fb[pos] = color & 255;
    fb[pos + 1] = (color >> 8) & 255;
    fb[pos + 2] = (color >> 16) & 255;
}

void vesa_draw_rectangle(int x1, int y1, int x2, int y2, int color) {
    int i, j;
    char blue = color & 255;
    char green = (color >> 8) & 255;
    char red = (color >> 16) & 255;
    int pos1 = x1 * vbe_bpp + y1 * vbe_pitch;
    char *draw = (char *) &fb[pos1];
    for (i = 0; i <= y2 - y1; i++) {
        for (j = 0; j <= x2 - x1; j++) {
            draw[vbe_bpp * j] = blue;
            draw[vbe_bpp * j + 1] = green;
            draw[vbe_bpp * j + 2] = red;
        }
        draw += vbe_pitch;
    }
}

void vesa_draw_char(char ch, int x, int y) {
    int mask[8] = {1, 2, 4, 8, 16, 32, 64, 128};
    unsigned char *glyph = font[ch - 32];

    for (int cy = 0; cy < 13; cy++) {
        for (int cx = 0; cx < 8; cx++) {
            if (glyph[cy] & mask[cx]) {
                vesa_set_pixel(x + 8 - cx, y + 13 - cy, terminal_color);
            }
        }
    }
}

void vesa_keyboard_char(char ch) {
    vesa_draw_rectangle(terminal_x, terminal_y, terminal_x + 10, terminal_y + 16, 0x0);

    if (ch == 0x08) {
        if (terminal_x != 0) terminal_x -= 10;
    } else if (ch == 0x09) {
        terminal_x += 8 * 10;
    } else if (ch == '\r') {
        terminal_x = 0;
    } else if (ch == '\n') {
        terminal_y += 15;
        terminal_x = 0;
        // terminal_scroll();
    } else if (ch >= ' ') {
        vesa_draw_char(ch, terminal_x, terminal_y);
        terminal_x += 10;
    }

    // Add new line on overflow
    if (terminal_x >= vbe_width) {
        terminal_x = 0;
        terminal_y += 15;
    }

    // terminal_scroll();
    vesa_draw_rectangle(terminal_x, terminal_y, terminal_x + 10, terminal_y + 16, terminal_color);
}

void vesa_draw_string(char *data) {
    vesa_clear();
    int i = 0;
    while (data[i] != '\0') {
        vesa_draw_char(data[i], terminal_x, terminal_y);
        terminal_x += 10;
        i++;
    }
}

void vesa_set_color(uint32_t color) {
    terminal_color = color;
}
