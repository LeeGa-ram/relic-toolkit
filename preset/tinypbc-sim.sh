#!/bin/bash 
cmake -DWORD=8 -DOPSYS=NONE -DSEED=LIBC -DSHLIB=OFF -DSTBIN=ON -DTIMER=NONE -DWITH="DV;BN;FB;EB;PB;CP;MD" -DBENCH=20 -DTESTS=20 -DCHECK=off -DVERBS=off -DSTRIP=on -DQUIET=on -DARITH=easy -DFB_METHD="INTEG;INTEG;QUICK;QUICK;QUICK;QUICK;EXGCD" $1
