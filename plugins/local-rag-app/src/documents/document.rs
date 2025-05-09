/*
**************************************************************************************************
* Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
**************************************************************************************************
*/

use minidom::Element;

use lopdf::Document as lopdoc;
use zip::read::ZipArchive;

use std::ffi::{OsStr, OsString};
use std::path::PathBuf;

use std::fs::{read_dir, read_to_string, DirEntry, File, Metadata, ReadDir};
use std::io::{Read, Result};

use std::collections::VecDeque;

use std::panic::catch_unwind;

type DocResult<T> = std::result::Result<T, UnsupportedDocumentError>;
use text_splitter::TextSplitter;

use std::io::{self, BufRead};
use std::path::Path;
/// SQL Error
#[derive(Debug, Clone)]
pub struct UnsupportedDocumentError;

///Enum containing supported documents.
pub enum DocType {
    Pdf,
    Docx,
    Txt,
    Unsupported,
}

///Document Struct meant to hold information about a document.
#[derive(Debug, Clone)]
pub struct Document {
    pub name: String,
    pub metadata: Option<Metadata>,
    pub path: PathBuf,
    pub data: Option<Vec<String>>,
    pub page_numbers: Option<Vec<u32>>,
    pub encodings: Option<Vec<Vec<f32>>>,
}

impl Document {
    /// Returns true if the document is of supported type
    pub fn is_supported(&self) -> bool {
        let splits: (&str, &str) = self.name.rsplit_once('.').unwrap();
        let supported_types: [&str; 3] = ["pdf", "docx", "txt"];
        supported_types.contains(&splits.1)
    }
    /// Returns the extension of a file/document as a string
    pub fn get_extension(&self) -> Result<String> {
        Ok(self.name.rsplit_once('.').unwrap().1.to_string())
    }

    /// Get the document type of  a Document
    pub fn get_doc_type(&self) -> DocType {
        let extension: (&str, &str) = self.name.rsplit_once('.').unwrap();
        match extension.1 {
            "pdf" => DocType::Pdf,
            "docx" => DocType::Docx,
            "txt" => DocType::Txt,
            _ => DocType::Unsupported,
        }
    }

    ///
    pub fn get_document_path_as_string(&self) -> Result<String> {
        Ok(self.path.clone().to_string_lossy().to_string())
    }
    /// Returns a string containing document name
    pub fn get_document_name_as_string(&self) -> Result<String> {
        Ok(self.name.clone())
    }
    /// Set document data
    pub fn set_document_data(&mut self, data: Vec<String>) {
        self.data = Some(data);
    }

    /// Set page numbers
    pub fn set_page_numbers(&mut self, page_numbers: Vec<u32>) {
        self.page_numbers = Some(page_numbers);
    }
    
    pub fn get_document_data(&self) -> &Option<Vec<String>> {
        &self.data
    }

    pub fn get_page_numbers(&self) -> &Option<Vec<u32>> {
        &self.page_numbers
    }

    pub fn get_document_data_as_string(&self) -> DocResult<String> {
        let doctype: DocType = self.get_doc_type();
        match doctype {
            DocType::Docx => match get_text_from_docx(self.path.to_string_lossy().to_string()) {
                Some(doc_text_result) => Ok(doc_text_result),
                None => Err(UnsupportedDocumentError),
            },
            DocType::Pdf => match get_text_from_pdf(self.path.to_string_lossy().to_string()) {
                Some(doc_text_result) => Ok(doc_text_result),
                None => Err(UnsupportedDocumentError),
            },
            DocType::Txt => match get_text_from_txt(self.path.to_string_lossy().to_string()) {
                Some(doc_text_result) => Ok(doc_text_result),
                None => Err(UnsupportedDocumentError),
            },
            DocType::Unsupported => Err(UnsupportedDocumentError),
        }
    }
}

pub fn get_file_from_name(file_name: String) -> Option<Document> {
    Some(Document {
        name: file_name.clone(),
        metadata: None,
        path: PathBuf::from(file_name),
        data: None,
        page_numbers: None,
        encodings: None,
    })
}

///Description: get_file() will get the file and create a Document object and store it in a vector. This function will then return a vector
pub fn get_file(path: &PathBuf) -> Option<Document> {
    if path.is_file() {
        let file_metadata: Option<Metadata> = Some(path.metadata().unwrap());
        let file_name: OsString = path.file_name().unwrap().into();
        let file_path: PathBuf = path.clone();
        match file_name.into_string() {
            Ok(fname) => Some(Document {
                name: fname,
                metadata: file_metadata,
                path: file_path,
                data: None,
                page_numbers:None,
                encodings: None,
            }),
            Err(os_str) => {
                return Some(Document {
                    name: os_str.to_string_lossy().to_string(),
                    metadata: file_metadata,
                    path: file_path,
                    data: None,
                    page_numbers: None,
                    encodings: None,
                })
            }
        }
    } else {
        println!("Provided path is not a file");
        None
    }
}

