
#include <stdint.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <linux/rtnetlink.h>
#include <netpacket/packet.h>
#include <linux/filter.h>
#include <linux/errqueue.h>

#include <linux/pkt_sched.h>
#include <netlink/object-api.h>
#include <netlink/netlink.h>
#include <netlink/socket.h>
#include <netlink/handlers.h>

#include "sync.h"

#define LOG_TAG  "WifiHAL"
//#define LOG_NDEBUG 1         //uncomment to enable verbose logging

#include <utils/Log.h>

#include "wifi_hal.h"
#include "common.h"
#include "cpp_bindings.h"

typedef enum {

    GSCAN_ATTRIBUTE_NUM_BUCKETS = 10,
    GSCAN_ATTRIBUTE_BASE_PERIOD,
    GSCAN_ATTRIBUTE_BUCKETS_BAND,
    GSCAN_ATTRIBUTE_BUCKET_ID,
    GSCAN_ATTRIBUTE_BUCKET_PERIOD,
    GSCAN_ATTRIBUTE_BUCKET_NUM_CHANNELS,
    GSCAN_ATTRIBUTE_BUCKET_CHANNELS,
    GSCAN_ATTRIBUTE_NUM_AP_PER_SCAN,
    GSCAN_ATTRIBUTE_REPORT_THRESHOLD,
    GSCAN_ATTRIBUTE_NUM_SCANS_TO_CACHE,
    GSCAN_ATTRIBUTE_BAND = GSCAN_ATTRIBUTE_BUCKETS_BAND,

    GSCAN_ATTRIBUTE_ENABLE_FEATURE = 20,
    GSCAN_ATTRIBUTE_SCAN_RESULTS_COMPLETE,              /* indicates no more results */
    GSCAN_ATTRIBUTE_FLUSH_FEATURE,                      /* Flush all the configs */
    GSCAN_ENABLE_FULL_SCAN_RESULTS,
    GSCAN_ATTRIBUTE_REPORT_EVENTS,

    /* remaining reserved for additional attributes */
    GSCAN_ATTRIBUTE_NUM_OF_RESULTS = 30,
    GSCAN_ATTRIBUTE_FLUSH_RESULTS,
    GSCAN_ATTRIBUTE_SCAN_RESULTS,                       /* flat array of wifi_scan_result */
    GSCAN_ATTRIBUTE_SCAN_ID,                            /* indicates scan number */
    GSCAN_ATTRIBUTE_SCAN_FLAGS,                         /* indicates if scan was aborted */
    GSCAN_ATTRIBUTE_AP_FLAGS,                           /* flags on significant change event */
    GSCAN_ATTRIBUTE_NUM_CHANNELS,
    GSCAN_ATTRIBUTE_CHANNEL_LIST,

    /* remaining reserved for additional attributes */

    GSCAN_ATTRIBUTE_SSID = 40,
    GSCAN_ATTRIBUTE_BSSID,
    GSCAN_ATTRIBUTE_CHANNEL,
    GSCAN_ATTRIBUTE_RSSI,
    GSCAN_ATTRIBUTE_TIMESTAMP,
    GSCAN_ATTRIBUTE_RTT,
    GSCAN_ATTRIBUTE_RTTSD,

    /* remaining reserved for additional attributes */

    GSCAN_ATTRIBUTE_HOTLIST_BSSIDS = 50,
    GSCAN_ATTRIBUTE_RSSI_LOW,
    GSCAN_ATTRIBUTE_RSSI_HIGH,
    GSCAN_ATTRIBUTE_HOTLIST_ELEM,
    GSCAN_ATTRIBUTE_HOTLIST_FLUSH,

    /* remaining reserved for additional attributes */
    GSCAN_ATTRIBUTE_RSSI_SAMPLE_SIZE = 60,
    GSCAN_ATTRIBUTE_LOST_AP_SAMPLE_SIZE,
    GSCAN_ATTRIBUTE_MIN_BREACHING,
    GSCAN_ATTRIBUTE_SIGNIFICANT_CHANGE_BSSIDS,
    GSCAN_ATTRIBUTE_SIGNIFICANT_CHANGE_FLUSH,

    /* EPNO */
    GSCAN_ATTRIBUTE_EPNO_SSID_LIST = 70,
    GSCAN_ATTRIBUTE_EPNO_SSID,
    GSCAN_ATTRIBUTE_EPNO_SSID_LEN,
    GSCAN_ATTRIBUTE_EPNO_RSSI,
    GSCAN_ATTRIBUTE_EPNO_FLAGS,
    GSCAN_ATTRIBUTE_EPNO_AUTH,
    GSCAN_ATTRIBUTE_EPNO_SSID_NUM,
    GSCAN_ATTRIBUTE_EPNO_FLUSH,

    /* remaining reserved for additional attributes */

    GSCAN_ATTRIBUTE_WHITELIST_SSID = 80,
    GSCAN_ATTRIBUTE_NUM_WL_SSID,
    GSCAN_ATTRIBUTE_WL_SSID_LEN,
    GSCAN_ATTRIBUTE_WL_SSID_FLUSH,
    GSCAN_ATTRIBUTE_WHITELIST_SSID_ELEM,
    GSCAN_ATTRIBUTE_NUM_BSSID,
    GSCAN_ATTRIBUTE_BSSID_PREF_LIST,
    GSCAN_ATTRIBUTE_BSSID_PREF_FLUSH,
    GSCAN_ATTRIBUTE_BSSID_PREF,
    GSCAN_ATTRIBUTE_RSSI_MODIFIER,

    /* remaining reserved for additional attributes */

    GSCAN_ATTRIBUTE_A_BAND_BOOST_THRESHOLD = 90,
    GSCAN_ATTRIBUTE_A_BAND_PENALTY_THRESHOLD,
    GSCAN_ATTRIBUTE_A_BAND_BOOST_FACTOR,
    GSCAN_ATTRIBUTE_A_BAND_PENALTY_FACTOR,
    GSCAN_ATTRIBUTE_A_BAND_MAX_BOOST,
    GSCAN_ATTRIBUTE_LAZY_ROAM_HYSTERESIS,
    GSCAN_ATTRIBUTE_ALERT_ROAM_RSSI_TRIGGER,
    GSCAN_ATTRIBUTE_LAZY_ROAM_ENABLE,

    /* BSSID blacklist */
    GSCAN_ATTRIBUTE_BSSID_BLACKLIST_FLUSH = 100,
    GSCAN_ATTRIBUTE_BLACKLIST_BSSID,

    /* ANQPO */
    GSCAN_ATTRIBUTE_ANQPO_HS_LIST = 110,
    GSCAN_ATTRIBUTE_ANQPO_HS_LIST_SIZE,
    GSCAN_ATTRIBUTE_ANQPO_HS_NETWORK_ID,
    GSCAN_ATTRIBUTE_ANQPO_HS_NAI_REALM,
    GSCAN_ATTRIBUTE_ANQPO_HS_ROAM_CONSORTIUM_ID,
    GSCAN_ATTRIBUTE_ANQPO_HS_PLMN,

    /* Adaptive scan attributes */
    GSCAN_ATTRIBUTE_BUCKET_STEP_COUNT = 120,
    GSCAN_ATTRIBUTE_BUCKET_MAX_PERIOD,

    GSCAN_ATTRIBUTE_MAX

} GSCAN_ATTRIBUTE;


// helper methods
wifi_error wifi_enable_full_scan_results(wifi_request_id id, wifi_interface_handle iface,
         wifi_scan_result_handler handler);
wifi_error wifi_disable_full_scan_results(wifi_request_id id, wifi_interface_handle iface);


/////////////////////////////////////////////////////////////////////////////

class GetCapabilitiesCommand : public WifiCommand
{
    wifi_gscan_capabilities *mCapabilities;
public:
    GetCapabilitiesCommand(wifi_interface_handle iface, wifi_gscan_capabilities *capabitlites)
        : WifiCommand("GetGscanCapabilitiesCommand", iface, 0), mCapabilities(capabitlites)
    {
        memset(mCapabilities, 0, sizeof(*mCapabilities));
    }

    virtual int create() {
        ALOGV("Creating message to get scan capablities; iface = %d", mIfaceInfo->id);

        int ret = mMsg.create(GOOGLE_OUI, GSCAN_SUBCMD_GET_CAPABILITIES);
        if (ret < 0) {
            return ret;
        }

        return ret;
    }

protected:
    virtual int handleResponse(WifiEvent& reply) {

        ALOGV("In GetCapabilities::handleResponse");

        if (reply.get_cmd() != NL80211_CMD_VENDOR) {
            ALOGD("Ignoring reply with cmd = %d", reply.get_cmd());
            return NL_SKIP;
        }

        int id = reply.get_vendor_id();
        int subcmd = reply.get_vendor_subcmd();

        void *data = reply.get_vendor_data();
        int len = reply.get_vendor_data_len();

        ALOGV("Id = %0x, subcmd = %d, len = %d, expected len = %d", id, subcmd, len,
                    sizeof(*mCapabilities));

        memcpy(mCapabilities, data, min(len, (int) sizeof(*mCapabilities)));

        return NL_OK;
    }
};


wifi_error wifi_get_gscan_capabilities(wifi_interface_handle handle,
        wifi_gscan_capabilities *capabilities)
{
    GetCapabilitiesCommand command(handle, capabilities);
    return (wifi_error) command.requestResponse();
}

