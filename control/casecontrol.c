#include "casecontrol.h"

int device_matches(libusb_device *dev, libusb_device_handle **handle);
int control_transfer_test(libusb_device_handle *handle);
int control_transfer_get(libusb_device_handle *handle, unsigned char *buf, int buf_len);
int control_transfer_set(libusb_device_handle *handle, int led, int value);
int get_ep_addr(libusb_device *dev);
int connect_device(libusb_context *ctx);

int daemonise();
void cleanup();
void sig_handler(int signo);
void call_scripts(char *script_dir, unsigned char value);

static const char test_msg[] = { 0xde, 0xad, 0xbe };
static const int test_msg_len = 3;

static libusb_device_handle *handle = NULL;
static unsigned char status[] = { 0, 0, 0 };
static int status_len = 3;

static unsigned char req_status[] = { 0, 0 };
static char alive = 1;
static char started = 0;

int
main(int argc, char *argv[])
{
	libusb_context *ctx = NULL;
	int ret;

	if(daemonise() == -1)
	{
		cleanup();
		return 1;
	}

	openlog(CASECONTROL_LOG_IDENT, LOG_PID, LOG_USER);

	if(signal(SIGINT, sig_handler) == SIG_ERR)
	{
		syslog(LOG_ERR, "Error installing signal handler for SIGINT: %s", strerror(errno));
		closelog();
		cleanup();
		return 1;
	}
	if(signal(SIGUSR1, sig_handler) == SIG_ERR)
	{
		syslog(LOG_ERR, "Error installing signal handler for SIGUSR1: %s", strerror(errno));
		closelog();
		cleanup();
		return 1;
	}
	if(signal(SIGUSR2, sig_handler) == SIG_ERR)
	{
		syslog(LOG_ERR, "Error installing signal handler for SIGUSR1: %s", strerror(errno));
		closelog();
		cleanup();
		return 1;
	}

	if(libusb_init(&ctx) < 0)
	{
		syslog(LOG_ERR, "Error initialising libusb");
		closelog();
		cleanup();
		return 1;
	}

	libusb_set_debug(ctx, 3);

	do
	{
		if((ret = connect_device(ctx)) == -1)
		{
			alive = 0;
		}
		else if(ret == -2)
		{
			sleep(5);
		}
	}
	while(alive);

	syslog(LOG_INFO, "Exiting...");

	libusb_exit(ctx);
	closelog();
	cleanup();

	return ret;
}

int
connect_device(libusb_context *ctx)
{
	libusb_device **devs;
	int ep_addr;

	int i, ret = 0;
	ssize_t count;

	int transferred = 0;

	if((count = libusb_get_device_list(ctx, &devs)) < 0)
	{
		syslog(LOG_ERR, "Error getting device list: %d", (int) count);
		return -1;
	}

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
			libusb_close(handle);
			handle = NULL;
			continue;
		}

		if((ep_addr = get_ep_addr(devs[i])) == -1)
		{
			libusb_close(handle);
			handle = NULL;
		}

		break;
	}

	libusb_free_device_list(devs, 1);

	if(handle == NULL)
	{
		if(started == 0)
			syslog(LOG_ERR, "No compatible devices found");
		started = 1;
		return -2;
	}

	syslog(LOG_INFO, "Compatible device found!");
	started = 1;

	if(control_transfer_get(handle, status, status_len) != 0)
	{
		libusb_close(handle);
		return -1;
	}

	req_status[0] = status[1];
	req_status[1] = status[2];

	while(alive)
	{
		if(req_status[0] != status[1])
			control_transfer_set(handle, 0, req_status[0]);
		if(req_status[1] != status[2])
			control_transfer_set(handle, 1, req_status[1]);

		if((ret = libusb_interrupt_transfer(handle, ep_addr, &(status[0]), 1, &transferred, CASECONTROL_INT_TIMEOUT)) == 0)
		{
			syslog(LOG_INFO, "SWITCH0: %hhu", status[0]);
			call_scripts(CASECONTROL_SWITCH0_SCRIPT_DIR, status[0]);
		}
		else if(ret == LIBUSB_ERROR_TIMEOUT)
			continue;
		else if(ret == LIBUSB_ERROR_NO_DEVICE)
		{
			syslog(LOG_INFO, "Device disconnected");
			libusb_close(handle);
			return -2;
		}
		else
		{
			syslog(LOG_ERR, "Error listening for interrupt: %d", ret);
			libusb_close(handle);
			return 1;
		}
	}

	libusb_close(handle);

	return 0;
}

int
daemonise()
{
	pid_t pid;
	int i, fd, len;
	char buf[10];

	openlog(CASECONTROL_LOG_IDENT, LOG_PID, LOG_USER);

	if((pid = fork()) == -1)
	{
		syslog(LOG_ERR, "fork(): %m");
		return -1;
	}
	else if(pid != 0)
	{
		syslog(LOG_INFO, "Daemon PID: %d", pid);

		if((fd = open(CASECONTROL_PID_FILE, O_CREAT | O_WRONLY, 00444)) == -1)
			syslog(LOG_ERR, "open(): %m");

		len = snprintf(buf, 9, "%d", pid);
		if(write(fd, buf, len) == -1)
			syslog(LOG_ERR, "write(): %m");
		close(fd); 

		closelog();
		exit(0);
	}

	if(setsid() == -1)
	{
		syslog(LOG_ERR, "setsid(): %m");
		return -1;
	}

	if(chdir(CASECONTROL_RUNDIR) == -1)
	{
		syslog(LOG_ERR, "chdir(): %m");
		return -1;
	}

	closelog();

	for(i = 0; i < NR_OPEN; ++i)
		close(i);

	open("/dev/null", O_RDWR); // stdin
	dup(0); // stdout
	dup(0); // stderr

	return 0;
}

