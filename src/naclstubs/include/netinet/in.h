#ifndef NACL_NETINET_STUB_H_
  #define NACL_NETINET_STUB_H_

typedef uint32_t in_addr_t;

/* Address to accept any incoming messages.  */
#define INADDR_ANY    ((in_addr_t) 0x00000000)


uint32_t ntohl (uint32_t __netlong);
uint16_t ntohs (uint16_t __netshort);
uint32_t htonl (uint32_t __hostlong);
uint16_t htons (uint16_t __hostshort);

#endif
