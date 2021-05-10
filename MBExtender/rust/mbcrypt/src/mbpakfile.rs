#![allow(dead_code)]

use crate::keystore;
use crate::memorystream;
extern crate aes;
extern crate blake3;
extern crate block_modes;
extern crate compress;
extern crate hex;
extern crate rsa;
extern crate sha2;
use aes::Aes128;
use aes::Aes256;
use block_modes::block_padding::Pkcs7;
use block_modes::{BlockMode, Cbc};
use log::trace;
use rsa::PublicKey;
use sha2::Digest;
use std::fs::File;
use std::io;
use std::io::prelude::*;
use std::io::SeekFrom;
#[warn(non_snake_case)]
pub struct MBPakFileEntry {
    pub file_path: String,
    pub uncompressed_size: u64,
    pub compressed_size: u32,
    pub file_offset: u64,
    pub encrypted: bool,
}

impl Clone for MBPakFileEntry {
    fn clone(&self) -> Self {
        MBPakFileEntry {
            file_path: self.file_path.clone(),
            uncompressed_size: self.uncompressed_size,
            compressed_size: self.compressed_size,
            file_offset: self.file_offset,
            encrypted: self.encrypted,
        }
    }
}

pub struct MBPakFile {
    pub name: String,
    pub path: String,
    pub entries: Vec<MBPakFileEntry>,
    pub key: Vec<u8>,
    pub key_store: keystore::KeyStore,
}

impl MBPakFile {
    pub fn new(
        path: &String,
        keys: keystore::KeyStore,
    ) -> std::result::Result<MBPakFile, &'static str> {
        let file_path = std::path::Path::new(path);
        let pakname = String::from(file_path.file_name().unwrap().to_str().unwrap());
        let file = match std::fs::File::open(&file_path) {
            Err(e) => panic!("Couldnt open file {}: {}", file_path.display(), e),
            Ok(file) => file,
        };

        let mut reader = std::io::BufReader::new(file);
        let mut data = Vec::new();
        let _readsize = reader.read_to_end(&mut data);

        let mut stream = memorystream::MemoryStream::new(&data);

        let mut pak = MBPakFile {
            path: path.clone(),
            key_store: keys,
            key: vec![],
            entries: vec![],
            name: pakname,
        };

        let res = pak.read_header(&mut stream);
        if res.is_ok() {
            return Ok(pak);
        } else {
            return Err(res.err().unwrap());
        }
    }

    pub fn read_header(
        &mut self,
        reader: &mut memorystream::MemoryStream,
    ) -> std::result::Result<(), &'static str> {
        let key_len = reader.readU32().unwrap();
        for _i in 0..key_len {
            self.key.push(reader.readU8().unwrap());
        }

        let entry_count = reader.readU32().unwrap();

        for _i in 0..entry_count {
            let filepath = reader.readstring().unwrap();
            let is_encrypted = reader.readU8().unwrap() == 1;
            let fileoffset = reader.readU64().unwrap();
            let uncompressed_size = reader.readU64().unwrap();
            let compressed_size = reader.readU32().unwrap();

            let entry = MBPakFileEntry {
                file_path: filepath,
                file_offset: fileoffset,
                encrypted: is_encrypted,
                uncompressed_size: uncompressed_size,
                compressed_size: compressed_size,
            };

            self.entries.push(entry);
        }

        let offset = reader.tell();

        for entry in self.entries.iter_mut() {
            entry.file_offset += offset as u64;
        }

        let remaining = reader.length() - reader.tell();

        let buf = reader.readBytes(remaining).unwrap();

        if !self.verifysignature(buf) {
            return Err("Invalid signature");
        }
        return Ok(());
    }

    fn verifysignature(&self, buffer: &[u8]) -> bool {
        let padding = rsa::PaddingScheme::new_pkcs1v15_sign(Option::from(rsa::Hash::SHA2_256));
        let pubkey = &self.key_store.publickey;
        let blakehash = blake3::hash(buffer);
        let blakebytes = blakehash.as_bytes();
        let mut sha = sha2::Sha256::new();
        sha.update(blakebytes);
        let hash = sha.finalize();
        let hashbytes = &hash[..];

        let res = pubkey.verify(padding, hashbytes, &self.key);
        if res.is_ok() {
            return true;
        } else {
            return false;
        }
    }

    fn decrypt(
        &self,
        filepath: String,
        key: Vec<u8>,
    ) -> std::result::Result<Vec<u8>, &'static str> {
        let mut entry: Option<&MBPakFileEntry> = None;
        let mut found: bool = false;
        for i in 0..self.entries.len() {
            if self.entries[i].file_path == filepath {
                entry = Some(&self.entries[i]);
                found = true;
                break;
            }
        }
        if found {
            let entryval = entry.unwrap();
            let file = std::fs::File::open(&self.path).unwrap();
            let mut reader = std::io::BufReader::new(file);
            reader.seek(std::io::SeekFrom::Start(entryval.file_offset));
            let mut buf: Vec<u8> = vec![0; entryval.compressed_size as usize];
            reader.read_exact(&mut buf);

            let mut memstr = memorystream::MemoryStream::new(&buf);
            let ivlen = memstr.readU32().unwrap();
            let mut iv: Vec<u8> = Vec::with_capacity(ivlen as usize);
            for i in 0..ivlen {
                let byte = memstr.readU8().unwrap();
                iv.push(byte);
            }

            let mut encrypteddata = Vec::new();

            while memstr.tell() != memstr.length() {
                encrypteddata.push(memstr.readU8().unwrap());
            }

            type AesCBC = block_modes::Cbc<aes::Aes256, block_modes::block_padding::Pkcs7>;
            let aes = AesCBC::new_from_slices(&key, &iv).unwrap();
            let decrypteddata = aes.decrypt(&mut encrypteddata).unwrap();

            let returndata = Vec::from(decrypteddata.clone());

            return Ok(returndata);
        } else {
            return Err("Not found");
        }
    }

    pub fn ReadFile(&self, filepath: String) -> std::result::Result<Vec<u8>, &'static str> {
        let mut entry: Option<&MBPakFileEntry> = None;
        let mut found: bool = false;
        for i in 0..self.entries.len() {
            if self.entries[i].file_path == filepath {
                entry = Some(&self.entries[i]);
                found = true;
                break;
            }
        }
        if found {
            let buf = self.decrypt(filepath, self.key_store.aeskey.clone()).unwrap();
            let cur = std::io::Cursor::new(buf);
            let mut uncompressed = Vec::new();
            compress::zlib::Decoder::new(cur).read_to_end(&mut uncompressed);
            return Ok(uncompressed);
        } else {
            return Err("Cant find file");
        }
    }
}
