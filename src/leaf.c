#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include "leaf.h"

uint64_t LEAF_POINTERS_SIZE = sizeof(uint32_t);
uint64_t LEAF_NUMS_SIZE = sizeof(uint64_t);
uint64_t LEAF_LEADING_STARTS_ENDS_SIZE = sizeof(uint64_t);

//{{{uint64_t leaf_data_serialize(void *deserialized, void **serialized)
uint64_t leaf_data_serialize(void *deserialized, void **serialized)
{
#if 0
    struct leaf_data *de = (struct leaf_data *)deserialized;

    uint32_t *data = (uint32_t *)malloc(
            3*sizeof(uint32_t) +
            ((de->num_leading + de->num_starts + de->num_ends)
             * sizeof(uint32_t))*2);
    
    data[0] = de->num_leading;
    data[1] = de->num_starts;
    data[2] = de->num_ends;

    uint8_t *output = (uint8_t *)(data + 3);
    int cs = fastlz_compress(de->data,
                             (de->num_leading + 
                             de->num_starts + 
                             de->num_ends) * sizeof(uint32_t),
                             output);
    //realloc(data, 3*sizeof(uint32_t) + cs*sizeof(int));
    *serialized = (void *)data;
    return 3*sizeof(uint32_t) + cs*sizeof(int);

#endif
#if 1

    struct leaf_data *de = (struct leaf_data *)deserialized;

#ifdef DEBUG_LEAF_DATA
//{{{
    fprintf(stderr,
            "leaf_data_serialize\t"
            "de->num_leading:%llu\t"
            "de->num_starts:%llu\t"
            "de->num_ends:%llu\n",
            de->num_leading,
            de->num_starts,
            de->num_ends);

    uint32_t i;
    fprintf(stderr,
            "leaf_data_serialize\t"
            "starts: ");
    for (i = 0; i < de->num_starts; ++i)
        fprintf(stderr, "%llu ", de->starts[i]);
    fprintf(stderr, "\n");

    fprintf(stderr,
            "leaf_data_serialize\t"
            "ends: ");
    for (i = 0; i < de->num_ends; ++i)
        fprintf(stderr, "%llu ", de->ends[i]);
    fprintf(stderr, "\n");

    fprintf(stderr,
            "leaf_data_serialize\t"
            "starts_pointers: ");
    for (i = 0; i < ORDER; ++i)
        fprintf(stderr, "%u ", de->starts_pointers[i]);
    fprintf(stderr, "\n");

    fprintf(stderr,
            "leaf_data_serialize\t"
            "ends_pointers: ");
    for (i = 0; i < ORDER; ++i)
        fprintf(stderr, "%u ", de->ends_pointers[i]);
    fprintf(stderr, "\n");
//}}}
#endif

    uint64_t data_size = 
            (2 * ORDER * LEAF_POINTERS_SIZE) // starts_pointers, ends_pointers
            + (3 *  LEAF_NUMS_SIZE) // num_leading,num_starts,num_ends
            + ((de->num_leading + de->num_starts + de->num_ends) 
                * LEAF_LEADING_STARTS_ENDS_SIZE); // leading,starts,ends

#ifdef DEBUG_LEAF_DATA
//{{{
    fprintf(stderr,
            "leaf_data_serialize\t"
            "data_size:%llu\n",
            data_size);
//}}}
#endif
 
    uint8_t *data = (uint8_t *) malloc(data_size);

    if (data == NULL)
        err(1, "calloc error in leaf_data_serialize.\n");

    memset(data, 0, data_size);

    // copy starts pointers
    memcpy(data,
           de->starts_pointers,
           ORDER * LEAF_POINTERS_SIZE);

    // copy ends pointers
    memcpy(data + (ORDER * LEAF_POINTERS_SIZE),
           de->ends_pointers,
           ORDER * LEAF_POINTERS_SIZE);

    // copy number of leading values
    memcpy(data + (2 * ORDER * LEAF_POINTERS_SIZE) + (0 * LEAF_NUMS_SIZE),
           &(de->num_leading),
           LEAF_NUMS_SIZE);

    // copy number of starts values
    memcpy(data + (2 * ORDER * LEAF_POINTERS_SIZE) + (1 * LEAF_NUMS_SIZE),
           &(de->num_starts),
           LEAF_NUMS_SIZE);

    // copy number of ends values
    memcpy(data + (2 * ORDER * LEAF_POINTERS_SIZE) + (2 * LEAF_NUMS_SIZE),
           &(de->num_ends),
           LEAF_NUMS_SIZE);

    // copy leading, starts, and ends
    memcpy(data + (2 * ORDER * LEAF_POINTERS_SIZE) + (3 * LEAF_NUMS_SIZE),
           de->data,
           (de->num_leading + de->num_starts + de->num_ends) 
                * LEAF_LEADING_STARTS_ENDS_SIZE);
    
#ifdef DEBUG_LEAF_DATA
//{{{
    fprintf(stderr,
            "leaf_data_serialize\t"
            "serialized start pointers:");
    for (i = 0; i < ORDER; ++i) {
        uint32_t v = ((uint32_t *)data)[i];
        fprintf(stderr," %u", v);
    }
    fprintf(stderr,"\n");

    fprintf(stderr,
            "leaf_data_serialize\t"
            "serialized end pointers:");
    for (i = 0; i < ORDER; ++i) {
        uint32_t v = ((uint32_t *)(data + ORDER * LEAF_POINTERS_SIZE))[i];
        fprintf(stderr," %u", v);
    }
    fprintf(stderr,"\n");

    uint64_t v;
    memcpy(&v,
           data + (2 * ORDER * LEAF_POINTERS_SIZE) + (0 * LEAF_NUMS_SIZE),
           LEAF_NUMS_SIZE);

    fprintf(stderr,
            "leaf_data_serialize\t"
            "serialized num_leading:%llu\n",
            v);

    memcpy(&v,
           data + (2 * ORDER * LEAF_POINTERS_SIZE) + (1 * LEAF_NUMS_SIZE),
           LEAF_NUMS_SIZE);

    fprintf(stderr,
            "leaf_data_serialize\t"
            "serialized num_starts:%llu\n",
            v);

    memcpy(&v,
           data + (2 * ORDER * LEAF_POINTERS_SIZE) + (2 * LEAF_NUMS_SIZE),
           LEAF_NUMS_SIZE);

    fprintf(stderr,
            "leaf_data_serialize\t"
            "serialized num_ends:%llu\n",
            v);
//}}}
#endif
    
    *serialized = (void *)data;
    return data_size;
#endif
}
//}}}

