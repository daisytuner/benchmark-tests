#include "kernel.h"
#include <tt-metalium/host_api.hpp>
#include <tt-metalium/event.hpp>

const int tt_row_block_count = 32; // number of rows per tile, each tile contains 32 rows

typedef float nnz_t;
inline constexpr auto zero_nnz = 0.0f;

struct __attribute__((packed)) tt_c_spmv_row {
    nnz_t data[tt_row_block_count];
    uint8_t zeroCounts[16]; // asumes little-endian because its 32 nibbles, matching data
};

struct __attribute__((packed)) tt_c_spmv_tile {
    uint16_t row_meta[40] = {0}; // 32+1 actual entries, 2 bytes per row for row metadata, Aligned to 16-byte boundary naturally
    tt_c_spmv_row content[tt_row_block_count]; // 32 rows, each with 32 floats and 16 zero counts is the max that can be stored in a tile. We want to distribute this across banks, accept it may be padded in each tile
};

struct tt_c_build_state {
    int trow = -1;
    int tcol = -1;
    uint32_t nnz_in_tile = 0;
    int intile_row = 0; // current row index in the tile
    int last_intile_col_idx = -1; // last column written to in the current row

    std::shared_ptr<tt::tt_metal::Event> event;

    tt_c_spmv_tile tile;

    void clear() {
        trow = -1;
        tcol = -1;
        nnz_in_tile = 0;
        intile_row = 0;
        last_intile_col_idx = -1;
    }

    void open(int row_idx, int col_idx) {
        if (event) { // wait until the chunk has been copied over to the device, so we can overwrite it.
            tt::tt_metal::EventSynchronize(event);
            event.reset();
        }

        if (this->trow < 0) {
            // std::cout << "Opening tile [" << row_idx/32 << ", " << col_idx/32 << "]!" << std::endl;
            this->trow = row_idx;
            this->tcol = col_idx;
        } else {
            // std::cout << "Reopening tile [" << trow/32 << ", " << tcol/32 << "]!" << std::endl;
        }
    }

    /// increment nnz_count and return the compressed coord of where to store said entry (crow_idx, crow_offset
    std::pair<int, int> reserve_entry() {
        auto nnz_in_tile = this->nnz_in_tile;
        if (nnz_in_tile >= 32 * 32) {
            throw std::runtime_error("Tile full!");
        }
        auto c_row_idx = nnz_in_tile >> 5;
        auto c_row_offset = nnz_in_tile & 0x1F; // 5 bits for the offset in the row
        this->nnz_in_tile = nnz_in_tile + 1;
        return {c_row_idx, c_row_offset};
    }

    /// return the compressed coord of where the last nnz was stored (crow_idx, crow_offset
    std::pair<int, int> get_last_caddr() {
        auto last_nnz_count = this->nnz_in_tile - 1;
        auto c_row_idx = last_nnz_count >> 5;
        auto c_row_offset = last_nnz_count & 0x1F; // 5 bits for the offset in the row
        return {c_row_idx, c_row_offset};
    }

    void entered_row(int row_idx) {
        auto last_row = this->trow + this->intile_row;
        if (row_idx != last_row+1 && row_idx != last_row) {
            throw std::runtime_error("Row index mismatch! Last: " + std::to_string(last_row) + ", got " + std::to_string(row_idx));
        }
        this->intile_row = row_idx - this->trow;
        this->last_intile_col_idx = -1; // not yet written to the current row
    }

    void seal_row() {
        if (last_intile_col_idx < 31) { // still need to write the last elem
            store_nnz_internal(zero_nnz, 31); // write a zero for the last column (will fill in row-start and intermediaries if needed
        }

        // we already wrote a value for the last column, either before this method or as part of it.
        write_last_zeroCount(0);

        auto intile_row_idx = this->intile_row;
        tile.row_meta[intile_row_idx+1] = nnz_in_tile;
    }

