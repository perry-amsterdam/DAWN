#include "dawn_iwinfo.h"

#include <limits.h>
#include <iwinfo.h>
#include <dirent.h>

#include "utils.h"
#include "ubus.h"

#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]

int call_iwinfo(char *client_addr);

int parse_rssi(char *iwinfo_string);

int get_rssi(const char *ifname, uint8_t *client_addr);

int get_bandwidth(const char *ifname, uint8_t *client_addr, float *rx_rate, float *tx_rate);

#define IWINFO_BUFSIZE    24 * 1024

#define IWINFO_ESSID_MAX_SIZE    32

int compare_essid_iwinfo(__uint8_t *bssid_addr, __uint8_t *bssid_addr_to_compare) {
    const struct iwinfo_ops *iw;

    char mac_buf[20];
    char mac_buf_to_compare[20];
    sprintf(mac_buf, MACSTR, MAC2STR(bssid_addr));
    sprintf(mac_buf_to_compare, MACSTR, MAC2STR(bssid_addr_to_compare));

    DIR *dirp;
    struct dirent *entry;
    dirp = opendir(hostapd_dir_glob);  // error handling?
    if (!dirp) {
        fprintf(stderr, "[COMPARE ESSID] No hostapd sockets!\n");
        return 0;
    }

    char *essid = NULL;
    char *essid_to_compare = NULL;

    char buf_essid[IWINFO_ESSID_MAX_SIZE + 1] = {0};
    char buf_essid_to_compare[IWINFO_ESSID_MAX_SIZE + 1] = {0};

    while ((entry = readdir(dirp)) != NULL && (essid == NULL || essid_to_compare == NULL)) {
        if (entry->d_type == DT_SOCK) {

            iw = iwinfo_backend(entry->d_name);

            static char buf_bssid[18] = {0};
            if (iw->bssid(entry->d_name, buf_bssid))
                snprintf(buf_bssid, sizeof(buf_bssid), "00:00:00:00:00:00");

            if (strcmp(mac_buf, buf_bssid) == 0) {

                if (iw->ssid(entry->d_name, buf_essid))
                    memset(buf_essid, 0, sizeof(buf_essid));
                essid = buf_essid;
            }

            if (strcmp(mac_buf_to_compare, buf_bssid) == 0) {
                if (iw->ssid(entry->d_name, buf_essid_to_compare))
                    memset(buf_essid_to_compare, 0, sizeof(buf_essid_to_compare));
                essid_to_compare = buf_essid_to_compare;
            }
        }
    }
    closedir(dirp);

    printf("Comparing: %s with %s\n", essid, essid_to_compare);

    if (essid == NULL || essid_to_compare == NULL) {
        return -1;
    }

    if (strcmp(essid, essid_to_compare) == 0) {
        return 0;
    }

    return -1;
}

int get_bandwidth_iwinfo(__uint8_t *client_addr, float *rx_rate, float *tx_rate) {

    DIR *dirp;
    struct dirent *entry;
    dirp = opendir(hostapd_dir_glob);  // error handling?
    if (!dirp) {
        fprintf(stderr, "[BANDWITH INFO]No hostapd sockets!\n");
        return 0;
    }

    int sucess = 0;

    while ((entry = readdir(dirp)) != NULL) {
        if (entry->d_type == DT_SOCK) {
            if (get_bandwidth(entry->d_name, client_addr, rx_rate, tx_rate)) {
                sucess = 1;
                break;
            }
        }
    }
    closedir(dirp);
    return sucess;
}

int get_bandwidth(const char *ifname, uint8_t *client_addr, float *rx_rate, float *tx_rate) {

    int i, len;
    char buf[IWINFO_BUFSIZE];
    struct iwinfo_assoclist_entry *e;
    const struct iwinfo_ops *iw;

    iw = iwinfo_backend(ifname);

    if (iw->assoclist(ifname, buf, &len)) {
        printf("No information available\n");
        iwinfo_finish();
        return 0;
    } else if (len <= 0) {
        printf("No station connected\n");
        iwinfo_finish();
        return 0;
    }

    for (i = 0; i < len; i += sizeof(struct iwinfo_assoclist_entry)) {
        e = (struct iwinfo_assoclist_entry *) &buf[i];

        if (mac_is_equal(client_addr, e->mac)) {
            *rx_rate = e->rx_rate.rate / 1000;
            *tx_rate = e->tx_rate.rate / 1000;
            return 1;
        }
    }

    return 0;
}

int get_rssi_iwinfo(__uint8_t *client_addr) {

    DIR *dirp;
    struct dirent *entry;
    dirp = opendir(hostapd_dir_glob);  // error handling?
    if (!dirp) {
        fprintf(stderr, "[RSSI INFO] No hostapd sockets!\n");
        return INT_MIN;
    }

    int rssi = INT_MIN;

    while ((entry = readdir(dirp)) != NULL) {
        if (entry->d_type == DT_SOCK) {
            rssi = get_rssi(entry->d_name, client_addr);
            if (rssi != INT_MIN)
                break;
        }
    }
    closedir(dirp);
    return rssi;
}