//{{{ uint64_t leaf_data_deserialize(void *serialized,
uint64_t leaf_data_deserialize(void *serialized,
                               uint64_t serialized_size,
                               void **deserialized)
{
    uint8_t *data = (uint8_t *)serialized;
    
    struct leaf_data *lf = (struct leaf_data *) 
            calloc(1, sizeof(struct leaf_data));
    if (lf == NULL)
        err(1, "calloc error in leaf_data_deserialize.\n");

    // copy number of leading values
    memcpy(&(lf->num_leading),
           data + (2 * ORDER * LEAF_POINTERS_SIZE) + (0 * LEAF_NUMS_SIZE),
           LEAF_NUMS_SIZE);

    // copy number of starts values
    memcpy(&(lf->num_starts),
           data + (2 * ORDER * LEAF_POINTERS_SIZE) + (1 * LEAF_NUMS_SIZE),
           LEAF_NUMS_SIZE);

    // copy number of ends values
    memcpy(&(lf->num_ends),
           data + (2 * ORDER * LEAF_POINTERS_SIZE) + (2 * LEAF_NUMS_SIZE),
           LEAF_NUMS_SIZE);

    // copy start pointers
    lf->starts_pointers = (uint32_t *)calloc(ORDER, LEAF_POINTERS_SIZE);
    if (lf->starts_pointers == NULL)
        err(1, "calloc error in leaf_data_deserialize.\n");

    memcpy(lf->starts_pointers,
           data,
           ORDER * LEAF_POINTERS_SIZE);
   
    // copy end pointers
    lf->ends_pointers = (uint32_t *)calloc(ORDER, LEAF_POINTERS_SIZE);
    if (lf->ends_pointers == NULL)
        err(1, "calloc error in leaf_data_deserialize.\n");

    memcpy(lf->ends_pointers,
           data + (ORDER * LEAF_POINTERS_SIZE),
           ORDER * LEAF_POINTERS_SIZE);

    // copy data / leading, starts, and ends
    lf->data = (uint64_t *)calloc(lf->num_leading 
                                    + lf->num_starts 
                                    + lf->num_ends,
                                  LEAF_LEADING_STARTS_ENDS_SIZE);
    if (lf->data == NULL)
        err(1, "calloc error in leaf_data_deserialize.\n");

    lf->leading = lf->data;
    lf->starts = lf->data + lf->num_leading;
    lf->ends = lf->data + lf->num_leading + lf->num_starts;

    memcpy(lf->data,
           data + (2 * ORDER * LEAF_POINTERS_SIZE) + (3 * LEAF_NUMS_SIZE),
           (lf->num_leading + lf->num_starts + lf->num_ends)
                * LEAF_LEADING_STARTS_ENDS_SIZE);

    *deserialized = (void *)lf;

    return sizeof(struct leaf_data);
}
//}}}

