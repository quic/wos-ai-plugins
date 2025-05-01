use std::fmt;

/// SQL Error
#[derive(Debug, Clone)]
pub struct KnowledgeBaseError;

impl fmt::Display for KnowledgeBaseError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "invalid sql transaction or connection")
    }
}
