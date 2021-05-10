use std::ffi::{CStr, CString};
use std::os::raw::c_char;

#[repr(C)]
pub struct _StringTable;

tge_methods! {
    pub fn tge_stringtable_insert(this: &mut _StringTable, string: *const c_char, caseSens: bool) -> *const c_char = tge_addr!(0x403314, 0x4AFF0);
}

tge_statics! {
    pub static mut STRING_TABLE: &mut _StringTable = tge_addr!(0x6A4FD0, 0x2DA55C);
}

impl _StringTable {
    pub fn insert(&mut self, string: &str, case_sens: bool) -> &str {
        let c_string = CString::new(string).unwrap();
        unsafe {
            let retr = mcall!(tge_stringtable_insert, self, c_string.as_ptr(), case_sens);
            let retstr = CStr::from_ptr(retr).to_str().unwrap();
            return retstr;
        }
    }
    pub fn insert_raw(&mut self, string: &str, case_sens: bool) -> *const c_char {
        let c_string = CString::new(string).unwrap();
        unsafe {
            let retr = mcall!(tge_stringtable_insert, self, c_string.as_ptr(), case_sens);
            return retr;
        }
    }
}
