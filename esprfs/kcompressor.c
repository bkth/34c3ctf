#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/pid_namespace.h>
#include <linux/module.h>
#include <linux/pagemap.h> 
#include <linux/fs.h>     
#include <asm/atomic.h>
#include <asm/uaccess.h>

#define DEBUG 0 

// aliasing that so i can replace it with kmalloc when porting
#define ALLOC(x) kmalloc(x, GFP_KERNEL)
#define FREE(x) kfree(x)

typedef struct bitstream {
    uint8_t bits[8];
    char* start;
    char* pos;
    uint8_t curr_bit;
} bitstream;


typedef struct node {
    uint32_t weight;
    uint8_t c;
    struct node* left;
    struct node* right;
} node;

// to create a stack allocated lookup table instead of walking the tree
// when writing the data
typedef struct huff_cache {
    uint32_t repr;
    uint16_t num_bits;
    uint16_t in_tree;
} huff_cache;

void f1(bitstream* bitstream) {
    bitstream->bits[0] = 0;
    bitstream->bits[1] = 0;
    bitstream->bits[2] = 0;
    bitstream->bits[3] = 0;
    bitstream->bits[4] = 0;
    bitstream->bits[5] = 0;
    bitstream->bits[6] = 0;
    bitstream->bits[7] = 0;
    bitstream->curr_bit = 0;
}


char* working_buffer;

void f11(bitstream* bitstream, char* buf) {
#if DEBUG
    printk(KERN_INFO "Initializing bitstream with %p\n", buf);
#endif
    f1(bitstream);
    bitstream->start = buf;	
    bitstream->pos = buf;	
}

void f2(bitstream* bitstream) {

    uint8_t byte = bitstream->bits[0];
    byte += bitstream->bits[1] << 1;
    byte += bitstream->bits[2] << 2;
    byte += bitstream->bits[3] << 3;
    byte += bitstream->bits[4] << 4;
    byte += bitstream->bits[5] << 5;
    byte += bitstream->bits[6] << 6;
    byte += bitstream->bits[7] << 7;
    *bitstream->pos++ = byte;
    f1(bitstream);

}

// assumes the buffer is byte aligned and reads a dword
// used to retrieve the number of chars encoded as a dword
// using a dword is overkill I might change that to a simple word
// to avoid insane memory usage so people can only create files with a string of length 2 ** 16 -1
uint32_t f12(bitstream* bitstream) {
    uint32_t val = *((uint32_t*) bitstream->pos);
    bitstream->pos += 4;
    return val;
}

uint8_t f22(bitstream* bitstream) {
    uint8_t ret;
    if (!bitstream->curr_bit)  {
        uint8_t byte = *bitstream->pos++;
        bitstream->bits[0] = byte & 1;
        bitstream->bits[1] = (byte >> 1) & 1;
        bitstream->bits[2] = (byte >> 2) & 1;
        bitstream->bits[3] = (byte >> 3) & 1;
        bitstream->bits[4] = (byte >> 4) & 1;
        bitstream->bits[5] = (byte >> 5) & 1;
        bitstream->bits[6] = (byte >> 6) & 1;
        bitstream->bits[7] = (byte >> 7) & 1;
    }
    /* printk(KERN_INFO "reading bit %d\n", bitstream->curr_bit); */
    ret = bitstream->bits[bitstream->curr_bit++];
    if (bitstream->curr_bit == 8) {
        bitstream->curr_bit = 0;
    }
    return ret;
}

uint8_t f21(bitstream* bitstream) {
    return f22(bitstream) +
        (f22(bitstream) << 1) +
        (f22(bitstream) << 2) +
        (f22(bitstream) << 3) +
        (f22(bitstream) << 4) +
        (f22(bitstream) << 5) +
        (f22(bitstream) << 6) +
        (f22(bitstream) << 7);
}

void f20(bitstream* bitstream, uint32_t dword) {
    *bitstream->pos++ = dword & 0xff;
    *bitstream->pos++ = (dword >> 8) & 0xff;
    *bitstream->pos++ = (dword >> 16) & 0xff;
    *bitstream->pos++ = (dword >> 24) & 0xff;
}

void f19(bitstream* bitstream, uint8_t bit) {

    if (bit > 1)
        return;

    bitstream->bits[bitstream->curr_bit++] = bit;

    if (bitstream->curr_bit == 8) {
        f2(bitstream);
    }	
}

