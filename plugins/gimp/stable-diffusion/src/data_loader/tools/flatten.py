#!/usr/bin/python3
"""
Copyright (c) 2024, Qualcomm Innovation Center, Inc. All rights reserved.

SPDX-License-Identifier: BSD-3-Clause
"""

import argparse
from datetime import datetime
import pickle
import copy
import sys
import os
import numpy as np
import pprint

pp = pprint.PrettyPrinter(indent=4)


def check_shape_type(name, tensor_np, expected_shape, expected_np_type):
    if tensor_np.shape != expected_shape:
        raise Exception(f"{name} shape is {tensor_np.shape}, expect {expected_shape}")
    if tensor_np.dtype != expected_np_type:
        raise Exception(f"{name} is {tensor_np.dtype}, expect {expected_np_type}")


def extract_data(ts_dict):
    iter = ts_dict["iteration"]
    if not "time_step" in ts_dict:
        raise Exception(f'Picke file parsing error: key "time_step" is not found')
    ts_np = ts_dict["time_step"].astype(np.int32)
    print(f"iteration {iter}, time_step: {ts_np}")

    if not "timeembedding" in ts_dict:
        raise Exception(f'Picke file parsing error: key "timeembedding" is not found')

    ts_embed_np = ts_dict["timeembedding"]
    print(
        f"iteration {iter}, timeembedding: ",
        type(ts_embed_np),
        ts_embed_np.shape,
        ts_embed_np.dtype,
        ts_embed_np[0, 0:4],
    )
    check_shape_type("timeembedding", ts_embed_np, (1, 1280), np.float32)
    if not "random_init" in ts_dict:
        raise Exception(f'Picke file parsing error: key "random_init" is not found')

    rad = ts_dict["random_init"]
    if not "seed" in rad:
        raise Exception(f'Picke file parsing error: key "seed" is not found')
    if not "latent_vector" in rad:
        raise Exception(f'Picke file parsing error: key "latent_vector" is not found')

    seed = rad["seed"]
    lv_np = rad["latent_vector"]
    print("random_init.latent_vector_nchw", type(lv_np), lv_np.shape, lv_np.dtype)
    check_shape_type("random_init.latent_vector", lv_np, (1, 4, 64, 64), np.float32)

    lv_nhwc_np = np.moveaxis(lv_np, [0, 1, 2, 3], [0, 3, 1, 2])
    print(
        f"iteration {iter}, seed: {seed}, random_init.latent_vector_nhwc",
        type(lv_nhwc_np),
        lv_nhwc_np.shape,
        lv_nhwc_np.dtype,
    )
    print(lv_nhwc_np[0, 0, 0:2, :])

    # extract uncond_text_embedding_np, TBD
    uncond_embedding_key = "uncond_text_embedding"
    if not uncond_embedding_key in ts_dict:
        print(f"Picke file parsing error: key {uncond_embedding_key} is not found")
        uncond_text_embedding_np = None
    else:
        uncond_text_embedding_np = ts_dict[uncond_embedding_key]
        print(
            f"iteration {iter}, {uncond_embedding_key}: ",
            type(uncond_text_embedding_np),
            uncond_text_embedding_np.shape,
            uncond_text_embedding_np.dtype,
            uncond_text_embedding_np[0, 0, 0:4],
        )
        check_shape_type(
            uncond_embedding_key, uncond_text_embedding_np, (1, 77, 768), np.float32
        )

    return (iter, ts_np, ts_embed_np, seed, lv_nhwc_np, uncond_text_embedding_np)


def parse_pickle(pickle_file):
    random_init_dict = {}
    ts_list_dict = {}
    ts_embed_list_dict = {}
    num_rec = 0

    print(f"parsing {pickle_file} ...")
    fd = open(pickle_file, "rb")
    tensor_dict_list = pickle.load(fd)

    num_steps = 0
    ts_list = []
    ts_embed_list = []
    for ts_dict in tensor_dict_list:
        if not "iteration" in ts_dict:
            raise Exception(f'Picke file parsing error: key "iteration" is not found')

        iter = ts_dict["iteration"]
        (iter, ts_np, ts_embed_np, seed, lv_nhwc_np, uncond_text_embedding_np) = (
            extract_data(ts_dict)
        )
        num_rec += 1
        if iter == 0 and iter < num_steps:
            print(
                f"num_steps is {num_steps}, iteration:{iter} wrap around, reset num_steps..."
            )
            # iter wrap around start of next session, the first session is done
            ts_list_dict[num_steps] = ts_list
            ts_embed_list_dict[num_steps] = ts_embed_list
            num_steps = 0
            ts_list = []
            ts_embed_list = []

        num_steps += 1
        ts_list.append(ts_np)
        ts_embed_list.append(ts_embed_np)

        random_init_dict[seed] = lv_nhwc_np

    ts_list_dict[num_steps] = ts_list
    ts_embed_list_dict[num_steps] = ts_embed_list

    return (
        num_rec,
        ts_list_dict,
        ts_embed_list_dict,
        random_init_dict,
        uncond_text_embedding_np,
    )


