// ---------------------------------------------------------------------
// Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause
// ---------------------------------------------------------------------

#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <unordered_map>
#include <vector>

#include "Helpers.hpp"
#include "DataLoader.h"

#ifdef WIN32
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

#define MY_ASSERT2(expr, f, l) "Assertion failed:" #expr " at " #f #l
#define MY_ASSERT(expr, format, ...) do { auto e = (expr); if (!e) {           \
    DEMO_ERROR("Assertion '%s' failed at %s:%d %s", #expr, __FILE__, __LINE__, __PRETTY_FUNCTION__); \
    DEMO_ERROR(format, ##__VA_ARGS__);                                            \
    throw std::runtime_error(MY_ASSERT2(#expr, __FILE__, __LINE__)); \
}} while(0)

// tensor spec
#define TS_EMBEDDING_ELEMENT_COUNT ( 1 * 1280)
#define CONST_TEXT_EMBEDDING_COUNT ( 1 * 77 * 768)
// the target order is nhwc
#define LATENT_ELEMENT_COUNT ( 1*64*64*4)

#define LATENT_BIN_FILE_EXT ".rand"
#define TIME_STEP_EMBEDDING_BIN_FILE_EXT ".ts"
#define CONST_TEXT_EMBEDDING_BIN_FILE_EXT ".cte"

// loader read environment variable to get tar file path
// if that is not defined, it uses a hard coded default path
// #define ENV_VAR_SD_TAR_FILE "SD_TAR_FILE"
#define DEFAULT_TAR_FILE_PATH "sd_precomute_data.tar"

template <typename T>
static void dump_tensor(std::stringstream& ss, std::vector<T>& tensor_data, int count)
{
    int num = std::min<int>(count, int(tensor_data.size()));
    ss << " num_elem: " << tensor_data.size();
    if (num > 0)
        ss << ", " << sizeof(T) << "-bytes-each[0.." << num - 1 << "]:";
    ss << std::fixed << std::setw(11) << std::setprecision(9)
        << std::setfill('0');
    for (int i = 0; i < num; i++)
    {
        ss << tensor_data[i] << ", ";
    }
}

class TarLoader
{

public:

    explicit TarLoader(const std::string& tar_name)
        : tar_name_(tar_name), tar_(tar_name, std::ios::binary)
    {
        init();
    }

    void register_parser(const std::string& file_ext, FileParser* parser_ptr)
    {
        parsers_[file_ext] = parser_ptr;
    }

    void init()
    {
        MY_ASSERT(tar_.is_open(), "Error in opening %s", tar_name_.c_str());

        auto const TAR_HEADER_FIELD_NAME_POS = 0;
        auto const TAR_HEADER_FIELD_SIZE_POS = 124;
        auto const TAR_HEADER_FIELD_TYPEFLAG_POS = 156;
        auto const BLOCKSIZE = 512;
        char hdr[BLOCKSIZE];

        Files bin_files;
        Files text_files;
        bool is_text_file = false;
        while (tar_.read(hdr, BLOCKSIZE) && hdr[0] != 0) {
            is_text_file = false;
            MY_ASSERT(hdr[TAR_HEADER_FIELD_TYPEFLAG_POS] == '0', "Unsupported typeflag:%c", hdr[TAR_HEADER_FIELD_TYPEFLAG_POS]);

            std::string filename{ hdr + TAR_HEADER_FIELD_NAME_POS };

            if (filename.size() >= 4 && filename.compare(filename.size() - 4, 4, ".txt") == 0)
            {
                is_text_file = true;
            }

            size_t offset = tar_.tellg();
            uint32_t size = std::stoi(hdr + TAR_HEADER_FIELD_SIZE_POS, nullptr, 8);
            if (is_text_file)
            {
                text_files.emplace(filename, std::pair<size_t, uint32_t>(offset, size));
                DEMO_DEBUG("text_files[%zu]: %s, offset:%zu, size:%u",
                    text_files.size() - 1, filename.c_str(), offset, size);
            }
            else
            {
                bin_files.emplace(filename, std::pair<size_t, uint32_t>(offset, size));
                DEMO_DEBUG("binary_files[%zu]: %s, offset:%zu, size:%u",
                    bin_files.size() - 1, filename.c_str(), offset, size);
            }
            auto seek = (size + BLOCKSIZE - 1) / BLOCKSIZE * BLOCKSIZE;
            tar_.seekg(seek, std::ios_base::cur);
        }

        bin_files_.swap(bin_files);
        for (auto it = text_files.begin(); it != text_files.end(); ++it)
        {
            DEMO_DEBUG("Dump text file %s ...", it->first.c_str());
            auto offset_size_pair = it->second;
            auto offset = offset_size_pair.first;
            auto size = offset_size_pair.second;
            tar_.seekg(offset);
            std::vector<char> text;
            text.resize(size + 1);
            tar_.read(reinterpret_cast<char*>(text.data()), size);
            text[size] = '\0';
            DEMO_DEBUG("%s", text.data());
        }
    }

    bool parse()
    {
        bool ret = true;
        for (auto file_iter = bin_files_.begin(); file_iter != bin_files_.end(); ++file_iter)
        {
            bool good = false;
            auto file_name = file_iter->first;
            for (auto parser_iter = parsers_.begin(); parser_iter != parsers_.end(); ++parser_iter)
            {
                auto ext_str = parser_iter->first;
                size_t ext_len = ext_str.size();
                if (file_name.size() >= ext_len && file_name.compare(file_name.size() - ext_len, ext_len, ext_str) == 0)
                {
                    auto offset_size_pair = file_iter->second;
                    auto offset = offset_size_pair.first;
                    auto size = offset_size_pair.second;
                    tar_.seekg(offset);
                    auto parser_ptr = parser_iter->second;
                    DEMO_DEBUG("Invoke parser for %s", file_name.c_str());
                    good = parser_ptr->parse(tar_, size);
                    ret = ret && good;
                }
            }
            if (!good)
                DEMO_ERROR("parsing %s failed or there is no parser for it", file_name.c_str());
        }
        return ret;
    }

private:
    const std::string tar_name_;
    std::ifstream tar_;

    // name:pair<offset,size>
    using Files = std::unordered_map<std::string, std::pair<size_t, uint32_t>>;
    Files bin_files_;
    std::unordered_map<std::string, FileParser*> parsers_;
};

class LatentParser : public FileParser
{
public:
    LatentParser() {}
    std::vector<int32_t> seed_seq_;
    std::vector<tensor_data_float32_t> latent_seq_;
    bool parse(std::ifstream& fin, size_t file_size)  override
    {
        int32_t count = 0;
        size_t total_read = 0;
        fin.read(reinterpret_cast<char*>(&count), sizeof(int32_t));
        total_read += sizeof(int32_t);
        int32_t seed = 0;
        for (int32_t i = 0; i < count; i++)
        {
            if (fin)
                fin.read(reinterpret_cast<char*>(&seed), sizeof(int32_t));
            if (fin)
            {
                total_read += sizeof(int32_t);
                seed_seq_.push_back(seed);
                DEMO_DEBUG("load seed[%d] %d ", i, seed);
            }

        }
        for (int32_t i = 0; i < count; i++)
        {
            tensor_data_float32_t latent;
            latent.resize(LATENT_ELEMENT_COUNT);
            if (fin)
                fin.read(reinterpret_cast<char*>(latent.data()),
                    sizeof(float32_t) * LATENT_ELEMENT_COUNT);
            if (fin)
            {
                total_read += sizeof(float32_t) * LATENT_ELEMENT_COUNT;
                latent_seq_.push_back(latent);
                DEMO_DEBUG("load random_init_latent[%d]", i);
                std::stringstream os;
                dump_tensor<float32_t>(os, latent, 8);
                DEMO_DEBUG("  %s", os.str().c_str());
            }

        }
        if (total_read != file_size)
        {
            DEMO_ERROR("load latent error, size mismatch: %zu vs %zu", total_read, file_size);
            return false;
        }
        return true;
    }
    virtual ~LatentParser() {}
};

class TsEmbeddingParser : public FileParser
{
public:
    TsEmbeddingParser() {}
    std::unordered_map<int32_t, std::vector<int32_t>> ts_seq_map_;
    std::unordered_map<int32_t, std::vector<tensor_data_float32_t>> ts_embedding_seq_map_;

    bool parse(std::ifstream& fin, size_t file_size)  override
    {
        int32_t steps = 0;
        size_t total_read = 0;
        if (fin)
        {
            fin.read(reinterpret_cast<char*>(&steps), sizeof(int32_t));
        }
        if (fin)
        {
            total_read += sizeof(int32_t);
            ts_seq_map_.emplace(std::pair<int32_t, std::vector<int32_t>>(steps, {}));
            ts_seq_map_[steps].resize(steps);

            fin.read(reinterpret_cast<char*>(ts_seq_map_[steps].data()), sizeof(int32_t) * steps);
        }
        if (fin)
        {
            total_read += sizeof(int32_t) * steps;
            ts_embedding_seq_map_.emplace(std::pair<int32_t, std::vector<tensor_data_float32_t>>(steps, {}));
            ts_embedding_seq_map_[steps].resize(steps);

            for (int32_t idx = 0; idx < steps; idx++)
            {
                ts_embedding_seq_map_[steps][idx].resize(TS_EMBEDDING_ELEMENT_COUNT);
                if (fin)
                {
                    fin.read(reinterpret_cast<char*>(ts_embedding_seq_map_[steps][idx].data()),
                        sizeof(float32_t) * TS_EMBEDDING_ELEMENT_COUNT);
                }
                if (fin)
                {
                    total_read += sizeof(float32_t) * TS_EMBEDDING_ELEMENT_COUNT;
                    DEMO_DEBUG("load ts_embedding %d/%d steps", idx + 1, steps);
                    std::stringstream os;
                    dump_tensor<float32_t>(os, ts_embedding_seq_map_[steps][idx], 8);
                    DEMO_DEBUG("  %s", os.str().c_str());
                }
            }
        }
        if (total_read != file_size)
        {
            DEMO_ERROR("load ts_embedding error, size mismatch: %zu vs %zu", total_read, file_size);
            return false;
        }
        return true;
    }

    virtual ~TsEmbeddingParser() {}
};

template <typename T>
class TensorParser : public FileParser
{
public:
    std::vector<T> tensor_data_;
    size_t num_elements_;
    std::string name_;

    TensorParser(const std::string& name, size_t num_elements)
        :num_elements_(num_elements), name_(name)
    {}

    bool parse(std::ifstream& fin, size_t file_size)  override
    {
        if (fin)
        {
            tensor_data_.resize(num_elements_);
            fin.read(reinterpret_cast<char*>(tensor_data_.data()),
                sizeof(T) * num_elements_);
        }
        if (fin)
        {
            DEMO_DEBUG("load %s size %zu", name_.c_str(), num_elements_);
            std::stringstream os;
            dump_tensor<T>(os, tensor_data_, 8);
            DEMO_DEBUG("  %s", os.str().c_str());
            return true;
        }
        else
        {
            DEMO_ERROR("load %s failed, cannot read %zu elements", name_.c_str(), num_elements_);
            return false;
        }
    }
    virtual ~TensorParser() {}
};


DataLoader::DataLoader()
    :loaded_(false), cur_num_steps_(20)
{

}

DataLoader::~DataLoader()
{

}


bool DataLoader::load(char* file_name)
{
    loaded_ = false;
    const char* default_tar_file_name = DEFAULT_TAR_FILE_PATH;

    if (file_name == nullptr)
    {
        file_name = (char*)default_tar_file_name;
    }

    DEMO_DEBUG("load %s ...", file_name);
    TarLoader tar_loader(file_name);

    // register parsers
    std::unique_ptr<FileParser> temp(new LatentParser());
    latent_parser_ptr_ = std::move(temp);
    tar_loader.register_parser(LATENT_BIN_FILE_EXT, latent_parser_ptr_.get());

    std::unique_ptr<FileParser> temp2(new TsEmbeddingParser());
    ts_embedding_parser_ptr_ = std::move(temp2);
    tar_loader.register_parser(TIME_STEP_EMBEDDING_BIN_FILE_EXT, ts_embedding_parser_ptr_.get());

    std::unique_ptr<FileParser> temp3(new TensorParser<float32_t>("const text embedding", CONST_TEXT_EMBEDDING_COUNT));
    const_text_embedding_parser_ptr_ = std::move(temp3);
    tar_loader.register_parser(CONST_TEXT_EMBEDDING_BIN_FILE_EXT, const_text_embedding_parser_ptr_.get());

    loaded_ = tar_loader.parse();
    if (loaded_)
        cur_num_steps_ = dynamic_cast<TsEmbeddingParser*>(ts_embedding_parser_ptr_.get())->ts_seq_map_.begin()->first;
    return loaded_;
}

void DataLoader::get_supported_num_steps(std::vector<int32_t>& step_seq) const
{
    step_seq.clear();
    for (auto const& element : dynamic_cast<TsEmbeddingParser*>(ts_embedding_parser_ptr_.get())->ts_embedding_seq_map_)
    {
        step_seq.push_back(element.first);
    }

}
bool DataLoader::set_num_steps(int32_t steps)
{
    if (!loaded_)
        return false;

    auto& ts_dict = dynamic_cast<TsEmbeddingParser*>(ts_embedding_parser_ptr_.get())->ts_seq_map_;
    if (ts_dict.find(steps) == ts_dict.end())
    {
        DEMO_ERROR("Set number of steps failed, %d is not supported, stay with %d",
            steps, cur_num_steps_);
        return false;
    }
    cur_num_steps_ = steps;
    DEMO_DEBUG("Set number of steps to %d", steps);
    return true;
}

void DataLoader::get_seed_list(std::vector<int32_t>& seed_list)
{
    auto& v = dynamic_cast<LatentParser*>(latent_parser_ptr_.get())->seed_seq_;
    seed_list = v;
}

int32_t DataLoader::get_num_initial_latents() const
{
    auto& latent_seq = dynamic_cast<LatentParser*>(latent_parser_ptr_.get())->latent_seq_;
    return int32_t(latent_seq.size());
}

bool DataLoader::get_random_init_latent(uint32_t seed_index,
    int32_t& seed,
    const tensor_data_float32_t*& t10_latent_ptr)
{
    if (!loaded_)
        return false;
    auto& latent_seq = dynamic_cast<LatentParser*>(latent_parser_ptr_.get())->latent_seq_;
    auto& seed_seq = dynamic_cast<LatentParser*>(latent_parser_ptr_.get())->seed_seq_;
    MY_ASSERT(seed_index < latent_seq.size(), "index %u too large", seed_index);
    seed_index = std::min<uint32_t>(seed_index, uint32_t(latent_seq.size() - 1));
    seed = seed_seq[seed_index];
    t10_latent_ptr = &(latent_seq[seed_index]);
    return true;
}

bool DataLoader::get_random_init_latent(int32_t seed,
    const tensor_data_float32_t*& t10_latent_ptr)
{
    auto& latent_seq = dynamic_cast<LatentParser*>(latent_parser_ptr_.get())->latent_seq_;
    auto& seed_seq = dynamic_cast<LatentParser*>(latent_parser_ptr_.get())->seed_seq_;

    for (uint32_t i = 0; i < seed_seq.size(); i++)
    {
        if (seed_seq[i] == seed)
        {
            t10_latent_ptr = &(latent_seq[i]);
            return true;
        }
    }
    MY_ASSERT(false, "seed %d not found", seed);
    return false;
}

bool DataLoader::get_ts_embedding(uint32_t step_index,
    const tensor_data_float32_t*& t4_ts_embedding_ptr)
{
    if (!loaded_)
        return false;
    auto& ts_embedding_dict = dynamic_cast<TsEmbeddingParser*>(ts_embedding_parser_ptr_.get())->ts_embedding_seq_map_;
    MY_ASSERT(step_index < ts_embedding_dict[cur_num_steps_].size(), "step_index %u out of boundary", step_index);
    step_index = std::min<uint32_t>(step_index, uint32_t(ts_embedding_dict[cur_num_steps_].size() - 1));
    t4_ts_embedding_ptr = &(ts_embedding_dict[cur_num_steps_][step_index]);

    return true;
}

bool DataLoader::get_time_step(uint32_t step_index, int32_t& t13_time_step)
{
    if (!loaded_)
        return false;
    auto& ts_dict = dynamic_cast<TsEmbeddingParser*>(ts_embedding_parser_ptr_.get())->ts_seq_map_;
    MY_ASSERT(step_index < ts_dict[cur_num_steps_].size(), "step_size %u out of bound", step_index);
    step_index = std::min<uint32_t>(step_index, uint32_t(ts_dict[cur_num_steps_].size() - 1));
    t13_time_step = ts_dict[cur_num_steps_][step_index];
    return true;
}

bool DataLoader::get_time_steps(const std::vector<int32_t>*& t9_time_steps_ptr)
{
    if (!loaded_)
        return false;
    auto& ts_dict = dynamic_cast<TsEmbeddingParser*>(ts_embedding_parser_ptr_.get())->ts_seq_map_;
    t9_time_steps_ptr = &(ts_dict[cur_num_steps_]);
    return true;
}


bool DataLoader::get_unconditional_text_embedding(const tensor_data_float32_t*& t3_text_embedding_ptr)
{
    if (!loaded_)
        return false;
    auto& v = dynamic_cast<TensorParser<float32_t>*>(const_text_embedding_parser_ptr_.get())->tensor_data_;
    t3_text_embedding_ptr = &(v);
    return true;
}

// for debugging purpose
void DataLoader::print(std::stringstream& os, uint32_t first_n_elem)
{
    auto& ts_dict = dynamic_cast<TsEmbeddingParser*>(ts_embedding_parser_ptr_.get())->ts_seq_map_;
    auto& ts_embedding_dict = dynamic_cast<TsEmbeddingParser*>(ts_embedding_parser_ptr_.get())->ts_embedding_seq_map_;
    auto& latent_seq = dynamic_cast<LatentParser*>(latent_parser_ptr_.get())->latent_seq_;
    auto& seed_seq = dynamic_cast<LatentParser*>(latent_parser_ptr_.get())->seed_seq_;
    auto& const_text_embedding = dynamic_cast<TensorParser<float32_t>*>(const_text_embedding_parser_ptr_.get())->tensor_data_;
    if (!loaded_)
    {
        os << "[loaded: false]";
        return;
    }
    for (auto map_iter = ts_dict.begin(); map_iter != ts_dict.end(); ++map_iter)
    {
        auto num_steps = map_iter->first;
        auto ts_seq = map_iter->second;
        auto ts_embedding_seq = ts_embedding_dict[num_steps];
        os << "NUM_STEPS: " << num_steps << std::endl;
        for (int i = 0; i < num_steps; i++)
        {
            os << "  step " << i + 1 << "/" << num_steps << ": ts " << ts_seq[i] << std::endl;
            os << "    ";
            dump_tensor<float32_t>(os, ts_embedding_seq[i], first_n_elem);
            os << std::endl;
        }
    }
    auto count = seed_seq.size();
    os << "NUM_SEEDS: " << count << std::endl;
    for (uint32_t i = 0; i < count; i++)
    {
        os << "  SEED[" << i << "]: " << seed_seq[i];
        dump_tensor<float32_t>(os, latent_seq[i], first_n_elem);
        os << std::endl;
    }
    os << "CONST_TEXT_EMBEDDING for constant prompt [\"\"] :";
    dump_tensor<float32_t>(os, const_text_embedding, first_n_elem);
}
