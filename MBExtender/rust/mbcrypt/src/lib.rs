use mbpakfile::{MBPakFile, MBPakFileEntry};
use mbx::con::{self, SimObject};
use mbx::core;
use mbx::platform;
use mbx::prelude::*;
use memorystream::MemoryStream;
use std::sync::Arc;
use std::{
    ffi::{CStr, CString},
    fmt::format,
};
use std::{os::raw::c_char, sync::Mutex};

mod keystore;
mod mbpakfile;
mod memorystream;
#[macro_use]
extern crate lazy_static;

struct OpenPak {
    file: Arc<core::File>,
    stream: memorystream::MemoryStream,
}

lazy_static! {
    static ref loadedPackages: Mutex<Vec<mbpakfile::MBPakFile>> = Mutex::new(Vec::new());
    static ref openPakFiles: Mutex<Vec<OpenPak>> = Mutex::new(Vec::new());
}

#[command(args = 2, usage = "loadMBPackage(package)")]
fn loadMBPackage(_obj: *mut SimObject, argc: i32, argv: con::Argv) {
    let args = con::collect_args(argc, argv);
    let packagename = String::from(args[1]);
    let zipn = packagename.clone() + ".mbpak";
    let path = format!("packages/{}", zipn);

    let pak = mbpakfile::MBPakFile::new(&path, keystore::KeyStore::Load());
    if pak.is_ok() {
        loadedPackages.lock().unwrap().push(pak.unwrap());
        con::print(format!("Package {} loaded", packagename));
    } else {
        con::error(format!("Could not load package {}", packagename));
    }
}

#[command(args = 2, usage = "unloadMBPackage(package)")]
fn unloadMBPackage(_obj: *mut SimObject, argc: i32, argv: con::Argv) {
    let args = con::collect_args(argc, argv);
    let packagename = String::from(args[1]);
    let indexToRemove = loadedPackages.lock().unwrap().iter().position(|x| x.name == packagename);
    match indexToRemove {
        Some(i) => {
            loadedPackages.lock().unwrap().remove(i);
        }
        None => {}
    };
}

#[command(args = 2, usage = "isMBPackageLoaded")]
fn isMBPackageLoaded(_obj: *mut SimObject, argc: i32, argv: con::Argv) -> *const c_char {
    let args = con::collect_args(argc, argv);
    let packagename = String::from(args[1]);
    let indexToRemove = loadedPackages.lock().unwrap().iter().position(|x| x.name == packagename);
    match indexToRemove {
        Some(i) => con::get_return_buffer("1"),
        None => con::get_return_buffer("0"),
    }
}

fn searchEntry(path: &str) -> Option<MBPakFileEntry> {
    let workingdir = platform::get_current_directory();
    let relativedir = str::replace(path, workingdir, "");
    for package in loadedPackages.lock().unwrap().iter() {
        for entry in package.entries.iter() {
            if entry.file_path.starts_with(&relativedir) {
                let value = entry.clone();
                return Some(value);
            }
        }
    }
    return None;
}

#[method_override(original_file_open)]
unsafe fn file_open(
    this: &mut core::File,
    filename: *const c_char,
    openMode: core::AccessMode,
) -> core::FileStatus {
    let mut opk = openPakFiles.lock().unwrap();
    let index = opk.iter().position(|x| std::ptr::eq(x.file.as_ref(), this));
    if index.is_some() {
        opk.remove(index.unwrap());
    }

    let fname = CStr::from_ptr(filename).to_str().unwrap();
    for package in loadedPackages.lock().unwrap().iter() {
        for entry in package.entries.iter() {
            if entry.file_path == fname {
                if openMode == core::AccessMode::Read {
                    let data = package.ReadFile(entry.file_path.clone()).unwrap();
                    let mem = MemoryStream::new(&data);
                    let opkdata = OpenPak {
                        file: Arc::from_raw(this),
                        stream: mem,
                    };
                    opk.push(opkdata);
                    this.setStatus(core::FileStatus::Ok);
                    this.setCapabilities(1);
                    return core::FileStatus::Ok;
                } else {
                    this.setStatus(core::FileStatus::IOError);
                    this.setCapabilities(0);
                    return core::FileStatus::IOError;
                }
            }
        }
    }

    original_file_open(this, filename, openMode)
}

