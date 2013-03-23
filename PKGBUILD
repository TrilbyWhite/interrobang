#Maintainer: Jesse McClure AKA "Trilby" <jmcclure [at] cns [dot] umass [dot] edu>
pkgname=swifer-git
pkgver=20130323
pkgrel=1
pkgdesc="A tiny launcher menu packing a big bang (syntax)"
url="http://github.com/TrilbyWhite/interobang.git"
arch=('any')
license=('GPLv3')
depends=('bash' 'bash-completion')
makedepends=('git')
_gitroot="git://github.com/TrilbyWhite/interobang.git"
_gitname="interobang"

build() {
    cd "$srcdir"
    msg "Connecting to GIT server...."
    if [ -d $_gitname ] ; then
        cd $_gitname && git pull origin
        msg "The local files are updated."
    else
        git clone $_gitroot $_gitname
    fi
    msg "GIT checkout done or server timeout"
    msg "Starting make..."
    rm -rf "$srcdir/$_gitname-build"
    git clone "$srcdir/$_gitname" "$srcdir/$_gitname-build"
    cd "$srcdir/$_gitname-build"
	make
}

package() {
	cd "$srcdir/$_gitname-build"
	make PREFIX=/usr DESTDIR=$pkgdir install
}