class GetChannelListCommand : public WifiCommand
{
    wifi_channel *channels;
    int max_channels;
    int *num_channels;
    int band;
public:
    GetChannelListCommand(wifi_interface_handle iface, wifi_channel *channel_buf, int *ch_num,
        int num_max_ch, int band)
        : WifiCommand("GetChannelListCommand", iface, 0), channels(channel_buf),
            max_channels(num_max_ch), num_channels(ch_num), band(band)
    {
        memset(channels, 0, sizeof(wifi_channel) * max_channels);
    }
    virtual int create() {
        ALOGV("Creating message to get channel list; iface = %d", mIfaceInfo->id);

        int ret = mMsg.create(GOOGLE_OUI, GSCAN_SUBCMD_GET_CHANNEL_LIST);
        if (ret < 0) {
            return ret;
        }

        nlattr *data = mMsg.attr_start(NL80211_ATTR_VENDOR_DATA);
        ret = mMsg.put_u32(GSCAN_ATTRIBUTE_BAND, band);
        if (ret < 0) {
            return ret;
        }

        mMsg.attr_end(data);

        return ret;
    }

protected:
    virtual int handleResponse(WifiEvent& reply) {

        ALOGV("In GetChannelList::handleResponse");

        if (reply.get_cmd() != NL80211_CMD_VENDOR) {
            ALOGD("Ignoring reply with cmd = %d", reply.get_cmd());
            return NL_SKIP;
        }

        int id = reply.get_vendor_id();
        int subcmd = reply.get_vendor_subcmd();
        int num_channels_to_copy = 0;

        nlattr *vendor_data = reply.get_attribute(NL80211_ATTR_VENDOR_DATA);
        int len = reply.get_vendor_data_len();

        ALOGV("Id = %0x, subcmd = %d, len = %d", id, subcmd, len);
        if (vendor_data == NULL || len == 0) {
            ALOGE("no vendor data in GetChannelList response; ignoring it");
            return NL_SKIP;
        }

        for (nl_iterator it(vendor_data); it.has_next(); it.next()) {
            if (it.get_type() == GSCAN_ATTRIBUTE_NUM_CHANNELS) {
                num_channels_to_copy = it.get_u32();
                ALOGI("Got channel list with %d channels", num_channels_to_copy);
                if (num_channels_to_copy > max_channels)
                    num_channels_to_copy = max_channels;
                *num_channels = num_channels_to_copy;
            } else if (it.get_type() == GSCAN_ATTRIBUTE_CHANNEL_LIST && num_channels_to_copy) {
                memcpy(channels, it.get_data(), sizeof(int) * num_channels_to_copy);
            } else {
                ALOGW("Ignoring invalid attribute type = %d, size = %d",
                        it.get_type(), it.get_len());
            }
        }

        return NL_OK;
    }
};

wifi_error wifi_get_valid_channels(wifi_interface_handle handle,
        int band, int max_channels, wifi_channel *channels, int *num_channels)
{
    GetChannelListCommand command(handle, channels, num_channels,
                                        max_channels, band);
    return (wifi_error) command.requestResponse();
}
/////////////////////////////////////////////////////////////////////////////

/* helper functions */

static int parseScanResults(wifi_scan_result *results, int num, nlattr *attr)
{
    memset(results, 0, sizeof(wifi_scan_result) * num);

    int i = 0;
    for (nl_iterator it(attr); it.has_next() && i < num; it.next(), i++) {

        int index = it.get_type();
        ALOGI("retrieved scan result %d", index);
        nlattr *sc_data = (nlattr *) it.get_data();
        wifi_scan_result *result = results + i;

        for (nl_iterator it2(sc_data); it2.has_next(); it2.next()) {
            int type = it2.get_type();
            if (type == GSCAN_ATTRIBUTE_SSID) {
                strncpy(result->ssid, (char *) it2.get_data(), it2.get_len());
                result->ssid[it2.get_len()] = 0;
            } else if (type == GSCAN_ATTRIBUTE_BSSID) {
                memcpy(result->bssid, (byte *) it2.get_data(), sizeof(mac_addr));
            } else if (type == GSCAN_ATTRIBUTE_TIMESTAMP) {
                result->ts = it2.get_u64();
            } else if (type == GSCAN_ATTRIBUTE_CHANNEL) {
                result->ts = it2.get_u16();
            } else if (type == GSCAN_ATTRIBUTE_RSSI) {
                result->rssi = it2.get_u8();
            } else if (type == GSCAN_ATTRIBUTE_RTT) {
                result->rtt = it2.get_u64();
            } else if (type == GSCAN_ATTRIBUTE_RTTSD) {
                result->rtt_sd = it2.get_u64();
            }
        }

    }

    if (i >= num) {
        ALOGE("Got too many results; skipping some");
    }

    return i;
}

int createFeatureRequest(WifiRequest& request, int subcmd, int enable) {

    int result = request.create(GOOGLE_OUI, subcmd);
    if (result < 0) {
        return result;
    }

    nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);
    result = request.put_u32(GSCAN_ATTRIBUTE_ENABLE_FEATURE, enable);
    if (result < 0) {
        return result;
    }

    request.attr_end(data);
    return WIFI_SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////
class FullScanResultsCommand : public WifiCommand
{
    int *mParams;
    wifi_scan_result_handler mHandler;
public:
    FullScanResultsCommand(wifi_interface_handle iface, int id, int *params,
                wifi_scan_result_handler handler)
        : WifiCommand("FullScanResultsCommand", iface, id), mParams(params), mHandler(handler)
    { }

    int createRequest(WifiRequest& request, int subcmd, int enable) {
        int result = request.create(GOOGLE_OUI, subcmd);
        if (result < 0) {
            return result;
        }

        nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);
        result = request.put_u32(GSCAN_ENABLE_FULL_SCAN_RESULTS, enable);
        if (result < 0) {
            return result;
        }

        request.attr_end(data);
        return WIFI_SUCCESS;

    }

    int start() {
        ALOGV("Enabling Full scan results");
        WifiRequest request(familyId(), ifaceId());
        int result = createRequest(request, GSCAN_SUBCMD_ENABLE_FULL_SCAN_RESULTS, 1);
        if (result != WIFI_SUCCESS) {
            ALOGE("failed to create request; result = %d", result);
            return result;
        }

        registerVendorHandler(GOOGLE_OUI, GSCAN_EVENT_FULL_SCAN_RESULTS);

        result = requestResponse(request);
        if (result != WIFI_SUCCESS) {
            ALOGE("failed to enable full scan results; result = %d", result);
            unregisterVendorHandler(GOOGLE_OUI, GSCAN_EVENT_FULL_SCAN_RESULTS);
            return result;
        }

        return result;
    }

    virtual int cancel() {
        ALOGV("Disabling Full scan results");

        WifiRequest request(familyId(), ifaceId());
        int result = createRequest(request, GSCAN_SUBCMD_ENABLE_FULL_SCAN_RESULTS, 0);
        if (result != WIFI_SUCCESS) {
            ALOGE("failed to create request; result = %d", result);
        } else {
            result = requestResponse(request);
            if (result != WIFI_SUCCESS) {
                ALOGE("failed to disable full scan results;result = %d", result);
            }
        }

        unregisterVendorHandler(GOOGLE_OUI, GSCAN_EVENT_FULL_SCAN_RESULTS);
        return WIFI_SUCCESS;
    }

    virtual int handleResponse(WifiEvent& reply) {
         ALOGD("Request complete!");
        /* Nothing to do on response! */
        return NL_SKIP;
    }

    virtual int handleEvent(WifiEvent& event) {
        ALOGV("Full scan results:  Got an event");
        //event.log();

        nlattr *vendor_data = event.get_attribute(NL80211_ATTR_VENDOR_DATA);
        unsigned int len = event.get_vendor_data_len();

        if (vendor_data == NULL || len < sizeof(wifi_scan_result)) {
            ALOGI("Full scan results: No scan results found");
            return NL_SKIP;
        }

        wifi_scan_result *result = (wifi_scan_result *)event.get_vendor_data();

        if (*mHandler.on_full_scan_result)
            (*mHandler.on_full_scan_result)(id(), result);

        ALOGV("Full scan result: %-32s %02x:%02x:%02x:%02x:%02x:%02x %d %d %lld %lld %lld\n",
            result->ssid, result->bssid[0], result->bssid[1], result->bssid[2], result->bssid[3],
            result->bssid[4], result->bssid[5], result->rssi, result->channel, result->ts,
            result->rtt, result->rtt_sd);

        return NL_SKIP;
    }

};
/////////////////////////////////////////////////////////////////////////////

class ScanCommand : public WifiCommand
{
    wifi_scan_cmd_params *mParams;
    wifi_scan_result_handler mHandler;
    static unsigned mGlobalFullScanBuckets;
    bool mLocalFullScanBuckets;
public:
    ScanCommand(wifi_interface_handle iface, int id, wifi_scan_cmd_params *params,
                wifi_scan_result_handler handler)
        : WifiCommand("ScanCommand", iface, id), mParams(params), mHandler(handler),
          mLocalFullScanBuckets(0)
    { }

    int createSetupRequest(WifiRequest& request) {
        int result = request.create(GOOGLE_OUI, GSCAN_SUBCMD_SET_CONFIG);
        if (result < 0) {
            return result;
        }

        nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);
        result = request.put_u32(GSCAN_ATTRIBUTE_BASE_PERIOD, mParams->base_period);
        if (result < 0) {
            return result;
        }

        result = request.put_u32(GSCAN_ATTRIBUTE_NUM_BUCKETS, mParams->num_buckets);
        if (result < 0) {
            return result;
        }

