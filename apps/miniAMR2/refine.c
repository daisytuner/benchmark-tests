#include <stdio.h>
#include <math.h>

#include "refine.h"
#include "params.h"
#include "block.h"

// Reset all refinement values such that every blocks refinement has to be determined
void reset_all(void) {
    for(int in = 0; in < sorted_index[num_refine + 1]; ++in) {
        if(blocks[sorted_list[in].n].number >= 0)
            blocks[sorted_list[in].n].refine = -1;
    }
    for(int i = 0; i < max_active_parent; ++i) {
        if(parents[i].number < 0)
            continue;
        parent* pp = &parents[i];
        pp->refine = -1;
        for(int c = 0; c < 8; ++c) {
            if(pp->child[c] < 0)
                pp->refine = 0;
        }
        if(pp->refine == 0) {
            for(int c = 0; c < 8; ++c) {
                if(pp->child[c] >= 0 && blocks[pp->child[c]].refine == -1)
                    blocks[pp->child[c]].refine = 0;
            }
        }
    }
}

// Sets inside to 1 if val is inside the interval [min, max], 0 if not
// Sets right_half to 1 if val is greater (or equal) than the interval middle, 0 if not
void check_interval_bounds(double val, double min, double max, int* inside, int* right_half) {
    *inside = (val >= min && val <= max);
    *right_half = (val >= (min + max) / 2.0);
}

// Sets intersect to 1 if the block (cor) needs to be refined because of the spheroid (op)
void check_surface_of_spheroid(double cor[3][2], object* op, int* intersect) {
    int x_in, y_in, z_in, x_right, y_right, z_right;
    check_interval_bounds(op->cen[0], cor[0][0], cor[0][1], &x_in, &x_right);
    check_interval_bounds(op->cen[1], cor[1][0], cor[1][1], &y_in, &y_right);
    check_interval_bounds(op->cen[2], cor[2][0], cor[2][1], &z_in, &z_right);
    #define spheroid_block_dist1(d, d_right) (fabs(cor[d][d_right] - op->cen[d]) < op->size[d])
    #define spheroid_block_dist2(d1, d1_right, d2, d2_right) ((((cor[d1][d1_right] - op->cen[d1]) / op->size[d1]) * \
                                                               ((cor[d1][d1_right] - op->cen[d1]) / op->size[d1]) + \
                                                               ((cor[d2][d2_right] - op->cen[d2]) / op->size[d2]) * \
                                                               ((cor[d2][d2_right] - op->cen[d2]) / op->size[d2])) < 1.0)
    *intersect = ((((cor[0][1 - x_right] - op->cen[0]) / op->size[0]) * ((cor[0][1 - x_right] - op->cen[0]) / op->size[0]) +
                   ((cor[1][1 - y_right] - op->cen[1]) / op->size[1]) * ((cor[1][1 - y_right] - op->cen[1]) / op->size[1]) +
                   ((cor[2][1 - z_right] - op->cen[2]) / op->size[2]) * ((cor[2][1 - z_right] - op->cen[2]) / op->size[2])) > 1.0);
    // Missing: x_in, y_in, z_in because already satisfied
    if(x_in) {
        if(y_in) {
            // x_in, y_in, !z_in
            *intersect &= !z_in && spheroid_block_dist1(2, z_right);
        } else {
            if(z_in)
                // x_in, !y_in, z_in
                *intersect &= spheroid_block_dist1(1, y_right);
            else
                // x_in, !y_in, !z_in
                *intersect &= spheroid_block_dist2(1, y_right, 2, z_right);
        }
    } else {
        if(y_in) {
            if(z_in)
                // !x_in, y_in, z_in
                *intersect &= spheroid_block_dist1(0, x_right);
            else
                // !x_in, y_in, !z_in
                *intersect &= spheroid_block_dist2(0, x_right, 2, z_right);
        } else {
            if(z_in)
                // !x_in, !y_in, z_in
                *intersect &= spheroid_block_dist2(0, x_right, 1, y_right);
            else
                // !x_in, !y_in, !z_in
                *intersect &= ((((cor[0][x_right] - op->cen[0]) / op->size[0]) * ((cor[0][x_right] - op->cen[0]) / op->size[0]) +
                                ((cor[1][y_right] - op->cen[1]) / op->size[1]) * ((cor[1][y_right] - op->cen[1]) / op->size[1]) +
                                ((cor[2][z_right] - op->cen[2]) / op->size[2]) * ((cor[2][z_right] - op->cen[2]) / op->size[2])) < 1.0);
        }
    }
}

