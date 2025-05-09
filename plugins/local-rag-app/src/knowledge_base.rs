/*
**************************************************************************************************
* Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
**************************************************************************************************
*/

use crate::database::sql_database::{create_sqlite_db, sql_get_all_doc_names, sql_search_by_id};
use crate::database::vectordb::{create_index, load_index, save_index};
use crate::documents::document::{get_file, get_file_list, get_file_text, get_file_text_with_pages, Document};
use crate::documents::document_util::DocumentEncoder;
use crate::models::model::encode_text;
use crate::npu_backend::EmbeddingNpuModel;
use hora::core::ann_index::ANNIndex;
use hora::index::hnsw_idx::HNSWIndex;
use rayon::prelude::*;
use serde::{Deserialize, Serialize};
use std::collections::HashSet;
use std::fs::create_dir;
use std::io::ErrorKind;
use std::path::PathBuf;
use std::process::exit;
use text_splitter::TextSplitter;
use crate::documents::document::get_file_from_name;

#[derive(Debug)]
pub struct Matches {
    pub document_name: String,
    pub line: String,
    pub page_number: u32,
}


#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct RagDb {
    path: PathBuf,
    model_name: String,
    config: Option<String>,
    device: String,
    embedding_npu_model: Option<EmbeddingNpuModel>,
    chunk_size: usize,
}

impl RagDb {
    /// Description: This function will the current directory for all files, and store all the supported file_types as
    /// a Vector of Documents doc_list. This is done by calling the get_file_list function. Then using doc_list extract all text from the supported file types and store the data
    /// in the data attribute of Document Structure respectively.That is done by calling the get_text_from_all_docs function.
    /// Next it will split the text for each document into an array of string. Each string will be the size of tokens that could be accepted by
    /// an embedding model in sentence_transformer.
    fn get_text_from_all_docs(&self, doc_list: &mut Vec<Document>) {
        doc_list.par_iter_mut().for_each(|doc: &mut Document| {
            println!("Now Processing {:?} ...", doc.get_document_name_as_string());
            let document_data_option: Option<Vec<String>> = get_file_text(doc, self.chunk_size);
            println!("Chunks: {}", document_data_option.clone().unwrap().len());
            match document_data_option {
                Some(doc_text) => doc.set_document_data(doc_text),
                None => println!(
                    "Error reading document {:?}",
                    doc.get_document_name_as_string()
                ),
            }
            println!(
                "\r Finished Processing file {:?}",
                doc.get_document_name_as_string()
            );
        });
    }

    fn get_text_from_doc(&self, doc: &mut Document) {
        let document_data_option = get_file_text(doc, self.chunk_size);
        println!("Chunks: {}", document_data_option.clone().unwrap().len());
        if let Some(doc_data) = document_data_option {
            doc.set_document_data(doc_data);
        } else {
            println!(
                "Error reading document {:?} no data",
                doc.get_document_name_as_string()
            );
        }
    }

    fn get_text_from_doc_with_pages(&self, doc: &mut Document) {
        let document_data_option = get_file_text_with_pages(doc, self.chunk_size);
        if let Some((text_vec, num_vec)) = document_data_option.clone() {
            println!("Text Chunks: {}, Number Chunks: {}", text_vec.len(), num_vec.len());
        }        
        if let Some(doc_data) = document_data_option {
            doc.set_document_data(doc_data.0);
            doc.set_page_numbers(doc_data.1);
        } else {
            println!(
                "Error reading document {:?} no data",
                doc.get_document_name_as_string()
            );
        }
    }

