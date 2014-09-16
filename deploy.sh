#!/bin/sh

#LINK=`which dso_tool`
#SCRIPT=$(readlink -f $LINK)
#TENGINE_DIR="${SCRIPT%/sbin*}"

TENGINE_DIR=/opt/tengine
SO_NAME=ngx_http_fbs_module
SO_FILE=objs/$SO_NAME.so

# if built with tup
[ -e src/$SO_NAME.so ] && SO_FILE=src/$SO_NAME.so

# unlink
[ -e $TENGINE_DIR/modules/$SO_NAME.so ] && unlink $TENGINE_DIR/modules/$SO_NAME.so

# copy
cp $SO_FILE $TENGINE_DIR/modules/

[ "$1" = "0" ] && exit 0

$TENGINE_DIR/sbin/nginx -s stop >> /dev/null 2>&1
$TENGINE_DIR/sbin/nginx

