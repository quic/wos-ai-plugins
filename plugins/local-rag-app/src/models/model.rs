use anyhow::Result;
use fastembed::{EmbeddingModel, InitOptions, TextEmbedding};
use backtrace::Backtrace;
use crate::npu_backend::EmbeddingNpuModel;

/*
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
*/

///Convert a string to an EmbeddingModel enum variant.
fn convert_from_string(model_name: String) -> EmbeddingModel {
    match model_name.as_str() {
        "AllMiniLML6V2" => EmbeddingModel::AllMiniLML6V2,
        "BGESmallENV15" => EmbeddingModel::BGESmallENV15,
        "NomicEmbedTextV15" => EmbeddingModel::NomicEmbedTextV15,
        "ParaphraseMLMpnetBaseV2" => EmbeddingModel::ParaphraseMLMpnetBaseV2,
        "BGESmallZHV15" => EmbeddingModel::BGESmallZHV15,
        "MultilingualE5Small" => EmbeddingModel::MultilingualE5Small,
        "MultilingualE5Base" => EmbeddingModel::MultilingualE5Base,
        "MultilingualE5Large" => EmbeddingModel::MultilingualE5Large,
        "MxbaiEmbedLargeV1" => EmbeddingModel::MxbaiEmbedLargeV1,
        "MxbaiEmbedLargeV1Q" => EmbeddingModel::MxbaiEmbedLargeV1Q,
        _ => EmbeddingModel::AllMiniLML6V2,
    }
}

///Description: Generating Vector encoding_model using sentence_transformers models using rust-bert.
///Use Case: Use this to create embeddings for sentences.   
pub fn encode_text(text: &Vec<String>, model_name: String, embedding_npu_model: Option<EmbeddingNpuModel>) -> Result<Vec<(String, Vec<f32>)>> {
    let mut encoded_texts = Vec::new();
    match embedding_npu_model {
        Some(npu_model) => {
            println!("Running on NPU!");
            for phrase in text {
                let encoded_phrase: Option<Vec<f32>> = npu_model.generate(phrase.to_string());
                // println!("encoded_phrase f32 vec size: {}", encoded_phrase.clone().unwrap().len());
                // print!("e : {:?}", encoded_phrase.clone().unwrap());
                encoded_texts.push(encoded_phrase.unwrap());
            }
        }
        None => {
            println!("No NPU model found, falling back on CPU");
            let model: EmbeddingModel= convert_from_string(model_name);
            let model = TextEmbedding::try_new(InitOptions {
                // model_name: EmbeddingModel::AllMiniLML6V2,
                model_name: model,
                show_download_progress: true,
                ..Default::default()
            })?;

            // split the text vec to batch size of 256
            let batches = text
                .chunks(256)
                .map(|batch| batch.to_vec())
                .collect::<Vec<Vec<String>>>();

            for batch in batches {
                // println!("###################### Batch ############");
                // let batch_clone = batch.clone();
                // for s in batch {
                //     println!("s: {}", s);
                // }
                let encoded_batch = model.embed(batch, None)?;
                // let batch_clone = encoded_batch.clone();
                // println!("encoded_batch_clone len: {}", batch_clone.len());
                // for s in batch_clone {
                //     println!("e: {:?}", s);
                // }
                encoded_texts.extend(encoded_batch);
            }
        }
    }
    
    // return vec of tuples (text, vector)
    // zip the text vec and encoded_text vec
    let zipped = text.iter().zip(encoded_texts).collect::<Vec<(_, _)>>();
    let zipped_resp = zipped
        .iter()
        .map(|(a, b)| (a.to_string(), b.clone()))
        .collect();
    Ok(zipped_resp)
}
