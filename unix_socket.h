    1 /*      -*- OpenSAF  -*-
    2  *
    3  * (C) Copyright 2016 The OpenSAF Foundation
    4  * Copyright Ericsson AB 2017 - All Rights Reserved.
    5  *
    6  * This program is distributed in the hope that it will be useful, but
    7  * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    8  * or FITNESS FOR A PARTICULAR PURPOSE. This file and program are licensed
    9  * under the GNU Lesser General Public License Version 2.1, February 1999.
   10  * The complete license can be accessed from the following location:
   11  * http://opensource.org/licenses/lgpl-license.php
   12  * See the Copying file included with the OpenSAF distribution for full
   13  * licensing terms.
   14  *
   15  * Author(s): Ericsson AB
   16  *
   17  */
   18 
   19 #ifndef BASE_UNIX_SOCKET_H_
   20 #define BASE_UNIX_SOCKET_H_
   21 
   22 #include <pthread.h>
   23 #include <sys/socket.h>
   24 #include <sys/un.h>
   25 #include <time.h>
   26 #include <cerrno>
   27 #include <cstring>
   28 #include <string>
   29 #include "base/macros.h"
   30 
   31 namespace base {
   32 
   33 // A class implementing operations on a UNIX domain socket. This class is not
   34 // thread-safe and must be protected by a mutex if shared between multiple
   35 // threads. You can select to use either blocking or non-blocking mode.
   36 class UnixSocket {
   37  public:
   38   // I/O mode for Send and Receive socket operations.
   39   enum Mode { kNonblocking, kBlocking };
   40   // Close the socket.
   41   virtual ~UnixSocket();
   42   // Returns the current file descriptor for this UNIX socket, or -1 if the
   43   // socket could not be opened. Note that the Send() and Recv() methods may
   44   // open and/or close the socket, and the file descriptor can therefore
   45   // potentially be change as a result of calling any of these two methods. If a
   46   // UnixSocket instance is shared between multiple threads (and therefore
   47   // protected with a mutex), this means that the result from the fd() method is
   48   // only valid for as long as the mutex protecting the instance is held.
   49   int fd() {
   50     int sock = fd_;
   51     if (sock < 0) sock = Open();
   52     return sock;
   53   }
   54   // Send a message in blocking or non-blocking mode to the specified
   55   // destination. This call will open the socket if it was not already open. The
   56   // EINTR error code from the sendto() libc function is handled by retrying the
   57   // sendto() call in a loop. In case of other errors, -1 is returned and errno
   58   // contains one of the codes listed in the sendto() libc function. The socket
   59   // will be closed in case the error was not EINTR, EAGAIN or EWOULDBLOCK.
   60   ssize_t SendTo(const void* buffer, size_t length,
   61                  const struct sockaddr_un* dest_addr, socklen_t addrlen) {
   62     int sock = fd();
   63     ssize_t result = -1;
   64     if (sock >= 0) {
   65       do {
   66         result = sendto(sock, buffer, length, MSG_NOSIGNAL,
   67                         reinterpret_cast<const struct sockaddr*>(dest_addr),
   68                         addrlen);
   69       } while (result < 0 && errno == EINTR);
   70       if (result < 0 && errno != EAGAIN && errno != EWOULDBLOCK) Close();
   71     }
   72     return result;
   73   }
   74   // Send a message in blocking or non-blocking mode. This call will open the
   75   // socket if it was not already open. The EINTR error code from the send()
   76   // libc function is handled by retrying the send() call in a loop. In case of
   77   // other errors, -1 is returned and errno contains one of the codes listed in
   78   // the send() libc function. The socket will be closed in case the error was
   79   // not EINTR, EAGAIN or EWOULDBLOCK.
   80   ssize_t Send(const void* buffer, size_t length) {
   81     int sock = fd();
   82     ssize_t result = -1;
   83     if (sock >= 0) {
   84       do {
   85         result = send(sock, buffer, length, MSG_NOSIGNAL);
   86       } while (result < 0 && errno == EINTR);
   87       if (result < 0 && errno != EAGAIN && errno != EWOULDBLOCK) Close();
   88     }
   89     return result;
   90   }
   91   // Receive a message in blocking or non-blocking mode and return the source
   92   // address. This call will open the socket if it was not already open. The
   93   // EINTR error code from the recvfrom() libc function is handled by retrying
   94   // the recvfrom() call in a loop. In case of other errors, -1 is returned and
   95   // errno contains one of the codes listed in the recvfrom() libc function. The
   96   // socket will be closed in case the error was not EINTR, EAGAIN or
   97   // EWOULDBLOCK.
   98   ssize_t RecvFrom(void* buffer, size_t length, struct sockaddr_un* src_addr,
   99                    socklen_t* addrlen) {
  100     int sock = fd();
  101     ssize_t result = -1;
  102     socklen_t len;
  103     if (sock >= 0) {
  104       do {
  105         len = *addrlen;
  106         result = recvfrom(sock, buffer, length, 0,
  107                           reinterpret_cast<struct sockaddr*>(src_addr), &len);
  108       } while (result < 0 && errno == EINTR);
  109       if (result < 0 && errno != EAGAIN && errno != EWOULDBLOCK) Close();
  110     }
  111     *addrlen = result >= 0 ? len : 0;
  112     return result;
  113   }
  114   // Receive a message in blocking or non-blocking mode. This call will open the
  115   // socket if it was not already open. The EINTR error code from the recv()
  116   // libc function is handled by retrying the recv() call in a loop. In case of
  117   // other errors, -1 is returned and errno contains one of the codes listed in
  118   // the recv() libc function. The socket will be closed in case the error was
  119   // not EINTR, EAGAIN or EWOULDBLOCK.
  120   ssize_t Recv(void* buffer, size_t length) {
  121     int sock = fd();
  122     ssize_t result = -1;
  123     if (sock >= 0) {
  124       do {
  125         result = recv(sock, buffer, length, 0);
  126       } while (result < 0 && errno == EINTR);
  127       if (result < 0 && errno != EAGAIN && errno != EWOULDBLOCK) Close();
  128     }
  129     return result;
  130   }
  131 
  132   // Set the path in a sockaddr_un structure. Takes a C++ string @a path and a
  133   // pointer to the sockarddr_un sturcture which is to be updated. Returns the
  134   // resulting number of used bytes in the structure, which is <=
  135   // sizeof(structaddr_un), or zero in case of an error (i.e. the @a path did
  136   // not fit into the structure). The pointer to the sockaddr_un structure
  137   // together with the returned size can be used as input to the SendTo method.
  138   //
  139   // Abstract unix domain addresses are supported, by setting the first byte in
  140   // @a path to '\0'.
  141   static socklen_t SetAddress(const std::string& path,
  142                               struct sockaddr_un* addr) {
  143     socklen_t addrlen;
  144     size_t size = path.size();
  145     if (size != 0 && path[0] != '\0') ++size;
  146     addr->sun_family = AF_UNIX;
  147     if (size <= sizeof(addr->sun_path)) {
  148       memcpy(addr->sun_path, path.c_str(), size);
  149       addrlen = sizeof(addr->sun_family) + size;
  150     } else {
  151       addr->sun_path[0] = '\0';
  152       addrlen = 0;
  153     }
  154     return addrlen;
  155   }
  156 
  157   // Returns true if the UNIX socket address @a addr is abstract, i.e. it is an
  158   // address which is not represented by a file in the file system.
  159   static bool IsAbstract(const struct sockaddr_un& addr) {
  160     return addr.sun_path[0] == '\0';
  161   }
  162 
  163   // Returns true if this UNIX socket has an abstract address, i.e. the address
  164   // is not represented by a file in the file system.
  165   bool IsAbstract() const { return IsAbstract(addr_); }
  166 
  167   // Get the path from a sockaddr_un structure. Takes a pointer to a
  168   // sockarddr_un sturcture and the size of the structure, and returns a C++
  169   // string containing the path name, or an abstract address starting with '\0'.
  170   // Returns an empty string in case of an error.
  171   static std::string GetAddress(const struct sockaddr_un& addr,
  172                                 socklen_t addrlen) {
  173     if (addrlen > sizeof(addr.sun_family) && addrlen <= sizeof(addr) &&
  174         addr.sun_family == AF_UNIX) {
  175       size_t size =
  176           IsAbstract(addr)
  177               ? (addrlen - sizeof(addr.sun_family))
  178               : strnlen(addr.sun_path, addrlen - sizeof(addr.sun_family));
  179       return std::string(addr.sun_path, size);
  180     } else {
  181       return std::string{};
  182     }
  183   }
  184 
  185  protected:
  186   UnixSocket(const std::string& path, Mode mode);
  187   UnixSocket(const sockaddr_un& addr, socklen_t addrlen, Mode mode);
  188   virtual bool OpenHook(int sock);
  189   virtual void CloseHook();
  190   const struct sockaddr* addr() const {
  191     return reinterpret_cast<const struct sockaddr*>(&addr_);
  192   }
  193   socklen_t addrlen() { return addrlen_; }
  194   int get_fd() const { return fd_; }
  195   const char* path() const { return addr_.sun_path; }
  196 
  197  private:
  198   int Open();
  199   void Close();
  200 
  201   int fd_;
  202   struct sockaddr_un addr_;
  203   socklen_t addrlen_;
  204   struct timespec last_failed_open_;
  205   int saved_errno_;
  206   Mode mode_;
  207 
  208   DELETE_COPY_AND_MOVE_OPERATORS(UnixSocket);
  209 };
  210 
  211 }  // namespace base
  212 
  213 #endif  // BASE_UNIX_SOCKET_H_