/*
 * FILE:    sdp.c
 * AUTHORS: Gerard Castillo  <gerard.castillo@i2cat.net>
 *          Martin Pulec <pulec@cesnet.cz>
 *
 * Copyright (c) 2005-2010 Fundació i2CAT, Internet I Innovació Digital a Catalunya
 * Copyright (c) 2018 CESNET, z. s. p. o.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, is permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *
 *      This product includes software developed by CESNET z.s.p.o.
 *
 * 4. Neither the name of the CESNET nor the names of its contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#include "config_unix.h"
#include "config_win32.h"
#endif

#include "debug.h"
#include "utils/sdp.h"

#define SDP_FILE "ug.sdp"

//TODO could be a vector of many streams
//struct sdp *sdp;

struct sdp *new_sdp(enum rtp_standard std, int port){
    struct sdp *sdp;
    sdp = NULL;
    sdp = malloc(sizeof(struct sdp));
    sdp->stream_count = 0;
    sdp->std_rtp = std;
    sdp->port = port;
    switch (std){
        case 0: //H264
            set_version(sdp);
            set_origin(sdp);
            set_session_name(sdp);
            set_connection(sdp);
            set_times(sdp);
            set_stream(sdp);
            return sdp;
            break;
        default://UNKNOWN
            free(sdp);
            return NULL;
    }
}

bool get_sdp(struct sdp *sdp){
    bool rc = 0;

    if(sdp!=NULL){
        FILE *fOut = fopen (SDP_FILE, "w+");
        if (fOut != NULL) {
            if (fprintf (fOut,sdp->version) >= 0) {
                rc = 1;
            }
            if (fprintf (fOut,sdp->origin) >= 0) {
                rc = 1;
            }
            if (fprintf (fOut,sdp->session_name) >= 0) {
                rc = 1;
            }
            if (fprintf (fOut,sdp->connection) >= 0) {
                rc = 1;
            }
            if (fprintf (fOut,sdp->times) >= 0) {
                rc = 1;
            }
            for (int i = 0; i < sdp->stream_count; ++i) {
                if (fprintf (fOut,sdp->stream[i].media_info) >= 0) {
                    rc = 1;
                }
                if (fprintf (fOut,sdp->stream[i].rtpmap) >= 0) {
                    rc = 1;
                }
            }
            fclose (fOut); // or for the paranoid: if (fclose (fOut) == EOF) rc = 0;
        }else rc=0;
    }else rc = 0;

    if(rc){
        printf("\n[SDP] File " SDP_FILE " created.\nPrinted version:\n");
        printf("%s",sdp->version);
        printf("%s",sdp->origin);
        printf("%s",sdp->session_name);
        printf("%s",sdp->connection);
        printf("%s",sdp->times);
        for (int i = 0; i < sdp->stream_count; ++i) {
            printf("%s",sdp->stream[i].media_info);
            printf("%s",sdp->stream[i].rtpmap);
        }
        printf("\n\n");
    }

    return rc;
}

void set_version(struct sdp *sdp){
    sdp->version = malloc(strLength);
    sdp->version = "v=0\n";
}
void get_version(struct sdp *sdp);

void set_origin(struct sdp *sdp){
    sdp->origin = malloc(strLength);
    sdp->origin = "o=- 0 0 IN IP4 127.0.0.1\n";
}
void get_origin(struct sdp *sdp);

void set_session_name(struct sdp *sdp){
    sdp->session_name = malloc(strLength);
    sdp->session_name = "s=Ultragrid streams\n";
}
void get_session_name(struct sdp *sdp);

void set_connection(struct sdp *sdp){
    sdp->connection =malloc(strLength);
    sdp->connection = "c=IN IP4 127.0.0.1\n";
}
void get_connection(struct sdp *sdp);

void set_times(struct sdp *sdp){
    sdp->times = malloc(strLength);
    sdp->times = "t=0 0\n";
}
void get_times(struct sdp *sdp);

void set_stream(struct sdp *sdp){
    if(new_stream(sdp)){
    }
    else{
        printf("[SDP] stream NOT added -> error: maximum stream definition reached\n");
    }

}
void get_stream(struct sdp *sdp, int index);


bool new_stream(struct sdp *sdp){
    //struct stream_info *stream;
    if(sdp->stream_count < MAX_STREAMS){
        sdp->stream_count++;
        set_stream_media_info(sdp, sdp->stream_count - 1);
        set_stream_rtpmap(sdp, sdp->stream_count - 1);

        return true;
    }
    return true;
}

char *set_stream_media_info(struct sdp *sdp, int index){
    debug_msg("[SDP] SETTING MEDIA INFO    \n\n");

    sprintf(sdp->stream[index].media_info,"m=video %d RTP/AVP 96\n",sdp->port);

    return sdp->stream[index].media_info;
}

char *set_stream_rtpmap(struct sdp *sdp, int index){
    debug_msg("[SDP] SETTING RTPMAP INFO    \n\n");
    strcpy(sdp->stream[index].rtpmap, "a=rtpmap:96 H264/90000\n");

    return sdp->stream[index].rtpmap;
}

void clean_sdp(struct sdp *sdp){
    free(sdp);
}

