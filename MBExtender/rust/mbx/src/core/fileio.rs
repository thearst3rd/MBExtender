use std::os::raw::c_char;

#[repr(C)]
pub enum FileStatus {
    Ok = 0,       //< Ok!
    IOError,      //< Read or Write error
    EOS,          //< End of Stream reached (mostly for reads)
    IllegalCall,  //< An unsupported operation used. Always accompanied by AssertWarn
    Closed,       //< Tried to operate on a closed stream (or detached filter)
    UnknownError, //< Catchall
}

#[repr(C)]
#[derive(PartialEq)]
pub enum AccessMode {
    Read = 0,        //< Open for read only, starting at beginning of file.
    Write = 1, //< Open for write only, starting at beginning of file; will blast old contents of file.
    ReadWrite = 2, //< Open for read-write.
    WriteAppend = 3, //< Write-only, starting at end of file.
}

tge_methods! {
    pub fn tge_file_open(this: &mut File, filename: *const c_char, openMode: AccessMode) -> FileStatus = tge_addr!(0x4019A1, 0x1EA350);
    pub fn tge_file_getposition(this: &mut File) -> u32 = tge_addr!(0x405E8E, 0x1E8D30);
    pub fn tge_file_setposition(this: &mut File, position: i32, absolutePos: bool) -> FileStatus = tge_addr!(0x404B8D, 0x1E9D70);
    pub fn tge_file_getsize(this: &mut File) -> u32 = tge_addr!(0x40889B, 0x1E94B0);
    pub fn tge_file_close(this: &mut File) -> FileStatus = tge_addr!(0x406D93, 0x1E9FA0);
    pub fn tge_file_read(this: &mut File, size: u32, dst: *mut u8, bytesRead: *mut u32) -> FileStatus = tge_addr!(0x4090FC, 0x1EA030);
    pub fn tge_file_write(this: &mut File, size: u32, src: *const u8, bytesWritten: *mut u32) -> FileStatus = tge_addr!(0x4017D5, 0x1EA0F0);
    pub fn tge_file_dtor(this: *const File) = tge_addr!(0x404B10, 0x1E9C50);
}

#[repr(C)]
pub struct File;

impl File {
    pub fn setStatus(&mut self, status: FileStatus) {
        unsafe { *field_ptr!(mut self, FileStatus, tge_addr!(0x8,0x8)) = status };
    }

    pub fn setCapabilities(&mut self, capabilities: u32) {
        unsafe { *field_ptr!(mut self, u32, tge_addr!(0xC,0xC)) = capabilities };
    }
}
