#ifndef NACL_NETDB_STUB_H_
  #define NACL_NETDB_STUB_H_

/* Description of data base entry for a single host.  */
struct hostent
{
  char *h_name;       /* Official name of host.  */
  char **h_aliases;   /* Alias list.  */
  int h_addrtype;     /* Host address type.  */
  int h_length;       /* Length of address.  */
  char **h_addr_list;   /* List of addresses from name server.  */
#if defined __USE_MISC || defined __USE_GNU
# define  h_addr  h_addr_list[0] /* Address, for backward compatibility.*/
#endif
};

/* Description of data base entry for a single service.  */
struct servent
{
  char *s_name;     /* Official service name.  */
  char **s_aliases;   /* Alias list.  */
  int s_port;     /* Port number.  */
  char *s_proto;    /* Protocol to use.  */
};

struct hostent *gethostbyname (const char *__name);

#endif
