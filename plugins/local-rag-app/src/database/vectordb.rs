/*
**************************************************************************************************
* Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
**************************************************************************************************
*/

use std::borrow::BorrowMut;
use std::path::PathBuf;

use hora::core::ann_index::ANNIndex;
use hora::core::ann_index::SerializableIndex;
use hora::core::metrics::Metric;
use hora::index::hnsw_idx::HNSWIndex;
use hora::index::hnsw_params::HNSWParams;

#[doc = "Create a HNSWIndex at location specified at path, with dimension dim"]
pub fn create_index(path: PathBuf, dim: usize) -> HNSWIndex<f32, usize> {
    HNSWIndex::<f32, usize>::new(dim, &HNSWParams::<f32>::default())
}

/// Load a HNSWIndex from the location specified path.
pub fn load_index(path: PathBuf) -> HNSWIndex<f32, usize> {
    let mut project_path: PathBuf = path.clone();
    project_path.push("ragdb.index");
    println!("Loading index from {:?}", project_path.clone());
    let index: HNSWIndex<f32, usize>;
    match HNSWIndex::<f32, usize>::load(project_path.to_str().unwrap()) {
        Ok(hnsw) => {
            index = hnsw;
        }
        Err(err) => {
            panic!("Error loading Index: {:?}", err)
        }
    }

    index
}

pub fn get_max_id(path: PathBuf) -> Option<usize> {
    let mut project_path: PathBuf = path.clone();
    project_path.push("ragdb.index");
    let index: HNSWIndex<f32, usize>;
    match HNSWIndex::<f32, usize>::load(project_path.to_str().unwrap()) {
        Ok(hnsw) => {
            index = hnsw;
        }
        Err(err) => return None,
    }

    Some(index.dimension())
}

pub fn save_index(index: &mut HNSWIndex<f32, usize>, project_path: PathBuf) {
    let mut path = project_path.clone();
    path.push("ragdb.index");

    match index.build(Metric::Euclidean) {
        Ok(_msg) => match index.dump(path.to_str().unwrap()) {
            Ok(_dump_msg) => {}
            Err(dump_err) => {
                println!("{}", dump_err);
            }
        },
        Err(err) => {
            println!("{}", err);
        }
    }
}

pub trait Utils {
    fn create(path: PathBuf) -> Self;
    fn load(path: PathBuf) -> Self;
    fn save(s: Self);
}
