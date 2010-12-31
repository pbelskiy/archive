# About

Very simple engine for parsing x86 code to internal representation (IR), making
some preprocessing and then build binary code from graph.

Originally was made on 2012 year.

Usage:
```
1) cgp_init(...);
2) operations with IR
3) cgp_build*
4) cgp_free(...);
```

Please read functions description in cgp.h file.

### Example of input.s processing

Before preprocesssing:
```assembly
    00401000    XOR EDI,EDI
    00401002    DEC EDI
    00401004    CALL 0040100A
    00401009    RETN
    0040100A    MOV EAX,EDI
    0040100C    MOV ECX,0
    00401011    XOR ECX,EAX
    00401013    DEC EAX
    00401015    CMP EAX,0
    00401018    JNE 00401011
    0040101E    RETN
```

After spaggeti preprocessing:
```assembly
    00401000    JMP 0040101E
    00401005    CMP EAX,0
    00401008    JMP 00401045
    0040100D    MOV ECX,0
    00401012    JMP 00401017
    00401017    XOR ECX,EAX
    00401019    JMP 00401026
    0040101E    XOR EDI,EDI
    00401020    JMP 0040103E
    00401025    RETN
    00401026    DEC EAX
    00401028    JMP 00401005
    0040102D    MOV EAX,EDI
    0040102F    JMP 0040100D
    00401034    CALL 0040102D
    00401039    JMP 00401025
    0040103E    DEC EDI
    00401040    JMP 00401034
    00401045    JNE 00401017
    0040104B    JMP 00401050
    00401050    RETN
```
