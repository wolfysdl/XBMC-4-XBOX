#ifndef SOCKETUTIL_H_
#define SOCKETUTIL_H_

#include <xtl.h>

struct hostent {
  char    FAR * h_name;           /* official name of host */
  char    FAR * FAR * h_aliases;  /* alias list */
  short   h_addrtype;             /* host address type */
  short   h_length;               /* length of address */
  char    FAR * FAR * h_addr_list; /* list of addresses */
#define h_addr  h_addr_list[0]          /* address, for backward compat */
};

typedef struct {
	struct hostent server;
	char name[128];
	char addr[16];
	char* addr_list[4];
} HostEnt;

int gethostname(char* buffer, int buffersize);

#ifdef __cplusplus
extern "C" {
#endif

struct hostent* FAR webs_gethostbyname(const char* _name);
char* webs_inet_ntoa (struct in_addr in);

#ifdef __cplusplus
}
#endif

#endif // SOCKETUTIL_H_