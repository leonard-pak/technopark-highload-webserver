#include "server/server.hh"

#include <netinet/in.h>
#include <sys/socket.h>
#include <array>

#include "spdlog/spdlog.h"

#include "server/config.hh"
#include "server/file.hh"
#include "server/http.hh"
#include "server/thread.hh"

namespace server {

Server::Server(int aPort) : kPort(aPort) {
  spdlog::set_pattern("[%H:%M:%S] [%^%l%$] [thread %t] %v");
#ifdef DEBUG
  spdlog::set_level(spdlog::level::debug);
  spdlog::debug("DEBUG MODE");
#endif
  spdlog::debug("Path: {}", Config<std::string>("document_root"));
  spdlog::info("Threads: {}", Config<int>("thread_limit"));
}

void Server::Start() {
  if ((mSocketFD = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    throw std::runtime_error("Socket failed");
  }

  auto constexpr opt = 1;
  if (setsockopt(mSocketFD, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                 sizeof(opt))) {
    throw std::runtime_error("Setsockopt failed");
  }

  auto address = sockaddr_in();
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(kPort);
  socklen_t addrlen = sizeof(address);

  if (bind(mSocketFD, reinterpret_cast<sockaddr*>(&address), addrlen) < 0) {
    throw std::runtime_error("Bind failed");
  }
  if (listen(mSocketFD, SOMAXCONN) < 0) {
    throw std::runtime_error("Listen failed");
  }

  int newSocket;
  std::array<char, MAX_RECV_LEN> buffer;
  auto threadPool = ThreadPool(Config<int>("thread_limit"));
  while (true) {
    if ((newSocket = accept(mSocketFD, reinterpret_cast<sockaddr*>(&address),
                            &addrlen)) < 0) {
      throw std::runtime_error("Accept failed");
    }
    threadPool.AddTask(newSocket);
  }

  shutdown(mSocketFD, SHUT_RDWR);
}

}  // namespace server
