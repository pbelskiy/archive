#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include "cgp.h"

int GetInstructionSize(uint8_t *pOpCode, uint32_t *pdwinstruction_size);

#define OPCODE_X86_JMP_REL8     0xEB
#define OPCODE_X86_JMP_REL32    0xE9
#define OPCODE_X86_CALL         0xE8
#define OPCODE_X86_RET          0xC3
#define OPCODE_X86_NOP          0x90

#define INVALID_OFFSET          0xFFFFFFFF
#define INVALID_VALUE           0xFFFFFFFF

#define BRANCH_STACK_LIMIT      0x1000
#define CODE_BUFFER_LIMIT       0x10000

enum insert_types {
    INSERT_BEFORE,
    INSERT_AFTER
};

enum node_types {
    NODE_LINE,
    NODE_JMP,
    NODE_JCC,
    NODE_CALL,
    NODE_RET,
    NODE_LABEL              /* abstract node */
};

typedef struct node {
    uint32_t type;
    uint8_t *data;
    uint32_t weight;
    uint32_t offset;        /* absolute offset of data */
    struct node *BLink;     /* backward */
    struct node *FLink;     /* forward */
    struct node *CLink;     /* condition */
} Node, *pNode;

static uint32_t nodes_count;
static pNode *nodes = NULL, first_node;

static uint32_t rand_seed;

static uint32_t jcc_count = 0;
static pNode jcc_stack[BRANCH_STACK_LIMIT];

static uint32_t call_count = 0;
static pNode call_stack[BRANCH_STACK_LIMIT];


static uint32_t cgp_random(uint32_t seed)
{
    rand_seed = 134775812 * rand_seed + 1;
    return (uint32_t) rand_seed * (long long) seed >> 32;
}

static void cgp_randomize(void)
{
    rand_seed = (uint32_t) time(NULL);
}

void export_to_gdl(const char *file_name)
{
    char out_path[1024];
    FILE *fh;
    uint32_t i;

    strcpy(&out_path[0], &file_name[0]);
    remove(out_path);

    fh = fopen(out_path, "w+");
    if (!fh) {
        printf("[CGP] error: can`t open %s\n", out_path);
        return;
    }

    printf("[CGP] export graph to file: %s\n", out_path);

    /* graph header begin */
    fprintf(fh,
            "graph: {\n"
            "manhattan_edges: yes\n"
            "layoutalgorithm: mindepth\n"
            "finetuning: no\n"
            "layout_downfactor: 100\n"
            "layout_upfactor: 0\n"
            "layout_nearfactor: 0\n\n");

    for (i = 0 ; i < nodes_count ; i++) {
        if (!nodes[i])
            continue;

        fprintf(fh,
                "node: {title: \"%p\" "
                       "label: \"Offset: 0x%04X "
                       "(size: %X) ",
                nodes[i],
                nodes[i]->offset,
                nodes[i]->weight);

        switch (nodes[i]->type) {
            case NODE_LINE:
                fprintf(fh, "LINEAR CODE\"}\n");
                break;

            case NODE_JMP:
                fprintf(fh, "JMP\"}\n");
                break;

            case NODE_RET:
                fprintf(fh, "RET\"}\n");
                break;

            case NODE_CALL:
                fprintf(fh, "CALL\"}\n");
                break;

            case NODE_JCC:
                fprintf(fh, "JCC\"}\n");
                break;
        }
    }

    fprintf(fh, "\n");

    for (i = 0 ; i < nodes_count ; i++) {
        if (!nodes[i])
            continue;

        switch (nodes[i]->type) {
            case NODE_LINE:
            case NODE_JMP:
            case NODE_RET:
                if (nodes[i]->FLink) {
                    fprintf(fh,
                            "edge: {sourcename: \"%p\" "
                                   "targetname: \"%p\"}\n",
                            nodes[i],
                            nodes[i]->FLink);
                }
                break;

            case NODE_JCC:
                if (nodes[i]->FLink)
                fprintf(fh,
                        "edge: {sourcename: \"%p\" "
                               "targetname: \"%p\" "
                               "label: \"false\" "
                               "color: red}\n",
                        nodes[i],
                        nodes[i]->FLink);

                if (nodes[i]->CLink) {
                    fprintf(fh,
                            "edge: {sourcename: \"%p\" "
                                   "targetname: \"%p\" "
                                   "label: \"true\" "
                                   "color: darkgreen}\n",
                            nodes[i],
                            nodes[i]->CLink);
                } else {
                    printf("[CGP] error: graph export haven't CLink (JCC)\n");
                    exit(1);
                }
                break;

            case NODE_CALL:
                if (nodes[i]->FLink)
                fprintf(fh,
                        "edge: {sourcename: \"%p\" targetname: \"%p\"}\n",
                        nodes[i],
                        nodes[i]->FLink);

                if (nodes[i]->CLink != NULL) {
                    fprintf(fh,
                            "edge: {sourcename: \"%p\" "
                                   "targetname: \"%p\" "
                                   "label: \"call\" "
                                   "color: blue}\n",
                            nodes[i],
                            nodes[i]->CLink);
                } else {
                    printf("[CGP] error: graph export haven't CLink (CALL)\n");
                    getchar();
                }
                break;
        }
    }

    /* graph header end */
    fprintf(fh, "}\n");
    fclose(fh);
}

