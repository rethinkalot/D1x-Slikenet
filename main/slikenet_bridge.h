#pragma once
#ifdef __cplusplus
extern "C" {
#endif
void slikenet_init(void);
void slikenet_shutdown(void);
int  slikenet_send(const void *data, int len);
int  slikenet_receive(void *buf, int maxlen, unsigned int *out_ip, unsigned short *out_port);
int  slikenet_connect(const char *host, unsigned short port);
void slikenet_disconnect(unsigned int ip, unsigned short port);
#ifdef __cplusplus
}
#endif
