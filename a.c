#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <setjmp.h>
#include <errno.h>

#define VERTEX
#define PIXELS
#define SHAPES

jmp_buf jmp_b;
char *g_err;

#define ERR_HANDLE	\
	g_err = message,\
	longjmp(jmp_b, 1);

#include <tegrine/instances.c>
#include <tegrine/term.c>
#include <tegrine/json.c>
#include <reader_png.c>

typedef struct {
	char *x;
	size_t len, max;
} String;

void sub(int *x) {
	*x -= *x ? 1 : 0;
}

void add_l(int *x, int max) {
	*x += *x < max - 1 ? 1 : 0;
}

size_t max_size(size_t x, size_t m) {
	return x > m ? m : x;
}

// :)
void add_s(String *b, char *s) {
	b->len += strlcpy(
		b->x + b->len,
		s,
		max_size(strlen(s) + 1 + b->len, b->max) - b->len
	);
}

void add_c(String *b, char c) {
	if (b->len >= b->max - 1)
		return;

	b->x[b->len++] = c;
}

// TODO: Negative numbers are dead :)
void itoa(String *b, int n) {
	if (b->len >= b->max)
		return;

	/* Alien to humans... well canny to computers :) */
	if (n < 0)
		b->x[b->len++] = '-';

	int a = b->len;

	do
		b->x[b->len++] = n % 10 + 48,
		n /= 10;
	while (n && b->len < b->max);


	/* Reverse >-<	*/
	for(int i = b->len; a < --i; ++a) {
		char c = b->x[a];

		b->x[a] = b->x[i],
		b->x[i] = c;
	}
}


// TODO: [N] Key [Motion]
typedef struct {
	_Bool px_hold;
	char	 c,
		*name;
	String status_bar;
	D2	cur,
		size;
	Instance *ins;
	RGBA color;
	int v_index;
	Tegrine te;
	Vertex *ve;
	Vertice *v;
} Tepevs;

#include "input.c"

// Slow
int get_pixel_index_at_pos(Pixels *px, D2 *pos, Instance *ins) {
	pos->x += ins->pos.x,
	pos->y += ins->pos.y;

	int r = -1;

	for(int i = 0; i < px->len; ++i) {
		Px *p = &px->x[i];

		if (p->pos.x == pos->x && p->pos.y == pos->y) {
			r = 1;

			break;
		}
	}

	pos->x -= ins->pos.x,
	pos->y -= ins->pos.y;

	return r;
}

// Slow
// TODO: Edge selection O_o
int get_vertex_index_at_pos(Vertex *v, D2 *pos, int exclude_i, Instance *ins) {
	pos->x += ins->pos.x,
	pos->y += ins->pos.y;

	int r = -1;

	for(int i = 0; i < v->len; ++i) {
		if (i == exclude_i)
			continue;

		Vertice *ve = &v->x[i];

		if (ve->pos.x == pos->x && ve->pos.y == pos->y) {
			r = 1;

			break;
		}
	}

	pos->x -= ins->pos.x,
	pos->y -= ins->pos.y;

	return r;
}

void new_px(Pixels *px, D2 *cur, D2 *pos, RGBA *color, Instance *ins) {
	int i = get_pixel_index_at_pos(px, cur, ins);

	cur->x -= pos->x,
	cur->y -= pos->y;

	// Exists
	if (i != -1)
		px->x[i] = (Px){ *cur, *color };
	else
		add_Px(px, cur, color);

	cur->x += pos->x,
	cur->y += pos->y;
}

void select_first_instance(Tepevs *z) {
	z->ins = z->te.x.len ?
		&z->te.x.x[0]
	:
		add_Instance(
			&z->te.x,
			&z->cur,
			&z->size
		);
}