//{{{void leaf_data_free_mem(void **deserialized)
void leaf_data_free_mem(void **deserialized)
{
    struct leaf_data **de = (struct leaf_data **)deserialized;
    free((*de)->starts_pointers);
    free((*de)->ends_pointers);
    free((*de)->data);
    free(*de);
    *de = NULL;
}
//}}}

//{{{uint32_t leaf_data_starts_start(struct leaf_data *ld,
uint32_t leaf_data_starts_start(struct leaf_data *ld,
                                struct bpt_node *ln,
                                int i)
{
    if (i == 0) {
        return 0;
    } else if (i >= BPT_NUM_KEYS(ln)) {
        return ld->starts_pointers[BPT_NUM_KEYS(ln) - 1];
    } else {
        return ld->starts_pointers[i - 1];
    }
}
//}}}

//{{{uint32_t leaf_data_starts_end(struct leaf_data *ld,
uint32_t leaf_data_starts_end(struct leaf_data *ld,
                              struct bpt_node *ln,
                              int i)
{
    if (i == -1) {
        return 0;
    } else if (i == BPT_NUM_KEYS(ln)) {
        return ld->starts_pointers[i - 1];
    } else {
        return ld->starts_pointers[i];
    }
}
//}}}

//{{{uint32_t leaf_data_ends_start(struct leaf_data *ld,
uint32_t leaf_data_ends_start(struct leaf_data *ld,
                              struct bpt_node *ln,
                              int i)
{
    if (i == 0) {
        return 0;
    } else if (i >= BPT_NUM_KEYS(ln)) {
        return ld->ends_pointers[BPT_NUM_KEYS(ln) - 1];
    } else {
        return ld->ends_pointers[i - 1];
    }
}
//}}}

//{{{uint32_t leaf_data_ends_end(struct leaf_data *ld,
uint32_t leaf_data_ends_end(struct leaf_data *ld,
                            struct bpt_node *ln,
                            int i)
{
    if (i == -1) {
        return 0;
    } else if (i == BPT_NUM_KEYS(ln)) {
        return ld->ends_pointers[i - 1];
    } else {
        return ld->ends_pointers[i];
    }
}
//}}}