    void store_nnz(nnz_t val, int row_idx, int col_idx) {
        assert(this->trow + this->row_idx == row_idx);
        auto intile_col = col_idx - this->tcol; // column index in the tile
        store_nnz_internal(val, intile_col);
    }

    void write_last_zeroCount(uint8_t zeroCount) {
        auto [last_crow, last_crow_offset] = get_last_caddr();
        int last_crow_offset_half = last_crow_offset / 2; // need nibbles, not bytes
        bool upper = (last_crow_offset & 0x1) != 0;
        auto& zeroCounts = tile.content[last_crow].zeroCounts;
        if (!upper) {
            zeroCounts[last_crow_offset_half] = zeroCount;
        } else {
            zeroCounts[last_crow_offset_half] |= (zeroCount << 4); // store the upper half, was already initialized to 0 in lower write
        }
    }

    void store_nnz_internal(nnz_t val, int intile_col) {
        if (intile_col < 0 || intile_col > 31) {
            throw std::runtime_error("wth");
        }
        if (intile_col != 0 && last_intile_col_idx < 0) { // no entries written to the current row, but need a row-start
            store_nnz_internal(zero_nnz, 0);
        }
        auto zeroCount = intile_col - last_intile_col_idx - 1; // to consecutive cols have a zeroCount of 0 between them
        assert(zeroCount >= 0 && zeroCount <= 32);
        while (zeroCount > 15) { // need intermediary nnz to reach target col_idx
            auto intermediary_col = last_intile_col_idx + 16; // max dist
            store_nnz_internal(zero_nnz, intermediary_col);
            zeroCount -= 16;
        }

        if (last_intile_col_idx >= 0) { // Now we know there was at least 1 nnz written already and we know were we want to write, so can fill out last ones zeroCount
            // must not move after [reserve_entry()], as that would modify the nnz_count on which this relies
            write_last_zeroCount(zeroCount);
        }

        // write the actual value (and increment nnz_count)
        auto [crow_idx, crow_offset] = reserve_entry();
        tile.content[crow_idx].data[crow_offset] = val;

        this->last_intile_col_idx = intile_col;
    }

    void queue_tile_transfer(tt::tt_metal::IDevice *tt_device0, uint64_t mat_col_tile_count, std::shared_ptr<tt::tt_metal::Buffer> tt_buf_mat) {
        int trow_idx = this->trow / 32;
        int tcol_idx = this->tcol / 32;
        // std::cout << "Uploading tile [" << trow_idx << ", " << tcol_idx << "] to device." << std::endl;
        uint64_t address_in_dev = sizeof(tt_c_spmv_tile) * trow_idx + sizeof(tt_c_spmv_tile) * tcol_idx;

        // write the tile to the TT buffer
        tt::tt_metal::EnqueueWriteSubBuffer(
            tt_device0->command_queue(0),
            tt_buf_mat, &tile,
            {address_in_dev, sizeof(tt_c_spmv_tile)},
            false
        );

        event = std::make_shared<tt::tt_metal::Event>();
        tt::tt_metal::EnqueueRecordEvent( // we can wait on this event to complete and it should complete once the transmission completed. Could also be used for the device to start using it
            tt_device0->command_queue(0),
            event
        );

        clear(); // reset build state. Only event must be preserved until its done!
    }
};

