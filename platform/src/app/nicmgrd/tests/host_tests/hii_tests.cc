#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stddef.h>
#include <sys/mman.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include <string>
#include <ctime>

/* Supply these for ionic_if.h */
typedef u_int8_t u8;
typedef u_int16_t u16;
typedef u_int16_t __le16;
typedef u_int32_t u32;
typedef u_int32_t __le32;
typedef u_int64_t u64;
typedef u_int64_t __le64;
typedef u_int64_t dma_addr_t;
typedef u_int64_t phys_addr_t;
typedef u_int64_t cpumask_t;
#define BIT(n)  (1 << (n))
#define BIT_ULL(n)  (1ULL << (n))
#define __iomem

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#include "platform/drivers/common/ionic_if.h"
#include "ionic_if.h"

#define DATA_SIZE (sizeof(((ionic_dev_cmd_regs *)NULL)->data))
#define IONIC_DEV_CMD_DONE              0x00000001
#define MAP_SZ	(4096)
#define MAX_DEVS	(3)

union ionic_dev_cmd_regs *cmd_regs[MAX_DEVS];
union ionic_dev_info_regs *info_regs[MAX_DEVS];
int dev_cnt;
char bdfs[MAX_DEVS][36];
void *mapped[MAX_DEVS];
char *progname;

struct args {
	bool oob_en;
	bool uid_led_on;
	bool vlan_en;
	__le16 vlan;
};

static void usage()
{
	printf("%s [-a[ALL] | -b[IDENTIFY] | -i[INIT] | -s[SET] | -g[GET] -r[RESET] \n",
           progname);
	exit(1);
}

static int go(int dev_no, union ionic_dev_cmd *cmd, void *req_data,
              union ionic_dev_cmd_comp *comp, void *resp_data)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(cmd->words); i++)
		cmd_regs[dev_no]->cmd.words[i] = cmd->words[i];
    if (req_data)
        memcpy((uint8_t *)cmd_regs[dev_no]->data, (uint8_t *)req_data, DATA_SIZE);

	cmd_regs[dev_no]->done = 0;
	cmd_regs[dev_no]->doorbell = 1;

	while (!(cmd_regs[dev_no]->done & IONIC_DEV_CMD_DONE));

	for (i = 0; i < ARRAY_SIZE(comp->words); i++)
		comp->words[i] = cmd_regs[dev_no]->comp.words[i];

    if (resp_data)
        memcpy((uint8_t *)resp_data, (uint8_t *)cmd_regs[dev_no]->data, DATA_SIZE);

	return 1;
}

static bool hii_identify(int dev_no, void *buf)
{
	union ionic_dev_cmd cmd = {};
	union ionic_dev_cmd_comp comp = {};

	if (!buf) {
		printf("dev %d: IDENTIFY ERR: needs resp_data buffer\n", dev_no);
		return false;
	}

	cmd.hii_identify.opcode = IONIC_CMD_HII_IDENTIFY;
	cmd.hii_identify.ver = IONIC_HII_IDENTITY_VERSION;

	go(dev_no, &cmd, NULL, &comp, buf);
	if (comp.hii_identify.status != 0) {
		printf("dev %d: IDENTIFY ERR: status: %d\n", dev_no, comp.hii_identify.status);
		return false;
	}
	printf("IDENTIFY SUCCESS\n");

    return true;
}

static bool hii_init(int dev_no, struct args *args)
{
	union ionic_dev_cmd cmd = {};
	union ionic_dev_cmd_comp comp = {};

	cmd.hii_init.opcode = IONIC_CMD_HII_INIT;
    	cmd.hii_init.oob_en = args->oob_en;
    	cmd.hii_init.uid_led_on = args->uid_led_on;
		cmd.hii_init.vlan_en = args->vlan_en;
    	cmd.hii_init.vlan = args->vlan;


	go(dev_no, &cmd, NULL, &comp, NULL);
	if (comp.hii_init.status != 0) {
		printf("dev %d: INIT ERR: status: %d\n", dev_no, comp.hii_init.status);
        return false;
	}
    printf("dev %d: INIT SUCCESS\n", dev_no);

    return true;
}