///Description: get_file_list() will get all the files in the current directory, create a Document object, and store it in a vector. This function will then return that vector.
pub fn get_file_list(path: &PathBuf) -> Option<Vec<Document>> {
    let path_objects: ReadDir = read_dir(path).unwrap();
    let mut file_list: Vec<Document> = vec![];
    for path_object in path_objects {
        let object = match path_object {
            Ok(obj) => obj,
            Err(err) => {
                println!("Error reading file: {:?}", err);
                continue;
            }
        };
        let file_metadata: Option<Metadata> = Some(object.metadata().unwrap());
        let file_name: OsString = object.file_name();
        let file_path: PathBuf = object.path();

        if file_path.is_file() && is_supported(&file_name) {
            match file_name.into_string() {
                Ok(fname) => file_list.push(Document {
                    name: fname,
                    metadata: file_metadata,
                    path: file_path,
                    data: None,
                    page_numbers: None,
                    encodings: None,
                }),
                Err(os_str) => file_list.push(Document {
                    name: os_str.to_string_lossy().to_string(),
                    metadata: file_metadata,
                    path: file_path,
                    data: None,
                    page_numbers: None,
                    encodings: None,
                }),
            }
        }
    }

    Some(file_list)
}

///This function return True if the file format is supported by this program. It takes in OsStr and converts that
///into a String type via lossy conversion.
pub fn is_supported(file_name: &OsStr) -> bool {
    let split_str: String = file_name.to_string_lossy().to_string();
    let splits = split_str.rsplit_once('.').unwrap();
    let supported_types: [&str; 3] = ["pdf", "docx", "txt"];
    supported_types.contains(&splits.1)
}

///Given a Document, this function will determine if the Document is supported or not via the is_supported function from
///the document, and if it is supported then call the appropriate text extraction function to extract text, then split the
///text into chunks based on the chunk_size parameter and return it as a vector of String.
pub fn get_file_text(doc: &Document, chunk_size: usize) -> Option<Vec<String>> {
    let file_path: String = doc.get_document_path_as_string().ok()?;
    let file_text_option = match doc.get_doc_type() {
        DocType::Pdf => get_text_from_pdf_with_pages(file_path),
        DocType::Docx => get_text_from_docx_with_pages(file_path),
        DocType::Txt => get_text_from_txt_with_pages(file_path),
        DocType::Unsupported => return None,
    };

    let splitter = TextSplitter::new(chunk_size);
    let text_vec: Vec<String> = file_text_option?.iter().flat_map(|text| {
        splitter.chunks(text).map(|chunk| chunk.to_string()).collect::<Vec<_>>()
    }).collect();

    Some(text_vec)
}

pub fn get_file_text_with_pages(doc: &Document, chunk_size: usize) -> Option<(Vec<String>, Vec<u32>)> {
    let file_path: String = match doc.get_document_path_as_string() {
        Ok(doc_path) => doc_path,
        Err(_err) => return None,
    };
    let file_text_option: Option<Vec<String>> = match doc.get_doc_type() {
        DocType::Pdf => get_text_from_pdf_with_pages(file_path),
        DocType::Docx => get_text_from_docx_with_pages(file_path),
        DocType::Txt => get_text_from_txt_with_pages(file_path),
        DocType::Unsupported => return None,
    };
    println!("File Data: {:#?}", file_text_option);
    let splitter = TextSplitter::new(chunk_size);
    let mut text_vec: Vec<String> = vec![];
    let mut page_vec: Vec<u32> = vec![];
    match file_text_option {
        Some(text_by_pages) => {
            for (page_number, text) in text_by_pages.iter().enumerate() {
                let chunks_iter = splitter.chunks(&text);
                let chunks: Vec<String> = chunks_iter.map(|s| s.to_string()).collect();
                text_vec.extend(chunks.clone());
                page_vec.extend(vec![(page_number + 1) as u32; chunks.len()]);
            }
        },
        None => {
            text_vec = vec![String::from("   ")];
        }
    }
    Some((text_vec, page_vec))
}

