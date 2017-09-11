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
 *
 * To find out which map your sound card uses
 *  aplay -l  # Check sndcard name name in between []
 *  amixer -D hw:v1340 maps # get supported maps
 *  amixer -Dhw:v1340 cget name=Power-Switch
 *  amixer -Dhw:v1340 cset name=Power-Switch true
 *
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>

#include "hal-apidef.h"

STATIC int ProcessOnMap (alsaHalMapT *alsaMap, json_object *mapJ) {
    
    json_object *alsaJ=NULL, *actionJ=NULL;
    
    int err= wrap_json_unpack (mapJ, "{s?s,s?i,s?s,s?o,s?o !}", "label",&alsaMap->label, "tag",&alsaMap->tag, "info",&alsaMap->info, "alsa",&alsaJ, "action", &actionJ);
    if (err) {
       AFB_ERROR ("ProcessOnMap: parsing error missing label|[info]|[alsa]|[callback] in:\n--%s", json_object_get_string(mapJ));
       goto OnErrorExit;
    }
    
    if (!alsaMap->tag && !alsaMap->label) {
       AFB_ERROR ("ProcessOnMap: halmap (label|tag) missing from:%s", json_object_get_string(mapJ));
       goto OnErrorExit;        
    }
    
    if (alsaJ) {
        alsaHalCtlMapT ctl=alsaMap->ctl;
        int err= wrap_json_unpack (alsaJ, "{s?s,s?i,s?i,s?i,s?i,s?i,s?i !}", "name",&ctl.name, "numid",&ctl.numid, "count",&ctl.count
            , "minval",&ctl.minval, "maxval",&ctl.maxval, "value",&ctl.value, "step",&ctl.step);
        if (err) {
           AFB_ERROR ("ProcessOnMap: parsing json alsa error missing name|numid|[count]|[value]|[step]|[minval]|[maxval]|[minval] in:\n-- %s", json_object_get_string(alsaJ));
           goto OnErrorExit;
        }        
    }

    if (actionJ) {
        CtlActionT *action = calloc(1, sizeof(CtlActionT));
        int err= ActionLoadOne(action, actionJ);
        if (err)  goto OnErrorExit;   
        alsaMap->action =action;
    }
    
    return 0;
    
OnErrorExit:
    return 1;
}

STATIC alsaHalMapT *MapsLoad(json_object * mapsJ) {
    alsaHalMapT *ctlMaps;
    int err=0;
    
    if (json_object_get_type(mapsJ) == json_type_array) {
        int length = json_object_array_length(mapsJ);
        ctlMaps = calloc (length+1, sizeof(alsaHalMapT));
        
        for (int idx=0; idx < length; idx++) {
            err += ProcessOnMap (&ctlMaps[idx], json_object_array_get_idx(mapsJ, idx));
        }
        
    } else {
        ctlMaps = calloc (2, sizeof(alsaHalMapT));
        err += ProcessOnMap (&ctlMaps[0], mapsJ);
    }
    
    if (err) return NULL;
    return ctlMaps;
}


// onload section receive one action or an array of maps
PUBLIC int MapConfigLoad(CtlSectionT *section, json_object *mapsJ) {
    alsaHalMapT *ctlMaps;
     
    // Load time parse maps in config file
    if (mapsJ != NULL) {
        ctlMaps= MapsLoad(mapsJ);
        section->handle=ctlMaps;
        
        if (!ctlMaps) {
            AFB_ERROR ("OnloadLoad config fail processing onload maps");
            goto OnErrorExit;
        }
        
    } else {
        // Exec time process onload action now
        ctlMaps=(alsaHalMapT*)section->handle;
        if (!ctlMaps) {
            AFB_ERROR ("OnloadLoad Cannot Exec Non Existing Onload Action");
            goto OnErrorExit;
        }

//        for (int idx=0; maps[idx].source.label != NULL; idx ++) {
//            ActionDispatchOne(&maps[idx], NULL, NULL_AFBREQ);
//        }              
    }

    return 0;

OnErrorExit:
    return 1;

}
