// MIT License, Copyright (c) 2020 Marvin Borner

#include <conv.h>
#include <def.h>
#include <gfx.h>
#include <gui.h>
#include <html.h>
#include <http.h>
#include <input.h>
#include <mem.h>
#include <net.h>
#include <print.h>
#include <str.h>
#include <xml.h>

#define WIDTH 640
#define HEIGHT 400
#define FONT_HEIGHT 24
#define LABEL_WIDTH 36 // Thx Lars

static struct element *code_label;
static struct element *output;

u32 status_color(char *http_code)
{
	u32 c = 0;
	switch (http_code[0]) {
	case '1': // Information response
		c = COLOR_BLUE;
		break;
	case '2': // Successful response
		c = COLOR_GREEN;
		break;
	case '3': // Redirects
		c = COLOR_YELLOW;
		break;
	case '4': // Client error
		c = COLOR_RED;
		break;
	case '5': // Server error
		c = COLOR_MAGENTA;
		break;
	default:
		c = COLOR_WHITE;
		break;
	}
	return c;
}

void print_indent(char *buf, u32 n)
{
	for (u32 i = 0; i < n; i++)
		strcat(buf, "\t");
}

void print_error(const char *error)
{
	gui_add_label(output, 0, 0, FONT_24, error, COLOR_BLACK, COLOR_RED);
}

void on_submit(void *event, struct element *box)
{
	(void)event;
	char *url = ((struct element_text_input *)box->data)->text;
	gui_remove_childs(output);

	u16 port = 80;
	char *port_str = strchr(url, ':');
	if (port_str) {
		port_str[0] = '\0';
		port_str++;
		port = atoi(port_str);
	}

	char *path = strchr(url, '/');
	if (path) {
		path[0] = '\0';
		path++;
	}
	char *query = http_query_get(url, path ? path : "/");

	u32 ip = 0;
	if (!ip_pton(url, &ip)) {
		ip = dns_request(url);
	}

	struct element_label *c = code_label->data;

	struct socket *socket = NULL;
	if (ip && (socket = net_open(S_TCP)) && net_connect(socket, ip, port, NET_TIMEOUT)) {
		net_send(socket, query, strlen(query));
		char *buf = NULL;
		if (!(buf = http_receive(socket)) || !html_render(output, http_data(buf), 4096)) {
			print_error("HTML parsing failed.\n");
		} else {
			c->text = http_code(buf);
			c->color_fg = status_color(c->text);
		}
	} else {
		print_error("Can't connect to server.\n");
		c->text = strdup("000");
		c->color_fg = COLOR_RED;
	}
	gui_sync(output);
	gui_sync(code_label);
	net_close(socket);
}

int main()
{
	// TODO: Dynamic element positioning
	struct element *root = gui_init("browser", WIDTH, HEIGHT, COLOR_BG);
	code_label = gui_add_label(root, 0, 0, FONT_24, "000", COLOR_BLACK, COLOR_WHITE);
	struct element *text_input =
		gui_add_text_input(root, LABEL_WIDTH, 0, 100, FONT_24, COLOR_WHITE, COLOR_BLACK);
	output = gui_add_container(root, 0, FONT_HEIGHT + 2, 100, 100, COLOR_WHITE);
	gui_add_label(output, 0, 0, FONT_24, "Enter URL and press Enter :)", COLOR_WHITE,
		      COLOR_BLACK);

	text_input->event.on_submit = on_submit;

	gui_event_loop(root);

	return 0;
}
