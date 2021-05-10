use std::ffi::{CStr, CString};
use std::os::raw::c_char;
use std::ptr;

use crate::core::TgeVec;

#[repr(C)]
pub struct FileTime {
    pub low: u32,
    pub hi: u32,
}

#[repr(C)]
pub struct FileInfo {
    pub fullpath: *const c_char,
    pub filename: *const c_char,
    pub filesize: u32,
}

tge_functions! {
    pub fn tge_platform_get_current_directory() -> *const c_char = tge_addr!(0x402F7C, 0x1E8E10);
    pub fn tge_platform_is_subdirectory(parent: *const c_char, child: *const c_char) -> bool = tge_addr!(0x4088A0, 0x1E8EE0);
    pub fn tge_platform_get_filetimes(path: *const c_char, createTime: *mut FileTime, modifyTime: *mut FileTime) -> bool = tge_addr!(0x4033D2, 0x1E97E0);
    pub fn tge_platform_dumppath(path: *const c_char, fileVector: &mut TgeVec<FileInfo>) -> bool = tge_addr!(0x403AF3, 0x1E9390);
}

pub fn get_current_directory() -> &'static str {
    unsafe {
        let cwd = tge_platform_get_current_directory();
        CStr::from_ptr(cwd).to_str().unwrap()
    }
}
