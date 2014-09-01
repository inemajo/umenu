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
main(int ac, char **av)
{
  int c;
  struct termios oldt, newt;
  int ch;
  int i;
  int count_items;
  char **items;

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
    ++i;
  }

  items = av + i;
  count_items = ac - i;
  if (count_items == 0)
    return (1);

  set_nb_items_per_page(&l, 10);
  set_items(&l, items, count_items);

  tcgetattr( STDIN_FILENO, &oldt );
  newt = oldt;
  newt.c_lflag &= ~( ICANON | ECHO );
  tcsetattr( STDIN_FILENO, TCSANOW, &newt);
  terminit();

  /* fd_in = STDIN_FILENO */
  draw_items(&l);
  draw_bar(&l);
  fflush(l.f_out);
  while ((c = getchar()) != 'q')
    {
      if (c == 'n' || c == ' ' || c == 'j')
	curseek(&l, 10);
      else if (c == 'p')
	curseek(&l, -10);
      else if (c >= '0' && c <= '9')
	{
	  if (c == '0')
	    c = '9'+1; /* car 0 == 10 et pas 0 */
	  select_item(c-'0'-1 + l.cur_item);
	  break;
	}
      cursor_top(&l, upstr);
      draw_items(&l);
      draw_bar(&l);
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
