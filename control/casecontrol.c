#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <signal.h>

#include <libusb-1.0/libusb.h>

#define CASECONTROL_VID		0x1781
#define CASECONTROL_PID		0x1111
#define CASECONTROL_REQUESTTYPE	(1 << 7) | (1 << 6)
#define CASECONTROL_RQ_GET	0
#define CASECONTROL_RQ_TEST	1
#define CASECONTROL_RQ_LED0	2
#define CASECONTROL_RQ_LED1	3

int device_matches(libusb_device *dev, libusb_device_handle **handle);
int control_transfer_test(libusb_device_handle *handle);
int control_transfer_get(libusb_device_handle *handle, unsigned char *buf, int buf_len);
int control_transfer_set(libusb_device_handle *handle, int led, int value);
void print_dev(libusb_device *dev);

void sig_handler(int signo);

static const char test_msg[] = { 0xde, 0xad, 0xbe };
static const int test_msg_len = 3;

static libusb_device_handle *handle = NULL;
static unsigned char status[] = { 0, 0, 0 };
static int status_len = 3;

static char alive = 1;

int
main(int argc, char *argv[])
{
	libusb_device **devs;
	libusb_context *ctx = NULL;

	int i, ret = 0;
	ssize_t count;

	if(signal(SIGINT, sig_handler) == SIG_ERR)
	{
		fprintf(stderr, "Error installing signal handler for SIGINT: %s\n", strerror(errno));
		return 1;
	}
	if(signal(SIGUSR1, sig_handler) == SIG_ERR)
	{
		fprintf(stderr, "Error installing signal handler for SIGUSR1: %s\n", strerror(errno));
		return 1;
	}
	if(signal(SIGUSR2, sig_handler) == SIG_ERR)
	{
		fprintf(stderr, "Error installing signal handler for SIGUSR1: %s\n", strerror(errno));
		return 1;
	}

	if(libusb_init(&ctx) < 0)
	{
		fprintf(stderr, "Error initialising libusb\n");
		return 1;
	}

	libusb_set_debug(ctx, 3);

	if((count = libusb_get_device_list(ctx, &devs)) < 0)
	{
		fprintf(stderr, "Error getting device list: %d\n", (int) count);
		libusb_exit(ctx);
		return 1;
	}

	printf("%d devices in list\n", (int) count);

	for(i = 0; i < count; ++i)
	{
		if(device_matches(devs[i], &handle) != 0)
			continue;

		if((ret = control_transfer_test(handle)) == -1)
		{
			libusb_close(handle);
			handle = NULL;
			break;
		}
		else if(ret == 1)
		{
			printf("Device %d failed transfer test...\n", i);
			libusb_close(handle);
			handle = NULL;
			continue;
		}

		printf("Device %d:\n", i);
		
		print_dev(devs[i]);

		break;
	}

	libusb_free_device_list(devs, 1);

	if(handle == NULL)
	{
		fprintf(stderr, "No compatible devices found\n");
		libusb_exit(ctx);
		return 1;
	}

	printf("Compatible device found!\n");

	control_transfer_get(handle, status, status_len);

	control_transfer_set(handle, 0, 1);
	control_transfer_set(handle, 1, 0);

	control_transfer_get(handle, status, status_len);

	while(alive)
		sleep(1);

	libusb_close(handle);
	libusb_exit(ctx);

	return 0;
}

int
device_matches(libusb_device *dev, libusb_device_handle **handle)
{
	struct libusb_device_descriptor desc;
	int ret;

	if(dev == NULL || handle == NULL)
		return 1;

	*handle = NULL;

	if((ret = libusb_get_device_descriptor(dev, &desc)) != 0)
	{
		fprintf(stderr, "Error getting device descriptor: %d\n", ret);
		return 1;
	}

	if(desc.idVendor != CASECONTROL_VID || desc.idProduct != CASECONTROL_PID)
		return 1;

	if((ret = libusb_open(dev, handle)) != 0)
	{
		fprintf(stderr, "Error opening device: %d\n", ret);
		*handle = NULL;
		return 1;
	}

	if((ret = libusb_kernel_driver_active(*handle, 0)) != 0)
	{
		if(ret == 1)
		{
			fprintf(stderr, "Kernel driver active, detaching.\n");

			if((ret = libusb_detach_kernel_driver(*handle, 0)) != 0)
			{
				fprintf(stderr, "Error detaching kernel driver: %d\n", ret);
				libusb_close(*handle);
				*handle = NULL;
				return 1;
			}
		}
		else
		{
			fprintf(stderr, "Error checking if kernel driver is attached: %d\n", ret);
			libusb_close(*handle);
			*handle = NULL;
			return 1;
		}
	}

	return 0;
}

