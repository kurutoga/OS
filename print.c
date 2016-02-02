#define BASE 10
#define HEX 16
#define OCTAL 8

typedef unsigned int u32;
char *table = "0123456789ABCDEF";
int _rpu(u32 x, u32 base) {

    char c;
    if (x) {
        c = table[x % base];
        _rpu(x/base, base);
        putchar(c);
    }
}

int printu(u32 x)
{
    if (x==0)
        putchar('0');
    else
        _rpu(x,BASE);
}

void prints(char *s) {
    while (*s)
        putchar(*(s++));
}

int printd(int x) {
    if (x<0) {
        putchar('-');
        x=-x;
    }
    printu(x);
}

int printo(u32 x) {
    putchar('0');
    if (x==0)
        putchar('0');
    else
        _rpu(x, OCTAL);
}

int printx(u32 x) {
    prints("0x");
    if (x==0)
        putchar('0');
    else
        _rpu(x, HEX);
}

int myprintf(char *fmt, ...) {
    char *cp;
    int *ip; 
    cp=fmt;             // cp set to fmt
    ip=(int *)&fmt+1;   // ip set to var 'a'
    while(*cp) {
        if (*cp=='%') {
            cp++;
            switch(*cp) {
                case 'c':
                    putchar(*ip);
                    break;
                case 's':
                    prints((char *)(*ip));
                    break;
                case 'u':
                    printu((u32)*ip);
                    break;
                case 'd':
                    printd((int)*ip);
                    break;
                case 'o':
                    printo((u32)*ip);
                    break;
                case 'x':
                    printx((u32)*ip);
                    break;
            }
            ip++;
        } else {
            if (*cp=='\n')
                putchar('\r');
            putchar(*cp);
        }
        cp++;
    }
}