static bool hii_setattr(int dev_no, int attr, struct args *args)
{
	union ionic_dev_cmd cmd = {};
	union ionic_dev_cmd_comp comp = {};
	std::string attr_name = "";

	cmd.hii_setattr.opcode = IONIC_CMD_HII_SETATTR;

	switch (attr)
	{
	case IONIC_HII_ATTR_OOB_EN:
		/* OOB Enable */
		cmd.hii_setattr.attr = IONIC_HII_ATTR_OOB_EN;
		cmd.hii_setattr.oob_en = args->oob_en;
		attr_name = "oob_en";
		break;

	case IONIC_HII_ATTR_UID_LED:
		/* Set UID Led */
		cmd.hii_setattr.attr = IONIC_HII_ATTR_UID_LED;
		cmd.hii_setattr.uid_led_on = args->uid_led_on;
		attr_name = "uid_led_on";
		break;
	case IONIC_HII_ATTR_VLAN:
		/* Vlan */
		cmd.hii_setattr.attr = IONIC_HII_ATTR_VLAN;
		cmd.hii_setattr.vlan.enable = args->vlan_en;
		cmd.hii_setattr.vlan.id = args->vlan;
		attr_name = "vlan";
		break;
	default:
		printf("dev %d: SETATTR: Invalid attr %d\n", dev_no, attr);
		break;
	}

	go(dev_no, &cmd, NULL, &comp, NULL);
	if (comp.hii_setattr.status != 0) {
		printf("dev %d: SETATTR ERR: status: %d\n", dev_no, comp.hii_setattr.status);
        return false;
	}
	printf("dev %d: SETATTR %s SUCCESS\n", dev_no, attr_name.c_str());

    return true;
}

static bool hii_getattr(int dev_no, int attr, struct args *args)
{
	union ionic_dev_cmd cmd = {};
	union ionic_dev_cmd_comp comp = {};
	std::string attr_name = "";

	cmd.hii_getattr.opcode = IONIC_CMD_HII_GETATTR;

	switch (attr)
	{
	case IONIC_HII_ATTR_OOB_EN:
		/* OOB Enable */
		cmd.hii_getattr.attr = IONIC_HII_ATTR_OOB_EN;
		attr_name = "oob_en";
		break;
	case IONIC_HII_ATTR_UID_LED:
		/* Set UID Led */
		cmd.hii_getattr.attr = IONIC_HII_ATTR_UID_LED;
		attr_name = "uid_led_on";
		break;
	case IONIC_HII_ATTR_VLAN:
		/* Vlan */
		cmd.hii_getattr.attr = IONIC_HII_ATTR_VLAN;
		attr_name = "vlan";
		break;
	default:
		printf("dev %d: GETATTR: Invalid attr %d\n", dev_no, attr);
		return false;
	}

	go(dev_no, &cmd, NULL, &comp, NULL);
	if (comp.hii_getattr.status != 0) {
		printf("dev %d: GETATTR ERR: status: %d\n", dev_no, comp.hii_getattr.status);
        return false;
	}

	switch (attr)
	{
	case IONIC_HII_ATTR_OOB_EN:
		args->oob_en = comp.hii_getattr.oob_en;
		break;
	case IONIC_HII_ATTR_UID_LED:
		args->uid_led_on = comp.hii_getattr.uid_led_on;
		break;
	case IONIC_HII_ATTR_VLAN:
		args->vlan_en = comp.hii_getattr.vlan.enable;
		args->vlan = comp.hii_getattr.vlan.id;
		break;
	default:
		return false;
	}
	printf("GETATTR %s SUCCESS\n", attr_name.c_str());

    return true;
}

