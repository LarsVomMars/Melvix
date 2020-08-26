// MIT License, Copyright (c) 2020 Marvin Borner
// Some GUI functions

#include <assert.h>
#include <bmp.h>
#include <gui.h>
#include <mem.h>
#include <psf.h>
#include <str.h>
#include <sys.h>
#include <vesa.h>

struct font *font;

static void write_char(struct window *win, int x, int y, const u32 c[3], char ch)
{
	int bypp = win->bpp >> 3;

	int pos = x * bypp + y * win->pitch;
	char *draw = (char *)&win->fb[pos];

	u32 stride = font->char_size / font->height;
	for (int cy = 0; cy < font->height; cy++) {
		for (int cx = 0; cx < font->width; cx++) {
			u8 bits = font->chars[ch * font->char_size + cy * stride + cx / 8];
			u8 bit = bits >> (7 - cx % 8) & 1;
			if (bit) {
				draw[bypp * cx] = c[2];
				draw[bypp * cx + 1] = c[1];
				draw[bypp * cx + 2] = c[0];
			}
		}
		draw += win->pitch;
	}
}

static void draw_rectangle(struct window *win, int x1, int y1, int x2, int y2, const u32 c[3])
{
	int bypp = win->bpp >> 3;
	u8 *draw = &win->fb[x1 * bypp + y1 * win->pitch];
	for (int i = 0; i < y2 - y1; i++) {
		for (int j = 0; j < x2 - x1; j++) {
			draw[bypp * j] = c[2];
			draw[bypp * j + 1] = c[1];
			draw[bypp * j + 2] = c[0];
		}
		draw += win->pitch;
	}
}

void gui_write_char(struct window *win, int x, int y, const u32 c[3], char ch)
{
	write_char(win, x, y, c, ch);
	gui_redraw();
}

void gui_write(struct window *win, int x, int y, const u32 c[3], char *text)
{
	for (u32 i = 0; i < strlen(text); i++) {
		write_char(win, x + i * font->width, y, c, text[i]);
	}
	gui_redraw();
}

void gui_load_image(struct window *win, char *path, int x, int y)
{
	struct bmp *bmp = bmp_load(path);
	assert(bmp && bmp->width + x <= win->width);
	assert(bmp && bmp->height + y <= win->height);

	// TODO: Support padding with odd widths
	int bypp = bmp->bpp >> 3;
	u8 *srcfb = &bmp->data[bypp];
	u8 *destfb = &win->fb[bypp];
	for (u32 cy = 0; cy < bmp->height; cy++) {
		memcpy(destfb, srcfb, bmp->pitch);
		srcfb += bmp->pitch;
		destfb += win->pitch;
	}
	gui_redraw();
}

void gui_load_wallpaper(struct window *win, char *path)
{
	gui_load_image(win, path, 0, 0);
}

void gui_copy(struct window *dest, struct window *src, int x, int y, u32 width, u32 height)
{
	int bypp = dest->bpp >> 3;
	u8 *srcfb = &src->fb[(x + 1) * bypp + y * src->pitch];
	u8 *destfb = &dest->fb[(x + 1) * bypp + y * dest->pitch];
	for (u32 cy = 0; cy < height; cy++) {
		memcpy(destfb, srcfb, width * (dest->bpp >> 3));
		srcfb += src->pitch;
		destfb += dest->pitch;
	}
}

// TODO: Optimize!
void gui_win_on_win(struct window *dest, struct window *src, int x, int y)
{
	if (src->width == dest->width && src->height == dest->height && src->x == 0 &&
	    dest->y == 0) {
		memcpy(dest->fb, src->fb, dest->pitch * dest->height);
		return;
	}

	int bypp = dest->bpp >> 3;
	u8 *srcfb = &src->fb[bypp];
	u8 *destfb = &dest->fb[(x + 1) * bypp + y * dest->pitch];
	for (u32 cy = 0; cy < src->height; cy++) {
		memcpy(destfb, srcfb, src->pitch);
		srcfb += src->pitch;
		destfb += dest->pitch;
	}
}

void gui_draw_rectangle(struct window *win, int x1, int y1, int x2, int y2, const u32 c[3])
{
	draw_rectangle(win, x1, y1, x2, y2, c);
	gui_redraw();
}

void gui_fill(struct window *win, const u32 c[3])
{
	draw_rectangle(win, 0, 0, win->width, win->height, c);
	gui_redraw();
}

void gui_border(struct window *win, const u32 c[3], u32 width)
{
	if (width <= 0)
		return;

	int bypp = win->bpp >> 3;
	u8 *draw = win->fb;
	for (u32 i = 0; i < win->height; i++) {
		for (u32 j = 0; j < win->width; j++) {
			if (j <= width - 1 || i <= width - 1 ||
			    j - win->width + width + 1 <= width ||
			    i - win->height + width <= width) {
				draw[bypp * j + 0] = c[2];
				draw[bypp * j + 1] = c[1];
				draw[bypp * j + 2] = c[0];
			}
		}
		draw += win->pitch;
	}
	gui_redraw();
}

void gui_init(char *font_path)
{
	font = psf_parse(read(font_path));
	assert(font);
}
