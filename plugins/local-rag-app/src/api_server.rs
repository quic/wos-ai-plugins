/*
**************************************************************************************************
* Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
**************************************************************************************************
*/

use async_lock::RwLock;
use std::{path::PathBuf, str::FromStr, sync::Arc};

use crate::knowledge_base::{RagDb, Util};
use anyhow::Result;
use axum::{
    extract::State,
    routing::{get, post},
    Json, Router,
};
use serde::{Deserialize, Serialize};
use tower_http::cors::{CorsLayer, Any};
use http::{Method};
use http::header::{CONTENT_TYPE};

use tower::ServiceBuilder;

pub type RagState = Arc<RwLock<RagDb>>;

pub fn load_or_create_db(
    path: String,
    model_name: Option<String>,
    config: Option<String>,
    device: Option<String>,
    chunk_size: usize,
) -> Result<RagState> {
    let state = Arc::new(RwLock::new(RagDb::new(
        PathBuf::from_str(&path)?,
        model_name,
        config,
        device,
        chunk_size,
    )));
    Ok(state)
}

pub async fn start_api_server(
    port: u16,
    project_path: String,
    model_name: Option<String>,
    config: Option<String>,
    device: Option<String>,
    chunk_size: usize,
) -> Result<()> {
    // initialize tracing
    println!("Starting API server");
    let state = load_or_create_db(project_path, model_name, config, device, chunk_size)?;

    let cors_layer = CorsLayer::new()
        .allow_origin(Any)  // Open access to selected route
        .allow_methods([Method::GET, Method::POST])
        .allow_headers([CONTENT_TYPE]);

    let app = Router::new()
        .route("/document", post(handle_doc_upload))
        .route("/document_content", post(handle_doc_upload_content))
        .route("/document", get(handle_get_docs))
        .route("/delete_all", post(handle_delete_all)) // <-- New route for delete_all
        .route("/search", post(handle_search))
        .layer(ServiceBuilder::new().layer(cors_layer))
        .with_state(state);
        
    println!("{}", format!("Listening at http://localhost:{port}..."));
    let listener = tokio::net::TcpListener::bind(format!("0.0.0.0:{port}"))
        .await
        .unwrap();
    axum::serve(listener, app).await.unwrap();

    Ok(())
}

#[derive(Serialize, Deserialize, Debug)]
struct UploadDocumentPayload {
    file_path: String,
}

#[derive(Serialize, Deserialize, Debug)]
struct UploadDocumentContentPayload {
    file_name: String,
    content: String,
}

#[derive(Serialize, Debug, Deserialize)]
struct UploadDocumentResponse {
    message: String,
}


// Define handler function that takes a reference to the Request object as its argument
async fn handle_doc_upload_content(
    State(rag_db): State<RagState>,
    Json(payload): Json<UploadDocumentContentPayload>,
) -> Json<UploadDocumentResponse> {
    // Extract data from the payload

    let file_name = &payload.file_name;
    let content = &payload.content;

    let rag_db_lock = rag_db.write_arc().await;

    (*rag_db_lock).add_document_content(file_name.to_string(), content.to_string());

    // Return success message wrapped in Result type
    Json(UploadDocumentResponse {
        message: "Document uploaded successfully".to_string().to_string(),
    })
}

// Define handler function that takes a reference to the Request object as its argument
async fn handle_doc_upload(
    State(rag_db): State<RagState>,
    Json(payload): Json<UploadDocumentPayload>,
) -> Json<UploadDocumentResponse> {
    // Extract data from the payload
    let file_path = &payload.file_path;

    let rag_db_lock = rag_db.write_arc().await;

    (*rag_db_lock).add_document(PathBuf::from_str(file_path).unwrap());

    // Return success message wrapped in Result type
    Json(UploadDocumentResponse {
        message: "Document uploaded successfully".to_string().to_string(),
    })
}

// Handler to search
async fn handle_search(
    State(rag_db): State<RagState>,
    Json(payload): Json<SearchRequest>,
) -> Json<SearchResponse> {
    let rag_db_lock = rag_db.read_arc().await;
    let search_matches = (*rag_db_lock).search(payload.query, payload.num_chunks.try_into().unwrap());
    let search_response: Vec<(String, String, u32)> = search_matches
        .unwrap()
        .iter()
        .map(|x| (x.document_name.clone(), x.line.clone(), x.page_number.clone()))
        .collect();
    let list_search_item = search_response.iter().map(|x| SearchItem {
        document_name: x.0.clone(),
        chunk: x.1.clone(),
        page_number: x.2.clone(),
    });
    Json(SearchResponse {
        results: list_search_item.collect(),
    })
}

#[derive(Serialize, Deserialize, Debug)]
struct SearchRequest {
    query: String,
    num_chunks: i32,
}

#[derive(Serialize, Deserialize, Debug)]
struct SearchResponse {
    results: Vec<SearchItem>,
}

#[derive(Serialize, Deserialize, Debug)]
struct SearchItem {
    document_name: String,
    chunk: String,
    page_number: u32,
}

#[derive(Serialize, Deserialize, Debug)]
struct GetDocumentsResponse {
    documents: Vec<String>,
}

// Handler to get all documents
async fn handle_get_docs(State(rag_db): State<RagState>) -> Json<GetDocumentsResponse> {
    let rag_db_lock = rag_db.read_arc().await;

    let added_doc_paths = (*rag_db_lock).get_added_doc_paths();
    Json(GetDocumentsResponse {
        documents: added_doc_paths,
    })
}

// Define handler function that takes a reference to the Request object as its argument
async fn handle_delete_all(State(rag_db): State<RagState>) -> Json<DeleteAllResponse> {
    let mut rag_db_lock = rag_db.write_arc().await;

    (*rag_db_lock).delete_all();

    // Return success message wrapped in Result type
    Json(DeleteAllResponse {
        message: "Deleted all documents successfully".to_string().to_string(),
    })
}

#[derive(Serialize, Debug, Deserialize)]
struct DeleteAllResponse {
    message: String,
}
