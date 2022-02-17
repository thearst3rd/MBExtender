import pathlib
import os
import io
import sys
import zlib
import random
import struct
import base64
from blake3 import *
from Crypto.Cipher import AES
from Crypto.PublicKey import RSA
from Crypto.Util.Padding import pad
from Crypto.Signature import pkcs1_15
from Crypto.Hash import SHA256


class MBPakFileEntry:
    def __init__(self):
        self.filePath: str = ""
        self.uncompressedSize: int = 0
        self.compressedContents: bytes = None
        self.fileOffset: int = 0
        self.encrypted: bool = False

    @staticmethod
    def makeEntry(rootPath: str, filePath: str):
        """
        Generates a pak file entry from given path, its not encrypted though
        """
        entry = MBPakFileEntry()
        entry.filePath = filePath.replace(rootPath, "").replace(os.path.sep, "/")
        entry.filePath = entry.filePath.lstrip("/")

        with open(filePath, "rb") as f:
            data = f.read()
            entry.uncompressedSize = len(data)
            entry.compressedContents = zlib.compress(data)

        return entry

    def encrypt(self, key: bytes):
        """
        Encrypted the entry using the aes key
        """
        if not self.encrypted:
            # iv is random so bruh
            iv = random.randbytes(16)  # python 3.9 bruh
            aes = AES.new(key, AES.MODE_CBC, iv)
            # encrypt the thing
            outputdata = aes.encrypt(pad(self.compressedContents, aes.block_size))
            with io.BytesIO() as finalstream:
                # we'll include the iv with the encrypted data cause bruh
                finalstream.write(struct.pack("i", len(aes.iv)))
                finalstream.write(aes.iv)
                finalstream.write(outputdata)
                # and put the data back
                self.compressedContents = finalstream.getvalue()
            self.encrypted = True

    def decrypt(self, key: bytes):
        """
        Decrypt the data using the aes key
        """
        if self.encrypted:
            # prepare to read the iv and stuff
            stream = io.BytesIO(self.compressedContents)
            (ivlen,) = struct.unpack("i", stream.read(4))
            iv = stream.read(ivlen)
            # now the encrypted data
            encrypteddata = stream.read(len(self.compressedContents) - stream.tell())
            aes = AES.new(key, AES.MODE_CBC, iv)
            # actually decrypt it now
            outputdata = aes.decrypt(encrypteddata)
            self.compressedContents = outputdata
            self.encrypted = False


class MBPakFile:
    def __init__(self):
        self.entries: list[MBPakFileEntry] = []

    @staticmethod
    def create(dirpath: str, key: bytes):
        """
        Creates a pak file from directory
        """
        # recursively get all the files
        filelist = pathlib.Path(dirpath).rglob("*")

        pak = MBPakFile()
        for file in filelist:
            if os.path.isfile(file):  # only files, we said
                # make the entry
                entry = MBPakFileEntry.makeEntry(dirpath, str(file))
                # encrypt it
                entry.encrypt(key)
                # and push it
                pak.entries.append(entry)

        return pak

    def write(self, outstream: io.FileIO, rsakey):
        """
        Writes the pak data to a file stream, data is signed using the rsa public/private key pair
        """

        # preprare the data buffer
        databuffer = None
        with io.BytesIO() as datastream:
            for entry in self.entries:
                entry.fileOffset = datastream.tell()
                datastream.write(entry.compressedContents)
                databuffer = datastream.getvalue()

        # hash the thing for signing
        blakehash = blake3(databuffer).digest()

        hash = SHA256.new(blakehash)

        publickey, privatekey = rsakey

        # sign the thing
        signer = pkcs1_15.new(privatekey)
        sign = signer.sign(hash)

        # write the signature
        outstream.write(struct.pack("i", len(sign)))
        outstream.write(sign)

        # write the entriees
        outstream.write(struct.pack("i", len(self.entries)))
        for entry in self.entries:
            outstream.write(struct.pack("b", len(entry.filePath)))
            outstream.write(entry.filePath.encode(encoding="utf-8"))
            outstream.write(struct.pack("b", 1 if entry.encrypted else 0))
            outstream.write(struct.pack("q", entry.fileOffset))
            outstream.write(struct.pack("q", entry.uncompressedSize))
            outstream.write(struct.pack("i", len(entry.compressedContents)))

        outstream.write(databuffer)

    def read(self, instream: io.FileIO, rsakey):
        # read the signature
        (signlen,) = struct.unpack("i", instream.read(4))
        signature = instream.read(signlen)

        # read the entries
        (entrylen,) = struct.unpack("i", instream.read(4))
        for i in range(0, entrylen):
            entry = MBPakFileEntry()
            (filepathlen,) = struct.unpack("b", instream.read(1))
            entry.filePath = instream.read(filepathlen).decode(encoding="utf-8")
            (isencrypted,) = struct.unpack("b", instream.read(1))
            entry.encrypted = True if isencrypted == 1 else False
            (entry.fileOffset,) = struct.unpack("q", instream.read(8))
            (entry.uncompressedSize,) = struct.unpack("q", instream.read(8))
            entry.compressedContents = [
                struct.unpack("i", instream.read(4))[0]
            ]  # temporary store
            entry.encrypted = True
            self.entries.append(entry)

        databuffer = instream.read()

        publickey, privatekey = rsakey

        # now verify integrity
        if self.verifysign(databuffer, publickey, signature):
            # now fill up the compressed contents and shit
            for entry in self.entries:
                datalen = entry.compressedContents[0]  # retrieve what we stored
                entry.compressedContents = databuffer[
                    entry.fileOffset : entry.fileOffset + datalen
                ]
        else:
            raise Exception("TAMPERED DATA")

    def verifysign(self, databuffer, publickey, sign):
        verifier = pkcs1_15.new(publickey)
        # blakehash = blake3(databuffer).digest()
        hash = SHA256.new(databuffer)

        try:
            verifier.verify(hash, sign)
            return True
        except ValueError:
            return False