static bool hii_reset(int dev_no)
{
	union ionic_dev_cmd cmd = {};
	union ionic_dev_cmd_comp comp = {};

	cmd.hii_init.opcode = IONIC_CMD_HII_RESET;

	go(dev_no, &cmd, NULL, &comp, NULL);
	if (comp.hii_init.status != 0) {
		printf("dev %d: RESET ERR: status: %d\n", dev_no, comp.hii_init.status);
        return false;
	}
    printf("dev %d: RESET SUCCESS\n", dev_no);

    return true;
}

bool hii_identify_test(int dev_no)
{
	uint8_t buf[DATA_SIZE];
	union ionic_hii_dev_identity *hii_ident;
	bool ncsi_cap;
	printf("\ndev %d: HII_IDENTIFY_TEST\n", dev_no);
	if (!hii_identify(dev_no, buf)) {
		printf("HII_IDENTIFY_TEST: failed\n");
		return false;
	}

	hii_ident = (union ionic_hii_dev_identity *)buf;
	printf("dev %d: HII_IDENTIFY_TEST: oob_en: %d uid_led_on %d vlan_en: %d vlan: %u\n",
		   dev_no, hii_ident->oob_en, hii_ident->uid_led_on, hii_ident->vlan_en, hii_ident->vlan);
	ncsi_cap = hii_ident->capabilities & (1 << IONIC_HII_CAPABILITY_NCSI);
	printf("Capabilities: %s\n", ncsi_cap ? "NCSI_ENABLED" : "NCSI_DISABLED");
	printf("dev %d: HII_IDENTIFY_TEST: Passed\n", dev_no);
	return true;
}

bool hii_init_test(int dev_no)
{
	struct args args = {
		.oob_en = std::rand() % 2,
		.uid_led_on = std::rand() % 2,
		.vlan_en = std::rand() % 2,
		.vlan = (u16)(std::rand() % 4096),
	};
	uint8_t buf[DATA_SIZE];
	union ionic_hii_dev_identity *hii_ident;
	bool fail = false;

	printf("\nHII_INIT_TEST\n");
	if (!hii_init(dev_no, &args)) {
		printf("dev %d: HII_INIT_TEST: failed\n", dev_no);
		return false;
	}
	if (!hii_identify(dev_no, buf)) {
		printf("dev %d: HII_INIT_TEST: HII_IDENTIFY Failed\n", dev_no);
		return false;
	}
	hii_ident = (union ionic_hii_dev_identity *)buf;
	if (hii_ident->oob_en != args.oob_en) {
		printf("dev %d: HII_INIT_TEST: failed, oob_en mismatch obs: %d cfg: %d\n",
		       dev_no, hii_ident->oob_en, args.oob_en);
		fail = true;
	}
	if (hii_ident->uid_led_on != args.uid_led_on) {
		printf("dev %d: HII_INIT_TEST: failed, uid_led_on mismatch obs: %d cfg: %d\n",
		       dev_no, hii_ident->uid_led_on, args.uid_led_on);
		fail = true;
	}
	if (hii_ident->vlan_en != args.vlan_en) {
		printf("dev %d: HII_INIT_TEST: failed, vlan_en mismatch obs: %d cfg: %d\n",
		       dev_no, hii_ident->vlan_en, args.vlan_en);
		fail = true;
	}
	if (hii_ident->vlan != args.vlan) {
		printf("dev %d: HII_INIT_TEST: failed, vlan mismatch obs: %u cfg: %u\n",
		       dev_no, hii_ident->vlan, args.vlan);
		fail = true;
	}
	if (fail)
		return false;

	printf("dev %d: HII_INIT_TEST: Passed\n", dev_no);
	return true;
}

