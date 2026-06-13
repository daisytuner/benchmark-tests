#include <stdio.h>
#include <stdlib.h>

#include "block.h"
#include "params.h"

// From header
int blockarray_switch;
block* blocks;
double* blockarrays;
size_t blockarrays_length;
parent* parents;
size_t max_active_block;
int max_active_parent, num_active;
sorted_block* sorted_list;
int* sorted_index;
int* num_blocks;
size_t mesh_size[3];
int* p2;
long num_blocks_init;

const int side[6][2][2] = { { {0, 2}, {4, 6} }, { {1, 3}, {5, 7} },
                            { {0, 1}, {4, 5} }, { {2, 3}, {6, 7} },
                            { {0, 1}, {2, 3} }, { {4, 5}, {6, 7} } };
const int off[6] = {1, -1, 2, -2, 4, -4};

// Returns the starting block number for a given level
int get_block_start(int level) {
    int start = 0;
    int num = 1;
    for(int i = 0; i < level; ++i) {
        start += num;
        num *= 8;
    }
    return start;
}

// Remove block from the sorted list
void del_sorted_list(int number, int level) {
    int i;

    for(i = sorted_index[level]; i < sorted_index[level + 1]; ++i) {
        if(number == sorted_list[i].number)
            break;
    }
    if(number != sorted_list[i].number) {
        printf("Error: Cannot delete from sorted block list - number %i not found", number);
        exit(3);
    }
    for(int j = level + 1; j <= (num_refine + 1); ++j)
        --sorted_index[j];
    for(int j = i; j < sorted_index[num_refine + 1]; ++j) {
        sorted_list[j].number = sorted_list[j + 1].number;
        sorted_list[j].n = sorted_list[j + 1].n;
    }
}

// From header
void add_sorted_list(int n, int number, int level) {
    int i;

    for(i = sorted_index[level]; i < sorted_index[level + 1]; ++i) {
        if(number > sorted_list[i].number)
            break;
    }
    for(int j = sorted_index[num_refine + 1]; j > i; --j) {
        sorted_list[j].number = sorted_list[j - 1].number;
        sorted_list[j].n = sorted_list[j - 1].n;
    }
    sorted_list[i].number = number;
    sorted_list[i].n = n;
    for(i = level + 1; i <= (num_refine + 1); ++i)
        ++sorted_index[i];
}

