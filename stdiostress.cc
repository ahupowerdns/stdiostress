#include "sclasses.hh"
#include <iostream>
#include <atomic>
#include <thread>
#include <boost/circular_buffer.hpp>
#include <sys/uio.h>
#include <mutex>
using namespace std;
std::atomic<uint32_t> g_counter{0}, g_written{0};

class WriteBuffer
{
public:
  explicit WriteBuffer(int fd, int size) : d_fd(fd), d_buffer(size)
  {
  }

  void write(std::string_view str);
  void flush()
  {
    std::unique_lock<std::mutex> lock(d_mutex);
    flushLocked();
  }
private:
  std::mutex d_mutex;
  void flushLocked();
  int d_fd;
  boost::circular_buffer<char> d_buffer;
};

void WriteBuffer::write(std::string_view str)
{
  std::unique_lock<std::mutex> lock(d_mutex);
  //  cout << "In buffer: "<<d_buffer.size() <<", "<<d_buffer.capacity() <<endl;
  //  cout << "Wants to write " << str.size()<<" bytes"<<endl;

  if(d_buffer.size() + str.size() > d_buffer.capacity())
    flushLocked();

  if(d_buffer.capacity() - d_buffer.size() < str.size())
    throw std::runtime_error("Full!");
  d_buffer.insert(d_buffer.end(), str.begin(), str.end());
}

void WriteBuffer::flushLocked()
{
  auto arr1 = d_buffer.array_one();
  auto arr2 = d_buffer.array_two();

  struct iovec iov[2];
  int pos=0;
  int total=0;
  for(const auto& arr : {arr1, arr2}) {
    if(arr.second) {
      iov[pos].iov_base = arr.first;
      iov[pos].iov_len = arr.second;
      total += arr.second;
      ++pos;
    }
  }

  //  cout<<"Attempting to flush "<< total <<" bytes in "<<pos<<" parts"<<endl;
  int res = writev(d_fd, iov, pos);
  if(res < 0) {
    throw std::runtime_error("Couldn't flush a thing: "+string(strerror(errno)));
  }
  if(!res) {
    throw std::runtime_error("EOF");
  }
  //  cout<<"Flushed "<<res<<" bytes out of " << total <<endl;
  while(res--)
    d_buffer.pop_front();
}

int main(int argc, char** argv)
{
  ComboAddress remote(argv[1]);
  Socket s(remote.sin4.sin_family, SOCK_STREAM);
  
  SConnect(s, remote);

  SetNonBlocking(s);

  WriteBuffer wb(s, 100000000);
  
  auto func = [&wb]() {
    std::string o;

    for(unsigned int n = 0 ; n < 1000000; ++n) {
      o = std::to_string(g_counter++)+"\n";
      if(!(g_counter % (16384*16)))
        cout << g_counter << endl;
      wb.write(o);
    }
  };


  std::thread t1(func), t2(func), t3(func), t4(func);
  t1.join();
  t2.join();
  t3.join();
  t4.join();

  cout << "Done writing"<<endl;
  SetNonBlocking(s, false);
  wb.flush();
  cout << "Done flushing" << endl;
  pause();
}
