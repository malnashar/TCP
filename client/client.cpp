#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <vector>
#include <bits/stdc++.h>
#include <regex>
using namespace std;
#define PORT 8081

vector<string> split(string request, char delimiter);
vector<pair<string, string>> getRequests(string filename);
void send(int socket, char* data, int *len);
void read_response(int socket);
vector<string> parse_response(string response);
string handle_get_request(string path);
void send_requests(string file_name, int socket);
void writ_in_file(string path, string filename, string data, int size);
void read_response(int socket,string name);
string get_name(string file_name);


int main(int argc, char const *argv[])
{
        //intiate the port number to run
        //either from terminal or codeblocks
        int port = 8080;
        string def_add = "127.0.0.1";
        char *add = &def_add[0];
        if(argc == 3)
        {
            port = atoi(argv[2]);
            strcpy(add,argv[1]);
        }
        int sock = 0;
        long valread;
        struct sockaddr_in serv_addr;
        char buffer[1024] = {0};
        //create socket to connect with server
        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            printf("\n Socket creation error \n");
            return -1;
        }

        memset(&serv_addr, '0', sizeof(serv_addr));

        //specify domain and port number to connect to server
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(port);

        // Convert IPv4 and IPv6 addresses from text to binary form
        if(inet_pton(AF_INET, add, &serv_addr.sin_addr)<=0)
        {
            printf("\nInvalid address/ Address not supported \n");
            return -1;
        }

        //connect to the server to start transform data
        if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        {
            printf("\nConnection Failed \n");
            return -1;
        }

        send_requests("requests.txt",sock);

    return 0;


}

//read all requests from the file to be sent to the server
//from file "requests" and check either if the request is
//post or get and the return all requests.
vector<pair<string, string>> getRequests(string filename)
{
    ifstream file (filename);
    string line;
    vector<pair<string, string>> all_requests;
    if (file.is_open())
    {
        while ( getline (file,line) )
        {
            vector<string> tokens = split(line,' ');
            string request_type = tokens[0];
            string path = tokens[1];
            if(request_type.compare("client_get") == 0)
            {
                all_requests.push_back(make_pair("GET", path));
            }
            else
            {
                all_requests.push_back(make_pair("POST", path));
            }
        }
        file.close();
    }
    else
    {
        cout <<"Error when read requests file"<<endl;
    }
    return all_requests;
}

//use this function to parse the request and split
//it to get the information of request like filepath
vector<string> split(string request, char delimiter)
{
    vector<string> parts;
    stringstream ss(request);
    string token;

    while(getline(ss, token, delimiter))
    {
        parts.push_back(token);
    }

    return parts;
}


//create the form of the get request
//by concatenate request type with filepath and the host
string handle_get_request(string path)
{
    string request = "GET " + path + " HTTP/1.1" + "\r\n\r\n";
    return request;
}

//function to handle all post requests
string handle_post_request(string path)
{
    string request = "POST " + path + " HTTP/1.1" + "\r\n";
    ifstream ifile;
    int file_exist = 0;
    char *data;
    int data_length = 0;
    string folder = "request";
    folder += path;
    //check if the file that will be sent to server exist or not
    ifile.open(folder);
    if(ifile)
    {
        file_exist = 1;
    }
    else
    {
        file_exist = 0;
    }

    if(file_exist == 0)
    {
        cout<<"file name is wrong"<<endl;
    }
    else
    {

        //load the data of the file
        //and formulate the request
        ifstream file(folder);
        string data((std::istreambuf_iterator<char>(file)),
                    std::istreambuf_iterator<char>());
        request += "Content-Length: ";
        request += to_string(data.size());
        request += "\r\n\r\n";
        request = request + data;
        //data_length = response.size();
    }
    return request;
}

//parse file name
string get_name(string file_name)
{
    vector<string> temp = split(file_name,'/');
    string file = temp[1];
    return file;
}

//for each request determine if it is get or post
//and then send it to the server and wait for the response back
void send_requests(string file_name, int socket)
{
    char * req;

    vector<pair<string,string>> requests = getRequests(file_name);
    for(int i = 0 ; i < requests.size() ; i ++)
    {
        pair<string, string> request = requests[i];
        cout<<"reuest "<< request.first<<" "<<request.second<<"HTTP/1.1"<<endl;
        string file = get_name(request.second);
        if(request.first.compare("GET") == 0)
        {
            string get_req = handle_get_request(request.second);
            req = &get_req[0];
            int reqlen = strlen(req);
            send(socket, req, &reqlen);

        }
        else
        {
            string str = handle_post_request(request.second);
            req = &str[0];
            int reqlen = str.size();
            send(socket, req, &reqlen);
        }
        read_response(socket,file);
        cout<<"**********************************"<<endl;
    }
}

//function to be sure that all the buffer data is sent
//to the server
void send(int socket, char* data, int *len)
{
    int amount_sent = 0;
    int amount_left = *len;
    int current;
    while(amount_sent < *len)
    {
        current = write(socket, data+amount_sent, amount_left);
        if (current == -1)
        {
            cout << "Error while sending"<<endl;
            break;
        }
        amount_sent += current;
        amount_left -= current;
    }
}

//use regex to parse the response back from the server
vector<string> parse_response(string response)
{
    vector<string>parts;
    vector<string>temp;
    if(response.find("\r\n\r\n") != std::string::npos)
    {
        response = regex_replace(response,regex("\r\n"), "%");
        regex e("%+");
        regex_token_iterator<string::iterator> i(response.begin(), response.end(), e, -1);
        regex_token_iterator<string::iterator> end;
        while (i != end)
            temp.push_back(*i++);
        parts.push_back(string(temp[0].c_str() + 9, temp[0].c_str() + temp[0].size()));
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

//read the response from the the socket and save the response
//in files
void read_response(int socket,string name)
{
    int rec_size = 0;
    char buffer[100000];
    if ((rec_size = read(socket, buffer, 100000)) < 0)
    {
        cout << "error while reading"<<endl;
        return;
    }
    string response = string(buffer,rec_size);
    cout<<"response--> "<<response<<endl;
    vector<string>res = parse_response(response);

    if(res[0] == "404 Not Found")
    {
        cout << "404 not found"<<endl;
    }
    else
    {
        if(res.size() == 3)
        {
            writ_in_file("response/",name,res[2],stoi(res[1]));
        }
        else if(res.size() == 2)
        {
            writ_in_file("response/",name,"",0);
        }
    }

}
//write data in the files.
void writ_in_file(string path, string filename, string data, int size) {
    ofstream file(path + filename,ios::binary);
    char * data_arr = &data[0];
    file.write(data_arr, size);
}
