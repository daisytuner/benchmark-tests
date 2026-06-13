#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "plot.h"
#include "params.h"
#include "block.h"

// plot values
typedef struct {
    int type;
    double cen[3];
    double size[3];
} plot_obj;
plot_obj** plot_objs;
typedef struct {
    int level;
    int cen[3];
} plot_block;
typedef struct {
    plot_block* blocks;
    size_t blocks_num;
} plot_block_step;
plot_block_step* plot_blocks;

// Copies data from obj into pobj
void copy_obj_data(object* obj, plot_obj* pobj) {
    pobj->type    = obj->type;
    pobj->cen[0]  = obj->cen[0];
    pobj->cen[1]  = obj->cen[1];
    pobj->cen[2]  = obj->cen[2];
    pobj->size[0] = obj->size[0];
    pobj->size[1] = obj->size[1];
    pobj->size[2] = obj->size[2];
}

void copy_block_data(block* blk, plot_block* pblk) {
    pblk->level  = blk->level;
    pblk->cen[0] = blk->cen[0];
    pblk->cen[1] = blk->cen[1];
    pblk->cen[2] = blk->cen[2];
}

// From header
void plot_init(void) {
    plot_objs = (plot_obj**) malloc(num_objects * sizeof(plot_obj*));
    for(int i = 0; i < num_objects; ++i) {
        plot_objs[i] = (plot_obj*) malloc((num_tsteps + 1) * sizeof(plot_obj));
        copy_obj_data(&objects[i], &plot_objs[i][0]);
    }
    plot_blocks = (plot_block_step*) malloc((num_tsteps + 1) * sizeof(plot_block_step));
}

// From header
void plot_update(int ts) {
    for(int i = 0; i < num_objects; ++i)
        copy_obj_data(&objects[i], &plot_objs[i][ts]);
    plot_block_step* step = &plot_blocks[ts];
    step->blocks_num = num_active;
    step->blocks = (plot_block*) malloc(num_active * sizeof(plot_block));
    int i = 0;
    for(size_t j = 0; j < max_active_block; ++j) {
        if(blocks[j].number < 0)
            continue;
        if(i > num_active) {
            printf("Error while gathering data for plot...\n");
            return;
        }
        copy_block_data(&blocks[j], &step->blocks[i]);
        ++i;
    }
}

// From header
void plot_dump(void) {
    FILE* f = fopen("../plot/data.json", "w");
    if(f == NULL) {
        printf("Error: Cannot open file ../plot/data.json\n");
        return;
    }
    fprintf(f, "{\"num_tsteps\":%i,\"num_objects\":%i,\"num_refine\":%i,\"block_size\":[%zu,%zu,%zu],\"objects\":[", (num_tsteps + 1), num_objects, num_refine, x_block_size, y_block_size, z_block_size);
    for(int i = 0; i < num_objects; ++i) {
        if(i > 0)
            fprintf(f, ",");
        fprintf(f, "{\"type\":%i,\"data\":[", plot_objs[i][0].type);
        for(int j = 0; j <= num_tsteps; ++j) {
            if(j > 0)
                fprintf(f, ",");
            fprintf(f, "{\"cen\":[%f,%f,%f],\"size\":[%f,%f,%f]}", plot_objs[i][j].cen[0], plot_objs[i][j].cen[1], plot_objs[i][j].cen[2], plot_objs[i][j].size[0], plot_objs[i][j].size[1], plot_objs[i][j].size[2]);
        }
        fprintf(f, "]}");
    }
    fprintf(f, "],\"blocks\":[");
    for(int i = 0; i <= num_tsteps; ++i) {
        if(i > 0)
            fprintf(f, ",");
        fprintf(f, "{\"blocks_num\":%li,\"blocks\":[", plot_blocks[i].blocks_num);
        for(size_t j = 0; j < plot_blocks[i].blocks_num; ++j) {
            if(j > 0)
                fprintf(f, ",");
            fprintf(f, "{\"level\":%i,\"cen\":[%i,%i,%i]}", plot_blocks[i].blocks[j].level, plot_blocks[i].blocks[j].cen[0], plot_blocks[i].blocks[j].cen[1], plot_blocks[i].blocks[j].cen[2]);
        }
        fprintf(f, "]}");
    }
    fprintf(f, "]}");
    fclose(f);
    free(plot_blocks);
    for(int i = 0; i < num_objects; ++i)
        free(plot_objs[i]);
    free(plot_objs);
}
