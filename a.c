// Did u know? This entire module is just initialization

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

#include "input.c"

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
	z.status_bar.x = malloc(z.status_bar.max = z.te.ws.x),
	strlcpy(z.status_bar.x, "\x1b[9999H\x1b[2K", 12),

	do_input(&z),

	free_tegrine(&z.te),
	free(z.status_bar.x);
}
