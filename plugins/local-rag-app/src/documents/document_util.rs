/*
**************************************************************************************************
* Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
**************************************************************************************************
*/

use crate::errors::KnowledgeBaseError;
use crate::models::model::encode_text;

use anyhow::{bail, Result};

use hora::core::ann_index::ANNIndex;
use hora::index::hnsw_idx::HNSWIndex;

use crate::documents::document::Document;

use crate::database::vectordb::{create_index, load_index, save_index};

use crate::database::sql_database::{get_max_id, insert_data_into_sql_db};

use std::path::PathBuf;
use crate::npu_backend::EmbeddingNpuModel;

pub trait DocumentEncoder {
    fn build_semantic_search(
        doc_list: &mut Vec<Document>,
        project_path: PathBuf,
        model_name: String,
        embedding_npu_model: Option<EmbeddingNpuModel>,
    ) -> Result<()>;
    fn add_document_to_index(
        doc: &mut Document,
        project_path: &PathBuf,
        model_name: String,
        embedding_npu_model: Option<EmbeddingNpuModel>,
    ) -> Result<()>;
    fn encode_text_via_model(&self, model_name: String, embedding_npu_model: Option<EmbeddingNpuModel>) -> Option<Vec<(String, Vec<f32>)>>;
    fn load(file_path: String) -> Result<Document>;
}

impl DocumentEncoder for Document {
    /// Building Semantic Search for a vector of documents.
    fn build_semantic_search(
        doc_list: &mut Vec<Document>,
        project_path: PathBuf,
        model_name: String,
        embedding_npu_model: Option<EmbeddingNpuModel>,
    ) -> Result<()> {
        // We iterate through documents, generate the embeddings, and add the embeddings to index.
        //Get the index
        let index_usize: usize;
        if embedding_npu_model.clone().is_some() {
            index_usize = 1024;
        } else {
            index_usize = 384;
        }
        let mut index: HNSWIndex<f32, usize> = create_index(project_path.clone(), index_usize);
        let mut id: usize = 0;

        for doc in doc_list {
            match doc.encode_text_via_model(model_name.clone(), embedding_npu_model.clone()) {
                Some(vector_embeddings) => {
                    match doc.get_document_data() {
                        Some(data) => {
                            match doc.get_page_numbers() {
                                Some(page_numbers) => {
                                    for (embedding_data, page_number) in vector_embeddings.iter().zip(page_numbers.iter())   {
                                        id += 1;
                                        let embedding = embedding_data.1.clone();
                                        {
                                            //Insert it into the index
                                            match index.add(&embedding, id) {
                                                Ok(_msg) => {}
                                                Err(err) => {
                                                    println!("Error: {}, on id:{}", err, id);
                                                }
                                            }
        
                                            //Insert it into sql db for text retreival
                                            match insert_data_into_sql_db(
                                                project_path.clone(),
                                                &doc.get_document_path_as_string().unwrap(),
                                                &embedding_data.0,
                                                page_number,
                                                id,
                                            ) {
                                                Ok(_msg) => {}
                                                Err(err) => {
                                                    println!(
                                                        "Error inserting line:{} due to error:{:?}",
                                                        embedding_data.0, err
                                                    );
                                                }
                                            }
                                        }
                                    }
                                }
                                None => {
                                    println!(
                                        "No page numbers found for file:{}",
                                        doc.get_document_name_as_string().unwrap()
                                    );
                                    continue;
                                }
                            }
                            
                        }
                        None => {
                            println!(
                                "No Text found for file:{}",
                                doc.get_document_name_as_string().unwrap()
                            );
                            continue;
                        }
                    }
                }
                None => {
                    println!(
                        "Error generating embeddings for: {:?}",
                        doc.get_document_name_as_string().unwrap()
                    );
                    continue;
                }
            }
        }

        save_index(&mut index, project_path);

        Ok(())
    }