static void shuffle_array(void *obj, size_t nmemb, size_t size)
{
    void *temp = malloc(size);
    size_t n = nmemb;

    while (n > 1) {
        size_t k = cgp_random(n--);
        memcpy(temp, (uint8_t*) (obj) + n * size, size);
        memcpy((uint8_t*)(obj) + n * size, (uint8_t*)(obj) + k * size, size);
        memcpy((uint8_t*)(obj) + k * size, temp, size);
    }

    free(temp);
}

static pNode cgp_allocate_node(void)
{
    nodes = (pNode*) realloc(nodes, sizeof(pNode) * (nodes_count + 1));

    nodes[nodes_count] = (pNode) calloc(1, sizeof(Node));
    nodes_count++;

    if (first_node == NULL)
        first_node = nodes[nodes_count - 1];

    return nodes[nodes_count - 1];
}

static void cgp_remove_node(pNode in_node)
{
    for (uint32_t i = 0 ; i < nodes_count ; i++) {
        if (nodes[i] != in_node)
            continue;

        free(nodes[i]->data);
        free(nodes[i]);
        nodes[i] = NULL;
    }
}

static pNode cgp_merge_nodes(pNode first, pNode second)
{
    first->FLink = second->FLink;
    first->weight += second->weight;
    return first;
}

static void cgp_except_node(pNode in_node)
{
    if (!in_node->BLink) {
        cgp_remove_node(in_node);
        return;
    }

    in_node->BLink->FLink = in_node->FLink;

    for (uint32_t i = 0 ; i < nodes_count ; i++) {
        if (!nodes[i])
            continue;

        if (nodes[i]->BLink == in_node)
            nodes[i]->BLink = in_node->BLink;
    }

    cgp_remove_node(in_node);
}

static pNode cgp_insert_node(pNode in_node, uint32_t type)
{
    pNode temp, new_node = cgp_allocate_node();

    if (type == INSERT_AFTER) {
        temp = in_node->FLink;
        in_node->FLink = new_node;
        new_node->BLink = in_node;

        if (temp) temp->BLink = new_node;

        new_node->FLink = temp;
    } else {
        if (in_node == first_node)
            first_node = new_node;

        temp = in_node->BLink;
        in_node->BLink = new_node;
        new_node->FLink = in_node;

        if (temp) temp->FLink = new_node;

        new_node->BLink = temp;
    }

    return new_node;
}

static pNode cgp_find_by_offset(uint32_t absolute_offset)
{
    for (uint32_t i = 0 ; i < nodes_count ; i++) {
        if (nodes[i] && nodes[i]->offset == absolute_offset)
            return nodes[i];
    }

    return NULL;
}