// we might be done writing in the middle of a byte so 0-pad
void f18(bitstream* bitstream) {

    while (bitstream->curr_bit < 8) {
        bitstream->bits[bitstream->curr_bit++] = 0;
    }
    f2(bitstream);
}

void f7(bitstream* bitstream) {
    if (!bitstream->curr_bit)
        return;
    while (bitstream->curr_bit < 8) {
        bitstream->bits[bitstream->curr_bit] = (((uint8_t) (*bitstream->pos)) & (1 << bitstream->curr_bit)) >> (bitstream->curr_bit);
        bitstream->curr_bit++;
    }
    f2(bitstream);
}

// write tree to compressed data
// 0 indicates a node
// 1 indicates a leave followed by 8 bit encoding the character represented by that node
// this is a simplification instead of shortening the encoding of the character but whatever
void f3(bitstream* bitstream, node* root) {
    if (root->left) {
#if DEBUG
        printk(KERN_INFO "Writing bit 0\n");
#endif
        f19(bitstream, 0);
        f3(bitstream, root->left);
        f3(bitstream, root->right);
        return; 
    }
#if DEBUG
    printk(KERN_INFO "Writing bit 1\n");
    printk(KERN_INFO "Writing char %c\n", root->c);
#endif
    f19(bitstream, 1);
    f19(bitstream, root->c & 1);
    f19(bitstream, (root->c >> 1) & 1);
    f19(bitstream, (root->c >> 2) & 1);
    f19(bitstream, (root->c >> 3) & 1);
    f19(bitstream, (root->c >> 4) & 1);
    f19(bitstream, (root->c >> 5) & 1);
    f19(bitstream, (root->c >> 6) & 1);
    f19(bitstream, (root->c >> 7) & 1);
}

void f8(bitstream* bitstream, uint32_t increment) {
    bitstream->curr_bit += increment;
    if (bitstream->curr_bit > 7) {
        bitstream->pos++;
        bitstream->curr_bit = bitstream->curr_bit % 8;
    }

}

#if DEBUG
void print_table(char* table) {
    int i;
    for (i = 0; i < 256; ++i) {
        if (table[i]) 
            printk(KERN_INFO "%x: %d\n", i, table[i]);
    }
}

void print_tree(node* root, uint32_t depth) {
    int i;
    if (root->left) {
        for (i = 0; i < depth; ++i)
            printk(KERN_INFO "\t");
        printk(KERN_INFO "LEFT: \n");
        print_tree(root->left, depth + 1);
    } 
    if (root->right) {
        for (i = 0; i < depth; ++i)
            printk(KERN_INFO "\t");
        printk(KERN_INFO "RIGHT: \n");
        print_tree(root->right, depth + 1);
    } else {
        for (i = 0; i < depth; ++i)
            printk(KERN_INFO "\t");
        printk(KERN_INFO "CHAR %c: %d\n", root->c, root->weight);
    }
}
#endif

int f15(node* root, uint32_t height) {
    uint32_t max_left;
    uint32_t max_right;

    if (!root->left && !root->right)
        return height;

    max_left = f15(root->left, height + 1);
    max_right = f15(root->right, height + 1);

    if (max_left > max_right)
        return max_left;

    return max_right;
}

int f17(node* root) {

    if (!root->left && !root->right)
        return 1;

    return 1 + f17(root->left) + f17(root->right);

}

int f10(node* root) {

    uint8_t ok;
    uint64_t height;
    uint64_t num_elem;
    uint64_t tmp;
    int i;

    ok = 0;
    if (!root->left && !root->right) {
        ok = 1;
    } else {
        height = f15(root, 0);
        num_elem = f17(root);
        tmp = 1;
        for (i = 0; i <= height; ++i) {
            tmp *= 2;
        }
        if (tmp - 1 == num_elem) {
            ok = 1;
        }
    }

    return ok;
}

uint8_t f13(node* root) {
    uint8_t left;
    uint8_t right;
    if (!root->left)
        return root->c;
    left = f13(root->left);
    right = f13(root->right);
    if (left > right)
        return left;
    return right;
}

uint8_t f9(node* root, char c) {
    uint8_t left;
    uint8_t right;
    if (!root->left)
        return root->c == c;
    left = f9(root->left, c);
    right = f9(root->right, c);
    return right + left;
}


