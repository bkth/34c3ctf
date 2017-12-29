Simple pwn challenge,

the program implents a super simplistic refcount based garbage collector which frees a string object when its reference count is at 0.

However the refcount is a `uint8_t` and it never checks for overflow when increasing it, by making 256 references we can make the count wrap around to 0 and the GC will free the string.

Now, some people had working exploit locally but not remotely. This is most likely because they did not use the provided libc. the libc of the challenge is 2.26 which has a new feature called t-cache which is a simple thread-local look aside list for small size allocation.

As the allocations happen in a different thread than the one where memory is freed, we need to "bypass" tcache in order to get a UAF. tcache has a concept of bucket for different sizes where the maximum capacity is 8 so we simply make sure we fill the tcache entry for the size we want and everything will work
