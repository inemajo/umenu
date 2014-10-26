#define  _POSIX_SOURCE
#include <stdlib.h>
#include <termcap.h>
#include <stdio.h>

#include <term.h>
#include <sys/ioctl.h>

#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>

#include <locale.h>
#include <wchar.h>
#include <string.h>

#include <signal.h>

#include "list.h"
static list_t l = {0};
static item_t **saved_items = NULL;
static size_t saved_items_len = 0;

static int nb_items_per_page = 10;
static char *separator = "\n";
static struct termios oldt, newt;
static struct winsize winsz;

#ifdef unix
static char termbuf[2048];
#else
#define termbuf 0
#endif

static char *bp;
static char *upstr;
static char *mrstr;
static char *mestr;
static char *clstr;
static char *dcstr;

static int fd_in = STDIN_FILENO;
static FILE *f_result;

void
cleanup_exit(int i)
{
  while (l.nb_items)
    remove_item(&l, l.nb_items-1);
  free(l.items);
  free(l.nb_colsw);

  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  _exit(i);
}


void
enter_result(int item)
{
  int len;

  if (item >= l.nb_items || item < 0)
    return ;
  fprintf(l.f_out, "\n");
  fflush(l.f_out);
  fclose(l.f_out);

  fprintf(f_result, "%s", l.items[item]->s);
  fflush(f_result);
  fclose(f_result);
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
  clstr = tgetstr("cl", &bp);
  dcstr = tgetstr("dc", &bp);
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

  for (i = 0; i < l.items_per_page && clist[i] != 0 && clist[i] != c ; i++);

  if (clist[i] == c)
    return i;

  return -1;
}


#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
void
search()
{
  char *result;
  size_t i;

  special(&l);
  fprintf(l.f_out, "\r");
  fflush(l.f_out);
  result = readline("Search? ");

  /* the spaces corresponding to strlen("Search? ") */
  fprintf(l.f_out, "%s        %0*c", upstr, strlen(result), ' '); /* bar  */
  if (saved_items_len)
    {
      memcpy(l.items, saved_items, sizeof(item_t *) * saved_items_len);
      l.nb_items = saved_items_len;
    }
  if (strlen(result) == 0)
    return ;

  if (saved_items_len != l.nb_items)
    saved_items = realloc(saved_items, sizeof(item_t *) * l.nb_items);
  memcpy(saved_items, l.items, sizeof(item_t *) * l.nb_items);
  saved_items_len = l.nb_items;
  l.nb_items = 0;

  i = 0;
  while (i < saved_items_len)
    {
      if (strcasestr(saved_items[i]->s, result) != 0)
	add_item(&l, saved_items[i]);
      ++i;
    }
  free(result);
}

void
enter_selected_items()
{
  size_t item;
  int first = 1;

  fprintf(l.f_out, "\n");
  fflush(l.f_out);
  for (item = 0; item != l.nb_items; item++)
    {
      if (l.items[item]->selected)
	{
	  if (first)
	    first = 0;
	  else
	    fprintf(f_result, "%s", separator);
	  
	  fprintf(f_result, "%s", l.items[item]->s);
	}
    }
  fflush(f_result);
  fclose(f_result);
}

void
usage()
{
  fprintf(stderr,
"usage: umenu [-r :result_file | result_fd ] [-o output_fd] [-l item_per_line]\n"
"             [-s multiple_selection_separator] item1 item2 ...\n"
"EXAMPLE: echo response $(umenu -r1 -o2 -l3 yes no \"why not\")\n"
"EXAMPLE: umenu -r :/tmp/result 'cow' 'super cow'\n"
"Keyboard shortcut:\n"
"j-k : move up/down cursor\n"
"p-n : move up/down page\n"
"space : select item\n"
"enter : write selected items\n"
"1,2,3... : write item corresponding\n");
  exit(1);
}

