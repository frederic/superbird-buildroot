/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
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

#include "wifi_hal.h"
#include "common.h"
#include "cpp_bindings.h"

int read_wifi(char *wifi_type)
{
    FILE *fd = fopen("/data/vendor/wifi/wid_fp", "r");
    if (fd) {
        fgets(wifi_type, 15, fd);
    } else
        return -1;

    fclose(fd);
    return 0;
}
const char *get_wifi_name()
{
    char wifi_type[15];
    read_wifi(wifi_type);
    if (strstr(wifi_type, "bcm6330") != NULL) {
        return "bcm6330";
    } else if(strstr(wifi_type, "bcm6210") != NULL) {
        return "bcm6210";
    } else if(strstr(wifi_type, "bcm6335") != NULL) {
        return "bcm6335";
    } else if(strstr(wifi_type, "bcm6234") != NULL) {
        return "bcm6234";
    } else if(strstr(wifi_type, "bcm4354") != NULL) {
        return "bcm4354";
    } else if(strstr(wifi_type, "bcm62x2") != NULL) {
        return "bcm62x2";
    } else if(strstr(wifi_type, "bcm6255") != NULL) {
        return "bcm6255";
    } else if(strstr(wifi_type, "bcm6212") != NULL) {
        return "bcm6212";
    } else if(strstr(wifi_type, "bcm6356") != NULL) {
        return "bcm6356";
    } else if(strstr(wifi_type, "bcm4359") != NULL) {
        return "bcm4359";
    } else if(strstr(wifi_type, "bcm4358") != NULL) {
        return "bcm4358";
    } else if(strstr(wifi_type, "qca9377") != NULL) {
        return "qca9377";
    } else if(strstr(wifi_type, "qca9379") != NULL) {
        return "qca9379";
    } else if(strstr(wifi_type, "qca6174") != NULL) {
        return "qca6174";
    } else if(strstr(wifi_type, "rtl8723bs") != NULL) {
        return "rtl8723bs";
    } else if(strncmp(wifi_type, "rtl", 3) == 0) {
        return "rtl";
    } else if(strstr(wifi_type, "rtl8189es") != NULL) {
        return "rtl8189es";
    } else if(strstr(wifi_type, "rtl8821cs") != NULL) {
        return "rtl8821cs";
    } else if(strstr(wifi_type, "rtl8822bs") != NULL) {
        return "rtl8822bs";
    } else if(strstr(wifi_type, "rtl8822bu") != NULL) {
        return "rtl8822bu";
    } else if(strstr(wifi_type, "rtl8189ftv") != NULL) {
        return "rtl8189ftv";
    } else if(strstr(wifi_type, "rtl8192es") != NULL) {
        return "rtl8192es";
    } else if(strstr(wifi_type, "rtl8188eu") != NULL) {
        return "rtl8188eu";
    } else if(strstr(wifi_type, "rtl8723bu") != NULL) {
        return "rtl8723bu";
    } else if(strstr(wifi_type, "rtl8723du") != NULL) {
        return "rtl8723du";
    } else if(strstr(wifi_type, "rtl8723ds") != NULL) {
        return "rtl8723ds";
    } else if(strstr(wifi_type, "rtl8821au") != NULL) {
        return "rtl8821au";
    } else if(strstr(wifi_type, "rtl8812au") != NULL) {
        return "rtl8812au";
    } else if(strstr(wifi_type, "rtl8188ftv") != NULL) {
        return "rtl8188ftv";
    } else if(strstr(wifi_type, "rtl8192cu") != NULL) {
        return "rtl8192cu";
    } else if(strstr(wifi_type, "rtl8192du") != NULL) {
        return "rtl8192du";
    } else if(strstr(wifi_type, "rtl8192eu") != NULL) {
        return "rtl8192eu";
    } else if(strstr(wifi_type, "mtk7601") != NULL) {
        return "mtk7601";
    } else if(strstr(wifi_type, "mtk7662") != NULL) {
        return "mtk7662";
    } else if(strstr(wifi_type, "mtk7668") != NULL) {
        return "mtk7668";
    } else if(strstr(wifi_type, "mtk7603") != NULL) {
        return "mtk7603";
    } else if(strstr(wifi_type, "bcm43751") != NULL) {
        return "bcm43751";
    } else if(strstr(wifi_type, "bcm43569") != NULL) {
        return "bcm43569";
    } else if(strstr(wifi_type, "bcm43458") != NULL) {
        return "bcm43458";
    } else if(strstr(wifi_type, "ssv6051") != NULL) {
        return "ssv6051";
    } else if(strstr(wifi_type, "ssv6155") != NULL) {
        return "ssv6155";
    } else {
#ifdef REALTEK_WIFI_SUPPORT
        return "rtl";
#elif  MTK_WIFI_SUPPORT
	return "mtk";
#elif  QCA_WIFI_SUPPORT
        return "qca";
#else
        return "bcm";
#endif
    }
}
interface_info *getIfaceInfo(wifi_interface_handle handle)
{
    return (interface_info *)handle;
}

