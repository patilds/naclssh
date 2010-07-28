#TODO: nodownload param for rebuild script

function die() {
  echo $1
  exit 1
}

if [ -z $NACL_SDK_ROOT ] ; then die "NACL_SDK_ROOT variable should be set" ; fi
#export NACL_SDK_ROOT=/home/gloom/Projects/nativeclient/native_client/toolchain/linux_x86/sdk/nacl-sdk

NACL_CC_PREFIX=$NACL_SDK_ROOT/bin/nacl-

DIR=$(pwd)

# src/
# src/naclssh
# src/naclstubs
# src/patches/*.diff
# downloads into src/third_party/


make -C naclstubs || die "naclstubs make failed"

mkdir third_party
cd third_party

wget http://www.openssl.org/source/openssl-1.0.0a.tar.gz || die "cannot download openssl"
tar -xzf openssl-1.0.0a.tar.gz || die "tar openssl failed"

patch -p1 < ../patches/libcrypto.diff || die "patch crypto failed"

cd openssl-1.0.0a
MACHINE=i686 ./config --cross-compile-prefix=$NACL_CC_PREFIX no-dso no-shared no-hw no-sse2 no-krb5 no-asm -I$DIR/naclstubs/include || die "config openssl failed"

make build_crypto || die "make build_crypto failed"


# TMP HACK
# TODO modify libssh2 config - no check for libssl is needed
mkdir lib
cp libcrypto.a lib/libssl.a
cp libcrypto.a lib/libcrypto.a

cd .. # in third_party


wget http://www.libssh2.org/download/libssh2-1.2.6.tar.gz || die "cannot download libssh2"
tar -xzf libssh2-1.2.6.tar.gz || die "tar libssh failed"

patch -p1 < ../patches/libssh.diff || die "patch libssh failed"
cd libssh2-1.2.6

export CC=${NACL_CC_PREFIX}gcc
export AR=${NACL_CC_PREFIX}ar
export RANLIB=${NACL_CC_PREFIX}ranlib

./configure --with-libssl-prefix=${DIR}/third_party/openssl-1.0.0a --host=nacl --enable-shared=no || die "configure libssh failed"

cp ../../patches/libssh2_nacl_tmpl.h src/libssh2_nacl.h

make -C src

cd .. # in third_party

wget http://zlib.net/zlib-1.2.5.tar.gz || die "cannot download zlib"
tar -xzf zlib-1.2.5.tar.gz || die "tar zlib failed"
cd zlib-1.2.5

export CFLAGS="-Dunlink=puts"
./configure --static || die "configure zlib failed"
make || die "make zlib failed"

cd ../../naclssh

make || die "make ssh_plugin failed"

