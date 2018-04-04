#ifndef HID_H
#define HID_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hid_struct hid_t;

#define RAWHID_STEP_DEV     1
#define RAWHID_STEP_IFACE   2
#define RAWHID_STEP_REPORT  3
#define RAWHID_STEP_OPEN    4

struct rawhid_detail {
    int step;
    // device
    unsigned long bus;
    unsigned long device;
    unsigned short vid;
    unsigned short pid;
    // interface
    unsigned char interface;
    unsigned char ifclass;
    unsigned char subclass;
    unsigned char protocol;
    // report desc
    const unsigned char *report_desc;
    unsigned short rdesc_len;
    unsigned short usage_page;
    unsigned short usage;
    // open
    hid_t *hid;
};

typedef int (*rawhid_filter_cb)(void *user, const struct rawhid_detail *detail);

hid_t *rawhid_open(int vid, int pid, int usage_page, int usage);
void rawhid_close(hid_t *hid);

int rawhid_openall(hid_t **hid, int max, int vid, int pid, int usage_page, int usage);
int rawhid_openall_filter(rawhid_filter_cb cb, void *user);

int rawhid_recv(hid_t *hid, void *buf, int len, int timeout);
int rawhid_send(hid_t *hid, const void *buf, int len, int timeout);

#ifdef __cplusplus
}
#endif

#endif // HID_H