// From header
void split_blocks(int cur_max_level) {
    size_t m;
    int p, sib[8];

    m = 0;
    for(int level = 0; level <= cur_max_level; ++level) {
        for(size_t i = 0; i < max_active_block; ++i) {
            if(blocks[i].number < 0 || blocks[i].level != level || blocks[i].refine != 1)
                continue;
            block* bp = &blocks[i];
            int nl = bp->number - get_block_start(level);
            int zp = nl / (p2[level] * p2[level]);
            int yp = (nl % (p2[level] * p2[level])) / p2[level];
            int xp = nl % p2[level];
            if((unsigned) (num_active + 8) > max_num_blocks) {
                printf("Error: Need more blocks %i/%zu\n", num_active, max_num_blocks);
                exit(2);
            }
            del_sorted_list(bp->number, level);
            num_active += 7;
            --num_blocks[level];
            num_blocks[level + 1] += 8;
            for(p = 0; p < max_active_parent; ++p) {
                if(parents[p].number < 0)
                    break;
            }
            if((unsigned) p == max_num_blocks) {
                printf("Error: Need more parents\n");
                exit(9);
            }
            if(p == max_active_parent)
                ++max_active_parent;
            parent* pp = &parents[p];
            pp->number = bp->number;
            pp->level = bp->level;
            pp->parent = bp->parent;
            pp->child_number = bp->child_number;
            if(pp->parent >= 0)
                parents[pp->parent].child[pp->child_number] = -1 - p;
            pp->refine = 0;
            pp->cen[0] = bp->cen[0];
            pp->cen[1] = bp->cen[1];
            pp->cen[2] = bp->cen[2];
            for(int c = 0; c < 8; ++c) {
                for(; m < max_num_blocks; ++m) {
                    if(blocks[m].number < 0)
                        break;
                }
                if(m == max_num_blocks) {
                    printf("Error: No inactive block available %i/%zu\n", num_active, max_num_blocks);
                    exit(4);
                }
                if((m + 1) > max_active_block)
                    max_active_block = m + 1;
                block* bpnew = &blocks[m];
                sib[c] = m;
                pp->child[c] = m;
                bpnew->refine = 0;
                bpnew->level = level + 1;
                bpnew->parent = p;
                bpnew->child_number = c;
                int x = c % 2;
                int y = (c / 2) % 2;
                int z = c / 4;
                bpnew->number = ((2 * zp + z) * p2[level + 1] + (2 * yp + y)) * p2[level + 1] + 2 * xp + x + get_block_start(level + 1);
                add_sorted_list(m, bpnew->number, level + 1);
                bpnew->cen[0] = bp->cen[0] + (2 * x - 1) * p2[num_refine - level - 1];
                bpnew->cen[1] = bp->cen[1] + (2 * y - 1) * p2[num_refine - level - 1];
                bpnew->cen[2] = bp->cen[2] + (2 * z - 1) * p2[num_refine - level - 1];
                x *= (x_block_size / 2);
                y *= (y_block_size / 2);
                z *= (z_block_size / 2);
                for(size_t j = 1, j2 = 1; j <= (x_block_size / 2); ++j, j2 += 2)
                    for(size_t k = 1, k2 = 1; k <= (y_block_size / 2); ++k, k2 += 2)
                        for(size_t l = 1, l2 = 1; l <= (z_block_size / 2); ++l, l2 += 2)
                            BLOCKARRAY_READ(m, j2    , k2    , l2    ) =
                            BLOCKARRAY_READ(m, j2 + 1, k2    , l2    ) =
                            BLOCKARRAY_READ(m, j2    , k2 + 1, l2    ) =
                            BLOCKARRAY_READ(m, j2 + 1, k2 + 1, l2    ) =
                            BLOCKARRAY_READ(m, j2    , k2    , l2 + 1) =
                            BLOCKARRAY_READ(m, j2 + 1, k2    , l2 + 1) =
                            BLOCKARRAY_READ(m, j2    , k2 + 1, l2 + 1) =
                            BLOCKARRAY_READ(m, j2 + 1, k2 + 1, l2 + 1) = BLOCKARRAY_READ(i, j + x, k + y, l + z) / 8.0;
            }
            for(int j = 0; j < 6; ++j) {
                for(int k = 0; k < 2; ++k) {
                    for(int l = 0; l < 2; ++l) {
                        blocks[sib[side[j][k][l] + off[j]]].nei_level[j] = level + 1;
                        blocks[sib[side[j][k][l] + off[j]]].nei[j][0][0] = sib[side[j][k][l]];
                    }
                }
                if(bp->nei_level[j] == -2) {
                    for(int k = 0; k < 2; ++k) {
                        for(int l = 0; l < 2; ++l) {
                            blocks[sib[side[j][k][l]]].nei_level[j] = -2;
                            blocks[sib[side[j][k][l]]].nei[j][0][0] = 0;
                        }
                    }
                } else if(bp->nei_level[j] == level - 1 && bp->nei[j][0][0] >= 0) {
                    printf("Error: Internal misconnected block %zu num %i c %i %i\n", i, bp->number, j, bp->nei[j][0][0]);
                    exit(5);
                } else if(bp->nei_level[j] == level && bp->nei[j][0][0] >= 0) {
                    int other = bp->nei[j][0][0];
                    blocks[other].nei_level[(j / 2) * 2 + (j + 1) % 2] = level + 1;
                    for(int k = 0; k < 2; ++k) {
                        for(int l = 0; l < 2; ++l) {
                            blocks[sib[side[j][k][l]]].nei_level[j] = level;
                            blocks[sib[side[j][k][l]]].nei[j][0][0] = other;
                            blocks[other].nei[(j / 2) * 2 + (j + 1) % 2][k][l] = sib[side[j][k][l]];
                        }
                    }
                } else if(bp->nei_level[j] == level + 1) {
                    for(int k = 0; k < 2; ++k) {
                        for(int l = 0; l < 2; ++l) {
                            if(bp->nei[j][k][l] >= 0) {
                                blocks[sib[side[j][k][l]]].nei_level[j] = level + 1;
                                blocks[sib[side[j][k][l]]].nei[j][0][0] = bp->nei[j][k][l];
                                blocks[bp->nei[j][k][l]].nei_level[(j / 2) * 2 + (j + 1) % 2] = level + 1;
                                blocks[bp->nei[j][k][l]].nei[(j / 2) * 2 + (j + 1) % 2][0][0] = sib[side[j][k][l]];
                            }
                        }
                    }
                } else {
                    printf("Error: Misconnected b %zu %i l %i nei[%i] %i\n", i, bp->number, level, j, bp->nei_level[j]);
                    exit(6);
                }
            }
            bp->number = -1;
            if((unsigned) i < m)
                m = (unsigned) i;
        }
    }
}

