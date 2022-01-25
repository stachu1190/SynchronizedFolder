#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <csignal>

#define READY "101"
#define CONTINUE "102"
#define UP_TO_DATE "103"
#define SEND_FROM_CLIENT "201"
#define SEND_FROM_SERVER "202"

std::vector<std::vector<std::string>> files;
std::vector<std::vector<std::string>> updated;

std::string toString(char *str, int first, int last)
{
  std::string ret = "";
  for (int i = first; i < last + 1; i++)
    ret.push_back(str[i]);
  return ret;
}

int main(void)
{
  struct pollfd poll_set[5];
  struct sockaddr_in sa, ca;
  int SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

  bool compressPollArr = false;

  if (SocketFD == -1)
  {
    perror("cannot create socket");
    exit(EXIT_FAILURE);
  }

  memset(&sa, 0, sizeof sa);

  sa.sin_family = AF_INET;
  sa.sin_port = htons(1100);
  sa.sin_addr.s_addr = htonl(INADDR_ANY);

  //fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK)

  if (bind(SocketFD, (struct sockaddr *)&sa, sizeof sa) == -1)
  {
    perror("bind failed");
    close(SocketFD);
    exit(EXIT_FAILURE);
  }

  if (listen(SocketFD, 10) == -1)
  {
    perror("listen failed");
    close(SocketFD);
    exit(EXIT_FAILURE);
  }

  poll_set[0].fd = SocketFD;
  poll_set[0].events = POLLIN;
  int n = 0;
  int currentSize = 0;
  int ndfs = 1;
  char buffer[256];

  int len = sizeof(ca);
  for (;;)
  {

    printf("Wait for poll..\n");
    n = poll(poll_set, ndfs, -1);

    if (n < 0)
    {
      perror("poll fail\n");
      break;
    }

    if (n == 0)
    {
      printf("poll timeout\n");
      break;
    }

    currentSize = ndfs;

    for (int i = 0; i < currentSize; i++)
    {

      if (poll_set[i].revents == 0)
      {
        continue;
      }

      if (poll_set[i].revents != POLLIN)
      {
        break;
      }

      if (poll_set[i].fd == SocketFD)
      {
        for (;;)
        {
          socklen_t ca_size = sizeof(struct sockaddr);
          int ConnectFD = accept(SocketFD, (struct sockaddr *)&ca, &ca_size);

          if (ConnectFD < 0)
          {
            perror("Connection error");
            break;
          }
          else
          {
            printf("connected\n");
          }

          poll_set[ndfs].fd = ConnectFD;
          poll_set[ndfs].events = POLLIN;
          ndfs++;
          int newSocket = ConnectFD;
          if (ConnectFD != -1)
          {
            char client_message[2000];
            char buffer[1024];
            int n;
            int size;
            int count;
            send(newSocket, READY, sizeof(char) * 3, 0);
            recv(newSocket, client_message, sizeof(char) * 8, 0);
            count = atoi(client_message);
            memset(&client_message, 0, sizeof(client_message));
            bool on_client[files.size()];
            bool files_empty = false;
            int size_before = files.size();
            if (files.size() == 0)
              files_empty = true;
            for (int i = 0; i < files.size(); i++)
              on_client[i] = false;
            for (int i = 0; i < count; i++)
            {
              send(newSocket, READY, sizeof(char) * 3, 0);
              n = recv(newSocket, client_message, sizeof(char) * 8, 0);
              if (n < 1)
                break;
              size = atoi(client_message);
              send(newSocket, READY, sizeof(char) * 3, 0);
              //n=recv(newSocket , &size , 8 , 0);
              memset(&client_message, 0, sizeof(client_message));
              n = recv(newSocket, client_message, size * 8, 0);
              //printf("%s\n",client_message);
              if (n < 1)
                break;
              //const char * hashsum = strtok(client_message, "_");
              char *quotPtr = strchr(client_message, '\n');
              int position = quotPtr - client_message;
              std::string name = toString(client_message, 0, position - 1);
              quotPtr = strchr(&client_message[position + 1], '\n');
              int position2 = quotPtr - client_message;
              std::string hashsum = toString(client_message, position + 1, position2 - 1);
              std::string date = toString(client_message, position2 + 1, size - 1);
              std::vector<std::string> temp;
              temp.push_back(name);
              temp.push_back(hashsum);
              temp.push_back(date);
              //files.push_back(temp);
              bool exists = false;
              bool leave = true;
              memset(&client_message, 0, sizeof(client_message));
              for (int i = 0; i < files.size(); i++)
              {
                if (files[i][0] == temp[0])
                {
                  on_client[i] = true;
                  if (files[i][1] != temp[1])
                  {
                    if (std::stof(files[i][2]) < std::stof(temp[2]))
                    {
                      leave = false;
                      files[i] = temp;
                      updated.push_back(temp);
                      std::fstream file;
                      file.open("./files/" + temp[0], std::ios::out | std::ios::trunc);
                      if (file.good() == true)
                      {
                        std::cout << "Uzyskano dostep do pliku!" << std::endl;
                      }
                      else
                      {
                        std::cout << "Dostep do pliku zostal zabroniony!" << std::endl;
                        send(newSocket, CONTINUE, sizeof(char) * 3, 0);
                        break;
                      }
                      send(newSocket, SEND_FROM_CLIENT, sizeof(char) * 3, 0);
                      recv(newSocket, client_message, sizeof(char) * 8, 0);
                      int length = atoi(client_message);
                      memset(&client_message, 0, sizeof(client_message));
                      int parts = length / 2000;
                      for (int i = 0; i < parts + 1; i++)
                      {
                        send(newSocket, SEND_FROM_CLIENT, sizeof(char) * 3, 0);
                        memset(&client_message, 0, sizeof(client_message));
                        recv(newSocket, client_message, sizeof(char) * 2000, 0);
                        file << client_message << std::endl;
                        std::cout << client_message << std::endl;
                      }
                      file.close();
                      exists = true;
                      break;
                    }
                    else
                    {
                      leave = false;
                      send(newSocket, SEND_FROM_SERVER, sizeof(char) * 3, 0);
                      std::fstream file;
                      file.open("./files/" + temp[0], std::ios::in);
                      if (file.good() == true)
                      {
                        std::cout << "Uzyskano dostep do pliku!" << std::endl;
                      }
                      else
                      {
                        std::cout << "Dostep do pliku zostal zabroniony!" << std::endl;
                        send(newSocket, CONTINUE, sizeof(char) * 3, 0);
                        break;
                      }
                      memset(&buffer, 0, sizeof(buffer));
                      file.read(buffer, 1024);
                      recv(newSocket, client_message, sizeof(char) * 3, 0);
                      int buffer_size = sizeof(buffer) / sizeof(buffer[0]);
                      memset(&client_message, 0, sizeof(client_message));
                      send(newSocket, buffer, sizeof(char) * buffer_size, 0);
                      file.close();
                      exists = true;
                      break;
                    }
                  }
                  else
                  {
                    exists = true;
                    break;
                  }
                }
              }
              if (!exists)
              {
                files.push_back(temp);
                updated.push_back(temp);
                std::fstream file;
                file.open("./files/" + temp[0], std::ios::out | std::ios::trunc);
                if (file.good() == true)
                {
                  std::cout << "Uzyskano dostep do pliku!" << std::endl;
                }
                else
                {
                  std::cout << "Dostep do pliku zostal zabroniony!" << std::endl;
                  send(newSocket, CONTINUE, sizeof(char) * 3, 0);
                  break;
                }
                send(newSocket, SEND_FROM_CLIENT, sizeof(char) * 3, 0);
                recv(newSocket, client_message, sizeof(char) * 8, 0);
                int length = atoi(client_message);
                std::cout << length << std::endl;
                int parts = length / 2000;
                for (int j = 0; j < parts + 1; j++)
                {
                  send(newSocket, SEND_FROM_CLIENT, sizeof(char) * 3, 0);
                  memset(&client_message, 0, sizeof(client_message));
                  recv(newSocket, client_message, sizeof(char) * 2000, 0);
                  file << client_message << std::endl;
                  std::cout << client_message << std::endl;
                }
                file.close();
              }
              memset(&client_message, 0, sizeof(client_message));
              if (exists && leave)
                send(newSocket, CONTINUE, sizeof(char) * 3, 0);
            }

            for (int j = 0; j < size_before; j++)
            {
              if (!on_client[j] && !files_empty)
              {
                send(newSocket, SEND_FROM_SERVER, sizeof(char) * 3, 0);
                char *msg = new char[files[j][0].length() + 1];
                strcpy(msg, files[j][0].c_str());
                recv(newSocket, client_message, sizeof(char) * 3, 0);
                send(newSocket, msg, sizeof(char) * strlen(msg), 0);
                recv(newSocket, client_message, sizeof(char) * 3, 0);
                std::fstream file;
                file.open("./files/" + files[j][0], std::ios::in);
                if (file.good() == true)
                {
                  std::cout << "Uzyskano dostep do pliku!" << std::endl;
                }
                else
                {
                  std::cout << "Dostep do pliku zostal zabroniony!" << std::endl;
                  send(newSocket, CONTINUE, sizeof(char) * 3, 0);
                  break;
                }
                send(newSocket, READY, sizeof(char) * 3, 0);
                memset(&buffer, 0, sizeof(buffer));
                file.read(buffer, 1024);
                recv(newSocket, client_message, sizeof(char) * 3, 0);
                int buffer_size = sizeof(buffer) / sizeof(buffer[0]);
                memset(&client_message, 0, sizeof(client_message));
                send(newSocket, buffer, sizeof(char) * buffer_size, 0);
                file.close();
              }
            }

            for (int i = 0; i < files.size(); i++)
              std::cout << files[i][0] << " " << files[i][1] << " " << files[i][2] << std::endl;
            send(newSocket, UP_TO_DATE, sizeof(char) * 3, 0);
            break;
          }
        }
      }
      else
      {

        for (;;)
        {
          printf("Can read \n");

          n = recv(poll_set[i].fd, buffer, sizeof(buffer), 0);
          if (n < 0)
          {
            break;
          }

          if (n == 0)
          {
            printf("Connection closed \n");
            break;
          }

          //do echo
          int len = n;

          n = send(poll_set[i].fd, buffer, n, 0);
        }

        close(poll_set[i].fd);
        printf("closing \n");
        poll_set[i].fd = -1;
        compressPollArr = true;
      }
    }

    if (compressPollArr)
    {
      compressPollArr = false;
      for (int i = 0; i < ndfs; i++)
      {
        if (poll_set[i].fd == -1)
        {
          for (int j = i; j < ndfs; j++)
          {
            poll_set[j].fd = poll_set[j + 1].fd;
          }
          i--;
          ndfs--;
        }
      }
    }
  }

  for (int i = 0; i < ndfs; i++)
  {
    if (poll_set[i].fd >= 0)
      close(poll_set[i].fd);
  }

  close(SocketFD);
  return EXIT_SUCCESS;
}
