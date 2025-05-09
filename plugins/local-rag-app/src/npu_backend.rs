/*
**************************************************************************************************
* Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
**************************************************************************************************
*/

use std::sync::Mutex;
use serde::{ Deserialize, Serialize };
use std::path::PathBuf;
use local_rag_app::{
    GENIE_STATUS_SUCCESS,
    GenieEmbeddingConfig_Handle_t,
    GenieEmbedding_Handle_t,
    Genie_Status_t,
    Genie_getApiMajorVersion,
    Genie_getApiMinorVersion,
    Genie_getApiPatchVersion,
    GenieEmbeddingConfig_createFromJson,
    GenieEmbeddingConfig_free,
    GenieEmbedding_create,
    GenieEmbedding_generate,
    GenieEmbedding_free
};
use std::fs::File;
use std::io::Read;
use std::ffi::CString;
use std::os::raw::{c_char, c_void};
use std::env;

static mut m_configHandle: GenieEmbeddingConfig_Handle_t = std::ptr::null_mut();
static mut m_embeddingHandle: GenieEmbedding_Handle_t = std::ptr::null_mut();
lazy_static! {
    static ref EMBEDDING_BUFFER: Mutex<Option<Vec<f32>>> = Mutex::new(None);
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct EmbeddingNpuModel {
    model_name: String,
    config_path: PathBuf,
}

impl EmbeddingNpuModel {

    pub fn new(model_name: String, config: Option<String>) -> EmbeddingNpuModel {
        let vec = vec![String::from("BGELargeENV15NPU_Int8"), String::from("BGELargeENV15NPU_Fp16")];
        if !vec.contains(&model_name.to_string()) {
            panic!("Unsupported NPU model {}", model_name);
        }
        let major_version: u32;
        let minor_version: u32;
        let patch_version: u32;
        unsafe {
            major_version = Genie_getApiMajorVersion();
            minor_version = Genie_getApiMinorVersion();
            patch_version = Genie_getApiPatchVersion();
        }

        println!("Genie version: v{}.{}.{}", major_version, minor_version, patch_version);
        
        let mut config_path;
        match config {
            Some(conf) => {
                config_path = PathBuf::from(conf);
            }
            None => {
                config_path = std::env::current_exe().ok().and_then(|p| p.parent().map(|p| p.to_path_buf())).unwrap();
                let config_file = PathBuf::from(format!("{}.json", model_name.clone()));
                config_path.push(&config_file);
            }
        }
        
        println!("{}", config_path.clone().display());

        EmbeddingNpuModel::load_model(config_path.clone());

        let embeddingNpuModel: EmbeddingNpuModel = EmbeddingNpuModel {
            // m_configHandle : &mut config_handle as *mut _,
            model_name: model_name,
            config_path: config_path,
        };

        embeddingNpuModel
    }

    pub fn load_model(config_path: PathBuf) {
        let config_content: String = EmbeddingNpuModel::read_json_file(&(config_path)).expect("Failed to read JSON file");
        let c_string = CString::new(config_content).expect("Failed to create CString");
        unsafe {
            let c_status: Genie_Status_t = GenieEmbeddingConfig_createFromJson(c_string.as_ptr(), &raw mut m_configHandle);
            if GENIE_STATUS_SUCCESS as i32 != c_status {
                panic!("Failed to create the embedding config. Error code: {}", c_status);
            }

            let e_status: Genie_Status_t = GenieEmbedding_create(m_configHandle, &raw mut m_embeddingHandle);
            if GENIE_STATUS_SUCCESS as i32 != e_status {
                panic!("Failed to create the embedding. Error code: {}", e_status);
            }
        }
    }

    extern "C" fn embedding_callback(
        dimensions: *const u32,
        rank: u32,
        embeddingBuffer: *const f32,
        _userData: *const c_void,
    ) {
        let dimensions_slice = unsafe { std::slice::from_raw_parts(dimensions, rank as usize) };
        let embedding_buffer_size: u64 = dimensions_slice.iter().map(|&dim| dim as u64).product();
    
        println!("RANK of DIMENSIONS: {}", rank);
        print!("EMBEDDING DIMENSIONS: [ ");
        for (i, &dim) in dimensions_slice.iter().enumerate() {
            print!("{}{}", dim, if i != rank as usize - 1 { ", " } else { " ]\n" });
        }
        println!("GENERATED EMBEDDING SIZE: {}", embedding_buffer_size);

        // Store the embedding buffer in the static variable
        let embedding_slice = unsafe { std::slice::from_raw_parts(embeddingBuffer, embedding_buffer_size as usize) };
        let mut buffer = EMBEDDING_BUFFER.lock().unwrap();
        *buffer = Some(embedding_slice.to_vec());
    }

    pub fn generate(&self, prompt: String) -> Option<Vec<f32>> {
        unsafe {
            let c_string = CString::new(prompt).expect("Failed to create CString");

            let status: Genie_Status_t =
                GenieEmbedding_generate(m_embeddingHandle, c_string.as_ptr(), Some(EmbeddingNpuModel::embedding_callback), std::ptr::null());
            if GENIE_STATUS_SUCCESS as i32 != status {
                panic!("Failed to generate embedding. Error code: {}", status);
            }

            let buffer = EMBEDDING_BUFFER.lock().unwrap();
            buffer.clone()
        }
    }

    pub fn unload_model(&self) {
        unsafe {
            if !m_configHandle.is_null() {
                let c_status: Genie_Status_t = GenieEmbeddingConfig_free(m_configHandle);
                if GENIE_STATUS_SUCCESS as i32 != c_status {
                    panic!("Failed to free the embedding config. Error code: {}", c_status);
                }
            }

            if !m_embeddingHandle.is_null() {
                let e_status: Genie_Status_t = GenieEmbedding_free(m_embeddingHandle);
                if GENIE_STATUS_SUCCESS as i32 != e_status {
                    panic!("Failed to free the embedding. Error code: {}", e_status);
                }
            }
        }
    }

    fn read_json_file(file_path: &PathBuf) -> Result<String, std::io::Error> {
        let mut file = File::open(file_path)?;
        let mut content = String::new();
        file.read_to_string(&mut content)?;
        Ok(content)
    }
}

