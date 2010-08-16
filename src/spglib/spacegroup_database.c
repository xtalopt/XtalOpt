/* spacegroup_database.c */
/* Copyright (C) 2008 Atsushi Togo */

/* This code is originally written in FORTRAN by ABINIT group. */
/* I converted FORTRAN to C using vi command and my hand. */
/* Copyright may belong to the ABINIT group. */

#include <string.h>
#include "spacegroup_database.h"

Spacegroup tbl_get_spacegroup_database(int spacegroup_number, int axis,
                                   int origin)
{

    char international_long[40] = "";
    char international[20];
    char schoenflies[10];
    char bravais_symbol[2];
    int multi = 1;
    Spacegroup spacegroup;

    switch (spacegroup_number) {
    case 1:
        strcpy(bravais_symbol, "P");
        strcpy(international, "1");
        strcpy(schoenflies, "C1^1");
        multi = 1;
        break;
    case 2:
        strcpy(bravais_symbol, "P");
        strcpy(international, "-1");
        strcpy(schoenflies, "Ci^1");
        multi = 2;
        break;
    case 3:
        strcpy(bravais_symbol, "P");
        strcpy(international, "2");
        strcpy(schoenflies, "C2^1");
        multi = 2;
        switch (axis) {
        case 1:
            strcpy(international_long, "P 2 _b = P 1 2 1");
            break;
        case 2:
            strcpy(international_long, "P 2_a = P 2 1 1");
            break;
        case 3:
            strcpy(international_long, "P 2 _c = P 1 1 2");
            break;
        }
        break;
    case 4:
        strcpy(bravais_symbol, "P");
        strcpy(international, "2_1");
        strcpy(schoenflies, "C2^2");
        multi = 2;
        switch (axis) {
        case 1:
            strcpy(international_long, "P 2 1 _b = P 1 2_1 1");
            break;
        case 2:
            strcpy(international_long, "P 2 1 _a = P 2_1 1 1");
            break;
        case 3:
            strcpy(international_long, "P 2 1 _c = P 1 1 2_1");
            break;
        }
        break;
    case 5:
        strcpy(bravais_symbol, "C");
        strcpy(international, "2");
        strcpy(schoenflies, "C2^3");
        multi = 2;
        switch (axis) {
        case 1:
            strcpy(international_long, "C 2 _b1 =  C 1 2 1");
            break;
        case 2:
            strcpy(international_long, "C 2 _a1 =  B 2 1 1");
            break;
        case 3:
            strcpy(international_long, "C 2 _a2 =  C 2 1 1");
            break;
        case 4:
            strcpy(international_long, "C 2 _a3 =  I 2 1 1");
            break;
        case 5:
            strcpy(international_long, "C 2 _b2 =  A 1 2 1");
            break;
        case 6:
            strcpy(international_long, "C 2 _b3 =  I 1 2 1");
            break;
        case 7:
            strcpy(international_long, "C 2 _c1 =  A 1 1 2");
            break;
        case 8:
            strcpy(international_long, "C 2 _c2 =  B 1 1 2 = B 2");
            break;
        case 9:
            strcpy(international_long, "C 2 _c3 =  I 1 1 2");
            break;
        }
        break;
    case 6:
        strcpy(bravais_symbol, "P");
        strcpy(international, "m");
        strcpy(schoenflies, "Cs^1");
        multi = 2;
        switch (axis) {
        case 1:
            strcpy(international_long, "P m _b = P 1 m 1");
            break;
        case 2:
            strcpy(international_long, "P m _a = P m 1 1");
            break;
        case 3:
            strcpy(international_long, "P m _c = P 1 1 m");
            break;
        }
        break;
    case 7:
        strcpy(bravais_symbol, "P");
        strcpy(international, "c");
        strcpy(schoenflies, "Cs^2");
        multi = 2;
        switch (axis) {
        case 1:
            strcpy(international_long, "P c _b1 = P 1 c 1");
            break;
        case 2:
            strcpy(international_long, "P c _a1 = P b 1 1");
            break;
        case 3:
            strcpy(international_long, "P c _a2 = P n 1 1");
            break;
        case 4:
            strcpy(international_long, "P c _a3 = P c 1 1");
            break;
        case 5:
            strcpy(international_long, "P c _b2 = P 1 n 1");
            break;
        case 6:
            strcpy(international_long, "P c _b3 = P 1 a 1");
            break;
        case 7:
            strcpy(international_long, "P c _c1 = P 1 1 a");
            break;
        case 8:
            strcpy(international_long, "P c _c2 = P 1 1 n");
            break;
        case 9:
            strcpy(international_long, "P c _c3 = P 1 1 b = P b");
            break;
        }
        break;
    case 8:
        strcpy(bravais_symbol, "C");
        strcpy(international, "m");
        strcpy(schoenflies, "Cs^3");
        multi = 4;
        switch (axis) {
        case 1:
            strcpy(international_long, "C m _b1 = C 1 m 1");
            break;
        case 2:
            strcpy(international_long, "C m _a1 = B m 1 1");
            break;
        case 3:
            strcpy(international_long, "C m _a2 = C m 1 1");
            break;
        case 4:
            strcpy(international_long, "C m _a3 = I m 1 1");
            break;
        case 5:
            strcpy(international_long, "C m _b2 = A 1 m 1");
            break;
        case 6:
            strcpy(international_long, "C m _b3 = I 1 m 1");
            break;
        case 7:
            strcpy(international_long, "C m _c1 = A 1 1 m");
            break;
        case 8:
            strcpy(international_long, "C m _c2 = B 1 1 m = B m");
            break;
        case 9:
            strcpy(international_long, "C m _c3 = I 1 1 m");
            break;
        }
        break;
    case 9:
        strcpy(bravais_symbol, "C");
        strcpy(international, "c");
        strcpy(schoenflies, "Cs^4");
        multi = 4;
        switch (axis) {
        case 1:
            strcpy(international_long, "C c _b1 = C 1 c 1");
            break;
        case 2:
            strcpy(international_long, "C c _a1 = B b 1 1");
            break;
        case 3:
            strcpy(international_long, "C c _a2 = C n 1 1");
            break;
        case 4:
            strcpy(international_long, "C c _a3 = I c 1 1");
            break;
        case 5:
            strcpy(international_long, "C c _b2 = A 1 n 1");
            break;
        case 6:
            strcpy(international_long, "C c _b3 = I 1 a 1");
            break;
        case 7:
            strcpy(international_long, "C c _c1 = A 1 1 a");
            break;
        case 8:
            strcpy(international_long, "C c _c2 = B 1 1 n");
            break;
        case 9:
            strcpy(international_long, "C c _c3 = I 1 1 b");
            break;
        }
        break;
    case 10:
        strcpy(bravais_symbol, "P");
        strcpy(international, "2/m");
        strcpy(schoenflies, "C2h^1");
        multi = 4;
        switch (axis) {
        case 1:
            strcpy(international_long, "P 2/m _b = P 1 2/m 1");
            break;
        case 2:
            strcpy(international_long, "P 2/m _a = P 2/m 1 1");
            break;
        case 3:
            strcpy(international_long, "P 2/m _c = P 1 1 2/m");
            break;
        }
        break;
    case 11:
        strcpy(bravais_symbol, "P");
        strcpy(international, "2_1/m");
        strcpy(schoenflies, "C2h^2");
        multi = 4;
        switch (axis) {
        case 1:
            strcpy(international_long, "P 2_1/m _b = P 1 2_1/m 1");
            break;
        case 2:
            strcpy(international_long, "P 2_1/m _a = P 2_1/m 1 1");
            break;
        case 3:
            strcpy(international_long, "P 2_1/m _c = P 1 1 2_1/m");
            break;
        }
        break;
    case 12:
        strcpy(bravais_symbol, "C");
        strcpy(international, "2/m");
        strcpy(schoenflies, "C2h^3");
        multi = 8;
        switch (axis) {
        case 1:
            strcpy(international_long, "C 2/m _b1 = C 1 2/m 1");
            break;
        case 2:
            strcpy(international_long, "C 2/m _a1 = B 2/m 1 1");
            break;
        case 3:
            strcpy(international_long, "C 2/m _a2 = C 2/m 1 1");
            break;
        case 4:
            strcpy(international_long, "C 2/m _a3 = I 2/m 1 1");
            break;
        case 5:
            strcpy(international_long, "C 2/m _b2 = A 1 2/m 1");
            break;
        case 6:
            strcpy(international_long, "C 2/m _b3 = I 1 2/m 1");
            break;
        case 7:
            strcpy(international_long, "C 2/m _c1 = A 1 1 2/m");
            break;
        case 8:
            strcpy(international_long, "C 2/m _c2 = B 1 1 2/m = B 2/m");
            break;
        case 9:
            strcpy(international_long, "C 2/m _c3 = I 1 1 2/m");
            break;
        }
        break;
    case 13:
        strcpy(bravais_symbol, "P");
        strcpy(international, "2/c");
        strcpy(schoenflies, "C2h^4");
        multi = 4;
        switch (axis) {
        case 1:
            strcpy(international_long, "P 2/c _b1 = P 1 2/c 1");
            break;
        case 2:
            strcpy(international_long, "P 2/c _a1 = P 2/b 1 1");
            break;
        case 3:
            strcpy(international_long, "P 2/c _a2 = P 2/n 1 1");
            break;
        case 4:
            strcpy(international_long, "P 2/c _a3 = P 2/c 1 1");
            break;
        case 5:
            strcpy(international_long, "P 2/c _b2 = P 1 2/n 1");
            break;
        case 6:
            strcpy(international_long, "P 2/c _b3 = P 1 2/a 1");
            break;
        case 7:
            strcpy(international_long, "P 2/c _c1 = P 1 1 2/a");
            break;
        case 8:
            strcpy(international_long, "P 2/c _c2 = P 1 1 2/n");
            break;
        case 9:
            strcpy(international_long, "P 2/c _c3 = P 1 1 2/b = P 2/b");
            break;
        }
        break;
    case 14:
        strcpy(bravais_symbol, "P");
        strcpy(international, "2_1/c");
        strcpy(schoenflies, "C2h^5");
        multi = 4;
        switch (axis) {
        case 1:
            strcpy(international_long, "P 2_1/c _b1 = P 1 2_1/c 1");
            break;
        case 2:
            strcpy(international_long, "P 2_1/c _a1 = P 2_1/b 1 1");
            break;
        case 3:
            strcpy(international_long, "P 2_1/c _a2 = P 2_1/n 1 1");
            break;
        case 4:
            strcpy(international_long, "P 2_1/c _a3 = P 2_1/c 1 1");
            break;
        case 5:
            strcpy(international_long, "P 2_1/c _b2 = P 1 2_1/n 1");
            break;
        case 6:
            strcpy(international_long, "P 2_1/c _b3 = P 1 2_1/a 1");
            break;
        case 7:
            strcpy(international_long, "P 2_1/c _c1 = P 1 1 2_1/a");
            break;
        case 8:
            strcpy(international_long, "P 2_1/c _c2 = P 1 1 2_1/n");
            break;
        case 9:
            strcpy(international_long,
                   "P 2_1/c _c3 = P 1 1 2_1/b = P 2_1/b");
            break;
        }
        break;
    case 15:
        strcpy(bravais_symbol, "C");
        strcpy(international, "2/c");
        strcpy(schoenflies, "C2h^6");
        multi = 8;
        switch (axis) {
        case 1:
            strcpy(international_long, "C 2/c _b1 = C 1 2/c 1");
            break;
        case 2:
            strcpy(international_long, "C 2/c _a1 = B 2/b 1 1");
            break;
        case 3:
            strcpy(international_long, "C 2/c _a2 = C 2/n 1 1");
            break;
        case 4:
            strcpy(international_long, "C 2/c _a3 = I 2/c 1 1");
            break;
        case 5:
            strcpy(international_long, "C 2/c _b2 = A 1 2/n 1");
            break;
        case 6:
            strcpy(international_long, "C 2/c _b3 = I 1 2/a 1");
            break;
        case 7:
            strcpy(international_long, "C 2/c _c1 = A 1 1 2/a");
            break;
        case 8:
            strcpy(international_long, "C 2/c _c2 = B 1 1 2/n");
            break;
        case 9:
            strcpy(international_long, "C 2/c _c3 = I 1 1 2/b");
            break;
        }
        break;
    case 16:
        strcpy(bravais_symbol, "P");
        strcpy(international, "2 2 2");
        strcpy(schoenflies, "D2^1");
        multi = 4;
        break;
    case 17:
        strcpy(bravais_symbol, "P");
        strcpy(international, "2 2 2_1");
        strcpy(schoenflies, "D2^2");
        multi = 4;
        switch (axis) {
        case 1:
            strcpy(international_long, "P 2 2 2_1");
            break;
        case 2:
            strcpy(international_long, "P 2_1 2 2");
            break;
        case 3:
            strcpy(international_long, "P 2 2_1 2");
            break;
        }
        break;
    case 18:
        strcpy(bravais_symbol, "P");
        strcpy(international, "2_1 2_1 2");
        strcpy(schoenflies, "D2^3");
        multi = 4;
        switch (axis) {
        case 1:
            strcpy(international_long, "P 2_1 2_1 2");
            break;
        case 2:
            strcpy(international_long, "P 2 2_1 2_1");
            break;
        case 3:
            strcpy(international_long, "P 2_1 2 2_1");
            break;
        }
        break;
    case 19:
        strcpy(bravais_symbol, "P");
        strcpy(international, "2_1 2_1 2_1");
        strcpy(schoenflies, "D2^4");
        multi = 4;
        break;
    case 20:
        strcpy(schoenflies, "D2^5");
        multi = 8;
        switch (axis) {
        case 1:
            strcpy(bravais_symbol, "C");
            strcpy(international, "2 2 2_1");
            break;
        case 2:
            strcpy(bravais_symbol, "A");
            strcpy(international, "2_1 2 2");
            break;
        case 3:
            strcpy(bravais_symbol, "B");
            strcpy(international, "2 2_1 2");
            break;
        }
        break;
    case 21:
        strcpy(schoenflies, "D2^6");
        multi = 8;
        switch (axis) {
        case 1:
            strcpy(bravais_symbol, "C");
            strcpy(international, "2 2 2");
            break;
        case 2:
            strcpy(bravais_symbol, "A");
            strcpy(international, "2 2 2");
            break;
        case 3:
            strcpy(bravais_symbol, "B");
            strcpy(international, "2 2 2");
            break;
        }
        break;
    case 22:
        strcpy(bravais_symbol, "F");
        strcpy(international, "2 2 2");
        strcpy(schoenflies, "D2^7");
        multi = 16;
        break;
    case 23:
        strcpy(bravais_symbol, "I");
        strcpy(international, "2 2 2");
        strcpy(schoenflies, "D2^8");
        multi = 8;
        break;
    case 24:
        strcpy(bravais_symbol, "I");
        strcpy(international, "2_1 2_1 2_1");
        strcpy(schoenflies, "D2^9");
        multi = 8;
        break;
    case 25:
        strcpy(bravais_symbol, "P");
        strcpy(schoenflies, "C2v^1");
        multi = 4;
        switch (axis) {
        case 1:
            strcpy(international, "m m 2");
            break;
        case 2:
            strcpy(international, "2 m m");
            break;
        case 3:
            strcpy(international, "m 2 m");
            break;
        }
        break;
    case 26:
        strcpy(bravais_symbol, "P");
        strcpy(schoenflies, "C2v^2");
        multi = 4;
        switch (axis) {
        case 1:
            strcpy(international, "m c 2_1");
            break;
        case 2:
            strcpy(international, "2_1 m a");
            break;
        case 3:
            strcpy(international, "b 2_1 m");
            break;
        case 4:
            strcpy(international, "m 2_1 b");
            break;
        case 5:
            strcpy(international, "c m 2_1");
            break;
        case 6:
            strcpy(international, "2_1 a m");
            break;
        }
        break;
    case 27:
        strcpy(bravais_symbol, "P");
        strcpy(schoenflies, "C2v^3");
        multi = 4;
        switch (axis) {
        case 1:
            strcpy(international, "c c 2");
            break;
        case 2:
            strcpy(international, "2 a a");
            break;
        case 3:
            strcpy(international, "b 2 b");
            break;
        }
        break;
    case 28:
        strcpy(bravais_symbol, "P");
        strcpy(schoenflies, "C2v^4");
        multi = 4;
        switch (axis) {
        case 1:
            strcpy(international, "m a 2");
            break;
        case 2:
            strcpy(international, "2 m b");
            break;
        case 3:
            strcpy(international, "c 2 m");
            break;
        case 4:
            strcpy(international, "m 2 a");
            break;
        case 5:
            strcpy(international, "b m 2");
            break;
        case 6:
            strcpy(international, "2 c m");
            break;
        }
        break;
    case 29:
        strcpy(bravais_symbol, "P");
        strcpy(schoenflies, "C2v^5");
        multi = 4;
        switch (axis) {
        case 1:
            strcpy(international, "c a 2_1");
            break;
        case 2:
            strcpy(international, "2_1 a b");
            break;
        case 3:
            strcpy(international, "c 2_1 b");
            break;
        case 4:
            strcpy(international, "b 2_1 a");
            break;
        case 5:
            strcpy(international, "b c 2_1");
            break;
        case 6:
            strcpy(international, "2_1 c a");
            break;
        }
        break;
    case 30:
        strcpy(bravais_symbol, "P");
        strcpy(schoenflies, "C2v^6");
        multi = 4;
        switch (axis) {
        case 1:
            strcpy(international, "n c 2");
            break;
        case 2:
            strcpy(international, "2 n a");
            break;
        case 3:
            strcpy(international, "b 2 n");
            break;
        case 4:
            strcpy(international, "n 2 b");
            break;
        case 5:
            strcpy(international, "c n 2");
            break;
        case 6:
            strcpy(international, "2 a n");
            break;
        }
        break;
    case 31:
        strcpy(bravais_symbol, "P");
        strcpy(schoenflies, "C2v^7");
        multi = 4;
        switch (axis) {
        case 1:
            strcpy(international, "m n 2_1");
            break;
        case 2:
            strcpy(international, "2_1 m n");
            break;
        case 3:
            strcpy(international, "n 2_1 m");
            break;
        case 4:
            strcpy(international, "m 2_1 n");
            break;
        case 5:
            strcpy(international, "n m 2_1");
            break;
        case 6:
            strcpy(international, "2_1 n m");
            break;
        }
        break;
    case 32:
        strcpy(bravais_symbol, "P");
        strcpy(schoenflies, "C2v^8");
        multi = 4;
        switch (axis) {
        case 1:
            strcpy(international, "b a 2");
            break;
        case 2:
            strcpy(international, "2 c b");
            break;
        case 3:
            strcpy(international, "c 2 a");
            break;
        }
        break;
    case 33:
        strcpy(bravais_symbol, "P");
        strcpy(schoenflies, "C2v^9");
        multi = 4;
        switch (axis) {
        case 1:
            strcpy(international, "n a 2_1");
            break;
        case 2:
            strcpy(international, "2_1 n b");
            break;
        case 3:
            strcpy(international, "c 2_1 n");
            break;
        case 4:
            strcpy(international, "n 2_1 a");
            break;
        case 5:
            strcpy(international, "b n 2_1");
            break;
        case 6:
            strcpy(international, "2_1 c n");
            break;
        }
        break;
    case 34:
        strcpy(bravais_symbol, "P");
        strcpy(schoenflies, "C2v^10");
        multi = 4;
        switch (axis) {
        case 1:
            strcpy(international, "n n 2");
            break;
        case 2:
            strcpy(international, "2 n n");
            break;
        case 3:
            strcpy(international, "n 2 n");
            break;
        }
        break;
    case 35:
        strcpy(schoenflies, "C2v^11");
        multi = 8;
        switch (axis) {
        case 1:
            strcpy(bravais_symbol, "C");
            strcpy(international, "m m 2");
            break;
        case 2:
            strcpy(bravais_symbol, "A");
            strcpy(international, "2 m m");
            break;
        case 3:
            strcpy(bravais_symbol, "B");
            strcpy(international, "m 2 m");
            break;
        }
        break;
    case 36:
        strcpy(schoenflies, "C2v^12");
        multi = 8;
        switch (axis) {
        case 1:
            strcpy(bravais_symbol, "C");
            strcpy(international, "m c 2_1");
            break;
        case 2:
            strcpy(bravais_symbol, "A");
            strcpy(international, "2_1 m a");
            break;
        case 3:
            strcpy(bravais_symbol, "B");
            strcpy(international, "b 2_1 m");
            break;
        case 4:
            strcpy(bravais_symbol, "B");
            strcpy(international, "m 2_1 b");
            break;
        case 5:
            strcpy(bravais_symbol, "C");
            strcpy(international, "c m 2_1");
            break;
        case 6:
            strcpy(bravais_symbol, "A");
            strcpy(international, "2_1 a m");
            break;
        }
        break;
    case 37:
        strcpy(schoenflies, "C2v^13");
        multi = 8;
        switch (axis) {
        case 1:
            strcpy(bravais_symbol, "C");
            strcpy(international, "c c 2");
            break;
        case 2:
            strcpy(bravais_symbol, "A");
            strcpy(international, "2 a a");
            break;
        case 3:
            strcpy(bravais_symbol, "B");
            strcpy(international, "b 2 b");
            break;
        }
        break;
    case 38:
        strcpy(schoenflies, "C2v^14");
        multi = 8;
        switch (axis) {
        case 1:
            strcpy(bravais_symbol, "A");
            strcpy(international, "m m 2");
            break;
        case 2:
            strcpy(bravais_symbol, "B");
            strcpy(international, "2 m m");
            break;
        case 3:
            strcpy(bravais_symbol, "C");
            strcpy(international, "m 2 m");
            break;
        case 4:
            strcpy(bravais_symbol, "A");
            strcpy(international, "m 2 m");
            break;
        case 5:
            strcpy(bravais_symbol, "B");
            strcpy(international, "m m 2");
            break;
        case 6:
            strcpy(bravais_symbol, "C");
            strcpy(international, "2 m m");
            break;
        }
        break;
    case 39:
        strcpy(schoenflies, "C2v^15");
        multi = 8;
        switch (axis) {
        case 1:
            strcpy(bravais_symbol, "A");
            strcpy(international, "b m 2");
            break;
        case 2:
            strcpy(bravais_symbol, "B");
            strcpy(international, "2 c m");
            break;
        case 3:
            strcpy(bravais_symbol, "C");
            strcpy(international, "m 2 a");
            break;
        case 4:
            strcpy(bravais_symbol, "A");
            strcpy(international, "c 2 m");
            break;
        case 5:
            strcpy(bravais_symbol, "B");
            strcpy(international, "m a 2");
            break;
        case 6:
            strcpy(bravais_symbol, "C");
            strcpy(international, "2 m b");
            break;
        }
        break;
    case 40:
        strcpy(schoenflies, "C2v^16");
        multi = 8;
        switch (axis) {
        case 1:
            strcpy(bravais_symbol, "A");
            strcpy(international, "m a 2");
            break;
        case 2:
            strcpy(bravais_symbol, "B");
            strcpy(international, "2 m b");
            break;
        case 3:
            strcpy(bravais_symbol, "C");
            strcpy(international, "c 2 m");
            break;
        case 4:
            strcpy(bravais_symbol, "A");
            strcpy(international, "m 2 a");
            break;
        case 5:
            strcpy(bravais_symbol, "B");
            strcpy(international, "b m 2");
            break;
        case 6:
            strcpy(bravais_symbol, "C");
            strcpy(international, "2 c m");
            break;
        }
        break;
    case 41:
        strcpy(schoenflies, "C2v^17");
        multi = 8;
        switch (axis) {
        case 1:
            strcpy(bravais_symbol, "A");
            strcpy(international, "b a 2");
            break;
        case 2:
            strcpy(bravais_symbol, "B");
            strcpy(international, "2 c b");
            break;
        case 3:
            strcpy(bravais_symbol, "C");
            strcpy(international, "c 2 a");
            break;
        case 4:
            strcpy(bravais_symbol, "A");
            strcpy(international, "c 2 a");
            break;
        case 5:
            strcpy(bravais_symbol, "B");
            strcpy(international, "b a 2");
            break;
        case 6:
            strcpy(bravais_symbol, "C");
            strcpy(international, "2 c b");
            break;
        }
        break;
    case 42:
        strcpy(bravais_symbol, "F");
        strcpy(schoenflies, "C2v^18");
        multi = 16;
        switch (axis) {
        case 1:
            strcpy(international, "m m 2");
            break;
        case 2:
            strcpy(international, "2 m m");
            break;
        case 3:
            strcpy(international, "m 2 m");
            break;
        }
        break;
    case 43:
        strcpy(bravais_symbol, "F");
        strcpy(schoenflies, "C2v^19");
        multi = 16;
        switch (axis) {
        case 1:
            strcpy(international, "d d 2");
            break;
        case 2:
            strcpy(international, "2 d d");
            break;
        case 3:
            strcpy(international, "d 2 d");
            break;
        }
        break;
    case 44:
        strcpy(bravais_symbol, "I");
        strcpy(schoenflies, "C2v^20");
        multi = 8;
        switch (axis) {
        case 1:
            strcpy(international, "m m 2");
            break;
        case 2:
            strcpy(international, "2 m m");
            break;
        case 3:
            strcpy(international, "m 2 m");
            break;
        }
        break;
    case 45:
        strcpy(bravais_symbol, "I");
        strcpy(schoenflies, "C2v^21");
        multi = 8;
        switch (axis) {
        case 1:
            strcpy(international, "b a 2");
            break;
        case 2:
            strcpy(international, "2 c b");
            break;
        case 3:
            strcpy(international, "c 2 a");
            break;
        }
        break;
    case 46:
        strcpy(bravais_symbol, "I");
        strcpy(schoenflies, "C2v^22");
        multi = 8;
        switch (axis) {
        case 1:
            strcpy(international, "m a 2");
            break;
        case 2:
            strcpy(international, "2 m b");
            break;
        case 3:
            strcpy(international, "c 2 m");
            break;
        case 4:
            strcpy(international, "m 2 a");
            break;
        case 5:
            strcpy(international, "b m 2");
            break;
        case 6:
            strcpy(international, "2 c m");
            break;
        }
        break;
    case 47:
        strcpy(bravais_symbol, "P");
        strcpy(international, "m m m");
        strcpy(schoenflies, "D2h^1");
        multi = 8;
        break;
    case 48:
        if (origin == 1) {
            strcpy(bravais_symbol, "P");
            strcpy(international, "n n n");
            strcpy(international_long, "n n n _1");
            strcpy(schoenflies, "D2h^2");
            multi = 8;
        }
        if (origin == 2) {
            strcpy(bravais_symbol, "P");
            strcpy(international, "n n n");
            strcpy(international_long, "n n n _2");
            strcpy(schoenflies, "D2h^2");
            multi = 8;
        }
        break;
    case 49:
        strcpy(bravais_symbol, "P");
        strcpy(schoenflies, "D2h^3");
        multi = 8;
        switch (axis) {
        case 1:
            strcpy(international, "c c m");
            break;
        case 2:
            strcpy(international, "m a a");
            break;
        case 3:
            strcpy(international, "b m b");
            break;
        }
        break;
    case 50:
        strcpy(bravais_symbol, "P");
        strcpy(schoenflies, "D2h^4");
        multi = 8;
        switch (origin) {
        case 1:
            switch (axis) {
            case 1:
                strcpy(international, "b a n");
                strcpy(international_long, "b a n _1");
                break;
            case 2:
                strcpy(international, "n c b");
                strcpy(international_long, "n c b _1");
                break;
            case 3:
                strcpy(international, "c n a");
                strcpy(international_long, "c n a _1");
                break;
            }
            break;
        case 2:
            switch (axis) {
            case 5:
                strcpy(international, "b a n");
                strcpy(international_long, "b a n _2");
                break;
            case 6:
                strcpy(international, "n c b");
                strcpy(international_long, "n c b _2");
                break;
            case 4:
                strcpy(international, "c n a");
                strcpy(international_long, "c n a _2");
                break;
            }
            break;
        }
        break;
    case 51:
        strcpy(bravais_symbol, "P");
        strcpy(schoenflies, "D2h^5");
        multi = 8;
        switch (axis) {
        case 1:
            strcpy(international, "m m a");
            break;
        case 2:
            strcpy(international, "b m m");
            break;
        case 3:
            strcpy(international, "m c m");
            break;
        case 4:
            strcpy(international, "m a m");
            break;
        case 5:
            strcpy(international, "m m b");
            break;
        case 6:
            strcpy(international, "c m m");
            break;
        }
        break;
    case 52:
        strcpy(bravais_symbol, "P");
        strcpy(schoenflies, "D2h^6");
        multi = 8;
        switch (axis) {
        case 1:
            strcpy(international, "n n a");
            break;
        case 2:
            strcpy(international, "b n n");
            break;
        case 3:
            strcpy(international, "n c n");
            break;
        case 4:
            strcpy(international, "n a n");
            break;
        case 5:
            strcpy(international, "n n b");
            break;
        case 6:
            strcpy(international, "c n n");
            break;
        }
        break;
    case 53:
        strcpy(bravais_symbol, "P");
        strcpy(schoenflies, "D2h^7");
        multi = 8;
        switch (axis) {
        case 1:
            strcpy(international, "m n a");
            break;
        case 2:
            strcpy(international, "b m n");
            break;
        case 3:
            strcpy(international, "n c m");
            break;
        case 4:
            strcpy(international, "m a n");
            break;
        case 5:
            strcpy(international, "n m b");
            break;
        case 6:
            strcpy(international, "c n m");
            break;
        }
        break;
    case 54:
        strcpy(bravais_symbol, "P");
        strcpy(schoenflies, "D2h^8");
        multi = 8;
        switch (axis) {
        case 1:
            strcpy(international, "c c a");
            break;
        case 2:
            strcpy(international, "b a a");
            break;
        case 3:
            strcpy(international, "b c b");
            break;
        case 4:
            strcpy(international, "b a b");
            break;
        case 5:
            strcpy(international, "c c b");
            break;
        case 6:
            strcpy(international, "c a a");
            break;
        }
        break;
    case 55:
        strcpy(bravais_symbol, "P");
        strcpy(schoenflies, "D2h^9");
        multi = 8;
        switch (axis) {
        case 1:
            strcpy(international, "b a m");
            break;
        case 2:
            strcpy(international, "m c b");
            break;
        case 3:
            strcpy(international, "c m a");
            break;
        }
        break;
    case 56:
        strcpy(bravais_symbol, "P");
        strcpy(schoenflies, "D2h^10");
        multi = 8;
        switch (axis) {
        case 1:
            strcpy(international, "c c n");
            break;
        case 2:
            strcpy(international, "n a a");
            break;
        case 3:
            strcpy(international, "b n b");
            break;
        }
        break;
    case 57:
        strcpy(bravais_symbol, "P");
        strcpy(schoenflies, "D2h^11");
        multi = 8;
        switch (axis) {
        case 1:
            strcpy(international, "b c m");
            break;
        case 2:
            strcpy(international, "m c a");
            break;
        case 3:
            strcpy(international, "b m a");
            break;
        case 4:
            strcpy(international, "c m b");
            break;
        case 5:
            strcpy(international, "c a m");
            break;
        case 6:
            strcpy(international, "m a b");
            break;
        }
        break;
    case 58:
        strcpy(bravais_symbol, "P");
        strcpy(schoenflies, "D2h^12");
        multi = 8;
        switch (axis) {
        case 1:
            strcpy(international, "n n m");
            break;
        case 2:
            strcpy(international, "m n n");
            break;
        case 3:
            strcpy(international, "n m n");
            break;
        }
        break;
    case 59:
        strcpy(bravais_symbol, "P");
        strcpy(schoenflies, "D2h^13");
        multi = 8;
        if (origin == 1)
            switch (axis) {
            case 1:
                strcpy(international, "m m n");
                strcpy(international_long, "m m n _1");
                break;
            case 2:
                strcpy(international, "m m n");
                strcpy(international_long, "n m m _1");
                break;
            case 3:
                strcpy(international, "m m n");
                strcpy(international_long, "m n m _1");
                break;
        } else
            switch (axis) {
            case 5:
                strcpy(international, "m m n");
                strcpy(international_long, "m m n _2");
                break;
            case 6:
                strcpy(international, "m m n");
                strcpy(international_long, "n m m _2");
                break;
            case 4:
                strcpy(international, "m m n");
                strcpy(international_long, "m n m _2");
                break;
            }
        break;
    case 60:
        strcpy(bravais_symbol, "P");
        strcpy(schoenflies, "D2h^14");
        multi = 8;
        switch (axis) {
        case 1:
            strcpy(international, "b c n");
            break;
        case 2:
            strcpy(international, "n c a");
            break;
        case 3:
            strcpy(international, "b n a");
            break;
        case 4:
            strcpy(international, "c n b");
            break;
        case 5:
            strcpy(international, "c a n");
            break;
        case 6:
            strcpy(international, "n a b");
            break;
        }
        break;
    case 61:
        strcpy(bravais_symbol, "P");
        strcpy(schoenflies, "D2h^15");
        multi = 8;
        if (axis == 1)
            strcpy(international, "b c a");
        if (axis == 2)
            strcpy(international, "c a b");
        break;
    case 62:
        strcpy(bravais_symbol, "P");
        strcpy(schoenflies, "D2h^16");
        multi = 8;
        switch (axis) {
        case 1:
            strcpy(international, "n m a");
            break;
        case 2:
            strcpy(international, "b n m");
            break;
        case 3:
            strcpy(international, "m c n");
            break;
        case 4:
            strcpy(international, "n a m");
            break;
        case 5:
            strcpy(international, "m n b");
            break;
        case 6:
            strcpy(international, "c m n");
            break;
        }
        break;
    case 63:
        strcpy(schoenflies, "D2h^17");
        multi = 16;
        switch (axis) {
        case 1:
            strcpy(bravais_symbol, "C");
            strcpy(international, "m c m");
            break;
        case 2:
            strcpy(bravais_symbol, "A");
            strcpy(international, "m m a");
            break;
        case 3:
            strcpy(bravais_symbol, "B");
            strcpy(international, "b m m");
            break;
        case 4:
            strcpy(bravais_symbol, "B");
            strcpy(international, "m m b");
            break;
        case 5:
            strcpy(bravais_symbol, "C");
            strcpy(international, "c m m");
            break;
        case 6:
            strcpy(bravais_symbol, "A");
            strcpy(international, "m a m");
            break;
        }
        break;
    case 64:
        strcpy(schoenflies, "D2h^18");
        multi = 16;
        switch (axis) {
        case 1:
            strcpy(bravais_symbol, "C");
            strcpy(international, "m c a");
            break;
        case 2:
            strcpy(bravais_symbol, "A");
            strcpy(international, "b m a");
            break;
        case 3:
            strcpy(bravais_symbol, "B");
            strcpy(international, "b c m");
            break;
        case 4:
            strcpy(bravais_symbol, "B");
            strcpy(international, "m a b");
            break;
        case 5:
            strcpy(bravais_symbol, "C");
            strcpy(international, "c m b");
            break;
        case 6:
            strcpy(bravais_symbol, "A");
            strcpy(international, "c a m");
            break;
        }
        break;
    case 65:
        strcpy(schoenflies, "D2h^19");
        multi = 16;
        switch (axis) {
        case 1:
            strcpy(bravais_symbol, "C");
            strcpy(international, "m m m");
            break;
        case 2:
            strcpy(bravais_symbol, "A");
            strcpy(international, "m m m");
            break;
        case 3:
            strcpy(bravais_symbol, "B");
            strcpy(international, "m m m");
            break;
        }
        break;
    case 66:
        strcpy(schoenflies, "D2h^20");
        multi = 16;
        switch (axis) {
        case 1:
            strcpy(bravais_symbol, "C");
            strcpy(international, "c c m");
            break;
        case 2:
            strcpy(bravais_symbol, "A");
            strcpy(international, "m a a");
            break;
        case 3:
            strcpy(bravais_symbol, "B");
            strcpy(international, "b m b");
            break;
        }
        break;
    case 67:
        strcpy(schoenflies, "D2h^21");
        multi = 16;
        switch (axis) {
        case 1:
            strcpy(bravais_symbol, "C");
            strcpy(international, "m m a");
            break;
        case 2:
            strcpy(bravais_symbol, "A");
            strcpy(international, "b m m");
            break;
        case 3:
            strcpy(bravais_symbol, "B");
            strcpy(international, "m c m");
            break;
        case 4:
            strcpy(bravais_symbol, "B");
            strcpy(international, "m a m");
            break;
        case 5:
            strcpy(bravais_symbol, "C");
            strcpy(international, "m m b");
            break;
        case 6:
            strcpy(bravais_symbol, "A");
            strcpy(international, "c m m");
            break;
        }
        break;
    case 68:
        strcpy(schoenflies, "D2h^22");
        multi = 16;
        if (origin == 1)
            switch (axis) {
            case 1:
                strcpy(bravais_symbol, "C");
                strcpy(international, "c c a");
                strcpy(international_long, "c c a _1");
                break;
            case 2:
                strcpy(bravais_symbol, "A");
                strcpy(international, "b a a");
                strcpy(international_long, "b a a _1");
                break;
            case 3:
                strcpy(bravais_symbol, "B");
                strcpy(international, "b c b");
                strcpy(international_long, "b c b _1");
                break;
            case 4:
                strcpy(bravais_symbol, "B");
                strcpy(international, "b a b");
                strcpy(international_long, "b a b _1");
                break;
            case 5:
                strcpy(bravais_symbol, "C");
                strcpy(international, "c c b");
                strcpy(international_long, "c c b _1");
                break;
            case 6:
                strcpy(bravais_symbol, "A");
                strcpy(international, "c a a");
                strcpy(international_long, "c a a _1");
                break;
        } else
            switch (axis) {
            case 1:
                strcpy(bravais_symbol, "C");
                strcpy(international, "c c a");
                strcpy(international_long, "c c a _2");
                break;
            case 2:
                strcpy(bravais_symbol, "A");
                strcpy(international, "b a a");
                strcpy(international_long, "b a a _2");
                break;
            case 3:
                strcpy(bravais_symbol, "B");
                strcpy(international, "b c b");
                strcpy(international_long, "b c b _2");
                break;
            case 4:
                strcpy(bravais_symbol, "B");
                strcpy(international, "b a b");
                strcpy(international_long, "b a b _2");
                break;
            case 5:
                strcpy(bravais_symbol, "C");
                strcpy(international, "c c b");
                strcpy(international_long, "c c b _2");
                break;
            case 6:
                strcpy(bravais_symbol, "A");
                strcpy(international, "c a a");
                strcpy(international_long, "c a a _2");
                break;
            }
        break;
    case 69:
        strcpy(bravais_symbol, "F");
        strcpy(international, "m m m");
        strcpy(schoenflies, "D2h^23");
        multi = 32;
        break;
    case 70:
        if (origin == 1) {
            strcpy(bravais_symbol, "F");
            strcpy(international, "d d d");
            strcpy(international_long, "d d d _1");
            strcpy(schoenflies, "D2h^24");
            multi = 32;
        }
        if (origin == 2) {
            strcpy(bravais_symbol, "F");
            strcpy(international, "d d d");
            strcpy(international_long, "d d d _2");
            strcpy(schoenflies, "D2h^24");
            multi = 32;
        }
        break;
    case 71:
        strcpy(bravais_symbol, "I");
        strcpy(international, "m m m");
        strcpy(schoenflies, "D2h^25");
        multi = 16;
        break;
    case 72:
        strcpy(bravais_symbol, "I");
        strcpy(schoenflies, "D2h^26");
        multi = 16;
        switch (axis) {
        case 1:
            strcpy(international, "b a m");
            break;
        case 2:
            strcpy(international, "m c b");
            break;
        case 3:
            strcpy(international, "c m a");
            break;
        }
        break;
    case 73:
        strcpy(bravais_symbol, "I");
        strcpy(schoenflies, "D2h^27");
        multi = 16;
        if (origin == 1)
            strcpy(international, "b c a");
        if (origin == 2)
            strcpy(international, "c a b");
        break;
    case 74:
        strcpy(bravais_symbol, "I");
        strcpy(schoenflies, "D2h^28");
        multi = 16;
        switch (axis) {
        case 1:
            strcpy(international, "m m a");
            break;
        case 2:
            strcpy(international, "b m m");
            break;
        case 3:
            strcpy(international, "m c m");
            break;
        case 4:
            strcpy(international, "m a m");
            break;
        case 5:
            strcpy(international, "m m b");
            break;
        case 6:
            strcpy(international, "c m m");
            break;
        }
        break;
    case 75:
        strcpy(bravais_symbol, "P");
        strcpy(international, "4");
        strcpy(schoenflies, "C4^1");
        multi = 4;
        break;
    case 76:
        strcpy(bravais_symbol, "P");
        strcpy(international, "4_1");
        strcpy(schoenflies, "C4^2");
        multi = 4;
        break;
    case 77:
        strcpy(bravais_symbol, "P");
        strcpy(international, "4_2");
        strcpy(schoenflies, "C4^3");
        multi = 4;
        break;
    case 78:
        strcpy(bravais_symbol, "P");
        strcpy(international, "4_3");
        strcpy(schoenflies, "C4^4");
        multi = 4;
        break;
    case 79:
        strcpy(bravais_symbol, "I");
        strcpy(international, "4");
        strcpy(schoenflies, "C4^5");
        multi = 8;
        break;
    case 80:
        strcpy(bravais_symbol, "I");
        strcpy(international, "4_1");
        strcpy(schoenflies, "C4^6");
        multi = 8;
        break;
    case 81:
        strcpy(bravais_symbol, "P");
        strcpy(international, "-4");
        strcpy(schoenflies, "S4^1");
        multi = 4;
        break;
    case 82:
        strcpy(bravais_symbol, "I");
        strcpy(international, "-4");
        strcpy(schoenflies, "S4^2");
        multi = 8;
        break;
    case 83:
        strcpy(bravais_symbol, "P");
        strcpy(international, "4/m");
        strcpy(schoenflies, "C4h^1");
        multi = 8;
        break;
    case 84:
        strcpy(bravais_symbol, "P");
        strcpy(international, "4_2/m");
        strcpy(schoenflies, "C4h^2");
        multi = 8;
        break;
    case 85:
        if (origin == 1) {
            strcpy(bravais_symbol, "P");
            strcpy(international, "4/n");
            strcpy(international_long, "4/n _1");
            strcpy(schoenflies, "C4h^3");
            multi = 8;
        }
        if (origin == 2) {
            strcpy(bravais_symbol, "P");
            strcpy(international, "4/n");
            strcpy(international_long, "4/n _2");
            strcpy(schoenflies, "C4h^3");
            multi = 8;
        }
        break;
    case 86:
        if (origin == 1) {
            strcpy(bravais_symbol, "P");
            strcpy(international, "4_2/n");
            strcpy(international_long, "4_2/n _1");
            strcpy(schoenflies, "C4h^4");
            multi = 8;
        }
        if (origin == 2) {
            strcpy(bravais_symbol, "P");
            strcpy(international, "4_2/n");
            strcpy(international_long, "4_2/n _2");
            strcpy(schoenflies, "C4h^4");
            multi = 8;
        }
        break;
    case 87:
        strcpy(bravais_symbol, "I");
        strcpy(international, "4/m");
        strcpy(schoenflies, "C4h^5");
        multi = 16;
        break;
    case 88:
        if (origin == 1) {
            strcpy(bravais_symbol, "I");
            strcpy(international, "4_1/a");
            strcpy(international_long, "4_1/a _1");
            strcpy(schoenflies, "C4h^6");
            multi = 16;
        }
        if (origin == 2) {
            strcpy(bravais_symbol, "I");
            strcpy(international, "4_1/a");
            strcpy(international_long, "4_1/a _2");
            strcpy(schoenflies, "C4h^6");
            multi = 16;
        }
        break;
    case 89:
        strcpy(bravais_symbol, "P");
        strcpy(international, "4 2 2");
        strcpy(schoenflies, "D4^1");
        multi = 8;
        break;
    case 90:
        strcpy(bravais_symbol, "P");
        strcpy(international, "4 2_1 2");
        strcpy(schoenflies, "D4^2");
        multi = 8;
        break;
    case 91:
        strcpy(bravais_symbol, "P");
        strcpy(international, "4_1 2 2");
        strcpy(schoenflies, "D4^3");
        multi = 8;
        break;
    case 92:
        strcpy(bravais_symbol, "P");
        strcpy(international, "4_1 2_1 2");
        strcpy(schoenflies, "D4^4");
        multi = 8;
        break;
    case 93:
        strcpy(bravais_symbol, "P");
        strcpy(international, "4_2 2 2");
        strcpy(schoenflies, "D4^5");
        multi = 8;
        break;
    case 94:
        strcpy(bravais_symbol, "P");
        strcpy(international, "4_2 2_1 2");
        strcpy(schoenflies, "D4^6");
        multi = 8;
        break;
    case 95:
        strcpy(bravais_symbol, "P");
        strcpy(international, "4_3 2 2");
        strcpy(schoenflies, "D4^7");
        multi = 8;
        break;
    case 96:
        strcpy(bravais_symbol, "P");
        strcpy(international, "4_3 2_1 2");
        strcpy(schoenflies, "D4^8");
        multi = 8;
        break;
    case 97:
        strcpy(bravais_symbol, "I");
        strcpy(international, "4 2 2");
        strcpy(schoenflies, "D4^9");
        multi = 16;
        break;
    case 98:
        strcpy(bravais_symbol, "I");
        strcpy(international, "4_1 2 2");
        strcpy(schoenflies, "D4^10");
        multi = 16;
        break;
    case 99:
        strcpy(bravais_symbol, "P");
        strcpy(international, "4 m m");
        strcpy(schoenflies, "C4v^1");
        multi = 8;
        break;
    case 100:
        strcpy(bravais_symbol, "P");
        strcpy(international, "4 b m");
        strcpy(schoenflies, "C4v^2");
        multi = 8;
        break;
    case 101:
        strcpy(bravais_symbol, "P");
        strcpy(international, "4_2 c m");
        strcpy(schoenflies, "C4v^3");
        multi = 8;
        break;
    case 102:
        strcpy(bravais_symbol, "P");
        strcpy(international, "4_2 n m");
        strcpy(schoenflies, "C4v^4");
        multi = 8;
        break;
    case 103:
        strcpy(bravais_symbol, "P");
        strcpy(international, "4 c c");
        strcpy(schoenflies, "C4v^5");
        multi = 8;
        break;
    case 104:
        strcpy(bravais_symbol, "P");
        strcpy(international, "4 n c");
        strcpy(schoenflies, "C4v^6");
        multi = 8;
        break;
    case 105:
        strcpy(bravais_symbol, "P");
        strcpy(international, "4_2 m c");
        strcpy(schoenflies, "C4v^7");
        multi = 8;
        break;
    case 106:
        strcpy(bravais_symbol, "P");
        strcpy(international, "4_2 b c");
        strcpy(schoenflies, "C4v^8");
        multi = 8;
        break;
    case 107:
        strcpy(bravais_symbol, "I");
        strcpy(international, "4 m m");
        strcpy(schoenflies, "C4v^9");
        multi = 16;
        break;
    case 108:
        strcpy(bravais_symbol, "I");
        strcpy(international, "4 c m");
        strcpy(schoenflies, "C4v^10");
        multi = 16;
        break;
    case 109:
        strcpy(bravais_symbol, "I");
        strcpy(international, "4_1 m d");
        strcpy(schoenflies, "C4v^11");
        multi = 16;
        break;
    case 110:
        strcpy(bravais_symbol, "I");
        strcpy(international, "4_1 c d");
        strcpy(schoenflies, "C4v^12");
        multi = 16;
        break;
    case 111:
        strcpy(bravais_symbol, "P");
        strcpy(international, "-4 2 m");
        strcpy(schoenflies, "D2d^1");
        multi = 8;
        break;
    case 112:
        strcpy(bravais_symbol, "P");
        strcpy(international, "-4 2 c");
        strcpy(schoenflies, "D2d^2");
        multi = 8;
        break;
    case 113:
        strcpy(bravais_symbol, "P");
        strcpy(international, "-4 2_1 m");
        strcpy(schoenflies, "D2d^3");
        multi = 8;
        break;
    case 114:
        strcpy(bravais_symbol, "P");
        strcpy(international, "-4 2_1 c");
        strcpy(schoenflies, "D2d^4");
        multi = 8;
        break;
    case 115:
        strcpy(bravais_symbol, "P");
        strcpy(international, "-4 m 2");
        strcpy(schoenflies, "D2d^5");
        multi = 8;
        break;
    case 116:
        strcpy(bravais_symbol, "P");
        strcpy(international, "-4 c 2");
        strcpy(schoenflies, "D2d^6");
        multi = 8;
        break;
    case 117:
        strcpy(bravais_symbol, "P");
        strcpy(international, "-4 b 2");
        strcpy(schoenflies, "D2d^7");
        multi = 8;
        break;
    case 118:
        strcpy(bravais_symbol, "P");
        strcpy(international, "-4 n 2");
        strcpy(schoenflies, "D2d^8");
        multi = 8;
        break;
    case 119:
        strcpy(bravais_symbol, "I");
        strcpy(international, "-4 m 2");
        strcpy(schoenflies, "D2d^9");
        multi = 16;
        break;
    case 120:
        strcpy(bravais_symbol, "I");
        strcpy(international, "-4 c 2");
        strcpy(schoenflies, "D2d^10");
        multi = 16;
        break;
    case 121:
        strcpy(bravais_symbol, "I");
        strcpy(international, "-4 2 m");
        strcpy(schoenflies, "D2d^11");
        multi = 16;
        break;
    case 122:
        strcpy(bravais_symbol, "I");
        strcpy(international, "-4 2 d");
        strcpy(schoenflies, "D2d^12");
        multi = 16;
        break;
    case 123:
        strcpy(bravais_symbol, "P");
        strcpy(international, "4/m m m");
        strcpy(schoenflies, "D4h^1");
        multi = 16;
        break;
    case 124:
        strcpy(bravais_symbol, "P");
        strcpy(international, "4/m c c");
        strcpy(schoenflies, "D4h^2");
        multi = 16;
        break;
    case 125:
        if (origin == 1) {
            strcpy(bravais_symbol, "P");
            strcpy(international, "4/n b m");
            strcpy(international_long, "4/n b m _1");
            strcpy(schoenflies, "D4h^3");
            multi = 16;
        }
        if (origin == 2) {
            strcpy(bravais_symbol, "P");
            strcpy(international, "4/n b m");
            strcpy(international_long, "4/n b m _2");
            strcpy(schoenflies, "D4h^3");
            multi = 16;
        }
        break;
    case 126:
        if (origin == 1) {
            strcpy(bravais_symbol, "P");
            strcpy(international, "4/n n c");
            strcpy(international_long, "4/n n c _1");
            strcpy(schoenflies, "D4h^4");
            multi = 16;
        }
        if (origin == 2) {
            strcpy(bravais_symbol, "P");
            strcpy(international, "4/n n c");
            strcpy(international_long, "4/n n c _2");
            strcpy(schoenflies, "D4h^4");
            multi = 16;
        }
        break;
    case 127:
        strcpy(bravais_symbol, "P");
        strcpy(international, "4/m b m");
        strcpy(schoenflies, "D4h^5");
        multi = 16;
        break;
    case 128:
        strcpy(bravais_symbol, "P");
        strcpy(international, "4/m n c");
        strcpy(schoenflies, "D4h^6");
        multi = 16;
        break;
    case 129:
        if (origin == 1) {
            strcpy(bravais_symbol, "P");
            strcpy(international, "4/n m m");
            strcpy(international_long, "4/n m m _1");
            strcpy(schoenflies, "D4h^7");
            multi = 16;
        }
        if (origin == 2) {
            strcpy(bravais_symbol, "P");
            strcpy(international, "4/n m m");
            strcpy(international_long, "4/n m m _2");
            strcpy(schoenflies, "D4h^7");
            multi = 16;
        }
        break;
    case 130:
        if (origin == 1) {
            strcpy(bravais_symbol, "P");
            strcpy(international, "4/n c c");
            strcpy(international_long, "4/n c c _1");
            strcpy(schoenflies, "D4h^8");
            multi = 16;
        }
        if (origin == 1) {
            strcpy(bravais_symbol, "P");
            strcpy(international, "4/n c c");
            strcpy(international_long, "4/n c c _1");
            strcpy(schoenflies, "D4h^8");
            multi = 16;
        }
        break;
    case 131:
        strcpy(bravais_symbol, "P");
        strcpy(international, "4_2/m m c");
        strcpy(schoenflies, "D4h^9");
        multi = 16;
        break;
    case 132:
        strcpy(bravais_symbol, "P");
        strcpy(international, "4_2/m c m");
        strcpy(schoenflies, "D4h^10");
        multi = 16;
        break;
    case 133:
        if (origin == 1) {
            strcpy(bravais_symbol, "P");
            strcpy(international, "4_2/n b c");
            strcpy(international_long, "4_2/n b c _1");
            strcpy(schoenflies, "D4h^11");
            multi = 16;
        }
        if (origin == 2) {
            strcpy(bravais_symbol, "P");
            strcpy(international, "4_2/n b c");
            strcpy(international_long, "4_2/n b c _2");
            strcpy(schoenflies, "D4h^11");
            multi = 16;
        }
        break;
    case 134:
        if (origin == 1) {
            strcpy(bravais_symbol, "P");
            strcpy(international, "4_2/n n m");
            strcpy(international_long, "4_2/n n m _1");
            strcpy(schoenflies, "D4h^12");
            multi = 16;
        }
        if (origin == 2) {
            strcpy(bravais_symbol, "P");
            strcpy(international, "4_2/n n m");
            strcpy(international_long, "4_2/n n m _2");
            strcpy(schoenflies, "D4h^12");
            multi = 16;
        }
        break;
    case 135:
        strcpy(bravais_symbol, "P");
        strcpy(international, "4_2/m b c");
        strcpy(schoenflies, "D4h^13");
        multi = 16;
        break;
    case 136:
        strcpy(bravais_symbol, "P");
        strcpy(international, "4_2/m n m");
        strcpy(schoenflies, "D4h^14");
        multi = 16;
        break;
    case 137:
        if (origin == 1) {
            strcpy(bravais_symbol, "P");
            strcpy(international, "4_2/n m c");
            strcpy(international_long, "4_2/n m c _1");
            strcpy(schoenflies, "D4h^15");
            multi = 16;
        }
        if (origin == 2) {
            strcpy(bravais_symbol, "P");
            strcpy(international, "4_2/n m c");
            strcpy(international_long, "4_2/n m c _2");
            strcpy(schoenflies, "D4h^15");
            multi = 16;
        }
        break;
    case 138:
        if (origin == 1) {
            strcpy(bravais_symbol, "P");
            strcpy(international, "4_2/n c m");
            strcpy(international_long, "4_2/n c m _1");
            strcpy(schoenflies, "D4h^16");
            multi = 16;
        }
        if (origin == 2) {
            strcpy(bravais_symbol, "P");
            strcpy(international, "4_2/n c m");
            strcpy(international_long, "4_2/n c m _2");
            strcpy(schoenflies, "D4h^16");
            multi = 16;
        }
        break;
    case 139:
        strcpy(bravais_symbol, "I");
        strcpy(international, "4/m m m");
        strcpy(schoenflies, "D4h^17");
        multi = 32;
        break;
    case 140:
        strcpy(bravais_symbol, "I");
        strcpy(international, "4/m c m");
        strcpy(schoenflies, "D4h^18");
        multi = 32;
        break;
    case 141:
        if (origin == 1) {
            strcpy(bravais_symbol, "I");
            strcpy(international, "4_1/a m d");
            strcpy(international_long, "4_1/a m d _1");
            strcpy(schoenflies, "D4h^19");
            multi = 32;
        }
        if (origin == 2) {
            strcpy(bravais_symbol, "I");
            strcpy(international, "4_1/a m d");
            strcpy(international_long, "4_1/a m d _2");
            strcpy(schoenflies, "D4h^19");
            multi = 32;
        }
        break;
    case 142:
        if (origin == 1) {
            strcpy(bravais_symbol, "I");
            strcpy(international, "4_1/a c d");
            strcpy(international_long, "4_1/a c d _1");
            strcpy(schoenflies, "D4h^20");
            multi = 32;
        }
        if (origin == 2) {
            strcpy(bravais_symbol, "I");
            strcpy(international, "4_1/a c d");
            strcpy(international_long, "4_1/a c d _2");
            strcpy(schoenflies, "D4h^20");
            multi = 32;
        }
        break;
    case 143:
        strcpy(bravais_symbol, "P");
        strcpy(international, "3");
        strcpy(schoenflies, "C3^1");
        multi = 3;
        break;
    case 144:
        strcpy(bravais_symbol, "P");
        strcpy(international, "3_1");
        strcpy(schoenflies, "C3^2");
        multi = 3;
        break;
    case 145:
        strcpy(bravais_symbol, "P");
        strcpy(international, "3_2");
        strcpy(schoenflies, "C3^3");
        multi = 3;
        break;
    case 146:
        if (origin == 1) {
            strcpy(bravais_symbol, "R");
            strcpy(international, "3");
            strcpy(international_long, "3 _H");
            strcpy(schoenflies, "C3^4");
            multi = 9;
        }
        if (origin == 2) {
            strcpy(bravais_symbol, "R");
            strcpy(international, "3");
            strcpy(international_long, "3 _R");
            strcpy(schoenflies, "C3^4");
            multi = 3;
        }
        break;
    case 147:
        strcpy(bravais_symbol, "P");
        strcpy(international, "-3");
        strcpy(schoenflies, "C3i^1");
        multi = 6;
        break;
    case 148:
        if (origin == 1) {
            strcpy(bravais_symbol, "R");
            strcpy(international, "-3");
            strcpy(international_long, "-3 _H");
            strcpy(schoenflies, "C3i^2");
            multi = 9;
        }
        if (origin == 2) {
            strcpy(bravais_symbol, "R");
            strcpy(international, "-3");
            strcpy(international_long, "-3 _R");
            strcpy(schoenflies, "C3i^2");
            multi = 9;
        }
        break;
    case 149:
        strcpy(bravais_symbol, "P");
        strcpy(international, "3 1 2");
        strcpy(schoenflies, "D3^1");
        multi = 6;
        break;
    case 150:
        strcpy(bravais_symbol, "P");
        strcpy(international, "3 2 1");
        strcpy(schoenflies, "D3^2");
        multi = 6;
        break;
    case 151:
        strcpy(bravais_symbol, "P");
        strcpy(international, "3_1 1 2");
        strcpy(schoenflies, "D3^3");
        multi = 6;
        break;
    case 152:
        strcpy(bravais_symbol, "P");
        strcpy(international, "3_1 2 1");
        strcpy(schoenflies, "D3^4");
        multi = 6;
        break;
    case 153:
        strcpy(bravais_symbol, "P");
        strcpy(international, "3_2 1 2");
        strcpy(schoenflies, "D3^5");
        multi = 6;
        break;
    case 154:
        strcpy(bravais_symbol, "P");
        strcpy(international, "3_2 2 1");
        strcpy(schoenflies, "D3^6");
        multi = 6;
        break;
    case 155:
        if (origin == 1) {
            strcpy(bravais_symbol, "R");
            strcpy(international, "3 2");
            strcpy(international_long, "3 2 _H");
            strcpy(schoenflies, "D3^7");
            multi = 18;
        }
        if (origin == 2) {
            strcpy(bravais_symbol, "R");
            strcpy(international, "3 2");
            strcpy(international_long, "3 2 _R");
            strcpy(schoenflies, "D3^7");
            multi = 6;
        }
        break;
    case 156:
        strcpy(bravais_symbol, "P");
        strcpy(international, "3 m 1");
        strcpy(schoenflies, "C3v^1");
        multi = 6;
        break;
    case 157:
        strcpy(bravais_symbol, "P");
        strcpy(international, "3 1 m");
        strcpy(schoenflies, "C3v^2");
        multi = 6;
        break;
    case 158:
        strcpy(bravais_symbol, "P");
        strcpy(international, "3 c 1");
        strcpy(schoenflies, "C3v^3");
        multi = 6;
        break;
    case 159:
        strcpy(bravais_symbol, "P");
        strcpy(international, "3 1 c");
        strcpy(schoenflies, "C3v^4");
        multi = 6;
        break;
    case 160:
        if (origin == 1) {
            strcpy(bravais_symbol, "R");
            strcpy(international, "3 m");
            strcpy(international_long, "3 m _H");
            strcpy(schoenflies, "C3v^5");
            multi = 18;
        }
        if (origin == 2) {
            strcpy(bravais_symbol, "R");
            strcpy(international, "3 m");
            strcpy(international_long, "3 m _R");
            strcpy(schoenflies, "C3v^5");
            multi = 6;
        }
        break;
    case 161:
        if (origin == 1) {
            strcpy(bravais_symbol, "R");
            strcpy(international, "3 c");
            strcpy(international_long, "3 m _H");
            strcpy(schoenflies, "C3v^6");
            multi = 18;
        }
        if (origin == 2) {
            strcpy(bravais_symbol, "R");
            strcpy(international, "3 c");
            strcpy(international_long, "3 m _R");
            strcpy(schoenflies, "C3v^6");
            multi = 6;
        }
        break;
    case 162:
        strcpy(bravais_symbol, "P");
        strcpy(international, "-3 1 m");
        strcpy(schoenflies, "D3d^1");
        multi = 12;
        break;
    case 163:
        strcpy(bravais_symbol, "P");
        strcpy(international, "-3 1 c");
        strcpy(schoenflies, "D3d^2");
        multi = 12;
        break;
    case 164:
        strcpy(bravais_symbol, "P");
        strcpy(international, "-3 m 1");
        strcpy(schoenflies, "D3d^3");
        multi = 12;
        break;
    case 165:
        strcpy(bravais_symbol, "P");
        strcpy(international, "-3 c 1");
        strcpy(schoenflies, "D3d^4");
        multi = 12;
        break;
    case 166:
        if (origin == 1) {
            strcpy(bravais_symbol, "R");
            strcpy(international, "-3 m");
            strcpy(international_long, "3 m _H");
            strcpy(schoenflies, "D3d^5");
            multi = 18;
        }
        if (origin == 2) {
            strcpy(bravais_symbol, "R");
            strcpy(international, "-3 m");
            strcpy(international_long, "3 m _R");
            strcpy(schoenflies, "D3d^5");
            multi = 6;
        }
        break;
    case 167:
        if (origin == 1) {
            strcpy(bravais_symbol, "R");
            strcpy(international, "-3 c");
            strcpy(international_long, "-3 c _H");
            strcpy(schoenflies, "D3d^6");
            multi = 36;
        }
        if (origin == 2) {
            strcpy(bravais_symbol, "R");
            strcpy(international, "-3 c");
            strcpy(international_long, "-3 c _R");
            strcpy(schoenflies, "D3d^6");
            multi = 12;
        }
        break;
    case 168:
        strcpy(bravais_symbol, "P");
        strcpy(international, "6");
        strcpy(schoenflies, "C6^1");
        multi = 6;
        break;
    case 169:
        strcpy(bravais_symbol, "P");
        strcpy(international, "6_1");
        strcpy(schoenflies, "C6^2");
        multi = 6;
        break;
    case 170:
        strcpy(bravais_symbol, "P");
        strcpy(international, "6_5");
        strcpy(schoenflies, "C6^3");
        multi = 6;
        break;
    case 171:
        strcpy(bravais_symbol, "P");
        strcpy(international, "6_2");
        strcpy(schoenflies, "C6^4");
        multi = 6;
        break;
    case 172:
        strcpy(bravais_symbol, "P");
        strcpy(international, "6_4");
        strcpy(schoenflies, "C6^5");
        multi = 6;
        break;
    case 173:
        strcpy(bravais_symbol, "P");
        strcpy(international, "6_3");
        strcpy(schoenflies, "C6^6");
        multi = 6;
        break;
    case 174:
        strcpy(bravais_symbol, "P");
        strcpy(international, "-6");
        strcpy(schoenflies, "C3h^1");
        multi = 6;
        break;
    case 175:
        strcpy(bravais_symbol, "P");
        strcpy(international, "6/m");
        strcpy(schoenflies, "C6h^1");
        multi = 12;
        break;
    case 176:
        strcpy(bravais_symbol, "P");
        strcpy(international, "6_3/m");
        strcpy(schoenflies, "C6h^2");
        multi = 12;
        break;
    case 177:
        strcpy(bravais_symbol, "P");
        strcpy(international, "6 2 2");
        strcpy(schoenflies, "D6^1");
        multi = 12;
        break;
    case 178:
        strcpy(bravais_symbol, "P");
        strcpy(international, "6_1 2 2");
        strcpy(schoenflies, "D6^2");
        multi = 12;
        break;
    case 179:
        strcpy(bravais_symbol, "P");
        strcpy(international, "6_5 2 2");
        strcpy(schoenflies, "D6^3");
        multi = 12;
        break;
    case 180:
        strcpy(bravais_symbol, "P");
        strcpy(international, "6_2 2 2");
        strcpy(schoenflies, "D6^4");
        multi = 12;
        break;
    case 181:
        strcpy(bravais_symbol, "P");
        strcpy(international, "6_4 2 2");
        strcpy(schoenflies, "D6^5");
        multi = 12;
        break;
    case 182:
        strcpy(bravais_symbol, "P");
        strcpy(international, "6_3 2 2");
        strcpy(schoenflies, "D6^6");
        multi = 12;
        break;
    case 183:
        strcpy(bravais_symbol, "P");
        strcpy(international, "6 m m");
        strcpy(schoenflies, "C6v^1");
        multi = 12;
        break;
    case 184:
        strcpy(bravais_symbol, "P");
        strcpy(international, "6 c c");
        strcpy(schoenflies, "C6v^2");
        multi = 12;
        break;
    case 185:
        strcpy(bravais_symbol, "P");
        strcpy(international, "6_3 c m");
        strcpy(schoenflies, "C6v^3");
        multi = 12;
        break;
    case 186:
        strcpy(bravais_symbol, "P");
        strcpy(international, "6_3 m c");
        strcpy(schoenflies, "C6v^4");
        multi = 12;
        break;
    case 187:
        strcpy(bravais_symbol, "P");
        strcpy(international, "-6 m 2");
        strcpy(schoenflies, "D3h^1");
        multi = 12;
        break;
    case 188:
        strcpy(bravais_symbol, "P");
        strcpy(international, "-6 c 2");
        strcpy(schoenflies, "D3h^2");
        multi = 12;
        break;
    case 189:
        strcpy(bravais_symbol, "P");
        strcpy(international, "-6 2 m");
        strcpy(schoenflies, "D3h^3");
        multi = 12;
        break;
    case 190:
        strcpy(bravais_symbol, "P");
        strcpy(international, "-6 2 c");
        strcpy(schoenflies, "D3h^4");
        multi = 12;
        break;
    case 191:
        strcpy(bravais_symbol, "P");
        strcpy(international, "6/m m m");
        strcpy(schoenflies, "D6h^1");
        multi = 24;
        break;
    case 192:
/* Note that the identification of */
/*    the mirror planes is still ambiguous for cF */
        strcpy(bravais_symbol, "P");
        strcpy(international, "6/m c c");
        strcpy(schoenflies, "D6h^2");
        multi = 24;
        break;
    case 193:
        strcpy(bravais_symbol, "P");
        strcpy(international, "6_3/m c m");
        strcpy(schoenflies, "D6h^3");
        multi = 24;
        break;
    case 194:
        strcpy(bravais_symbol, "P");
        strcpy(international, "6_3/m m c");
        strcpy(schoenflies, "D6h^4");
        multi = 24;
        break;
    case 195:
        strcpy(bravais_symbol, "P");
        strcpy(international, "2 3");
        strcpy(schoenflies, "T^1");
        multi = 12;
        break;
    case 196:
        strcpy(bravais_symbol, "F");
        strcpy(international, "2 3");
        strcpy(schoenflies, "T^2");
        multi = 48;
        break;
    case 197:
        strcpy(bravais_symbol, "I");
        strcpy(international, "2 3");
        strcpy(schoenflies, "T^3");
        multi = 24;
        break;
    case 198:
        strcpy(bravais_symbol, "P");
        strcpy(international, "2_1 3");
        strcpy(schoenflies, "T^4");
        multi = 12;
        break;
    case 199:
        strcpy(bravais_symbol, "I");
        strcpy(international, "2_1 3");
        strcpy(schoenflies, "T^5");
        multi = 24;
        break;
    case 200:
        strcpy(bravais_symbol, "P");
        strcpy(international, "m -3");
        strcpy(schoenflies, "Th^1");
        multi = 24;
        break;
    case 201:
        if (origin == 1) {
            strcpy(bravais_symbol, "P");
            strcpy(international, "n -3");
            strcpy(international_long, "n -3 _1");
            strcpy(schoenflies, "Th^2");
            multi = 24;
        }
        if (origin == 2) {
            strcpy(bravais_symbol, "P");
            strcpy(international, "n -3");
            strcpy(international_long, "n -3 _2");
            strcpy(schoenflies, "Th^2");
            multi = 24;
        }
        break;
    case 202:
        strcpy(bravais_symbol, "F");
        strcpy(international, "m -3");
        strcpy(schoenflies, "Th^3");
        multi = 96;
        break;
    case 203:
        if (origin == 1) {
            strcpy(bravais_symbol, "F");
            strcpy(international, "d -3");
            strcpy(international_long, "d -3 _1");
            strcpy(schoenflies, "Th^4");
            multi = 96;
        }
        if (origin == 2) {
            strcpy(bravais_symbol, "F");
            strcpy(international, "d -3");
            strcpy(international_long, "d -3 _2");
            strcpy(schoenflies, "Th^4");
            multi = 96;
        }
        break;
    case 204:
        strcpy(bravais_symbol, "I");
        strcpy(international, "m -3");
        strcpy(schoenflies, "Th^5");
        multi = 48;
        break;
    case 205:
        strcpy(bravais_symbol, "P");
        strcpy(international, "a -3");
        strcpy(schoenflies, "Th^6");
        multi = 24;
        break;
    case 206:
        strcpy(bravais_symbol, "I");
        strcpy(international, "a -3");
        strcpy(schoenflies, "Th^7");
        multi = 48;
        break;
    case 207:
        strcpy(bravais_symbol, "P");
        strcpy(international, "4 3 2");
        strcpy(schoenflies, "O^1");
        multi = 24;
        break;
    case 208:
        strcpy(bravais_symbol, "P");
        strcpy(international, "4_2 3 2");
        strcpy(schoenflies, "O^2");
        multi = 24;
        break;
    case 209:
        strcpy(bravais_symbol, "F");
        strcpy(international, "4 3 2");
        strcpy(schoenflies, "O^3");
        multi = 96;
        break;
    case 210:
        strcpy(bravais_symbol, "F");
        strcpy(international, "4_1 3 2");
        strcpy(schoenflies, "O^4");
        multi = 96;
        break;
    case 211:
        strcpy(bravais_symbol, "I");
        strcpy(international, "4 3 2");
        strcpy(schoenflies, "O^5");
        multi = 48;
        break;
    case 212:
        strcpy(bravais_symbol, "P");
        strcpy(international, "4_3 3 2");
        strcpy(schoenflies, "O^6");
        multi = 24;
        break;
    case 213:
        strcpy(bravais_symbol, "P");
        strcpy(international, "4_1 3 2");
        strcpy(schoenflies, "O^7");
        multi = 24;
        break;
    case 214:
        strcpy(bravais_symbol, "I");
        strcpy(international, "4_1 3 2");
        strcpy(schoenflies, "O^8");
        multi = 48;
        break;
    case 215:
        strcpy(bravais_symbol, "P");
        strcpy(international, "-4 3 m");
        strcpy(schoenflies, "Td^1");
        multi = 24;
        break;
    case 216:
        strcpy(bravais_symbol, "F");
        strcpy(international, "-4 3 m");
        strcpy(schoenflies, "Td^2");
        multi = 96;
        break;
    case 217:
        strcpy(bravais_symbol, "I");
        strcpy(international, "-4 3 m");
        strcpy(schoenflies, "Td^3");
        multi = 48;
        break;
    case 218:
        strcpy(bravais_symbol, "P");
        strcpy(international, "-4 3 n");
        strcpy(schoenflies, "Td^4");
        multi = 24;
        break;
    case 219:
        strcpy(bravais_symbol, "F");
        strcpy(international, "-4 3 c");
        strcpy(schoenflies, "Td^5");
        multi = 96;
        break;
    case 220:
        strcpy(bravais_symbol, "I");
        strcpy(international, "-4 3 d");
        strcpy(schoenflies, "Td^6");
        multi = 48;
        break;
    case 221:
        strcpy(bravais_symbol, "P");
        strcpy(international, "m -3 m");
        strcpy(schoenflies, "Oh^1");
        multi = 48;
        break;
    case 222:
        if (origin == 1) {
            strcpy(bravais_symbol, "P");
            strcpy(international, "n -3 n");
            strcpy(international_long, "n -3 n _1");
            strcpy(schoenflies, "Oh^2");
            multi = 48;
        }
        if (origin == 2) {
            strcpy(bravais_symbol, "P");
            strcpy(international, "n -3 n");
            strcpy(international_long, "n -3 n _2");
            strcpy(schoenflies, "Oh^2");
            multi = 48;
        }
        break;
    case 223:
        strcpy(bravais_symbol, "P");
        strcpy(international, "m -3 n");
        strcpy(schoenflies, "Oh^3");
        multi = 48;
        break;
    case 224:
        if (origin == 1) {
            strcpy(bravais_symbol, "P");
            strcpy(international, "n -3 m");
            strcpy(schoenflies, "Oh^4");
            strcpy(international_long, "n -3 m _1");
            multi = 48;
        }
        if (origin == 2) {
            strcpy(bravais_symbol, "P");
            strcpy(international, "n -3 m");
            strcpy(schoenflies, "Oh^4");
            strcpy(international_long, "n -3 m _2");
            multi = 48;
        }
        break;
    case 225:
        strcpy(bravais_symbol, "F");
        strcpy(international, "m -3 m");
        strcpy(schoenflies, "Oh^5");
        multi = 192;
        break;
    case 226:
        strcpy(bravais_symbol, "F");
        strcpy(international, "m -3 c");
        strcpy(schoenflies, "Oh^6");
        multi = 192;
        break;
    case 227:
        if (origin == 1) {
            strcpy(bravais_symbol, "F");
            strcpy(international, "d -3 m");
            strcpy(international_long, "d -3 m _1");
            strcpy(schoenflies, "Oh^7");
            multi = 192;
        }
        if (origin == 2) {
            strcpy(bravais_symbol, "F");
            strcpy(international, "d -3 m");
            strcpy(international_long, "d -3 m _2");
            strcpy(schoenflies, "Oh^7");
            multi = 192;
        }
        break;
    case 228:
        if (origin == 1) {
            strcpy(bravais_symbol, "F");
            strcpy(international, "d -3 c");
            strcpy(international_long, "d -3 c _1");
            strcpy(schoenflies, "Oh^8");
            multi = 192;
        }
        if (origin == 2) {
            strcpy(bravais_symbol, "F");
            strcpy(international, "d -3 c");
            strcpy(international_long, "d -3 c _2");
            strcpy(schoenflies, "Oh^8");
            multi = 192;
        }
        break;
    case 229:
/*         Note that the identification of */
/*             the mirror planes is still ambiguous for cI */
        strcpy(bravais_symbol, "I");
        strcpy(international, "m -3 m");
        strcpy(schoenflies, "Oh^9");
        multi = 96;
        break;
    case 230:
        strcpy(bravais_symbol, "I");
        strcpy(international, "a -3 d");
        strcpy(schoenflies, "Oh^10");
        multi = 96;
        break;
    }

    spacegroup.number = spacegroup_number;
    strcpy(spacegroup.international, international);
    if (strcmp(international_long, ""))
        strcpy(spacegroup.international_long, international_long);
    else
        strcpy(spacegroup.international_long, international);
    strcpy(spacegroup.schoenflies, schoenflies);
    strcpy(spacegroup.bravais_symbol, bravais_symbol);
    spacegroup.multi = multi;

    return spacegroup;
}


