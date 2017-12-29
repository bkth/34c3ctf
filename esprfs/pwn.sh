#!/bin/bash
#gcc -o WVUTSRQPONMLKJI pwn.c && ./WVUTSRQPONMLKJI
#gcc -o /tmp/pwn pwn.c
(echo 'base64 -d > WVUTSRQPONMLKJI <<EOS'
base64 -w0 exploit/WVUTSRQPONMLKJI
echo
echo 'EOS'
echo 'chmod +x WVUTSRQPONMLKJI'
echo './WVUTSRQPONMLKJI')
