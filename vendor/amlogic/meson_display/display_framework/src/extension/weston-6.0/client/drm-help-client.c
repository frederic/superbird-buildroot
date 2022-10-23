/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: 
 */

#include "drm-help-client.h"
#include "ipc/ipc.h"

bool json_format_to_drm_output_mode(json_object* j_mode, drm_output_mode* mode) {
    const char* name = NULL;
    if (mode == NULL) {
        return false;
    }
    name = json_object_get_string(json_object_object_get(j_mode , "name"));
    if (name == NULL) {
        return false;
    }
    strncpy(mode->name, name, DRM_DISPLAY_MODE_LEN);
    mode->refresh = json_object_get_int(json_object_object_get(j_mode, "vrefresh"));
    mode->width = json_object_get_int(json_object_object_get(j_mode, "hdisplay"));
    mode->height = json_object_get_int(json_object_object_get(j_mode, "vdisplay"));
    DEBUG_INFO("\tMode: %s", json_object_to_json_string(j_mode));
    return true;
}

void send_cmd(drm_client_ctx* client, const char* cmd, void* opt, opt_type type) {
    json_object* data = json_object_new_object();
    json_object* value = NULL;
    int ret = 0;
    if (cmd == NULL || data == NULL)
        return;
    ret = json_object_object_add(data, "cmd", json_object_new_string(cmd));
    if (ret < 0) {
        json_object_put(data);
        return;
    }

    if (opt != NULL) {
        switch (type) {
            case OPT_TYPE_NULL:
                value = NULL;
                ret = 0;
                break;
            case OPT_TYPE_DOUBLE:
                value = json_object_new_double(*(double*)opt);
                break;
            case OPT_TYPE_INT:
                value = json_object_new_int(*(int*)opt);
                break;
            case OPT_TYPE_STRING:
                value = json_object_new_string(*(char**)opt);
                break;
                break;
            case OPT_TYPE_JSON_OBJECT:
                value = *(json_object**)opt;
                break;
            case OPT_TYPE_MAX:
                ret = -1;
                break;
        };
        if (!ret && value) {
            ret = json_object_object_add(data, "value", value);
            if (ret < 0) {
                json_object_put(value);
            }
        }
    }

    if (ret != 0) {
        DEBUG_INFO("can't send CMD, may be format issue");
        json_object_put(data);
        return;
    }

    if (client_send_request((client_ctx*)client, data) <= 0)
        DEBUG_INFO("Server disconnected");

    return;
}

void set_properties(drm_client_ctx* client, const char* cmd, const char* name, uint64_t value) {
    json_object* data = json_object_new_object();
    json_object* json_value = json_object_new_object();
    int ret = 0;

    if (cmd == NULL || name == NULL || data == NULL || json_value == NULL)
        return;

    if (0 != json_object_object_add(data, "cmd", json_object_new_string(cmd)))
        goto set_properties_error;

    if (0 != json_object_object_add(json_value, name, json_object_new_int64((int64_t)value)))
        goto set_properties_error;

    if (0 != json_object_object_add(data, "value", json_value))
        goto set_properties_error;

    if (client_send_request((client_ctx*)client, data) <= 0) {
        DEBUG_INFO("Server disconnected");
    }
    return;

set_properties_error:
    json_object_put(json_value);
    json_object_put(data);
    DEBUG_INFO("can't set properties, may be format issue");
}

json_object* send_cmd_sync(drm_client_ctx* client, const char* cmd, void* opt, opt_type type) {
    json_object* data = NULL;
    json_object* reply = NULL;
    json_object* ret = NULL;

    send_cmd(client, cmd, opt, type);
    if (client_send_request_wait_reply((client_ctx*)client, &data) < 0) {
        DEBUG_INFO("Server disconnected");
    } else {
        reply = json_object_object_get(data, "ret");
        if (reply == NULL) {
            DEBUG_INFO("The reply wrong");
            ret = NULL;
        } else {
            DEBUG_INFO("Get reply: %s", json_object_to_json_string(reply));
            json_object_deep_copy(reply, &ret, NULL);
        }
    }
    json_object_put(data);
    return ret;
}

drm_client_ctx* drm_help_client_create(void) {
    return (drm_client_ctx*)client_create("weston_drm_helper");
}
void drm_help_client_destory(drm_client_ctx* client) {
    client_destory((client_ctx*)client);
}

