// ---------------------------------------------------------------------
// Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause
// ---------------------------------------------------------------------

#pragma once
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <cstdint>
#include <map>
#include <memory>

#ifndef float32_t
using float32_t = float;
#endif

// tensor spec
#define TS_EMBEDDING_ELEMENT_COUNT (1 * 1280)
#define CONST_TEXT_EMBEDDING_COUNT_SD_1_5 (1 * 77 * 768)
#define CONST_TEXT_EMBEDDING_COUNT_SD_2_1 (1 * 77 * 1024)
// the target order is nhwc
#define LATENT_ELEMENT_COUNT (1 * 64 * 64 * 4)

#define LATENT_BIN_FILE_EXT ".rand"
#define TIME_STEP_EMBEDDING_BIN_FILE_EXT ".ts"
#define CONST_TEXT_EMBEDDING_BIN_FILE_EXT ".cte"

// loader read environment variable to get tar file path
// if that is not defined, it uses a hard coded default path
// #define ENV_VAR_SD_TAR_FILE "SD_TAR_FILE"
#define DEFAULT_TAR_FILE_PATH "sd_precomute_data.tar"

// flatted tensor data,
using tensor_data_float32_t = std::vector<float32_t>;

struct FileParser
{
    virtual ~FileParser(){};
    virtual bool parse(std::ifstream &fin, size_t file_size) = 0;
};

class DataLoader
{
public:
    DataLoader();
    ~DataLoader();
    // load data from files, this must be called first
    // load read data from a tar file defined by enviroment variable "SD_TAR_FILE"

    bool load(char *file_name = nullptr, std::string model_version=NULL);

    // get a list of supported number of steps

    void get_supported_num_steps(std::vector<int32_t> &step_seq) const;

    // what is the current number of steps used?

    int32_t get_current_num_steps() const
    {
        return cur_num_steps_;
    }

    // set number of steps for denoising process
    // the number must be set before denoising starts, and
    // cannot change in the middle

    bool set_num_steps(int32_t steps);

    // get the time step embedding pointed by step_index
    // each embedding is a 2-d tensor of (1x1280) stored in C major order
    // returns:
    //  ts_embedding_ptr, the caller should not do delete on it

    bool get_ts_embedding(uint32_t step_index,
                          const tensor_data_float32_t *&t4_ts_embedding_ptr);

    // get the time step  pointed by step_index

    bool get_time_step(uint32_t step_index, int32_t &t13_time_step);

    // get the time step from user provided time steps
    bool get_time_steps(const std::vector<int32_t> *&t9_time_steps_ptr);

    // how many precomputed random intialized latents?

    int32_t get_num_initial_latents() const;

    void get_seed_list(std::vector<int32_t> &seed_list);

    // get the random initial latent pointed by seed_index
    // each latent is a 4-d tensor of (1x64x64x4) stored in C major order
    // returns:
    //   latent_ptr, the caller should not do delete on it,

    bool get_random_init_latent(uint32_t seed_index,
                                int32_t &seed,
                                const tensor_data_float32_t *&t10_latent_ptr);

    // get the random initial latent pointed by seed
    // each latent is a 4-d tensor of (1x64x64x4) stored in C major order
    // returns:
    //   latent_ptr, the caller should not do delete on it

    bool get_random_init_latent(int32_t seed,
                                const tensor_data_float32_t *&t10_latent_ptr);

    // get offline precomputed text embedding from input of [""] to text encoder
    // [""] length is 1 due to batch-size-1 UNET inference, the size of the data is 1x77x768 for v1.5 and 1x77x1024 for v2.1
    // returns:
    //  text_embedding_ptr, the caller should not do delete on it

    bool get_unconditional_text_embedding(const tensor_data_float32_t *&t3_text_embedding_ptr);

    // for debugging purpose

    void print(std::stringstream &os, uint32_t first_n_elem = 8);

private:
    bool loaded_;
    int32_t cur_num_steps_;
    std::unique_ptr<FileParser> latent_parser_ptr_;
    std::unique_ptr<FileParser> ts_embedding_parser_ptr_;
    std::unique_ptr<FileParser> const_text_embedding_parser_ptr_;
};
