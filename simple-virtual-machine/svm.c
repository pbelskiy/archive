/*
    Code: 2011
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#define SIZE_OF_ARRAY(x) sizeof(x) / sizeof(*x)

unsigned char *pcode;
unsigned int pcode_len;
char name[256];

enum VM_Commands {
    MOV_REG_NUM = 0x00,
    MOV_REG_REG = 0x01,
    ADD_REG_NUM = 0x10,
    ADD_REG_REG = 0x11,
    SUB_REG_NUM = 0x20,
    SUB_REG_REG = 0x21,
    XOR_REG_NUM = 0x30,
    XOR_REG_REG = 0x31
};

enum VM_Operand {
    REG_1 = 0x00,
    REG_2 = 0x01,
    REG_3 = 0x02,
    REG_4 = 0x03
};

void generate_pcode(void)
{
    unsigned char *OutCode = pcode;
    unsigned char Opcode;
    unsigned int i, j, SR1, SR2, Pos = 0;

    // MOV R2, rand
    *(uint32_t *) &OutCode[Pos + 0] = 0x0100;
    *(uint32_t *) &OutCode[Pos + 2] = pow(rand(),rand() % 5) + rand();
    Pos += 6;

    // MOV R3, rand
    *(uint32_t *) &OutCode[Pos + 0] = 0x0200;
    *(uint32_t *) &OutCode[Pos + 2] = pow(rand(),rand() % 5) + rand();
    Pos += 6;

    // MOV R4, rand
    *(uint32_t *) &OutCode[Pos + 0] = 0x0300;
    *(uint32_t *) &OutCode[Pos + 2] = pow(rand(),rand() % 5) + rand();
    Pos += 6;

    for(i=0 ; i < 100 ; i++) {
        Opcode = (1 + (rand() % 3)) * 0x10;
        if(rand() % 2 == 0) {
            Opcode += 0; // number
            OutCode[Pos + 0] = Opcode; // command
            OutCode[Pos + 1] = (rand() % 3) + 1; // reg
            *(uint32_t *) &OutCode[Pos + 2] = pow(rand(),rand() % 5) + rand();
            Pos += 6;
        } else {
            Opcode += 1; // register
            OutCode[Pos + 0] = Opcode; // command
            OutCode[Pos + 1] = (rand() % 3) + 1; // reg
            OutCode[Pos + 2] = (rand() % 4); // reg
            Pos += 3;
        }
    }

    *(uint32_t *) &OutCode[Pos] = 0x010031; Pos += 3; // XOR R1, R2
    *(uint32_t *) &OutCode[Pos] = 0x020031; Pos += 3; // XOR R1, R3
    *(uint32_t *) &OutCode[Pos] = 0x030031; Pos += 3; // XOR R1, R4

    pcode_len = Pos;
    printf("pcode_len = %d\n", pcode_len);
}

unsigned int run_pcode(unsigned int InputKey)
{
    unsigned int EIP = 0;
    unsigned int RNumber;
    unsigned int R1, R2, R3, R4;
    int Result = 0;

    R1 = InputKey;

    while (EIP < pcode_len) {
        switch (pcode[EIP]) {
         case MOV_REG_NUM:
            RNumber = *(uint32_t *) &pcode[EIP + 2];
            switch (pcode[EIP + 1]) {
                case REG_1: R1 = RNumber; break;
                case REG_2: R2 = RNumber; break;
                case REG_3: R3 = RNumber; break;
                case REG_4: R4 = RNumber; break;
            }

            EIP += 6;
            break;

        case MOV_REG_REG:
            switch (pcode[EIP + 1]) {
                case REG_1:
                    switch (pcode[EIP + 2]) {
                        case REG_1: R1 = R1; break;
                        case REG_2: R1 = R2; break;
                        case REG_3: R1 = R3; break;
                        case REG_4: R1 = R4; break;
                    }
                    break;

                case REG_2:
                    switch (pcode[EIP + 2]) {
                        case REG_1: R2 = R1; break;
                        case REG_2: R2 = R2; break;
                        case REG_3: R2 = R3; break;
                        case REG_4: R2 = R4; break;
                    }
                    break;

                case REG_3:
                    switch (pcode[EIP + 2]) {
                        case REG_1: R3 = R1; break;
                        case REG_2: R3 = R2; break;
                        case REG_3: R3 = R3; break;
                        case REG_4: R3 = R4; break;
                    }
                    break;

                case REG_4:
                    switch (pcode[EIP + 2]) {
                        case REG_1: R4 = R1; break;
                        case REG_2: R4 = R2; break;
                        case REG_3: R4 = R3; break;
                        case REG_4: R4 = R4; break;
                    }
                    break;
            }

            EIP += 3;
            break;

        case ADD_REG_NUM:
            RNumber = *(uint32_t *) &pcode[EIP + 2];
            switch (pcode[EIP + 1]) {
                case REG_1: R1 += RNumber; break;
                case REG_2: R2 += RNumber; break;
                case REG_3: R3 += RNumber; break;
                case REG_4: R4 += RNumber; break;
            }

            EIP += 6;
            break;

        case ADD_REG_REG:
            switch (pcode[EIP + 1]) {
                case REG_1:
                    switch (pcode[EIP + 2]) {
                        case REG_1: R1 += R1; break;
                        case REG_2: R1 += R2; break;
                        case REG_3: R1 += R3; break;
                        case REG_4: R1 += R4; break;
                    }
                    break;

                case REG_2:
                    switch (pcode[EIP + 2]) {
                        case REG_1: R2 += R1; break;
                        case REG_2: R2 += R2; break;
                        case REG_3: R2 += R3; break;
                        case REG_4: R2 += R4; break;
                    }
                    break;

                case REG_3:
                    switch (pcode[EIP + 2]) {
                        case REG_1: R3 += R1; break;
                        case REG_2: R3 += R2; break;
                        case REG_3: R3 += R3; break;
                        case REG_4: R3 += R4; break;
                    }
                    break;

                case REG_4:
                    switch (pcode[EIP + 2]) {
                        case REG_1: R4 += R1; break;
                        case REG_2: R4 += R2; break;
                        case REG_3: R4 += R3; break;
                        case REG_4: R4 += R4; break;
                    }
                    break;
            }

            EIP += 3;
            break;

        case SUB_REG_NUM:
            RNumber = *(uint32_t *) &pcode[EIP + 2];
            switch (pcode[EIP + 1]) {
                case REG_1: R1 -= RNumber; break;
                case REG_2: R2 -= RNumber; break;
                case REG_3: R3 -= RNumber; break;
                case REG_4: R4 -= RNumber; break;
            }

            EIP += 6;
            break;

        case SUB_REG_REG:
            switch (pcode[EIP + 1]) {
                case REG_1:
                    switch (pcode[EIP + 2]) {
                        case REG_1: R1 -= R1; break;
                        case REG_2: R1 -= R2; break;
                        case REG_3: R1 -= R3; break;
                        case REG_4: R1 -= R4; break;
                    }
                    break;

                case REG_2:
                    switch (pcode[EIP + 2]) {
                        case REG_1: R2 -= R1; break;
                        case REG_2: R2 -= R2; break;
                        case REG_3: R2 -= R3; break;
                        case REG_4: R2 -= R4; break;
                    }
                    break;

                case REG_3:
                    switch (pcode[EIP + 2]) {
                        case REG_1: R3 -= R1; break;
                        case REG_2: R3 -= R2; break;
                        case REG_3: R3 -= R3; break;
                        case REG_4: R3 -= R4; break;
                    }
                    break;

                case REG_4:
                    switch (pcode[EIP + 2]) {
                        case REG_1: R4 -= R1; break;
                        case REG_2: R4 -= R2; break;
                        case REG_3: R4 -= R3; break;
                        case REG_4: R4 -= R4; break;
                    }
                    break;
            }

            EIP += 3;
            break;

        case XOR_REG_NUM:
            RNumber = *(uint32_t *) &pcode[EIP + 2];
            switch (pcode[EIP + 1]) {
                case REG_1: R1 ^= RNumber; break;
                case REG_2: R2 ^= RNumber; break;
                case REG_3: R3 ^= RNumber; break;
                case REG_4: R4 ^= RNumber; break;
            }

            EIP += 6;
            break;

        case XOR_REG_REG:
            switch (pcode[EIP + 1]) {
                case REG_1:
                    switch (pcode[EIP + 2]) {
                        case REG_1: R1 ^= R1; break;
                        case REG_2: R1 ^= R2; break;
                        case REG_3: R1 ^= R3; break;
                        case REG_4: R1 ^= R4; break;
                    }
                    break;

                case REG_2:
                    switch (pcode[EIP + 2]) {
                        case REG_1: R2 ^= R1; break;
                        case REG_2: R2 ^= R2; break;
                        case REG_3: R2 ^= R3; break;
                        case REG_4: R2 ^= R4; break;
                    }
                    break;

                case REG_3:
                    switch (pcode[EIP + 2]) {
                        case REG_1: R3 ^= R1; break;
                        case REG_2: R3 ^= R2; break;
                        case REG_3: R3 ^= R3; break;
                        case REG_4: R3 ^= R4; break;
                    }
                    break;

                case REG_4:
                    switch (pcode[EIP + 2]) {
                        case REG_1: R4 ^= R1; break;
                        case REG_2: R4 ^= R2; break;
                        case REG_3: R4 ^= R3; break;
                        case REG_4: R4 ^= R4; break;
                    }
                    break;
            }

            EIP += 3;
            break;
        }
    }

    return R1;
}

unsigned int crc32(unsigned char *buf, unsigned int len)
{
    unsigned int crc_table[256];
    unsigned int crc;
    int i, j;

    for (i = 0; i < 256; i++) {
        crc = i;
        for (j = 0; j < 8; j++)
            crc = crc & 1 ? (crc >> 1) ^ 0xEDB88320UL : crc >> 1;
            crc_table[i] = crc;
    };

    crc = 0xFFFFFFFFUL;
    while (len--)
        crc = crc_table[(crc ^ *buf++) & 0xFF] ^ (crc >> 8);

    return crc ^ 0xFFFFFFFFUL;
};

int main()
{
    unsigned int password;
    int result;

    // Correct pair: th3mis 0xA28027FF

    printf("Enter name: ");
    scanf("%s", name);
    printf("Enter password: ");
    scanf("%x", &password);

    pcode = (unsigned char*) malloc(0x1000);
    srand(0);
    // srand(time(NULL));
    generate_pcode();

    result = run_pcode(crc32(name, strlen(name)));
    if (result == password) {
        printf("Password correct!\n");
    } else {
        printf("Password NOT correct! (correct is 0x%X)\n", result);
    }

    free(pcode);
    return 0;
}
