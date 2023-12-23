# FlatLink - a flat mode free linker

Created for generating Phar Lap EXP files using [NASM - the Netwide Assembler](https://github.com/netwide-assembler/nasm) in December 2023.

NASM can generate COM file directly, but cannot be generated from OBJ files.
Also, if you wanted to generate EXP using NASM, there was no easy-to-use free linker.

This software is used by [Free386](https://github.com/nabe-abk/free386).
Free386 is a PDS software compatible with 386|DOS-Extender "RUN386.EXE".


## Overview

* Version: 0.81
* Date: **2023-12-21**
* Author: [nabe@abk](https://twitter.com/nabe_abk)
* Link Target: 386|DOS-Extender Binary(.exp), MS-DOS Binary(.com)
* Linker Host: Linux, 386|DOS-Extender([Free386](https://github.com/nabe-abk/free386) or RUN386) on MS-DOS
* Licence: **PDS**


## Features

- Support Microsoft 16bit/32bit OMF (standard OBJ) format.
- Support Phar Lap Easy OMF-386 object format.
- Generate EXP(P3) and COM file on Linux and MS-DOS.


### Not support

- Link from LIB files.
- Generate EXE file.

OBJ files in C and other languages are not supported.
**When using a non-assembler development tool, it is wise to use the linker that comes with that development tool.**


## Command Line Options

Please refer to the displayed command line help.

```
Usage: flatlink [options] obj_file ...

    -o file         output file. '.com' or '.bin' is set 'com' format
    -f format       'exp' or 'com' or 'bin'
    -m mapfile      link map file
    -v              verbose
    -vv             more verbose
    -q              quiet (close stdout)
    -h              view this help
    -strip          strip exp file header
    -offset  num    exp file's load offset  (default 1000h)
    -stack   num    exp file's stack size   (default 1000h)
    -mindata num    exp file's minimum heap (default 1000h)
    -maxdata num    exp file's maximum heap

    -maxsegs   num  maximum segments     (default 100)
    -maxpubs   num  maximum public names (default 3000)
    -maxfixups num  maximum fixups       (default 3000)
```

This options will be similar to 386|LINK (386LINK.EXE, 386LINKP.EXP).


## Download Binary

You can probably find it among the [tools included in Free386](https://github.com/nabe-abk/free386/tree/main/tools).


## How to Build

### Linux binary on Linux

1. Install **gcc** and **make** tool.
2. Run ./make_gcc.sh

### Phar Lap binary(.exp) on Linux

1. Install [Open Watcom](https://github.com/open-watcom/open-watcom-v2) with target operating system option "DOS".
2. Run ./make_exp.sh

If you run "./make_exp.sh linux", get linux binary by Open Watcom.


### Phar Lap binary(.exp) on MS-DOS with High-C Compiler

1. Install High-C Compiler, and set PATH to "hcd386.exe" and "hc386.exe".
2. Run "make_hic.bat"


## Japanese Memo

[Free386](https://github.com/nabe-abk/free386)に「free386.com」と「サンプルプログラム」のビルドスクリプトを付属しているのですが、最初に書いたとおりNASMを使ってEXPを生成できる適当なリンカがないので困っていました。Phar Lap DOS-Extenderは老舗ですが、DOS4GやGO32に比べるとマイナーで対応リンカがそもそも少ない。NASMのOBJファイルからCOMファイルやEXPファイルを生成するだけならば、すごく単純なリンカで用が足りるのですが**それがない**。

ライセンスフリーのリンカとしては、Open Watcomのwlink（やその派生の[JWlink](https://github.com/JWasm/JWlink)）がありますが、NASM用に使用するのは規模が大きく不便です。Phar Lap純正の386|LINKはそもそも有償ですし、Linuxでは動きません。Free386はもっぱらLinux上でmakeしているので、どうせならサンプルプログラムもLinuxで生成したいんです……。

とか考えると、リンカ作りたくなりませんか？（笑）

そんなわけで「リンカ作りたい」とTwitter(X)で呟いてから約10日、リンカを作り始めてから数日でこれができました。

本当はもうひとつ理由があって、EXP生成に使用していた[Easy OMF対応のNASM](https://www.purose.net/befis/download/nasm/)が、Ver0.98時代のものなのでVer2系に更新したかったんです。NASM Ver2のEXP実行ファイルを生成することは難しくありませんが、純正は当然Easy OMF非対応なのでリンカがないなと思っていました。……そう思い込みです。テストする限り、**386|LINKは Microsoft 32bit OMF（標準OBJ形式）に対応しているので、Easy OMF（Phar Lap 32bit OMF）を出力する必要は全くありません。**

なんてこったい。