// From header
void consolidate_blocks(int cur_max_level) {
    size_t m;

    for(int level = cur_max_level; level >= 0; --level) {
        for(int p = 0; p < max_active_parent; ++p) {
            if(parents[p].number < 0 || parents[p].level != level || parents[p].refine != -1)
                continue;
            for(m = 0; m < max_num_blocks; ++m)
                if(blocks[m].number < 0)
                    break;
            if(m == max_num_blocks) {
                printf("Error: Out of free blocks in consolidate_blocks\n");
                exit(7);
            }
            block* bp = &blocks[m];
            parent* pp = &parents[p];
            if((m + 1) > max_active_block)
                max_active_block = m + 1;
            num_active -= 7;
            ++num_blocks[level];
            num_blocks[level + 1] -= 8;
            bp->number = pp->number;
            pp->number = -1;
            bp->level = pp->level;
            bp->parent = pp->parent;
            bp->child_number = pp->child_number;
            if(bp->parent >= 0)
                parents[bp->parent].child[bp->child_number] = m;
            add_sorted_list(m, bp->number, level);
            bp->refine = 0;
            bp->cen[0] = pp->cen[0];
            bp->cen[1] = pp->cen[1];
            bp->cen[2] = pp->cen[2];
            for(int c = 0; c < 8; ++c) {
                block* bpold = &blocks[pp->child[c]];
                del_sorted_list(bpold->number, level + 1);
                bpold->number = -1;
                int x = (c % 2) * (x_block_size / 2);
                int y = ((c / 2) % 2) * (y_block_size / 2);
                int z = (c / 4) * (z_block_size / 2);
                for(size_t i = 1, i2 = 1; i <= (x_block_size / 2); ++i, i2 += 2)
                    for(size_t j = 1, j2 = 1; j <= (y_block_size / 2); ++j, j2 += 2)
                        for(size_t k = 1, k2 = 1; k <= (z_block_size / 2); ++k, k2 += 2)
                            BLOCKARRAY_READ(m, i + x, j + y, k + z) = BLOCKARRAY_READ(pp->child[c], i2    , j2    , k2    ) +
                                                                      BLOCKARRAY_READ(pp->child[c], i2 + 1, j2    , k2    ) +
                                                                      BLOCKARRAY_READ(pp->child[c], i2    , j2 + 1, k2    ) +
                                                                      BLOCKARRAY_READ(pp->child[c], i2 + 1, j2 + 1, k2    ) +
                                                                      BLOCKARRAY_READ(pp->child[c], i2    , j2    , k2 + 1) +
                                                                      BLOCKARRAY_READ(pp->child[c], i2 + 1, j2    , k2 + 1) +
                                                                      BLOCKARRAY_READ(pp->child[c], i2    , j2 + 1, k2 + 1) +
                                                                      BLOCKARRAY_READ(pp->child[c], i2 + 1, j2 + 1, k2 + 1);
            }
            for(int i = 0; i < 6; ++i) {
                int other = pp->child[side[i][0][0]];
                if(blocks[other].nei_level[i] == -2) {
                    bp->nei_level[i] = -2;
                    bp->nei_refine[i] = 0;
                } else if(blocks[other].nei_level[i] == level) {
                    if(blocks[other].nei[i][0][0] >= 0) {
                        bp->nei[i][0][0] = blocks[other].nei[i][0][0];
                        bp->nei_level[i] = level;
                        bp->nei_refine[i] = 0;
                        blocks[blocks[other].nei[i][0][0]].nei[(i / 2) * 2 + (i + 1) % 2][0][0] = m;
                        blocks[blocks[other].nei[i][0][0]].nei_level[(i / 2) * 2 + (i + 1) % 2] = level;
                        blocks[blocks[other].nei[i][0][0]].nei_refine[(i / 2) * 2 + (i + 1) % 2] = 0;
                    }
                } else {
                    for(int j = 0; j < 2; ++j) {
                        for(int k = 0; k < 2; ++k) {
                            other = pp->child[side[i][j][k]];
                            if(blocks[other].nei[i][0][0] < 0)
                                continue;
                            if(blocks[other].nei_level[i] == level + 2) {
                                printf("Error: %i con %zu block %i c %i wrong level %i", p, m, other, i, level);
                                exit(8);
                            }
                            bp->nei[i][j][k] = blocks[other].nei[i][0][0];
                            bp->nei_level[i] = level + 1;
                            bp->nei_refine[i] = 0;
                            blocks[blocks[other].nei[i][0][0]].nei[(i / 2) * 2 + (i + 1) % 2][0][0] = m;
                            blocks[blocks[other].nei[i][0][0]].nei_level[(i / 2) * 2 + (i + 1) % 2] = level;
                            blocks[blocks[other].nei[i][0][0]].nei_refine[(i / 2) * 2 + (i + 1) % 2] = 0;
                        }
                    }
                }
            }
        }
    }
}

// From header
void switch_blockarrays(void) {
    blockarray_switch = (blockarray_switch + 1) % 2;
}