static int cgp_get_node_type(uint8_t *buff)
{
    /* long JCC */
    if (*buff == 0x0F && *(buff + 1) >= 0x80 && *(buff + 1) <= 0x8F)
        return NODE_JCC;

    /* short JCC */
    if (*buff >= 0x70 && *buff <= 0x7F)
        return NODE_JCC;

    if (*buff >= 0xE0 && *buff <= 0xE3) {
        printf("[CGP] warning: LOOPD opcodes aren't supported now\n");
        return NODE_JCC; // LOOP, etc (all short jmp)
    }

    if (*buff == OPCODE_X86_JMP_REL8 || *buff == OPCODE_X86_JMP_REL32)
        return NODE_JMP;

    if (*buff == OPCODE_X86_RET)
        return NODE_RET;

    if (*buff == OPCODE_X86_CALL)
        return NODE_CALL;

    return NODE_LINE;
}

static uint32_t cgp_get_branch_offset(uint8_t *buff, uint32_t code_offset)
{
    uint32_t r_offset, abs_offset;

    /* long jcc */
    if (buff[code_offset] == 0x0F &&
        buff[code_offset + 1] >= 0x80 &&
        buff[code_offset + 1] <= 0x8F)
    {
        r_offset = *(uint32_t*) &buff[code_offset + 2];
        if ((int) r_offset >= 0) {
            abs_offset = code_offset + (r_offset + 6);
        } else {
            abs_offset = code_offset - (~r_offset - 5);
        }

        return abs_offset;
    }

    /* short jcc */
    if (buff[code_offset] >= 0x70 && buff[code_offset] <= 0x7F) {
        r_offset = buff[code_offset + 1];
        if ((char) r_offset >= 0) {
            abs_offset = code_offset + (r_offset + 2);
        } else {
            abs_offset = code_offset - (~r_offset ^ 0xFFFFFF00) + 1;
        }

        return abs_offset;
    }

    if (buff[code_offset] == OPCODE_X86_JMP_REL8) {
        r_offset = buff[code_offset + 1];
        if ((char) r_offset >= 0) {
            abs_offset = code_offset + (r_offset + 2);
        } else {
            abs_offset = code_offset - (~r_offset ^ 0xFFFFFF00) + 1;
        }

        return abs_offset;
    }

    if (buff[code_offset] == OPCODE_X86_JMP_REL32) {
        r_offset = *(uint32_t*) &buff[code_offset + 1];
        if ((int) r_offset >= 0) {
            abs_offset = code_offset + (r_offset + 5);
        } else {
            abs_offset = code_offset - (~r_offset - 4);
        }

        return abs_offset;
    }

    if (buff[code_offset] == OPCODE_X86_CALL) {
        r_offset = *(uint32_t*) &buff[code_offset + 1];

        if ((int) r_offset >= 0) {
            abs_offset = code_offset + (r_offset + 5);
        } else {
            abs_offset = code_offset - (~r_offset - 4);
        }

        return abs_offset;
    }

    printf("[CGP] error: cgp_get_branch_offset -> invalid node type\n");
    exit(0);

    return 0;
}

uint32_t cgp_push_call(pNode owner)
{
    assert(call_count < BRANCH_STACK_LIMIT);

    call_stack[call_count] = owner;
    return ++call_count;
}

pNode cgp_pop_call(void)
{
    return call_stack[--call_count];
}

uint32_t cgp_push_jcc(pNode owner)
{
    assert(jcc_count < BRANCH_STACK_LIMIT);

    jcc_stack[jcc_count] = owner;
    return ++jcc_count;
}

pNode cgp_pop_jcc(void)
{
    assert(jcc_count >= 0);

    return jcc_stack[--jcc_count];
}