#[method_override(original_file_close)]
unsafe fn file_close(this: &mut core::File) -> core::FileStatus {
    let mut opk = openPakFiles.lock().unwrap();
    let index = opk.iter().position(|x| std::ptr::eq(x.file.as_ref(), this));
    if index.is_some() {
        opk.remove(index.unwrap());
    }
    original_file_close(this)
}

#[method_override(original_file_getposition)]
unsafe fn file_getposition(this: &mut core::File) -> u32 {
    let opk = openPakFiles.lock().unwrap();
    let index = opk.iter().position(|x| std::ptr::eq(x.file.as_ref(), this));
    if index.is_some() {
        let op = &opk[index.unwrap()];
        return op.stream.tell() as u32;
    }
    original_file_getposition(this)
}

#[method_override(original_file_setposition)]
unsafe fn file_setposition(
    this: &mut core::File,
    position: i32,
    absolutePos: bool,
) -> core::FileStatus {
    let mut opk = openPakFiles.lock().unwrap();
    let index = opk.iter().position(|x| std::ptr::eq(x.file.as_ref(), this));
    if index.is_some() {
        let op = &mut opk[index.unwrap()];
        if absolutePos {
            op.stream.seek(position as usize);
        } else {
            op.stream.seek(op.stream.tell() + (position as usize));
        }
        if op.stream.length() == op.stream.tell() {
            this.setStatus(core::FileStatus::EOS);
            return core::FileStatus::EOS;
        }
        this.setStatus(core::FileStatus::Ok);
        return core::FileStatus::Ok;
    }
    original_file_setposition(this, position, absolutePos)
}

#[method_override(original_file_getsize)]
unsafe fn file_getsize(this: &mut core::File) -> u32 {
    let opk = openPakFiles.lock().unwrap();
    let index = opk.iter().position(|x| std::ptr::eq(x.file.as_ref(), this));
    if index.is_some() {
        let op = &opk[index.unwrap()];
        return op.stream.length() as u32;
    }
    original_file_getsize(this)
}

#[method_override(original_file_read)]
unsafe fn file_read(
    this: &mut core::File,
    size: u32,
    dst: *mut u8,
    bytesRead: *mut u32,
) -> core::FileStatus {
    let mut opk = openPakFiles.lock().unwrap();
    let index = opk.iter().position(|x| std::ptr::eq(x.file.as_ref(), this));
    if index.is_some() {
        let op = &mut opk[index.unwrap()];
        let stream = &mut op.stream;
        let retbytes: &[u8];
        if stream.tell() + (size as usize) > stream.length() {
            *bytesRead = (stream.length() - stream.tell()) as u32;
            retbytes = stream.readBytes(*bytesRead as usize).unwrap();
            std::ptr::copy(retbytes.as_ptr(), dst, *bytesRead as usize);
            this.setStatus(core::FileStatus::EOS);
            return core::FileStatus::EOS;
        } else {
            *bytesRead = size;
            retbytes = stream.readBytes(size as usize).unwrap();
        }
        std::ptr::copy(retbytes.as_ptr(), dst, *bytesRead as usize);
        this.setStatus(core::FileStatus::Ok);
        return core::FileStatus::Ok;
    }
    original_file_read(this, size, dst, bytesRead)
}

#[method_override(original_file_dtor)]
unsafe fn file_dtor(this: *const core::File) {
    let mut opk = openPakFiles.lock().unwrap();
    let index = opk.iter().position(|x| std::ptr::eq(x.file.as_ref(), this));
    if index.is_some() {
        opk.remove(index.unwrap());
    }
    original_file_dtor(this)
}

fn combine(left: &str, right: &str) -> String {
    if left.is_empty() {
        return String::from(right);
    } else if right.is_empty() {
        return String::from(left);
    }
    let leftEnd = left.chars().nth(left.len() - 1).unwrap();
    let rightStart = right.chars().nth(0).unwrap();
    if leftEnd == '/' || rightStart == '/' {
        return format!("{}{}", left, right);
    } else {
        return format!("{}/{}", left, right);
    }
}

