/*
 * Copyright (C) 2017 Amlogic Corporation.
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>
#include <aml_conf_loader.h>
#include <aml_conf_parser.h>

int main( int argc, char** argv )
{
	unsigned char ver[2];
	struct parser *gParser = NULL;
    unsigned char *index = "audio.pre.gain.for.av";
	gParser = aml_config_load(AML_PARAM_AUDIO_HAL_PARAM);
    if (gParser != NULL) {
        //section dump
        //printf("\n[Dump TV Section]\n\n");
		//aml_config_dump(gParser, "TV");
        //printf("\n[Dump ATV Section]\n\n");
		//aml_config_dump(gParser, "ATV");
        //get string value
        printf("index: %s, string  value: %s\n",
            index,
            aml_config_get_str(gParser, "TV", index, NULL));
        //get int value
        printf("index: %s, integer value: %d\n",
            index,
            aml_config_get_int(gParser, "TV", index, NULL));
        //get int value
        printf("index: %s, integer value: %d\n",
            "audiohal.lfe.gain",
            aml_config_get_int(gParser, "AUDIO_HAL", "audiohal.lfe.gain", NULL));

        printf("\n[Dump AUDIO_HAL Section][0]\n\n");
		aml_config_dump(gParser, "AUDIO_HAL");

#if 0
		aml_config_set_str(gParser, "AUDIO_HAL", "audiohal.lfe.gain",  "15");
		aml_config_set_int(gParser, "AUDIO_HAL", "audiohal.lfe.cutoff", 120);
		aml_config_set_float(gParser, "AUDIO_HAL", "audiohal.lfe.invert", 5.12);
        printf("\n[Dump AUDIO_HAL Section][1]\n\n");
		aml_config_dump(gParser, "AUDIO_HAL");

		aml_config_set_str(gParser, "AUDIO_HAL", "audiohal.lfe.gain",  "10");
		aml_config_set_int(gParser, "AUDIO_HAL", "audiohal.lfe.cutoff", 150);
		aml_config_set_float(gParser, "AUDIO_HAL", "audiohal.lfe.invert", 3.1415926);
        printf("\n[Dump AUDIO_HAL Section][2]\n\n");
		aml_config_dump(gParser, "AUDIO_HAL");

		aml_config_set_str(gParser, "AUDIO_HAL", "audiohal.lfe.gain",   "");
		aml_config_set_str(gParser, "AUDIO_HAL", "audiohal.lfe.cutoff", "");
		aml_config_set_str(gParser, "AUDIO_HAL", "audiohal.lfe.invert", "");
        printf("\n[Dump AUDIO_HAL Section][3]\n\n");
		aml_config_dump(gParser, "AUDIO_HAL");
#endif
        //unload
        aml_config_unload(gParser);
	}

	return 0;
}