pNode cgp_link_nodes(pNode first, pNode second)
{
    pNode current_node;

    current_node = cgp_allocate_node();
    current_node->type = NODE_JMP;
    current_node->weight = 5;
    current_node->data = (uint8_t*) malloc(current_node->weight);
    current_node->data[0] = OPCODE_X86_JMP_REL32;
    current_node->data[1] = 0xCC;
    current_node->data[2] = 0xCC;
    current_node->data[3] = 0xCC;
    current_node->data[4] = 0xCC;
    current_node->BLink = first;
    current_node->FLink = second;
    first->FLink = current_node;

    return current_node;
}

void cgp_long2short(pNode in_node)
{
    if (in_node->data[0] == OPCODE_X86_JMP_REL32) {
        in_node->data[0] = OPCODE_X86_JMP_REL8;
        in_node->data[1] = 0xCC;
        in_node->weight = 2;
        return;
    }

    if (in_node->data[0] == 0x0F && in_node->data[1] >= 0x70 &&
        in_node->data[1] <= 0x7F)
    {
        in_node->data[0] = in_node->data[1] - 0x10;
        in_node->data[1] = 0xCC;
        in_node->weight = 2;
        return;
    }
}

void cgp_short2long(pNode in_node)
{
    if (in_node->data[0] == OPCODE_X86_JMP_REL8) {
        in_node->data[0] = OPCODE_X86_JMP_REL32;
        *(uint32_t*) &in_node->data[1] = 0xCCCCCCCC;
        in_node->weight = 5;
        return;
    }

    if (in_node->data[0] >= 0x70 && in_node->data[0] <= 0x7F) {
        in_node->data[1] = in_node->data[0] + 0x10;
        in_node->data[0] = 0x0F;
        *(uint32_t*) &in_node->data[2] = 0xCCCCCCCC;
        in_node->weight = 6;
        return;
    }
}

int cgp_write_offset(pNode in_node, uint8_t *buff)
{
    pNode temp = NULL;

    if (!in_node || in_node->offset == INVALID_OFFSET)
        return 0;

    if (buff[in_node->offset] == OPCODE_X86_JMP_REL32) {
        if (!in_node->FLink || in_node->FLink->offset == INVALID_OFFSET)
            return 0;

        if (in_node->CLink) {
            temp = in_node->FLink;
            in_node->FLink = in_node->CLink;
        }

        if (in_node->offset > in_node->FLink->offset) {
            *(uint32_t*) &buff[in_node->offset + 1] =
                                ~(in_node->offset - in_node->FLink->offset) - 4;
        } else {
            *(uint32_t*) &buff[in_node->offset + 1] =
                                   in_node->FLink->offset - in_node->offset - 5;
        }

        if (temp != NULL) {
            in_node->CLink = in_node->FLink;
            in_node->FLink = temp;
        }

        return 1;
    }

    /* short jmp */
    if (buff[in_node->offset] == OPCODE_X86_JMP_REL8) {
        if (!in_node->FLink || in_node->FLink->offset == INVALID_OFFSET)
            return 0;

        if (in_node->CLink) {
            temp = in_node->FLink;
            in_node->FLink = in_node->CLink;
        }

        if (in_node->offset > in_node->FLink->offset) {
            *(uint8_t*) &buff[in_node->offset + 1] =
                                ~(in_node->offset - in_node->FLink->offset) - 1;
        } else {
            *(uint8_t*) &buff[in_node->offset + 1] =
                                   in_node->FLink->offset - in_node->offset - 2;
        }

        if (temp != NULL) {
            in_node->CLink = in_node->FLink;
            in_node->FLink = temp;
        }

        return 1;
    }

    /* long jcc */
    if (buff[in_node->offset] == 0x0F &&
        buff[in_node->offset+1] >= 0x80 &&
        buff[in_node->offset+1] <= 0x8F)
    {
        if (!in_node->CLink || in_node->CLink->offset == INVALID_OFFSET)
            return 0;

        if (in_node->offset > in_node->CLink->offset) {
            *(uint32_t*) &buff[in_node->offset + 2] =
                                ~(in_node->offset - in_node->CLink->offset) - 5;
        } else {
            *(uint32_t*) &buff[in_node->offset + 2] =
                                   in_node->CLink->offset - in_node->offset - 6;
        }

        return 1;
    }

    /* short jcc */
    if (buff[in_node->offset] >= 0x70 && buff[in_node->offset] <= 0x7F) {
        if (!in_node->CLink || in_node->CLink->offset == INVALID_OFFSET)
            return 0;

        if (in_node->offset > in_node->CLink->offset) {
            *(uint8_t*) &buff[in_node->offset + 1] =
                                ~(in_node->offset - in_node->CLink->offset) - 1;
        } else {
            *(uint8_t*) &buff[in_node->offset + 1] =
                                   in_node->CLink->offset - in_node->offset - 2;
        }

        return 1;
    }

    if (buff[in_node->offset] == OPCODE_X86_CALL) {
        if (!in_node->CLink || in_node->CLink->offset == INVALID_OFFSET)
            return 0;

        if (in_node->offset > in_node->CLink->offset) {
            *(uint32_t*) &buff[in_node->offset + 1] =
                                ~(in_node->offset - in_node->CLink->offset) - 4;
        } else {
            *(uint32_t*) &buff[in_node->offset + 1] =
                                   in_node->CLink->offset - in_node->offset - 5;
        }

        return 1;
    }

    return 0;
}