int get_rssi(const char *ifname, uint8_t *client_addr) {

    int i, len;
    char buf[IWINFO_BUFSIZE];
    struct iwinfo_assoclist_entry *e;
    const struct iwinfo_ops *iw;

    iw = iwinfo_backend(ifname);

    if (iw->assoclist(ifname, buf, &len)) {
        printf("No information available\n");
        iwinfo_finish();
        return INT_MIN;
    } else if (len <= 0) {
        printf("No station connected\n");
        iwinfo_finish();
        return INT_MIN;
    }

    for (i = 0; i < len; i += sizeof(struct iwinfo_assoclist_entry)) {
        e = (struct iwinfo_assoclist_entry *) &buf[i];

        if (mac_is_equal(client_addr, e->mac))
            return e->signal;
    }

    iwinfo_finish();
    return INT_MIN;
}

int get_expected_throughput_iwinfo(__uint8_t *client_addr) {

    DIR *dirp;
    struct dirent *entry;
    dirp = opendir(hostapd_dir_glob);  // error handling?
    if (!dirp) {
        fprintf(stderr, "[RSSI INFO] No hostapd sockets!\n");
        return INT_MIN;
    }

    int exp_thr = INT_MIN;

    while ((entry = readdir(dirp)) != NULL) {
        if (entry->d_type == DT_SOCK) {
            exp_thr = get_expected_throughput(entry->d_name, client_addr);
            if (exp_thr != INT_MIN)
                break;
        }
    }
    closedir(dirp);
    return exp_thr;
}

int get_expected_throughput(const char *ifname, uint8_t *client_addr) {

    int i, len;
    char buf[IWINFO_BUFSIZE];
    struct iwinfo_assoclist_entry *e;
    const struct iwinfo_ops *iw;

    iw = iwinfo_backend(ifname);

    if (iw->assoclist(ifname, buf, &len)) {
        printf("No information available\n");
        return INT_MIN;
    } else if (len <= 0) {
        printf("No station connected\n");
        return INT_MIN;
    }

    for (i = 0; i < len; i += sizeof(struct iwinfo_assoclist_entry)) {
        e = (struct iwinfo_assoclist_entry *) &buf[i];

        if (mac_is_equal(client_addr, e->mac))
            return e->thr;
    }
    iwinfo_finish();

    return INT_MIN;
}

int get_bssid(const char *ifname, uint8_t *bssid_addr) {
    const struct iwinfo_ops *iw;

    printf("GETTING BSSID OF: %s\n", ifname);

    iw = iwinfo_backend(ifname);

    printf("IW TYPE: %s\n", iwinfo_type(ifname));

    static char buf[18] = { 0 };

    if (iw->bssid(ifname, buf))
        snprintf(buf, sizeof(buf), "00:00:00:00:00:00");

    hwaddr_aton(buf,bssid_addr);
    iwinfo_finish();

    return 0;
}

int get_ssid(const char *ifname, char* ssid) {
    const struct iwinfo_ops *iw;

    char buf[IWINFO_ESSID_MAX_SIZE+1] = { 0 };
    iw = iwinfo_backend(ifname);
    if (iw->ssid(ifname, buf))
        memset(buf, 0, sizeof(buf));
    printf("SSID: %s\n", buf);
    memcpy(ssid, buf, (SSID_MAX_LEN) * sizeof(char));
    strcpy(ssid, buf);
    printf("SSID: %s\n", ssid);
    return 0;
}

int get_channel_utilization(const char *ifname, uint64_t *last_channel_time, uint64_t *last_channel_time_busy) {
    printf("GETTING UTILKIATUION FOR : %s\n", ifname);

    const struct iwinfo_ops *iw;
    struct iwinfo_survey_entry survey_entry;

    iw = iwinfo_backend(ifname);
    if (iw->survey(ifname, &survey_entry))
        return 0;

    uint64_t dividend = survey_entry.channel_time_busy - *last_channel_time_busy;
    uint64_t divisor =  survey_entry.channel_time - *last_channel_time;
    *last_channel_time = survey_entry.channel_time;
    *last_channel_time_busy = survey_entry.channel_time_busy;
    printf("dvidend: %llu\n", dividend);
    printf("divisior: %llu\n", divisor);

    printf("last_channel_time: %llu\n", *last_channel_time);
    printf("last_channel_time_busy: %llu\n", *last_channel_time_busy);

    printf("GOT SURVEY INFO!\n");
    return (int)(dividend * 255 / divisor);
    iwinfo_finish();
}

int support_ht(const char *ifname) {
    const struct iwinfo_ops *iw;

    iw = iwinfo_backend(ifname);
    int htmodes = 0;

    if (iw->htmodelist(ifname, &htmodes))
    {
        printf("No HT mode information available\n");
        return 0;
    }

    uint32_t ht_support_bitmask = (1 << 0) | (1 << 2);
    int ret = htmodes & ht_support_bitmask ? 1 : 0;
    iwinfo_finish();
    return ret;
}

int support_vht(const char *ifname) {
    const struct iwinfo_ops *iw;

    iw = iwinfo_backend(ifname);
    int htmodes = 0;

    if (iw->htmodelist(ifname, &htmodes))
    {
        printf("No HT mode information available\n");
        return 0;
    }

    uint32_t vht_support_bitmask = (1 << 2) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5) | (1 << 6);
    int ret = htmodes & vht_support_bitmask ? 1 : 0;
    iwinfo_finish();
    return ret;
}