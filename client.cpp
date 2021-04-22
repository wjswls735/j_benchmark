#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <vector>
#include <string>
#include <queue>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <iostream>
#include <errno.h>
#include <thread>
#include <random>
#include <map>
#include <boost/algorithm/string.hpp>

using namespace std;


class client {
public:
    boost::asio::io_service io_s;
    boost::asio::ip::tcp::socket socket;
	boost::asio::ip::tcp::iostream stream;
    boost::system::error_code error;
    char *recv_buf;
    int recv_len;
    long long send_count;
    long long read_count;
    map<string, string> key_value_table;

    bool sock_close_flag;

    vector<long long> return_latency;
    vector<long long> start_latency;

    client() : socket(io_s) {
        recv_buf = (char *)malloc(1024*1024);
        recv_len = 1060;
//        socket.non_blocking(true, error);
        send_count=0;
        read_count=0;
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
        sock_close_flag = false;
    }

    void send_req(string& msg){
        
        long long start;
        start = ustime();
        boost::asio::write(socket, boost::asio::buffer(msg, msg.length()));
        start_latency.push_back(start);
        send_count++;
        
    }
    void close(){
        socket.close();
        sock_close_flag==true;
    }

    static long long ustime(void){
        struct timeval tv;
        long long ust;
        gettimeofday(&tv, NULL);
        ust= ((long)tv.tv_sec)*1000000;
        ust+=tv.tv_usec;
        return ust;
    }

    vector<string> split(const string &s, const string &sep){

        std::vector<std::string> result;
        int start = 0;
        int end = 0;
        std::string empty = "";
        
        //find를 통해 sep문자가 있는지 확인
        //find를 통해 문자가 확인되면 substr으로 start지점부터 발견지점까지 잘라서 vector에 집어넣음
        //문장이 empty이면 vector에 넣지않고 넘어감
        while(end!=std::string::npos){
            end = s.find(sep, start);
            if(empty.compare(s.substr(start,end-start))) result.push_back(s.substr(start, end-start));
            start = end + sep.length();
        }
        return result;
    }

    unsigned int recv_req(){

        if(read_count >=send_count) return 0;
        long long end;  
        size_t reply_length = boost::asio::read(socket, boost::asio::buffer(recv_buf, 5));
        end = ustime()-start_latency[read_count];

//        printf("%s ------ \n", recv_buf); 
//        vector<string> slice;
//        string tmp(recv_buf);

        if(boost::algorithm::contains(recv_buf, "+OK")){
//            slice = split(tmp, "+OK");
            return_latency.push_back(end);
  //          printf("return latency = %lld\n", end);
   //         printf("return_latency.size() = %ld", return_latency.size());
        }
        else{
  //          slice = split(tmp, "j_benchmark0000");
            return_latency.push_back(end);
        }

//        printf("%s", recv_buf);
      //  printf("\n");
        read_count++;

        return reply_length;

    }

    void create_request(int count, bool flag){
	
		int size;
		if(flag == true){
			//flag == true -> SET
			string key="j_bench";
			key+=to_string(count);
            
			string value = "j_benchmark0000";
            char *xvalue;
            xvalue = (char*)malloc(1024*1024);

            memset(xvalue, 'x', 1024*1024*sizeof(char)-15); 
            string xvalue2(xvalue);
            value+=xvalue2;
//			value+=to_string(count);
		
			string msg ="*3\r\n$3\r\nSET\r\n";
			size = key.length();
			msg+="$";
			msg+=(to_string(size));
			msg+=("\r\n");
			msg+=(key);
			msg+=("\r\n");

			size = value.length();
			msg+=("$");
			msg+=(to_string(size));
			msg+=("\r\n");
			msg+=(value);
			msg+=("\r\n");

            key_value_table.insert(make_pair(key,value));

			send_req(msg);
            free(xvalue);
		}
		else{
			//flag == false -> GET
			string key="j_bench";
			key+=to_string(count);
		
			string msg ="*2\r\n$3\r\nGET\r\n";
			size = key.length();
			msg+=("$");
			msg+=(to_string(size));
			msg+=("\r\n");
			msg+=(key);
			msg+=("\r\n");

			send_req(msg);
		}
	}

}; 

client c;

void recv_data(bool th_flag, int total){

    printf("th_flag = %d\n", th_flag);
    if(th_flag == false) return;
    while(1){
        c.recv_req();
        if(c.sock_close_flag == true) break;
        if(c.read_count == total) break;
    }
}

int main(int argc, char* argv[]){
    try{

        if(argc != 5){
            cerr << "Usage : <host> <port> <operation number> <lambda>" << endl;
            return 1;
        }

        unsigned int size;

        long long average=0;
        c.create_conn(argv[1], argv[2]);
        
		int count=0;
		int operation_count = atoi(argv[3]);

        mt19937 gen(operation_count);
        poisson_distribution<> d(atoi(argv[4]));

        bool th_flag=true;
        
        if(atoi(argv[4]) == 0){
           
            th_flag=false;
        }
        thread tid(recv_data, th_flag, operation_count);

        while(1){
			bool flag=true;
            c.create_request(count, flag);
			count++;
			if(count == operation_count) break;
//            printf("d(gen) = %d\n", d(gen));
            if(atoi(argv[4]) == 0){
                c.recv_req();
            }
            else{
                usleep(d(gen));
            }

//            printf("%d " ,d(gen));
        }
        
        for(auto x : c.return_latency){
            average +=x;
//            printf("x = %lld \n", x);
//            printf("average = %lld \n", average);

        }
        cout << "set average latency = " << average/(long long)c.return_latency.size() << "us" << endl;

        /*

        count=0;
        while(1){
			bool flag=false;
            c.create_request(count, flag);
			count++;
			if(count == operation_count) break;
            if(atoi(argv[4]) == 0){
                c.recv_req();
            }
            else{
                usleep(d(gen));
            }

        }
        average=0;
        for(auto x : c.return_latency){
            average +=x;
        }
        cout << "get average latency = " << average/(c.return_latency.size()) << "us" << endl;
        */
        c.close();
        
        if(atoi(argv[4]) != 0){
            tid.join();
        }
        printf("%d\n", size);
        //printf(" %s\n", c.recv_buf);

    }catch(exception& e){
        cerr<<"Exception : " << e.what() << endl;
    }
    return 0;
}

