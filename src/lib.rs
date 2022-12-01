use ctor::ctor;
use simplelog::*;
use std::{
    ffi::{c_char, CString},
    path::Path,
};
mod dir_walker;

fn pchar_to_str<'a>(s: *const c_char) -> Option<&'a str> {
    if s.is_null() {
        None
    } else {
        unsafe { Some(std::ffi::CStr::from_ptr(s).to_str().ok()?) }
    }
}

fn get_path<'a>(path: *const c_char) -> Option<&'a Path> {
    pchar_to_str(path).map(|x| Path::new(x))
}

fn walk_game_dir<T, F>(path: &Path, cb: F) -> Option<T>
where
    F: Fn(i32, &str) -> Option<T>,
{
    let walker = dir_walker::DirWalker::from_patterns(path, &["**/*.ide"]);
    for entry in walker {
        let Ok(e) = entry else {
            continue;
        };
        let Ok(content) = std::fs::read_to_string(&e) else {
            log::error!("Failed to read file: {}", e.display());
            continue;
        };
        let Ok(ide) = gta_ide_parser::parse(&content) else {
            log::error!("Failed to parse IDE: {}", e.display());
            continue;
        };
        for (section_name, lines) in ide {
            if ["txdp", "path", "2dfx"].contains(&section_name.as_str()) {
                continue;
            }
            for line in lines {
                let Ok(id) = line[0].parse::<i32>() else {
                    log::error!("Failed to parse ID: {} in {}", line[0], e.display());
                    continue;
                };
                match cb(id, line[1]) {
                    Some(x) => return Some(x),
                    None => continue,
                }
            }
        }
    }
    None
}

#[no_mangle]
pub extern "C" fn GetObjectID(objname: *const c_char, path: *const c_char) -> i32 {
    let Some(game_dir) = get_path(path) else {
        log::error!("Invalid input path");
        return -1;
    };
    let Some(obj_name) = pchar_to_str(objname) else {
        log::error!("Invalid input object name");
        return -1;
    };
    log::debug!(
        "Searching the model with name {obj_name} in {}",
        game_dir.display()
    );
    match walk_game_dir(game_dir, |id, name| -> Option<i32> {
        if name.eq_ignore_ascii_case(obj_name) {
            return Some(id);
        }
        return None;
    }) {
        Some(id) => id,
        None => {
            log::error!("Failed to find the model with name {}", obj_name);
            -1
        }
    }
}

#[no_mangle]
pub extern "C" fn GetObjectName(id: i32, path: *const c_char, buf: *mut c_char) -> i32 {
    let Some(game_dir) = get_path(path) else {
        log::error!("Invalid input path");
        return -1;
    };
    log::debug!("Searching the model with id {id} in {}", game_dir.display());
    match walk_game_dir(game_dir, |id2, name| -> Option<CString> {
        if id == id2 {
            return Some(CString::new(name).unwrap());
        }
        return None;
    }) {
        Some(s) => unsafe {
            libc::strncpy(buf, s.as_ptr(), 32);
            return 1;
        },
        None => {
            log::error!("Failed to find the model with id {}", id);
            return -1;
        }
    }
}

#[ctor]
fn main() {
    if cfg!(debug_assertions) {
        let config = ConfigBuilder::new()
            .set_level_padding(LevelPadding::Off)
            .set_time_to_local(true)
            .set_thread_level(LevelFilter::Off)
            .build();

        let _ = WriteLogger::init(
            LevelFilter::max(),
            config,
            std::fs::File::create("GetObjectID.log").unwrap(),
        );
    }
}