// create a cache to get for each character its representation and the number of bits needed
// to encode it corresponding to its depth in the tree
void f14(huff_cache* table, node* root, uint8_t depth, uint32_t path) {
    if (!depth && !root->left && !root->right) {
        table[root->c].num_bits = 1;
        table[root->c].repr = path;
        table[root->c].in_tree = 1;
        return;
    }
    if (root->left) {
        f14(table, root->left, depth + 1, path /* + (0 << depth) */);
        f14(table, root->right, depth + 1, path + (1 << depth));
        return;
    }
    table[root->c].num_bits = depth;
    table[root->c].repr = path;
    table[root->c].in_tree = 1;

}

uint32_t f16(node* root, uint8_t depth) {
    uint32_t tmp;
    if (!depth && !root->left && !root->right) {
        return root->weight;
    }
    if (root->left) {
        tmp = f16(root->left, depth + 1);
        tmp += f16(root->right, depth + 1);
        return tmp;
    }
    return root->weight * depth;

}

// called after the tree has been written: writes the compressed data
void f6(bitstream* bitstream, char* buf, uint32_t size, node* root) {
    int i;
    node* it;
    unsigned char c;
    uint32_t repr;
    huff_cache* cache_table;
    cache_table = ALLOC(256 * sizeof(huff_cache));
    for (i = 0; i < 256; ++i) {
        cache_table[i].in_tree = 0; 
    }
    /* print_tree(root, 0); */
    f14(&cache_table[0], root, 0, 0);

#if DEBUG
    for (i = 0; i < 256; ++i) {
        if (cache_table[i].in_tree)
            printk(KERN_INFO "Char %c: repr %x, num_bits %d\n", i, cache_table[i].repr, cache_table[i].num_bits); 
    }
    printk(KERN_INFO "\nWriting Data\n");
#endif
    while (size) {
        it = root;
        c = *buf++;
        // TODO REDO CHECK
        if (!cache_table[c].in_tree) {
            // DOUBLE FETCH
            size--;    
            continue;
        }
        repr = cache_table[c].repr;
        /* #if DEBUG */
        /*         printk(KERN_INFO "Writing char %c\n", c); */
        /* #endif */
        for (i = 0; i < cache_table[c].num_bits; ++i) {
            /* #if DEBUG */
            /*             printk(KERN_INFO "\twriting bit %d\n", repr & 1); */
            /* #endif */
            f19(bitstream, repr & 1);
            repr >>= 1;
        }
        size--;
    }
    FREE(cache_table);
}

char* f4(node* root, char* buf, uint32_t size, uint8_t* compressed, uint64_t* real_length) {

    uint32_t len;
    char* ret;
    bitstream w;
    uint8_t max;
    char* working_copy;

    //printk(KERN_INFO "Write Called with buf %p and size %u\n", buf, size);

    len = f16(root, 0);
    working_copy = ALLOC(size + 1);
    if ((uint64_t) buf < 0x8000000000000000) {
        copy_from_user(working_copy, buf, size);
    }
    else {
        memcpy(working_copy, buf, size);
    }
    //printk(KERN_INFO "WRITE: buf is %s\n", working_copy);
    buf = working_copy;
    working_copy[size] = '\0';

    max = f13(root);
#if DEBUG
    printk(KERN_INFO "Max is %x\n", max);
#endif
    if (!working_buffer)
        working_buffer = (char*) ALLOC(0x10000);
    memset(working_buffer, 0, 0x10000);
    f11(&w, working_buffer);
    f20(&w, size); // write number of characters
    f3(&w, root); // write out the tree
    len = len + (w.pos - w.start) * 8 + w.curr_bit;
    len = (len / 8) + 1;
    if (len > size + 4) {
#if DEBUG
        printk(KERN_INFO "STORING UNCOMPRESSED\n");
#endif
        ret = ALLOC(size + 4);
        *(uint32_t*) ret = size;
        *real_length = size;
        memcpy(ret + 4, working_copy, size);
        *compressed = 0;
        FREE(buf);
        return ret;
    }
    f6(&w, buf, size, root); // write out the data
    f18(&w); // 0-pad
#if DEBUG
    printk(KERN_INFO "Computed length %u, Compressed data is %lu bytes long\n", len, w.pos - w.start);
    for (i = 0; i < 8; ++i) {
        printk(KERN_INFO "%p: 0x%8x\n", ((uint32_t*)w.start) + i, *(((uint32_t*)w.start) + i));
    }
#endif
    ret = ALLOC(len);
    memcpy(ret, w.start, w.pos - w.start);
    FREE(buf);
    *compressed = 1;
    *real_length = len;
    return ret;
}

