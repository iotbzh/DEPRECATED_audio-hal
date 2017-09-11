/*
 * AlsaLibMapping -- provide low level interface with AUDIO lib (extracted from alsa-json-gateway code)
 * Copyright (C) 2015,2016,2017, Fulup Ar Foll fulup@iot.bzh
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _HAL_ALSAMAP_PLUGIN_
#define _HAL_ALSAMAP_PLUGIN_

#include <alsa/asoundlib.h>
#include <json-c/json.h>
#include <hal-interface.h>
#include <ctl-plugin.h>


typedef struct {
    int min;
    int max;
    int step;
    int mute;
} alsaHalDBscaleT;

typedef struct {
    char* name;
    int numid;
    snd_ctl_elem_type_t type;
    int count;
    int minval;
    int maxval;
    int value;
    int step;
    const char **enums;
    alsaHalDBscaleT *dbscale;
} alsaHalCtlMapT;

typedef struct {
    halCtlsTagT (*LabelToTag)(const char *label);
    int (*SetCtlByTag)(halCtlsTagT tag, int value);
    json_object* (*GetCtlByTag)(halCtlsTagT tag);
} HalpCallbackT;


#endif /* _HAL_ALSAMAP_PLUGIN_ */

