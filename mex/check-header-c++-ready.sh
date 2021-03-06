#!/bin/sh

# Checks if a header is protected by #ifdef/#define/#endif guards that respect
# the __HEADER_H__ convention

LC_ALL=C
export LC_ALL

test -z "$srcdir" && srcdir=.
stat=0

test "x$HEADERS" = x && HEADERS=`find $srcdir -maxdepth 1 -name '*.h'`
test "x$SOURCES" = x && SOURCES=`find $srcdir -maxdepth 1 -name '*.c'`

SKIP="mex.h mex-player-common.h"

for x in $HEADERS; do
  xx=`echo "$x" | sed 's@.*/@@'`
  echo $SKIP | grep -q $xx && continue
  #echo "file: $x"
  if ! grep -q G_BEGIN_DECLS "$x" || ! grep -q G_END_DECLS "$x"; then
    echo "Ooops, header file $x does not have G_{BEGIN,END}_DECLS"
    stat=1
  fi
done

exit $stat