wifi_handle getWifiHandle(wifi_interface_handle handle)
{
    return getIfaceInfo(handle)->handle;
}

hal_info *getHalInfo(wifi_handle handle)
{
    return (hal_info *)handle;
}

hal_info *getHalInfo(wifi_interface_handle handle)
{
    return getHalInfo(getWifiHandle(handle));
}

wifi_handle getWifiHandle(hal_info *info)
{
    return (wifi_handle)info;
}

wifi_interface_handle getIfaceHandle(interface_info *info)
{
    return (wifi_interface_handle)info;
}

wifi_error wifi_register_handler(wifi_handle handle, int cmd, nl_recvmsg_msg_cb_t func, void *arg)
{
    hal_info *info = (hal_info *)handle;

    /* TODO: check for multiple handlers? */
    pthread_mutex_lock(&info->cb_lock);

    wifi_error result = WIFI_ERROR_OUT_OF_MEMORY;

    if (info->num_event_cb < info->alloc_event_cb) {
        info->event_cb[info->num_event_cb].nl_cmd  = cmd;
        info->event_cb[info->num_event_cb].vendor_id  = 0;
        info->event_cb[info->num_event_cb].vendor_subcmd  = 0;
        info->event_cb[info->num_event_cb].cb_func = func;
        info->event_cb[info->num_event_cb].cb_arg  = arg;
        ALOGV("Successfully added event handler %p:%p for command %d at %d",
                arg, func, cmd, info->num_event_cb);
        info->num_event_cb++;
        result = WIFI_SUCCESS;
    }

    pthread_mutex_unlock(&info->cb_lock);
    return result;
}

wifi_error wifi_register_vendor_handler(wifi_handle handle,
        uint32_t id, int subcmd, nl_recvmsg_msg_cb_t func, void *arg)
{
    hal_info *info = (hal_info *)handle;

    /* TODO: check for multiple handlers? */
    pthread_mutex_lock(&info->cb_lock);

    wifi_error result = WIFI_ERROR_OUT_OF_MEMORY;

    if (info->num_event_cb < info->alloc_event_cb) {
        info->event_cb[info->num_event_cb].nl_cmd  = NL80211_CMD_VENDOR;
        info->event_cb[info->num_event_cb].vendor_id  = id;
        info->event_cb[info->num_event_cb].vendor_subcmd  = subcmd;
        info->event_cb[info->num_event_cb].cb_func = func;
        info->event_cb[info->num_event_cb].cb_arg  = arg;
        ALOGV("Added event handler %p:%p for vendor 0x%0x and subcmd 0x%0x at %d",
                arg, func, id, subcmd, info->num_event_cb);
        info->num_event_cb++;
        result = WIFI_SUCCESS;
    }

    pthread_mutex_unlock(&info->cb_lock);
    return result;
}

void wifi_unregister_handler(wifi_handle handle, int cmd)
{
    hal_info *info = (hal_info *)handle;

    if (cmd == NL80211_CMD_VENDOR) {
        ALOGE("Must use wifi_unregister_vendor_handler to remove vendor handlers");
        return;
    }

    pthread_mutex_lock(&info->cb_lock);

    for (int i = 0; i < info->num_event_cb; i++) {
        if (info->event_cb[i].nl_cmd == cmd) {
            ALOGV("Successfully removed event handler %p:%p for cmd = 0x%0x from %d",
                    info->event_cb[i].cb_arg, info->event_cb[i].cb_func, cmd, i);

            memmove(&info->event_cb[i], &info->event_cb[i+1],
                (info->num_event_cb - i - 1) * sizeof(cb_info));
            info->num_event_cb--;
            break;
        }
    }

    pthread_mutex_unlock(&info->cb_lock);
}