drm_connection_list* dump_modes(json_object* data) {
    drm_connection_list* conn = NULL;
    if (data == NULL) {
        DEBUG_INFO("The reply wrong");
        return NULL;
    }
    drm_connection_list** curr = &conn;
    json_object_object_foreach(data, connector, val) {
        *curr = (drm_connection_list*)calloc(sizeof(drm_connection_list), 1);
        if (*curr == NULL) {
            DEBUG_INFO("Memory low");
            break;
        }
        {//don't remove
            DEBUG_INFO("Connection %s:%s", connector, json_object_to_json_string(val));
            (*curr)->id = json_object_get_int(json_object_object_get(val, "connection"));
            (*curr)->type = json_object_get_int(json_object_object_get(val, "connection_type"));
            (*curr)->encoder_id = json_object_get_int(json_object_object_get(val, "encoder_id"));
            (*curr)->mmWidth = json_object_get_int(json_object_object_get(val, "mmWidth"));
            (*curr)->mmHeight = json_object_get_int(json_object_object_get(val, "mmHeight"));
            (*curr)->count_encoders = json_object_get_int(json_object_object_get(val, "count_encoders"));
            (*curr)->count_modes = json_object_get_int(json_object_object_get(val, "count_modes"));
            json_object* modes;
            drm_output_mode_list** curr_mode = &((*curr)->modes);
            modes = json_object_object_get(val, "modes");
            if (modes == NULL) {
                DEBUG_INFO("has no modes");
                break;
            }
            json_object_object_foreach(modes, key, value) {
                *curr_mode = (drm_output_mode_list*)calloc(sizeof(drm_output_mode_list), 1);
                if (*curr_mode == NULL) {
                    DEBUG_INFO("Memory low");
                    break;
                }
                if (false == json_format_to_drm_output_mode(value, &(*curr_mode)->mode)) {
                    free_modes(*curr_mode);
                    *curr_mode = NULL;
                    break;
                }
                curr_mode = &((*curr_mode)->next);
            }
        }
        curr = &((*curr)->next);
    }

    json_object_put(data);
    return conn;
}

drm_connection_list* drm_help_client_get_connection(drm_client_ctx* client) {
    return dump_modes(send_cmd_sync((client_ctx*)client, "get modes", NULL, OPT_TYPE_NULL));
}

bool drm_help_client_get_display_mode(drm_client_ctx* client, drm_output_mode* mode, int connection_id) {
    json_object* reply;
    reply = send_cmd_sync((client_ctx*)client, "get display mode", NULL, OPT_TYPE_NULL);
    if (reply == NULL) {
        DEBUG_INFO("The reply wrong");
        return false;
    }
    json_format_to_drm_output_mode(reply , mode);
    json_object_put(reply);
    return true;
}

bool drm_help_client_get_ui_rect(drm_client_ctx* client, drm_output_rect* rect, int connection_id) {
    json_object* reply;
    reply = send_cmd_sync((client_ctx*)client, "get ui rect", NULL, OPT_TYPE_NULL);
    if (reply == NULL) {
        DEBUG_INFO("The reply wrong");
        return false;
    }
    rect->x = json_object_get_int(json_object_object_get(reply, "x"));
    rect->y = json_object_get_int(json_object_object_get(reply, "y"));
    rect->w = json_object_get_int(json_object_object_get(reply, "w"));
    rect->h = json_object_get_int(json_object_object_get(reply, "h"));
    json_object_put(reply);
    return true;
}

void drm_help_client_set_ui_rect(drm_client_ctx* client, drm_output_rect* rect, int connection_id) {
    json_object* info;
    int ret = 0;
    info = json_object_new_object();
    ret = json_object_object_add(info, "x", json_object_new_int(rect->x));
    ret |= json_object_object_add(info, "y", json_object_new_int(rect->y));
    ret |= json_object_object_add(info, "w", json_object_new_int(rect->w));
    ret |= json_object_object_add(info, "h", json_object_new_int(rect->h));
    if (ret != 0) {
        DEBUG_INFO("set ui rect error");
        json_object_put(info);
        return;
    }
    send_cmd((client_ctx*)client, "set ui rect", (void*)&info, OPT_TYPE_JSON_OBJECT);
}

void drm_help_client_switch_mode_s(drm_client_ctx* client, const char* mode_s, int connection_id) {
    if (mode_s != NULL)
        send_cmd(client, "set mode", &mode_s, OPT_TYPE_STRING);
}

void drm_help_client_set_connector_properties(drm_client_ctx* client, const char* name, uint64_t value) {
    set_properties(client, "set connector range properties", name, value);
}

void free_modes(drm_output_mode_list* data) {
    if (data->next != NULL) {
        free_modes(data->next);
    }
    free(data);
}

void free_connection_list(drm_connection_list* data) {
    if (data == NULL) {
        return;
    }
    if (data->next != NULL) {
        free_connection_list(data->next);
    }
    if (data->modes) {
        free_modes(data->modes);
    }
    free(data);
}
