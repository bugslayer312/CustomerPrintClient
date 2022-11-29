cd wxWidgets-3.1.3
mkdir tmp_build
cd tmp_build
../configure --disable-shared --with-gtk=3 --disable-compat30 --enable-stl --enable-threads --enable-webview --with-cxx=14 --enable-debug --prefix=DESTDIR --lib-dir=DESTDIR/libd
make -j4
make install

rm -rf ./*
../configure --disable-shared --with-gtk=3 --disable-compat30 --enable-stl --enable-threads --enable-webview --with-cxx=14 --prefix=DESTDIR
make -j4
make install
