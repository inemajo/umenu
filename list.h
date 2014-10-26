#ifndef __LIST_H__
#define __LIST_H__

#include <locale.h>
#include <wchar.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#define GET_ITEM_I(l, y) (l->item_top_page + y)
#define GET_ITEM(l, y) (l->items[GET_ITEM_I(l, y)])
#define GET_CUR_ITEM(l) GET_ITEM(l, l->cursor_y)
#define GET_CUR_ITEM_I(l) GET_ITEM_I(l, l->cursor_y)

#define ABSOLUTE 0
#define RELATIVE 1

typedef struct
{
  wchar_t *ws;
  size_t w_len;
  size_t w_len_used;
  size_t w_cols_used;

  char *s;
  size_t s_len;
  size_t s_len_used;
  int selected;
} item_t;

typedef struct
{
  FILE *f_out;

  item_t **items;
  size_t nb_items;

  size_t *nb_colsw;

  size_t alloc_items;

  size_t cur_page;
  size_t item_top_page;
  size_t items_per_page;

  size_t pre_item_sz; /* taille des chars avant l'item */

  size_t col_width;
  size_t pre_width;

  size_t cursor_y;

  char bottom_msg[16];
  size_t bottom_msg_col_s;

  int *prefix;
  int y;
  char *upstr;
  char *dcstr;
} list_t;


int draw_bottom(list_t *l);
int set_bottom_msg(list_t *l, const char *format, ...);
void page_seek(list_t *l, int relative, int nb);
void change_cursor_y(list_t *l, int new_y);
void cursor_seek(list_t *l, int relative, int nb);
int draw_bar(list_t *l);
void draw_line(list_t *l, int ly);
void draw_items(list_t *l);
void select_item(list_t *l, int y);
void clear_win(list_t *l);
void set_items_per_page(list_t *l, size_t nb);
size_t add_item(list_t *l, item_t *item);
item_t *create_item(list_t *l, const char *s);
void remove_item(list_t *l, size_t item);
void clear_items(list_t *l);
size_t copy_items(list_t *dst, list_t *src);
void set_items(list_t *l, char **items, size_t nb_items);
#endif
