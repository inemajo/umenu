#include "umenu.h"

/* size_t
s_col_len(const char *s)
{
  size_t len;
  wchar_t *ws;

  ws = calloc(strlen(s), sizeof(wchar_t));
  len = mbstowcs(ws, s, strlen(s));
  len = wcswidth(ws, len);
  free(ws);
  return len;
  \*  return mbstowcs(NULL, s, 0) + 1;*\
}
*/

/* static size_t */
/* wcnbcol(const char *ws) */
/* { */
/*   size_t len; */

/*   len = wcswidth(ws, wcslen(ws)); */
/*   return len; */
/* } */

void
page_seek(list_t *l, int relative, int nb)
{
  if (relative)
    nb += l->cur_page;

  if (nb < 0)
    nb = l->nb_items / l->items_per_page;
  else if (nb * l->items_per_page > l->nb_items)
    nb = 0;

  l->cur_page = nb;
  l->item_top_page = l->cur_page * l->items_per_page;

  if (l->item_top_page + l->cursor_y >= l->nb_items)
    l->cursor_y -= l->item_top_page + l->cursor_y - l->nb_items + 1;
  draw_items(l);
}

void
change_cursor_y(list_t *l, int new_y)
{
  int old_y = l->cursor_y;

  if (old_y == new_y || new_y >= l->items_per_page)
    return ;

  l->cursor_y = new_y;
  draw_line(l, old_y);
  draw_line(l, l->cursor_y);
}

void
cursor_seek(list_t *l, int relative, int nb)
{
  int new_page;
  int new_y;

  if (relative)
    nb += l->cursor_y + l->item_top_page;

  if (nb < 0)
    nb = l->nb_items - 1;
  else if (nb >= l->nb_items)
    nb = 0;

  new_page = nb / l->items_per_page;
  new_y = nb % l->items_per_page;

  if (new_page != l->cur_page) {
    l->cursor_y = new_y;
    page_seek(l, 0, new_page);
  }
  else if (new_y != l->cursor_y)
    change_cursor_y(l, new_y);
}

static void
go_y(list_t *l, int ly)
{
  while (l->y < ly) {
    fprintf(l->f_out, "\n");
    ++l->y;
  }
  while (l->y > ly) {
    fprintf(l->f_out, "%s", l->upstr);
    --l->y;
  }
}

int
set_bottom_msg(list_t *l, const char *format, ...)
{
  va_list ap;

  va_start(ap, format);
  vsnprintf(l->bottom_msg, 15, format, ap);
  va_end(ap);
  draw_bottom(l);
}

void
special(list_t *l)
{
  go_y(l, l->items_per_page);
  if (l->bottom_msg_col_s) {
    fprintf(l->f_out, "\r%0*c", l->bottom_msg_col_s, ' ');
    l->bottom_msg_col_s = 0;
  }
}

int
draw_bottom(list_t *l)
{
  int cur_p = l->cur_page;
  int nb_p;
  int nbw;

  nb_p = l->nb_items / l->items_per_page;
  if (l->nb_items % l->items_per_page)
    ++nb_p;
  go_y(l, l->items_per_page);

  if (l->bottom_msg[0] != '\0')
    nbw = fprintf(l->f_out, "\rPage %d/%d %s", cur_p+1, nb_p, l->bottom_msg);
  else
    nbw = fprintf(l->f_out, "\rPage %d/%d", cur_p+1, nb_p);

  while (l->bottom_msg_col_s > nbw) {
    fprintf(l->f_out, " ");
    --l->bottom_msg_col_s;
  }
  l->bottom_msg_col_s = nbw;
  return 0;
}

void
draw_line(list_t *l, int ly)
{
  item_t *item;
  int nbw;

  go_y(l, ly);
  fprintf(l->f_out, "\r");
  nbw = 0;
  if (GET_ITEM_I(l, ly) < l->nb_items)
    {
      item = GET_ITEM(l, ly);
      if (item == GET_CUR_ITEM(l))
	nbw += fprintf(l->f_out, "+");
      else
	nbw += fprintf(l->f_out, " ");
      
      if (item->selected)
	nbw += fprintf(l->f_out, "%c * ", l->prefix[ly]);
      else
	nbw += fprintf(l->f_out, "%c - ", l->prefix[ly]);
      
      fprintf(l->f_out, "%s", item->s);
      nbw += item->colsneed;
  }
  if (nbw < l->nb_colsw[ly])
    fprintf(l->f_out, "%0*c", l->nb_colsw[ly] - nbw, ' ');
  l->nb_colsw[ly] = nbw;
}

void
draw_items(list_t *l)
{
  int i;

  for (i = 0; i < l->items_per_page; i++)
    draw_line(l, i);
  draw_bottom(l);
}

void
select_item(list_t *l, int y)
{
  if (l->item_top_page + y < l->nb_items)
    l->items[l->item_top_page + y].selected = !l->items[l->item_top_page + y].selected;
}

void
clear_win(list_t *l)
{
  int t;

  go_y(l, 0);
  for (t = 0; t != l->items_per_page; t++)
    {
      fprintf(l->f_out, "\r%0*c", l->nb_colsw[t], ' ');
      go_y(l, t);
      l->nb_colsw[t] = 0;
    }

  fflush(l->f_out);
}

void
set_items_per_page(list_t *l, size_t nb)
{

  l->nb_colsw = realloc(l->nb_colsw, sizeof(size_t) * nb);
  while (l->items_per_page < nb)
    {
      l->nb_colsw[l->items_per_page] = 0;
      ++l->items_per_page;
    }
  /* Faire disparaitre les anciennes lignes  */
}

size_t
add_item(list_t *l, char *s)
{
  wchar_t *ws;
  size_t item_n;
  size_t len;  

  item_n = l->nb_items;
  if (l->alloc_items <= item_n) {
    l->alloc_items += l->items_per_page;
    l->items = realloc(l->items, l->alloc_items * sizeof(item_t));
  }

  l->items[item_n].s = s;

  ws = calloc(strlen(s)+1, sizeof(wchar_t));
  len = mbstowcs(ws, s, strlen(s));
  len = wcswidth(ws, len);
  l->items[item_n].colsneed = len;
  free(ws);

  l->items[item_n].len = strlen(l->items[item_n].s);

  l->items[item_n].selected = 0;
  ++l->nb_items;
  return item_n;
}

void
remove_item(list_t *l, size_t item)
{
  if (item >= l->nb_items)
    return ;

  while (item+1 < l->nb_items)
    {
      memcpy(l->items + item, l->items + item + 1, sizeof(item_t));
      ++item;
    }
    --l->nb_items;

    if (l->alloc_items - l->nb_items > l->items_per_page) {
      l->alloc_items -= l->items_per_page;
      l->items = realloc(l->items, l->alloc_items * sizeof(item_t));
    }
}

void
clear_items(list_t *l)
{
  while (l->nb_items)
    remove_item(l, 0);
}


size_t
copy_items(list_t *dst, list_t *src)
{
  size_t i;

  clear_items(dst);
  for (i = 0; i < src->nb_items; i++) {
    add_item(dst, src->items[i].s);
  }
}

void
set_items(list_t *l, char **items, size_t nb_items)
{
  size_t i;
  
  clear_items(l);
  for (i = 0; i != nb_items; i++)
    add_item(l, items[i]);
}
