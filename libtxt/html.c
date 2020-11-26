// MIT License, Copyright (c) 2020 Marvin Borner
// HTML parsing is mainly based on the XML parser

#include <assert.h>
#include <gui.h>
#include <html.h>
#include <list.h>
#include <mem.h>
#include <print.h>
#include <str.h>
#include <xml.h>

static int is_self_closing(const char *tag)
{
	const char *void_elements[] = { "area",	 "base", "br",	 "col",	  "embed",  "hr",    "img",
					"input", "link", "meta", "param", "source", "track", "wbr" };

	for (u32 i = 0; i < sizeof(void_elements) / sizeof(void_elements[0]); ++i) {
		if (!strcmp(void_elements[i], tag))
			return 1;
	}
	return 0;
}

static char *normalize_tag_name(char *tag)
{
	for (char *p = tag; *p; ++p)
		*p = *p > 0x40 && *p < 0x5b ? *p | 0x60 : *p;
	return tag;
}

static struct dom *new_object(const char *tag, struct dom *parent)
{
	struct dom *object = malloc(sizeof(*object));
	object->tag = normalize_tag_name(strdup(tag));
	object->parent = parent;
	object->content = NULL;
	object->children = list_new();
	return object;
}

static void print_dom(struct dom *dom, u32 level)
{
	struct node *iterator = dom->children->head;
	while (iterator != NULL) {
		struct dom *obj = iterator->data;
		for (u32 i = 0; i < level; i++)
			print("\t");
		printf("'%s': '%s'\n", obj->tag, obj->content ? obj->content : "");
		if (obj->children->head)
			print_dom(obj, level + 1);
		iterator = iterator->next;
	}
}

static struct dom *generate_dom(char *data, u32 length)
{
	struct xml_token tokens[128];
	struct xml parser;
	xml_init(&parser);
	void *buffer = data;
	enum xml_error err = xml_parse(&parser, buffer, length, tokens, 128);

	if (err != XML_SUCCESS && err != XML_ERROR_BUFFERDRY) {
		printf("\nXML parse error: %d\n", err);
		return 0;
	}

	struct dom *root = new_object("root", NULL);
	struct dom *current = root;

	char name[256] = { 0 };
	for (u32 i = 0; i < parser.ntokens; i++) {
		const struct xml_token *token = tokens + i;
		name[0] = '\0';
		switch (token->type) {
		case XML_START_TAG:
			memcpy(&name, (u8 *)buffer + token->start_pos,
			       token->end_pos - token->start_pos);
			name[token->end_pos - token->start_pos] = '\0';
			current = new_object(name, current);
			printf("Adding %s to %s\n", current->tag, current->parent->tag);
			list_add(current->parent->children, current);
			break;
		case XML_END_TAG:
			memcpy(&name, (u8 *)buffer + token->start_pos,
			       token->end_pos - token->start_pos);
			name[token->end_pos - token->start_pos] = '\0';
			assert(current && !strcmp(name, current->tag));
			current = current->parent;
			break;
		case XML_CHARACTER:
			if (!current)
				continue;

			if (token->end_pos == token->start_pos + 2) {
				const char *ptr = (char *)buffer + token->start_pos;

				if (ptr[0] == '\r' && ptr[1] == '\n')
					continue;
			}
			memcpy(&name, (u8 *)buffer + token->start_pos,
			       token->end_pos - token->start_pos);
			name[token->end_pos - token->start_pos] = '\0';
			char *clean_name = name;
			for (u32 j = 0; j < strlen(name); j++) {
				if (name[j] == ' ' || name[j] == '\n' || name[j] == '\r' ||
				    name[j] == '\t') {
					clean_name++;
				} else {
					break;
				}
			}
			if (!strlen(clean_name))
				break;
			current->content = strdup(clean_name);
			break;
		default:
			break;
		}

		i += token->size;
	}

	print("GENERATED!\n");
	print_dom(root, 0);
	return root;
}

static struct html_element *new_html_element(struct element *container, struct dom *dom)
{
	struct html_element *elem = malloc(sizeof(*elem));
	elem->x_offset = 0;
	elem->y_offset = 0;
	elem->dom = dom;
	elem->obj = container;
	return elem;
}

// TODO: Better structure?
// TODO: Less code duplication (e.g. for headings)
#define CMP(tag, tag_string) (!strcmp((tag), (tag_string)))
static struct html_element *render_object(struct html_element *container, struct dom *dom)
{
	char *tag = dom->tag;

	if (CMP(tag, "html")) {
		struct element *obj =
			gui_add_container(container->obj, 0, 0, 100, 100, COLOR_WHITE);
		return new_html_element(obj, dom);
	} else if (CMP(tag, "body")) {
		struct element *obj =
			gui_add_container(container->obj, 0, 0, 100, 100, COLOR_WHITE);
		return new_html_element(obj, dom);
	} else if (CMP(tag, "h1")) {
		struct element *obj =
			gui_add_label(container->obj, container->x_offset, container->y_offset,
				      FONT_64, dom->content, COLOR_WHITE, COLOR_BLACK);
		container->x_offset = 0;
		container->y_offset += obj->ctx->height;
		return new_html_element(obj, dom);
	} else if (CMP(tag, "h2")) {
		struct element *obj =
			gui_add_label(container->obj, container->x_offset, container->y_offset,
				      FONT_32, dom->content, COLOR_WHITE, COLOR_BLACK);
		container->x_offset = 0;
		container->y_offset += obj->ctx->height;
		return new_html_element(obj, dom);
	} else if (CMP(tag, "h3")) {
		struct element *obj =
			gui_add_label(container->obj, container->x_offset, container->y_offset,
				      FONT_24, dom->content, COLOR_WHITE, COLOR_BLACK);
		container->x_offset = 0;
		container->y_offset += obj->ctx->height;
		return new_html_element(obj, dom);
	} else if (CMP(tag, "p")) {
		struct element *obj =
			gui_add_label(container->obj, container->x_offset, container->y_offset,
				      FONT_16, dom->content, COLOR_WHITE, COLOR_BLACK);
		container->x_offset = 0;
		container->y_offset += obj->ctx->height;
		return new_html_element(obj, dom);
	} else if (CMP(tag, "hr")) {
		gfx_draw_rectangle(container->obj->ctx, container->x_offset, container->y_offset,
				   container->obj->ctx->width - container->x_offset,
				   container->y_offset + 2, COLOR_BLACK);
		container->x_offset = 0;
		container->y_offset += 2;
		return container;
	} else {
		printf("UNKNOWN %s\n", tag);
		return container;
	}
}

int html_render_dom(struct html_element *container, struct dom *dom)
{
	struct node *iterator = dom->children->head;
	while (iterator != NULL) {
		struct dom *obj = iterator->data;
		struct html_element *rendered = render_object(container, obj);
		if (obj->children->head && rendered)
			html_render_dom(rendered, obj);
		iterator = iterator->next;
	}
	return 1;
}

int html_render(struct element *container, char *data, u32 length)
{
	struct dom *dom = generate_dom(data, length);
	struct html_element *obj = new_html_element(container, dom);
	return dom && obj && html_render_dom(obj, dom);
}