        for (int i = 0; i < mParams->num_buckets; i++) {
            nlattr * bucket = request.attr_start(i);    // next bucket
            result = request.put_u32(GSCAN_ATTRIBUTE_BUCKET_ID, mParams->buckets[i].bucket);
            if (result < 0) {
                return result;
            }
            result = request.put_u32(GSCAN_ATTRIBUTE_BUCKET_PERIOD, mParams->buckets[i].period);
            if (result < 0) {
                return result;
            }
            result = request.put_u32(GSCAN_ATTRIBUTE_BUCKETS_BAND,
                    mParams->buckets[i].band);
            if (result < 0) {
                return result;
            }
            result = request.put_u32(GSCAN_ATTRIBUTE_BUCKET_STEP_COUNT,
                    mParams->buckets[i].step_count);
            if (result < 0) {
                return result;
            }
            result = request.put_u32(GSCAN_ATTRIBUTE_BUCKET_MAX_PERIOD,
                    mParams->buckets[i].max_period);
            if (result < 0) {
                return result;
            }
            result = request.put_u32(GSCAN_ATTRIBUTE_REPORT_EVENTS,
                    mParams->buckets[i].report_events);
            if (result < 0) {
                return result;
            }

            result = request.put_u32(GSCAN_ATTRIBUTE_BUCKET_NUM_CHANNELS,
                    mParams->buckets[i].num_channels);
            if (result < 0) {
                return result;
            }

            if (mParams->buckets[i].num_channels) {
                nlattr *channels = request.attr_start(GSCAN_ATTRIBUTE_BUCKET_CHANNELS);
                ALOGV(" channels: ");
                for (int j = 0; j < mParams->buckets[i].num_channels; j++) {
                    result = request.put_u32(j, mParams->buckets[i].channels[j].channel);
                    ALOGV(" %u", mParams->buckets[i].channels[j].channel);

                    if (result < 0) {
                        return result;
                    }
                }
                request.attr_end(channels);
            }

            request.attr_end(bucket);
        }

        request.attr_end(data);
        return WIFI_SUCCESS;
    }

    int createScanConfigRequest(WifiRequest& request) {
        int result = request.create(GOOGLE_OUI, GSCAN_SUBCMD_SET_SCAN_CONFIG);
        if (result < 0) {
            return result;
        }

        nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);
        result = request.put_u32(GSCAN_ATTRIBUTE_NUM_AP_PER_SCAN, mParams->max_ap_per_scan);
        if (result < 0) {
            return result;
        }

        result = request.put_u32(GSCAN_ATTRIBUTE_REPORT_THRESHOLD,
                mParams->report_threshold_percent);
        if (result < 0) {
            return result;
        }

        int num_scans = mParams->report_threshold_num_scans;

        result = request.put_u32(GSCAN_ATTRIBUTE_NUM_SCANS_TO_CACHE, num_scans);
        if (result < 0) {
            return result;
        }

        request.attr_end(data);
        return WIFI_SUCCESS;
    }

    int createStartRequest(WifiRequest& request) {
        return createFeatureRequest(request, GSCAN_SUBCMD_ENABLE_GSCAN, 1);
    }

    int createStopRequest(WifiRequest& request) {
        return createFeatureRequest(request, GSCAN_SUBCMD_ENABLE_GSCAN, 0);
    }

    int enableFullScanResultsIfRequired() {
        /* temporary workaround till we have full support for per bucket scans */

        int nBuckets = 0;
        for (int i = 0; i < mParams->num_buckets; i++) {
            if (mParams->buckets[i].report_events == 2) {
                nBuckets++;
            }
        }

        ALOGV("enableFullScanResultsIfRequired num %u needed %u global %u",
            mParams->num_buckets, nBuckets, mGlobalFullScanBuckets);

        if (mGlobalFullScanBuckets == 0 && nBuckets != 0) {
            int result = wifi_enable_full_scan_results(0x1000, ifaceHandle(), mHandler);
            if (result != WIFI_SUCCESS) {
                ALOGE("failed to enable full scan results");
                return result;
            } else {
                ALOGV("successfully enabled full scan results");
            }
        } else {
            ALOGV("mGlobalFullScanBuckets = %d, nBuckets = %d", mGlobalFullScanBuckets, nBuckets);
        }

        mLocalFullScanBuckets = nBuckets;
        mGlobalFullScanBuckets += nBuckets;
        return WIFI_SUCCESS;
    }

    int disableFullScanResultsIfRequired() {
        /* temporary workaround till we have full support for per bucket scans */

        if (mLocalFullScanBuckets == 0) {
            return WIFI_SUCCESS;
        }

        mGlobalFullScanBuckets -= mLocalFullScanBuckets;
        if (mGlobalFullScanBuckets == 0) {
            int result = wifi_disable_full_scan_results(0x1000, ifaceHandle());
            if (result != WIFI_SUCCESS) {
                ALOGE("failed to disable full scan results");
            } else {
                ALOGV("successfully disable full scan results");
            }
        }

        return WIFI_SUCCESS;
    }

    int start() {
        ALOGV("GSCAN start");
        WifiRequest request(familyId(), ifaceId());
        int result = createSetupRequest(request);
        if (result != WIFI_SUCCESS) {
            ALOGE("failed to create setup request; result = %d", result);
            return result;
        }

        result = requestResponse(request);
        if (result != WIFI_SUCCESS) {
            ALOGE("failed to configure setup; result = %d", result);
            return result;
        }

        request.destroy();

        result = createScanConfigRequest(request);
        if (result != WIFI_SUCCESS) {
            ALOGE("failed to create scan config request; result = %d", result);
            return result;
        }

        result = requestResponse(request);
        if (result != WIFI_SUCCESS) {
            ALOGE("failed to configure scan; result = %d", result);
            return result;
        }

        ALOGV(" ....starting scan");

        result = createStartRequest(request);
        if (result != WIFI_SUCCESS) {
            ALOGE("failed to create start request; result = %d", result);
            return result;
        }

        registerVendorHandler(GOOGLE_OUI, GSCAN_EVENT_SCAN_RESULTS_AVAILABLE);
        registerVendorHandler(GOOGLE_OUI, GSCAN_EVENT_COMPLETE_SCAN);

        result = requestResponse(request);
        if (result != WIFI_SUCCESS) {
            ALOGE("failed to start scan; result = %d", result);
            unregisterVendorHandler(GOOGLE_OUI, GSCAN_EVENT_COMPLETE_SCAN);
            unregisterVendorHandler(GOOGLE_OUI, GSCAN_EVENT_SCAN_RESULTS_AVAILABLE);
            return result;
        }

        result = enableFullScanResultsIfRequired();
        return result;
    }

    virtual int cancel() {
        ALOGV("Stopping scan");

        WifiRequest request(familyId(), ifaceId());
        int result = createStopRequest(request);
        if (result != WIFI_SUCCESS) {
            ALOGE("failed to create stop request; result = %d", result);
        } else {
            result = requestResponse(request);
            if (result != WIFI_SUCCESS) {
                ALOGE("failed to stop scan; result = %d", result);
            }
        }

        unregisterVendorHandler(GOOGLE_OUI, GSCAN_EVENT_COMPLETE_SCAN);
        unregisterVendorHandler(GOOGLE_OUI, GSCAN_EVENT_SCAN_RESULTS_AVAILABLE);
        disableFullScanResultsIfRequired();

        return WIFI_SUCCESS;
    }

    virtual int handleResponse(WifiEvent& reply) {
        /* Nothing to do on response! */
        return NL_SKIP;
    }

    virtual int handleEvent(WifiEvent& event) {
        ALOGV("Got a scan results event");
        //event.log();

        nlattr *vendor_data = event.get_attribute(NL80211_ATTR_VENDOR_DATA);
        int len = event.get_vendor_data_len();
        int event_id = event.get_vendor_subcmd();

        if (event_id == GSCAN_EVENT_COMPLETE_SCAN) {
            if (vendor_data == NULL || len != 4) {
                ALOGI("Scan complete type not mentioned!");
                return NL_SKIP;
            }
            wifi_scan_event evt_type;

            evt_type = (wifi_scan_event) event.get_u32(NL80211_ATTR_VENDOR_DATA);
            ALOGV("Scan complete: Received event type %d", evt_type);
            if (*mHandler.on_scan_event)
                (*mHandler.on_scan_event)(evt_type, evt_type);
        } else {

            if (vendor_data == NULL || len != 4) {
                ALOGI("No scan results found");
                return NL_SKIP;
            }

            int num = event.get_u32(NL80211_ATTR_VENDOR_DATA);
            ALOGV("Found %d scan results", num);
            if (*mHandler.on_scan_results_available)
                (*mHandler.on_scan_results_available)(id(), num);
        }
        return NL_SKIP;
    }
};

unsigned ScanCommand::mGlobalFullScanBuckets = 0;

wifi_error wifi_start_gscan(
        wifi_request_id id,
        wifi_interface_handle iface,
        wifi_scan_cmd_params params,
        wifi_scan_result_handler handler)
{
    wifi_handle handle = getWifiHandle(iface);

    ALOGV("Starting GScan, halHandle = %p", handle);

    ScanCommand *cmd = new ScanCommand(iface, id, &params, handler);
    wifi_register_cmd(handle, id, cmd);
    wifi_error result = (wifi_error)cmd->start();
    if (result != WIFI_SUCCESS) {
        wifi_unregister_cmd(handle, id);
    }
    return result;
}

wifi_error wifi_stop_gscan(wifi_request_id id, wifi_interface_handle iface)
{
    wifi_handle handle = getWifiHandle(iface);
    ALOGV("Stopping GScan, wifi_request_id = %d, halHandle = %p", id, handle);

    if (id == -1) {
        wifi_scan_result_handler handler;
        wifi_scan_cmd_params dummy_params;
        wifi_handle handle = getWifiHandle(iface);
        memset(&handler, 0, sizeof(handler));

        ScanCommand *cmd = new ScanCommand(iface, id, &dummy_params, handler);
        cmd->cancel();
        cmd->releaseRef();
        return WIFI_SUCCESS;
    }

    return wifi_cancel_cmd(id, iface);
}


