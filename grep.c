/*
 * UNG's Not GNU
 * 
 * Copyright (c) 2011, Jakob Kaivo <jakob@kaivo.net>
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <regex.h>

// GNU libc unsets ARG_MAX
#ifndef ARG_MAX
# define ARG_MAX _POSIX_ARG_MAX
#endif


const char *grep_desc = "search a file for a pattern";
const char *grep_inv  = "grep [-E|-F] [-c|-l|-q] [-insvx] -e pattern_list [-e pattern_list]... [-f pattern_file]... [file...]\ngrep [-E|-F] [-c|-l|-q] [-insvx] [-e pattern_list]... -f pattern_file [-f pattern_file]... [file...]\ngrep [-E|-F] [-c|-l|-q] [-insvx] pattern_list [file...]";

#define BASIC		0
#define EXTENDED	1
#define FIXED		2

#define NORMAL	0
#define COUNT	1
#define LIST	2
#define QUIET	3

#define IGNORECASE	1 << 0
#define LINENUMBERS	1 << 1
#define SUPPRESS	1 << 2
#define INVERT		1 << 3
#define WHOLELINE	1 << 4
#define FILENAMES	1 << 5

static int grep (int patterns, char **p, int type, int display, int flags, char *path)
{
  FILE *f = stdin;
  char *buf = NULL;
  int line = 0;
  size_t nmatch;
  size_t len = 0;
  size_t nread;
  regmatch_t pmatch[1];
  regex_t re;
  int i;
  int count = 0;
  struct stat st;

  stat (path, &st);
  if (S_ISDIR(st.st_mode))
    return 0;

  if (strcmp ("-", path))
    f = fopen (path, "r");

  if (f == NULL) {
    if (! (flags & SUPPRESS))
      perror (path);
    return 0;
  }

  while (!feof (f)) {
    if ((nread = getline (&buf, &len, f)) != -1) {
      line++;
      for (i = 0; i < patterns; i++) {
        // FIXME: fixed pattern
        if (regcomp (&re, p[i], (type == BASIC ? 0 : REG_EXTENDED) | (flags & IGNORECASE ? REG_ICASE : 0)) != 0) {
          printf ("Bad regex: %s\n", p[i]);
          exit(2);
        } else if (regexec (&re, buf, 1, pmatch, 0) == (flags & INVERT ? REG_NOMATCH : 0)) {
          if (!(flags & WHOLELINE) || (flags & WHOLELINE && pmatch[0].rm_so == 0 && pmatch[0].rm_eo == nread - 1)) {
            count++;
            if (NORMAL == display) {
              if (flags & FILENAMES) printf ("%s:", path);
              if (flags & LINENUMBERS) printf ("%d:", line);
              fwrite (buf, sizeof(char), nread, stdout);
            }
          }
        }
        regfree (&re);
      }
      if (buf != NULL) { free (buf); buf = NULL; }
      len = 0;
    }
  }

  if (strcmp ("-", path))
    fclose (f);

  if (count > 0 && LIST == display) {
    if (strcmp ("-", path))
      printf ("%s\n", path);
    else
      printf ("(standard input)\n");
  } else if (COUNT == display) {
    if (flags & FILENAMES) printf ("%s:", path);
    printf ("%d\n", count);
  }

  return count;
}

int
main (int argc, char **argv)
{
  int c;
  int n = 10;
  int showname = 0;
  char *end;
  int type = BASIC;
  int display = NORMAL;
  int flags = 0;
  int patterns = 0;
  int pfiles = 0;
  char *p[ARG_MAX];
  char *pf[ARG_MAX];
  int found = 0;

  while ((c = getopt (argc, argv, ":EFclqinsvxe:f:")) != -1) {
    switch (c) {
      case 'E':
        type = EXTENDED;
        break;
      case 'F':
        type = FIXED;				// FIXME
        break;
      case 'c':
        display = COUNT;
        break;
      case 'e':
        p[patterns++] = optarg;
        break;
      case 'f':
        pf[pfiles++] = optarg;
        break;
      case 'i':
        flags |= IGNORECASE;
        break;
      case 'l':
        display = LIST;
        break;
      case 'n':
        flags |= LINENUMBERS;
        break;
      case 'q':
        display = QUIET;
        break;
      case 's':
        flags |= SUPPRESS;
        break;
      case 'v':
        flags |= INVERT;
        break;
      case 'x':
        flags |= WHOLELINE;
        break;
      default:
        fprintf (stderr, "Usage: %s\n", grep_inv);
        return 2;
    }
  }

  if (pfiles > 0) {
    // FIXME: read in the files
  }

  if (patterns == 0 && optind >= argc) {
    return 2;
  } else if (patterns == 0)
    p[patterns++] = argv[optind++];

  if (optind >= argc)
     found = grep (patterns, p, type, display, flags, "-");
  else {
    if (argc - optind > 1) flags |= FILENAMES;
    while (optind < argc)
       found += grep (patterns, p, type, display, flags, argv[optind++]);
  }
  
  return (found == 0 ? 1 : 0);
}

