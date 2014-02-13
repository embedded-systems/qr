
int main() {
    int i, a, b;
    int *c, **d;
    int e[4];
    int f[2][2];
    char g[] = "hoei\n";
    char x = -1;

    e[0] = 1;
    e[1] = 2;
    e[2] = 3;
    e[3] = 4;

    f[0][0] = 1;
    f[0][1] = 2;
    f[1][0] = 3;
    f[1][1] = 4;

    printf("hello, world! %d %d %d %d %d\n", e, e[0], e[1], e[2], e[3]);
    printf("hello, world! %d %d %d %d %d\n", f, f[0][0], f[0][1], f[1][0], f[1][1]);

    for(i=0; i<4 ; i++) {
         printf("loop\n");
    }

    while
       (i++) {
        printf("hoi\n");
        if(i==8)
            break;
    }

    while(--i);

    i = 1;
    do {
        printf("hoi\n");
        if(i==4)
            break;

    } while(i++);

    i = 4;
    do { printf("hoi\n"); 
    } while(i-- > 0);

    if(a<b) {
        printf("hoera\n");
    }

    if(1<2) 
        printf("hoera\n");
    
    if(1<2) printf("hoera\n");

//    while(1);

    return 0;

}

