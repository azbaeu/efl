#!/bin/sh

brew update
brew unlink python
brew install gettext check bullet dbus fontconfig freetype fribidi gst-plugins-good gstreamer luajit openssl webp libsndfile glib libspectre libraw librsvg poppler lz4 pulseaudio ccache
export CFLAGS="-I/usr/local/opt/openssl/include $CFLAGS" LDFLAGS="-L/usr/local/opt/openssl/lib $LDFLAGS" PKG_CONFIG_PATH="/usr/local/opt/openssl/lib/pkgconfig"