    fn add_document_to_index(
        doc: &mut Document,
        project_path: &PathBuf,
        model_name: String,
        embedding_npu_model: Option<EmbeddingNpuModel>,
    ) -> Result<()> {
        let mut index: HNSWIndex<f32, usize> = load_index(project_path.clone());

        // Get the maximum existing ID
        let mut id = 0;
        if let Some(max_id) = get_max_id(project_path.to_path_buf()) {
            id = max_id;
        }
        println!("Max id: {}", id);
        match doc.encode_text_via_model(model_name, embedding_npu_model) {
            Some(vector_embeddings) => {
                match doc.get_document_data() {
                    Some(data) => {
                       match doc.get_page_numbers() {
                                Some(page_numbers) => {
                                    for (embedding_data, page_number) in vector_embeddings.iter().zip(page_numbers.iter())   {
                                        id += 1;
                                        let embedding = embedding_data.1.clone();
                                        {
                                            //Insert it into the index
                                            match index.add(&embedding, id) {
                                                Ok(_msg) => {}
                                                Err(err) => {
                                                    println!("Error: {}, on id:{}", err, id);
                                                }
                                            }
        
                                            //Insert it into sql db for text retreival
                                            match insert_data_into_sql_db(
                                                project_path.clone(),
                                                &doc.get_document_path_as_string().unwrap(),
                                                &embedding_data.0,
                                                page_number,
                                                id,
                                            ) {
                                                Ok(_msg) => {}
                                                Err(err) => {
                                                    println!(
                                                        "Error inserting line:{} due to error:{:?}",
                                                        embedding_data.0, err
                                                    );
                                                }
                                            }
                                        }
                                    }
                                }
                                None => {
                                    println!(
                                        "No page numbers found for file:{}",
                                        doc.get_document_name_as_string().unwrap()
                                    );
                                }
                            }
                    }
                    None => {
                        println!(
                            "No Text found for file:{}",
                            doc.get_document_name_as_string().unwrap()
                        );
                    }
                }
            }
            None => {
                println!(
                    "Error generating embeddings for: {:?}",
                    doc.get_document_name_as_string().unwrap()
                );
            }
        }
        save_index(&mut index, project_path.to_path_buf());
        Ok(())
    }

    /// Encode text of the current document via a sentence embedding model.
    /// If the model was unable to encode the text into vector embeddings then
    /// none is returned.
    fn encode_text_via_model(&self, model_name: String, embedding_npu_model: Option<EmbeddingNpuModel>) -> Option<Vec<(String, Vec<f32>)>> {
        let mut _encodings: Vec<Vec<f32>> = vec![];
        let sentences: Vec<String>;
        match self.get_document_data() {
            Some(vec_string) => {
                sentences = vec_string.to_vec();
            }
            None => {
                println!("Document has no parsed data");
                return None;
            }
        }
        match encode_text(&sentences, model_name, embedding_npu_model) {
            Ok(embedded_sentences) => Some(embedded_sentences),
            Err(_) => {
                println!(
                    "Unable to generate Embeddings for document:{:?}",
                    self.get_document_name_as_string()
                );
                None
            }
        }
    }

    fn load(file_path: String) -> Result<Document> {
        let file_metadata: std::fs::Metadata;

        match std::fs::metadata(&file_path) {
            Ok(mdata) => {
                file_metadata = mdata;
            }
            Err(_err) => bail!(KnowledgeBaseError),
        }

        if !file_metadata.is_file() {
            //If the Path is not a Document return an error
            bail!(KnowledgeBaseError);
        }
        let file_name: String;
        match std::path::Path::new(&file_path).file_name() {
            Some(name) => {
                file_name = name.to_string_lossy().to_string();
            }
            None => bail!(KnowledgeBaseError),
        }
        let document: Document = Document {
            //
            name: file_name,
            path: PathBuf::from(file_path),
            metadata: Some(file_metadata),
            data: None,
            page_numbers:None,
            encodings: None,
        };
        Ok(document)
    }
}