void wifi_unregister_vendor_handler(wifi_handle handle, uint32_t id, int subcmd)
{
    hal_info *info = (hal_info *)handle;

    pthread_mutex_lock(&info->cb_lock);

    for (int i = 0; i < info->num_event_cb; i++) {

        if (info->event_cb[i].nl_cmd == NL80211_CMD_VENDOR
                && info->event_cb[i].vendor_id == id
                && info->event_cb[i].vendor_subcmd == subcmd) {
            ALOGV("Successfully removed event handler %p:%p for vendor 0x%0x, subcmd 0x%0x from %d",
                    info->event_cb[i].cb_arg, info->event_cb[i].cb_func, id, subcmd, i);
            memmove(&info->event_cb[i], &info->event_cb[i+1],
                (info->num_event_cb - i - 1) * sizeof(cb_info));
            info->num_event_cb--;
            break;
        }
    }

    pthread_mutex_unlock(&info->cb_lock);
}


wifi_error wifi_register_cmd(wifi_handle handle, int id, WifiCommand *cmd)
{
    hal_info *info = (hal_info *)handle;

    ALOGV("registering command %d", id);

    wifi_error result = WIFI_ERROR_OUT_OF_MEMORY;

    if (info->num_cmd < info->alloc_cmd) {
        info->cmd[info->num_cmd].id   = id;
        info->cmd[info->num_cmd].cmd  = cmd;
        ALOGV("Successfully added command %d: %p at %d", id, cmd, info->num_cmd);
        info->num_cmd++;
        result = WIFI_SUCCESS;
    } else {
        ALOGE("Failed to add command %d: %p at %d, reached max limit %d",
                id, cmd, info->num_cmd, info->alloc_cmd);
    }

    return result;
}

WifiCommand *wifi_unregister_cmd(wifi_handle handle, int id)
{
    hal_info *info = (hal_info *)handle;

    ALOGV("un-registering command %d", id);

    WifiCommand *cmd = NULL;

    for (int i = 0; i < info->num_cmd; i++) {
        if (info->cmd[i].id == id) {
            cmd = info->cmd[i].cmd;
            memmove(&info->cmd[i], &info->cmd[i+1], (info->num_cmd - i - 1) * sizeof(cmd_info));
            info->num_cmd--;
            ALOGV("Successfully removed command %d: %p from %d", id, cmd, i);
            break;
        }
    }

    if (!cmd) {
        ALOGI("Failed to remove command %d: %p", id, cmd);
    }

    return cmd;
}

WifiCommand *wifi_get_cmd(wifi_handle handle, int id)
{
    hal_info *info = (hal_info *)handle;

    WifiCommand *cmd = NULL;

    for (int i = 0; i < info->num_cmd; i++) {
        if (info->cmd[i].id == id) {
            cmd = info->cmd[i].cmd;
            break;
        }
    }

    return cmd;
}

void wifi_unregister_cmd(wifi_handle handle, WifiCommand *cmd)
{
    hal_info *info = (hal_info *)handle;

    for (int i = 0; i < info->num_cmd; i++) {
        if (info->cmd[i].cmd == cmd) {
            int id = info->cmd[i].id;
            memmove(&info->cmd[i], &info->cmd[i+1], (info->num_cmd - i - 1) * sizeof(cmd_info));
            info->num_cmd--;
            ALOGV("Successfully removed command %d: %p from %d", id, cmd, i);
            break;
        }
    }
}

wifi_error wifi_cancel_cmd(wifi_request_id id, wifi_interface_handle iface)
{
    wifi_handle handle = getWifiHandle(iface);

    WifiCommand *cmd = wifi_unregister_cmd(handle, id);
    ALOGV("Cancel WifiCommand = %p", cmd);
    if (cmd) {
        cmd->cancel();
        cmd->releaseRef();
        return WIFI_SUCCESS;
    }

    return WIFI_ERROR_INVALID_ARGS;
}