wifi_error wifi_enable_full_scan_results(
        wifi_request_id id,
        wifi_interface_handle iface,
        wifi_scan_result_handler handler)
{
    wifi_handle handle = getWifiHandle(iface);
    int params_dummy;

    ALOGV("Enabling full scan results, halHandle = %p", handle);

    FullScanResultsCommand *cmd = new FullScanResultsCommand(iface, id, &params_dummy, handler);
    wifi_register_cmd(handle, id, cmd);

    wifi_error result = (wifi_error)cmd->start();
    if (result != WIFI_SUCCESS) {
        wifi_unregister_cmd(handle, id);
    }
    return result;
}

wifi_error wifi_disable_full_scan_results(wifi_request_id id, wifi_interface_handle iface)
{
    ALOGV("Disabling full scan results");
    wifi_handle handle = getWifiHandle(iface);

    if (id == -1) {
        wifi_scan_result_handler handler;
        wifi_handle handle = getWifiHandle(iface);
        int params_dummy;

        memset(&handler, 0, sizeof(handler));
        FullScanResultsCommand *cmd = new FullScanResultsCommand(iface, 0, &params_dummy, handler);
        cmd->cancel();
        cmd->releaseRef();
        return WIFI_SUCCESS;
    }

    return wifi_cancel_cmd(id, iface);
}


/////////////////////////////////////////////////////////////////////////////

class GetScanResultsCommand : public WifiCommand {
    wifi_cached_scan_results *mScans;
    int mMax;
    int *mNum;
    int mRetrieved;
    byte mFlush;
    int mCompleted;
    static const int MAX_RESULTS = 320;
    wifi_scan_result mScanResults[MAX_RESULTS];
    int mNextScanResult;
public:
    GetScanResultsCommand(wifi_interface_handle iface, byte flush,
            wifi_cached_scan_results *results, int max, int *num)
        : WifiCommand("GetScanResultsCommand", iface, -1), mScans(results), mMax(max), mNum(num),
                mRetrieved(0), mFlush(flush), mCompleted(0), mNextScanResult(0)
    { }

    int createRequest(WifiRequest& request, int num, byte flush) {
        int result = request.create(GOOGLE_OUI, GSCAN_SUBCMD_GET_SCAN_RESULTS);
        if (result < 0) {
            return result;
        }

        nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);
        result = request.put_u32(GSCAN_ATTRIBUTE_NUM_OF_RESULTS, num);
        if (result < 0) {
            return result;
        }

        result = request.put_u8(GSCAN_ATTRIBUTE_FLUSH_RESULTS, flush);
        if (result < 0) {
            return result;
        }

        request.attr_end(data);
        return WIFI_SUCCESS;
    }

    int execute() {
        WifiRequest request(familyId(), ifaceId());
        ALOGV("retrieving %d scan results", mMax);

        for (int i = 0; i < 10 && mRetrieved < mMax; i++) {
            int num_to_retrieve = mMax - mRetrieved;
            // ALOGI("retrieving %d scan results in one shot", num_to_retrieve);
            int result = createRequest(request, num_to_retrieve, mFlush);
            if (result < 0) {
                ALOGE("failed to create request");
                return result;
            }

            int prev_retrieved = mRetrieved;

            result = requestResponse(request);

            if (result != WIFI_SUCCESS) {
                ALOGE("failed to retrieve scan results; result = %d", result);
                return result;
            }

            if (mRetrieved == prev_retrieved || mCompleted) {
                /* no more items left to retrieve */
                break;
            }

            request.destroy();
        }

        ALOGV("GetScanResults read %d results", mRetrieved);
        *mNum = mRetrieved;
        return WIFI_SUCCESS;
    }

    virtual int handleResponse(WifiEvent& reply) {
        ALOGV("In GetScanResultsCommand::handleResponse");

        if (reply.get_cmd() != NL80211_CMD_VENDOR) {
            ALOGD("Ignoring reply with cmd = %d", reply.get_cmd());
            return NL_SKIP;
        }

        int id = reply.get_vendor_id();
        int subcmd = reply.get_vendor_subcmd();

        ALOGV("Id = %0x, subcmd = %d", id, subcmd);

        /*
        if (subcmd != GSCAN_SUBCMD_SCAN_RESULTS) {
            ALOGE("Invalid response to GetScanResultsCommand; ignoring it");
            return NL_SKIP;
        }
        */

        nlattr *vendor_data = reply.get_attribute(NL80211_ATTR_VENDOR_DATA);
        int len = reply.get_vendor_data_len();

        if (vendor_data == NULL || len == 0) {
            ALOGE("no vendor data in GetScanResults response; ignoring it");
            return NL_SKIP;
        }

        for (nl_iterator it(vendor_data); it.has_next(); it.next()) {
            if (it.get_type() == GSCAN_ATTRIBUTE_SCAN_RESULTS_COMPLETE) {
                mCompleted = it.get_u8();
                ALOGV("retrieved mCompleted flag : %d", mCompleted);
            } else if (it.get_type() == GSCAN_ATTRIBUTE_SCAN_RESULTS || it.get_type() == 0) {
                for (nl_iterator it2(it.get()); it2.has_next(); it2.next()) {
                    int scan_id = 0, flags = 0, num = 0;
                    if (it2.get_type() == GSCAN_ATTRIBUTE_SCAN_ID) {
                        scan_id = it2.get_u32();
                        ALOGV("retrieved scan_id : 0x%0x", scan_id);
                    } else if (it2.get_type() == GSCAN_ATTRIBUTE_SCAN_FLAGS) {
                        flags = it2.get_u8();
                        ALOGV("retrieved scan_flags : 0x%0x", flags);
                    } else if (it2.get_type() == GSCAN_ATTRIBUTE_NUM_OF_RESULTS) {
                        num = it2.get_u32();
                        ALOGV("retrieved num_results: %d", num);
                    } else if (it2.get_type() == GSCAN_ATTRIBUTE_SCAN_RESULTS) {
                        num = it2.get_len() / sizeof(wifi_scan_result);
                        num = min(MAX_RESULTS - mNextScanResult, num);
                        num = min((int)MAX_AP_CACHE_PER_SCAN, num);
                        memcpy(mScanResults + mNextScanResult, it2.get_data(),
                                sizeof(wifi_scan_result) * num);
                        ALOGV("Retrieved %d scan results", num);
                        wifi_scan_result *results = (wifi_scan_result *)it2.get_data();
                        for (int i = 0; i < num; i++) {
                            wifi_scan_result *result = results + i;
                            ALOGV("%02d  %-32s  %02x:%02x:%02x:%02x:%02x:%02x  %04d", i,
                                result->ssid, result->bssid[0], result->bssid[1], result->bssid[2],
                                result->bssid[3], result->bssid[4], result->bssid[5],
                                result->rssi);
                        }
                        mScans[mRetrieved].scan_id = scan_id;
                        mScans[mRetrieved].flags = flags;
                        mScans[mRetrieved].num_results = num;
                        memcpy(mScans[mRetrieved].results,
                                &(mScanResults[mNextScanResult]), num * sizeof(wifi_scan_result));
                        mNextScanResult += num;
                        mRetrieved++;
                        if (mRetrieved >= mMax && it.has_next()) {
                            ALOGW("Ignoring attributes after this scan");
                        }
                    } else {
                        ALOGW("Ignoring invalid attribute type = %d, size = %d",
                                it.get_type(), it.get_len());
                    }
                }
            } else {
                ALOGW("Ignoring invalid attribute type = %d, size = %d",
                        it.get_type(), it.get_len());
            }
        }
        ALOGV("GetScanResults read %d results", mRetrieved);
        return NL_OK;
    }
};

wifi_error wifi_get_cached_gscan_results(wifi_interface_handle iface, byte flush,
        int max, wifi_cached_scan_results *results, int *num) {
    ALOGV("Getting cached scan results, iface handle = %p, num = %d", iface, *num);

    GetScanResultsCommand *cmd = new GetScanResultsCommand(iface, flush, results, max, num);
    wifi_error err = (wifi_error) cmd->execute();
    delete cmd;
    return err;
}

/////////////////////////////////////////////////////////////////////////////

class BssidHotlistCommand : public WifiCommand
{
private:
    wifi_bssid_hotlist_params mParams;
    wifi_hotlist_ap_found_handler mHandler;
    static const int MAX_RESULTS = 64;
    wifi_scan_result mResults[MAX_RESULTS];
public:
    BssidHotlistCommand(wifi_interface_handle handle, int id,
            wifi_bssid_hotlist_params params, wifi_hotlist_ap_found_handler handler)
        : WifiCommand("BssidHotlistCommand", handle, id), mParams(params), mHandler(handler)
    { }

    int createSetupRequest(WifiRequest& request) {
        int result = request.create(GOOGLE_OUI, GSCAN_SUBCMD_SET_HOTLIST);
        if (result < 0) {
            return result;
        }

        nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);
        result = request.put_u8(GSCAN_ATTRIBUTE_HOTLIST_FLUSH, 1);
        if (result < 0) {
            return result;
        }

        result = request.put_u32(GSCAN_ATTRIBUTE_LOST_AP_SAMPLE_SIZE, mParams.lost_ap_sample_size);
        if (result < 0) {
            return result;
        }