void cgp_parse(uint8_t *buff, uint32_t buff_size, uint32_t entry_point)
{
    uint32_t i, instruction_size, abs_offset, offset = entry_point;
    pNode found_node, stack_top, current_node = NULL, last_node = NULL;

    first_node = NULL;

    while (1) {
        /* out of range OR node has been analyzed */
        if (offset >= buff_size || (found_node = cgp_find_by_offset(offset))) {

            /* branch was analysed */
            if (offset < buff_size && last_node && found_node) {
                if (last_node->type == NODE_LINE) {
                    /* link by Jmp */
                    last_node = cgp_link_nodes(last_node, found_node);
                }

                if (last_node->type == NODE_JCC) {
                    if (!last_node->FLink) {
                        last_node->FLink = found_node;
                    } else {
                        last_node->CLink = found_node;
                    }
                }

                if (last_node->type == NODE_CALL) {
                    if (!last_node->FLink) {
                        last_node->FLink = found_node;
                    } else {
                        last_node->CLink = found_node;
                    }
                }
            }

            /* branch are out of analyse scope */
            if (offset > buff_size && last_node) {
                if (last_node->type == NODE_LINE) {
                    current_node = cgp_allocate_node();
                    current_node->type = NODE_LABEL;
                    last_node->offset = INVALID_OFFSET;
                    last_node->FLink = current_node;
                }

                if (last_node->type == NODE_JCC) {
                    current_node = cgp_allocate_node();
                    current_node->type = NODE_LABEL;

                    if (!last_node->FLink) {
                        last_node->FLink = current_node;
                    } else {
                        last_node->CLink = current_node;
                    }
                }

                if (last_node->type == NODE_CALL) {
                    printf("[CGP] error: undefined\n");
                    exit(1);
                }
            }

            /* have not processed branch */
            if (jcc_count != 0) {
                stack_top = cgp_pop_jcc();
                offset = cgp_get_branch_offset(buff, stack_top->offset);
                last_node = stack_top;
                continue;
            }

            if (call_count != 0) {
                stack_top = cgp_pop_call();
                offset = cgp_get_branch_offset(buff, stack_top->offset);
                last_node = stack_top;
                continue;
            }

            /* all done */
            break;
        }

        if (GetInstructionSize(&buff[offset], &instruction_size) != 1) {
            printf("[CGP] error: lde error!\n");
            exit(1);
        }

        /* do not process jmp */
        if (cgp_get_node_type(&buff[offset]) == NODE_JMP) {
            offset = cgp_get_branch_offset(buff, offset);
            continue;
        }

        /* allocate new node */
        current_node = cgp_allocate_node();
        current_node->type = cgp_get_node_type(&buff[offset]);

        /* reserve memory for long jcc */
        if (current_node->type == NODE_JMP || current_node->type == NODE_JCC) {
            current_node->data = (uint8_t*) malloc(10);
        } else {
            current_node->data = (uint8_t*) malloc(instruction_size);
        }

        current_node->weight = instruction_size;
        current_node->offset = offset;
        current_node->BLink  = last_node;

        /* configurate previous node */
        if (last_node) {
            switch (last_node->type) {
                case NODE_LINE:
                    last_node->FLink = current_node;
                    break;

                case NODE_JMP:
                    last_node->FLink = current_node;
                    break;

                case NODE_JCC:
                    if (!last_node->FLink) {
                        last_node->FLink = current_node;
                    } else {
                        last_node->CLink = current_node;
                    }
                    break;

                case NODE_CALL:
                    if (!last_node->FLink) {
                        last_node->FLink = current_node;
                    } else {
                        last_node->CLink = current_node;
                    }
                    break;

                case NODE_RET:
                    last_node->FLink = current_node;
                    break;
            }
        }

        last_node = current_node;

        memcpy(&current_node->data[0], &buff[offset], instruction_size);

        /* convert JMP or JCC to LONG type */
        if (current_node->type == NODE_JMP ||
            current_node->type == NODE_JCC ||
            current_node->type == NODE_CALL)
        {
            cgp_short2long(current_node);

            if (current_node->type == NODE_JMP ||
                current_node->type == NODE_CALL) {
                *(uint32_t*) &current_node->data[1] = 0xCCCCCCCC;
            } else {
                *(uint32_t*) &current_node->data[2] = 0xCCCCCCCC;
            }
        }

        /* line code */
        if (current_node->type == NODE_LINE) {
            offset += instruction_size;
            continue;
        }

        /* uncondition branch */
        // if (current_node->type == NODE_JMP)

        /* condition branch */
        if (current_node->type == NODE_JCC) {

            abs_offset = cgp_get_branch_offset(buff, offset);
            found_node = cgp_find_by_offset(abs_offset);

            if (found_node) {
                current_node->CLink = found_node;
                offset += instruction_size;
            } else {
                /* push jcc absolute branch address */
                cgp_push_jcc(current_node);

                /* and process next instruction */
                offset += instruction_size;
            }

            continue;
        }

        /* call */
        if (current_node->type == NODE_CALL) {

            abs_offset = cgp_get_branch_offset(buff, offset);

            /* condition and next address are same */

            found_node = cgp_find_by_offset(abs_offset);

            if (found_node) {
                current_node->CLink = found_node;
                offset += instruction_size;
            } else {
                /* push address of next instruction */
                cgp_push_call(current_node);

                /* and process call routine */
                offset += instruction_size;
            }

            continue;
        }

        /* return */
        if (current_node->type == NODE_RET) {
            if (call_count == 0)
                continue;

            stack_top = cgp_pop_call();
            offset = cgp_get_branch_offset(buff, stack_top->offset);
            last_node = cgp_find_by_offset(stack_top->offset); // callback addr
            continue;
        }
    }

    for (i = 0 ; i < nodes_count ; i++) {
        if (nodes[i]->type != NODE_CALL)
            continue;

        if (!nodes[i]->FLink)
            printf("%X CALL have't FLink\n", nodes[i]->offset);

        if (!nodes[i]->CLink)
            printf("%X CALL have't CLink\n", nodes[i]->offset);
    }
}