// Returns 1 if block needs to refined becaus of an object
int check_block(double cor[3][2]) {
    int intersect = 0;
    for(int i = 0; i < num_objects; ++i) {
        if(intersect > 0 || objects[i].size[0] < 0.0 || objects[i].size[1] < 0.0 || objects[i].size[2] < 0.0)
            // skip since already determined to be refined
            // or is not able to refine
            // or size is less than zero
            continue;
        else if(objects[i].type == 2)
            // surface of a spheroid
            check_surface_of_spheroid(cor, &objects[i], &intersect);
        else
            printf("Undefined object %i\n", objects[i].type);
    }
    return intersect;
}

// Determines the blocks to be refined
void check_objects(void) {
    double cor[3][2];

    for(int in = 0; in < sorted_index[num_refine + 1]; ++in) {
        if(blocks[sorted_list[in].n].number < 0)
            continue;
        block* bp = &blocks[sorted_list[in].n];
        // half size of block
        int sz = p2[num_refine - bp->level];
        for(int d = 0; d < 3; ++d) {
            cor[d][0] = ((double) (bp->cen[d] - sz)) / ((double) mesh_size[d]);
            cor[d][1] = ((double) (bp->cen[d] + sz)) / ((double) mesh_size[d]);
        }
        //bp->refine = check_block(cor);
        if(check_block(cor))
            bp->refine = 1;
        // if at max refinement, then can not refine
        if((bp->level == num_refine && bp->refine == 1) || bp->refine == 0) {
            bp->refine = 0;
            if(bp->parent != -1) {
                parent* pp = &parents[bp->parent];
                pp->refine = 0;
                for(int c = 0; c < 8; ++c)
                    if(pp->child[c] >= 0 && blocks[pp->child[c]].refine == -1)
                        blocks[pp->child[c]].refine = 0;
            }
        }
        // if 0 level, we can not unrefine
        if(bp->level == 0 && bp->refine == -1)
            bp->refine = 0;
    }
}

// Check for blocks at level level that will refine
void refine_level_changing(int level) {
    int change;

    do {
        change = 0;
        for(int in = sorted_index[level]; in < sorted_index[level + 1]; ++in) {
            if(blocks[sorted_list[in].n].number < 0 || blocks[sorted_list[in].n].level != level)
                continue;
            block* bp = &blocks[sorted_list[in].n];
            if(bp->refine == 1) {
                if(bp->parent != -1) {
                    parent* pp = &parents[bp->parent];
                    if(pp->refine == -1)
                        pp->refine = 0;
                    for(int c = 0; c < 8; ++c) {
                        if(pp->child[c] >= 0 && blocks[pp->child[c]].refine == -1) {
                            blocks[pp->child[c]].refine = 0;
                            ++change;
                        }
                    }
                }
                for(int i = 0; i < 6; ++i) {
                    if(bp->nei_level[i] == level) {
                        if(bp->nei[i][0][0] < 0 || blocks[bp->nei[i][0][0]].refine != -1)
                            continue;
                        int nei = bp->nei[i][0][0];
                        blocks[nei].refine = 0;
                        ++change;
                        if(blocks[nei].parent == -1)
                            continue;
                        parent* pp = &parents[blocks[nei].parent];
                        if(pp->refine == -1)
                            pp->refine = 0;
                        for(int c = 0; c < 8; ++c) {
                            if(pp->child[c] >= 0 && blocks[pp->child[c]].refine == -1) {
                                blocks[pp->child[c]].refine = 0;
                                ++change;
                            }
                        }
                    } else if(bp->nei_level[i] == level - 1) {
                        if(bp->nei[i][0][0] < 0 || blocks[bp->nei[i][0][0]].refine == 1)
                            continue;
                        blocks[bp->nei[i][0][0]].refine = 1;
                        ++change;
                    }
                }
            } else if(bp->refine == -1) {
                for(int i = 0; i < 6; ++i) {
                    if(bp->nei_level[i] != level + 1)
                        continue;
                    bp->refine = 0;
                    ++change;
                    if(bp->parent == -1)
                        continue;
                    parent* pp = &parents[bp->parent];
                    if(pp->refine == -1)
                        pp->refine = 0;
                    for(int c = 0; c < 8; ++c)
                        if(pp->child[c] >= 0 && blocks[pp->child[c]].refine == -1)
                            blocks[pp->child[c]].refine = 0;
                }
            }
        }
    } while(change);
}