/// Split a string of text into a vector of substring of length specified by chunk_size.
///  # Example
///  ```
///let my_string:String = String::from("Split this string!");
///let split_string:Vec<String> = ragdb::documents::document::split_text_into_chunks(my_string, 5).unwrap();
///assert_eq!(split_string, vec!["Split", " this"," stri","ng!"]);
///  ```
pub fn split_text_into_chunks(text: String, chunk_size: usize) -> Result<Vec<String>> {
    let text_vector: Vec<String> = text
        .as_bytes()
        .chunks(chunk_size)
        .map(|chunk| String::from_utf8_lossy(chunk).to_string())
        .collect::<Vec<_>>();
    Ok(text_vector)
}

///Description: This function reads text form a .txt file and returns it as an Option<String>.
pub fn get_text_from_txt(file_path: String) -> Option<String> {
    let text_result = read_to_string(file_path);
    match text_result {
        Ok(text) => Some(text),
        Err(_err) => None,
    }
}

/// Extracts text from a .txt file as a single page
pub fn get_text_from_txt_with_pages(file_path: String) -> Option<Vec<String>> {
    let path = Path::new(&file_path);
    let file = File::open(&path).ok()?;
    let reader = io::BufReader::new(file);
    let mut result: Vec<String> = Vec::new();
    let mut current_page: String = String::new();

    for line in reader.lines() {
        let line = line.ok()?;
        current_page.push_str(&line);
        current_page.push('\n');
    }

    result.push(current_page);
    Some(result)
}

///This function aims to get text data from a word file. The file_path is passed in as string,
///and then from there using zip's ZipArchive the files within docx file are read. We get the document.xml file
///and then using minidom we extract the text data from the file using breadth first traversal, and return it as string
pub fn get_text_from_docx(file_path: String) -> Option<String> {
    let mut result: String = String::new();
    let mut xml_string: String = String::new();

    let file: File = match File::open(file_path) {
        Ok(f) => f,
        Err(_e) => return None,
    };

    let mut zip_reader: ZipArchive<_>;
    match ZipArchive::new(file) {
        Ok(zp) => zip_reader = zp,
        Err(_err) => return None,
    }
    let mut document_xml_file: zip::read::ZipFile<'_, std::fs::File>;
    match zip_reader.by_name("word/document.xml") {
        Ok(zpf) => document_xml_file = zpf,
        Err(_err) => return None,
    }

    let _outcome: std::result::Result<usize, std::io::Error> =
        document_xml_file.read_to_string(&mut xml_string);
    let element: Element = xml_string.parse().unwrap();
    let mut node_que: VecDeque<&Element> = VecDeque::new();
    let mut _text_string: String = String::new();
    node_que.push_back(&element);

    while let Some(node) = node_que.pop_front() {
        //Breadth First Traversal of XML Tree
        if node.name() == "t" {
            result.push_str(&node.text());
            result.push('\n');
        }
        for child in node.children() {
            node_que.push_back(child);
        }
    }
    if result.is_empty() {
        //In case the string is empty
        result.push_str("   ");
    }
    Some(result)
}

/// Extracts text from a Word file (.docx) with pages
pub fn get_text_from_docx_with_pages(file_path: String) -> Option<Vec<String>> {
    use quick_xml::Reader;
    use quick_xml::events::Event;
    
    let file = File::open(&file_path).ok()?;
    let mut archive = ZipArchive::new(file).ok()?;
    
    let mut document_xml = String::new();
    archive.by_name("word/document.xml").ok()?.read_to_string(&mut document_xml).ok()?;
    
    let mut reader = Reader::from_str(&document_xml);
    reader.trim_text(true);
    
    let mut buf = Vec::new();
    let mut pages: Vec<String> = vec![String::new()];
    let mut current_text = String::new();
    let mut page_breaks_found = false;
    
    loop {
        match reader.read_event(&mut buf) {
            Ok(Event::Eof) => break, // Exit loop on end of file
            Ok(Event::Text(e)) => {
                current_text.push_str(&String::from_utf8_lossy(&e.unescaped().unwrap_or_default()));
            }
            Ok(Event::End(ref e)) if e.name().as_ref() as &[u8] == b"w:p" => {
                pages.last_mut().unwrap().push_str(&current_text);
                pages.last_mut().unwrap().push('\n');
                current_text.clear();
            }
            Ok(Event::Empty(ref e)) | Ok(Event::Start(ref e)) if e.name().as_ref() as &[u8] == b"w:br" => {
                let is_page_break = e.attributes().any(|a| {
                    if let Ok(attr) = a {
                        attr.key.as_ref() as &[u8] == b"w:type" && attr.value.as_ref() as &[u8] == b"page"
                    } 
                    else {
                        false
                    }
                });
                if is_page_break {
                    page_breaks_found = true;
                    pages.push(String::new());
                }
            }
            Ok(_) => {} // Ignore other events
            Err(e) => {
                eprintln!("Error while parsing DOCX XML: {:?}", e);
                break;
            }
        }
        buf.clear();
    }
    
    if !page_breaks_found {
    let joined = pages.concat();
    return Some(vec![joined]);
    }
    
    Some(pages.into_iter().filter(|s| !s.trim().is_empty()).collect())
    }