        struct nlattr * attr = request.attr_start(GSCAN_ATTRIBUTE_HOTLIST_BSSIDS);
        for (int i = 0; i < mParams.num_bssid; i++) {
            nlattr *attr2 = request.attr_start(GSCAN_ATTRIBUTE_HOTLIST_ELEM);
            if (attr2 == NULL) {
                return WIFI_ERROR_OUT_OF_MEMORY;
            }
            result = request.put_addr(GSCAN_ATTRIBUTE_BSSID, mParams.ap[i].bssid);
            if (result < 0) {
                return result;
            }
            result = request.put_u8(GSCAN_ATTRIBUTE_RSSI_HIGH, mParams.ap[i].high);
            if (result < 0) {
                return result;
            }
            result = request.put_u8(GSCAN_ATTRIBUTE_RSSI_LOW, mParams.ap[i].low);
            if (result < 0) {
                return result;
            }
            request.attr_end(attr2);
        }

        request.attr_end(attr);
        request.attr_end(data);
        return result;
    }

    int createTeardownRequest(WifiRequest& request) {
        int result = request.create(GOOGLE_OUI, GSCAN_SUBCMD_SET_HOTLIST);
        if (result < 0) {
            return result;
        }

        nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);
        result = request.put_u8(GSCAN_ATTRIBUTE_HOTLIST_FLUSH, 1);
        if (result < 0) {
            return result;
        }

        struct nlattr * attr = request.attr_start(GSCAN_ATTRIBUTE_HOTLIST_BSSIDS);
        request.attr_end(attr);
        request.attr_end(data);
        return result;
    }

    int start() {
        ALOGI("Executing hotlist setup request, num = %d", mParams.num_bssid);
        WifiRequest request(familyId(), ifaceId());
        int result = createSetupRequest(request);
        if (result < 0) {
            return result;
        }

        result = requestResponse(request);
        if (result < 0) {
            ALOGI("Failed to execute hotlist setup request, result = %d", result);
            unregisterVendorHandler(GOOGLE_OUI, GSCAN_EVENT_HOTLIST_RESULTS_FOUND);
            unregisterVendorHandler(GOOGLE_OUI, GSCAN_EVENT_HOTLIST_RESULTS_LOST);
            return result;
        }

        ALOGI("Successfully set %d APs in the hotlist ", mParams.num_bssid);
        result = createFeatureRequest(request, GSCAN_SUBCMD_ENABLE_GSCAN, 1);
        if (result < 0) {
            return result;
        }

        registerVendorHandler(GOOGLE_OUI, GSCAN_EVENT_HOTLIST_RESULTS_FOUND);
        registerVendorHandler(GOOGLE_OUI, GSCAN_EVENT_HOTLIST_RESULTS_LOST);

        result = requestResponse(request);
        if (result < 0) {
            unregisterVendorHandler(GOOGLE_OUI, GSCAN_EVENT_HOTLIST_RESULTS_FOUND);
            unregisterVendorHandler(GOOGLE_OUI, GSCAN_EVENT_HOTLIST_RESULTS_LOST);
            return result;
        }

        ALOGI("successfully restarted the scan");
        return result;
    }

    virtual int cancel() {
        /* unregister event handler */
        unregisterVendorHandler(GOOGLE_OUI, GSCAN_EVENT_HOTLIST_RESULTS_FOUND);
        unregisterVendorHandler(GOOGLE_OUI, GSCAN_EVENT_HOTLIST_RESULTS_LOST);
        /* create set hotlist message with empty hotlist */
        WifiRequest request(familyId(), ifaceId());
        int result = createTeardownRequest(request);
        if (result < 0) {
            return result;
        }

        result = requestResponse(request);
        if (result < 0) {
            return result;
        }

        ALOGI("Successfully reset APs in current hotlist");
        return result;
    }

    virtual int handleResponse(WifiEvent& reply) {
        /* Nothing to do on response! */
        return NL_SKIP;
    }

    virtual int handleEvent(WifiEvent& event) {
        ALOGI("Hotlist AP event");
        int event_id = event.get_vendor_subcmd();
        // event.log();

        nlattr *vendor_data = event.get_attribute(NL80211_ATTR_VENDOR_DATA);
        int len = event.get_vendor_data_len();

        if (vendor_data == NULL || len == 0) {
            ALOGI("No scan results found");
            return NL_SKIP;
        }

        memset(mResults, 0, sizeof(wifi_scan_result) * MAX_RESULTS);

        int num = len / sizeof(wifi_scan_result);
        num = min(MAX_RESULTS, num);
        memcpy(mResults, event.get_vendor_data(), num * sizeof(wifi_scan_result));

        if (event_id == GSCAN_EVENT_HOTLIST_RESULTS_FOUND) {
            ALOGI("FOUND %d hotlist APs", num);
            if (*mHandler.on_hotlist_ap_found)
                (*mHandler.on_hotlist_ap_found)(id(), num, mResults);
        } else if (event_id == GSCAN_EVENT_HOTLIST_RESULTS_LOST) {
            ALOGI("LOST %d hotlist APs", num);
            if (*mHandler.on_hotlist_ap_lost)
                (*mHandler.on_hotlist_ap_lost)(id(), num, mResults);
        }
        return NL_SKIP;
    }
};

class ePNOCommand : public WifiCommand
{
private:
    wifi_epno_network *ssid_list;
    int num_ssid;
    wifi_epno_handler mHandler;
    static const int MAX_RESULTS = 32;
    wifi_scan_result mResults[MAX_RESULTS];
public:
    ePNOCommand(wifi_interface_handle handle, int id,
            int num_networks, wifi_epno_network *networks, wifi_epno_handler handler)
        : WifiCommand("ePNOCommand", handle, id), mHandler(handler)
    {
        ssid_list = networks;
        num_ssid = num_networks;
    }

    int createSetupRequest(WifiRequest& request) {
        int result = request.create(GOOGLE_OUI, GSCAN_SUBCMD_SET_EPNO_SSID);
        if (result < 0) {
            return result;
        }

        nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);
        result = request.put_u8(GSCAN_ATTRIBUTE_EPNO_FLUSH, 1);
        if (result < 0) {
            return result;
        }

        result = request.put_u8(GSCAN_ATTRIBUTE_EPNO_SSID_NUM, num_ssid);
        if (result < 0) {
            return result;
        }

        struct nlattr * attr = request.attr_start(GSCAN_ATTRIBUTE_EPNO_SSID_LIST);
        for (int i = 0; i < num_ssid; i++) {
            nlattr *attr2 = request.attr_start(i);
            if (attr2 == NULL) {
                return WIFI_ERROR_OUT_OF_MEMORY;
            }
            result = request.put(GSCAN_ATTRIBUTE_EPNO_SSID, ssid_list[i].ssid, 32);
            ALOGI("PNO network: SSID %s rssi_thresh %d flags %d auth %d", ssid_list[i].ssid,
                (signed char)ssid_list[i].rssi_threshold, ssid_list[i].flags,
                ssid_list[i].auth_bit_field);
            if (result < 0) {
                return result;
            }
            result = request.put_u8(GSCAN_ATTRIBUTE_EPNO_SSID_LEN, strlen(ssid_list[i].ssid));
            if (result < 0) {
                return result;
            }
            result = request.put_u8(GSCAN_ATTRIBUTE_EPNO_RSSI, ssid_list[i].rssi_threshold);
            if (result < 0) {
                return result;
            }
            result = request.put_u8(GSCAN_ATTRIBUTE_EPNO_FLAGS, ssid_list[i].flags);
            if (result < 0) {
                return result;
            }
            result = request.put_u8(GSCAN_ATTRIBUTE_EPNO_AUTH, ssid_list[i].auth_bit_field);
            if (result < 0) {
                return result;
            }
            request.attr_end(attr2);
        }

        request.attr_end(attr);
        request.attr_end(data);
        return result;
    }

    int createTeardownRequest(WifiRequest& request) {
        int result = request.create(GOOGLE_OUI, GSCAN_SUBCMD_SET_EPNO_SSID);
        if (result < 0) {
            return result;
        }

        nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);
        result = request.put_u8(GSCAN_ATTRIBUTE_EPNO_FLUSH, 1);
        if (result < 0) {
            return result;
        }
        request.attr_end(data);
        return result;
    }

    int start() {
        ALOGI("Executing ePNO setup request, num = %d", num_ssid);
        WifiRequest request(familyId(), ifaceId());
        int result = createSetupRequest(request);
        if (result < 0) {
            return result;
        }

        result = requestResponse(request);
        if (result < 0) {
            ALOGI("Failed to execute ePNO setup request, result = %d", result);
            unregisterVendorHandler(GOOGLE_OUI, GSCAN_EVENT_EPNO_EVENT);
            return result;
        }

        ALOGI("Successfully set %d SSIDs for ePNO", num_ssid);
        registerVendorHandler(GOOGLE_OUI, GSCAN_EVENT_EPNO_EVENT);
        ALOGI("successfully restarted the scan");
        return result;
    }

    virtual int cancel() {
        /* unregister event handler */
        unregisterVendorHandler(GOOGLE_OUI, GSCAN_EVENT_EPNO_EVENT);
        /* create set hotlist message with empty hotlist */
        WifiRequest request(familyId(), ifaceId());
        int result = createTeardownRequest(request);
        if (result < 0) {
            return result;
        }

        result = requestResponse(request);
        if (result < 0) {
            return result;
        }

        ALOGI("Successfully reset APs in current hotlist");
        return result;
    }

    virtual int handleResponse(WifiEvent& reply) {
        /* Nothing to do on response! */
        return NL_SKIP;
    }

    virtual int handleEvent(WifiEvent& event) {
        ALOGI("ePNO event");
        int event_id = event.get_vendor_subcmd();
        // event.log();

        nlattr *vendor_data = event.get_attribute(NL80211_ATTR_VENDOR_DATA);
        int len = event.get_vendor_data_len();

        if (vendor_data == NULL || len == 0) {
            ALOGI("No scan results found");
            return NL_SKIP;
        }

        memset(mResults, 0, sizeof(wifi_scan_result) * MAX_RESULTS);

        int num = len / sizeof(wifi_pno_result_t);
        int i;
        num = min(MAX_RESULTS, num);
        wifi_pno_result_t *res = (wifi_pno_result_t *) event.get_vendor_data();
        for (i = 0; i < num; i++) {
            if (res[i].flags == PNO_SSID_FOUND) {
                memcpy(mResults[i].ssid, res[i].ssid, res[i].ssid_len);
                memcpy(mResults[i].bssid, res[i].bssid, sizeof(mac_addr));

                mResults[i].ssid[res[i].ssid_len] = '\0';
                mResults[i].channel = res[i].channel;
                mResults[i].rssi = res[i].rssi;
            }
        }
        if (*mHandler.on_network_found)
            (*mHandler.on_network_found)(id(), num, mResults);
        return NL_SKIP;
    }
};

