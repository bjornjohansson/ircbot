#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "searchidle.h"

#define max(a,b) ((a) > (b) ? (a) : (b))
#define min(a,b) ((a) < (b) ? (a) : (b))

int searchidle(const char *file,
	       const char *nick,
	       long *timestamp, 
	       char *line,
	       long linelength)
{
  int mapsize,pagesize,i,j,k;
  off_t offset=0;
  char *map;
  int found=0;
  struct stat logstat;
  FILE *log=fopen(file,"r");
  if ( log == NULL ) {
    return 1;
  }
      
  if ( fstat(fileno(log),&logstat) == -1 ) {
    fclose(log);
    return 1;
  }
  pagesize=getpagesize();
  if ( LEAST_MMAP_WINDOW < pagesize ) {
    fprintf(stderr,"This binary is compiled with a too small mmap window.\n");
    fprintf(stderr,"Very bad things can and probably will happen.\n");
  }

  mapsize=pagesize*(int)(LEAST_MMAP_WINDOW/pagesize);
  offset=logstat.st_size-mapsize;
  if ( offset < 0 ) offset=0;
  if ( (offset % pagesize) != 0 && (mapsize+offset) > logstat.st_size ) {
    offset-=offset % pagesize;
  } else if ( (offset % pagesize) != 0 ) {
    offset+=pagesize - (offset % pagesize);
  }
  if ( (mapsize + offset) > logstat.st_size ) {
    mapsize=logstat.st_size-offset;
  }

  while ( found == 0 && offset >= 0) {
    map=(char*)mmap(NULL,mapsize,PROT_READ,MAP_SHARED,fileno(log),offset);
    if ( map == MAP_FAILED ) {
      fclose(log);
      return 1;
    }
    for(i=mapsize-1;i>=0 && found == 0;i--) {
      if ( map[i] == '\n' || ( i == 0 && offset == 0 )) {
	int ts_begin=i+1,ts_end;
	j=1;
	while ( (i+j) < mapsize &&
		( (map[i+j] >= '0' && map[i+j] <= '9') || map[i+j] == ':' ) )
	  j++;
	ts_end=i+j;
	while ( (i+j+1) < mapsize 
		&& map[i+j] != ':' 
		&& map[i+j+1] != ' '
		&& map[i+j] != '\n')
	  j++;
	if ( (i+j) >= mapsize || map[i+j] == '\n' )
	  continue;
	
	j+=2;
	if ( (i+j+strlen(nick)+1) < mapsize && map[i+j] == '<' ) {
	  if ( !strncmp(nick,&map[i+j+1],strlen(nick)) 
	       && map[i+j+1+strlen(nick)] == '>') {
	    j+=1+strlen(nick)+2;
	    if ( (i+j) < mapsize && map[i+j-1] == ' ')
	    {
	      if ( map[ts_begin+2] == ':' ) {
		time_t ts=time(NULL);
		struct tm *tms=localtime(&ts);
		tms->tm_hour=atoi(&map[ts_begin]);
		tms->tm_min=atoi(&map[ts_begin+3]);
		ts=mktime(tms);
		while ( ts >= time(NULL) ) {
		  ts-=60*60*24;
		}
		*timestamp=ts;
	      } else {
		strncpy(line,&map[ts_begin],min(ts_end-ts_begin,linelength));
		line[ts_end-ts_begin]=0;
		*timestamp=atoi(line);
	      }
	      
	      k=0;
	      while ( map[i+j+k] != '\n' 
		      && (i+j+k) < mapsize 
		      && k < (linelength-1)) {
		line[k]=map[i+j+k];
		k++;
	      }
	      line[k]=0;
	      found=1;
	    }
	  }
	}
      }
    }
    if ( munmap(map,mapsize) == -1 ) {
      perror("unmap failed");
      return 1;
    }
    if ( offset == 0 && found == 0 ) {
      fclose(log);
      return 1;
    }
    /* Overlap with one page, in case the maps cut the relevant line in half */
    mapsize=pagesize*(int)(LEAST_MMAP_WINDOW/pagesize);
    offset-=mapsize+pagesize;
    if ( offset < 0 )
      offset=0;
    if ( (mapsize + offset) > logstat.st_size ) {
      mapsize=logstat.st_size-offset;
    }
  }
  fclose(log);
  if ( found == 1 ) {
    return 0;
  }
  return 1;
}

