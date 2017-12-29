#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>


#define true 1
#define false 0
#define bool uint8_t
#define MAX 0x60


typedef struct group {
	char* buf;
	uint8_t ref_cnt;
} group_t;

typedef struct user {
	uint8_t age;
	char* name;
	char* group;
} user_t;


user_t* users[MAX];
group_t* names[MAX];

void read_until_nl_or_max(char *dst, size_t len) {

	size_t i = 0;
	for (;;) {
		
		if (read(0, &dst[i], 1) == -1)
			exit(1);
		if (dst[i] == '\n') {
			dst[i] = '\0';
			return;
		}
		
		i++;
		if (i >= len) {
			dst[i - 1] = '\0';
			return;
		}
	}
}

group_t* find_group_and_inc_ref_cnt(char* s) {

	uint16_t i = 0;
  // //fprintf(stderr, "called with %s\n", s);
	for (; i < MAX; ++i) {
		
		if (!names[i])
			continue;

		if (!strcmp(s, names[i]->buf) && names[i]->ref_cnt > 0) {
			names[i]->ref_cnt++;
			/*printf("incrementing ref count\n");*/
			return names[i];
		}

	}
	return NULL;
}

group_t* create_group_impl(char* s) {

	/*printf("creating new group\n");*/
	char* buf = (char*) malloc(24);
	group_t* res = (group_t*) malloc(sizeof(group_t));
	res->ref_cnt = 1;

	res->buf = buf; 
	/*printf("len: %d, str: %s\n", len, s);*/
	memcpy(res->buf, s, 24);
	return res;
}

group_t* try_create_group(char* s) {
	uint16_t i;

	for (i = 0; i < MAX; ++i) {
		if (!names[i])
			break;
	}

	if (i >= MAX)
		exit(1);

	names[i] = create_group_impl(s);

	return names[i];
	
}

void create_user() {
	
	char tmp[256];
	char tmp2[8];	
	char tmp3[32];	
	memset(tmp, 0, 256);
	memset(tmp2, 0, 8);
	memset(tmp3, 0, 24);
	
	printf("Please enter the user's name: ");
	read_until_nl_or_max(tmp, 0xc0);


	printf("Please enter the user's group: ");
	read_until_nl_or_max(tmp3, 24);

	

	printf("Please enter your age: ");
	read_until_nl_or_max(tmp2, 4);

	int age = atoi(tmp2);


	group_t* group = find_group_and_inc_ref_cnt(tmp3);
	if (!group) {
		// group not found, try to create it
		/*printf("String not found, adding it\n");*/
		group = try_create_group(tmp3);
	}
	uint16_t i;

	for (i = 0; i < MAX; ++i) {
		if (!users[i])
			break;
	}

	if (i >= MAX) {
		printf("User database full\n");
		exit(1);
	}

	users[i] = (user_t*) malloc(sizeof(user_t));
	users[i]->group = group->buf;
	users[i]->age = age;

	uint32_t len = strlen(tmp) + 1;
	users[i]->name = (char*) malloc(len);
	memcpy(users[i]->name, tmp, len);

	printf("User created\n");
}


void print_user(user_t* user) {

	printf("User:\n\tName: %s\n\tGroup: %s\n\tAge: %u\n", user->name, user->group, user->age);
}

void display_user() {
	
	char tmp[8];
	printf("Enter index: ");
	read_until_nl_or_max(tmp, 4);

	uint32_t index = atoi(tmp);

	if (index >= MAX) {
		printf("invalid index\n");
		return;
	}

	if (!users[index]) { 
		printf("No users at %u\n", index);
		return;
	}
	print_user(users[index]);
}


void display_group() {
	
	char tmp[32];
	printf("Enter group name: ");
	read_until_nl_or_max(tmp, 24);

	for (uint32_t i = 0; i < MAX; ++i) {

		if (!users[i])
			continue;

		if (!strcmp(tmp, users[i]->group)) {
			print_user(users[i]);	
		}
	}

}

void decrease_refcount(char* s) {

	uint16_t i = 0;

	for (; i < MAX; ++i) {
		
		if (!names[i])
			continue;

		if (!strcmp(s, names[i]->buf) && names[i]->ref_cnt > 0) {
			names[i]->ref_cnt--;
		}

	}
}

void delete_user() {

	char tmp[8];
	printf("Enter index: ");
	read_until_nl_or_max(tmp, 4);

	uint32_t index = atoi(tmp);
	
	if (index >= MAX) {
		printf("invalid index\n");
		return;
	}

	if (!users[index])
		return;
	
	decrease_refcount(users[index]->group);
	free(users[index]);	
	users[index] = NULL;

}


void* GC(void* args) {

	//fprintf(stderr, "Starting GC\n");
	sleep(1);
	uint32_t cycle = 0;
	for (;;) {
			
		//fprintf(stderr, "GC Cycle %u\n", cycle++);
		uint32_t i;
		for (i = 0; i < MAX; ++i) {
			if (!names[i]) {
				continue;
			}
			// fprintf(stderr, "checking ref_cnt for %8s, ", names[i]->buf);
			// fprintf(stderr, "ref_cnt is %hhu\n", names[i]->ref_cnt);
			if (names[i]->ref_cnt > 0) {
				continue;
			}

			// free group
	//		fprintf(stderr, "freeing group\n");
			free(names[i]->buf);
			free(names[i]);
			names[i] = NULL;

		}

		sleep(2);
	}
}

void edit_group() {

	char tmp[8];
	
	printf("Enter index: ");
	read_until_nl_or_max(tmp, 4);

	uint32_t index = atoi(tmp);
	
	if (!users[index])
		return;

	
	printf("Would you like to propagate the change, this will update the group of all the users sharing this group(y/n): ");
	read_until_nl_or_max(tmp, 2);
	printf("Enter new group name: ");
	if (tmp[0] == 'y') {
		read_until_nl_or_max(users[index]->group, 24);
		return;
	}

	char tmp2[32];
	read_until_nl_or_max(tmp2, 24);
	
	group_t* group = find_group_and_inc_ref_cnt(tmp2);
	if (!group) {
		group_t* new = try_create_group(tmp2);
		users[index]->group = new->buf;
		return;
	}
	users[index]->group = group->buf;
}

int main(void) {

	
	setvbuf(stdin, NULL, _IONBF, 0);
	setvbuf(stdout, NULL, _IONBF, 0);
	//fprintf(stderr, "sizeof user struct %lu\n", sizeof(user_t));
	pthread_t id;
	int c;

	alarm(160);

	pthread_create(&id, NULL, &GC, NULL);

	for (;;) {
		printf("0: Add a user\n");
		printf("1: Display a group\n");
		printf("2: Display a user\n");
		printf("3: Edit a group\n");
		printf("4: Delete a user\n");
		printf("5: Exit\n");
		printf("Action: ");
		if (scanf("%d", &c) == EOF)
			exit(1);
		if (c == 0) {	
			create_user();
		}
		if (c == 1) {
			display_group();
		}
		if (c == 2) {
			display_user();
		}
		if (c == 3) {
			edit_group();
		}
		if (c == 4) {
			delete_user();
		}
		if (c == 5) {
			printf("Bye\n");
			exit(0);
		}
	}
	//create_user();
	//delete_user();
		
	for (;;);
	return 0;
}
