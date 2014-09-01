#ifndef __LIST_H__
#define __LIST_H__

#include <locale.h>
#include <wchar.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct
{
  char *s;
  size_t len;
  size_t colsneed;
} item_t;

typedef struct
{
  FILE *f_out;
  item_t *items;
  size_t nb_items;
  size_t *count_charw; /* futurement last_line_nbw  */
  size_t alloc_items;
  size_t cur_item;
  size_t nb_items_per_page;
} list_t;

#endif
