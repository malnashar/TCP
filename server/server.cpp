#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <bits/stdc++.h>
#include <sstream>
#include <fstream>
#include <experimental/filesystem>
#include <pthread.h>
using namespace std;
namespace fs = std::experimental::filesystem;
void* handle_requests(void* th);
vector<string> split(string request, string delimiter);
void send_response(int socket, char *data, int *len);
void handle_get_request(string path, int socket);
void handle_post_request(string path, int socket);
void writ_in_file(string path, string filename, string data, int size);
vector<string> parse_request(string request);
void handle_post_request(string path, int socket,vector<string>req);
#define PORT 8081
struct threadVar
{
    int socket;
};
int main(int argc, char const *argv[])
{

    //intiate the port number to run
    //either from terminal or codeblocks
    int port = 8080;
    if(argc == 2)
    {
        port = atoi(argv[1]);
    }

    int server_socket;
    int new_socket;
    long valread;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Creating socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("faild to create socket");
        exit(EXIT_FAILURE);
        //close(server_socket);
    }

    //specify domain and address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( port );

    memset(address.sin_zero, '\0', sizeof address.sin_zero);


    //configure the socket at the address to start transformation
    //if there is any request
    if (bind(server_socket, (struct sockaddr *)&address, sizeof(address))<0)
    {
        perror("In bind");
        exit(EXIT_FAILURE);
        //close(server_socket);
    }
    //listen to the socket for any requests and connections
    //from client
    if (listen(server_socket, 10) < 0)
    {
        perror("In listen");
        exit(EXIT_FAILURE);
    }
    while(1)
    {
        //accept the connection from the clinet
        cout <<"+++++++ Waiting new connection to accept++++++++"<<endl;
        if ((new_socket = accept(server_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
        {
            perror("In accept");
            exit(EXIT_FAILURE);
        }


        //create thread to handle the requests in this connection
        pthread_t thread;
        struct threadVar *th = (struct threadVar *)malloc(sizeof(struct threadVar));
        th->socket=new_socket;
        pthread_create(&thread, NULL, &handle_requests, (void*)th);


    }
    return 0;
}


//function that executed by each thread to handle
//requests. so it read from the socket the request and then check if it
//is get or post and handle poth cases
void* handle_requests(void* th)
{

    int socket = ((struct threadVar*)th)->socket;
    int rec_size;
    string requests = "";
    int i = 0;
    while(1)
    {
        i++;
        char buffer[100000] = {0};
        rec_size = read(socket, buffer, 100000);
        if(rec_size  <=  0)
        {
            break;
        }
        requests = string(buffer,rec_size);
        cout<<"request--> "<<requests<<flush<<endl;
        if (requests.find("\r\n\r\n") != std::string::npos)
        {
            vector<string> post = parse_request(requests);
            vector<string> split_requests = split(requests," ");
            string req = split_requests[0];
            string file = split_requests[1];

            if (req == "GET")
            {

                handle_get_request(file,socket);
            }
            else if (req == "POST")
            {
                handle_post_request(file,socket,post);
            }
        }
        cout<<"*******************************************"<<endl;

    }
    close(socket);
}

//helpful function to parse the request sent from the client
vector<string> split(string request, string delimiter)
{
    vector<string> parts;
    string token;
    size_t pos = 0;
    while ((pos = request.find(delimiter)) != std::string::npos)
    {
        token = request.substr(0, pos);
        parts.push_back(token);
        request.erase(0, pos + delimiter.length());
    }
    return parts;
}

//handle get request
void handle_get_request(string path, int socket)
{
    string file_name = "data";
    string response = "";
    string ex = "";
    char *data;
    int data_length = 0;
    file_name += path;
    int file_exist = 0;
    ifstream ifile;
    //check if the requested file is exist
    ifile.open(file_name);
    if(ifile)
    {
        file_exist = 1;
    }
    else
    {
        file_exist = 0;
    }

    //create the response with the details
    //and then send it to client
    if(file_exist == 0)
    {
        response = "HTTP/1.1 404 Not Found\r\n\r\n";
        data_length =response.size();
    }
    else
    {

        ifstream file(file_name);
        string data((std::istreambuf_iterator<char>(file)),
                    std::istreambuf_iterator<char>());
        response = "HTTP/1.1 200 OK\r\n";
        response += "Content-Length: ";
        response += to_string(data.size());
        response += "\r\n\r\n";
        response = response + data;
        data_length = response.size();
    }
    cout <<"response--> "<<endl<<response<<endl;
    data = &response[0];
    send_response(socket, data, &data_length);
}

//function to be sure that all the data sent
void send_response(int socket, char *data, int *len)
{
    int amount_sent = 0;
    int amount_left = *len;
    int current;
    while(amount_sent < *len)
    {
        current = write(socket, data+amount_sent, amount_left);
        if (current <= 0)
        {
            break;
        }
        amount_sent += current;
        amount_left -= current;
    }
}
//handle the post request and write it to file
//and create the response to the client
void handle_post_request(string path, int socket,vector<string>req)
{
    string response;
    if(req.size() == 2)
    {
        writ_in_file("request",path,req[1],stoi(req[0]));
    }
    else if(req.size() == 1)
    {
        writ_in_file("request",path,"",0);
    }

    response = "HTTP/1.1 200 OK\r\n\r\n";
    cout<<"response--> "<<endl<<response<<endl;
    char *data;
    int data_length = 0;
    data = &response[0];
    data_length = response.size();
    write(socket,data,data_length);
    return;
}

//use regex to parse the post request
vector<string> parse_request(string request)
{
    vector<string>parts;
    vector<string>temp;
    if(request.find("\r\n\r\n") != std::string::npos)
    {
        request = regex_replace(request,regex("\r\n"), "%");
        regex e("%+");
        regex_token_iterator<string::iterator> i(request.begin(), request.end(), e, -1);
        regex_token_iterator<string::iterator> end;
        while (i != end)
            temp.push_back(*i++);

        if(temp.size() == 2)
        {
            parts.push_back(string(temp[1].c_str() + 16, temp[1].c_str() + temp[1].size()));
        }
        else if(temp.size() == 3)
        {
            parts.push_back(string(temp[1].c_str() + 16, temp[1].c_str() + temp[1].size()));
            parts.push_back(temp[2]);
        }
    }
    return parts;
}

//write data to file
void writ_in_file(string path, string filename, string data, int size)
{
    ofstream file(path + filename,ios::binary);
    char * data_arr = &data[0];
    file.write(data_arr, size);
}



