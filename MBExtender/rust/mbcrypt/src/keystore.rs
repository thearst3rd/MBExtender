extern crate rsa;
extern crate rsa_der;
#[warn(non_snake_case)]
pub struct KeyStore {
    pub publickey: rsa::RSAPublicKey,
    pub aeskey: Vec<u8>,
}

impl KeyStore {
    pub fn Load() -> KeyStore {
        let publickey = r#"
-----BEGIN RSA PUBLIC KEY-----
MIGeMA0GCSqGSIb3DQEBAQUAA4GMADCBiAKBgHjoRejckfgUTePKXGOxHPGUl8ns
dVbY0qqG+JDpsI2faCJus1EIX6dM0D0H2z5IV0U+LgX7TeyEPHh20UpvcCYzJDFQ
l2S+SB6xaXbZMXbCZZfGZlz+lXnizyliTrSoZJ1cByI7j3OftLDGrFblcdHjdntb
4sHbos8R2UqFTIs9AgMBAAE=
-----END RSA PUBLIC KEY-----"#;

        let der_encoded = publickey.lines().filter(|line| !line.starts_with("-")).fold(
            String::new(),
            |mut data, line| {
                data.push_str(&line);
                data
            },
        );
        let der_bytes = vec![
            255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
            255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
            255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
            255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
            255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
            255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
            255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
            255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
            255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
            255, 255, 255, 255, 255, 255, 255, 255,
        ];
        let (n, e) = rsa_der::public_key_from_der(&der_bytes).unwrap();
        let public_key = rsa::RSAPublicKey::new(
            rsa::BigUint::from_bytes_be(&n),
            rsa::BigUint::from_bytes_be(&e),
        )
        .unwrap();

        let aeskey: Vec<u8> = vec![
            255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
            255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        ];

        let keystore = KeyStore {
            publickey: public_key,
            aeskey: aeskey,
        };
        return keystore;
    }
}
