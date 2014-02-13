#include <x32.h>

int crap = 0x12345678;


void breakhierook(int uh, int arg) {
    char c;
    char c2[] = "heuhh?";
    char *d;
    char ***e;
    int bla[4];
    int *heph;
    int **hepl;

    int i;
    short int j; 
    unsigned short int k;

    k = 34000;
    k += k;
    arg = 1; 
    c = -10;
    bla[0] = 18;
    bla[0] += c;
 
    c = 140;
    c += c;
    c = 74;
    c += c;
    c = -120;
    c += c;

    c = 'x';
    d = &c;
    *d = 'y';
    c = 12;
    
    c = c2[2];

    bla[0] = 8;
    bla[1] = 9;
    heph = bla;
    bla[0] = 10;
    hepl = &heph;

    i = 257;
    j = 257;
    k = 257;
    
}

void breakhier() {
    int a;

    a = 1;
    a = 2;

    breakhierook(1, 7);

    printf("breakhier\n");
}

int main() {
        printf("start\n");

        breakhier();
        breakhier();
        breakhier();

    printf("done\n");
    printf("done\n");
    return 0;
}
