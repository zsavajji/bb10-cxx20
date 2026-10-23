#!/usr/bin/env python3
"""Downloads the LLVM 19.1.7 release sources the runtimes are built from,
checks their sha256, and unpacks them into DEST as an llvm-project tree (of
llvm itself only its CMake modules). Python only, since the toolchain image
has neither curl nor xz.

  scripts/fetch-llvm.py DEST [TARBALLS]

TARBALLS (default: the folder above DEST) keeps the downloads; good ones
already there are not downloaded again.
"""
import hashlib
import os
import shutil
import sys
import tarfile
import urllib.request

VERSION = "19.1.7"
URL = "https://github.com/llvm/llvm-project/releases/download/llvmorg-%s/%s-%s.src.tar.xz"
SHA256 = {
    "cmake": "11c5a28f90053b0c43d0dec3d0ad579347fc277199c005206b963c19aae514e3",
    "llvm": "96f833c6ad99a3e8e1d9aca5f439b8fd2c7efdcf83b664e0af1c0712c5315910",
    "runtimes": "404c54c2df45a307331eba1372917ec480addf3b77ca9943ed4d7ea445efb08f",
    "libcxx": "b736109650ffc750dbdc506483347b3713ded9d0300f48432b820ad66b6a9052",
    "libcxxabi": "4d25125109e3febe1393e9e30f93153333d82b48ce7bed637bd2daf52bf40172",
    "libunwind": "10e650f436bc416674f01b5e01177405886f4d0f0b2568c6808044eacad52ea0",
    "compiler-rt": "c12b6e764202c615c1a3af9a13d477846878757ae0e29e5f8979215a6958fffc",
}


def sha256(path):
    digest = hashlib.sha256()
    with open(path, "rb") as f:
        for block in iter(lambda: f.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest()


def main():
    dest = os.path.abspath(sys.argv[1])
    cache = os.path.abspath(sys.argv[2]) if len(sys.argv) > 2 else os.path.dirname(dest)
    os.makedirs(dest, exist_ok=True)
    os.makedirs(cache, exist_ok=True)
    for name, expected in SHA256.items():
        tarball = os.path.join(cache, name + ".tar.xz")
        if not os.path.exists(tarball) or sha256(tarball) != expected:
            url = URL % (VERSION, name, VERSION)
            print("downloading", url, flush=True)
            with urllib.request.urlopen(url) as response, open(tarball, "wb") as out:
                shutil.copyfileobj(response, out)
            if sha256(tarball) != expected:
                sys.exit("%s: sha256 mismatch" % tarball)
        target = os.path.join(dest, name)
        if os.path.exists(target):
            sys.exit("%s exists; remove it to unpack it again" % target)
        top = "%s-%s.src" % (name, VERSION)
        keep = top + "/cmake" if name == "llvm" else top
        with tarfile.open(tarball) as tar:
            members = [m for m in tar.getmembers() if m.name == keep or m.name.startswith(keep + "/")]
            tar.extractall(dest, members=members, filter="tar") if hasattr(tarfile, "data_filter") \
                else tar.extractall(dest, members=members)
        os.rename(os.path.join(dest, top), target)
    print("sources:", dest)


main()
