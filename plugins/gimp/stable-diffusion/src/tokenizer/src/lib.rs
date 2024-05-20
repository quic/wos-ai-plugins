// ---------------------------------------------------------------------
// Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause
// ---------------------------------------------------------------------

use std::sync::Mutex;
use lazy_static::lazy_static;
use libc::{c_char};
use std::ffi::CStr;
use std::ptr;

use tokenizers::Tokenizer;
use tokenizers::utils::padding::{PaddingParams, PaddingStrategy};
use tokenizers::utils::truncation::{TruncationParams};

lazy_static! {
    static ref CURR_DATA_PATH: Mutex<String> = Mutex::new(String::new());
}

lazy_static!{
    static ref TOKENIZER: Tokenizer = {
        let max_length = 77;            // designed by Riffusion pipeline
        let id_endoftext = 49407;       // vocab.json

        let currdatapath = CURR_DATA_PATH.lock().unwrap();
        let mut tokenizer_ = Tokenizer::from_file(&(currdatapath.clone()+"/openai-clip-vit-base-patch32")).expect("Should");
        tokenizer_.with_truncation(Some(TruncationParams {
                max_length:max_length,
                ..Default::default()
            }))
            .with_padding(Some(PaddingParams {
                strategy: PaddingStrategy::Fixed(max_length),
                pad_token: String::from("<|endoftext|>"),
                pad_id: id_endoftext,
                ..Default::default()
            }));
        tokenizer_
    };
}

#[no_mangle]
pub unsafe extern "C" fn get_token_ids(text: *mut c_char, buffer: *mut u32, buffer_length: usize) -> usize {
    let text_str = CStr::from_ptr(text);
    let text_input = (text_str.to_str().unwrap()).to_string();

    let encoding = match TOKENIZER.encode(text_input, true) {
        Err(e) => panic!("Error: {:?}", e),
        Ok(f) => f,
    };

    let token_ids = encoding.get_ids();
    let len = if token_ids.len() > buffer_length {buffer_length} else {token_ids.len()};
    let ptr = token_ids.as_ptr();
    ptr::copy(ptr, buffer, len);
    len
}

#[no_mangle]
pub unsafe extern "C" fn set_datapath(datapath: *mut c_char) {
    *CURR_DATA_PATH.lock().unwrap() = (CStr::from_ptr(datapath).to_str().unwrap()).to_string();
    // Calling get_token_ids for dummy input to initialize the Tokenizer
    get_token_ids([0].as_mut_ptr(), [0].as_mut_ptr(), 0);
}