static node* arr;
static node** stageing_list;

node* f5(char* buf, uint32_t size) {

    uint32_t count;
    int i;
    uint32_t j;
    uint32_t min1;
    uint32_t min2;
    uint32_t remaining;
    node* nd;
    node** nd1;
    node** nd2;
    uint16_t table[256];
    char* working_copy;

    //printk(KERN_INFO "Create tree Called with buf %p, size %d\n", buf, size);
    working_copy = ALLOC(size + 1);

    if ((uint64_t) buf < 0x8000000000000000) {
        copy_from_user(working_copy, buf, size);
    }
    else {
        memcpy(working_copy, buf, size);
    }
    //printk(KERN_INFO "CREATE_TREE: buf is %s\n", working_copy);
    working_copy[size] = '\0';
    buf = working_copy;
    for (i = 0; i < 256; ++i) {
        table[i] = 0;
    }
    for (i = 0; i < size; ++i) {
        ++table[(uint8_t) buf[i]];
    }

    nd1 = NULL;
    nd2 = NULL;
    count = 0;
    for (i = 0; i < 256; ++i) {
        if (table[i])
            count++;
    }
    if (count * 2 - 1 > count)
        count = count * 2 + 1;	
    arr = (node*) ALLOC(sizeof(node) * count); 
    stageing_list = (node**) ALLOC(sizeof(node*) * ((count + 1) / 2)); 
    memset(arr, 0, sizeof(node) * count);
    memset(stageing_list, 0, sizeof(node*) * ((count + 1) / 2));

    // create initial set of nodes and stage them
    j = 0;
    for (i = 0; i < 256; ++i) {
        if (table[i]) {
            node* nd = &arr[j];
            nd->left = NULL;			
            nd->c = i;
            nd->right = NULL;
            nd->weight = table[i];			
            stageing_list[j] = nd;
            ++j;
        }
    }

    min1 = 0xffffffff;
    min2 = 0xffffffff;
    remaining = 0;
    for (;;) {
        min1 = 0xffffffff;
        min2 = 0xffffffff;
        remaining = 0;
        for (i = 0; i < ((count + 1) / 2); ++i) {
            if (stageing_list[i]) {
                remaining++;
            }
        }
        if (remaining == 1)
            break;
        for (i = 0; i < ((count + 1) / 2); ++i) {
            if (!stageing_list[i])
                continue;
            if (stageing_list[i]->weight < min1) {
                min2 = min1;
                min1 = stageing_list[i]->weight;
                nd2 = nd1;
                nd1 = &stageing_list[i];		
            } else if (stageing_list[i]->weight < min2) {
                min2 = stageing_list[i]->weight;
                nd2 = &stageing_list[i];		
            }
        }
        nd = &arr[j];
        nd->right = *nd1;
        nd->left = *nd2;
        nd->weight = (*nd1)->weight + (*nd2)->weight;
        j++;
        *nd1 = nd;
        *nd2 = 0;	
    }
    //printk(KERN_INFO "HERE\n");
    for (i = 0; i < ((count + 1) / 2); ++i) {
        if (stageing_list[i]) {
            FREE(buf);
            return stageing_list[i]; // memory never freed, need to fix that at some point maybe 
        }
    }
    FREE(buf);
    return NULL;
}


char* deflate(char* buf, uint32_t size, uint8_t* compressed, uint64_t* real_length) {
    node* huff_tree;
    char* ret;
    if (!size)
        return NULL;
    /* print_table(table); */
    huff_tree = f5(buf, size);
    /* print_tree(huff_tree, 0); */
    ret = f4(huff_tree, buf, size, compressed, real_length);
    FREE(arr);
    FREE(stageing_list);
    return ret;
}

