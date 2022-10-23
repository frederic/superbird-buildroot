/*
 * Copyright (C) 2018 Amlogic Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "aml_audio_callback"

#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <sys/prctl.h>

#include "log.h"
#include "aml_callback_api.h"
#include "audio_hw.h"

//#define CALLBACK_USE_ARRAY


#ifdef CALLBACK_USE_ARRAY
#define MAX_CALLBACK_ARRAY 32
#endif

typedef struct aml_audio_cbslot{
#ifndef CALLBACK_USE_ARRAY
	struct listnode listnode;
#endif
    audio_callback_info_t info;   // save callback info
    audio_callback_func_t callback_func; // save callback func
	audio_callback_data_t data;   // used for callback data
	int                   bTrigger;
	int                   bUsed;

}aml_audio_cbslot_t;


struct aml_audiocb_handle{
#ifdef CALLBACK_USE_ARRAY	
	aml_audio_cbslot_t callback_info[MAX_CALLBACK_ARRAY];
#else
	struct listnode callback_list;
#endif
	int thread_exit;
	pthread_t callback_threadID;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
} ;


static void *audio_callback_threadloop(void *data)
{
	aml_audiocb_handle_t *pcallback_handle = NULL;
	aml_audio_cbslot_t *callback = NULL;
    int ret;
	int i = 0;
	struct listnode *node = NULL;

    ALOGI("++%s", __FUNCTION__);


    prctl(PR_SET_NAME, (unsigned long) "audio_callback_thread");
	pcallback_handle = (aml_audiocb_handle_t *)data;

    while (!pcallback_handle->thread_exit) {
		pthread_mutex_lock(&pcallback_handle->mutex);

		pthread_cond_wait(&pcallback_handle->cond, &pcallback_handle->mutex);
#ifdef CALLBACK_USE_ARRAY		
		for (i = 0; i < MAX_CALLBACK_ARRAY; i++) {
			callback = &pcallback_handle->callback_info[i];
			if(callback->bTrigger) {
				if(callback->callback_func) {
					callback->callback_func(&callback->info, (void*)&callback->data);
				}
				callback->bTrigger = 0;
			}
		}
#else		
		list_for_each(node, &pcallback_handle->callback_list) {
			callback = node_to_item(node, aml_audio_cbslot_t, listnode);
			if(callback->bTrigger) {
				if(callback->callback_func) {
					callback->callback_func(&callback->info, (void*)&callback->data);
				}
				callback->bTrigger = 0;
			}
		}  

#endif
		
		pthread_mutex_unlock(&pcallback_handle->mutex);
	}

    ALOGI("--%s", __FUNCTION__);
    return ((void *)0);
}

void trigger_audio_callback (aml_audiocb_handle_t *pcallback_handle, audio_callback_type_t type, audio_callback_data_t * data) {
	aml_audio_cbslot_t *callback = NULL;
	int i = 0;
	struct listnode *node = NULL;

	if (pcallback_handle == NULL) {
		ALOGD("%s pcallback_handle is NULL\n",__func__);
		return;
	}
	
	pthread_mutex_lock(&pcallback_handle->mutex);
#ifdef CALLBACK_USE_ARRAY		
	for (i = 0; i < MAX_CALLBACK_ARRAY; i++) {
		callback = &pcallback_handle->callback_info[i];
		if(callback->info.type == type && callback->callback_func) {
			if(data != NULL) {
				memcpy(&callback->data, data, sizeof(audio_callback_data_t));
			}
			callback->bTrigger = 1;
			pthread_cond_signal(&pcallback_handle->cond);
			break;
		}	

	}
#else

	list_for_each(node, &pcallback_handle->callback_list) {
		callback = node_to_item(node, aml_audio_cbslot_t, listnode);
		if(callback->info.type == type && callback->callback_func) {
			if(data != NULL) {
				memcpy(&callback->data, data, sizeof(audio_callback_data_t));
			}
			callback->bTrigger = 1;
			pthread_cond_signal(&pcallback_handle->cond);
			break;
		}	

	}  

#endif	
	pthread_mutex_unlock(&pcallback_handle->mutex);	

	return;
}



int init_audio_callback (aml_audiocb_handle_t ** ppcallback_handle) {
	int ret = -1;
    // init an array to save calback
	aml_audiocb_handle_t *pcallback_handle = NULL;

	pcallback_handle = calloc(1, sizeof(aml_audiocb_handle_t));

	if(pcallback_handle == NULL) {

		return -1;
	}
#ifndef CALLBACK_USE_ARRAY		
	list_init(&pcallback_handle->callback_list);
#endif
	pthread_mutex_init(&pcallback_handle->mutex, NULL);
	pthread_cond_init(&pcallback_handle->cond, NULL);

    // create a thread to trigger callback

	ret = pthread_create(& (pcallback_handle->callback_threadID), NULL,
						 &audio_callback_threadloop, pcallback_handle);

	*ppcallback_handle = (void*)pcallback_handle;
	
	return ret;
}


int exit_audio_callback (aml_audiocb_handle_t ** ppcallback_handle) {
	aml_audiocb_handle_t *pcallback_handle = NULL;
	aml_audio_cbslot_t *callback = NULL;
	struct listnode *node = NULL;

	ALOGD("%s enter\n",__func__);
	if (ppcallback_handle == NULL) {
		ALOGD("%s ppcallback_handle is NULL\n",__func__);
		return -1;
	}
	
	pcallback_handle = *ppcallback_handle;

	if (pcallback_handle == NULL) {
		ALOGD("%s pcallback_handle is NULL\n",__func__);
		return -1;
	}

	pcallback_handle->thread_exit = 1;
	pthread_cond_signal(&pcallback_handle->cond);
	pthread_join(pcallback_handle->callback_threadID, NULL);	
	
	pthread_mutex_destroy(&pcallback_handle->mutex);
	pthread_cond_destroy(&pcallback_handle->cond);
#ifndef CALLBACK_USE_ARRAY	
    // release the list resources
    list_for_each(node, &pcallback_handle->callback_list) {
        callback = node_to_item(node, aml_audio_cbslot_t, listnode);
			ALOGD("remove callback type=%d callback=%p\n",callback->info.type,callback);
			list_remove(&callback->listnode);
			free(callback);		
			node = &pcallback_handle->callback_list;
    }  
#endif

	free(pcallback_handle);
	*ppcallback_handle = NULL;
	ALOGD("%s exit\n",__func__);
	return 0;
}


int install_audio_callback(aml_audiocb_handle_t *pcallback_handle,
                                                         audio_callback_info_t * callback_info,
                                                         audio_callback_func_t   callback_func) {
                                                         
	aml_audio_cbslot_t *callback = NULL;
	int i = 0;
	int ret = -1;
	int bInstalled = 0;
	struct listnode *node = NULL;

	if(callback_info == NULL) {
		ALOGD("%s input is NULL\n",__func__);
		return -1;
	}

	if (pcallback_handle == NULL) {
		ALOGD("%s pcallback_handle is NULL\n",__func__, pcallback_handle);
		return -1; 
	}


    // install the callback into callback pool
	pthread_mutex_lock(&pcallback_handle->mutex);

#ifdef CALLBACK_USE_ARRAY	
	for (i = 0; i < MAX_CALLBACK_ARRAY; i++) {
		callback = &pcallback_handle->callback_info[i];
		if(callback->info.type == callback_info->type && callback->callback_func == callback_func) {
			ALOGD("%s callback %d func=%p is insatlled\n",__func__,callback_info->type,callback_func);
			bInstalled = 1;
			break;
		}
	}

    // there is no duplicated callback, install it
	if(bInstalled == 0) {
		for (i = 0; i < MAX_CALLBACK_ARRAY; i++) {
			callback = &pcallback_handle->callback_info[i];			
			if(callback->bUsed == 0) {
				callback->info.type = callback_info->type;
				callback->info.id   = callback_info->id;
				callback->bUsed     = 1;
				callback->callback_func  = callback_func;
				bInstalled = 1;
				ALOGD("%s Install callback %d func=%p success\n", __func__,callback_info->type,callback_func);
				break;
			}
		}
	}
#else
    list_for_each(node, &pcallback_handle->callback_list) {
        callback = node_to_item(node, aml_audio_cbslot_t, listnode);
		if(callback->info.type == callback_info->type && callback->callback_func == callback_func) {
			ALOGD("%s callback %d func=%p is insatlled\n",__func__,callback_info->type,callback_func);
			bInstalled = 1;
			break;
		}
    }    

	// there is no duplicated callback, install it
	if(bInstalled == 0) {
		callback = calloc(1,sizeof(aml_audio_cbslot_t));
		callback->info.type = callback_info->type;
		callback->info.id	= callback_info->id;
		callback->bUsed 	= 1;
		callback->callback_func  = callback_func;
		bInstalled = 1;
		ALOGD("%s Install %p callback %d func=%p success\n", __func__,callback,callback_info->type,callback_func);
		list_add_head(&pcallback_handle->callback_list, &callback->listnode);
	}

#endif	
	pthread_mutex_unlock(&pcallback_handle->mutex);	

	if(bInstalled == 1) {
		ret = 0;
	}	
	
	return ret;
}


int remove_audio_callback(aml_audiocb_handle_t *pcallback_handle, audio_callback_info_t * callback_info) {
                                                    
	aml_audio_cbslot_t *callback = NULL;
	int i = 0;
	int ret = -1;
	int bRemoved = 0;
	struct listnode *node = NULL;

	if(callback_info == NULL) {
		ALOGD("%s input is NULL\n",__func__);
		return -1;
	}
	if (pcallback_handle == NULL) {
		ALOGD("%s pcallback_handle is NULL\n",__func__, pcallback_handle);
		return -1; 
	}
    // remove the callback from callback pool
	pthread_mutex_lock(&pcallback_handle->mutex);

#ifdef CALLBACK_USE_ARRAY	
	for (i = 0; i < MAX_CALLBACK_ARRAY; i++) {
		callback = &pcallback_handle->callback_info[i];
		if(callback->info.type == callback_info->type && callback->info.id == callback_info->id ) {
			callback->bUsed = 0;
			ALOGD("%s callback type=%d id=%d func=%p is removed\n",__func__,callback_info->type,callback_info->id,callback->callback_func);
			bRemoved = 1;
			break;
		}
	}
#else
	list_for_each(node, &pcallback_handle->callback_list) {
		callback = node_to_item(node, aml_audio_cbslot_t, listnode);
		if(callback->info.type == callback_info->type && callback->info.id == callback_info->id ) {
			callback->bUsed = 0;
			ALOGD("%s %p callback type=%d id=%d func=%p is removed\n",__func__,callback,callback_info->type,callback_info->id,callback->callback_func);
			bRemoved = 1;
			list_remove(&callback->listnode);
			free(callback);
			break;
		}

	} 
	
#endif	
	pthread_mutex_unlock(&pcallback_handle->mutex);	
	
	return 0;
}


