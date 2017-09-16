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
 * To find out which control your sound card uses
 *  aplay -l  # Check sndcard name name in between []
 *  amixer -D hw:v1340 controls # get supported controls
 *  amixer -Dhw:v1340 cget name=Power-Switch
 *  amixer -Dhw:v1340 cset name=Power-Switch true
 *
 */
#define _GNU_SOURCE
#include <json-c/json_object.h>

#include "hal-apidef.h"
        

STATIC CtlConfigT *ctlConfig=NULL;

STATIC void pingtest(struct afb_req request) {
    static int count=0;
    afb_req_success(request, json_object_new_int(count++), "hal-json-binding");
}

// HAL sound card mapping info
STATIC alsaHalSndCardT *halSndCard = NULL;

// This receive all event this binding subscribe to
STATIC void HalOneEvent(const char *evtname, json_object *eventJ) {
    int numid;
    alsaHalMapT *halCtls = halSndCard->ctls;
    json_object *numidJ, *valuesJ;

    AFB_DEBUG("halServiceEvent evtname=%s [msg=%s]", evtname, json_object_get_string(eventJ));

    json_object_object_get_ex(eventJ, "id", &numidJ);
    numid = json_object_get_int(numidJ);
    if (!numid) {
        AFB_ERROR("halServiceEvent noid: evtname=%s [msg=%s]", evtname, json_object_get_string(eventJ));
        return;
    }
    json_object_object_get_ex(eventJ, "val", &valuesJ);

    // search it corresponding numid in halCtls attach a callback
    if (numid) {
        for (int idx = 0; halCtls[idx].ctl.numid; idx++) {
            if (halCtls[idx].ctl.numid == numid && halCtls[idx].action != NULL) {
                
                //halCtls[idx].cb.callback(halCtls[idx].tag, &halCtls[idx].ctl, halCtls[idx].cb.handle, valuesJ);
                (void)ActionExecOne(halCtls[idx].action, valuesJ);                
            }
        }
    }
}

static HalpCallbackT halCallbacks = {
    .LabelToTag  = halLabelToTag,
    .SetCtlByTag = halSetCtlByTag,
    .GetCtlByTag = halGetCtlByTag,
};

#define HALMAP_IDX 2

// Config Section definition (note: controls section index should match handle retrieval in HalConfigExec)
static CtlSectionT ctlSections[]= {
    [0]={.key="plugins" , .loadCB= PluginConfig, .handle= &halCallbacks},
    [1]={.key="onload"  , .loadCB= OnloadConfig},
    [HALMAP_IDX]={.key="halmap"  , .loadCB= MapConfigLoad},
    [3]={.key=NULL}
};


STATIC int HalConfigLoad () {
    
    // check if config file exist
    const char *dirList= getenv("AUDIOHAL_CONFIG_PATH");
    if (!dirList) dirList=CONTROL_CONFIG_PATH;

    ctlConfig = CtlConfigLoad(dirList, ctlSections);
    if (!ctlConfig) goto OnErrorExit;        

    // retrieve alsaHalMap from config section handle
    halSndCard = calloc (1, sizeof(alsaHalSndCardT));
    halSndCard->name = ctlConfig->label;
    halSndCard->info = ctlConfig->info;
    halSndCard->ctls = ( alsaHalMapT*) ctlConfig->sections[HALMAP_IDX].handle; // Warning [index].handle should match with ctlSections static definition

    int err = halMapAlsaLoad (halSndCard); 
    return err; 
    
OnErrorExit:
   return 1;
}

STATIC int HalConfigExec () {
    // process config sessions
    int err = CtlConfigExec (ctlConfig);

    // Should exec AlsaMap here because of full config access    
    if (!err) err = halMapAlsaExec (ctlConfig->api, halSndCard); 
    
    return err;   
}



// Every HAL export the same API & Interface Mapping from SndCard to AudioLogic is done through alsaHalSndCardT
STATIC afb_verb_v2 halApiVerbs[] = {
    /* VERB'S NAME         FUNCTION TO CALL         SHORT DESCRIPTION */
    { .verb = "ping",    .callback = pingtest, .info = "ping test for API"},
    { .verb = "ctllist", .callback = halListCtls, .info = "List AGL normalised Sound Controls"},
    { .verb = "ctlget", .callback = halGetCtls, .info = "Get one/many sound controls"},
    { .verb = "ctlset", .callback = halSetCtls, .info = "Set one/many sound controls"},
    { .verb = "evtsub", .callback = halSubscribe, .info = "Subscribe to HAL events"},
    { .verb = NULL} /* marker for end of the array */
};


// API prefix should be unique for each snd card
PUBLIC const struct afb_binding_v2 afbBindingV2 = {
    .api     = "audio-hal",
    .preinit = HalConfigLoad,
    .init    = HalConfigExec,
    .verbs   = halApiVerbs,
    .onevent = HalOneEvent,
};
