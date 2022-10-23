/*
 * Copyright (C) 2010 Amlogic Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */



#ifndef _AUDIO_HW_PROFILE_H_
#define _AUDIO_HW_PROFILE_H_

int get_external_card(int type);
int get_aml_card();
int get_spdif_port();
char*  get_hdmi_sink_cap(const char *keys,audio_format_t format);
char*  get_hdmi_arc_cap(unsigned *ad, int maxsize, const char *keys);
char *strdup_hdmi_arc_cap_default(const char *keys, audio_format_t format);
#endif