void kernel_spmv(float *y, float *A, int *IA, int *JA, int M, const float *x) {
    tt::tt_metal::IDevice *tt_device0 = tt::tt_metal::CreateDevice(0);

    int N = M; // currently square. needed for how many tiles per row we need

    std::cout << M << " rows, " << IA[M] << " non-zero elements." << std::endl;

    auto h2d_start = std::chrono::high_resolution_clock::now();

    uint64_t matrix_tile_entries = 32 * 32; // 32x32 tile size
    uint64_t matrix_tile_bytes = matrix_tile_entries * sizeof(float);

    uint64_t vector_tile_entries = 32;
    uint64_t vector_tile_bytes = vector_tile_entries * sizeof(float); // TODO this is only the ratio, we may want to increase the minimum size

    uint64_t used_vec_size = M * sizeof(int);
    uint64_t aligned_vec_size = ((used_vec_size + (vector_tile_bytes - 1)) / vector_tile_bytes) * vector_tile_bytes;
    uint64_t aligned_usable_vec_size = aligned_vec_size != used_vec_size
                                              ? aligned_vec_size - vector_tile_bytes
                                              : aligned_vec_size;
    if (aligned_usable_vec_size < used_vec_size) {
        std::cerr <<
            "Warn: x/y is " << used_vec_size << " bytes, but can only transfer " << aligned_vec_size << " bytes to TT, due to alignment! Last values will be invalid!" << std::endl;
        exit(1);
    }

    // Transmit the x vector. Then operations may start as soon as the tile is available to the Tensix core
    std::shared_ptr<tt::tt_metal::Buffer> tt_buf_x = tt::tt_metal::CreateBuffer({ //TODO probably smaller tiles as less vector entries per input (matrix-rows per tile)
        .device = tt_device0,
        .size = static_cast<tt::tt_metal::DeviceAddr>(aligned_vec_size),
        .page_size = static_cast<tt::tt_metal::DeviceAddr>(vector_tile_bytes),
        .buffer_type = tt::tt_metal::BufferType::DRAM
    });

    // start transfer in background
    tt::tt_metal::EnqueueWriteBuffer(
        tt_device0->command_queue(0),
        tt_buf_x, x,
        false
    );

    // results will go here
    std::shared_ptr<tt::tt_metal::Buffer> tt_buf_y = tt::tt_metal::CreateBuffer({
        .device = tt_device0,
        .size = static_cast<tt::tt_metal::DeviceAddr>(aligned_vec_size),
        .page_size = static_cast<tt::tt_metal::DeviceAddr>(vector_tile_bytes),
        .buffer_type = tt::tt_metal::BufferType::DRAM
    });

    uint64_t mat_row_tile_count = ((M +31)/32); //  in the matrix, each tile contains 32 rows // TODO do we need multiple col-tiles?
    uint64_t mat_col_tile_count = ((N +31)/32); // in the matrix, each tile contains 32 cols

    // Buffer for tiles matrix in compressed format
    std::shared_ptr<tt::tt_metal::Buffer> tt_buf_mat = tt::tt_metal::CreateBuffer({
        .device = tt_device0,
        .size = static_cast<tt::tt_metal::DeviceAddr>(sizeof(tt_c_spmv_tile) * mat_col_tile_count * mat_row_tile_count),
        .page_size = static_cast<tt::tt_metal::DeviceAddr>(sizeof(tt_c_spmv_tile)), // lets see if it takes as a non-multiple of 32*32
        .buffer_type = tt::tt_metal::BufferType::DRAM
    });

    std::vector<tt_c_build_state> build_states(mat_col_tile_count); // buffer memory for build states, one per tile

    int tcol_idx;
    int trow_idx = 0;
    int tile_col_start;
    int end_tile_col_start = mat_col_tile_count * 32;


    for (int tile_row_start=0; tile_row_start < M; tile_row_start += tt_row_block_count) { // assume cols in natural order in each row
        tt_c_build_state* current_tile = nullptr;
        int tile_last_row_idx = std::min(tile_row_start + tt_row_block_count, M)-1; // last row index in A for this tile / end of Matrix

        // for every row inside the curent row of tiles (32)
        for (int row_idx = tile_row_start; row_idx <= tile_last_row_idx; ++row_idx) {
            tcol_idx = -1; // reset tile-column index for each tile-row
            tile_col_start = -32; // reset column to 0
            int next_tile_col_start = 0;

            int A_idx_row_start = IA[row_idx]; // start index in A for this tile
            int A_idx_row_next_start = IA[row_idx+1]; // first index in next row


            // iterates over all nnz inputs in the current row
            for (int A_idx = A_idx_row_start; A_idx < A_idx_row_next_start; ++A_idx) {

                int col_index = JA[A_idx]; // column index of the current non-zero element

                while (col_index >= next_tile_col_start) {
                    if (current_tile) {
                        current_tile->seal_row();
                        if (row_idx == tile_last_row_idx) { // current_tile was just completed
                            current_tile->queue_tile_transfer(tt_device0, mat_col_tile_count, tt_buf_mat);
                        }
                    }

                    // switch to the next tile in the row
                    tile_col_start += 32;
                    next_tile_col_start += 32;
                    ++tcol_idx;
                    current_tile = &build_states[tcol_idx];
                    current_tile->open(tile_row_start, tile_col_start);
                    current_tile->entered_row(row_idx);
                }

                float val = A[A_idx]; // value of the current non-zero element
                current_tile->store_nnz(val, row_idx, col_index);
            }
            // end any remaining tiles in this row

            while (tile_col_start < end_tile_col_start) {
                if (current_tile) {
                    current_tile->seal_row();
                    if (row_idx == tile_last_row_idx) { // current_tile was just completed
                        current_tile->queue_tile_transfer(tt_device0, mat_col_tile_count, tt_buf_mat);
                    }
                }
                tile_col_start += 32;
                if (tile_col_start < end_tile_col_start) {
                    ++tcol_idx;
                    current_tile = &build_states[tcol_idx];
                    current_tile->open(tile_row_start, tile_col_start);
                    current_tile->entered_row(row_idx);
                } else {
                    current_tile = nullptr; // no more tiles in this row
                }
            }
        }
        // we filled up a row of tiles!


        // for (int tile_idx = 0; tile_idx < mat_col_tile_count; ++tile_idx) {
        //     auto* state_final = &build_states[tile_idx];
        //     // reset for reuse
        //     state_final->clear();
        // }

        ++trow_idx;
    }

    uint64_t aligned_buf_size_A = ((used_vec_size + (matrix_tile_bytes - 1)) / matrix_tile_bytes) * matrix_tile_bytes;

    std::shared_ptr<tt::tt_metal::Buffer> tt_buf_A = tt::tt_metal::CreateBuffer({
        .device = tt_device0,
        .size = static_cast<tt::tt_metal::DeviceAddr>(aligned_buf_size_A),
        .page_size = static_cast<tt::tt_metal::DeviceAddr>(matrix_tile_bytes),
        .buffer_type = tt::tt_metal::BufferType::DRAM
    });

    std::shared_ptr<tt::tt_metal::Buffer> tt_buf_JA = tt::tt_metal::CreateBuffer({
        .device = tt_device0,
        .size = static_cast<tt::tt_metal::DeviceAddr>(aligned_buf_size_A),
        .page_size = static_cast<tt::tt_metal::DeviceAddr>(matrix_tile_bytes),
        .buffer_type = tt::tt_metal::BufferType::DRAM
    });

    std::shared_ptr<tt::tt_metal::Buffer> tt_buf_IA = tt::tt_metal::CreateBuffer({
        .device = tt_device0,
        .size = static_cast<tt::tt_metal::DeviceAddr>(aligned_vec_size),
        .page_size = static_cast<tt::tt_metal::DeviceAddr>(matrix_tile_bytes),
        .buffer_type = tt::tt_metal::BufferType::DRAM
    });

    tt::tt_metal::Synchronize(tt_device0);

    auto h2d_end = std::chrono::high_resolution_clock::now();

    auto h2d_time = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(h2d_end - h2d_start).count());

    std::cout << "H2D transfer time: " << h2d_time << " ms" << std::endl;
    auto kernel_start = std::chrono::high_resolution_clock::now();
    tt::tt_metal::Program tt_program_1;
    // tt::tt_metal::CoreCoord tt_full_grid_1 = tt_device0->compute_with_storage_grid_size();
    tt::tt_metal::CoreCoord tt_one_core_1({0, 0});
    //    auto tt_cb_1_0_config = tt::tt_metal::CircularBufferConfig(8192, {{0, tt::DataFormat::UInt32}})
    //    	.set_page_size(0, 4096);
    //    auto tt_cb_1_0 = tt::tt_metal::CreateCircularBuffer(tt_program_1, tt_one_core_1, tt_cb_1_0_config);
    //    auto tt_cb_1_1_config = tt::tt_metal::CircularBufferConfig(8192, {{1, tt::DataFormat::UInt32}})
    //    	.set_page_size(1, 4096);
    //    auto tt_cb_1_1 = tt::tt_metal::CreateCircularBuffer(tt_program_1, tt_one_core_1, tt_cb_1_1_config);
    //
    //    auto daisy_tt_comp_kid_1 = tt::tt_metal::CreateKernel(tt_program_1, "/home/ramon/git/docc/tests/spmv-tt/tenstorrent_kernel_1.comp.cpp", tt_one_core_1, tt::tt_metal::ComputeConfig { .compile_args = {} });
    //    tt::tt_metal::SetRuntimeArgs(tt_program_1, daisy_tt_comp_kid_1, tt_one_core_1, {});
    //    auto daisy_tt_movRd_kid_1 = tt::tt_metal::CreateKernel(tt_program_1, "/home/ramon/git/docc/tests/spmv-tt/extract/tenstorrent_kernel_1.movRd.cpp", tt_one_core_1, tt::tt_metal::ReaderDataMovementConfig({}, {}));
    //    tt::tt_metal::SetRuntimeArgs(tt_program_1, daisy_tt_movRd_kid_1, tt_one_core_1, {tt_buf_0->address()});
    //    auto daisy_tt_movWr_kid_1 = tt::tt_metal::CreateKernel(tt_program_1, "/home/ramon/git/docc/tests/spmv-tt/extract/tenstorrent_kernel_1.movWr.cpp", tt_one_core_1, tt::tt_metal::WriterDataMovementConfig({}, {}));
    //    tt::tt_metal::SetRuntimeArgs(tt_program_1, daisy_tt_movWr_kid_1, tt_one_core_1, {tt_buf_0->address()});
    //    std::cout << "Dispatching tt_1" << std::endl;
    //    tt::tt_metal::EnqueueProgram(tt_device0->command_queue(0), tt_program_1, false);
    //
    auto kernel_end = std::chrono::high_resolution_clock::now();
    auto kernel_time = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(kernel_end - kernel_start).count());
    std::cout << "TT Kernel time: " << kernel_time << " ms" << std::endl;


    auto d2h_start = std::chrono::high_resolution_clock::now();
    tt::tt_metal::EnqueueReadBuffer(
        tt_device0->command_queue(0),
        tt_buf_y, y,
        true
    );
    auto d2h_end = std::chrono::high_resolution_clock::now();

    auto d2h_time = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(d2h_end - d2h_start).count());

    std::cout << "D2H transfer time: " << d2h_time << " ms" << std::endl;

    tt::tt_metal::CloseDevice(tt_device0);
}