void
cleanup()
{
	unlink(CASECONTROL_PID_FILE);
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
		syslog(LOG_ERR, "Error getting device descriptor: %d", ret);
		return 1;
	}

	if(desc.idVendor != CASECONTROL_VID || desc.idProduct != CASECONTROL_PID)
		return 1;

	if((ret = libusb_open(dev, handle)) != 0)
	{
		syslog(LOG_ERR, "Error opening device: %d", ret);
		*handle = NULL;
		return 1;
	}

	if((ret = libusb_kernel_driver_active(*handle, 0)) != 0)
	{
		if(ret == 1)
		{
			syslog(LOG_ERR, "Kernel driver active, detaching.");

			if((ret = libusb_detach_kernel_driver(*handle, 0)) != 0)
			{
				syslog(LOG_ERR, "Error detaching kernel driver: %d", ret);
				libusb_close(*handle);
				*handle = NULL;
				return 1;
			}
		}
		else
		{
			syslog(LOG_ERR, "Error checking if kernel driver is attached: %d", ret);
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
		syslog(LOG_ERR, "Error making control transfer: %d", len);
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
		syslog(LOG_ERR, "Error making control transfer: %d", len);
		return -1;
	}

	if(len < buf_len)
		return 1;

	syslog(LOG_INFO, "SWITCH0: %hhu", status[0]);
	call_scripts(CASECONTROL_SWITCH0_SCRIPT_DIR, status[0]);

	for(i = 1; i < status_len; ++i)
	{
		syslog(LOG_INFO, "LED%d: %hhu", i - 1, status[i]);
		if(i - 1 == 0)
			call_scripts(CASECONTROL_LED0_SCRIPT_DIR, status[i]);
		else if(i - 1 == 1)
			call_scripts(CASECONTROL_LED1_SCRIPT_DIR, status[i]);
	}

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
		syslog(LOG_ERR, "Invalid LED number");
		return -1;
	}

	if((len = libusb_control_transfer(handle, CASECONTROL_REQUESTTYPE, rq, value, 0, ret, 1, 0)) < 0)
	{
		syslog(LOG_ERR, "Error making control transfer: %d", len);
		return -1;
	}

	if(len != 1)
		return -1;

	status[led + 1] = ret[0];

	syslog(LOG_INFO, "LED%d: %hhu", led, ret[0]);

	if(led == 0)
		call_scripts(CASECONTROL_LED0_SCRIPT_DIR, ret[0]);
	else
		call_scripts(CASECONTROL_LED1_SCRIPT_DIR, ret[0]);

	return 0;
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
		return;
	}
	else
		return;

	if(handle == NULL)
		return;

	if(status[led + 1] == 0)
		req_status[led] = 1;
	else
		req_status[led] = 0;

	return;
}

int
get_ep_addr(libusb_device *dev)
{
	struct libusb_device_descriptor desc;
	struct libusb_config_descriptor *config;

	const struct libusb_interface *inter;
	const struct libusb_interface_descriptor *interdesc;
	const struct libusb_endpoint_descriptor *epdesc;

	int ep_addr;

	if(libusb_get_device_descriptor(dev, &desc) < 0)
	{
		syslog(LOG_ERR, "Error getting device descriptor");
		return -1;
	}

	libusb_get_config_descriptor(dev, 0, &config);

	inter = &config->interface[0];
	interdesc = &inter->altsetting[0];
	epdesc = &interdesc->endpoint[0];
	ep_addr = (int)epdesc->bEndpointAddress;

	libusb_free_config_descriptor(config);

	return ep_addr;
}

void
call_scripts(char *script_dir, unsigned char value)
{
	struct dirent **dirs;
	int num_dirs, i;
	int pid, status;
	char path[PATH_MAX];

	char val_str[2];

	if(value == 0)
		snprintf(val_str, 2, "0");
	else
		snprintf(val_str, 2, "1");

	if((num_dirs = scandir(script_dir, &dirs, NULL, alphasort)) == -1)
	{
		syslog(LOG_ERR, "scandir(%s): %s", script_dir, strerror(errno));
		return;
	}

	for(i = 0; i < num_dirs; ++i)
	{
		if(dirs[i]->d_type == DT_DIR)
			continue;

		snprintf(path, PATH_MAX - 1, "%s/%s", script_dir, dirs[i]->d_name);
		free(dirs[i]);

		syslog(LOG_INFO, "Executing \"%s\"", path);

		if((pid = fork()) == -1)
		{
			syslog(LOG_ERR, "fork(): %s", strerror(errno));
			continue;
		}
		else if(pid == 0)
		{
			execlp("/bin/sh", "/bin/sh", path, val_str, NULL);
		}
		wait(&status);

		if(status == 0)
		{
			syslog(LOG_INFO, "Executed \"%s\": SUCCESS", path);
		}
		else
		{
			syslog(LOG_ERR, "Executed \"%s\": FAILURE (%d)", path, status);
		}
	}

	free(dirs);

	return;
}

#if 0
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
#endif