int
control_transfer_test(libusb_device_handle *handle)
{
	unsigned char buf[test_msg_len];
	int len;

	if(handle == NULL)
		return -1;

	if((len = libusb_control_transfer(handle, CASECONTROL_REQUESTTYPE, CASECONTROL_RQ_TEST, 0, 0, buf, test_msg_len, 0)) < 0)
	{
		fprintf(stderr, "Error making control transfer: %d\n", len);
		return -1;
	}

	if(len < test_msg_len)
		return 1;
	else if(strncmp((const char *) test_msg, (const char *) buf, test_msg_len) != 0)
		return 1;

	return 0;
}

int
control_transfer_get(libusb_device_handle *handle, unsigned char *buf, int buf_len)
{
	int i, len;

	if(handle == NULL)
		return -1;

	if((len = libusb_control_transfer(handle, CASECONTROL_REQUESTTYPE, CASECONTROL_RQ_GET, 0, 0, buf, buf_len, 0)) < 0)
	{
		fprintf(stderr, "Error making control transfer: %d\n", len);
		return -1;
	}

	if(len < buf_len)
		return 1;

	printf("SWITCH0:\t%hhu\n", status[0]);
	for(i = 1; i < status_len; ++i)
		printf("LED%d:\t\t%hhu\n", i - 1, status[i]);

	return 0;
}

int
control_transfer_set(libusb_device_handle *handle, int led, int value)
{
	int rq, len;
	unsigned char ret[1];

	if(handle == NULL)
		return -1;

	if(led == 0)
		rq = CASECONTROL_RQ_LED0;
	else if(led == 1)
		rq = CASECONTROL_RQ_LED1;
	else
	{
		fprintf(stderr, "Invalid LED number\n");
		return -1;
	}

	if((len = libusb_control_transfer(handle, CASECONTROL_REQUESTTYPE, rq, value, 0, ret, 1, 0)) < 0)
	{
		fprintf(stderr, "Error making control transfer: %d\n", len);
		return -1;
	}

	if(len != 1)
		return -1;

	status[led + 1] = ret[0];

	return 0;
}

void
print_dev(libusb_device *dev)
{
	struct libusb_device_descriptor desc;
	struct libusb_config_descriptor *config;

	int i, j, k;

	if(libusb_get_device_descriptor(dev, &desc) < 0)
	{
		fprintf(stderr, "Error getting device descriptor\n");
		return;
	}

	printf("\tNum configurations: %d\n", (int) desc.bNumConfigurations);
	printf("\tDevice class: %d\n", (int) desc.bDeviceClass);
	printf("\tVendor ID: 0x%.04x\n", (int) desc.idVendor);
	printf("\tProduct ID: 0x%.04x\n", (int) desc.idProduct);

	libusb_get_config_descriptor(dev, 0, &config);

	printf("\tNum interfaces: %d\n", (int) config->bNumInterfaces);

	for(i = 0; i < config->bNumInterfaces; ++i)
	{
		const struct libusb_interface *inter = &config->interface[i];

		printf("\tInterface %d\n", i);

		printf("\t\tNum alt settings: %d\n", inter->num_altsetting);

		for(j = 0; j < inter->num_altsetting; ++j)
		{
			const struct libusb_interface_descriptor *interdesc = &inter->altsetting[j];

			printf("\t\t\tInterface Number: %d\n", (int) interdesc->bInterfaceNumber);
			printf("\t\t\tNumber of Endpoints: %d\n", (int) interdesc->bNumEndpoints);

			for(k = 0; k < interdesc->bNumEndpoints; ++k)
			{
				const struct libusb_endpoint_descriptor *epdesc = &interdesc->endpoint[k];

				printf("\t\t\t\tEndpoint %d\n", k);
				printf("\t\t\t\t\tDescriptor type: %d\n", (int)epdesc->bDescriptorType);
				printf("\t\t\t\t\tEP Address: %d\n", (int)epdesc->bEndpointAddress);
			}
			
		}
	}

	libusb_free_config_descriptor(config);
}

void
sig_handler(int signo)
{
	int led;

	if(signo == SIGUSR1)
		led = 0;
	else if(signo == SIGUSR2)
		led = 1;
	else if(signo == SIGINT)
	{
		alive = 0;
		fprintf(stderr, "Interrupted!\n");
		return;
	}
	else
		return;

	if(handle == NULL)
		return;

	if(status[led + 1] == 0)
		control_transfer_set(handle, led, 1);
	else
		control_transfer_set(handle, led, 0);

	return;
}
