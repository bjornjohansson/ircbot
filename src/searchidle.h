#ifndef _SEARCHIDLE_H
#define _SEARCHIDLE_H

/* Map at least four MB, if this value happens to be lower than the
   pagesize of the system things will go wrong. */
#define LEAST_MMAP_WINDOW (4<<20)

#ifdef __cplusplus
extern "C" {
#endif

int searchidle(const char *file,
	       const char *nick,
	       long *timestamp, 
	       char *line,
	       long linelength);

#ifdef __cplusplus
}
#endif

#endif
