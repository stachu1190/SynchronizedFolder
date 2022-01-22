import socket
from hashlib import md5
import os
from constant import READY

def get_filenames(foldername):
    files = []
    for filename in os.listdir(foldername):
        files.append(os.path.join(foldername, filename))
    return files

def file_hash(filename):
    with open(filename, "rb") as a_file:
        md5_hash = md5()        
        content = a_file.read()
        md5_hash.update(content)
        digest = md5_hash.hexdigest()
        return digest


def generate_dict(files):
    filesums = dict()
    for name in files:
        filesums[name] = file_hash(name)
    return filesums
    

foldername = "../folder1"
files = get_filenames(foldername)

HOST="127.0.0.1"
PORT=1100



with socket.socket(socket.AF_INET,socket.SOCK_STREAM) as s:
    s.connect((HOST,PORT))
    
    files = get_filenames(foldername)
    filesums = generate_dict(files)
    status = s.recv(3).decode()
    if(status == READY):
        s.send((str(len(files)).zfill(8)).encode())
    for i in range(len(files)):
        filename = os.path.basename(files[i])
        string = filename + "\n" + filesums[files[i]]
        status = s.recv(3).decode()
        if(status == READY):
            s.send((str(len(string)).zfill(8)).encode())
            status = s.recv(3).decode()
            if(status == READY):
                print(string)
                s.send(string.encode())

    s.close()