def parse_random_latent_init_pickle(pickle_file):
    print(f"parsing {pickle_file} ...")
    tensor_dict_list = None
    with open(pickle_file, "rb") as fd:
        tensor_dict_list = pickle.load(fd)

    random_init_dict = {}
    for seed in tensor_dict_list:
        lv_np = tensor_dict_list[seed]
        print("random_init.latent_vector_nchw", type(lv_np), lv_np.shape, lv_np.dtype)
        check_shape_type("random_init.latent_vector", lv_np, (1, 4, 64, 64), np.float32)

        lv_nhwc_np = np.moveaxis(lv_np, [0, 1, 2, 3], [0, 3, 1, 2])
        print(
            f"seed: {seed}, random_init.latent_vector_nhwc",
            type(lv_nhwc_np),
            lv_nhwc_np.shape,
            lv_nhwc_np.dtype,
        )
        print(lv_nhwc_np[0, 0, 0:2, :])

        random_init_dict[int(seed)] = lv_nhwc_np

    return len(tensor_dict_list.keys()), random_init_dict


def parse_ts_embedding_pickle(pickle_file):
    print(f"parsing {pickle_file} ...")
    tensor_dict_list = None
    with open(pickle_file, "rb") as fd:
        tensor_dict_list = pickle.load(fd)

    keys = sorted([int(key) for key in tensor_dict_list.keys()])
    print(keys)

    ts_embed_list = []
    for iter_num in keys:
        ts_embed_np = tensor_dict_list[str(iter_num)]
        print(
            f"iteration {iter_num}, timeembedding: ",
            type(ts_embed_np),
            ts_embed_np.shape,
            ts_embed_np.dtype,
            ts_embed_np[0, 0:4],
        )
        check_shape_type("timeembedding", ts_embed_np, (1, 1280), np.float32)
        ts_embed_list.append(ts_embed_np)

    return len(keys), ts_embed_list


def parse_ts_list_pickle(pickle_file):
    print(f"parsing {pickle_file} ...")
    ts_data = None
    with open(pickle_file, "rb") as fd:
        ts_data = pickle.load(fd)

    ts_np = ts_data.astype(np.int32)
    print(f"time_step: {ts_np}")

    return len(ts_np), ts_np


def parse_unconditional_encoding_pickle(pickle_file):
    print(f"parsing {pickle_file} ...")
    uncond_text_embedding_np = None
    with open(pickle_file, "rb") as fd:
        uncond_text_embedding_np = pickle.load(fd)

    print(
        f"uncond_text_embedding: ",
        type(uncond_text_embedding_np),
        uncond_text_embedding_np.shape,
        uncond_text_embedding_np.dtype,
        uncond_text_embedding_np[0, 0, 0:4],
    )
    check_shape_type(
        "uncond_text_embedding", uncond_text_embedding_np, (1, 77, 768), np.float32
    )

    return uncond_text_embedding_np


def dump_data(
    pickle_stats,
    ts_list_dict,
    ts_embed_list_dict,
    random_init_dict,
    uncond_text_embedding_np,
    dumpdir,
):

    file_list = []

    # dump random_init variables
    seed_list = list(sorted(random_init_dict.keys()))
    shape_str = "x".join([str(e) for e in random_init_dict[seed_list[0]].shape])
    f_name = os.path.join(
        dumpdir, f"rand_init_{len(seed_list)}_seeds_{shape_str}_float32.bin.rand"
    )
    with open(f_name, "wb") as rand_file:
        v_np = np.array(len(seed_list)).astype(np.int32)
        v_np.tofile(rand_file)
        for seed in seed_list:
            v_np = np.array(seed).astype(np.int32)
            v_np.tofile(rand_file)
        for seed in seed_list:
            random_init_dict[seed].tofile(rand_file)
        file_list.append(f_name)

    # dump ts and ts_embed variables
    for num_steps in ts_list_dict:
        ts_list = ts_list_dict[num_steps]
        ts_embed_list = ts_embed_list_dict[num_steps]
        shape_str = "x".join([str(e) for e in ts_embed_list[0].shape])
        f_name = os.path.join(
            dumpdir,
            f"timestep_steps_{num_steps}_int32_embedding_{shape_str}_float32.bin.ts",
        )
        with open(f_name, "wb") as ts_file:
            v_np = np.array(len(ts_list)).astype(np.int32)
            v_np.tofile(ts_file)
            for l in ts_list:
                l.tofile(ts_file)
            for l in ts_embed_list:
                l.tofile(ts_file)
            file_list.append(f_name)

    # dump uncond_text_embedding variables
    shape_str = "x".join([str(e) for e in uncond_text_embedding_np.shape])
    uncond_text_embedding_file = os.path.join(
        dumpdir, f"batch_1_uncond_text_embedding_{shape_str}_float32.bin.cte"
    )
    with open(uncond_text_embedding_file, "wb") as f:
        uncond_text_embedding_np.tofile(f)

    with open(os.path.join(dumpdir, "readme.txt"), "w") as f:
        print("From:", file=f)
        for pfile in pickle_stats:
            f_stat = os.stat(pfile)
            dts = datetime.fromtimestamp(f_stat.st_ctime)
            (v0, v1, v2) = pickle_stats[pfile]
            print(f"    {pfile}, total_rec: {v0}, created on {dts} ", file=f)
            print(f"        num_steps: {v1}", file=f)
            print(f"        {len(v2)} unique_random_seeds: {v2}", file=f)
        (v1, v2) = (list(sorted(ts_list_dict.keys())), sorted(seed_list))
        print("\nTotal:", file=f)
        print(f"    num_steps: {v1}", file=f)
        print(f"    {len(v2)} unique_random_seeds: {v2}", file=f)
        print(f"    number of uncond_text_embedding: 1", file=f)
    # tar the files together
    tar_file = os.path.join(dumpdir, f"sd_precomute_data.tar")
    cmd = (
        f'/bin/tar cvf {tar_file} {os.path.join(dumpdir, "readme.txt")} '
        + " ".join(file_list)
        + f" {uncond_text_embedding_file}"
    )
    print(f"Run {cmd}")
    os.system(cmd)


