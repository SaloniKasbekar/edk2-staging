//
// MEM dump tool
//
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#define FMTSTR "%04X "/*short*/
#define TYPE short
#define ITEMSIZE sizeof(TYPE)
#define ITEMS_PER_LINE (16 / ITEMSIZE)
#define ITEM_MASK ((1LL << (8 * ITEMSIZE))-1)

int getitem(intptr_t addr)
{
    TYPE* p = (TYPE*)addr;
    return *p;            // get the data
}

int main(int argc, char **argv)
{
    int i, num, t;
    intptr_t start;

    if (argc < 3) {
        printf("ERROR: MEMdmp missing parameter, use:\n    MEMdmp start numbytes -- hex format\n");
        exit(EXIT_FAILURE);
    }

    t = sscanf(argv[1], "%llx", &start);
    t = sscanf(argv[2], "%x", &num);
    // additionally the numbers of tokens scanned can be checked, ignored here

    for (i = 0; i < num/ITEMSIZE; i++ )
    {
        int l/*ine*/, r/*everse*/;
        int linebuf[ITEMS_PER_LINE];

        if (0 == (i % ITEMS_PER_LINE))                              //
            printf("%016zX: ", start + i * ITEMSIZE);               // print the address
        linebuf[i % ITEMS_PER_LINE] = getitem(start + i * ITEMSIZE);// get the value to a temporary line buffer
        printf(FMTSTR, (int)(ITEM_MASK & linebuf[i % ITEMS_PER_LINE])); // print the value in hex format
                                                                    //
        if (0 == ((i + 1) % (ITEMS_PER_LINE/2))                     // check mid of line
            && 0 != ((i + 1) % ITEMS_PER_LINE))                     //
            printf("- ");                                           // print the separator
                                                                    //
        if (0 == ((i + 1) % ITEMS_PER_LINE))                        // check end of line
        {                                                           //
            printf("  ");                                           // insert space after hex print, 
            for (l = 0; l < ITEMS_PER_LINE; l++)                    // print ASCII representation
            {                                                       //
                for (r = ITEMSIZE * 8 - 8; r >= 0; r -= sizeof(char) * 8)
                {                                                   //
                    int v = linebuf[l] >> r;                        //
                    printf("%c", isalnum(0xFF & v) ? 0xFF & v : '.');
                }                                                   //
                printf(ITEMSIZE > 1 ? " " : "");                    //
            }                                                       //
            printf("\n");                                           // new line
        }
    }
    return EXIT_SUCCESS;
}