wifi_error wifi_set_bssid_hotlist(wifi_request_id id, wifi_interface_handle iface,
        wifi_bssid_hotlist_params params, wifi_hotlist_ap_found_handler handler)
{
    wifi_handle handle = getWifiHandle(iface);

    BssidHotlistCommand *cmd = new BssidHotlistCommand(iface, id, params, handler);
    wifi_register_cmd(handle, id, cmd);
    wifi_error result = (wifi_error)cmd->start();
    if (result != WIFI_SUCCESS) {
        wifi_unregister_cmd(handle, id);
    }
    return result;
}

wifi_error wifi_reset_bssid_hotlist(wifi_request_id id, wifi_interface_handle iface)
{
    return wifi_cancel_cmd(id, iface);
}


/////////////////////////////////////////////////////////////////////////////

class SignificantWifiChangeCommand : public WifiCommand
{
    typedef struct {
        mac_addr bssid;                     // BSSID
        wifi_channel channel;               // channel frequency in MHz
        int num_rssi;                       // number of rssi samples
        wifi_rssi rssi[8];                   // RSSI history in db
    } wifi_significant_change_result_internal;

private:
    wifi_significant_change_params mParams;
    wifi_significant_change_handler mHandler;
    static const int MAX_RESULTS = 64;
    wifi_significant_change_result_internal mResultsBuffer[MAX_RESULTS];
    wifi_significant_change_result *mResults[MAX_RESULTS];
public:
    SignificantWifiChangeCommand(wifi_interface_handle handle, int id,
            wifi_significant_change_params params, wifi_significant_change_handler handler)
        : WifiCommand("SignificantWifiChangeCommand", handle, id), mParams(params),
            mHandler(handler)
    { }

    int createSetupRequest(WifiRequest& request) {
        int result = request.create(GOOGLE_OUI, GSCAN_SUBCMD_SET_SIGNIFICANT_CHANGE_CONFIG);
        if (result < 0) {
            return result;
        }

        nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);
        result = request.put_u8(GSCAN_ATTRIBUTE_SIGNIFICANT_CHANGE_FLUSH, 1);
        if (result < 0) {
            return result;
        }
        result = request.put_u16(GSCAN_ATTRIBUTE_RSSI_SAMPLE_SIZE, mParams.rssi_sample_size);
        if (result < 0) {
            return result;
        }
        result = request.put_u16(GSCAN_ATTRIBUTE_LOST_AP_SAMPLE_SIZE, mParams.lost_ap_sample_size);
        if (result < 0) {
            return result;
        }
        result = request.put_u16(GSCAN_ATTRIBUTE_MIN_BREACHING, mParams.min_breaching);
        if (result < 0) {
            return result;
        }

        struct nlattr * attr = request.attr_start(GSCAN_ATTRIBUTE_SIGNIFICANT_CHANGE_BSSIDS);

        for (int i = 0; i < mParams.num_bssid; i++) {
            nlattr *attr2 = request.attr_start(i);
            if (attr2 == NULL) {
                return WIFI_ERROR_OUT_OF_MEMORY;
            }
            result = request.put_addr(GSCAN_ATTRIBUTE_BSSID, mParams.ap[i].bssid);
            if (result < 0) {
                return result;
            }
            result = request.put_u8(GSCAN_ATTRIBUTE_RSSI_HIGH, mParams.ap[i].high);
            if (result < 0) {
                return result;
            }
            result = request.put_u8(GSCAN_ATTRIBUTE_RSSI_LOW, mParams.ap[i].low);
            if (result < 0) {
                return result;
            }
            request.attr_end(attr2);
        }

        request.attr_end(attr);
        request.attr_end(data);

        return result;
    }

    int createTeardownRequest(WifiRequest& request) {
        int result = request.create(GOOGLE_OUI, GSCAN_SUBCMD_SET_SIGNIFICANT_CHANGE_CONFIG);
        if (result < 0) {
            return result;
        }

        nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);
        result = request.put_u16(GSCAN_ATTRIBUTE_SIGNIFICANT_CHANGE_FLUSH, 1);
        if (result < 0) {
            return result;
        }

        request.attr_end(data);
        return result;
    }

    int start() {
        ALOGI("Set significant wifi change config");
        WifiRequest request(familyId(), ifaceId());

        int result = createSetupRequest(request);
        if (result < 0) {
            return result;
        }

        result = requestResponse(request);
        if (result < 0) {
            ALOGI("failed to set significant wifi change config %d", result);
            return result;
        }

        ALOGI("successfully set significant wifi change config");

        result = createFeatureRequest(request, GSCAN_SUBCMD_ENABLE_GSCAN, 1);
        if (result < 0) {
            return result;
        }

        registerVendorHandler(GOOGLE_OUI, GSCAN_EVENT_SIGNIFICANT_CHANGE_RESULTS);

        result = requestResponse(request);
        if (result < 0) {
            unregisterVendorHandler(GOOGLE_OUI, GSCAN_EVENT_SIGNIFICANT_CHANGE_RESULTS);
            return result;
        }

        ALOGI("successfully restarted the scan");
        return result;
    }

    virtual int cancel() {
        /* unregister event handler */
        unregisterVendorHandler(GOOGLE_OUI, GSCAN_EVENT_SIGNIFICANT_CHANGE_RESULTS);

        /* create set significant change monitor message with empty hotlist */
        WifiRequest request(familyId(), ifaceId());

        int result = createTeardownRequest(request);
        if (result < 0) {
            return result;
        }

        result = requestResponse(request);
        if (result < 0) {
            return result;
        }

        ALOGI("successfully reset significant wifi change config");
        return result;
    }

    virtual int handleResponse(WifiEvent& reply) {
        /* Nothing to do on response! */
        return NL_SKIP;
    }

    virtual int handleEvent(WifiEvent& event) {
        ALOGV("Got a significant wifi change event");

        nlattr *vendor_data = event.get_attribute(NL80211_ATTR_VENDOR_DATA);
        int len = event.get_vendor_data_len();

        if (vendor_data == NULL || len == 0) {
            ALOGI("No scan results found");
            return NL_SKIP;
        }

        typedef struct {
            uint16_t flags;
            uint16_t channel;
            mac_addr bssid;
            s8 rssi_history[8];
        } ChangeInfo;

        int num = min(len / sizeof(ChangeInfo), MAX_RESULTS);
        ChangeInfo *ci = (ChangeInfo *)event.get_vendor_data();

        for (int i = 0; i < num; i++) {
            memcpy(mResultsBuffer[i].bssid, ci[i].bssid, sizeof(mac_addr));
            mResultsBuffer[i].channel = ci[i].channel;
            mResultsBuffer[i].num_rssi = 8;
            for (int j = 0; j < mResultsBuffer[i].num_rssi; j++)
                mResultsBuffer[i].rssi[j] = (int) ci[i].rssi_history[j];
            mResults[i] = reinterpret_cast<wifi_significant_change_result *>(&(mResultsBuffer[i]));
        }

        ALOGV("Retrieved %d scan results", num);

        if (num != 0) {
            (*mHandler.on_significant_change)(id(), num, mResults);
        } else {
            ALOGW("No significant change reported");
        }

        return NL_SKIP;
    }
};

wifi_error wifi_set_significant_change_handler(wifi_request_id id, wifi_interface_handle iface,
        wifi_significant_change_params params, wifi_significant_change_handler handler)
{
    wifi_handle handle = getWifiHandle(iface);

    SignificantWifiChangeCommand *cmd = new SignificantWifiChangeCommand(
            iface, id, params, handler);
    wifi_register_cmd(handle, id, cmd);
    wifi_error result = (wifi_error)cmd->start();
    if (result != WIFI_SUCCESS) {
        wifi_unregister_cmd(handle, id);
    }
    return result;
}

wifi_error wifi_reset_significant_change_handler(wifi_request_id id, wifi_interface_handle iface)
{
    return wifi_cancel_cmd(id, iface);
}

wifi_error wifi_reset_epno_list(wifi_request_id id, wifi_interface_handle iface)
{
    return wifi_cancel_cmd(id, iface);
}

wifi_error wifi_set_epno_list(wifi_request_id id, wifi_interface_handle iface,
        int num_networks, wifi_epno_network *networks, wifi_epno_handler handler)
{
     wifi_handle handle = getWifiHandle(iface);

     ePNOCommand *cmd = new ePNOCommand(iface, id, num_networks, networks, handler);
     wifi_register_cmd(handle, id, cmd);
     if (num_networks == 0 || networks == NULL) {
         return wifi_reset_epno_list(id, iface);
     }
     wifi_error result = (wifi_error)cmd->start();
     if (result != WIFI_SUCCESS) {
         wifi_unregister_cmd(handle, id);
     }
     return result;
}

