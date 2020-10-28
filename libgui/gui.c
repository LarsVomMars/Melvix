// MIT License, Copyright (c) 2020 Marvin Borner
// Mostly GFX function wrappers

#include <def.h>
#include <gfx.h>
#include <gui.h>
#include <list.h>
#include <mem.h>
#include <print.h>
#include <str.h>
#include <sys.h>

#define MAX_WINDOWS 10

u32 window_count = 0;
static struct window windows[MAX_WINDOWS] = { 0 };

static struct window *new_window(const char *title, int x, int y, u32 width, u32 height, int flags)
{
	if (window_count + 1 >= MAX_WINDOWS)
		return NULL;

	struct window *win = &windows[window_count + 1];
	win->ctx = malloc(sizeof(*win->ctx));
	win->ctx->x = x > 0 ? x : 50;
	win->ctx->y = y > 0 ? y : 50;
	win->ctx->width = width > 0 ? width : 600;
	win->ctx->height = height > 0 ? height : 400;
	win->ctx->flags = flags;
	win->id = window_count + 1;
	win->title = title;
	win->childs = list_new();
	gfx_new_ctx(win->ctx);

	if (!win->ctx->fb)
		return NULL;

	window_count++;

	return win;
}

static void merge_elements(struct element *container)
{
	if (!container || !container->childs || !container->childs->head)
		return;

	struct node *iterator = container->childs->head;
	while (iterator != NULL) {
		struct element *elem = iterator->data;
		struct context *ctx = elem->ctx;
		printf("Merging %dx%d onto %dx%d\n", ctx->width, ctx->height, container->ctx->width,
		       container->ctx->height);
		merge_elements(elem);
		gfx_ctx_on_ctx(container->ctx, ctx, ctx->x, ctx->y);
		iterator = iterator->next;
	}
}

static struct element *element_at(struct element *container, int x, int y)
{
	if (!container || !container->childs || !container->childs->head)
		return NULL;

	struct node *iterator = container->childs->head;
	while (iterator != NULL) {
		struct context *ctx = ((struct element *)iterator->data)->ctx;

		int relative_x, relative_y;
		if (container->type == GUI_TYPE_ROOT) {
			relative_x = ctx->x;
			relative_y = ctx->y;
		} else {
			relative_x = ctx->x + container->ctx->x;
			relative_y = ctx->y + container->ctx->y;
		}

		if (ctx != container->ctx && ctx->flags & WF_RELATIVE && x >= relative_x &&
		    x <= relative_x + (int)ctx->width && y >= relative_y &&
		    y <= relative_y + (int)ctx->height) {
			struct element *recursive = NULL;
			if ((recursive = element_at(iterator->data, x, y)))
				return recursive;
			else
				return iterator->data;
		}

		iterator = iterator->next;
	}

	return NULL;
}

void gui_sync_button(struct element *elem)
{
	struct element_button *button = elem->data;
	gfx_fill(elem->ctx, button->color_bg);
	gfx_write(elem->ctx, 0, 0, button->font_type, button->color_fg, button->text);
}

void gui_sync_container(struct element *elem)
{
	struct element_container *container = elem->data;
	gfx_fill(elem->ctx, container->color_bg);
	// TODO: Handle container flags
}

struct element *gui_add_button(struct element *container, int x, int y, enum font_type font_type,
			       char *text, u32 color_bg, u32 color_fg)
{
	if (!container || !container->childs)
		return NULL;

	gfx_resolve_font(font_type);

	struct element *button = malloc(sizeof(*button));
	button->type = GUI_TYPE_BUTTON;
	button->window_id = container->window_id;
	button->ctx = malloc(sizeof(*button->ctx));
	button->ctx->x = x;
	button->ctx->y = y;
	button->ctx->width = strlen(text) * gfx_font_width(font_type);
	button->ctx->height = gfx_font_height(font_type);
	button->ctx->flags = WF_RELATIVE;
	button->childs = list_new();
	button->data = malloc(sizeof(struct element_button));
	((struct element_button *)button->data)->text = text;
	((struct element_button *)button->data)->color_fg = color_fg;
	((struct element_button *)button->data)->color_bg = color_bg;
	((struct element_button *)button->data)->font_type = font_type;

	gfx_new_ctx(button->ctx);
	list_add(container->childs, button);
	gui_sync_button(button);
	merge_elements(container);

	return button;
}

struct element *gui_add_container(struct element *container, int x, int y, u32 width, u32 height,
				  u32 color_bg)
{
	if (!container || !container->childs)
		return NULL;

	struct element *new_container = malloc(sizeof(*new_container));
	new_container->type = GUI_TYPE_CONTAINER;
	new_container->window_id = container->window_id;
	new_container->ctx = malloc(sizeof(*new_container->ctx));
	new_container->ctx->x = x;
	new_container->ctx->y = y;
	new_container->ctx->width = width;
	new_container->ctx->height = height;
	new_container->ctx->flags = WF_RELATIVE;
	new_container->childs = list_new();
	new_container->data = malloc(sizeof(struct element_container));
	((struct element_container *)new_container->data)->color_bg = color_bg;
	((struct element_container *)new_container->data)->flags = 0;

	gfx_new_ctx(new_container->ctx);
	list_add(container->childs, new_container);
	gui_sync_container(new_container);
	merge_elements(container);

	return new_container;
}

void gui_event_loop(struct element *container)
{
	if (!container)
		return;

	struct message *msg;
	while (1) {
		if (!(msg = msg_receive())) {
			yield();
			continue;
		}

		switch (msg->type) {
		case GUI_MOUSE: {
			struct gui_event_mouse *event = msg->data;
			struct element *elem = element_at(container, event->x, event->y);
			if (!elem)
				continue;

			if (elem->type == GUI_TYPE_BUTTON) {
				struct element_button *button = elem->data;
				if (event->but1 && button->on_click)
					button->on_click();
			}
		}
		}
	}
}

struct element *gui_init(const char *title, u32 width, u32 height)
{
	if (window_count != 0)
		return NULL;

	// TODO: Add center flag
	struct window *win = new_window(title, 30, 30, width, height, WF_DEFAULT);
	if (!win)
		return NULL;

	gfx_fill(win->ctx, COLOR_BG);

	struct element *container = malloc(sizeof(*container));
	container->type = GUI_TYPE_ROOT;
	container->window_id = win->id;
	container->ctx = win->ctx;
	container->childs = list_new();
	container->data = NULL;
	list_add(win->childs, container);

	return container;
}
