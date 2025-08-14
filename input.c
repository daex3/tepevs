_Bool empty_input = 0;
char input[1024];

// Define a default value as a parameter, print that as fainted text(placeholder) and while not empty clear it
// TODO: Limit the message (x,y)
void input_any(Tepevs *x, char *message) {
	add_s(&x->status_bar, message),
	fputs(x->status_bar.x, stdout),
	fflush(stdout),
	x->status_bar.len = 11;

	int i = 0;
	char c;

	while (i < 1024 && read(0, &c, 1) != -1) {
		switch (c) {
			case 13:
				goto hell;

			case '\b':
				--i;

				break;
		}

		input[i++] = c,
		putchar(isspace(c) ? ' ' : c),
		fflush(stdout);
	}
	
	hell:
	input[i] = 0;
}

// TODO: Backspace
int input_num(Tepevs *x, char *message) {
	if (empty_input)
		return 0;

	input_any(x, message);

	if (!*input)
		empty_input = 1;
	
	return atoi(input);
}
