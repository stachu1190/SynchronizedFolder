import socket
from hashlib import md5
import os
import sys
from constant import READY, SEND_FROM_CLIENT, SEND_FROM_SERVER, CONTINUE, UP_TO_DATE

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
def get_dates(files):
    filedates = dict()
    for name in files:
        filedates[name] = os.path.getmtime(name)
    return filedates

try:
  foldername = sys.argv[1]
except:
  exit(-1)
files = get_filenames(foldername)

HOST="127.0.0.1"
PORT=1100



with socket.socket(socket.AF_INET,socket.SOCK_STREAM) as s:
    s.connect((HOST,PORT))
    
    files = get_filenames(foldername)
    filesums = generate_dict(files)
    filedates = get_dates(files)
    status = s.recv(3).decode()
    if(status == READY):
        s.send((str(len(files)).zfill(8)).encode())
    for i in range(len(files)):
        filename = os.path.basename(files[i])
        print(filename + " " + filesums[files[i]])
        print("----------------------")
        string = filename + "\n" + filesums[files[i]] + "\n" + str(filedates[files[i]])
        status = s.recv(3).decode() # zawiesza
        print(status)
        if(status == READY):
            s.send((str(len(string)).zfill(8)).encode())
            status = s.recv(3).decode()
            print(status)
            if(status == READY):
                s.send(string.encode())
                status = s.recv(3).decode()
                print(status)
                if(status == SEND_FROM_CLIENT):
                    file = open(files[i],mode='r')
                    content = file.read()
                    s.send((str(len(content)).zfill(8)).encode())
                    parts = [content[i:i+2000] for i in range(0, len(content), 2000)]
                    for j in range(len(parts)):
                        status = s.recv(3).decode()
                        print(status)
                        s.send(parts[j].encode())
                    file.close()
                if(status == SEND_FROM_SERVER):
                    file = open(files[i], "w")
                    s.send(str(READY).encode())
                    msg = s.recv(2000).decode()
                    print("wiadomość", msg)
                    file.write(msg)
                    file.close()

        print("--------------------")
    while True:
        status = s.recv(3).decode()
        print(status)
        if status == SEND_FROM_SERVER:
            s.send(str(READY).encode())
            name = s.recv(100).decode()
            s.send(str(READY).encode())
            status = s.recv(3).decode()
            file = open(foldername + "/" + name, "w")
            if status == READY:
                s.send(str(READY).encode())
                msg = s.recv(2000).decode()
                file.write(msg)
                file.close()
            else:
                print("nie udało się otworzyć pliku")
                file.close()
        if status == UP_TO_DATE:
            print("Up to date")
            input()
    

    s.close()
