#include "agentGreg.hh"
#include <stdlib.h>
#include <memory.h>

/* Global agent data structure */
typedef struct {
	/* JVMTI Environment */
	jvmtiEnv      *jvmti;
	jrawMonitorID  lock;
} GlobalAgentData;

typedef struct DeleteQueue {
	jobject obj;
	DeleteQueue * next;
} DeleteQueue;

static int gc_count;
static GlobalAgentData *gdata;

//Queue of global references that need to be cleaned up
static DeleteQueue * deleteQueue = NULL;
static jrawMonitorID deleteQueueLock;


static void check_jvmti_error(jvmtiEnv *jvmti, jvmtiError errnum,
		const char *str) {
	if (errnum != JVMTI_ERROR_NONE) {
		char *errnum_str;

		errnum_str = NULL;
		(void) jvmti->GetErrorName(errnum, &errnum_str);

		printf("ERROR: JVMTI: %d(%s): %s\n", errnum,
				(errnum_str == NULL ? "Unknown" : errnum_str),
				(str == NULL ? "" : str));
	}
}


/* Heap object callback */
static jvmtiIterationControl JNICALL accumulateHeap(jlong class_tag, jlong size, jlong* tag_ptr, void* user_data)
//(jlong class_tag, jlong size, jlong* tag_ptr, jint length, void* user_data)
{
	jint *total;
	total = (jint *)user_data;
	(*total)++;
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
		printf("Total objects in Heap memory %d\n",totalCount);
	}
	return totalCount;
}

/* Callback for JVMTI_EVENT_GARBAGE_COLLECTION_START */
static void JNICALL gc_start(jvmtiEnv* jvmti_env)
{
	jvmtiError err;
	err = gdata->jvmti->RawMonitorEnter(gdata->lock);
	check_jvmti_error(gdata->jvmti, err, "raw monitor enter");
	gc_count++;
	err = gdata->jvmti->RawMonitorNotify(gdata->lock);
	check_jvmti_error(gdata->jvmti, err, "raw monitor notify");
	err = gdata->jvmti->RawMonitorExit(gdata->lock);
	check_jvmti_error(gdata->jvmti, err, "raw monitor exit");
}


static void JNICALL
gcWorker(jvmtiEnv* jvmti, JNIEnv* jni, void *p)
{
	jvmtiError err;
	for (;;) {
		err = jvmti->RawMonitorEnter(gdata->lock);
		check_jvmti_error(jvmti, err, "raw monitor enter");
		while (gc_count == 0) {
			err = jvmti->RawMonitorWait(gdata->lock, 0);
			if (err != JVMTI_ERROR_NONE) {
				printf("There is an error - Greg\n");
				err = jvmti->RawMonitorExit(gdata->lock);
				check_jvmti_error(jvmti, err, "raw monitor wait");
				return;
			}
		}
		gc_count = 0;

		err = jvmti->RawMonitorExit(gdata->lock);
		check_jvmti_error(jvmti, err, "raw monitor exit");
		
		getCurrentHeapMemory();

		DeleteQueue * tmp;
		while(deleteQueue)
		{
			err = jvmti->RawMonitorEnter(deleteQueueLock);
			check_jvmti_error(jvmti, err, "raw monitor enter");

			

			tmp = deleteQueue;
			//tmp->obj;
			
			
			deleteQueue = deleteQueue->next;
			err = jvmti->RawMonitorExit(deleteQueueLock);
			check_jvmti_error(jvmti, err, "raw monitor exit");
			jni->DeleteGlobalRef(tmp->obj);

			free(tmp);
		}
		printf("Garbage Collector End detected\n");
		
	}
}

/* Callback for JVMTI_EVENT_GARBAGE_COLLECTION_END */
static void JNICALL gc_end(jvmtiEnv* jvmti_env)
{
	printf("Garbage Collection Ended...\n");
}


static jthread alloc_thread(JNIEnv *env) {
	jclass thrClass;
	jmethodID cid;
	jthread res;

	thrClass = env->FindClass("java/lang/Thread");
	if (thrClass == NULL) {
		printf("Cannot find Thread class\n");
	}
	cid = env->GetMethodID(thrClass, "<init>", "()V");
	if (cid == NULL) {
		printf("Cannot find Thread constructor method\n");
	}
	res = env->NewObject(thrClass, cid);
	if (res == NULL) {
		printf("Cannot create new Thread object\n");
	}
	return res;
}

static void JNICALL vm_init(jvmtiEnv *jvmti, JNIEnv *env, jthread thread)
{
	printf("vm_init called\n");

	jvmtiError err;

	err = jvmti->RunAgentThread(alloc_thread(env), &gcWorker, NULL,
			JVMTI_THREAD_MAX_PRIORITY);
	check_jvmti_error(jvmti, err, "Unable to run agent cleanup thread");

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

	jvmti->CreateRawMonitor("agentGreg", &(gdata->lock));
	jvmti->CreateRawMonitor("agent gc queue", &(deleteQueueLock));
	gdata->jvmti = jvmti;

	jvmtiCapabilities capabilities;
	capabilities.can_tag_objects  = 1;
	capabilities.can_get_source_file_name  = 1;
	capabilities.can_get_line_numbers  = 1;
	capabilities.can_generate_vm_object_alloc_events  = 1;
	capabilities.can_generate_garbage_collection_events = 1;
	jvmti->AddCapabilities(&capabilities);

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

