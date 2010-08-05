#TODO: nodownload param for rebuild script

function die() {
  echo $1
  exit 1
}

if [ -z $NACL_SDK_ROOT ] ; then die "NACL_SDK_ROOT variable should be set" ; fi
#export NACL_SDK_ROOT=/home/gloom/Projects/nativeclient/native_client/toolchain/linux_x86/sdk/nacl-sdk

if [ -z $1 ]
then
	die "-m32 or -m64 should be specified"
else
	if [[ $1 == -m32 || $1 == -m64 ]] ; then MACHINE_FLAG=$1 ; else die "wrong flag: should be -m32 or -m64" ; fi
fi

NACL_CC_PREFIX=$NACL_SDK_ROOT/bin/nacl-

DIR=$(pwd)

# src/
# src/naclssh
# src/naclstubs
# src/patches/*.diff
# downloads into src/third_party/

export CFLAGS=${MACHINE_FLAG}

make -C naclstubs || die "naclstubs make failed"

mkdir third_party
cd third_party

wget http://www.openssl.org/source/openssl-1.0.0a.tar.gz || die "cannot download openssl"
tar -xzf openssl-1.0.0a.tar.gz || die "tar openssl failed"

patch -p1 < ../patches/libcrypto.diff || die "patch crypto failed"

cd openssl-1.0.0a
MACHINE=i686 ./config $MACHINE_FLAG --cross-compile-prefix=$NACL_CC_PREFIX no-dso no-shared no-hw no-sse2 no-krb5 no-asm -I$DIR/naclstubs/include || die "config openssl failed"

make build_crypto || die "make build_crypto failed"


# TMP HACK
# TODO modify libssh2 config - no check for libssl is needed
if [[ $MACHINE_FLAG == -m32 ]]
then
	mkdir lib
	cp libcrypto.a lib/libssl.a
	cp libcrypto.a lib/libcrypto.a
else
	mkdir lib64
	cp libcrypto.a lib64/libssl.a
	cp libcrypto.a lib64/libcrypto.a
fi


cd .. # in third_party


wget http://www.libssh2.org/download/libssh2-1.2.6.tar.gz || die "cannot download libssh2"
tar -xzf libssh2-1.2.6.tar.gz || die "tar libssh failed"

patch -p1 < ../patches/libssh.diff || die "patch libssh failed"
cd libssh2-1.2.6

# CFLAGS passing to libssh2 problem: no other way to pass -m64 to configure (??)
if [[ $MACHINE_FLAG == -m64 ]]
then
	NACL_CC_PREFIX=$NACL_SDK_ROOT/bin/nacl64-
fi

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

export CFLAGS="-Dunlink=puts "${MACHINE_FLAG}
./configure --static || die "configure zlib failed"
make || die "make zlib failed"

cd ../../naclssh

make clean

if [[ $MACHINE_FLAG == -m32 ]]
then
	make ssh_plugin_x86_32.nexe || die "make ssh_plugin failed"
else
	make ssh_plugin_x86_64.nexe || die "make ssh_plugin failed"
fi

