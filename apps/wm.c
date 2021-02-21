// MIT License, Copyright (c) 2020 Marvin Borner

#include <assert.h>
#include <def.h>
#include <gfx.h>
#include <input.h>
#include <keymap.h>
#include <list.h>
#include <random.h>
#include <vesa.h>

//#define FLUSH_TIMEOUT 6

struct client {
	u32 pid;
};

struct window {
	u32 id;
	const char *name;
	struct context ctx;
	struct client client;
	u32 flags;
	vec2 pos;
	vec2 pos_prev;
};

struct rectangle {
	vec2 pos1; // Upper left
	vec2 pos2; // Lower right
	void *data;
};

static struct vbe screen = { 0 };
static struct list *windows = NULL; // THIS LIST SHALL BE SORTED BY Z-INDEX!
static struct window *root = NULL;
static struct window *direct = NULL;
static struct window *cursor = NULL;
static struct keymap *keymap = NULL;
static struct client wm_client = { 0 };
static struct {
	u8 shift : 1;
	u8 alt : 1;
	u8 ctrl : 1;
} special_keys = { 0 };
static struct {
	vec2 pos;
	u8 left : 1;
	u8 mid : 1;
	u8 right : 1;
} mouse = { 0 };

static struct window *window_create(struct client client, const char *name, struct vec2 pos,
				    struct vec2 size, u32 flags)
{
	struct window *win = malloc(sizeof(*win));
	win->id = rand();
	win->name = name;
	win->ctx.size = size;
	win->ctx.bpp = screen.bpp;
	win->ctx.pitch = size.x * (win->ctx.bpp >> 3);
	win->ctx.bytes = win->ctx.pitch * win->ctx.size.y;
	if (flags && (flags & WF_NO_FB) == 0)
		win->ctx.fb = zalloc(size.y * win->ctx.pitch);
	win->client = client;
	win->flags = flags;
	win->pos = pos;
	list_add(windows, win);
	return win;
}

static struct window *window_find(u32 id)
{
	struct node *iterator = windows->head;
	while (iterator) {
		struct window *win = iterator->data;
		if (win->id == id)
			return win;
		iterator = iterator->next;
	}
	return NULL;
}

static void window_destroy(struct window *win)
{
	free(win->ctx.fb);
	free(win);
}

static void buffer_flush()
{
#ifdef FLUSH_TIMEOUT
	static u32 time_flush = 0;
	u32 time_now = time();
	if (time_now - time_flush > FLUSH_TIMEOUT) {
		memcpy(direct->ctx.fb, root->ctx.fb, root->ctx.bytes);
		time_flush = time_now;
	}
#else
	memcpy(direct->ctx.fb, root->ctx.fb, root->ctx.bytes);
#endif
}

static void windows_at_rec(vec2 pos1, vec2 pos2, struct list *list)
{
	struct node *iterator = windows->head;
	while (iterator) {
		struct window *win = iterator->data;
		u8 starts_in = (win->pos.x > pos1.x && win->pos.x < pos2.x) &&
			       (win->pos.y > pos1.y && win->pos.y < pos2.y);
		if (starts_in)
			list_add(list, win);
		iterator = iterator->next;
	}
}

static struct rectangle rectangle_at(vec2 pos1, vec2 pos2, struct window *excluded)
{
	u32 width = pos2.x - pos1.x;
	u32 height = pos2.y - pos1.y;
	void *data = malloc(width * height * 4);

	struct list *windows_at = list_new();
	windows_at_rec(pos1, pos2, windows_at);
	struct node *iterator = windows_at->head;
	while (iterator) {
		struct window *win = iterator->data;

		/* int bypp = win->ctx.bpp >> 3; */
		/* u8 *srcfb = &win->ctx.fb[pos1.x * bypp + pos1.y * win->ctx.pitch]; */
		/* u8 *destfb = data; */
		/* u32 cnt = 0; */
		/* for (u32 cy = 0; cy < height; cy++) { */
		/* 	memcpy(destfb, srcfb, width * bypp); */
		/* 	srcfb += win->ctx.pitch; */
		/* 	destfb += win->ctx.pitch; */
		/* 	cnt += win->ctx.pitch; */
		/* } */

		iterator = iterator->next;

		if (win == excluded)
			continue;

		log("Window found: %s\n", win->name);
	}
	list_destroy(windows_at);

	return (struct rectangle){ .pos1 = pos1, .pos2 = pos2, .data = data };
}

static void redraw_window(struct window *win)
{
	if (win->ctx.size.x == win->ctx.size.y) {
		// TODO: Redraw rectangle
		rectangle_at(win->pos_prev, vec2_add(win->pos_prev, win->ctx.size), win);
		gfx_draw_rectangle(&root->ctx, win->pos_prev,
				   vec2_add(win->pos_prev, win->ctx.size), 0);
	} else {
		err(1, "Rectangle splitting isn't supported yet!\n");
	}

	gfx_ctx_on_ctx(&root->ctx, &win->ctx, win->pos);
	buffer_flush();
}