///Description: This function extracts text from a pdf file file via the pdf_extract crate.  
pub fn get_text_from_pdf(file_path: String) -> Option<String> {
    // with catch unwind try pdf_extract if panic then try read_pdf
    match catch_unwind(|| pdf_extract::extract_text(&file_path)) {
        Ok(Ok(text)) => Some(text),
        _ => match catch_unwind(|| read_pdf(&file_path)) {
            Ok(Ok(text)) => Some(text),
            _ => None,
        },
    }
}

pub fn get_text_from_pdf_with_pages(file_path: String) -> Option<Vec<String>> {
    // with catch unwind try pdf_extract if panic then try read_pdf
    match catch_unwind(|| pdf_extract::extract_text_by_pages(&file_path)) {
        Ok(Ok(text_vector)) => Some(text_vector),
        Err(err) => {
            println!("Caught a panic: {:?}", err);
            None
        },
        Ok(Err(err)) =>  {
            println!("Caught a panic: {:?}", err);
            None
        },
    }
}

pub fn read_pdf(pdf_file: &String) -> Result<String> {
    // Read the PDF file
    let doc = lopdoc::load(pdf_file).unwrap();

    // Get all pages in the PDF file
    let pages = doc.get_pages();
    let mut pdf_page_nums: Vec<u32> = Vec::new();
    let mut pdf_texts: Vec<String> = Vec::new();

    // Traverse through the PDF pages to extract text
    for (i, _) in pages.iter().enumerate() {
        let page_number: u32 = (i + 1) as u32;

        // Extract text from a single PDF page
        let text: String = doc.extract_text(&[page_number]).unwrap();

        pdf_page_nums.push(page_number);
        pdf_texts.push(text);
    }

    Ok(pdf_texts.join("\n\n"))
}

/// Read a PDF files to extract its contents
///
/// ## Input Parameters
/// - `pdf_file` contains the PDF file to be read
///
/// ## Returns
/// - Extract text from PDF file
// pub fn read_pdf(pdf_file: &String) -> Result<(Vec<u32>, Vec<String>)> {
//     // Read the PDF file
//     let doc = lopdoc::load(&pdf_file).unwrap();

//     // Get all pages in the PDF file
//     let pages = doc.get_pages();
//     let mut pdf_page_nums: Vec<u32> = Vec::new();
//     let mut pdf_texts: Vec<String> = Vec::new();

//     // Traverse through the PDF pages to extract text
//     for (i, _) in pages.iter().enumerate() {
//         let page_number: u32 = (i + 1) as u32;

//         // Extract text from a single PDF page
//         let text: String = doc.extract_text(&[page_number]).unwrap();

//         pdf_page_nums.push(page_number);
//         pdf_texts.push(text);
//     }

//     Ok((pdf_page_nums, pdf_texts))
// }

mod test {

    use std::path::PathBuf;
    #[test]
    fn test_get_file_list() {
        let path: PathBuf = PathBuf::from("./iceland.pdf");
        assert!(path.exists());

        let path: PathBuf = PathBuf::from("./test_file.pdf");
        assert!(path.exists());
    }

    #[test]
    fn test_parse() {
        let path: PathBuf = PathBuf::from("./iceland.pdf");
        assert!(path.exists());
        //extract text
        let text_result = super::get_text_from_pdf(path.to_string_lossy().to_string());
        println!("{:#?}", text_result);

        let path: PathBuf = PathBuf::from("./test_file.pdf");
        assert!(path.exists());
        //extract text
        let text_result = super::get_text_from_pdf(path.to_string_lossy().to_string());
        println!("{:#?}", text_result);
    }

    #[test]
    fn test_lopdf() {
        let path: PathBuf = PathBuf::from("./iceland.pdf");
        assert!(path.exists());
        //extract text
        let text_result = super::read_pdf(&path.to_string_lossy().to_string());
        println!("{:#?}", text_result);

        let path: PathBuf = PathBuf::from("./test_file.pdf");
        assert!(path.exists());
        //extract text
        let text_result = super::read_pdf(&path.to_string_lossy().to_string());
        println!("{:#?}", text_result);
    }
}