# SAMPLE KEY FILE, no newline at the end
# -----BEGIN RSA PRIVATE KEY-----
# RSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEY
# RSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEY
# RSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEY
# RSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEY
# RSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEY
# RSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEY
# RSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEY
# RSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEY
# RSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEY
# RSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEY
# RSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEY
# RSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEY
# RSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEY
# -----END RSA PRIVATE KEY-----
# -----BEGIN RSA PUBLIC KEY-----
# RSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEY
# RSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEY
# RSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEY
# RSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEYRSAKEY
# -----END RSA PUBLIC KEY-----
# -----BEGIN AES KEY-----
# SHA256HASH
# -----END AES KEY-----
def parsekeyfile(file):
    publickey, privatekey, aeskey = None, None, None
    with open(file, "r") as f:
        lines = f.readlines()

        startindex = lines.index("-----BEGIN RSA PRIVATE KEY-----\n")
        endindex = lines.index("-----END RSA PRIVATE KEY-----\n")
        pemfile = "".join(lines[startindex : endindex + 1])
        pemfile = pemfile.rstrip("\n")
        privatekey = RSA.import_key(pemfile)

        startindex = lines.index("-----BEGIN RSA PUBLIC KEY-----\n")
        endindex = lines.index("-----END RSA PUBLIC KEY-----\n")
        pemfile = "".join(lines[startindex : endindex + 1])
        pemfile = pemfile.rstrip("\n")
        publickey = RSA.import_key(pemfile)

        startindex = lines.index("-----BEGIN AES KEY-----\n")
        endindex = lines.index("-----END AES KEY-----")
        aeskey = base64.b64decode("".join(lines[startindex + 1 : endindex]))

    return publickey, privatekey, aeskey


if __name__ == "__main__":
    print("MBCrypt 1.1")
    print("Packs files or folders into a single mbpak file to be read by PQ")

    mode: int = 0
    dir: str = ""
    key: str = ""

    for i in range(0, len(sys.argv)):
        if sys.argv[i] == "-e":
            mode = 0
        if sys.argv[i] == "-d":
            mode = 1

        if sys.argv[i] == "-i":
            dir = sys.argv[i + 1]

        if sys.argv[i] == "-k":
            key = sys.argv[i + 1]

    publickey, privatekey, aeskey = parsekeyfile(key)

    if mode == 0:
        dirpath = os.path.abspath(dir)
        pak = MBPakFile.create(dirpath, aeskey)

        with open(dir + ".mbpak", "wb") as f:
            pak.write(f, (publickey, privatekey))

    if mode == 1:
        pak = MBPakFile()
        with open(dir, "rb") as f:
            pak.read(f, (publickey, privatekey))

        for entry in pak.entries:
            entry.decrypt(
                aeskey
            )  # this would do nothing if the thing is already decrypted
            data = zlib.decompress(
                entry.compressedContents, zlib.MAX_WBITS, entry.uncompressedSize
            )
            pathdir = os.path.dirname(entry.filePath)
            if not os.path.exists(pathdir):
                os.makedirs(pathdir)
            with open(entry.filePath, "wb") as f:
                f.write(data)
