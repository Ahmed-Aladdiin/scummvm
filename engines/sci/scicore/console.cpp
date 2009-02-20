/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $URL$
 * $Id$
 *
 */

/* First part of the console implmentation: VM independent stuff */
/* Remember, it doesn't have to be fast. */

#include "sci/include/sci_memory.h"
#include "sci/include/engine.h"
#ifdef SCI_CONSOLE

int con_passthrough = 0;
FILE *con_file = NULL;

static void(*_con_string_callback)(char*) = NULL;
static void (*_con_pixmap_callback)(gfx_pixmap_t *) = NULL;


/****************************************/
/* sciprintf                            */
/****************************************/


int
sciprintf(const char *fmt, ...) {
	va_list argp;
	size_t bufsize = 256;
	unsigned int i;
	char *buf 	= (char *) sci_malloc(bufsize);

	if (NULL == fmt) {
		error("console.c: sciprintf(): NULL passed for parameter fmt\n");
		return -1;
	}

	if (NULL == buf) {
		error("console.c: sciprintf(): malloc failed for buf\n");
		return -1;
	}

	va_start(argp, fmt);
	while ((i = vsnprintf(buf, bufsize - 1, fmt, argp)) == -1
	        || (i >= bufsize - 2)) {
		/* while we're out of space... */
		va_end(argp);
		va_start(argp, fmt);	/* reset argp */

		free(buf);
		buf = (char *) sci_malloc(bufsize <<= 1);
	}
	va_end(argp);

	if (con_passthrough)
		printf("%s", buf);
	if (con_file)
		fprintf(con_file, "%s", buf);


	if (_con_string_callback)
		_con_string_callback(buf);
	else
		free(buf);

	return 1;
}

void
con_set_string_callback(void(*callback)(char *)) {
	_con_string_callback = callback;
}

void
con_set_pixmap_callback(void(*callback)(gfx_pixmap_t *)) {
	_con_pixmap_callback = callback;
}

int
con_can_handle_pixmaps(void) {
	return _con_pixmap_callback != NULL;
}

int
con_insert_pixmap(gfx_pixmap_t *pixmap) {
	if (_con_pixmap_callback)
		_con_pixmap_callback(pixmap);
	else
		return 1;
	return 0;
}


void
open_console_file(char *filename) {
	if (con_file != NULL)
		fclose(con_file);

	if (NULL == filename) {
		error("console.c: open_console_file(): NULL passed for parameter filename\r\n");
	}
#ifdef WIN32
	con_file = fopen(filename, "wt");
#else
	con_file = fopen(filename, "w");
#endif

	if (NULL == con_file)
		error("console.c: open_console_file(): Could not open output file %s\n", filename);

}

void
close_console_file(void) {
	if (con_file != NULL) {
		fclose(con_file);
		con_file = NULL;
	}
}


#endif /* SCI_CONSOLE */
