# Maintainer: John Z. <johnz -at_ pleasantnightmare _dot- com>
pkgname=reentrant-progress-notificator
pkgver=1.0
pkgrel=1
arch=('any')
pkgdesc="An X11 dialog that displays an SVG image and a progressbar, that can be updated by sending USR1 and USR2 signals"
depends=('libx11' 'cairo' 'glib2' 'librsvg')
makedepends=('make' 'gcc' 'pkgconf')
provides=(reentrant-progress-notificator)
install=".install"
source=("git+file:///home/john/workspace/reentrant-progress-notificator")
#source=("git+https://github.com/machinedgod/reentrant-progress-notificator.git")
md5sums=('SKIP')

build() {
	cd "$pkgname"
	make
}

package() {
	cd "$pkgname"

	# Bins
	install -Dm755 "$pkgname" "$pkgdir/usr/bin/$pkgname"
	install -Dm755 "volume-notification" "$pkgdir/usr/bin/volume-notification"

	# Man
	gzip --force "$pkgname.man"
	install -Dm644 "$pkgname.man.gz" "$pkgdir/usr/share/man/man1/$pkgname.1.gz"
}