    ///Description: This function will the current directory for all files, and store all the supported file_types as
    ///a Vector of Documents doc_list. This is done by calling the get_file_list function. Then using doc_list extract all text from the supported file types and store the data
    ///in the data attribute of Document Structure respectively.That is done by calling the get_text_from_all_docs function.
    ///Next it will split the text for each document into an array of string. Each string will be the size of tokens that could be accepted by
    ///an embedding model in sentence_transformer.
    pub fn search_and_build_index(&self, path: &PathBuf) {
        let project_path: PathBuf = path.clone();
        //Get all the supported Documents from the directory and store it in documents
        let mut doc_list: Vec<Document>;
        match get_file_list(path) {
            Some(f_list) => {
                doc_list = f_list;
            }
            None => {
                panic!("Unable to get a list of file!")
            }
        }

        //Extract text from all the documents in the doc_list
        self.get_text_from_all_docs(&mut doc_list);

        //Generate encodings for all the text of a document in the list doc_list\
        let _ =
            Document::build_semantic_search(&mut doc_list, project_path, self.model_name.clone(), self.embedding_npu_model.clone());
    }

    // Add a document content to the knowledge base
    pub fn add_document_content(&self, file_name: String, content: String) {
        let mut project_path: PathBuf = self.path.clone();
        project_path.push(".ragdb");

        // println!("inside add_document_content");
        
        let mut doc: Document;
        if let Some(_doc) = get_file_from_name(file_name.clone()) {
            doc = _doc;
            let splitter = TextSplitter::new(self.chunk_size);
            let chunks_iter = splitter.chunks(&content);
            let chunks: Vec<String> = chunks_iter.map(|s| s.to_string()).collect();

            println!("got chunks {}", chunks.len());
            doc.set_document_data(chunks.clone()); 
            let vec_1: Vec<u32> = vec![1; chunks.len()];
            doc.set_page_numbers(vec_1);                       
            println!("chunks set");
            match Document::add_document_to_index(&mut doc, &project_path, self.model_name.clone(), self.embedding_npu_model.clone())
            {
                Ok(_msg) => {}
                Err(err) => {
                    println!("Error adding document to index: {:?}", err);
                }
            }
            println!(
                "Added {:?} document successfully",
                doc.get_document_name_as_string()
            );
        } else {
            println!("Invalid document provided: {:?}", file_name);
        }
    }

    // Add a document (individual file) to the knowledge base
    pub fn add_document(&self, doc_path: PathBuf) {
        let mut project_path: PathBuf = self.path.clone();
        project_path.push(".ragdb");
        let mut doc: Document;
        if let Some(_doc) = get_file(&doc_path) {
            doc = _doc;
            self.get_text_from_doc_with_pages(&mut doc);
            match Document::add_document_to_index(&mut doc, &project_path, self.model_name.clone(), self.embedding_npu_model.clone())
            {
                Ok(_msg) => {}
                Err(err) => {
                    println!("Error adding document to index: {:?}", err);
                }
            }
            println!(
                "Added {:?} document successfully",
                doc.get_document_name_as_string()
            );
        } else {
            println!("Invalid document path provided: {:?}", doc_path);
        }
    }

    pub fn get_added_doc_paths(&self) -> Vec<String> {
        let mut project_path: PathBuf = self.path.clone();
        project_path.push(".ragdb");
        let mut added_files: HashSet<String> = HashSet::new();
        if let Some(doc_names) = sql_get_all_doc_names(project_path) {
            for doc_name in doc_names {
                added_files.insert(doc_name.clone());
            }
        }
        added_files.into_iter().collect()
    }

    ///Search Index for related queries, and covert it back to original text
    pub fn search(&self, query: String, count: usize) -> Option<Vec<Matches>> {
        let mut results: Vec<usize> = vec![];
        let mut project_path = self.path.clone();
        project_path.push(".ragdb");
        match encode_text(&vec![query], self.model_name.clone(), self.embedding_npu_model.clone()) {
            //Generate Embeddings for the query
            Ok(encodings) => {
                for encoding in encodings {
                    let index: HNSWIndex<f32, usize> = load_index(project_path.clone());
                    let mut embedding = encoding.1;
                    println!("EmbedQuery: {:?}", embedding);
                    results.append(&mut index.search(&mut embedding, count));
                }
            }
            Err(_) => return None,
        }
        println!("Search Res: {:?}", results);
        let mut search_results: Vec<Matches> = vec![];
        match sql_search_by_id(project_path, results) {
            Some(search_output) => {
                for output in search_output {
                    search_results.push(Matches {
                        document_name: output.0,
                        line: output.1,
                        page_number: output.2,
                    });
                }
            }
            None => return None,
        }
        Some(search_results)
    } 
}