// Check for blocks at level level that will remain at this level
void refine_level_remaining(int level) {
    int change;

    do {
        change = 0;
        for(int in = sorted_index[level]; in < sorted_index[level + 1]; ++in) {
            if(blocks[sorted_list[in].n].number < 0 || blocks[sorted_list[in].n].level != level || blocks[sorted_list[in].n].refine != 0)
                continue;
            block* bp = &blocks[sorted_list[in].n];
            for(int i = 0; i < 6; ++i) {
                if(bp->nei_level[i] == level - 1) {
                    if(bp->nei[i][0][0] < 0 || blocks[bp->nei[i][0][0]].refine != -1)
                        continue;
                    int nei = bp->nei[i][0][0];
                    blocks[nei].refine = 0;
                    ++change;
                    if(blocks[nei].parent == -1 || parents[blocks[nei].parent].refine != -1)
                        continue;
                    parent* pp = &parents[blocks[nei].parent];
                    pp->refine = 0;
                    for(int c = 0; c < 8; ++c)
                        if(pp->child[c] >= 0 && blocks[pp->child[c]].refine == -1)
                            blocks[pp->child[c]].refine = 0;
                } else if(bp->nei_level[i] == level) {
                    if(bp->nei[i][0][0] >= 0)
                        blocks[bp->nei[i][0][0]].nei_refine[(i / 2) * 2 + (i + 1) % 2] = 0;
                } else if(bp->nei_level[i] == level + 1) {
                    for(int j = 0; j < 2; ++j)
                        for(int k = 0; k < 2; ++k)
                            if(bp->nei[i][j][k] >= 0)
                                blocks[bp->nei[i][j][k]].nei_refine[(i / 2) * 2 + (i + 1) % 2] = 0;
                }
            }
        }
    } while(change);
}

void refine_level(int cur_max_level) {
    for(int level = cur_max_level; level >= 0; --level) {
        refine_level_changing(level);
        refine_level_remaining(level);
    }
}

// From header
void refine(int ts) {
    int cur_max_level;

    for(int i = 0; i < num_refine; ++i) {
        for(int j = num_refine; j >= 0; --j) {
            if(num_blocks[j]) {
                cur_max_level = j;
                break;
            }
        }
        reset_all();
        check_objects();
        refine_level(cur_max_level);
        split_blocks(cur_max_level);
        consolidate_blocks(cur_max_level);
    }
    for(int i = 0; i <= num_refine; ++i)
        printf("Number of blocks at level %i at timestamp %i is %i\n", i, ts, num_blocks[i]);
    printf("\n");
}

// From header
void move(void) {
    for(int i = 0; i < num_objects; ++i)
        for(int j = 0; j < 3; ++j) {
            objects[i].cen[j] += objects[i].move[j];
            if(objects[i].bounce) {
                if(objects[i].cen[j] >= 1.0) {
                    objects[i].cen[j] = 2.0 - objects[i].cen[j];
                    objects[i].move[j] = -objects[i].move[j];
                } else if(objects[i].cen[j] <= 0.0){
                    objects[i].cen[j] = 0.0 - objects[i].cen[j];
                    objects[i].move[j] = -objects[i].move[j];
                }
            }
            objects[i].size[j] += objects[i].inc[j];
        }
}