char* inflate(char* compressed_buf) {

    int8_t leaves_remaining;
    uint16_t idx;
    uint32_t num_char;
    char* dst;
    bitstream rd;
    uint8_t bit;
    uint8_t c;
    node* parents[30];
    int8_t idx_parent;
    node* table;
    node* root;
    char* pos; 


    table = ALLOC(512 * sizeof(node));
    idx_parent = 0;
    f11(&rd, compressed_buf);
    num_char = f12(&rd);
    dst = ALLOC(num_char + 1);
    dst[num_char] = 0;
    leaves_remaining = 1;
    idx = 0;
    do {
        /* printk(KERN_INFO "\n-------------------------\n"); */
        bit = f22(&rd);
        /* printk(KERN_INFO "Got BIT %d\n", bit); */
        if (!bit) {
            parents[idx_parent] = &table[idx];
            leaves_remaining++;
        } else {
            c = f21(&rd);
            /* printk(KERN_INFO "Got char %c\n", c); */
            table[idx].c = c;
            leaves_remaining--;
        }
        table[idx].left = NULL;
        table[idx].right = NULL;

        if (idx_parent - 1 >= 0) {
            if (!parents[idx_parent - 1]->left) {
                /* printk(KERN_INFO "setting left node\n"); */
                parents[idx_parent - 1]->left = &table[idx];
            } else if (!parents[idx_parent - 1]->right) {
                /* printk(KERN_INFO "setting right node\n"); */
                parents[idx_parent - 1]->right= &table[idx];
                if (bit)
                    while (idx_parent-1 >=0 && parents[idx_parent-1]->right)
                        idx_parent--;
            } else {
                FREE(table);
                return NULL;
            }
        }
        if (!bit)
            idx_parent++;
        idx++;
        /* print_tree(&table[0], 0); */
    } while (leaves_remaining > 0);
#if DEBUG
    print_tree(&table[0], 0);
#endif

    pos = dst; 
    if (!table[0].left) {
        // single char fast path
#if DEBUG
        printk(KERN_INFO "fast path\n");
#endif
        c = table[0].c;
        while (num_char--) {
            *pos++ = c;
        }
        FREE(table);
        return dst;
    }
    while (num_char--) {
        root = &table[0];
        for (;;) {
            bit = f22(&rd);
#if DEBUG
            printk(KERN_INFO "got bit %d\n", bit);
#endif
            if (bit) 
                root = root->right;
            else 
                root = root->left;
            if (!root->left) {
#if DEBUG
                printk(KERN_INFO "setting char %c\n", root->c);
#endif
                *pos++ = root->c;
                break;
            }
        }
    }
    FREE(table);
    return dst;
}

int try_inplace_edit(char* compressed_buf, char* new_buf, size_t offset, size_t len) {

    int8_t leaves_remaining;
    uint16_t idx;
    uint32_t num_char;
    bitstream rd;
    uint8_t bit;
    uint8_t c;
    node* parents[30];
    int8_t idx_parent;
    node* table;
    node* root;
    uint64_t increment;
    char in_tree_cache[256];
    int i;


    for (i = 0; i < 256; ++i) {
        in_tree_cache[i] = '\0';
    }

    table = ALLOC(512 * sizeof(node));
    idx_parent = 0;
    f11(&rd, compressed_buf);
    num_char = f12(&rd);
    leaves_remaining = 1;
    idx = 0;

    // Don't reuse tree if change is more than 10%
    if (len * 10 > num_char) {
        return 0;
    }
    do {
        bit = f22(&rd);
        if (!bit) {
            parents[idx_parent] = &table[idx];
            leaves_remaining++;
        } else {
            c = f21(&rd);
            table[idx].c = c;
            leaves_remaining--;
        }
        table[idx].left = NULL;
        table[idx].right = NULL;

        if (idx_parent - 1 >= 0) {
            if (!parents[idx_parent - 1]->left) {
                parents[idx_parent - 1]->left = &table[idx];
            } else if (!parents[idx_parent - 1]->right) {
                parents[idx_parent - 1]->right= &table[idx];
                if (bit)
                    while (idx_parent-1 >=0 && parents[idx_parent-1]->right)
                        idx_parent--;
            } else {
                FREE(table);
                return 0;
            }
        }
        if (!bit)
            idx_parent++;
        idx++;
    } while (leaves_remaining > 0);

    if (!f10(&table[0])) {
        return 0;
    }

    for (i = 0; i < len; ++i) {
        if (in_tree_cache[(uint8_t) new_buf[i]]) {
            // cache hit
            continue;
        }
        if (!f9(&table[0], new_buf[i])) {
            FREE(table);
            return 0;
        }
        in_tree_cache[(uint8_t) new_buf[i]] = 1;
    }

    increment = 0;

    root = &table[0];
    if (!root->left) {
        increment = 1;
    } else {
        while (root->left) {
            root = root->left;
            increment++;
        }
    }

    while (offset--) {
        f8(&rd, increment);
    }    

    rd.pos--;
    f6(&rd, new_buf, len, &table[0]);
    f7(&rd); 
    FREE(table);
    return 1;

} 