pub trait Util {
    fn new(path: PathBuf, model_path: Option<String>, config: Option<String>, device: Option<String>, chunk_size: usize) -> Self;
    fn load(path: PathBuf) -> Self;
    fn delete_all(&mut self);
}

impl Util for RagDb {
    /// Create a new RagDb Project. path points to where the new project will be created.
    /// local is a bool value indicating whether to use a remote model, or a local model
    /// If local is true then model_path will contain the path to the directory where all the model files are located.
    /// If local is false then the model_path will be an empty string.
    fn new(path: PathBuf, model_name: Option<String>, config: Option<String>, device: Option<String>, chunk_size: usize) -> RagDb {
        let mut project_path: PathBuf = path.clone();
        project_path.push(".ragdb");
        match create_dir(&project_path) {
            Ok(_msg) => println!("Created project folder"),
            Err(err) => {
                if err.kind() == ErrorKind::AlreadyExists {
                    println!("Loading RagDb from previous project")
                } else {
                    panic!("Error creating ragdb project: {:?}", err)
                }
            }
        }

        // Init NPU model
        let embeddingNpuModel: Option<EmbeddingNpuModel>;
        let index_usize: usize;
        if device.clone().unwrap_or(String::from("npu")) == "npu" {
            index_usize = 1024;
            embeddingNpuModel = Some(EmbeddingNpuModel::new(model_name.clone().unwrap(), config.clone()));
            embeddingNpuModel.clone().expect("Failed to load NPU model");
        } else {
            index_usize = 384;
            embeddingNpuModel = None;
        }

        
        //Create the index to be saved in .ragdb
        let mut _index: HNSWIndex<f32, usize> = create_index((*project_path).to_path_buf(), index_usize);
        save_index(&mut _index, project_path.clone());

        //Create the sqlite database to be saved in .ragdb
        let conn = create_sqlite_db((*project_path).to_path_buf());
        match conn.close() {
            Ok(_c) => println!("Successfully created database"),
            Err(err) => panic!("Error close database connection: {:?}", err),
        }

        let rag_db: RagDb = RagDb {
            path: path.to_path_buf(),
            model_name: model_name.expect("No model name provided"),
            config: config.clone(),
            device: device.unwrap_or(String::from("npu")),
            embedding_npu_model: embeddingNpuModel.clone(),
            chunk_size,
        };

        let rag_db_toml = toml::to_string(&rag_db).expect("Could not encode TOML value");

        project_path.push("ragdb.toml");

        std::fs::write(project_path, rag_db_toml).expect("Error writing to .toml file");

        rag_db
    }

    fn delete_all(&mut self) {
        let mut project_path: PathBuf = self.path.clone();
        project_path.push(".ragdb");

        if project_path.exists() {
            std::fs::remove_dir_all(&project_path).expect("Failed to remove project directory.");
        }
        if self.embedding_npu_model.clone().is_some() {
            self.embedding_npu_model.clone().unwrap().unload_model();
        }

        let rag_db = RagDb::new(
            self.path.clone(),
            Some(self.model_name.clone()),
            self.config.clone(),
            Some(self.device.clone()),
            self.chunk_size,
        );
        *self = rag_db;
    }

