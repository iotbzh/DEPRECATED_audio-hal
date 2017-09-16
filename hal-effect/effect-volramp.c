/*
 * Copyright (C) 2016 "IoT.bzh"
 * Author Fulup Ar Foll <fulup@iot.bzh>
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
 *
 */

#define _GNU_SOURCE  // needed for vasprintf

#define AFB_BINDING_VERSION 2
#include <afb/afb-binding.h>
#include <systemd/sd-event.h>
#include <json-c/json_object.h>

#include "wrap-json.h"
#include "hal-plugin.h"

CTLP_REGISTER("volRamp-effect");

typedef struct {
    const char *label;
    halCtlsTagT slave;
    int delay; // delay between volset in us
    int stepDown; // linear %
    int stepUp; // linear %
    int current; // current volume for slave ctl
    int target; // target volume
    sd_event_source *evtsrc; // event loop timer source
    HalpCallbackT *api; // need to duplicate API has sdtimer only support one handle
} halvolRampT;

typedef struct {
  int count;
  halvolRampT *volRamps;
  HalpCallbackT *api; 
} VolPluginCtxT;


// Call at initialisation time
PUBLIC CTLP_ONLOAD(plugin, api) {
    VolPluginCtxT *pluginCtx= (VolPluginCtxT*)calloc (1, sizeof(VolPluginCtxT));
    pluginCtx->api = (HalpCallbackT*)api;
    
    AFB_NOTICE ("HAL-EFFECT-SAMPLE:Onload label='%s' version='%s' info='%s'", plugin->label, plugin->version, plugin->info);
    return (void*)pluginCtx;
}

STATIC int RampTimerCB(sd_event_source* source, uint64_t timer, void* handle) {
    halvolRampT *volRamp = (halvolRampT*) handle;
    int err;
    uint64_t usec;

    // RampDown
    if (volRamp->current > volRamp->target) {
        volRamp->current = volRamp->current - volRamp->stepDown;
        if (volRamp->current < volRamp->target) volRamp->current = volRamp->target;
    }

    // RampUp
    if (volRamp->current < volRamp->target) {
        volRamp->current = volRamp->current + volRamp->stepUp;
        if (volRamp->current > volRamp->target) volRamp->current = volRamp->target;
    }

    // request current Volume Level
    err = volRamp->api->SetCtlByTag(volRamp->slave, volRamp->current);
    if (err) goto OnErrorExit;

    // we reach target stop volram event
    if (volRamp->current == volRamp->target) sd_event_source_unref(source);
    else {
        // otherwise validate timer for a new run
        sd_event_now(afb_daemon_get_event_loop(), CLOCK_MONOTONIC, &usec);
        sd_event_source_set_enabled(source, SD_EVENT_ONESHOT);
        err = sd_event_source_set_time(source, usec + volRamp->delay);
    }

    return 0;

OnErrorExit:
    AFB_WARNING("RampTimerCB Fail to set HAL ctl tag=%d vol=%d", Master_Playback_Volume, volRamp->current);
    sd_event_source_unref(source); // abandon volRamp
    return -1;
}

STATIC void SetRampTimer(void *handle) {
    halvolRampT *volRamp = (halvolRampT*) handle;
    uint64_t usec;

    // set a timer with ~250us accuracy
    sd_event_now(afb_daemon_get_event_loop(), CLOCK_MONOTONIC, &usec);
    sd_event_add_time(afb_daemon_get_event_loop(), &volRamp->evtsrc, CLOCK_MONOTONIC, usec, 250, RampTimerCB, volRamp);
}

STATIC int volumeDoRamp(halvolRampT *volRamp, json_object *volumeJ) {
    json_object *responseJ;

    // request current Volume Level
    responseJ = volRamp->api->GetCtlByTag(volRamp->slave);
    if (!responseJ) {
        AFB_WARNING("volumeDoRamp Fail to get HAL ctl tag=%d", Master_Playback_Volume);
        goto OnErrorExit;
    }

    // use 1st volume value as target for ramping
    switch (json_object_get_type(volumeJ)) {
        case json_type_array:
            volRamp->target = json_object_get_int(json_object_array_get_idx(volumeJ, 0));
            break;

        case json_type_int:
            volRamp->target = json_object_get_int(volumeJ);
            break;

        default:
            AFB_WARNING("volumeDoRamp Invalid volumeJ=%s", json_object_get_string(volumeJ));
            goto OnErrorExit;
    }

    // use 1st volume value as current for ramping
    switch (json_object_get_type(responseJ)) {
        case json_type_array:
            volRamp->current = json_object_get_int(json_object_array_get_idx(responseJ, 0));
            break;

        case json_type_int:
            volRamp->current = json_object_get_int(responseJ);
            break;

        default:
            AFB_WARNING("volumeDoRamp Invalid reponseJ=%s", json_object_get_string(responseJ));
            goto OnErrorExit;
    }

    SetRampTimer(volRamp);

    return 0;

OnErrorExit:
    return 1;
}



STATIC int CreateOnevolRamp (VolPluginCtxT *ctx, int index, json_object*argsJ) {

   const char* slave;
   HalpCallbackT *api = ctx->api;
   halvolRampT *volRamp = &ctx->volRamps[index];
   
   if (!argsJ) goto OnErrorExit;
    
   int err= wrap_json_unpack (argsJ, "{ss,ss,si,si,si !}", "label",&volRamp->label, "slave",&slave, "delay",&volRamp->delay, "stepup", &volRamp->stepUp, "stepdown",&volRamp->stepDown);
   if (err) {
       AFB_ERROR ("CreateOnevolRamp: parsing error missing label|slave|delay|stepup|stepdown in:\n-- %s", json_object_get_string(argsJ));
       goto OnErrorExit;
   }

   volRamp->api = api;
   volRamp->slave = api->LabelToTag(slave);
   
   return 0;
   
OnErrorExit:
    return 1;
   
}

CTLP_CAPI (CreateRampEffect, source, argsJ, UNUSED_ARG(queryJ)) {
    
    VolPluginCtxT *ctx=(VolPluginCtxT*)source->context;
    int err=0;
    
    if (json_object_get_type(argsJ) == json_type_array) {
        int length = json_object_array_length(argsJ);
        ctx->volRamps= calloc (length, sizeof(halvolRampT));
        ctx->count=length;
        
        for (int idx=0; idx < length; idx++) {
            err += CreateOnevolRamp (ctx, idx, json_object_array_get_idx(argsJ, idx));
        }
    } else {
        ctx->volRamps= calloc (1, sizeof(halvolRampT));  
        ctx->count=1;        
        err = CreateOnevolRamp (ctx, 0, argsJ);
    }
    
    return 0;
}


CTLP_CAPI (SoftVolumeRamp, source, UNUSED_ARG(argsJ), eventJ) {
    
    VolPluginCtxT *ctx=(VolPluginCtxT*)source->context;
    halvolRampT *volRamp = NULL;
    
    // map softvol type on action label
    const char* softctl= source->label;
    
    // search for control by name into list
    for (int idx=0; idx < ctx->count; idx++) {
        if (!strcasecmp (softctl, ctx->volRamps[idx].label)) {
            volRamp =  &ctx->volRamps[idx];
            break;
        }
    }
     
    if (!volRamp) {
        AFB_ERROR ("SoftVolumeRamp: HalMap SoftControl not declare label='%s' args='%s' plugin='%s'", source->label, softctl, CtlPluginMagic.label);
        goto OnErrorExit;
    }

    
    int ret = volumeDoRamp(volRamp, eventJ);
    return ret;
    
OnErrorExit:
    return 1;
}
