#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <vector>
#include <string>
#include <queue>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <iostream>
#include <errno.h>

using namespace std;

class client {
public:
    boost::asio::io_service io_s;
    boost::asio::ip::tcp::socket socket;
    boost::system::error_code error;
    char *recv_buf;
    int recv_len;


    client() : socket(io_s) {
        recv_buf = (char *)malloc(1024*16);
        recv_len = 1024;
    }
    ~client(){
        free(recv_buf);
    }

    void create_conn(const string& host, const string& port) {

        /*connection*/
        //도메인 이름을 TCP 종단점으로 바꾸기위해 Resolver를 사용
        boost::asio::ip::tcp::resolver resolver(io_s);
        //port와 host주소를 소켓에 넣음
        boost::asio::connect(socket, resolver.resolve({host, port}));
    }

    void send_req(string& msg){
        
        boost::asio::write(socket, boost::asio::buffer(msg, msg.length()));
        printf("%s", msg.c_str());
        //cout << msg << endl;

    }

    unsigned int recv_req(){

        size_t reply_length = boost::asio::read(socket, boost::asio::buffer(recv_buf, recv_len));
        
        printf("%s", recv_buf);

        return reply_length;

    }

}; 

int main(int argc, char* argv[]){
    try{
        if(argc != 3){
            cerr << "Usage : <host> <port>" << endl;
            return 1;
        }

        client c;
        unsigned int size;
        c.create_conn(argv[1], argv[2]);
       // string msg = "*3\r\n$3\r\nSET\r\n$3\r\nbbb\r\n$5\r\nvalue\r\n";
        //string msg = "*2\r\n$3\r\nGET\r\n$3\r\nkey\r\n";
        string msg = "*2\r\n$3\r\nGET\r\n$3\r\nbbb\r\n";
        c.send_req(msg);
        size=c.recv_req();

        printf("%d\n", size);
        //printf(" %s\n", c.recv_buf);

    }catch(exception& e){
        cerr<<"Exception : " << e.what() << endl;
    }
    return 0;
}

