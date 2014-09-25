umenu
=====

menu list in curses like dialog but more faster to choose

* USAGE: `umenu [-rxx -oxx] item1 item2 ...`
 -r : select file descriptor to write selected item
 -o : select file descriptor to write menu output

* EXAMPLE: `echo Your response is $(umenu -r1 -o2 yes no "i don't know")`

* Keys :
 q : quit
 j,n : down
 k,p : up
 / : search