Pointgroup tbl_get_pointgroup_database(int spacegroup_number)
{
    char point_international[10];
    char point_schoenflies[10];
    Pointgroup pointgroup;

    switch (spacegroup_number) {
    case 1:
        strcpy(point_international, "1");
        strcpy(point_schoenflies, "C1");
        break;
    case 2:
        strcpy(point_international, "-1");
        strcpy(point_schoenflies, "Ci");
        break;
    case 3:
    case 4:
    case 5:
        strcpy(point_international, "2");
        strcpy(point_schoenflies, "C2");
        break;
    case 6:
    case 7:
    case 8:
    case 9:
        strcpy(point_international, "m");
        strcpy(point_schoenflies, "Cs = C1h ");
        break;
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
        strcpy(point_international, "2/m");
        strcpy(point_schoenflies, "C2h");
        break;
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
    case 22:
    case 23:
    case 24:
        strcpy(point_international, "2 2 2");
        strcpy(point_schoenflies, "D2");
        break;
    case 25:
    case 26:
    case 27:
    case 28:
    case 29:
    case 30:
    case 31:
    case 32:
    case 33:
    case 34:
    case 35:
    case 36:
    case 37:
    case 38:
    case 39:
    case 40:
    case 41:
    case 42:
    case 43:
    case 44:
    case 45:
    case 46:
        strcpy(point_international, "m m 2");
        strcpy(point_schoenflies, "C2v");
        break;
    case 47:
    case 48:
    case 49:
    case 50:
    case 51:
    case 52:
    case 53:
    case 54:
    case 55:
    case 56:
    case 57:
    case 58:
    case 59:
    case 60:
    case 61:
    case 62:
    case 63:
    case 64:
    case 65:
    case 66:
    case 67:
    case 68:
    case 69:
    case 70:
    case 71:
    case 72:
    case 73:
    case 74:
        strcpy(point_international, "m m m");
        strcpy(point_schoenflies, "D2h");
        break;
    case 75:
    case 76:
    case 77:
    case 78:
    case 79:
    case 80:
        strcpy(point_international, "4");
        strcpy(point_schoenflies, "C4");
        break;
    case 81:
    case 82:
        strcpy(point_international, "-4");
        strcpy(point_schoenflies, "S4");
        break;
    case 83:
    case 84:
    case 85:
    case 86:
    case 87:
    case 88:
        strcpy(point_international, "4/m");
        strcpy(point_schoenflies, "C4h");
        break;
    case 89:
    case 90:
    case 91:
    case 92:
    case 93:
    case 94:
    case 95:
    case 96:
    case 97:
    case 98:
        strcpy(point_international, "4 2 2");
        strcpy(point_schoenflies, "D4");
        break;
    case 99:
    case 100:
    case 101:
    case 102:
    case 103:
    case 104:
    case 105:
    case 106:
    case 107:
    case 108:
    case 109:
    case 110:
        strcpy(point_international, "4 m m");
        strcpy(point_schoenflies, "C4v");
        break;
    case 111:
    case 112:
    case 113:
    case 114:
    case 121:
    case 122:
        strcpy(point_international, "-4 2 m");
        strcpy(point_schoenflies, "D2d^1");
        break;
    case 115:
    case 116:
    case 117:
    case 118:
    case 119:
    case 120:
        strcpy(point_international, "-4 m 2");
        strcpy(point_schoenflies, "D2h^2");
        break;
    case 123:
    case 124:
    case 125:
    case 126:
    case 127:
    case 128:
    case 129:
    case 130:
    case 131:
    case 132:
    case 133:
    case 134:
    case 135:
    case 136:
    case 137:
    case 138:
    case 139:
    case 140:
    case 141:
    case 142:
        strcpy(point_international, "4/m m m");
        strcpy(point_schoenflies, "D4h");
        break;
    case 143:
    case 144:
    case 145:
    case 146:
        strcpy(point_international, "3");
        strcpy(point_schoenflies, "C3");
        break;
    case 147:
    case 148:
        strcpy(point_international, "-3");
        strcpy(point_schoenflies, "C3i");
        break;
    case 149:
    case 151:
    case 153:
        strcpy(point_international, "3 1 2");
        strcpy(point_schoenflies, "D3^1");
        break;
    case 150:
    case 152:
    case 154:
    case 155:
        strcpy(point_international, "3 2 1");
        strcpy(point_schoenflies, "D3^2");
        break;
    case 156:
    case 158:
    case 160:
    case 161:
        strcpy(point_international, "3 m 1");
        strcpy(point_schoenflies, "C3v^1");
        break;
    case 157:
    case 159:
        strcpy(point_international, "3 1 m");
        strcpy(point_schoenflies, "C3v^2");
        break;
    case 162:
    case 163:
        strcpy(point_international, "-3 1 m");
        strcpy(point_schoenflies, "D3d^1");
        break;
    case 164:
    case 165:
    case 166:
    case 167:
        strcpy(point_international, "-3 m 1");
        strcpy(point_schoenflies, "D3d^2");
        break;
    case 168:
    case 169:
    case 170:
    case 171:
    case 172:
    case 173:
        strcpy(point_international, "6");
        strcpy(point_schoenflies, "C6");
        break;
    case 174:
        strcpy(point_international, "-6");
        strcpy(point_schoenflies, "C3h");
        break;
    case 175:
    case 176:
        strcpy(point_international, "6/m");
        strcpy(point_schoenflies, "C6h");
        break;
    case 177:
    case 178:
    case 179:
    case 180:
    case 181:
    case 182:
        strcpy(point_international, "6 2 2");
        strcpy(point_schoenflies, "D6");
        break;
    case 183:
    case 184:
    case 185:
    case 186:
        strcpy(point_international, "6 m m");
        strcpy(point_schoenflies, "C6v");
        break;
    case 187:
    case 188:
        strcpy(point_international, "-6 m 2");
        strcpy(point_schoenflies, "D3h^1");
        break;
    case 189:
    case 190:
        strcpy(point_international, "-6 2 m");
        strcpy(point_schoenflies, "D3h^2");
        break;
    case 191:
    case 192:
    case 193:
    case 194:
        strcpy(point_international, "6/m m m");
        strcpy(point_schoenflies, "D6h");
        break;
    case 195:
    case 196:
    case 197:
    case 198:
    case 199:
        strcpy(point_international, "2 3");
        strcpy(point_schoenflies, "T");
        break;
    case 200:
    case 201:
    case 202:
    case 203:
    case 204:
    case 205:
    case 206:
        strcpy(point_international, "m 3");
        strcpy(point_schoenflies, "Th");
        break;
    case 207:
    case 208:
    case 209:
    case 210:
    case 211:
    case 212:
    case 213:
    case 214:
        strcpy(point_international, "4 3 2");
        strcpy(point_schoenflies, "O");
        break;
    case 215:
    case 216:
    case 217:
    case 218:
    case 219:
    case 220:
        strcpy(point_international, "4 3 m");
        strcpy(point_schoenflies, "Td");
        break;
    case 221:
    case 222:
    case 223:
    case 224:
    case 225:
    case 226:
    case 227:
    case 228:
    case 229:
    case 230:
        strcpy(point_international, "m -3 m");
        strcpy(point_schoenflies, "Oh");
        break;
    }

    strcpy(pointgroup.international, point_international);
    strcpy(pointgroup.schoenflies, point_schoenflies);

    return pointgroup;
}
