#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include "cgp.h"

void input_routine_begin();
void input_routine_end();

void *get_code_from_s_file(uint32_t *size)
{
    void *code_pointer = input_routine_begin;
    uint32_t code_size = input_routine_end - input_routine_begin;

    printf("input code = %p, input size = %X\n", code_pointer, code_size);

    *size = code_size;
    return code_pointer;
}

int main(int argc, char *argv[])
{
    FILE *fh;
    uint8_t *input_code, *output_code;
    uint32_t input_size, output_size, entry_point;

    entry_point = 0;
    input_code = get_code_from_s_file(&input_size);

    cgp_init(input_code, input_size, entry_point);
    export_to_gdl("graph_in.gdl");

    /* few variants of using */
    // output_size = cgp_build(&output_code);
    // output_size = cgp_build_reduntant_nop(&output_code);
    output_size = cgp_build_spaghetti(&output_code);

    export_to_gdl("graph_out.gdl");
    cgp_free();

    printf("out code size = %d bytes\n", output_size);

    fh = fopen("out.bin", "w");
    if(!fh) {
        printf("Can't open 'out.bin'\n");
        return 1;
    }

    fseek(fh, 0, SEEK_SET);
    fwrite(output_code, 1, output_size, fh);
    free(output_code);
    return 0;
}
