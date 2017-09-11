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

#ifndef _HAL_COMMON_INCLUDE_
#define _HAL_COMMON_INCLUDE_

#ifndef PUBLIC
  #define PUBLIC
#endif
#define STATIC static


// Soft control have dynamically allocated numid
#define CTL_AUTO -1

typedef enum {
  QUERY_QUIET   =0,
  QUERY_COMPACT =1,
  QUERY_VERBOSE =2,
  QUERY_FULL    =3,
} halQueryMode;

// Most controls are MIXER but some vendor specific are possible
typedef enum {
    OUTVOL,
    PCMVOL,
    INVOL,
    SWITCH,
    ROUTE,
    CARD,
    ENUM,
} halGroupEnumT;

typedef enum {
    READ,
    WRITE,
    RW,
} halAclEnumT;

typedef enum {
    StartHalCrlTag=0,

    // volume RAMP
    Vol_Ramp_Set_Mode,
    Vol_Ramp_Set_Delay,
    Vol_Ramp_Set_Down,
    Vol_Ramp_Set_Up,
    Vol_Ramp_Set_Slave,

    // HighLevel Audio Control List,
    Master_Playback_Volume,
    Master_Playback_Ramp,
    PCM_Playback_Volume,
    PCM_Playback_Switch,
    Capture_Volume,
    Master_OnOff_Switch,
    Telephony_Playback_Volume,
    Multimedia_Playback_Switch,
    Navigation_Playback_Switch,
    Emergency_Playback_Switch,
    Telephony_Playback_Switch,
            

    // Application Virtual Channels
    Multimedia_Playback_Volume,
    Navigation_Playback_Volume,
    Emergency_Playback_Volume,

   EndHalCrlTag // used to compute number of ctls
} halCtlsTagT;


extern const char *halVolRampModes[];
extern const char *halCtlsLabels[];

PUBLIC halCtlsTagT halGetTagByLabel (const char *label);

#endif /* _HAL_COMMON_INCLUDE_ */

