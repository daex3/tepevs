#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>

#define VERTEX
#define PIXELS
#define SHAPES

#include <tegrine/instances.c>
#include <tegrine/term.c>
#include <tegrine/json.c>
#include <reader_png.c>

void sub(int *x) {
	*x -= *x ? 1 : 0;
}

void add_l(int *x, int max) {
	*x += *x < max - 1 ? 1 : 0;
}

_Bool empty_input = 0;

// TODO: Backspace
int input_num(char *message) {
	if (empty_input)
		return 0;

	empty_input = 1,
	printf("\x1b[9999H\x1b[2K%s", message),
	fflush(stdout);

	int n = 0;

	char c;
	while (read(0, &c, 1) != -1 && isdigit(c))
		n *= 10,
		n += c - 48,
		putchar(c),
		fflush(stdout),
		empty_input = 0;
	
	return n;
}

char input[1024];

// Define a default value as a parameter, print that as fainted text(placeholder) and while not empty clear it
void input_any(char *message) {
	printf("\x1b[9999H\x1b[2K%s", message),
	fflush(stdout);

	int i = 0;
	char c;

	while (i < 1024 && read(0, &c, 1) != -1 && c != 13)
		input[i++] = c,
		putchar(c),
		fflush(stdout);
	
	input[i] = 0;
}

// Slow
// TODO: Bug doesn't add +instance.pos
size_t get_pixel_index_at_pos(Pixels *px, D2 *pos) {
	for(size_t i = 0; i < px->len; ++i) {
		Px *p = &px->x[i];

		if (p->pos.x == pos->x && p->pos.y == pos->y)
			return i;
	}

	return -1;
}

// Slow
size_t get_vertex_index_at_pos(Vertex *v, D2 *pos, size_t exclude_i) {
	for(size_t i = 0; i < v->len; ++i) {
		if (i == exclude_i)
			continue;

		Vertice *ve = &v->x[i];

		if (ve->pos.x == pos->x && ve->pos.y == pos->y)
			return i;
	}

	return -1;
}

_Bool saved_file_exists(char *name) {
	char	 path[1024] = { },
		*home = getenv("HOME");
	
	if (!home)
		return 0;

	strlcat(path, home, 1024),
	strlcat(path, "/.tegrine/saved/", 1024),
	strlcat(path, name, 1024);

	int fd = open(path, O_RDONLY);

	close(fd);

	return fd != -1;
}

void new_px(Pixels *px, D2 *cur, D2 *pos, RGBA *color) {
	size_t i = get_pixel_index_at_pos(px, cur);

	cur->x -= pos->x,
	cur->y -= pos->y;

	// Exists
	if (i != (size_t)-1)
		px->x[i] = (Px){ *cur, *color };
	else
		add_Px(px, cur, color);

	cur->x += pos->x,
	cur->y += pos->y;
}

typedef struct {
	_Bool px_hold;
	char	c,
		*name;
	D2	cur,
		size;
	Instance *ins;
	RGBA color;
	size_t v_index;
	Tegrine te;
	Vertex *ve;
	Vertice *v;
} Tepevs;