uint32_t cgp_build(uint8_t **out_buff)
{
    uint32_t offset = 0;
    uint8_t *buff;
    pNode new_node, curr_node;

    /* allocate buffer */
    buff = (uint8_t *) calloc(sizeof(uint8_t), CODE_BUFFER_LIMIT);

    call_count = 0;
    jcc_count = 0;

    for (uint32_t i = 0 ; i < nodes_count ; i++) {
        if (nodes[i])
            nodes[i]->offset = INVALID_OFFSET;
    }

    curr_node = first_node;

    /* write instructions */
    while (curr_node) {
        assert(offset < CODE_BUFFER_LIMIT);

        if (curr_node->offset == INVALID_OFFSET) {

            if (curr_node->type == NODE_LABEL) {
                curr_node->offset = offset;
            } else {
                switch (curr_node->type) {
                    case NODE_CALL:
                        if (curr_node->CLink->offset == INVALID_OFFSET)
                            cgp_push_call(curr_node);
                        break;

                    case NODE_JCC:
                        if (curr_node->CLink->offset == INVALID_OFFSET)
                            cgp_push_jcc(curr_node);
                        break;
                }

                /* remove redundant code */
                if (curr_node->type == NODE_JMP &&
                    curr_node->FLink->offset == INVALID_OFFSET) {
                    cgp_except_node(curr_node);
                } else {
                    curr_node->offset = offset;
                    memcpy(&buff[offset], &curr_node->data[0], curr_node->weight);
                    offset += curr_node->weight;
                }

                if (curr_node->type != NODE_JMP && curr_node->FLink &&
                    curr_node->FLink->offset != INVALID_OFFSET)
                {
                    new_node = cgp_allocate_node();

                    new_node->offset = offset;
                    new_node->type = NODE_JMP;
                    new_node->weight = 5;
                    new_node->data = (uint8_t *) malloc(new_node->weight);
                    new_node->data[0] = OPCODE_X86_JMP_REL32;
                    new_node->data[1] = 0xCC;
                    new_node->data[2] = 0xCC;
                    new_node->data[3] = 0xCC;
                    new_node->data[4] = 0xCC;
                    new_node->BLink = curr_node->BLink;
                    new_node->FLink = curr_node->FLink;

                    memcpy(&buff[offset], &new_node->data[0], new_node->weight);
                    offset += new_node->weight;
                }
            }

            curr_node = curr_node->FLink;
        } else {
            curr_node = NULL;
        }

        /* process JCC | CALL branch */
        if (!curr_node) {
            if (jcc_count != 0) {
                curr_node = cgp_pop_jcc();
                curr_node = curr_node->CLink;
                continue;
            }

            if (call_count != 0) {
                curr_node = cgp_pop_call();
                curr_node = curr_node->CLink;
                continue;
            }
        }
    }

    /* check that all nodes has been written */
    for (uint32_t i = 0 ; i < nodes_count ; i++) {
        if (nodes[i] && nodes[i]->offset == INVALID_OFFSET) {
            curr_node = nodes[i];
            curr_node->offset = offset;
            memcpy(&buff[offset], &curr_node->data[0], curr_node->weight);
            offset += curr_node->weight;
        }
    }

    /* configure branch's address */
    for (uint32_t i = 0 ; i < nodes_count ; i++) if (nodes[i]) {
        if (nodes[i]->type != NODE_JMP &&
            nodes[i]->type != NODE_JCC &&
            nodes[i]->type != NODE_CALL)
            continue;

        cgp_write_offset(nodes[i], buff);
    }

    *out_buff = buff;
    return offset;
}

