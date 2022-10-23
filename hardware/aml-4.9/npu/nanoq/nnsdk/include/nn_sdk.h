/****************************************************************************
*
*    Copyright (c) 2019  by amlogic Corp.  All rights reserved.
*
*    The material in this file is confidential and contains trade secrets
*    of amlogic Corporation. No part of this work may be disclosed,
*    reproduced, copied, transmitted, or used in any way for any purpose,
*    without the express written permission of amlogic Corporation.
*
*****************************************************************************/

#ifndef _AMLNNSDK_H
#define _AMLNNSDK_H
#include "nn_api.h"
#ifdef __cplusplus
extern "C" {
#endif
#define MAX_DETECT_NUM 15
#define SUPPORT_NET_NUM 18

typedef struct __box
{
	float x;
	float y;
	float w;
	float h;
	float score;
	float objectClass;
}detBox;

typedef struct __point
{
	float x;
	float y;
}point_t;

typedef struct __nn_image_out
{
	int height;
	int width;
	int channel;
	unsigned char *data;  //this buffer is returned by aml_module_output_get
}image_out_t;

/*=================================================
output of aml_module_t type:IMAGE_CLASSIFY
score: top 5 class confidence
top:   the class of top 5
=================================================*/
typedef struct __nn_image_classify
{
	float   score[5];
	unsigned int  topClass[5];
}img_classify_out_t;
/*=================================================
output of aml_module_t type:OBJECT_DETECT
detNum: the number of object detect
pBox:   the object info detect
=================================================*/
typedef struct __nn_obj_detect
{
	unsigned int  detNum;
	detBox *pBox;
}obj_detect_out_t;
/*=================================================
output of aml_module_t type:FACE_DETECTION
detNum: the number of face detect
pBox:   the face info detect
=================================================*/
typedef struct __nn_face_detect
{
	unsigned int  detNum;
	detBox pBox[MAX_DETECT_NUM];
}face_detect_out_t;
/*=================================================
output of aml_module_t type:FACE_LANDMARK_5
pos value:
0:left-eye,
1:right-eye,
2:nose,
3:left-mouth,
4:right-mouth
=================================================*/
typedef struct __nn_face_landmark_5
{
	unsigned int   detNum;
	detBox facebox[MAX_DETECT_NUM];
	point_t pos[MAX_DETECT_NUM][5];
}face_landmark5_out_t;

/*=================================================
output of aml_module_t type:FACE_LANDMARK_68
68 pos index:
0-16是下颌线，
17-21是右眼眉，
22-26是左眼眉，
27-35是鼻子，
36-41是右眼，
42-47是左眼，
48-60是嘴外轮廓，
61-67是嘴内轮廓。
=================================================*/
typedef struct __nn_face_landmark_68
{
	unsigned int detNum;
	detBox facebox[MAX_DETECT_NUM];
	point_t pos[MAX_DETECT_NUM][68];
}face_landmark68_out_t;
/*=================================================
output of aml_module_t type:FACE_RECOGNIZE
faceVector: the vector generate by input face,
can recognized by this vector
=================================================*/
typedef struct __nn_face_recognize
{
	float faceVector[512];
}face_recognize_out_t;
/*=================================================
output of aml_module_t type:FACE_COMPARISION
compare the input two image whether the same person or not:
compareScore: the score of compare two input face,
if compareScore >= 1.18, same person for input two face
if compareScore < 1.18,  not same person
=================================================*/
typedef struct __nn_face_compare
{
	float compareScore;
}face_compare_out_t;
/*=================================================
output of aml_module_t type:FACE_AGE
age value:
=================================================*/
typedef struct __nn_face_age
{
	int age;
}face_age_out_t;
/*=================================================
output of aml_module_t type:FACE_GENDER
female rate:
gender > 0.5: female
gender < 0.5: male
=================================================*/
typedef struct __nn_face_gender
{
	float gender;
}face_gender_out_t;
/*=================================================
output of aml_module_t type:FACE_EMOTION
0: 'angry',
1: 'disgust',
2: 'fear',
3: 'happy',
4: 'sad',
5: 'surprise',
6: 'neutral'
=================================================*/
typedef struct __nn_face_emotion
{
	int emotion;
}face_emotion_out_t;
/*=================================================
output of aml_module_t type:BODY_POSE
now just support single person pose
0:'Nose',
1:'Neck',
2:'R-Sho',
3:'R-Elb',
4:'R-Wr',
5:'L-Sho',
6:'L-Elb',
7:'L-Wr',
8:'R-Hip',
9:'R-Knee',
10:'R-Ank',
11:'L-Hip',
12:'L-Knee',
13:'L-Ank',
14:'R-Eye',
15:'L-Eye',
16:'R-Ear',
17:'L-Ear'
=================================================*/
typedef struct __nn_bodypos
{
	int valid;    //whether this point is valid
	point_t pos;
}bodypos_t;
typedef struct __nn_body_pose
{
	bodypos_t bpos[18];
}body_pose_out_t;


/*=================================================
output of aml_module_t type:FINGER_POSE
=================================================*/
typedef struct __nn_finger_pose
{
	image_out_t fingerOut;
}finger_pose_out_t;
/*=================================================
output of aml_module_t type:IMAGE_SR
=================================================*/
typedef struct __nn_sr_out
{
	image_out_t srOut;
}sr_out_t;
/*=================================================
output of aml_module_t type:IMAGE_SEGMENTATION
=================================================*/
typedef struct __nn_segment_out
{
	image_out_t segOut;
}segment_out_t;

/*=================================================
output of aml_module_t type:HEAD_DETECTION
the return value are detect region for these type.
=================================================*/
typedef struct __nn_head_detect
{
	obj_detect_out_t headOut;
}head_det_out_t;
/*=================================================
output of aml_module_t type:CARPLATE_DETECTION
the return value are detect region for these type.
=================================================*/
typedef struct __nn_car_detect
{
	obj_detect_out_t carOut;
}car_det_out_t;
/*=================================================
output of aml_module_t type:TEXT_DETECTION
the return value are detect region for these type.
=================================================*/
typedef struct __nn_text_detect
{
	obj_detect_out_t textOut;
}text_det_out_t;
/*=================================================
output of aml_module_t type:CARPLATE_RECOG
the output for car license number
num: the number of character for car license
val: the index number for license value;
10(0-9)+26(A-Z)+32(chinese character)
=================================================*/
typedef struct __nn_car_recognize
{
	int num;
	unsigned char val[32];
}car_license_out_t;

/*=================================================
the common type for sdk api
=================================================*/
typedef struct __aml_nn_config
{
	const char *path; //the nbg path
	amlnn_model_type modelType;
	assign_user_address_t inOut;
}aml_config;
/*=============================================================
create new module, should set config struct
==============================================================*/
void* aml_module_create(aml_config* config);

/*=============================================================
the input value for context, the size should set for type:

	IMAGE_CLASSIFY      = 0,     //224*224*3
	OBJECT_DETECT       = 1,     //obj_detect_out_t
    FACE_DETECTION      = 2,     //640*384*3
	FACE_LANDMARK_5     = 3,     //640*384*3
    FACE_LANDMARK_68    = 4,     //face_landmark68_out_t
    FACE_RECOGNIZE      = 5,     //112*112*3
	FACE_COMPARISION    = 6,     //set two input,every 112*112*3
	FACE_AGE            = 7,     //227*227*3
	FACE_GENDER         = 8,     //227*227*3
	FACE_EMOTION        = 9,     //48*48*1
    BODY_POSE           = 10,    //640*480*3
    FINGER_POSE         = 11,    //640*480*3
    HEAD_DETECTION      = 12,    //head_det_out_t
    CARPLATE_DETECTION  = 13,    //car_det_out_t
    CARPLATE_RECOG      = 14,    //car_license_out_t
	TEXT_DETECTION      = 15,    //text_det_out_t
	IMAGE_SR            = 16,    //sr_out_t
	IMAGE_SEGMENTATION  = 17,    //segment_out_t
==============================================================*/
int aml_module_input_set(void* context,nn_input *pInput);

/*==============================================================
the output data type for input network type:
	IMAGE_CLASSIFY      = 0,     //img_classify_out_t
	OBJECT_DETECT       = 1,     //obj_detect_out_t
    FACE_DETECTION      = 2,     //face_detect_out_t
	FACE_LANDMARK_5     = 3,     //face_landmark5_out_t
    FACE_LANDMARK_68    = 4,     //face_landmark68_out_t
    FACE_RECOGNIZE      = 5,     //face_recognize_out_t
	FACE_COMPARISION    = 6,     //face_compare_out_t
	FACE_AGE            = 7,     //face_age_out_t
	FACE_GENDER         = 8,     //face_gender_out_t
	FACE_EMOTION        = 9,     //face_emotion_out_t
    BODY_POSE           = 10,    //body_pose_out_t
    FINGER_POSE         = 11,    //finger_pose_out_t
    HEAD_DETECTION      = 12,    //head_det_out_t
    CARPLATE_DETECTION  = 13,    //car_det_out_t
    CARPLATE_RECOG      = 14,    //car_license_out_t
	TEXT_DETECTION      = 15,    //text_det_out_t
	IMAGE_SR            = 16,    //sr_out_t
	IMAGE_SEGMENTATION  = 17,    //segment_out_t
=============================================================*/
void* aml_module_output_get(void* context,aml_output_config_t outconfig);

/*============================================================
destroy context and release input/output memory
==============================================================*/
int aml_module_destroy(void* context);
/*============================================================
             some aml sdk util functions
==============================================================*/
unsigned char * aml_util_mallocAlignedBuffer(int mem_size);
void aml_util_freeAlignedBuffer(unsigned char *addr);
int aml_util_swapInputBuffer(void *context,void *newBuffer,unsigned int inputId);

#ifdef __cplusplus
} //extern "C"
#endif
#endif // _AMLNNSDK_H