class SSIDWhitelistCommand : public WifiCommand
{
private:
    int mNumNetworks;
    wifi_ssid *mSSIDs;
public:
    SSIDWhitelistCommand(wifi_interface_handle handle, int id,
            int num_networks, wifi_ssid *ssids)
        : WifiCommand("SSIDWhitelistCommand", handle, id), mNumNetworks(num_networks), mSSIDs(ssids)
    { }

    int createRequest(WifiRequest& request) {
        int result = request.create(GOOGLE_OUI, WIFI_SUBCMD_SET_SSID_WHITE_LIST);
        if (result < 0) {
            return result;
        }

        nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);
        result = request.put_u32(GSCAN_ATTRIBUTE_NUM_WL_SSID, mNumNetworks);
        if (result < 0) {
            return result;
        }
        if (!mNumNetworks) {
            result = request.put_u32(GSCAN_ATTRIBUTE_WL_SSID_FLUSH, 1);
            if (result < 0) {
                return result;
            }
        }
        for (int i = 0; i < mNumNetworks; i++) {
            nlattr *attr = request.attr_start(GSCAN_ATTRIBUTE_WHITELIST_SSID_ELEM);
            if (attr == NULL) {
                return WIFI_ERROR_OUT_OF_MEMORY;
            }
            result = request.put_u32(GSCAN_ATTRIBUTE_WL_SSID_LEN, strlen(mSSIDs[i].ssid));
            if (result < 0) {
                return result;
            }
            result = request.put(GSCAN_ATTRIBUTE_WHITELIST_SSID, mSSIDs[i].ssid, 32);
            if (result < 0) {
                return result;
            }
            request.attr_end(attr);
        }
        request.attr_end(data);
        return result;
    }

    int start() {
        ALOGI("Executing whitelist ssid request, num = %d", mNumNetworks);
        WifiRequest request(familyId(), ifaceId());
        int result = createRequest(request);
        if (result < 0) {
            return result;
        }

        result = requestResponse(request);
        if (result < 0) {
            ALOGI("Failed to execute whitelist ssid request, result = %d", result);
            return result;
        }

        ALOGI("Successfully whitlisted %d ssids", mNumNetworks);
        if (result < 0) {
            return result;
        }
        return result;
    }


    virtual int handleResponse(WifiEvent& reply) {
        /* Nothing to do on response! */
        return NL_SKIP;
    }

};

wifi_error wifi_set_ssid_white_list(wifi_request_id id, wifi_interface_handle iface,
        int num_networks, wifi_ssid *ssids)
{
    wifi_handle handle = getWifiHandle(iface);

    SSIDWhitelistCommand *cmd = new SSIDWhitelistCommand(iface, id, num_networks, ssids);
    wifi_register_cmd(handle, id, cmd);
    wifi_error result = (wifi_error)cmd->start();
    if (result != WIFI_SUCCESS) {
        wifi_unregister_cmd(handle, id);
    }
    return result;
}


class RoamParamsCommand : public WifiCommand
{
private:
    wifi_roam_params *mParams;
public:
    RoamParamsCommand(wifi_interface_handle handle, int id, wifi_roam_params *params)
        : WifiCommand("RoamParamsCommand", handle, id), mParams(params)
    { }

    int createRequest(WifiRequest& request) {
        int result = request.create(GOOGLE_OUI, WIFI_SUBCMD_SET_ROAM_PARAMS);
        if (result < 0) {
            return result;
        }

    nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);

    result = request.put_u32(GSCAN_ATTRIBUTE_A_BAND_BOOST_THRESHOLD, mParams->A_band_boost_threshold);
    if (result < 0) {
        return result;
    }
    result = request.put_u32(GSCAN_ATTRIBUTE_A_BAND_PENALTY_THRESHOLD, mParams->A_band_penalty_threshold);
    if (result < 0) {
        return result;
    }
    result = request.put_u32(GSCAN_ATTRIBUTE_A_BAND_BOOST_FACTOR, mParams->A_band_boost_factor);
    if (result < 0) {
        return result;
    }
    result = request.put_u32(GSCAN_ATTRIBUTE_A_BAND_PENALTY_FACTOR, mParams->A_band_penalty_factor);
    if (result < 0) {
        return result;
    }
    result = request.put_u32(GSCAN_ATTRIBUTE_A_BAND_MAX_BOOST, mParams->A_band_max_boost);
    if (result < 0) {
        return result;
    }
    result = request.put_u32(GSCAN_ATTRIBUTE_LAZY_ROAM_HYSTERESIS, mParams->lazy_roam_hysteresis);
    if (result < 0) {
        return result;
    }
    result = request.put_u32(GSCAN_ATTRIBUTE_ALERT_ROAM_RSSI_TRIGGER, mParams->alert_roam_rssi_trigger);
    if (result < 0) {
        return result;
    }
        request.attr_end(data);
        return result;
    }

    int start() {
        ALOGV("Executing roam params set request");
        WifiRequest request(familyId(), ifaceId());
        int result = createRequest(request);
        if (result < 0) {
            return result;
        }

        result = requestResponse(request);
        if (result < 0) {
            ALOGE("Failed to execute Roam params set request, result = %d", result);
            return result;
        }

        ALOGI("Successfully set roam params");
        if (result < 0) {
            return result;
        }
        return result;
    }


    virtual int handleResponse(WifiEvent& reply) {
        /* Nothing to do on response! */
        return NL_SKIP;
    }

};

wifi_error wifi_set_gscan_roam_params(wifi_request_id id, wifi_interface_handle iface,
                                        wifi_roam_params * params)
{
    wifi_handle handle = getWifiHandle(iface);

    RoamParamsCommand *cmd = new RoamParamsCommand(iface, id, params);
    wifi_register_cmd(handle, id, cmd);
    wifi_error result = (wifi_error)cmd->start();
    if (result != WIFI_SUCCESS) {
        wifi_unregister_cmd(handle, id);
    }
    return result;
}

class LazyRoamCommand : public WifiCommand
{
private:
    int mEnable;
public:
    LazyRoamCommand(wifi_interface_handle handle, int id, int enable)
        : WifiCommand("LazyRoamCommand", handle, id), mEnable(enable)
    { }

    int createRequest(WifiRequest& request) {
        int result = request.create(GOOGLE_OUI, WIFI_SUBCMD_ENABLE_LAZY_ROAM);
        if (result < 0) {
            return result;
        }

        nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);

        result = request.put_u32(GSCAN_ATTRIBUTE_LAZY_ROAM_ENABLE, mEnable);
        if (result < 0) {
            return result;
        }
        request.attr_end(data);
        return result;
    }

    int start() {
        WifiRequest request(familyId(), ifaceId());
        int result = createRequest(request);
        if (result < 0) {
            return result;
        }

        result = requestResponse(request);
        if (result < 0) {
            ALOGI("Failed to enable lazy roam, result = %d", result);
            return result;
        }

        ALOGI("Successfully enabled lazy roam");
        if (result < 0) {
            return result;
        }
        return result;
    }


    virtual int handleResponse(WifiEvent& reply) {
        /* Nothing to do on response! */
        return NL_SKIP;
    }

};
wifi_error wifi_enable_lazy_roam(wifi_request_id id, wifi_interface_handle iface, int enable)
{
    wifi_handle handle = getWifiHandle(iface);

    LazyRoamCommand *cmd = new LazyRoamCommand(iface, id, enable);
    wifi_register_cmd(handle, id, cmd);
    wifi_error result = (wifi_error)cmd->start();
    if (result != WIFI_SUCCESS) {
        wifi_unregister_cmd(handle, id);
    }
    return result;
}

class BssidBlacklistCommand : public WifiCommand
{
private:
    wifi_bssid_params *mParams;
public:
    BssidBlacklistCommand(wifi_interface_handle handle, int id,
            wifi_bssid_params *params)
        : WifiCommand("BssidBlacklistCommand", handle, id), mParams(params)
    { }
     int createRequest(WifiRequest& request) {
        int result = request.create(GOOGLE_OUI, WIFI_SUBCMD_SET_BSSID_BLACKLIST);
        if (result < 0) {
            return result;
        }

        nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);
        result = request.put_u32(GSCAN_ATTRIBUTE_NUM_BSSID, mParams->num_bssid);
        if (result < 0) {
            return result;
        }
        if (!mParams->num_bssid) {
            result = request.put_u32(GSCAN_ATTRIBUTE_BSSID_BLACKLIST_FLUSH, 1);
            if (result < 0) {
                return result;
            }
        }
        for (int i = 0; i < mParams->num_bssid; i++) {
            result = request.put_addr(GSCAN_ATTRIBUTE_BLACKLIST_BSSID, mParams->bssids[i]);
            if (result < 0) {
                return result;
            }
        }
        request.attr_end(data);
        return result;
    }

    int start() {
        ALOGV("Executing bssid blacklist request, num = %d", mParams->num_bssid);
        WifiRequest request(familyId(), ifaceId());
        int result = createRequest(request);
        if (result < 0) {
            return result;
        }

        result = requestResponse(request);
        if (result < 0) {
            ALOGE("Failed to execute bssid blacklist request, result = %d", result);
            return result;
        }

        ALOGI("Successfully added %d blacklist bssids", mParams->num_bssid);
        if (result < 0) {
            return result;
        }
        return result;
    }


    virtual int handleResponse(WifiEvent& reply) {
        /* Nothing to do on response! */
        return NL_SKIP;
    }
};

