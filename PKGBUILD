pkgname=fyi
pkgver=1.0.2
pkgrel=1
pkgdesc="Command line utility to create desktop notifications"
arch=('x86_64' 'aarch64')
url=https://codeberg.org/dnkl/fyi
license=(mit)
makedepends=('meson' 'ninja')
depends=('dbus')
source=()
#changelog=CHANGELOG.md

pkgver() {
  cd ../.git &> /dev/null && git describe --tags --long | sed 's/^v//;s/\([^-]*-g\)/r\1/;s/-/./g' ||
      head -3 ../meson.build | grep version | cut -d "'" -f 2
}

build() {
  export CFLAGS="${CFLAGS} -fno-exceptions"
  meson --prefix=/usr --buildtype=minsize --wrap-mode=nofallback ..
  ninja
}

package() {
  DESTDIR="${pkgdir}/" ninja install
}