/*
	Every single one of those fixes are math ;-;

	TODO: SIGWINCH
	TODO: Help message
	TODO: Flip
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
		.v_index	= -1,
		.status_bar	= { }
	};


	// For loading first :P
	if (setjmp(jmp_b))
		fputs("\x1b[9999H\x1b[2K", stderr),
		perror(g_err),
		exit(-1);

	term_raw();

	if (z.name)
		load_from_json(&z.te, z.name);
	
	// Then we'll catch those errors reselecting if needed L
	if (setjmp(jmp_b))
		printf("\x1b[9999H\x1b[2K%s: %s", g_err, strerror(errno)),
		read(0, &z.c, 1);

	select_first_instance(&z),

	z.ve = &z.ins->vertex,

	set_ws(&z.te.ws),
	draw(&z.te, &z.cur);

	z.status_bar.len = 11,
	/*
	z.status_bar.x = strndup("\x1b[9999H\x1b[2K", z.status_bar.max = z.te.ws.x);
	*/
	z.status_bar.x = malloc(z.status_bar.max = z.te.ws.x),
	strlcpy(z.status_bar.x, "\x1b[9999H\x1b[2K", 12);

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

			case 'G':
				// *Keystrokes* Secure it is :)
				add_s(&z.status_bar, "2D "),
				itoa(&z.status_bar, z.cur.x),
				add_c(&z.status_bar, ','),
				itoa(&z.status_bar, z.cur.y),
				add_s(&z.status_bar, " RGBA "),
				itoa(&z.status_bar, (int)z.color.r),
				add_c(&z.status_bar, ','),
				itoa(&z.status_bar, (int)z.color.g),
				add_c(&z.status_bar, ','),
				itoa(&z.status_bar, (int)z.color.b),
				add_c(&z.status_bar, ','),
				itoa(&z.status_bar, (int)z.color.a);

				break;

			// TODO: Mix get_* into a single function -> <T> obviously void *
			case 'P': {
				int ind = get_vertex_index_at_pos(
					z.ve,
					&z.cur,
					z.v_index,
					z.ins
				);

				if (ind != -1)
					z.color = z.ve->x[ind].color;
				else if ((ind = get_pixel_index_at_pos(
						&z.ins->pixels,
						&z.cur,
						z.ins
				)) != -1)
					z.color = z.ins->pixels.x[ind].color;
			}
			break;

			// Hold drawing Pixels
			case 'H':
				z.px_hold = z.px_hold ? 0 : (z.v = NULL, 1);

				break;
			case ' ':
				new_px(&z.ins->pixels, &z.cur, &z.ins->pos, &z.color, z.ins);

				break;
			case 'v': {
				// TODO: Multiple selection is required to expand this part
				int i = !z.v ?
					get_vertex_index_at_pos(
						z.ve,
						&z.cur,
						z.v_index,
						z.ins
					)
				: -1;

				/*
					Selects existing z.vertex
					Adds z.vertex
				*/
				if (i != -1)
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
							&z.color,
							z.v_index += 1
						);

					z.v = add_Vertice(z.ve, &z.cur, &z.color, -1),
					z.cur.x += z.ins->pos.x,
					z.cur.y += z.ins->pos.y,

					add_s(&z.status_bar, "Added "),
					add_c(&z.status_bar, z.v ? '1' : '2'),
					add_s(&z.status_bar, " vertice; Reconnected "),
					add_c(&z.status_bar, z.v ? '1' : '0');
				}

				z.px_hold = 0;
			}
			break;
			case 'e': {
				D2 size = {
					input_num(&z, "Size x: "),
					input_num(&z, "Size y: ")
				};

				z.cur.x -= z.ins->pos.x,
				z.cur.y -= z.ins->pos.y,
				add_Shape(
					&z.ins->shapes,
					&z.cur,
					&size,
					&z.color,
					input_num(
						&z, 
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
				z.cur.y += z.ins->pos.y,

				add_s(&z.status_bar, "Added shape sized "),
				itoa(&z.status_bar, size.x),
				add_c(&z.status_bar, ','),
				itoa(&z.status_bar, size.y);
			}
			break;

			// Enter = Done
			case 13:
				// Merges the z.vertex for u :3
				if (z.v) {
					int ind = get_vertex_index_at_pos(
						z.ve,
						&z.cur,
						z.v_index,
						z.ins
					);

					if (ind != -1) {
						// Atleast trutly remoz.ve at end
						if (z.v_index == z.ve->len - 1)
							--z.ve->len;
						else
							z.v->pos.x = -1;

						int recon = 0;

						// Reconnect ez.very z.vertex to the one found
						for(int i = 0; i < z.ve->len; ++i) {
							Vertice *p = &z.ve->x[i];

							if (p->neighbor == z.v_index)
								p->neighbor = ind,
								++recon;
						}

						add_s(&z.status_bar, "Reconnected "),
						itoa(&z.status_bar, recon),
						add_s(&z.status_bar, " vertices to "),
						itoa(&z.status_bar, ind),
						add_s(&z.status_bar, "; Deselected vertice");
					}

					z.v = NULL;
				}

				break;

			// Change
			case 'c':
				z.color.r = input_num(&z, "RGBA r: "),
				z.color.g = input_num(&z, "RGBA g: "),
				z.color.b = input_num(&z, "RGBA b: "),
				z.color.a = input_num(&z, "RGBA a: ");

				// Defaults for u <3
				if (!z.color.a)
					z.color.a = 255;

				break;
			case 'p':
				z.ins->pos.x = input_num(&z, "Position x: "),
				z.ins->pos.y = input_num(&z, "Position y: ");

				break;
			/*
				Incredily slowwww: 10 minutes was possible
				Fix: Hash
				Resizal sucks :)
			*/
			case 's': {
				D2 new = { 
					input_num(&z, "Size x: "),
					input_num(&z, "Size y: "),
				};

				resize_instance(z.ins, &new),

				add_s(&z.status_bar, "Resized instance within "),
				itoa(&z.status_bar, z.te.ws.x),
				add_c(&z.status_bar, ','),
				itoa(&z.status_bar, z.te.ws.y);
			}
			break;

			/*
				a: Add a empty instance
				A:	 PNG
			*/
			case 'a':
			case 'A': {
				D2 size = {
					input_num(&z, "Size x: "),
					input_num(&z, "Size y: ")
				};

				z.ve = &(z.ins = add_Instance(
					&z.te.x,
					&z.cur,
					&size
				))->vertex;

				add_s(
					&z.status_bar,
					z.c == 'A' ?
						input_any(&z, "PNG file to load: "),

						z.ins->pixels = read_png(input),
						resize_pixels(&z.ins->pixels, &z.te.ws),

						add_s(&z.status_bar, "Added instance; Loaded PNG file from ./"),

						input
					:
						"Added empty instance"
				);
			}
			break;

			case 'n':
			case 'N': {
				_Bool	was_n	= z.c == 'n';
				int	i	= input_num(
					&z, 
					was_n ?	"Select z.instance index: "
					:
						"Delete z.instance index: "
				);

				if (i < 0)
					break;

				if (i < z.te.x.len) {
					if (was_n)
						z.ins = &z.te.x.x[i],

						add_s(&z.status_bar, "Selected instance "),
						itoa(&z.status_bar, i);
					else {
						remove_Instance(&z.te.x, i),
						add_s(&z.status_bar, "Removed instance "),
						itoa(&z.status_bar, i),
						add_s(&z.status_bar, z.te.x.len ? "; Added; Selected" : "; Selected first instance");

						z.ins = z.te.x.len ?
							z.te.x.x
						:
							add_Instance(
								&z.te.x,
								&z.cur,
								&z.size
							);
					}
				}
			}
			break;

			// Save me
			case 'z':
				if (!z.name)
					input_any(&z, "JSON file to save as: "),
					z.name = input;

				// TODO: More formats to save as: PNG
				save_to_json(&z.te, z.name),

				add_s(&z.status_bar, "Saved to JSON file at $HOME/.tegrine/saved/"),
				add_s(&z.status_bar, z.name);

				break;

			// Load
			// Requires a placeholder to work correctly, for now u can do it manually via `mv ~/.tegrine/saved/abc`
			// TODO: There's a double free somewhere... where i don't belong :3
			case 'Z':
				if (!z.name)
					input_any(&z, "JSON file to load from: "),
					z.name = input;

				// TODO: More formats to load from
				load_from_json(&z.te, z.name),
				select_first_instance(&z),

				add_s(
					&z.status_bar,
					"Freed + Selected first instance + Loaded JSON file from $HOME/.tegrine/saved/"
				),

				add_s(&z.status_bar, z.name);

				break;

			// Deletes over the cursor
			// TODO: Delete shape
			case 'd': {
				int ind = get_vertex_index_at_pos(
					z.ve,
					&z.cur,
					z.v_index,
					z.ins
				);

				if (ind != -1) {
					remove_Vertice(&z.ins->vertex, ind);

					int discon = 0;

					// Disconnect all
					for(int i = 0; i < z.ve->len; ++i) {
						Vertice *v = &z.ve->x[i];

						if (v->neighbor == ind)
							v->neighbor = -1,
							++discon;
					}

					add_s(&z.status_bar, "Removed vertex + Disconnected "),
					itoa(&z.status_bar, discon);
				} else {
					ind = get_pixel_index_at_pos(
						&z.ins->pixels,
						&z.cur,
						z.ins
					);

					if (ind != -1)
						remove_Px(&z.ins->pixels, ind),

						add_s(&z.status_bar, "Removed pixel");
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
			new_px(&z.ins->pixels, &z.cur, &z.ins->pos, &z.color, z.ins);
		else if (z.v)
			z.v->pos = z.cur;

		empty_input = 0,
		draw(&z.te, &z.cur);

		if (z.status_bar.len > 11)
			fwrite(z.status_bar.x, z.status_bar.len, 1, stdout),
			printf("\x1b[%d;%dH", 1 + z.cur.y, 1 + z.cur.x),
			fflush(stdout),
			z.status_bar.len = 11;
	}

	free_tegrine(&z.te),
	free(z.status_bar.x);
}
