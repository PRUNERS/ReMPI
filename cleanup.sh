make -k maintainer-clean
rm ./config.h.in
rm ./aclocal.m4
rm ./config.status
rm ./config.log
rm -r autom4te.cache
cd m4
rm libtool.m4  ltoptions.m4  ltsugar.m4  ltversion.m4  lt~obsolete.m4
cd -