/*
	Every single one of those fixes are math ;-;

	TODO: Ultimately selection with the z.instance itself as for multiple ones...
	TODO: Multiple z.vertex neighbors
	TODO: Temporarily create a new z.instance until pressing Enter to apply
	TODO: Vi-like shortcuts for improved movement, selection, etc... :>
	TODO: Improve color selection
	TODO: Aspect ratio
	TODO: Check unicode characters: The thing is the faces of the vertex
	TODO: Undo
*/
int main(int argc, char **argv) {
	Tepevs z = {
		.name		= argc > 1 ? argv[1] : NULL,
		.v_index	= -1
	};

	if (z.name && saved_file_exists(z.name))
		load_from_json(&z.te, z.name);

	z.ins = z.te.x.len ?
		&z.te.x.x[0]
	:
		add_Instance(
			&z.te.x,
			&z.cur,
			&z.size,
			&z.color
		);

	z.ve = &z.ins->vertex,

	set_ws(&z.te.ws),
	term_raw(),
	draw(&z.te, &z.cur);

	// TODO: Make more shortcuts for properties vi-like
	while (read(0, &z.c, 1) != -1) {
		switch (z.c) {
			// Cursor moz.vement
			// Idea: Create a program that loops triggering every key to detect runtime vulnerabilities
			case 'h':
				sub(&z.cur.x);
				
				break;
			case 'k':
				sub(&z.cur.y);
				
				break;
			case 'j':
				add_l(&z.cur.y, z.te.ws.y);
				
				break;
			case 'l':
				add_l(&z.cur.x, z.te.ws.x);
				
				break;

			// Hold drawing Pixels
			case 'H':
				z.px_hold = z.px_hold ? 0 : (z.v = NULL, 1);

				break;
			case ' ':
				new_px(&z.ins->pixels, &z.cur, &z.ins->pos, &z.color);

				break;
			case 'v': {
				// TODO: Multiple selection is required to expand this part
				size_t i = !z.v ?
					get_vertex_index_at_pos(
						z.ve,
						&z.cur,
						z.v_index
					)
				: (size_t)-1;

				/*
					Selects existing z.vertex
					Adds z.vertex
				*/
				if (i != (size_t)-1)
					z.v = &z.ve->x[z.v_index = i];
				else {
					z.v_index = z.ve->len,
					z.cur.x -= z.ins->pos.x,
					z.cur.y -= z.ins->pos.y;

					// Reconnect last selected z.vertex
					if (z.v)
						z.v->neighbor = z.ve->len;
					else
						add_Vertice(
							z.ve,
							&z.cur,
							z.v_index += 1
						);

					z.v = add_Vertice(z.ve, &z.cur, -1),
					z.cur.x += z.ins->pos.x,
					z.cur.y += z.ins->pos.y;
				}

				z.px_hold = 0;
			}
			break;
			case 'e': {
				D2 size = {
					input_num("Size x: "),
					input_num("Size y: ")
				};

				z.cur.x -= z.ins->pos.x,
				z.cur.y -= z.ins->pos.y,
				add_Shape(
					&z.ins->shapes,
					&z.cur,
					&size,
					input_num(
						"Shape"
						"\r\n\n"
						"	1	Rectangle"
						"\r\n\n"
						"	2	Triangle"
						"\r\n\n"
						"	3	Circle"
						"\r\n\n"
						"Index: "
					)
				),
				z.cur.x += z.ins->pos.x,
				z.cur.y += z.ins->pos.y;
			}
			break;

			// Enter = Done
			case 13:
				// Merges the z.vertex for u :3
				if (z.v) {
					size_t ind = get_vertex_index_at_pos(
						z.ve,
						&z.cur,
						z.v_index
					);

					if (ind != (size_t)-1) {
						// Atleast trutly remoz.ve at end
						if (z.v_index == z.ve->len - 1)
							--z.ve->len;
						else
							z.v->pos.x = -1;

						// Reconnect ez.very z.vertex to the one found
						for(size_t i = 0; i < z.ve->len; ++i) {
							Vertice *p = &z.ve->x[i];

							if (p->neighbor == z.v_index)
								p->neighbor = ind;
						}
					}

					z.v = NULL;
				}

				break;

			// Change
			case 'c':
				z.color.r = input_num("RGBA r: "),
				z.color.g = input_num("RGBA g: "),
				z.color.b = input_num("RGBA b: ");

				break;
			case 'C':
				z.ins->color.r = input_num("instance->color r: "),
				z.ins->color.g = input_num("instance->color g: "),
				z.ins->color.b = input_num("instance->color b: ");

				break;
			case 'p':
				z.ins->pos.x = input_num("Position x: "),
				z.ins->pos.y = input_num("Position y: ");

				break;
			// Incredily slowwww: 10 minutes was possible
			case 's': {
				D2 new = { 
					input_num("Size x: "),
					input_num("Size y: "),
				};

				resize_instance(z.ins, &new);
			}
			break;

			/*
				a: Add a empty instance
				A:	 PNG
			*/
			case 'a':
			case 'A': {
				D2 size = {
					input_num("Size x: "),
					input_num("Size y: ")
				};

				z.ve = &(z.ins = add_Instance(
					&z.te.x,
					&z.cur,
					&size,
					&z.color
				))->vertex;

				// Has the relative path from the current path
				if (z.c == 'A')
					input_any("PNG file to load: "),

					z.ins->pixels = read_png(input),
					resize_pixels(&z.ins->pixels, &z.te.ws);
			}
			break;

			case 'n':
			case 'N': {
				_Bool	was_n	= z.c == 'n';
				int	i	= input_num(
					was_n ?	"Select z.instance index: "
					:
						"Delete z.instance index: "
				);

				// Obvious underflow
				if (i < (int)z.te.x.len) {
					if (was_n)
						z.ins = &z.te.x.x[i];
					else {
						remove_instance(&z.te, i);

						if (!z.te.x.len)
							z.ins = add_Instance(
								&z.te.x,
								&z.cur,
								&z.size,
								&z.color
							);
						else
							z.ins = z.te.x.x;
					}
				}
			}
			break;

			// Save me
			case 'z':
				if (!z.name)
					input_any("JSON file to save as: "),
					z.name = input;

				// TODO: More formats to save as
				save_to_json(&z.te, z.name);

				break;

			// Load
			/*
				Requires a placeholder to work correctly, for now u can do it manually via `mv ~/.tegrine/saved/abc`
				A status message could be nice as for information: 'Saved as `abc`'
			*/
			case 'Z':
				if (!z.name)
					input_any("JSON file to load from: "),
					z.name = input;

				// TODO: More formats to load from
				load_from_json(&z.te, z.name);

				break;

			// "Deletes" over the cursor
			case 'd': {
				size_t ind = get_vertex_index_at_pos(
					z.ve,
					&z.cur,
					z.v_index
				);

				if (ind != (size_t)-1) {
					Vertice *v = &z.ve->x[ind];

					v->pos.x = DELETED,
					v->neighbor = -1;

					// Disconnect all
					for(size_t i = 0; i < z.ve->len; ++i)
						if ((v = &z.ve->x[i])->neighbor == ind)
							v->neighbor = -1;
				} else {
					ind = get_pixel_index_at_pos(
						&z.ins->pixels,
						&z.cur
					);

					if (ind != (size_t)-1)
						z.ins->pixels.x[ind].pos.x = DELETED;
				}

				z.v		= NULL,
				z.v_index	= -1;
			}
			break;

			case 'q':
				exit(0);
				
				break;
		}

		if (z.px_hold)
			new_px(&z.ins->pixels, &z.cur, &z.ins->pos, &z.color);
		else if (z.v)
			z.v->pos = z.cur;

		empty_input = 0,
		draw(&z.te, &z.cur);
	}

	free_tegrine(&z.te);
}