#[fn_override(original_platform_dumppath)]
unsafe fn platform_dumppath(
    path: *const c_char,
    fileVector: &mut core::TgeVec<platform::FileInfo>,
) -> bool {
    let ret = original_platform_dumppath(path, fileVector);
    let pathstr = CStr::from_ptr(path).to_str().unwrap();

    let workingdir = platform::get_current_directory();
    let relativedir = str::replace(pathstr, workingdir, "");
    for package in loadedPackages.lock().unwrap().iter() {
        for entry in package.entries.iter() {
            if entry.file_path.starts_with(&relativedir) {
                let file = entry;

                let a = file.file_path.rfind('/').unwrap();
                let mut dpath = &file.file_path[..a];
                dpath = &dpath[relativedir.len()..];
                let thispath = combine(pathstr, dpath);
                let fpath = std::path::Path::new(file.file_path.as_str());
                let fname = fpath.file_name().unwrap().to_str().unwrap();

                let f = platform::FileInfo {
                    filesize: file.uncompressed_size as u32,
                    filename: core::STRING_TABLE.insert_raw(fname, false),
                    fullpath: core::STRING_TABLE.insert_raw(thispath.as_str(), true),
                };

                fileVector.push(f);
            }
        }
    }
    return fileVector.len() != 0;
}

#[fn_override(original_platform_get_filetimes)]
unsafe fn platform_get_filetimes(
    path: *const c_char,
    createTime: *mut platform::FileTime,
    modifyTime: *mut platform::FileTime,
) -> bool {
    let pathstr = CStr::from_ptr(path).to_str().unwrap();
    let search = searchEntry(pathstr);
    if search.is_some() {
        if !createTime.is_null() {
            (*createTime).low = 0;
            (*createTime).hi = 0;
        }

        if !modifyTime.is_null() {
            (*modifyTime).low = 0;
            (*modifyTime).hi = 0;
        }
        return true;
    }
    original_platform_get_filetimes(path, createTime, modifyTime)
}

#[fn_override(original_platform_is_subdirectory)]
unsafe fn platform_is_subdirectory(parent: *const c_char, child: *const c_char) -> bool {
    let workingdir = platform::get_current_directory();
    let mut relativedir = CStr::from_ptr(parent).to_str().unwrap();
    let replacer = str::replace(relativedir, workingdir, "");
    relativedir = replacer.as_str();
    let dirStr = format!("{}{}", relativedir, CStr::from_ptr(child).to_str().unwrap());
    for package in loadedPackages.lock().unwrap().iter() {
        for entry in package.entries.iter() {
            if entry.file_path.starts_with(&dirStr) {
                return true;
            }
        }
    }
    return original_platform_is_subdirectory(parent, child);
}

#[plugin_main]
fn main(plugin: &Plugin) -> Result<(), &'static str> {
    con::add_command(&loadMBPackage);

    plugin.intercept(
        platform::tge_platform_is_subdirectory,
        platform_is_subdirectory,
        &original_platform_is_subdirectory,
    )?;

    plugin.intercept(
        platform::tge_platform_get_filetimes,
        platform_get_filetimes,
        &original_platform_get_filetimes,
    )?;

    plugin.intercept(core::tge_file_open, file_open, &original_file_open)?;
    plugin.intercept(core::tge_file_close, file_close, &original_file_close)?;
    plugin.intercept(core::tge_file_getposition, file_getposition, &original_file_getposition)?;
    plugin.intercept(core::tge_file_setposition, file_setposition, &original_file_setposition)?;
    plugin.intercept(core::tge_file_read, file_read, &original_file_read)?;
    plugin.intercept(core::tge_file_getsize, file_getsize, &original_file_getsize)?;
    plugin.intercept(core::tge_file_dtor, file_dtor, &original_file_dtor)?;
    plugin.intercept(
        platform::tge_platform_dumppath,
        platform_dumppath,
        &original_platform_dumppath,
    )?;

    Ok(())
}
