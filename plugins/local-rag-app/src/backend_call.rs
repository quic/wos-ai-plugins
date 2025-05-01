use libloading::{Library, Symbol};
use std::ffi::c_void;

fn main() -> Result<(), Box<dyn std::error::Error>> {
    // Attempt to load the DLL
    let lib = match Library::new("Genie.dll") {
        Ok(lib) => lib,
        Err(e) => {
            eprintln!("Failed to load DLL: {}", e);
            return Err(Box::new(e));
        }
    };

    // Define the function signature
    unsafe {
        let func: Symbol<unsafe extern "C" fn() -> i32> = match lib.get(b"your_function_name") {
            Ok(func) => func,
            Err(e) => {
                eprintln!("Failed to get function: {}", e);
                return Err(Box::new(e));
            }
        };
        
        // Call the function
        let result = func();
        println!("Function result: {}", result);
    }

    Ok(())
}