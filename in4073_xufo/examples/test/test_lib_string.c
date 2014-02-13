#include <assert.h>
#include <string.h>

const char* string1 = "STRING";
const char* string2 = "string";
const char* string3 = "1234567890";
const char* string4 = "1234";

void test_lib_string() {
	char string5[100];
	char mem1[50];
	char mem2[50];
	char mem3[50];
	char mem4[50];
	int i;
	
	// strlen
	assert(strlen(string1)==6);
	assert(strlen(string2)==6);
	assert(strlen(string3)==10);
	assert(strlen(string4)==4);

	// strcmp
	assert(strcmp(string1, string1)==0);
	assert(strcmp(string1, string2)<0);
	assert(strcmp(string2, string1)>0);
	assert(strcmp(string2, string2)==0);

	// strcpy
	strcpy(string5, string3);
	assert(strcmp(string5, string3) == 0);
	assert(strlen(string5) == 10);
	strcpy(string5, string2);
	assert(strcmp(string5, string2) == 0);
	assert(strlen(string5) == 6);
	strcpy(string5, string3);
	assert(strcmp(string5, string3) == 0);
	assert(strlen(string5) == 10);
	
	// strncpy, strncmp
	strncpy(string5, string4, 8);
	assert(strlen(string5) == 4);
	assert(strcmp(string5, string4) == 0);
	assert(strncmp(string5, string3, 4) == 0);
	strncpy(string5, string1, 2);
	assert(strncmp(string5, string1, 2)==0);
	assert(strncmp(string5+2, string3+2, 2)==0);

	// strcat
	strcpy(string5, string1);
	strcat(string5, string3);
	assert(strcmp(string5+strlen(string1), string3)==0);

	// strchr
	assert(strchr(string1, 'S')==string1);
	assert(strchr(string1, 'R')==string1+2);
	assert(strchr(string1, 'G')==string1+5);
	assert(strchr(string1, 'v')==0);

	// strspn
	assert(strspn(string1, "ST")==2);
	assert(strspn(string1, "STA")==2);
	assert(strspn(string1, "SAT")==2);
	assert(strspn(string1, "sat")==0);
	assert(strspn(string1, "")==0);

	// strcspn
	assert(strcspn(string1, "ST")==0);
	assert(strcspn(string1, "STA")==0);
	assert(strcspn(string1, "SAT")==0);
	assert(strcspn(string1, "sat")==6);
	assert(strcspn(string1, "")==6);
	assert(strcspn(string1, "N")==4);

	// strtok
	strcpy(string5, string3);
	strcat(string5, string3);
	strcat(string5, string3);
	strcat(string5, string3);
	strcat(string5, string3);
	assert(strtok(string5, "9012")-string5==2);
	assert(strtok(NULL, "9012")-string5==12);
	assert(strtok(NULL, "9012")-string5==22);
	assert(strtok(NULL, "9012")-string5==32);
	assert(strtok(NULL, "9012")-string5==42);
	assert(strtok(NULL, "9012")==0);
	assert(strtok(NULL, "9012")==0);

	// memset, memcmp
	memset(mem1, '1', 50);
	memset(mem2, '2', 50);
	memset(mem3, '3', 50);
	memset(mem4, '4', 50);
	for (i = 0; i < 50; i++) assert(mem1[i]=='1');
	
	assert(memcmp(mem1, mem2, 10)<0);
	assert(memcmp(mem2, mem3, 10)<0);
	assert(memcmp(mem4, mem3, 10)>0);
	assert(memcmp(mem4, mem4, 10)==0);

	// memcpy
	memcpy(mem1+10, mem2, 20);
	for (i = 0; i < 10; i++) assert(mem1[i]=='1');
	for (i = 10; i < 30; i++) assert(mem1[i]=='2');
	for (i = 30; i < 50; i++) assert(mem1[i]=='1');

	// memchr
	assert(memchr(mem1, '2', 50)==mem1+10);

	//memmove
	memmove(mem1, mem1+20, 30);
	for (i = 0; i < 10; i++) assert(mem1[i]=='2');
	for (i = 10; i < 50; i++) assert(mem1[i]=='1');
	memmove(mem1+10, mem1, 20);
	for (i = 0; i < 20; i++) assert(mem1[i]=='2');
	for (i = 20; i < 50; i++) assert(mem1[i]=='1');
}

