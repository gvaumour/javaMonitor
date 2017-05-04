/*
 * @(#)gctest.c	1.5 04/07/27
 * 
 * Copyright (c) 2004 Sun Microsystems, Inc. All Rights Reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * -Redistribution of source code must retain the above copyright notice, this
 *  list of conditions and the following disclaimer.
 * 
 * -Redistribution in binary form must reproduce the above copyright notice, 
 *  this list of conditions and the following disclaimer in the documentation
 *  and/or other materials provided with the distribution.
 * 
 * Neither the name of Sun Microsystems, Inc. or the names of contributors may 
 * be used to endorse or promote products derived from this software without 
 * specific prior written permission.
 * 
 * This software is provided "AS IS," without a warranty of any kind. ALL 
 * EXPRESS OR IMPLIED CONDITIONS, REPRESENTATIONS AND WARRANTIES, INCLUDING
 * ANY IMPLIED WARRANTY OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
 * OR NON-INFRINGEMENT, ARE HEREBY EXCLUDED. SUN MIDROSYSTEMS, INC. ("SUN")
 * AND ITS LICENSORS SHALL NOT BE LIABLE FOR ANY DAMAGES SUFFERED BY LICENSEE
 * AS A RESULT OF USING, MODIFYING OR DISTRIBUTING THIS SOFTWARE OR ITS
 * DERIVATIVES. IN NO EVENT WILL SUN OR ITS LICENSORS BE LIABLE FOR ANY LOST 
 * REVENUE, PROFIT OR DATA, OR FOR DIRECT, INDIRECT, SPECIAL, CONSEQUENTIAL, 
 * INCIDENTAL OR PUNITIVE DAMAGES, HOWEVER CAUSED AND REGARDLESS OF THE THEORY 
 * OF LIABILITY, ARISING OUT OF THE USE OF OR INABILITY TO USE THIS SOFTWARE, 
 * EVEN IF SUN HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
 * 
 * You acknowledge that this software is not designed, licensed or intended
 * for use in the design, construction, operation or maintenance of any
 * nuclear facility.
 */

/* Example of using JVMTI_EVENT_GARBAGE_COLLECTION_START and
 *                  JVMTI_EVENT_GARBAGE_COLLECTION_FINISH events.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jni.h"
#include "jvmti.h"

enum state
{
	VMSTART,
	VMINIT,
	GCBEGIN,
	GCEND,
	NUM_STATE
};

/* Global static data */
static jvmtiEnv     *jvmti;
static int           gc_count;
static jrawMonitorID lock;

static void check_jvmti_error(jvmtiEnv *jvmti, jvmtiError errnum,
		const char *str) {
	if (errnum != JVMTI_ERROR_NONE) {
		char *errnum_str;

		errnum_str = NULL;
		(void) (*jvmti)->GetErrorName(jvmti , errnum, &errnum_str);

		printf("ERROR: JVMTI: %d(%s): %s\n", errnum,
				(errnum_str == NULL ? "Unknown" : errnum_str),
				(str == NULL ? "" : str));
	}
}


/* Callback for JVMTI_EVENT_VM_INIT */
static void JNICALL 
vm_init(jvmtiEnv *jvmti, JNIEnv *env, jthread thread)
{
	jvmtiError err;
	fprintf(stdout, "!=%d VMINIT\n" , VMINIT);
}

/* Callback for JVMTI_EVENT_GARBAGE_COLLECTION_START */
static void JNICALL 
gc_start(jvmtiEnv* jvmti_env) 
{
    fprintf(stdout, "!=%d GCBEGIN\n", GCBEGIN);
}

/* Callback for JVMTI_EVENT_GARBAGE_COLLECTION_FINISH */
static void JNICALL 
gc_finish(jvmtiEnv* jvmti_env) 
{
    fprintf(stdout, "!=%d GCEND\n", GCEND);
}


static void JNICALL vm_start(jvmtiEnv *jvmti, JNIEnv *env) {

    fprintf(stdout, "!=%d VMSTART \n", VMSTART);

}

/* Agent_OnLoad() is called first, we prepare for a VM_INIT event here. */
JNIEXPORT jint JNICALL
Agent_OnLoad(JavaVM *vm, char *options, void *reserved)
{
	jint                rc;
	jvmtiError          err;
	jvmtiCapabilities   capabilities;
	jvmtiEventCallbacks callbacks;

	/* Get JVMTI environment */
	rc = (*vm)->GetEnv(vm, (void **)&jvmti, JVMTI_VERSION);
	if (rc != JNI_OK) {
		fprintf(stderr, "ERROR: Unable to create jvmtiEnv, GetEnv failed, error=%d\n", rc);
		return -1;
	}

	/* Get/Add JVMTI capabilities */ 
	err = (*jvmti)->GetCapabilities(jvmti, &capabilities);
	if (err != JVMTI_ERROR_NONE) {
		fprintf(stderr, "ERROR: GetCapabilities failed, error=%d\n", err);
	}

	capabilities.can_signal_thread = 1;
	capabilities.can_tag_objects  = 1;
	capabilities.can_get_source_file_name  = 1;
	capabilities.can_get_line_numbers  = 1;
	capabilities.can_generate_vm_object_alloc_events  = 1;
	capabilities.can_generate_garbage_collection_events = 1;
	
	err = (*jvmti)->AddCapabilities(jvmti, &capabilities);
	if (err != JVMTI_ERROR_NONE) {
		fprintf(stderr, "ERROR: AddCapabilities failed, error=%d\n", err);
		return -1;
	}

	/* Set callbacks and enable event notifications */
	memset(&callbacks, 0, sizeof(callbacks));
	callbacks.VMInit                  = &vm_init;
	callbacks.VMStart                  = &vm_start;
	callbacks.GarbageCollectionStart  = &gc_start;
	callbacks.GarbageCollectionFinish = &gc_finish;
	(*jvmti)->SetEventCallbacks(jvmti, &callbacks, sizeof(callbacks));


	(*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE, 
			JVMTI_EVENT_VM_INIT, NULL);
	(*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE, 
			JVMTI_EVENT_GARBAGE_COLLECTION_START, NULL);
	(*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE, 
			JVMTI_EVENT_GARBAGE_COLLECTION_FINISH, NULL);
	(*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE, 
			JVMTI_EVENT_VM_START, NULL);

	/* Create the necessary raw monitor */
	err = (*jvmti)->CreateRawMonitor(jvmti, "lock", &lock);
	if (err != JVMTI_ERROR_NONE) {
		fprintf(stderr, "ERROR: Unable to create raw monitor: %d\n", err);
		return -1;
	}
		
	return 0;
}

/* Agent_OnUnload() is called last */
JNIEXPORT void JNICALL
Agent_OnUnload(JavaVM *vm)
{
}

