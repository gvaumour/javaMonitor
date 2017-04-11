#include "agentGreg.hh"
#include <stdlib.h>
#include <memory.h>

/* Global agent data structure */
typedef struct {
	/* JVMTI Environment */
	jvmtiEnv      *jvmti;
	jrawMonitorID  lock;
} GlobalAgentData;

static GlobalAgentData *gdata;


/* Heap object callback */
static jvmtiIterationControl JNICALL accumulateHeap(jlong class_tag, jlong size, jlong* tag_ptr, void* user_data)
//(jlong class_tag, jlong size, jlong* tag_ptr, jint length, void* user_data)
{
	jint *total;
	total = (jint *)user_data;
	(*total)+=size;
	return JVMTI_ITERATION_CONTINUE;
}

jlong getCurrentHeapMemory()
{
	jint totalCount=0;
	jint rc;

	/* This returns the JVMTI_ERROR_UNATTACHED_THREAD */
	rc = gdata->jvmti->IterateOverHeap((jvmtiHeapObjectFilter)0 ,&accumulateHeap,&totalCount);
	    //(0, &heapCallbacks, &totalCount);
	if (rc != JVMTI_ERROR_NONE){
		printf("Iterating over heap objects failed, returning error %d\n",rc);
		return rc;
	}
	else {
		printf("Heap memory calculated %d\n",totalCount);
	}
	return totalCount;
}

/* Callback for JVMTI_EVENT_GARBAGE_COLLECTION_START */
static void JNICALL gc_start(jvmtiEnv* jvmti_env)
{
	jint rc;
	printf("Garbage Collection Started...\n");
	rc = gdata->jvmti->RawMonitorEnter(gdata->lock);
	if (rc != JVMTI_ERROR_NONE)
	{
		printf("Failed to get lock for heap memory collection, skipping gc_start collection\n");
		return;
	}

	getCurrentHeapMemory();

	rc = gdata->jvmti->RawMonitorExit(gdata->lock);
	if (rc != JVMTI_ERROR_NONE)
	{
		printf("Failed to release lock for heap memory collection, skipping gc_start collection\n");
		return;
	}

}

/* Callback for JVMTI_EVENT_GARBAGE_COLLECTION_END */
static void JNICALL gc_end(jvmtiEnv* jvmti_env)
{
	printf("Garbage Collection Ended...\n");
}

static void JNICALL vm_init(jvmtiEnv *jvmti, JNIEnv *env, jthread thread)
{
	printf("vm_init called\n");
}

JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *vm, char *options, void *reserved)
{

	static GlobalAgentData data;
	jvmtiEnv* jvmti;
	jint res;
	jint rc;
	jvmtiEventCallbacks callbacks;
	
	(void)memset((void*)&data, 0, sizeof(data));
	gdata = &data;

	res = (*vm).GetEnv((void **)&jvmti, JVMTI_VERSION_1);

	if (res != JNI_OK) {
		printf("ERROR: Unable to access JVMTI Version 1 (0x%x),"
			" is your J2SE a 1.5 or newer version?"
			" JNIEnv's GetEnv() returned %d\n",
		       JVMTI_VERSION_1, res);
		return JNI_ERR;
	}
	gdata->jvmti = jvmti;


	memset(&callbacks, 0x00, sizeof(callbacks));
	callbacks.GarbageCollectionStart = gc_start;
	callbacks.GarbageCollectionFinish = gc_end;
	callbacks.VMInit = vm_init;

	rc = jvmti->SetEventCallbacks(&callbacks, sizeof(callbacks));
	
	if(rc != JVMTI_ERROR_NONE){
		printf("Failed to set JVMTI event handlers, quitting\n");
		return JNI_ERR;
	}

	rc = jvmti->SetEventNotificationMode(JVMTI_ENABLE,JVMTI_EVENT_GARBAGE_COLLECTION_START,NULL);
	rc &= jvmti->SetEventNotificationMode(JVMTI_ENABLE,JVMTI_EVENT_GARBAGE_COLLECTION_FINISH,NULL);
	rc &= jvmti->SetEventNotificationMode(JVMTI_ENABLE,JVMTI_EVENT_VM_INIT,NULL);
	
	if (rc != JVMTI_ERROR_NONE){
		printf("Failed to set JVMTI event notification mode, quitting\n");
		return JNI_ERR;
	}

	return JNI_OK;
}

JNIEXPORT void JNICALL 
Agent_OnUnload(JavaVM *vm)
{
    /* Skip any cleanup, VM is about to die anyway */
}

