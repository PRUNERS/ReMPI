#!/bin/sh
#
# Run an extra libtoolize before autoreconf to ensure that
# libtool macros can be found if libtool is in PATH, but its
# macros are not in default aclocal search path.
#
echo "Running libtoolize --automake --copy ... "
libtoolize --automake --copy 
echo "Running autoreconf --verbose --install"
autoreconf --verbose --install
echo "Cleaning up ..."
rm -rf autom4te.cache 
echo "Now run ./configure."
