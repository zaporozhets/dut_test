/*
 * dut_control_test.c
 *
 *  Created on: Apr 18, 2012
 *      Author: taras
 */

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

static int dev_info(int s, int dev_id, long arg);

int dut_hci_open(const char* hci_port_name);
int dut_hci_close(int sock);

int bt_reset(int sock);
int bt_le_tx_test(int sock, uint8_t freq, uint8_t payload, uint8_t len);
int bt_le_test_end(int sock);

int main(int argc, char* argv[])
{
    int sock;
    int retval = 0;

    char* hci_dev = "hci0";
    uint8_t freq = 0;
    uint8_t pattern = 0x01; // Repeated ‘11110000’
    uint8_t len = 20;

    int do_scan = 0;

    char c = 0;
    while ((c = getopt(argc, argv, "shci_for_each_dev(HCI_RAW, dev_info, 0);d:f:p:l:")) != -1) {
        switch (c) {
        case 'd': {
            hci_dev = "hci0";
            break;
        }
        case 'f': {
            freq = atoi(optarg);
            break;
        }
        case 'p': {
            pattern = atoi(optarg);
            break;
        }
        case 'l': {
            len = atoi(optarg);
            break;
        }
        case 's': {
            do_scan = 1;
            break;
        }
        case '?':
        default: {
            printf("Usage: dut_test -d hci0 -f 0 \n");
            printf("Options:\n");

            printf("/t-d/t/tHci device, default hci0\n");

            printf("/t-f/t/tBLE channel, default 0\n");

            printf("/t-f/t/tBLE test pattern, default 0x01\n");
            printf("/t/t0x00 - PRBS9 sequence ‘11111111100000111101…’ (in transmission order) as described in Volume 6 Part F Section 4.1.5\n");
            printf("/t/t0x01 - Repeated ‘11110000’ (in transmission order) sequence as described in Volume 6 Part F, Section 4.1.5\n");
            printf("/t/t0x02 - Repeated ‘10101010’ (in transmission order) sequence as described in Volume 6 Part F, Section 4.1.5\n");
            printf("/t/t0x03 - PRBS15 sequence as described in Volume 6 Part F, Section 4.1.5\n");
            printf("/t/t0x04 - Repeated ‘11111111’ (in transmission order) sequence\n");
            printf("/t/t0x05 - Repeated ‘00000000’ (in transmission order) sequence\n");
            printf("/t/t0x06 - Repeated ‘00001111’ (in transmission order) sequence\n");
            printf("/t/t0x07 - Repeated ‘01010101’ (in transmission order) sequence\n");

            printf("/t-l/t/tBLE test payload length, default 20 bytes\n");
            return 1;
        }
        }
    }

    if(do_scan) {
        printf("Devices:\n");
        hci_for_each_dev(HCI_UP, dev_info, 0);
        return 0;
    }

    printf("Device  : %s\n", hci_dev);
    printf("Freq    : %d\n", freq);
    printf("Pattern : 0x%02X\n", pattern);
    printf("Length  : %d\n", len);

    sock = dut_hci_open(hci_dev);

    retval = bt_reset(sock);

    retval = bt_le_tx_test(sock, freq, pattern, len);
    if (0 > retval) {
        fprintf(stderr, "Failed to start le transmitter test!\n");
        return retval;
    }
    getchar();

    retval = bt_le_test_end(sock);

    return 0;
}

int bt_reset(int sock)
{
    int retval;
    uint8_t status;
    struct hci_request req;

    printf("%s():\n", __func__);

    req.ogf = OGF_HOST_CTL;
    req.ocf = OCF_RESET;
    req.rparam = &status;
    req.rlen = 1;

    retval = hci_send_req(sock, &req, 1000);
    printf("retval = %d\n", retval);
    printf("status = %d\n", (int)status);

    return retval;
}

int bt_le_tx_test(int sock, uint8_t freq, uint8_t payload, uint8_t len)
{
    int retval = 0;
    le_transmitter_test_cp cmd;
    struct hci_request req;
    unsigned char status;

    printf("%s():\n", __func__);

    cmd.frequency = freq;
    cmd.length = len;

    cmd.payload = payload;

    req.ogf = OGF_LE_CTL;
    req.ocf = OCF_LE_TRANSMITTER_TEST;

    req.cparam = &cmd;
    req.clen = LE_TRANSMITTER_TEST_CP_SIZE;

    req.rparam = &status;
    req.rlen = 1;

    retval = hci_send_req(sock, &req, 1000);
    printf("retval = %d\n", retval);
    printf("status = %d\n", (int)status);
    return retval;
}

int bt_le_test_end(int sock)
{
    int retval = 0;
    le_test_end_rp status;
    struct hci_request req;

    printf("%s():\n", __func__);

    req.ogf = OGF_LE_CTL;
    req.ocf = OCF_LE_TEST_END;

    req.cparam = NULL;
    req.clen = 0;

    req.rparam = &status;
    req.rlen = LE_TEST_END_RP_SIZE;

    retval = hci_send_req(sock, &req, 1000);
    printf("retval = %d\n", retval);
    printf("status = %d\n", (int)status.status);
    printf("num_pkts = %d\n", (int)status.num_pkts);

    return retval;
}

static int dev_info(int s, int dev_id, long arg)
{
    (void)arg;
    struct hci_dev_info di = { .dev_id = dev_id };
    char addr[18];

    if (ioctl(s, HCIGETDEVINFO, (void*)&di))
        return 0;

    ba2str(&di.bdaddr, addr);
    printf("\t%s\t%s\n", di.name, addr);
    return 0;
}

int dut_hci_open(const char* hci_port_name)
{
    int dev_id;
    int sock;
    int ctl;

    dev_id = hci_devid(hci_port_name);

    /* Open HCI socket  */
    if ((ctl = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI)) < 0) {
        fprintf(stderr, "Can't open HCI socket.");
        return -1;
    }

    /* Up device HCI */
    if (ioctl(ctl, HCIDEVUP, dev_id) < 0) {
        if (errno != EALREADY) {
            fprintf(stderr, "Can't init device hci%d: %s (%d)\n", dev_id, strerror(errno), errno);
            return -1;
        }
    }

    /* Open HCI socket */
    sock = hci_open_dev(dev_id);

    if (0 > sock) {
        fprintf(stderr, "Fail to open hci device: %s\n", strerror(errno));
        return sock;
    }

    return sock;
}

int dut_hci_close(int sock)
{
    close(sock);
    return 0;
}
