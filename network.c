#include "network.h"
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>

char* get_host_ipv4(void){
    struct ifaddrs *ifs;
    if (getifaddrs(&ifs) == -1)
        return NULL;

    char *out = NULL;
    for (struct ifaddrs *ifa = ifs; ifa; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr || ifa->ifa_addr->sa_family != AF_INET)
            continue;
        struct sockaddr_in *sa = (struct sockaddr_in *)ifa->ifa_addr;
        if (ntohl(sa->sin_addr.s_addr) == INADDR_LOOPBACK) continue; /* skip loopback */
        char buf[INET_ADDRSTRLEN];
        if (!inet_ntop(AF_INET, &sa->sin_addr, buf, sizeof(buf))) continue;
        out = strdup(buf);
        break;
    }

    freeifaddrs(ifs);
    return out;
}