wifi_error wifi_set_bssid_blacklist(wifi_request_id id, wifi_interface_handle iface,
        wifi_bssid_params params)
{
    wifi_handle handle = getWifiHandle(iface);

    BssidBlacklistCommand *cmd = new BssidBlacklistCommand(iface, id, &params);
    wifi_error result = (wifi_error)cmd->start();
    //release the reference of command as well
    cmd->releaseRef();
    return result;
}

class BssidPreferenceCommand : public WifiCommand
{
private:
    int mNumBssid;
    wifi_bssid_preference *mPrefs;
public:
    BssidPreferenceCommand(wifi_interface_handle handle, int id,
            int num_bssid, wifi_bssid_preference *prefs)
        : WifiCommand("BssidPreferenceCommand", handle, id), mNumBssid(num_bssid), mPrefs(prefs)
    { }

    int createRequest(WifiRequest& request) {
        int result = request.create(GOOGLE_OUI, WIFI_SUBCMD_SET_BSSID_PREF);
        if (result < 0) {
            return result;
        }

        nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);

        result = request.put_u32(GSCAN_ATTRIBUTE_NUM_BSSID, mNumBssid);
        if (result < 0) {
            return result;
        }
        if (!mNumBssid) {
            result = request.put_u32(GSCAN_ATTRIBUTE_BSSID_PREF_FLUSH, 1);
            if (result < 0) {
                return result;
            }
        }
       struct nlattr * attr = request.attr_start(GSCAN_ATTRIBUTE_BSSID_PREF_LIST);
        if (attr == NULL) {
            return WIFI_ERROR_OUT_OF_MEMORY;
        }
        for (int i = 0; i < mNumBssid; i++) {

            nlattr *attr1 = request.attr_start(i);
            if (attr == NULL) {
                return WIFI_ERROR_OUT_OF_MEMORY;
            }
            result = request.put_addr(GSCAN_ATTRIBUTE_BSSID_PREF, mPrefs[i].bssid);
            if (result < 0) {
                return result;
            }
            result = request.put_u32(GSCAN_ATTRIBUTE_RSSI_MODIFIER, mPrefs[i].rssi_modifier);
            if (result < 0) {
                return result;
            }
            request.attr_end(attr1);
        }
        request.attr_end(attr);
        request.attr_end(data);

        return result;
    }

    int start() {
        ALOGV("Executing bssid prefernce change request, num = %d", mNumBssid);
        WifiRequest request(familyId(), ifaceId());
        int result = createRequest(request);
        if (result < 0) {
            return result;
        }

        result = requestResponse(request);
        if (result < 0) {
            ALOGE("Failed to execute bssid preference change request, result = %d", result);
            return result;
        }

        ALOGI("Successfully changed %d bssid preferences", mNumBssid);
        if (result < 0) {
            return result;
        }
        return result;
    }


    virtual int handleResponse(WifiEvent& reply) {
        /* Nothing to do on response! */
        return NL_SKIP;
    }

};

wifi_error wifi_set_bssid_preference(wifi_request_id id, wifi_interface_handle iface,
                                    int num_bssid, wifi_bssid_preference *prefs)
{
    wifi_handle handle = getWifiHandle(iface);

    BssidPreferenceCommand *cmd = new BssidPreferenceCommand(iface, id, num_bssid, prefs);
    wifi_error result = (wifi_error)cmd->start();
    //release the reference of command as well
    cmd->releaseRef();
    return result;
}

////////////////////////////////////////////////////////////////////////////////

class AnqpoConfigureCommand : public WifiCommand
{
    int num_hs;
    wifi_passpoint_network *mNetworks;
    wifi_passpoint_event_handler mHandler;
public:
    AnqpoConfigureCommand(wifi_request_id id, wifi_interface_handle iface,
        int num, wifi_passpoint_network *hs_list, wifi_passpoint_event_handler handler)
        : WifiCommand("AnqpoConfigureCommand", iface, id), num_hs(num), mNetworks(hs_list),
            mHandler(handler)
    {
    }

    int createRequest(WifiRequest& request, int val) {

        int result = request.create(GOOGLE_OUI, GSCAN_SUBCMD_ANQPO_CONFIG);
        result = request.put_u32(GSCAN_ATTRIBUTE_ANQPO_HS_LIST_SIZE, num_hs);
        if (result < 0) {
            return result;
        }

        nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);

        struct nlattr * attr = request.attr_start(GSCAN_ATTRIBUTE_ANQPO_HS_LIST);
        for (int i = 0; i < num_hs; i++) {
            nlattr *attr2 = request.attr_start(i);
            if (attr2 == NULL) {
                return WIFI_ERROR_OUT_OF_MEMORY;
            }
            result = request.put_u32(GSCAN_ATTRIBUTE_ANQPO_HS_NETWORK_ID, mNetworks[i].id);
            if (result < 0) {
                return result;
            }
            result = request.put(GSCAN_ATTRIBUTE_ANQPO_HS_NAI_REALM, mNetworks[i].realm, 256);
            if (result < 0) {
                return result;
            }
            result = request.put(GSCAN_ATTRIBUTE_ANQPO_HS_ROAM_CONSORTIUM_ID,
                         mNetworks[i].roamingConsortiumIds, 128);
            if (result < 0) {
                return result;
            }
            result = request.put(GSCAN_ATTRIBUTE_ANQPO_HS_PLMN, mNetworks[i].plmn, 3);
            if (result < 0) {
                return result;
            }

            request.attr_end(attr2);
        }

        request.attr_end(attr);
        request.attr_end(data);

        return WIFI_SUCCESS;
    }

    int start() {

        WifiRequest request(familyId(), ifaceId());
        int result = createRequest(request, num_hs);
        if (result != WIFI_SUCCESS) {
            ALOGE("failed to create request; result = %d", result);
            return result;
        }

        registerVendorHandler(GOOGLE_OUI, GSCAN_EVENT_ANQPO_HOTSPOT_MATCH);

        result = requestResponse(request);
        if (result != WIFI_SUCCESS) {
            ALOGE("failed to set ANQPO networks; result = %d", result);
            unregisterVendorHandler(GOOGLE_OUI, GSCAN_EVENT_ANQPO_HOTSPOT_MATCH);
            return result;
        }

        return result;
    }

    virtual int cancel() {

        WifiRequest request(familyId(), ifaceId());
        int result = createRequest(request, 0);
        if (result != WIFI_SUCCESS) {
            ALOGE("failed to create request; result = %d", result);
        } else {
            result = requestResponse(request);
            if (result != WIFI_SUCCESS) {
                ALOGE("failed to reset ANQPO networks;result = %d", result);
            }
        }

        unregisterVendorHandler(GOOGLE_OUI, GSCAN_EVENT_ANQPO_HOTSPOT_MATCH);
        return WIFI_SUCCESS;
    }

    virtual int handleResponse(WifiEvent& reply) {
         ALOGD("Request complete!");
        /* Nothing to do on response! */
        return NL_SKIP;
    }

    virtual int handleEvent(WifiEvent& event) {
        typedef struct {
            u16 channel;        /* channel of GAS protocol */
            u8  dialog_token;   /* GAS dialog token */
            u8  fragment_id;    /* fragment id */
            u16 status_code;    /* status code on GAS completion */
            u16 data_len;       /* length of data to follow */
            u8  data[1];        /* variable length specified by data_len */
        } wifi_anqp_gas_resp;

        ALOGI("ANQPO hotspot matched event!");

        nlattr *vendor_data = event.get_attribute(NL80211_ATTR_VENDOR_DATA);
        unsigned int len = event.get_vendor_data_len();

        if (vendor_data == NULL || len < sizeof(wifi_scan_result)) {
            ALOGI("No scan results found");
            return NL_SKIP;
        }

        wifi_scan_result *result = (wifi_scan_result *)event.get_vendor_data();
        byte *anqp = (byte *)result + offsetof(wifi_scan_result, ie_data) + result->ie_length;
        wifi_anqp_gas_resp *gas = (wifi_anqp_gas_resp *)anqp;
        int anqp_len = offsetof(wifi_anqp_gas_resp, data) + gas->data_len;
        int networkId = *(int *)((byte *)anqp + anqp_len);

        ALOGI("%-32s\t", result->ssid);

        ALOGI("%02x:%02x:%02x:%02x:%02x:%02x ", result->bssid[0], result->bssid[1],
                result->bssid[2], result->bssid[3], result->bssid[4], result->bssid[5]);

        ALOGI("%d\t", result->rssi);
        ALOGI("%d\t", result->channel);
        ALOGI("%lld\t", result->ts);
        ALOGI("%lld\t", result->rtt);
        ALOGI("%lld\n", result->rtt_sd);

        if (*mHandler.on_passpoint_network_found)
            (*mHandler.on_passpoint_network_found)(id(), networkId, result, anqp_len, anqp);

        return NL_SKIP;
    }
};

wifi_error wifi_set_passpoint_list(wifi_request_id id, wifi_interface_handle iface, int num,
        wifi_passpoint_network *networks, wifi_passpoint_event_handler handler)
{
    wifi_handle handle = getWifiHandle(iface);

    AnqpoConfigureCommand *cmd = new AnqpoConfigureCommand(id, iface, num, networks, handler);
    wifi_register_cmd(handle, id, cmd);
    wifi_error result = (wifi_error)cmd->start();
    if (result != WIFI_SUCCESS) {
        wifi_unregister_cmd(handle, id);
    }
    return result;
}

wifi_error wifi_reset_passpoint_list(wifi_request_id id, wifi_interface_handle iface)
{
    return wifi_cancel_cmd(id, iface);
}