    /// Load an existing RagDb project by reading the ragdb.toml file and returning the resulting
    /// RagDb object.
    fn load(path: PathBuf) -> RagDb {
        let mut project_path: PathBuf = path.clone();
        project_path.push(".ragdb");
        project_path.push("ragdb.toml");
        let data = match std::fs::read_to_string(project_path) {
            Ok(d) => d,
            Err(err) => {
                println!("Error reading ragdb.toml file: {:?}", err);
                exit(1);
            }
        };
        let rag_db: RagDb = match toml::from_str(&data) {
            Ok(c) => c,
            Err(err) => {
                println!(
                    "Error Generating RagDb object from ragdb.toml file: {:?}",
                    err
                );
                exit(1)
            }
        };
        rag_db
    }
}

mod test {
    use std::{thread::sleep, time};

    use crate::knowledge_base::{RagDb, Util};

    fn clean_tmp(i: i32) {
        //clean /tmp/test1
        if std::path::Path::new("./tmp/test")
            .join(i.to_string())
            .exists()
        {
            std::fs::remove_dir_all("./tmp").unwrap();
        }
        sleep(time::Duration::from_millis(1000));
    }

    fn test_rag_create(i: i32) {
        clean_tmp(i);

        // create folder ./tmp/test
        std::fs::create_dir_all("./tmp/test1").unwrap();

        //create rag at path ./tmp/test
        let rag_db = RagDb::new(std::path::PathBuf::from("./tmp/test1"), None, None, None, 256);
        assert_eq!(&rag_db.path.to_string_lossy().to_string(), "./tmp/test1");
        sleep(time::Duration::from_millis(1000));
    }

    #[test]
    fn test_rag_load() {
        test_rag_create(1);
        let rag_db = RagDb::load(std::path::PathBuf::from("./tmp/test1"));
        assert_eq!(&rag_db.path.to_string_lossy().to_string(), "./tmp/test1");
    }

    #[test]
    fn test_add_doc() {
        test_rag_create(2);
        //create rag at path ./tmp/test
        let rag_db = RagDb::new(
            std::path::PathBuf::from("./tmp/test2"),
            Some("./model".to_string()),
            None,
            256,
        );

        sleep(time::Duration::from_millis(1000));

        rag_db.add_document(std::path::PathBuf::from(
            "./test_assets/test_pdfs/iceland.pdf",
        ));

        let added_doc = rag_db.get_added_doc_paths();

        println!("Added Documents : {:?}", added_doc);
    }

    #[test]
    fn test_add_multi_doc() {
        test_rag_create(3);
        //create rag at path ./tmp/test
        let rag_db = RagDb::new(
            std::path::PathBuf::from("./tmp/test3"),
            Some("./model".to_string()),
            None,
            256,
        );

        sleep(time::Duration::from_millis(1000));

        rag_db.add_document(std::path::PathBuf::from(
            "./test_assets/test_pdfs/iceland.pdf",
        ));
        rag_db.add_document(std::path::PathBuf::from(
            "./test_assets/test_pdfs/qaic_embedding.pdf",
        ));

        let added_doc = rag_db.get_added_doc_paths();

        println!("Added Documents : {:?}", added_doc);
    }

    #[test]
    fn test_add_multi_doc_and_search() {
        test_rag_create(4);
        //create rag at path ./tmp/test
        let rag_db = RagDb::new(
            std::path::PathBuf::from("./tmp/test4"),
            Some("./model".to_string()),
            None,
            256,
        );

        sleep(time::Duration::from_millis(1000));

        rag_db.add_document(std::path::PathBuf::from(
            "./test_assets/test_pdfs/iceland.pdf",
        ));
        rag_db.add_document(std::path::PathBuf::from(
            "./test_assets/test_pdfs/qaic_embedding.pdf",
        ));

        let added_doc = rag_db.get_added_doc_paths();

        println!("Added Documents : {:?}", added_doc);

        let search_res = rag_db.search("Where is iceland".to_string(), 10).unwrap();

        println!("Search Results: \n");
        for res in search_res {
            println!("Document Name: {}\nLine: {}\n", res.document_name, res.line);
        }
    }
}