#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/dhcpv4.h>
#include <zephyr/net/dhcpv4_server.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/wifi_credentials.h>
#include <zephyr/logging/log.h>
#include "phone_net.h"

LOG_MODULE_REGISTER(phone_net, LOG_LEVEL_INF);

/* the lead's network: Povodok .1, the phone from .2 (PHONE.md, The link) */
#define LEAD_ADDR    { { { 192, 168, 7, 1 } } }
#define LEAD_MASK    { { { 255, 255, 255, 0 } } }
#define LEAD_POOL    { { { 192, 168, 7, 2 } } }

static struct net_if *lead;               /* the USB network adapter's interface */
static struct net_if *wifi;
static struct net_mgmt_event_callback if_cb, wifi_cb;
static bool wifi_up;

static void lead_up(void)
{
	struct net_in_addr addr = LEAD_ADDR, mask = LEAD_MASK, pool = LEAD_POOL;
	static bool served;

	if (served) {
		return;
	}
	if (net_if_ipv4_addr_add(lead, &addr, NET_ADDR_MANUAL, 0) == NULL) {
		LOG_ERR("the lead's address did not take");
		return;
	}
	net_if_ipv4_set_netmask_by_addr(lead, &addr, &mask);
	if (net_dhcpv4_server_start(lead, &pool)) {
		LOG_ERR("the lead's DHCP server did not start");
		return;
	}
	served = true;
	LOG_INF("the lead's network up, 192.168.7.1, the phone from .2");
}

static void on_if_event(struct net_mgmt_event_callback *cb, uint64_t event, struct net_if *iface)
{
	ARG_UNUSED(cb);
	if (event == NET_EVENT_IF_UP && iface == lead) {
		lead_up();                       /* the cable's network, as the phone's host sees it */
	}
}

static void on_wifi_event(struct net_mgmt_event_callback *cb, uint64_t event, struct net_if *iface)
{
	const struct wifi_status *status = cb->info;

	if (event == NET_EVENT_WIFI_CONNECT_RESULT) {
		wifi_up = status != NULL && status->status == 0;
		if (wifi_up) {
			net_dhcpv4_start(iface);  /* the network's address, by its DHCP */
			LOG_INF("the Wi-Fi joined");
		} else {
			LOG_WRN("the Wi-Fi refused");
		}
	} else if (event == NET_EVENT_WIFI_DISCONNECT_RESULT) {
		wifi_up = false;
	}
}

int phone_net_init(void)
{
	lead = net_if_lookup_by_dev(DEVICE_DT_GET(DT_NODELABEL(cdc_ncm_eth0)));
	wifi = net_if_get_first_wifi();
	net_mgmt_init_event_callback(&if_cb, on_if_event, NET_EVENT_IF_UP);
	net_mgmt_add_event_callback(&if_cb);
	net_mgmt_init_event_callback(&wifi_cb, on_wifi_event,
				     NET_EVENT_WIFI_CONNECT_RESULT | NET_EVENT_WIFI_DISCONNECT_RESULT);
	net_mgmt_add_event_callback(&wifi_cb);
	if (lead != NULL && net_if_is_up(lead)) {
		lead_up();
	}
	if (wifi != NULL) {
		/* the network kept in the settings, joined at the start; nothing kept, nothing done */
		(void)net_mgmt(NET_REQUEST_WIFI_CONNECT_STORED, wifi, NULL, 0);
	}
	return lead != NULL ? 0 : -ENODEV;
}

int phone_net_wifi_set(const char *ssid, size_t ssid_len, const char *password, size_t pw_len)
{
	int rc;

	if (wifi == NULL || ssid_len == 0 || ssid_len > WIFI_SSID_MAX_LEN ||
	    pw_len > WIFI_PSK_MAX_LEN) {
		return -EINVAL;
	}
	/* kept in the settings, at 5 GHz alone, the band of PHONE.md */
	rc = wifi_credentials_set_personal(ssid, ssid_len,
					   pw_len ? WIFI_SECURITY_TYPE_PSK : WIFI_SECURITY_TYPE_NONE,
					   NULL, 0, password, pw_len, WIFI_CREDENTIALS_FLAG_5GHz, 0, 0);
	if (rc) {
		return rc;
	}
	return net_mgmt(NET_REQUEST_WIFI_CONNECT_STORED, wifi, NULL, 0);
}

bool phone_net_wifi_connected(void)
{
	return wifi_up;
}

void phone_net_forget(void)
{
	wifi_credentials_delete_all();
	if (wifi != NULL) {
		(void)net_mgmt(NET_REQUEST_WIFI_DISCONNECT, wifi, NULL, 0);
	}
}
