#![allow(dead_code)]
pub struct MemoryStream {
    buffer: Vec<u8>,
    position: usize,
}
#[warn(non_snake_case)]

impl MemoryStream {
    pub fn new(frombuf: &Vec<u8>) -> MemoryStream {
        MemoryStream {
            buffer: frombuf.clone(),
            position: 0,
        }
    }

    pub fn tell(&self) -> usize {
        return self.position;
    }

    pub fn length(&self) -> usize {
        return self.buffer.len();
    }

    pub fn seek(&mut self, pos: usize) {
        self.position = pos;
    }

    fn checkEOF(&self) -> std::result::Result<(), &'static str> {
        let bufsize = self.buffer.len();
        if self.position >= bufsize {
            return Err("Out of Stream");
        }
        return Ok(());
    }

    pub fn readBytes(&mut self, num: usize) -> std::result::Result<&[u8], &'static str> {
        if self.position + num > self.buffer.len() {
            return Err("Out of range");
        }
        let bytes = &self.buffer[self.position..(self.position + num)];
        self.position += num;
        return Ok(bytes);
    }

    pub fn readU8(&mut self) -> std::result::Result<u8, &'static str> {
        let res = self.checkEOF();
        if res.is_ok() {
            let byte = self.buffer[self.position];
            self.position += 1;
            return Ok(byte);
        } else {
            return Err(res.unwrap_err());
        }
    }

    pub fn readU16(&mut self) -> std::result::Result<u16, &'static str> {
        let res = self.checkEOF();
        if res.is_ok() {
            let mut num: u16 = 0;
            for i in 0..2 {
                let byte = self.readU8().unwrap();
                num += (byte as u16) << (8 * i);
            }
            return Ok(num);
        } else {
            return Err(res.unwrap_err());
        }
    }

    pub fn readU32(&mut self) -> std::result::Result<u32, &'static str> {
        let res = self.checkEOF();
        if res.is_ok() {
            let mut num: u32 = 0;
            for i in 0..4 {
                let byte = self.readU8().unwrap();
                num += (byte as u32) << (8 * i);
            }
            return Ok(num);
        } else {
            return Err(res.unwrap_err());
        }
    }

    pub fn readU64(&mut self) -> std::result::Result<u64, &'static str> {
        let res = self.checkEOF();
        if res.is_ok() {
            let mut num: u64 = 0;
            for i in 0..8 {
                let byte = self.readU8().unwrap();
                num += (byte as u64) << (8 * i);
            }
            return Ok(num);
        } else {
            return Err(res.unwrap_err());
        }
    }

    pub fn readstring(&mut self) -> std::result::Result<String, &'static str> {
        let res = self.checkEOF();
        if res.is_ok() {
            let length = self.readU8().unwrap();
            let mut bytes = Vec::with_capacity(length as usize);
            for _ in 0..length {
                bytes.push(self.readU8().unwrap());
            }
            let string = String::from_utf8(bytes).unwrap();
            return Ok(string);
        } else {
            return Err(res.unwrap_err());
        }
    }
}
