#include <stdio.h>

#include <native_netlib.h>
#include <ape_dns.h>

int ape_running;
ape_global * g_ape;

#define MIN_TIMEOUT 8

static int resolve_cb(const char *ip, void * arg, int status)
{
	ape_running = 0;
	return 1;
}


static int interval_cb(void *param) {
	int *p;

	p = (int*) param;
	return *p;
}

int main(const int argc, const char **argv)
{
	int minTime, resolve;

	g_ape = native_netlib_init();
	g_ape->is_running = ape_running = 1;

	resolve = 0;
	if (argc == 1) {
		minTime = MIN_TIMEOUT;
	} else if (argc == 2) {
		minTime = atoi(argv[1]);
	} else if (argc == 3) {
		minTime = atoi(argv[1]);
		resolve = 1;
	} else {
		printf("Usage: %s [timeout] [resolve]\n\tdefault timeout: %d\n\tactivate dns socket: %d", argv[0], MIN_TIMEOUT, resolve);
		exit(1);
	}
	printf("starting interval with %d with%s resolving\n", minTime, (resolve)?"":"out");
	ape_gethostbyname("nidium.com", resolve_cb, NULL, g_ape);
	if (resolve){
		add_timer(&g_ape->timersng, minTime, interval_cb, &minTime);
	}
	
	events_loop(g_ape);
	native_netlib_destroy(g_ape);

	return 0;
}
