#include    <stdio.h>

int main()
{
    int i, m = 1024 * 512 * 45;

    printf("<html><body><pre>\n");
    for (i = 0; i < m; i++) {
        printf("%08d aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n", i);
    }
    printf("</pre></body></html>\n");

    return 0;
}

