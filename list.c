#include "umenu.h"

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

void
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
  item_t *item = NULL;
  int nbw;
  int i;

  go_y(l, ly);
  fprintf(l->f_out, "\r");
  nbw = 0;
  if (GET_ITEM_I(l, ly) < l->nb_items)
    {
      item = GET_ITEM(l, ly);
      if (item == GET_CUR_ITEM(l))
	nbw += fprintf(l->f_out, "+"); /*!!! penser à calibre width */
      else
	nbw += fprintf(l->f_out, " "); /* le jour où on modifie  */
      
      if (item->selected)
	nbw += fprintf(l->f_out, "%c * ", l->prefix[ly]); /* ça aussi */
      else
	nbw += fprintf(l->f_out, "%c - ", l->prefix[ly]);

      fprintf(l->f_out, "%.*s", item->s_len_used, item->s);
      nbw += item->w_cols_used;
  }
  if (nbw < l->nb_colsw[ly]) {
    /* fprintf(l->f_out, "%0*c", l->nb_colsw[ly] - nbw, ' '); */
    i = l->nb_colsw[ly] - nbw;
    while (i--)
      fprintf(l->f_out, "%s", l->dcstr);
  }
  l->nb_colsw[ly] = nbw;

  fprintf(l->f_out, "\n%s", l->upstr);
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
    l->items[l->item_top_page + y]->selected = 
      !l->items[l->item_top_page + y]->selected;
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

static void
calcul_item_width(list_t *l, item_t *item)
{
  int len;
  int cols_width;
  wchar_t *tmp;

  len = item->w_len;
  while ((cols_width = wcswidth(item->ws, len)) > l->col_width - l->pre_width)
    --len;

  item->w_len_used = len;
  item->w_cols_used = cols_width;
  tmp = item->ws;
  item->s_len_used = wcsnrtombs(NULL, &tmp,
				item->w_len_used, item->s_len, NULL);
}

void
calibre_width(list_t *l)
{
  int i;
  int biggest = 0;
  int tmp = 0;

  for (i = 0; i != l->items_per_page; i++) {
    tmp = wcwidth(l->prefix[i]);
    if (tmp > biggest)
      biggest = tmp;
  }
  biggest += 4; /* !!! ok tant qu'on affiche en statique le + %s - %s */
  
  l->pre_width = biggest + 1;
  for (i = 0; i != l->nb_items; i++)
    calcul_item_width(l, l->items[i]);
}

item_t *
create_item(list_t *l, const char *s)
{
  wchar_t *ws;
  size_t len;  
  item_t *item;

  item = malloc(sizeof(item_t));

  item->s_len = strlen(s);
  item->s = strdup(s);
  ws = calloc(item->s_len+1, MB_CUR_MAX); /* MB_CUR_MAX || sizeof(wchar_t) ? */
  item->w_len = mbstowcs(ws, s, item->s_len);
  item->ws = ws;

  item->selected = 0;
  calcul_item_width(l, item);

  return item;
}

size_t
add_item(list_t *l, item_t *item)
{
  int item_n;

  item_n = l->nb_items;
  if (l->alloc_items <= item_n) {
    l->alloc_items += l->items_per_page;
    l->items = realloc(l->items, l->alloc_items * sizeof(item_t *));
  }

  l->items[item_n] = item;
  ++l->nb_items;
  return item_n;
}

void
remove_item(list_t *l, size_t item)
{
  if (item >= l->nb_items)
    return ;

  free(l->items[item]->ws);
  free(l->items[item]->s);
  free(l->items[item]);
  while (item+1 < l->nb_items)
    {
      memcpy(l->items + item, l->items + item + 1, sizeof(item_t *));
      ++item;
    }
    --l->nb_items;

    if (l->alloc_items - l->nb_items > l->items_per_page) {
      l->alloc_items -= l->items_per_page;
      l->items = realloc(l->items, l->alloc_items * sizeof(item_t *));
    }
}

void
clear_items(list_t *l)
{
  while (l->nb_items)
    remove_item(l, 0);
}

void
set_items(list_t *l, char **items, size_t nb_items)
{
  size_t i;
  
  clear_items(l);
  for (i = 0; i != nb_items; i++)
    add_item(l, create_item(l, items[i]));
}