static void cgp_add_reduntant_nop(void)
{
    uint32_t original_nodes_count = nodes_count;
    pNode insert_node;

    for (uint32_t i = 0 ; i < original_nodes_count - 1 ; i++) {
        if (!nodes[i])
            continue;

        insert_node = cgp_insert_node(nodes[i], INSERT_AFTER);

        insert_node->weight = 1;
        insert_node->data = (uint8_t*) malloc(insert_node->weight);
        insert_node->data[0] = OPCODE_X86_NOP;

        insert_node->CLink = NULL;
        insert_node->type = NODE_LINE;
    }
}

uint32_t cgp_build_reduntant_nop(uint8_t **out_buff)
{
    cgp_add_reduntant_nop();
    return cgp_build(out_buff);
}

uint32_t cgp_build_spaghetti(uint8_t **out_buff)
{
    uint32_t i, original_nodes_count, offset = 0;
    uint8_t *buff;
    pNode curr_node, new_node;

    buff = (uint8_t*) calloc(sizeof(uint8_t), CODE_BUFFER_LIMIT);

    #define SKIP_FIRST_X_NODES 0

    shuffle_array(&nodes[SKIP_FIRST_X_NODES],
                  nodes_count - SKIP_FIRST_X_NODES,
                  sizeof(pNode));

    original_nodes_count = nodes_count;

    if (first_node && SKIP_FIRST_X_NODES == 0) {
        new_node = cgp_allocate_node();

        new_node->offset = offset;
        new_node->type = NODE_JMP;
        new_node->weight = 5;
        new_node->data = (uint8_t*) malloc(new_node->weight);
        new_node->data[0] = OPCODE_X86_JMP_REL32;
        new_node->data[1] = 0xCC;
        new_node->data[2] = 0xCC;
        new_node->data[3] = 0xCC;
        new_node->data[4] = 0xCC;
        new_node->FLink = first_node;

        memcpy(&buff[offset], &new_node->data[0], new_node->weight);
        offset += new_node->weight;
    }

    for (i = 0 ; i < original_nodes_count ; i++) {
        if (!nodes[i])
            continue;

        assert(offset < CODE_BUFFER_LIMIT);

        nodes[i]->offset = offset;
        memcpy(&buff[offset], &nodes[i]->data[0], nodes[i]->weight);
        offset += nodes[i]->weight;

        if (i < SKIP_FIRST_X_NODES)
            continue;

        curr_node = nodes[i];

        /* there execution flow after RET */
        if (curr_node->type == NODE_RET)
            continue;

        new_node = cgp_allocate_node();

        new_node->offset = offset;
        new_node->type = NODE_JMP;
        new_node->weight = 5;
        new_node->data = (uint8_t*) malloc(new_node->weight);
        new_node->data[0] = OPCODE_X86_JMP_REL32;
        new_node->data[1] = 0xCC;
        new_node->data[2] = 0xCC;
        new_node->data[3] = 0xCC;
        new_node->data[4] = 0xCC;
        new_node->BLink = curr_node->BLink;
        new_node->FLink = curr_node->FLink;

        memcpy(&buff[offset], &new_node->data[0], new_node->weight);
        offset += new_node->weight;
    }

    /* calculate branch address */
    for (i = 0 ; i < nodes_count ; i++) {
        if (!nodes[i])
            continue;

        if (nodes[i]->type != NODE_JMP &&
            nodes[i]->type != NODE_JCC &&
            nodes[i]->type != NODE_CALL)
            continue;

        cgp_write_offset(nodes[i], buff);
    }

    *out_buff = buff;
    return offset;
}

void cgp_remove_simple_obfuscation(void)
{
    for (uint32_t i = 0 ; i < nodes_count ; i++) {
        if (!nodes[i])
            continue;

        /* CALL -> JMP */
        if (nodes[i]->type == NODE_CALL &&
           *(uint32_t*) &nodes[i]->CLink->data[0] == 0x0424648D) {
            cgp_except_node(nodes[i]->FLink);
            cgp_except_node(nodes[i]->CLink);
            cgp_except_node(nodes[i]);
        }
    }
}

void cgp_init(uint8_t *in_buff, uint32_t in_size, uint32_t entry_point)
{
    cgp_randomize();
    nodes_count = 0;
    nodes = NULL;
    cgp_parse(in_buff, in_size, entry_point);
}

void cgp_free(void)
{
    for (uint32_t i = 0 ; i < nodes_count ; i++) {
        if (!nodes[i])
            continue;

        free(nodes[i]->data);
        free(nodes[i]);
    }

    free(nodes);
}
