/*
**************************************************************************************************
* Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
**************************************************************************************************
*/

#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
#[macro_use]
extern crate lazy_static;

include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

mod api_server;
mod database;
mod documents;
mod errors;
mod knowledge_base;
mod models;
mod npu_backend;

use clap::{Args, Parser, Subcommand};
use knowledge_base::{Matches, RagDb, Util};

use std::{path::PathBuf, time::Instant};

#[derive(Parser, Debug)]
#[clap(author, version, about)]
struct CommandArgs {
    #[clap(subcommand)]
    pub entity_type: EntityType,
}

#[derive(Debug, Subcommand)]
pub enum EntityType {
    Init(InitKnowledgeBaseCommand),
    Create(CreateKnowledgeBaseCommand),
    Build(BuildKnowledgeBaseCommand),
    Answer(QueryKnowledgeBaseCommand),
    Server(APIServerCommand),
}

#[derive(Debug, Args)]
pub struct APIServerCommand {
    #[arg(long = "port")]
    port: u16,
    #[arg(short = 'p', long = "project-path")]
    project_path: String,
    #[arg(short = 'm', long = "model-name")]
    model_name: Option<String>,
    #[arg(short = 'c', long = "config")]
    config: Option<String>,
    #[arg(short = 'd', long = "device")]
    device: Option<String>,
    #[arg(long = "chunk-size")]
    chunk_size: usize,
}

#[derive(Debug, Args)]
pub struct CreateKnowledgeBaseCommand {
    pub project_name: String,
    #[arg(short = 'm', long = "model")]
    model_name: Option<String>,
}

#[derive(Debug, Args)]
pub struct BuildKnowledgeBaseCommand {}

#[derive(Debug, Args)]
pub struct InitKnowledgeBaseCommand {
    #[arg(short = 'm', long = "model")]
    model_name: Option<String>,
}

#[derive(Debug, Args)]
pub struct QueryKnowledgeBaseCommand {
    pub query: String,
}

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    match CommandArgs::parse().entity_type {
        EntityType::Init(init_command) => match init_command.model_name {
            Some(model_name) => {
                let current_dir_path: PathBuf = std::env::current_dir().unwrap();
                let _ragdb_knowledge_base: RagDb =
                    RagDb::new(current_dir_path, Some(model_name), None, None, 256);
            }
            None => {
                let current_dir_path: PathBuf = std::env::current_dir().unwrap();
                let _ragdb_knowledge_base: RagDb = RagDb::new(current_dir_path, None, None, None, 256);
            }
        },
        EntityType::Create(project_command) => {
            let mut current_dir_path: PathBuf = std::env::current_dir().unwrap();
            current_dir_path.push(project_command.project_name);
            match std::fs::create_dir(&current_dir_path) {
                Ok(_) => {}
                Err(err) => {
                    panic!("Error creating directory: {:?}", err)
                }
            }
            match project_command.model_name {
                Some(path) => {
                    let _ragdb_knowledge_base = RagDb::new(current_dir_path, Some(path), None, None, 256);
                }
                None => {
                    let _ragdb_knowledge_base = RagDb::new(current_dir_path, None, None, None, 256);
                }
            }
        }
        EntityType::Build(_) => {
            let current_dir_path: PathBuf = std::env::current_dir().unwrap();
            let ragdb_knowledge_base = RagDb::load(current_dir_path.clone());
            ragdb_knowledge_base.search_and_build_index(&current_dir_path);
        }
        EntityType::Answer(query) => {
            let now = Instant::now();
            let current_dir_path: PathBuf = std::env::current_dir().unwrap();
            let ragdb_knowledge_base = RagDb::load(current_dir_path.clone());
            let matches: Vec<Matches> = ragdb_knowledge_base.search(query.query, 5).unwrap();
            for search_result in matches {
                println!("{}, {:#?}", search_result.document_name, search_result);
            }

            println!("Time to generate results: {:?}", now.elapsed().as_secs());
        }
        EntityType::Server(server_command) => {
            let server = api_server::start_api_server(
                server_command.port,
                server_command.project_path,
                server_command.model_name,
                server_command.config,
                server_command.device,
                server_command.chunk_size,
            )
            .await;
            if let Err(e) = server {
                println!("Unable to start the server: {}", e);
            }
        }
    };

    Ok(())
}
