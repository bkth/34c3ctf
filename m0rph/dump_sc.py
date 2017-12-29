import subprocess 
import sys

size_func = 17

def run_cmd(s):
        return subprocess.check_output(s.split())

out = run_cmd("objdump -d ./keycheck").split("\n")


with open("./keycheck", "rb") as f:

    
    for i, v in enumerate(out):
        if "<main>:" in v:
            j = i
            while "libc_csu_init" not in out[j]:
                j += 1
            start = int(out[i].split()[0], 16)
            end = int(out[j - 6].split()[0], 16) + 1

            print hex(start)
            print hex(end)


            stream = f.read()[start:end]
            print len(stream)
            output = bytearray(stream)
           

            j = -1
            size = -1
            for i, v in enumerate(output):
                if output[i] == 0x56 and output[i+1] == 0x52:
                    j += 1
                    print "treating byte %x" % (start + i)
                    print "size is %d" % size
                    size = 0
                elif output[i] == 0x90 and output[i+1] == 0x90:
                    break

                size += 1
                output[i] = output[i] ^ ((j * 273) % 256)


            print r'\x' + r'\x'.join(chr(c).encode('hex') for c in output)

            sys.exit(0)