bool hii_setget_test (int dev_no)
{
	struct args args = {
		.oob_en = std::rand() % 2,
		.uid_led_on = std::rand() % 2,
		.vlan_en = std::rand() % 2,
		.vlan = (u16)(std::rand() % 4096),
	};
	struct args args2 = {.oob_en = false, .uid_led_on = false, .vlan_en = false, .vlan = 21};

	printf("\ndev %d: HII_SETGET_TEST\n", dev_no);
	/* OOB_EN */
	if (!hii_setattr(dev_no, IONIC_HII_ATTR_OOB_EN, &args)) {
		printf("dev %d: HII_SETGET_TEST: failed, set oob_en\n", dev_no);
		return false;
	}
	if (!hii_getattr(dev_no, IONIC_HII_ATTR_OOB_EN, &args2)) {
		printf("dev %d: HII_SETGET_TEST: failed, get oob_en\n", dev_no);
		return false;
	}
	if (args.oob_en != args2.oob_en) {
		printf("dev %d: HII_SETGET_TEST: failed, oob_en mismatch cfg: %d obs: %d\n",
		       dev_no, args.oob_en, args2.oob_en);
		return false;
	}
	printf("dev %d: HII_SETGET_TEST: oob_en: %d PASS\n", dev_no, args.oob_en);

	/* Set UID Led */
	if (!hii_setattr(dev_no, IONIC_HII_ATTR_UID_LED, &args)) {
		printf("dev %d: HII_SETGET_TEST: failed, set uid_led_on\n", dev_no);
		return false;
	}
	if (!hii_getattr(dev_no, IONIC_HII_ATTR_UID_LED, &args2)) {
		printf("dev %d: HII_SETGET_TEST: failed, get uid_led_on\n", dev_no);
		return false;
	}
	if (args.uid_led_on != args2.uid_led_on) {
		printf("dev %d: HII_SETGET_TEST: failed, uid_led_on mismatch cfg: %d obs: %d\n",
			   dev_no, args.uid_led_on, args2.uid_led_on);
		return false;
	}
	printf("dev %d: HII_SETGET_TEST: uid_led_on: %d PASS\n", dev_no, args.uid_led_on);

	/* VLAN_EN */
	if (!hii_setattr(dev_no, IONIC_HII_ATTR_VLAN, &args)) {
		printf("dev %d: HII_SETGET_TEST: failed, set vlan\n", dev_no);
		return false;
	}
	if (!hii_getattr(dev_no, IONIC_HII_ATTR_VLAN, &args2)) {
		printf("dev %d: HII_SETGET_TEST: failed, get vlan\n", dev_no);
		return false;
	}
	if (args.vlan_en != args2.vlan_en) {
		printf("dev %d: HII_SETGET_TEST: failed, vlan_en mismatch cfg: %d obs: %d\n",
			   dev_no, args.vlan_en, args2.vlan_en);
		return false;
	}
	if (args.vlan && args.vlan != args2.vlan) {
		printf("dev %d: HII_SETGET_TEST: failed, vlan mismatch cfg: %u obs: %u\n",
		       dev_no, args.vlan, args2.vlan);
		return false;
	}
	printf("dev %d: HII_SETGET_TEST: vlan_en: %d vlan: %u PASS\n", dev_no, args.vlan_en, args.vlan);

	printf("dev %d: HII HII_SETGET_TEST: PASS\n", dev_no);
	return true;
}

bool hii_reset_test(int dev_no)
{
	printf("dev %d: HII RESET_TEST\n", dev_no);
	if(!hii_reset(dev_no)) {
		printf("dev %d: HII_RESET_TEST: failed\n", dev_no);
		return false;
	}
	printf("dev %d: HII RESET_TEST: PASS\n", dev_no);
	return true;
}

bool hii_all_tests()
{
	int fail = 0;
	int test_cnt = 0;
	for (int dev_no = 0; dev_no < dev_cnt; dev_no++) {
		if (!hii_identify_test(dev_no))
			fail++;
		if (!hii_init_test(dev_no))
			fail++;
		if (!hii_setget_test(dev_no))
			fail++;
		if (!hii_reset_test(dev_no))
			fail++;
		test_cnt += 4;
	}
	if (fail) {
		printf("\n%d tests failed out of %d\n", fail, test_cnt);
		return false;
	}
    printf("\nALL TESTS PASSED!\n");
    return true;
}