static void handle_event_keyboard(struct event_keyboard *event)
{
	if (event->magic != KEYBOARD_MAGIC) {
		log("Keyboard magic doesn't match!\n");
		return;
	}

	if (event->scancode == KEY_LEFTSHIFT || event->scancode == KEY_RIGHTSHIFT)
		special_keys.shift ^= 1;
	else if (event->scancode == KEY_LEFTALT || event->scancode == KEY_RIGHTALT)
		special_keys.alt ^= 1;
	else if (event->scancode == KEY_LEFTCTRL || event->scancode == KEY_RIGHTCTRL)
		special_keys.ctrl ^= 1;

	char ch;
	if (special_keys.shift)
		ch = keymap->shift_map[event->scancode];
	else if (special_keys.alt)
		ch = keymap->alt_map[event->scancode];
	else
		ch = keymap->map[event->scancode];

	(void)ch;
}

static void handle_event_mouse(struct event_mouse *event)
{
	if (event->magic != MOUSE_MAGIC) {
		log("Mouse magic doesn't match!\n");
		return;
	}

	cursor->pos_prev = mouse.pos;

	mouse.pos.x += event->diff_x;
	mouse.pos.y -= event->diff_y;

	// Fix x overflow
	if ((signed)mouse.pos.x < 0)
		mouse.pos.x = 0;
	else if (mouse.pos.x + cursor->ctx.size.x > (unsigned)screen.width - 1)
		mouse.pos.x = screen.width - cursor->ctx.size.x - 1;

	// Fix y overflow
	if ((signed)mouse.pos.y < 0)
		mouse.pos.y = 0;
	else if (mouse.pos.y + cursor->ctx.size.y > (unsigned)screen.height - 1)
		mouse.pos.y = screen.height - cursor->ctx.size.y - 1;

	/* log("%d %d\n", mouse.pos.x, mouse.pos.y); */
	cursor->pos = mouse.pos;

	redraw_window(cursor);
}

int main(int argc, char **argv)
{
	(void)argc;
	screen = *(struct vbe *)argv[1];
	wm_client = (struct client){ .pid = getpid() };
	log("WM loaded: %dx%d\n", screen.width, screen.height);

	windows = list_new();
	keymap = keymap_parse("/res/keymaps/en.keymap");

	direct = window_create(wm_client, "direct", vec2(0, 0), vec2(screen.width, screen.height),
			       WF_NO_FB | WF_NO_DRAG | WF_NO_FOCUS | WF_NO_RESIZE);
	direct->ctx.fb = screen.fb;
	direct->flags ^= WF_NO_FB;
	root = window_create(wm_client, "root", vec2(0, 0), vec2(screen.width, screen.height),
			     WF_NO_DRAG | WF_NO_FOCUS | WF_NO_RESIZE);
	cursor = window_create(wm_client, "cursor", vec2(0, 0), vec2(32, 32),
			       WF_NO_DRAG | WF_NO_FOCUS | WF_NO_RESIZE);

	/* gfx_write(&direct->ctx, vec2(0, 0), FONT_32, COLOR_FG, "Loading Melvix..."); */
	gfx_load_wallpaper(&root->ctx, "/res/wall.png");
	gfx_load_wallpaper(&cursor->ctx, "/res/cursor.png");

	struct message msg = { 0 };
	struct event_keyboard event_keyboard = { 0 };
	struct event_mouse event_mouse = { 0 };
	const char *listeners[] = { "/dev/kbd", "/dev/mouse", "/proc/self/msg" };
	while (1) {
		int poll_ret = 0;
		if ((poll_ret = poll(listeners)) >= 0) {
			if (poll_ret == 0) {
				if (read(listeners[poll_ret], &event_keyboard, 0,
					 sizeof(event_keyboard)) > 0)
					handle_event_keyboard(&event_keyboard);
				continue;
			} else if (poll_ret == 1) {
				if (read(listeners[poll_ret], &event_mouse, 0,
					 sizeof(event_mouse)) > 0)
					handle_event_mouse(&event_mouse);
				continue;
			} else if (poll_ret == 2) {
				if (read(listeners[poll_ret], &msg, 0, sizeof(msg)) <= 0)
					continue;
			}
		} else {
			err(1, "POLL ERROR!\n");
		}

		if (msg.magic != MSG_MAGIC) {
			log("Message magic doesn't match!\n");
			continue;
		}

		log("not implemented!\n");
	};

	// TODO: Execute?
	free(keymap);
	list_destroy(windows);

	return 0;
}