void kernel_org_data_spmv(float *y, float *A, int *IA, int *JA, int M, const float *x) {
    tt::tt_metal::IDevice *tt_device0 = tt::tt_metal::CreateDevice(0);

    std::cout << M << " rows, " << IA[M] << " non-zero elements." << std::endl;

    auto h2d_start = std::chrono::high_resolution_clock::now();

    int lastExValIdx = IA[M];
    int valCount = lastExValIdx;
    uint64_t tile_size_bytes = 32 * 32 * sizeof(float);

    assert(sizeof(int) == sizeof(float));

    uint64_t used_buf_size = valCount * sizeof(float); // rely on every input buffer being allocated to allow r/w up to a multiple of 4096!!
    uint64_t aligned_buf_size_A = ((used_buf_size + (tile_size_bytes - 1)) / tile_size_bytes) * tile_size_bytes;

    std::shared_ptr<tt::tt_metal::Buffer> tt_buf_A = tt::tt_metal::CreateBuffer({
        .device = tt_device0,
        .size = static_cast<tt::tt_metal::DeviceAddr>(aligned_buf_size_A),
        .page_size = static_cast<tt::tt_metal::DeviceAddr>(tile_size_bytes),
        .buffer_type = tt::tt_metal::BufferType::DRAM
    });

    std::shared_ptr<tt::tt_metal::Buffer> tt_buf_JA = tt::tt_metal::CreateBuffer({
        .device = tt_device0,
        .size = static_cast<tt::tt_metal::DeviceAddr>(aligned_buf_size_A),
        .page_size = static_cast<tt::tt_metal::DeviceAddr>(tile_size_bytes),
        .buffer_type = tt::tt_metal::BufferType::DRAM
    });

    used_buf_size = M * sizeof(int);
    uint64_t aligned_buf_size_IA = ((used_buf_size + (tile_size_bytes - 1)) / tile_size_bytes) * tile_size_bytes;
    uint64_t aligned_usable_buf_size_IA = aligned_buf_size_IA != used_buf_size
                                              ? aligned_buf_size_IA - tile_size_bytes
                                              : aligned_buf_size_IA;
    if (aligned_usable_buf_size_IA < used_buf_size) {
        std::cerr <<
            "Warn: IA/x/y is " << used_buf_size << " bytes, but can only transfer " << aligned_buf_size_IA << " bytes to TT, due to alignment! Last values will be invalid!" << std::endl;
    }

    std::shared_ptr<tt::tt_metal::Buffer> tt_buf_IA = tt::tt_metal::CreateBuffer({
        .device = tt_device0,
        .size = static_cast<tt::tt_metal::DeviceAddr>(aligned_buf_size_IA),
        .page_size = static_cast<tt::tt_metal::DeviceAddr>(tile_size_bytes),
        .buffer_type = tt::tt_metal::BufferType::DRAM
    });

    std::shared_ptr<tt::tt_metal::Buffer> tt_buf_x = tt::tt_metal::CreateBuffer({
        .device = tt_device0,
        .size = static_cast<tt::tt_metal::DeviceAddr>(aligned_buf_size_IA),
        .page_size = static_cast<tt::tt_metal::DeviceAddr>(tile_size_bytes),
        .buffer_type = tt::tt_metal::BufferType::DRAM
    });

    std::shared_ptr<tt::tt_metal::Buffer> tt_buf_y = tt::tt_metal::CreateBuffer({
        .device = tt_device0,
        .size = static_cast<tt::tt_metal::DeviceAddr>(aligned_buf_size_IA),
        .page_size = static_cast<tt::tt_metal::DeviceAddr>(tile_size_bytes),
        .buffer_type = tt::tt_metal::BufferType::DRAM
    });


    // for (int i = 0; i < M; ++i) {
    //     for (int j = IA[i]; j < IA[i + 1]; ++j) {
    //         int colIdx = JA[j];
    //
    //         y[i] = A[j] * x[colIdx];
    //     }
    // }


    tt::tt_metal::EnqueueWriteSubBuffer(
        tt_device0->command_queue(0),
        tt_buf_A, A,
        {0, aligned_buf_size_A},
        false
    );
    tt::tt_metal::EnqueueWriteSubBuffer(
        tt_device0->command_queue(0),
        tt_buf_JA, JA,
        {0, aligned_buf_size_A},
        false
    );
    tt::tt_metal::EnqueueWriteSubBuffer(
        tt_device0->command_queue(0),
        tt_buf_IA, IA,
        {0, aligned_buf_size_IA},
        false
    );
    tt::tt_metal::EnqueueWriteSubBuffer(
        tt_device0->command_queue(0),
        tt_buf_x, x,
        {0, aligned_buf_size_IA},
        true
    );
    auto h2d_end = std::chrono::high_resolution_clock::now();

    auto h2d_time = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(h2d_end - h2d_start).count());

    std::cout << "H2D transfer time: " << h2d_time << " ms" << std::endl;
    auto kernel_start = std::chrono::high_resolution_clock::now();
    tt::tt_metal::Program tt_program_1;
    // tt::tt_metal::CoreCoord tt_full_grid_1 = tt_device0->compute_with_storage_grid_size();
    tt::tt_metal::CoreCoord tt_one_core_1({0, 0});
    //    auto tt_cb_1_0_config = tt::tt_metal::CircularBufferConfig(8192, {{0, tt::DataFormat::UInt32}})
    //    	.set_page_size(0, 4096);
    //    auto tt_cb_1_0 = tt::tt_metal::CreateCircularBuffer(tt_program_1, tt_one_core_1, tt_cb_1_0_config);
    //    auto tt_cb_1_1_config = tt::tt_metal::CircularBufferConfig(8192, {{1, tt::DataFormat::UInt32}})
    //    	.set_page_size(1, 4096);
    //    auto tt_cb_1_1 = tt::tt_metal::CreateCircularBuffer(tt_program_1, tt_one_core_1, tt_cb_1_1_config);
    //
    //    auto daisy_tt_comp_kid_1 = tt::tt_metal::CreateKernel(tt_program_1, "/home/ramon/git/docc/tests/spmv-tt/tenstorrent_kernel_1.comp.cpp", tt_one_core_1, tt::tt_metal::ComputeConfig { .compile_args = {} });
    //    tt::tt_metal::SetRuntimeArgs(tt_program_1, daisy_tt_comp_kid_1, tt_one_core_1, {});
    //    auto daisy_tt_movRd_kid_1 = tt::tt_metal::CreateKernel(tt_program_1, "/home/ramon/git/docc/tests/spmv-tt/extract/tenstorrent_kernel_1.movRd.cpp", tt_one_core_1, tt::tt_metal::ReaderDataMovementConfig({}, {}));
    //    tt::tt_metal::SetRuntimeArgs(tt_program_1, daisy_tt_movRd_kid_1, tt_one_core_1, {tt_buf_0->address()});
    //    auto daisy_tt_movWr_kid_1 = tt::tt_metal::CreateKernel(tt_program_1, "/home/ramon/git/docc/tests/spmv-tt/extract/tenstorrent_kernel_1.movWr.cpp", tt_one_core_1, tt::tt_metal::WriterDataMovementConfig({}, {}));
    //    tt::tt_metal::SetRuntimeArgs(tt_program_1, daisy_tt_movWr_kid_1, tt_one_core_1, {tt_buf_0->address()});
    //    std::cout << "Dispatching tt_1" << std::endl;
    //    tt::tt_metal::EnqueueProgram(tt_device0->command_queue(0), tt_program_1, false);
    //
    auto kernel_end = std::chrono::high_resolution_clock::now();
    auto kernel_time = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(kernel_end - kernel_start).count());
    std::cout << "TT Kernel time: " << kernel_time << " ms" << std::endl;


    auto d2h_start = std::chrono::high_resolution_clock::now();
    tt::tt_metal::EnqueueReadBuffer(
        tt_device0->command_queue(0),
        tt_buf_y, y,
        true
    );
    auto d2h_end = std::chrono::high_resolution_clock::now();

    auto d2h_time = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(d2h_end - d2h_start).count());

    std::cout << "D2H transfer time: " << d2h_time << " ms" << std::endl;

    tt::tt_metal::CloseDevice(tt_device0);
}
