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

#ifndef _HAL_APIDEF_INCLUDE_
#define _HAL_APIDEF_INCLUDE_

#include <stdio.h>
#include <alsa/asoundlib.h>

#define AFB_BINDING_VERSION 2
#include <afb/afb-binding.h>
#include <json-c/json.h>
#include <filescan-utils.h>
#include <wrap-json.h>

#include <ctl-config.h>
#include "hal-interface.h"
#include "hal-plugin.h"



// hal-mapalsa.c

typedef enum {
    ACTION_SET,
    ACTION_GET
} ActionSetGetT;


typedef struct {
    halCtlsTagT tag;
    const char *label;
    char* info;
    alsaHalCtlMapT ctl;
    CtlActionT *action;
} alsaHalMapT;

typedef struct {
    const char *name;
    const char *info;
    alsaHalMapT *ctls;
    const char *devid;
    json_object* (*volumeCB)(ActionSetGetT action, const alsaHalCtlMapT *halCtls, json_object *valuesJ);
} alsaHalSndCardT;

PUBLIC void halListCtls(afb_req request);
PUBLIC int halSetCtlByTag(halCtlsTagT tag, int value);
PUBLIC void halSetCtls(afb_req request);
PUBLIC json_object *halGetCtlByTag(halCtlsTagT tag);
PUBLIC void halGetCtls(afb_req request);
PUBLIC void halSubscribe(afb_req request);
PUBLIC int halMapAlsaInit(const char *apiPrefix, alsaHalSndCardT *alsaHalSndCard);

// hal-volramp.c
PUBLIC void volumeRamp(halCtlsTagT halTag, alsaHalCtlMapT *control, void* handle, json_object *valJ);

// hal-mapalsa.c
PUBLIC halCtlsTagT halLabelToTag(const char* label);

// hal-volume.c
PUBLIC json_object *volumeNormalise(ActionSetGetT action, const alsaHalCtlMapT *halCtls, json_object *valuesJ);

// hal-mapconfig.c
PUBLIC int MapConfigLoad(CtlSectionT *section, json_object *mapsJ);


#endif /* _HAL_APIDEF_INCLUDE_ */

