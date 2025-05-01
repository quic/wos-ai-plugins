# local-rag-app
Local Rust Based App for RAG (Embedding and Search)

## Build the project

1. Download LLVM from https://github.com/llvm/llvm-project/releases/download/llvmorg-19.1.7/LLVM-19.1.7-win64.exe. and Set $env:LIBCLANG_PATH = "C:\Program Files\LLVM\lib"
Alo set "C:\Program Files\LLVM\bin" path to PATH
2. Install Rust and Cargo
3. Download Arm64 toolchain from VisualStudio
4. Build using cargo commands
```
    rustup target add aarch64-pc-windows-msvc
    cargo build --release --target aarch64-pc-windows-msvc
```
or build using below script
```
./build_plugin.ps1
```

## Fixing build issues
### 1. TLS error while downloading ort
```
thread 'main' panicked at build.rs:20:31:
  Failed to GET `https://parcel.pyke.io/v2/delivery/ortrs/packages/msort-binary/1.17.0/ortrs-msort_static-v1.17.0-aarch64-pc-windows-msvc.tgz`: https://parcel.pyke.io/v2/delivery/ortrs/packages/msort-binary/1.17.0/ortrs-msort_static-v1.17.0-aarch64-pc-windows-msvc.tgz: Connection Failed: tls connection init failed: invalid peer certificate: UnknownIssuer
```

As a workaround, download the https://parcel.pyke.io/v2/delivery/ortrs/packages/msort-binary/1.17.0/ortrs-msort_static-v1.17.0-aarch64-pc-windows-msvc.tgz 
and place onnxruntime at `C:\Users\<user>\AppData\Local\ort.pyke.io\dfbin\aarch64-pc-windows-msvc\27DDC61E1416E3F1BC6137C8365B563F73BA5A6CE8D7008E5CD4E36B4F037FDA\onnxruntime`



## How to start the RAG API server
1. Download the binary from the releases page.
2. Run the binary with the following command:
    For NPU:
    ```powershell
    local_rag_app.exe server --port 8080 --model-name BGELargeENV15NPU_Int8 --project-path .\data\ --chunk-size 256
    ```

    For CPU:
    ```powershell
    local_rag_app.exe server --port 8080 --model-name BGELargeENV15 --project-path .\data\ --chunk-size 256 --device cpu
    ```

3. The server will be running on http://localhost:8080.
4. You can use any tool like Postman to send requests to the server.

## Supported Model List on NPU
```
/// v1.5 release of the large English model
BGELargeENV15NPU_Int8,
BGELargeENV15NPU_Fp16,
```

## Supported Model List on CPU
```
/// Sentence Transformer model, MiniLM-L6-v2
AllMiniLML6V2,
/// v1.5 release of the base English model
BGEBaseENV15,
/// Quantized v1.5 release of the base English model
BGEBaseENV15Q,
/// v1.5 release of the large English model
BGELargeENV15,
/// Quantized v1.5 release of the large English model
BGELargeENV15Q,
/// Fast and Default English model
BGESmallENV15,
/// Quantized Fast and Default English model
BGESmallENV15Q,
/// 8192 context length english model
NomicEmbedTextV1,
/// v1.5 release of the 8192 context length english model
NomicEmbedTextV15,
/// Quantized v1.5 release of the 8192 context length english model
NomicEmbedTextV15Q,
/// Multi-lingual model
ParaphraseMLMiniLML12V2,
/// Quantized Multi-lingual model
ParaphraseMLMiniLML12V2Q,
/// Sentence-transformers model for tasks like clustering or semantic search
ParaphraseMLMpnetBaseV2,
/// v1.5 release of the small Chinese model
BGESmallZHV15,
/// Small model of multilingual E5 Text Embeddings
MultilingualE5Small,
/// Base model of multilingual E5 Text Embeddings
MultilingualE5Base,
/// Large model of multilingual E5 Text Embeddings
MultilingualE5Large,
/// Large English embedding model from MixedBreed.ai
MxbaiEmbedLargeV1,
/// Quantized Large English embedding model from MixedBreed.ai
MxbaiEmbedLargeV1Q,
```


Rust API Documentation
======================

Overview
--------

The provided Rust application is an HTTP server built using Axum framework. It exposes several endpoints to interact with a knowledge base system.

Endpoints
---------
### POST /document_content

*   Description: Add a new document content to the database.
*   Request Body: `UploadDocumentContentPayload`
*   Response Body: `UploadDocumentResponse`

#### UploadDocumentPayload

`{   "file_name": "<File name>" , "content": "File content" }`

#### UploadDocumentResponse

`{   "message": "Document uploaded successfully" }`

### POST /document

*   Description: Add a new document to the database.
*   Request Body: `UploadDocumentPayload`
*   Response Body: `UploadDocumentResponse`

#### UploadDocumentPayload

`{   "file_path": "<path_to_the_document>" }`

#### UploadDocumentResponse

`{   "message": "Document uploaded successfully" }`

### GET /document

*   Description: Retrieve a list of all documents in the database.
*   Response Body: `GetDocumentsResponse`

#### GetDocumentsResponse

`{   "documents": ["<document_name>", "<document_name>", ...] }`

### POST /delete\_all

*   Description: Deletes all documents from the database.
*   Response Body: `DeleteAllResponse`

#### DeleteAllResponse

`{   "message": "Deleted all documents successfully" }`

### POST /search

*   Description: Perform a search query against the documents in the database.
*   Request Body: `SearchRequest`
*   Response Body: `SearchResponse`

#### SearchRequest

`{   "query": "<search_query>",   "num_chunks": <number_of_results> }`

#### SearchResponse

```json
{   
    "results": 
    [
        {       
            "document_name": "<document_name>",       
            "chunk": "<relevant_text>",       
            "page_number": 1
        },
        ...   
    ] 
}
```


## Usage Example


### GET ALL documents
GET http://localhost:8080/document


### Add Document
POST http://localhost:8080/document
Content-Type: application/json

{
    "file_path": "C:\\Users\\user\\DEV\\norway.pdf"
}


### Add Document content
POST http://localhost:8080/document_content
Content-Type: application/json

{
    "file_name": "Celebrity Dayout Quotation.pdf",
    "content" : " \n \nQUOTATION ..."
}


### Add Document
POST http://localhost:8080/document
Content-Type: application/json

{
    "file_path": "C:\\Users\\user\\iceland.pdf"
}

### Search 
POST http://localhost:8080/search
Content-Type: application/json

{
    "query": "When did iceland start to form?",
    "num_chunks": 4
}


### Search
POST http://localhost:8080/search
Content-Type: application/json

{
    "query": "How to perform inference",
    "num_chunks": 30
}

### Detele All
POST http://localhost:8080/delete_all
