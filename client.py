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
    while True:
        files = get_filenames(foldername)
        filesums = generate_dict(files)
        filename = os.path.basename(files[1])
        string = filename + " " + filesums[files[1]]
        recieved = s.recv(3).decode()
        if(recieved == READY):
            print("in")
            s.send(string.encode())

        data=s.recv(1024)
        print(data.decode())

    s.close()
