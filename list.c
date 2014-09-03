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
curseek(list_t *l, int nb)
{
  if (l->cur_item + nb < 0)
    l->cur_item = 0;
  else if (l->cur_item + nb >= l->nb_items)
    return;
  else
    l->cur_item += nb;
}

void
draw_bar(list_t *l)
{
  int cur_p;
  int nb_p;
  int nb_items_per_page = l->nb_items > l->nb_items_per_page ? l->nb_items_per_page : l->nb_items;

  cur_p = l->cur_item / nb_items_per_page;
  if (l->cur_item % nb_items_per_page)
    ++cur_p;

  nb_p = l->nb_items / nb_items_per_page;
  if (l->nb_items % nb_items_per_page)
    ++nb_p;
  fprintf(l->f_out, "Page %d/%d", cur_p+1, nb_p);
}

void
draw_items(list_t *l, const int *pre)
{
  int i;
  int max;
  int nbw;
  int nb_items_per_page = l->nb_items > l->nb_items_per_page ? l->nb_items_per_page : l->nb_items;

  if (l->cur_item + nb_items_per_page > l->nb_items)
    max = l->nb_items - l->cur_item;
  else
    max = nb_items_per_page;
  for (i = 0; i != max; i++)
    {
      fprintf(l->f_out, "%c - %s", pre[i], l->items[l->cur_item + i].s);
      nbw = l->items[l->cur_item + i].colsneed;
      if (nbw < l->count_charw[i])
	fprintf(l->f_out, "%0*c", l->count_charw[i] - nbw, ' ');
      l->count_charw[i] = nbw;

      fprintf(l->f_out, "\n");
    }
  while (i < nb_items_per_page) {
    fprintf(l->f_out, "    %0*c\n", l->count_charw[i], ' ');
    ++i;
  }
}

void
cursor_top(list_t *l, const char *upstr)
{
  int t;
  int nb_items_per_page = l->nb_items > l->nb_items_per_page ? l->nb_items_per_page : l->nb_items;

  fprintf(l->f_out, "\r");
  t = nb_items_per_page;
  while (t--) {
      fprintf(l->f_out, "%s", upstr);
  }

  fflush(l->f_out);
  /* fprintf(l->f_out, "%0*s\r", l->nb_items_per_page, upstr); */
}

void
set_nb_items_per_page(list_t *l, size_t nb)
{
  int diff;

  l->count_charw = realloc(l->count_charw, sizeof(size_t) * nb);

  if (nb > l->nb_items_per_page) {
    diff = nb - l->nb_items_per_page;
    while (diff--)
      l->count_charw[l->nb_items_per_page + diff] = 0;
  }

  l->nb_items_per_page = nb;
}

size_t
add_item(list_t *l, char *s)
{
  wchar_t *ws;
  size_t item_n;
  size_t len;  

  item_n = l->nb_items;
  if (l->alloc_items <= item_n) {
    l->alloc_items += l->nb_items_per_page;
    l->items = realloc(l->items, l->alloc_items * sizeof(item_t));
  }

  /* strdup ?? */
  l->items[item_n].s = s;

  ws = calloc(strlen(s)+1, sizeof(wchar_t));
  len = mbstowcs(ws, s, strlen(s));
  len = wcswidth(ws, len);
  l->items[item_n].colsneed = len;
  free(ws);

  l->items[item_n].len = strlen(l->items[item_n].s);

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

    if (l->alloc_items - l->nb_items > l->nb_items_per_page) {
      l->alloc_items -= l->nb_items_per_page;
      l->items = realloc(l->items, l->alloc_items * sizeof(item_t));
    }
}

void
set_items(list_t *l, char **items, size_t nb_items)
{
  size_t i;
  
  while (l->nb_items)
    remove_item(l, 0);
  for (i = 0; i != nb_items; i++)
    add_item(l, items[i]);
}
