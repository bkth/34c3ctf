LFA Challenge,

Ruby native extension running inside a sandbox ruby interpreter

the sandbox prevents all syscalls not needed to ruby to execute properly after the runtime has been initialized and only allows the readv syscall. As the server actually opens the flag file and dups it to fd 1023, it should have been clear that the goal is to execute a readv syscall on fd 1023

the native extensions provides a segment based array implementation with a special case where the base segment is inlined, it also uses a length property like JS engines to keep track of the latest index set, this allows for a fast bailout when accessing indices above or equal to that value.

the bug is that when we delete an element which renders a segment empty, it will try to re-inline the remaining segment if it is the segment starting at index 0 but it does not update the length which gives us a heap OOB R/W

The goal of this challenge was to give a powerful primitive but which was not enough to just directly get an arbitrary R/W in order to give people freedom in how to proceed to get there


My exploit then works by spraying a lot of strings, due to the ways string are represented internally we can lineraly scan the heap to find objects that look like our strings.

in order to be sure we corrupted a string, I overwrite the pointer buffer LSB with a null byte, this guarantees that it will still point to a valid address but when checking the first characters, we can check if we get the original string or garbage,

so now we have an arbitrary R/W

We need a libc address, the way to do it is to once again scan the heap for what looks like library pointers. it turns out that the way ptmalloc2 works, with close to 100% chance (I never had this fail actually) there will be a pointer with an offset corresponding to `main_arena+88` on the heap.

Then we can leak the stack by reading `program_invocation_name`

Then we can stack the scan backwards to find the `__libc_start_main` address on the stack which is the return address of `main` and write our rop chain there.

the exploit might fail from time to time due to ASCII - UTF8 conflict but is 90% stable.