int get_pcie_bdf()
{
	FILE *fp;
	char output[1024];
	int sz;

	fp = popen("lspci | egrep 'Ethernet' | egrep 'Device 1dd8' | awk '{print $1}'", "r");
	if (fp == NULL) {
		printf("Failed to run cmd\n");
		exit(1);
	}

	while (fgets(bdfs[dev_cnt], sizeof(bdfs[dev_cnt]), fp) != NULL) {
		printf("bdf: %s", bdfs[dev_cnt]);
		if ((sz = strlen(bdfs[dev_cnt])) <= 1) {
			printf("Invalid pcie bdf %s", bdfs[dev_cnt]);
			exit(1);
		}
		if (bdfs[dev_cnt][sz - 1] == '\n') {
			bdfs[dev_cnt][sz - 1] = 0;
		}
		if (++dev_cnt >= MAX_DEVS)
			break;
	}
	fclose(fp);
}

void init_eth_devices()
{
	char pci_res0_file[128];
	int pci_res0_fd;
	int dev_no;

	get_pcie_bdf();
	for (dev_no = 0; dev_no < dev_cnt; dev_no++) {
		/* get the 64-bit bar offset from setpci in two 32-bit chunks */
		printf("Initializing pcie dev: %s\n", bdfs[dev_no]);
		snprintf(pci_res0_file, sizeof(pci_res0_file),
			"/sys/bus/pci/devices/0000:%s/resource0", bdfs[dev_no]);
		printf("pci_res0_file: %s\n", pci_res0_file);

		pci_res0_fd = open(pci_res0_file, O_RDWR | O_SYNC);
		if (pci_res0_fd == -1) {
			printf("Failed to open: %s err: %s\n", pci_res0_file, strerror(errno));
			exit(1);
		}

		mapped[dev_no] = mmap(NULL, MAP_SZ, PROT_READ | PROT_WRITE, MAP_SHARED,
								pci_res0_fd, 0);
		if (mapped[dev_no] == MAP_FAILED) {
			printf("mmap failed for bdf: %s err: %s\n", bdfs[dev_no], strerror(errno));
			exit(1);
		}
		close(pci_res0_fd);

		info_regs[dev_no] = (union ionic_dev_info_regs *)((uint8_t *)mapped[dev_no] + IONIC_BAR0_DEV_INFO_REGS_OFFSET);

		if (info_regs[dev_no]->signature != IONIC_DEV_INFO_SIGNATURE) {
			fprintf(stderr,
				"Signature mismatch. Expected 0x%08x, got 0x%08x\n",
				IONIC_DEV_INFO_SIGNATURE, info_regs[dev_no]->signature);
		}
		cmd_regs[dev_no] = (union ionic_dev_cmd_regs *) ((uint8_t *)mapped[dev_no] + IONIC_BAR0_DEV_CMD_REGS_OFFSET);
		printf("Initializing pcie devs done\n");
	}
}

void deinit_eth_devices()
{
	printf("Cleanup\n");
		for (int dev_no = 0; dev_no < dev_cnt; dev_no++)
    		if (mapped[dev_no])
	    		munmap(mapped[dev_no], MAP_SZ);
}

int main(int argc, char **argv)
{
	int num_devs;
	int i;
	progname = argv[0];
	int opt;
    bool all_tests = false;
    bool identify_test = false;
    bool init_test = false;
    bool set_test = false;
    bool get_test = false;
    bool reset_test = false;

	std::srand(std::time(nullptr));

    /* Init devices */
	init_eth_devices();

	if (dev_cnt >= 2)
		dev_cnt = 2;

	while ((opt = getopt(argc, argv, "abisr")) != EOF) {
		switch (opt)
		{
		case 'a':
			hii_all_tests();
            goto test_done;
		case 'b':
	    	hii_identify_test(0);
            goto test_done;
        case 'i':
			hii_init_test(0);
			goto test_done;
		case 's':
			hii_setget_test(0);
			goto test_done;
        case 'r':
			hii_reset_test(0);
            goto test_done;
		default:
			printf("Invalid command");
			usage();
			goto test_done;
		}
	}



test_done:
    /* Clean up */
	deinit_eth_devices();
	return 0;
}