int
get_options(int ac, char **av)
{
  extern char *optarg;
  extern int optind;
  int opt;

  while ((opt = getopt(ac, av, "r:o:l:s:")) != -1) {
    switch (opt) {
    case 'r':
      if (optarg[0] == ':')
	f_result = fopen(optarg+1, "w");
      else
	f_result = fdopen(atoi(optarg), "w");
      if (!f_result) {
	perror("Error in -r option");
	_exit(1);
      }
      break;
    case 'o':
      l.f_out = fdopen(atoi(optarg), "w");
      if (!l.f_out) {
	perror("Error in -o option");
	_exit(1);
      }      
      break;
    case 'l':
      nb_items_per_page = atoi(optarg);
      ioctl(STDOUT_FILENO, TIOCGWINSZ, &winsz);
      if (nb_items_per_page > winsz.ws_row)
	nb_items_per_page = winsz.ws_row - 1;
      break;
    case 's':
      separator = optarg;
      break;
    case '?':
      usage();
      break;
    }

  }

  return optind;
}

void
sigwinch(int sig)
{
  int i;
  (void)sig;

  fflush(l.f_out);
  fprintf(l.f_out, "\n\nThis alpha version have resize disabled\n");
  fflush(l.f_out);
  fflush(f_result);
  fclose(f_result);
  exit(1);

  ioctl(STDOUT_FILENO, TIOCGWINSZ, &winsz);
  if (winsz.ws_row <= nb_items_per_page)
    l.items_per_page = winsz.ws_row - 1;
  else
    l.items_per_page = nb_items_per_page;
  l.col_width = winsz.ws_col;
  calibre_width(&l);

  for (i = 0 ; i != l.items_per_page; i++) {
    if (l.nb_colsw[i] > l.col_width)
      while(l.nb_colsw[i]-- > l.col_width) 
	fprintf(l.f_out, "%s", dcstr);
  }
  draw_items(&l);
  signal(SIGWINCH, sigwinch);
}

int
main(int ac, char **av)
{
  int c;
  int ch;
  int i;
  int count_items;
  char **items;
  int shortcut[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', 'a', 'b',
		    'c', 'd', 'e', 'f', 'g', 'h', 'i', 'k', 'l', 'm', 'n', 'o',
		    'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', 0};
  int shortcut_len = sizeof shortcut / sizeof(int);
  int l_num;
  int mode = 0;

  if (ac == 1)
    usage();

  init_locale();
  f_result = stderr;
  l.f_out = stdout;  
  i = get_options(ac, av);
  items = av + i;
  count_items = ac - i;
  if (count_items == 0)
    return (1);

  if (nb_items_per_page <= 0 || nb_items_per_page >= shortcut_len)
    return 1;

  tcgetattr( STDIN_FILENO, &oldt );
  newt = oldt;
  newt.c_lflag &= ~( ICANON | ECHO );
  tcsetattr( STDIN_FILENO, TCSANOW, &newt);
  terminit();

  l.prefix = shortcut;
  l.upstr = upstr;
  l.dcstr = dcstr;
  l.bottom_msg[0] = '\0';
  l.bottom_msg_col_s = 0;
  l.y = 0;
  l.cursor_y = 0;
  l.item_top_page = 0;

  set_items_per_page(&l, nb_items_per_page);
  set_items(&l, items, count_items);
  signal(SIGWINCH, sigwinch);
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &winsz);
  l.col_width = winsz.ws_col;
  calibre_width(&l);
  draw_items(&l);

  fflush(l.f_out);
  while ((c = getchar()) != 'q') {
    if (c == -1) {
      if (errno == EINTR)
	continue;
      exit(1);
    }
    switch (c) {
    case '/':
      tcsetattr( STDIN_FILENO, TCSANOW, &oldt);
      search();
      l.cur_page = 0;
      l.cursor_y = 0;
      draw_items(&l);
      tcsetattr( STDIN_FILENO, TCSANOW, &newt);
      break;
      
    case 'n' /*|| ' ' || 'j' */:
      page_seek(&l, RELATIVE, 1);
      break;
    case 'p' /*|| 'k'*/:
      page_seek(&l, RELATIVE, -1);
      break;
    case 'j':
      cursor_seek(&l, 1, 1);
      break;
    case 'k':
      cursor_seek(&l, 1, -1);
      break;

    case ' ':
      select_item(&l, l.cursor_y);
      cursor_seek(&l, RELATIVE, 1);
      break;

    case '\n':
      enter_selected_items();
      cleanup_exit(0);
      break;

    default:
      if ((l_num = has_c(c, shortcut)) >= 0) {
	enter_result(l_num + l.item_top_page);
	cleanup_exit(0);
      }
      break;
    }
    fflush(l.f_out);
  }

  cleanup_exit(0);
  return 0;
}