# for debugging only
def create_smaller_pickle(file_name, count=10):
    s_file_name = "small_" + file_name
    with open(file_name, "rb") as f:
        data_dict_seq = pickle.load(f)
    with open(s_file_name, "wb") as f:
        pickle.dump(data_dict_seq[0:count], f)


if __name__ == "__main__":
    default_logdir = os.path.join(
        "tar_output", datetime.now().strftime("%Y-%m-%d-%H-%M-%S")
    )

    parser = argparse.ArgumentParser(
        description="Generates sd_precomute_data.tar file based on provided pkl files."
    )

    parser.add_argument(
        "--random_latent_init",
        type=str,
        required=True,
        help="Path to a random-latent-init pkl file containing random initial latents.",
    )
    parser.add_argument(
        "--time_step_embedding",
        type=str,
        required=True,
        help="Comma seperated time-step-embedding pkl files containing ts-embedding.",
    )
    parser.add_argument(
        "--time_step_list",
        type=str,
        required=True,
        help="Comma seperated time-step-list pkl files containing timestep.",
    )
    parser.add_argument(
        "--unconditional_text_emb",
        type=str,
        required=True,
        help="Path to a unconditional-text-emb pkl file containing unconditional text embedding.",
    )
    parser.add_argument(
        "--dumpdir",
        type=str,
        default=default_logdir,
        help="Path to a directory for dumping.\
                              Default value is 'tar_output/<Y-m-d-H-M-S>'",
    )

    config = parser.parse_args()
    config.time_step_embedding = config.time_step_embedding.split(",")
    config.time_step_list = config.time_step_list.split(",")

    os.makedirs(config.dumpdir, exist_ok=True)

    pickle_stats = {}

    num_rec, random_init_dict = parse_random_latent_init_pickle(
        config.random_latent_init
    )
    # pickle_stats[config.random_latent_init] = (num_rec, list(sorted(random_init_dict.keys())))

    ts_embed_list_dict = {}
    for ts_embedd_file in config.time_step_embedding:
        length, data = parse_ts_embedding_pickle(ts_embedd_file)
        ts_embed_list_dict[length] = data
        # pickle_stats[ts_embedd_file] = (length, [1])

    ts_list_dict = {}
    for ts_list_file in config.time_step_list:
        length, data = parse_ts_list_pickle(ts_list_file)
        ts_list_dict[length] = data
        # pickle_stats[ts_list_file] = (length, [1])

    uncond_text_embedding_np = parse_unconditional_encoding_pickle(
        config.unconditional_text_emb
    )

    if sorted(ts_embed_list_dict.keys()) != sorted(ts_list_dict.keys()):
        raise Exception("Wrong files for time_step_embedding and time_step_list")

    # pickle_files = sys.argv[1:]
    # pickle_stats ={}
    # ts_list_dict, ts_embed_list_dict, random_init_dict, uncond_text_embedding_np = {}, {}, {}, None
    # for file in pickle_files:
    #     # create smaller pickle for debugging purpose
    #     #create_smaller_pickle(file)
    #     num_rec, the_ts_list_dict, the_ts_embed_list_dict, the_random_init_dict, the_uncond_text_embedding_np = parse_pickle(file)
    #     ts_list_dict.update(the_ts_list_dict)
    #     ts_embed_list_dict.update(the_ts_embed_list_dict)
    #     random_init_dict.update(the_random_init_dict)
    #     pickle_stats[file] = (num_rec, list(sorted(the_ts_list_dict.keys())), list(sorted(the_random_init_dict.keys())))
    #     if the_uncond_text_embedding_np is not None:
    #         uncond_text_embedding_np = the_uncond_text_embedding_np
    # if uncond_text_embedding_np is None:
    #     raise Exception('uncond_text_embedding not found in all pickle files')

    dump_data(
        pickle_stats,
        ts_list_dict,
        ts_embed_list_dict,
        random_init_dict,
        uncond_text_embedding_np,
        config.dumpdir,
    )
