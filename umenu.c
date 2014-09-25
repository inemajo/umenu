#include <stdlib.h>
#include <termcap.h>

#include <term.h>

#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>

#include <locale.h>
#include <wchar.h>
#include <string.h>

#include "list.h"
static list_t l = {0};
static list_t def = {0};

#ifdef unix
static char termbuf[2048];
#else
#define termbuf 0
#endif

static char *bp;
static char *upstr;
static char *mrstr;
static char *mestr;


static int fd_in;
/* static FILE *f_out; */
static int fd_result;

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
  //return mbstowcs(NULL, s, 0) + 1;
}
*/

void
select_item(int item)
{
  int len;

  if (item >= l.nb_items || item < 0)
    return ;
  fprintf(l.f_out, "\n");
  fflush(l.f_out);

  (void)write(fd_result, l.items[item].s, l.items[item].len);
}

int
terminit()
{
  char *term;
  
  if ((term = getenv("TERM")) == NULL)
    {
      perror("can't determine terminal\n");
      exit(1);
    }
  if (tgetent(termbuf, term) != 1)
    {
      perror("problem with tgetent\n");
      exit(1);
    }

#ifdef unix
  /* Here we assume that an explicit termbuf
     was provided to tgetent.  */
  bp = (char *) malloc (strlen (termbuf));
#else
  bp = NULL;
#endif
  upstr = tgetstr("up", &bp);
  mrstr = tgetstr("mr", &bp);
  mestr = tgetstr("me", &bp);
}

int
init_locale()
{
  char *locale_use;

  if ((locale_use = getenv("LC_ALL")) == NULL)
    if ((locale_use = getenv("LANG")) == NULL)
      if ((locale_use = getenv("LC_CTYPE")) == NULL)
	locale_use = "en_US.UTF-8"; /* By default  */

  if (setlocale(LC_ALL, locale_use) == NULL) {
    perror("setlocale");
    exit(EXIT_FAILURE);
  }
}

int
has_c(int c, const int *clist)
{
  int i;

  for (i = 0; clist[i] != 0 && clist[i] != c; i++);

  if (clist[i] == 0)
    return -1;

  return i;
}


#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
void
search()
{
  char *result;
  size_t i;

  fprintf(l.f_out, "\r");
  fflush(l.f_out);
  result = readline("Search? ");
  /* the spaces corresponding to strlen("Search? ") */
  fprintf(l.f_out, "%s        %0*c", upstr, strlen(result), ' '); /* bar  */
  if (strlen(result) == 0) {
    copy_items(&l, &def);
    return ;
  }

  /* cursor_top(&l, upstr); */
  clear_items(&l);
  i = 0;
  while (i < def.nb_items)
    {
      if (strcasestr(def.items[i].s, result) != NULL)
	add_item(&l, def.items[i].s);
      i++;
  }
  if (l.nb_items == 0) {
    i = fprintf(l.f_out, "\r\"%s\" Not found!", result);
    fflush(l.f_out);
    fprintf(l.f_out, "\r%0*c", i, ' ');
    copy_items(&l, &def);
    sleep(1);
  }
}

void
usage()
{
  fprintf(stderr, "USAGE: umenu [-r -o -l] item1 item2...\n\
\t-r : select file descriptor to display selected item (by default 2)\n\
\t-o : select file descriptor to display list (by default 1)\n\
\t-l : select nb item per page (by default 10)\n\
EXAMPLE: echo response $(umenu -r1 -o2 -l3 yes no \"why not\")\n");
  exit(1);
}

int
main(int ac, char **av)
{
  int c;
  struct termios oldt, newt;
  int ch;
  int i;
  int count_items;
  char **items;
  int shortcut[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', 'a', 'b',
		    'c', 'd', 'e', 'f', 'g', 'h', 'i', 'k', 'l', 'm', 'n', 'o',
		    'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', 0};
  int shortcut_len = sizeof shortcut / sizeof(int);
  int l_num;
  int nb_items_per_page = 10;

  if (ac == 1)
    usage();

  fd_result = STDERR_FILENO;
  init_locale();

  l.f_out = stdout;  
  i = 1;
  while (i < ac && av[i][0] == '-') {
    if (av[i][1] == 'r') {
      fd_result = atoi(av[i]+2);
    }
    else if (av[i][1] == 'o') {
      l.f_out = fdopen(atoi(av[i]+2), "w");
    }
    else if (av[i][1] == 'l') {
      nb_items_per_page = atoi(av[i]+2);
    }
    ++i;
  }

  items = av + i;
  count_items = ac - i;
  if (count_items == 0)
    return (1);

  if (nb_items_per_page <= 0 || nb_items_per_page >= shortcut_len)
    return 1;
  set_nb_items_per_page(&l, nb_items_per_page);
  set_items(&l, items, count_items);
  set_nb_items_per_page(&def, nb_items_per_page);
  set_items(&def, items, count_items);

  tcgetattr( STDIN_FILENO, &oldt );
  newt = oldt;
  newt.c_lflag &= ~( ICANON | ECHO );
  tcsetattr( STDIN_FILENO, TCSANOW, &newt);
  terminit();

  /* fd_in = STDIN_FILENO */
  draw_items(&l, shortcut);
  fflush(l.f_out);
  while ((c = getchar()) != 'q')
    {
      if (c == '/') {
	tcsetattr( STDIN_FILENO, TCSANOW, &oldt);
	search();
	l.cur_item = 0;
	tcsetattr( STDIN_FILENO, TCSANOW, &newt);
      }
      if (c == 'n' || c == ' ' || c == 'j')
	curseek(&l, nb_items_per_page);
      else if (c == 'p' || c == 'k')
	curseek(&l, -nb_items_per_page);
      else if ((l_num = has_c(c, shortcut)) >= 0)
	{
	  select_item(l_num + l.cur_item);
	  break;
	}
      cursor_top(&l, upstr);
      draw_items(&l, shortcut);
      fflush(l.f_out);
    }
  tcsetattr( STDIN_FILENO, TCSANOW, &oldt);

  while (l.nb_items)
    remove_item(&l, l.nb_items-1);
  free(l.items);
  free(l.count_charw);
  _exit(0);
  return 0;
}
