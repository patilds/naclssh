// Author: Kate Volkova <gloom.msk@gmail.com>

#ifndef LIBSSH2_NACL_H_
  #define LIBSSH2_NACL_H_

  void nacl_set_block(int flag);

  int nacl_send(int socket, char* buffer, int length);
  int nacl_recv_lock(int socket, char* buffer, int length);
  int nacl_recv_non_lock(int socket, char* buffer, int length);

  int nacl_wait_for_sock();

  int base64_to_binary_data(char** data, unsigned int* datalen,
                            const char* src, unsigned int src_len);

#endif  // LIBSSH2_NACL_H_
