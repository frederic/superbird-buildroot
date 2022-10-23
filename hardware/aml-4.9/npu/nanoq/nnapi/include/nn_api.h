/****************************************************************************
*   amlogic nn api header file
*   
*   Neural Network appliction network definition header file
*
*   Date: 2019.8
****************************************************************************/

#ifndef _AMLOGIC_NN_API_H
#define _AMLOGIC_NN_API_H

#ifdef __cplusplus
extern "C"{
#endif

//////////////////////////////////nn api/////////////////////////////
#define OUTPUT_MAX_NUM 32

typedef enum _amlnn_model_ {
    CAFFE    = 0, 
	TENSORFLOW, 
    TENSORFLOWLITE,
    DARKNET, 
    ONNX, 
	KERAS,
    MEDEL_MAX
} amlnn_model_type;
/*=================================================================================

==================================================================================*/
typedef enum _amlnn_input_ {
	RGB24_RAW_DATA = 0,
	TENSOR_RAW_DATA,
	QTENSOR_RAW_DATA,
	BINARY_RAW_DATA,
	RAW_DATA_MAX
} amlnn_input_type;

 /*  QUERY_PERF_RUN,  query the time of run,should query after amlnn_outputs_get. */
typedef enum _amlnn_query_cmd {
    QUERY_INPUT_NUM = 0,                                /* query the number of input  tensor. */
	QUERY_OUTPUT_NUM,                                   /* query the number of output  tensor. */
    QUERY_INPUT_ATTR,                                   /* query the attribute of input tensor. */
    QUERY_OUTPUT_ATTR,                                  /* query the attribute of output tensor. */
    QUERY_SDK_VERSION,                                  /* query the sdk & driver version */
    QUERY_CMD_MAX
} amlnn_query_cmd;
typedef enum _nn_buffer_format_e
{
    /*! \brief A float type of buffer data */
    NN_BUFFER_FORMAT_FP32       = 0,
    /*! \brief A half float type of buffer data */
    NN_BUFFER_FORMAT_FP16       = 1,
    /*! \brief A 8 bit unsigned integer type of buffer data */
    NN_BUFFER_FORMAT_UINT8      = 2,
    /*! \brief A 8 bit signed integer type of buffer data */
    NN_BUFFER_FORMAT_INT8       = 3,
    /*! \brief A 16 bit unsigned integer type of buffer data */
    NN_BUFFER_FORMAT_UINT16     = 4,
    /*! \brief A 16 signed integer type of buffer data */
    NN_BUFFER_FORMAT_INT16      = 5
}   nn_buffer_format_e;

typedef enum _nn_buffer_quantize_format_e
{
    /*! \brief Not quantized format */
    NN_BUFFER_QUANTIZE_NONE                    = 0,
    /*! \brief The data is quantized with dynamic fixed point */
    NN_BUFFER_QUANTIZE_DYNAMIC_FIXED_POINT     = 1,
    /*! \brief The data is quantized with TF asymmetric format */
    NN_BUFFER_QUANTIZE_TF_ASYMM                = 2
}   nn_buffer_quantize_format_e;


typedef struct _nn_buffer_create_params_t
{
    unsigned int     num_of_dims; /*!< \brief The number of dimensions specified in *sizes*/
    unsigned int      sizes[4];       /*!< \brief The pointer to an array of dimension */
    nn_buffer_format_e          data_format; /*!< \brief Data format for the tensor, see <tt>\ref nn_buffer_format_e </tt> */
    nn_buffer_quantize_format_e quant_format; /*!< \brief Quantized format see <tt>\ref nn_buffer_quantize_format_e </tt>. */
    union {
        struct {
            unsigned char fixed_point_pos; /*!< \brief Specifies the fixed point position when the input element type is int16, if 0 calculations are performed in integer math */
        } dfp;

        struct {
            float        scale;       /*!< \brief Scale value for the quantized value */
            unsigned int zeroPoint;  /*!< \brief  A 32 bit integer, in range [0, 255] */
        } affine;
     }
     quant_data; /*<! \brief The union of quantization information */
} nn_buffer_params_t;

typedef struct out_buf
{
	unsigned int size;
	unsigned char *buf;
	nn_buffer_params_t *param;
}outBuf_t;

typedef struct __nnout
{
	unsigned int num;   /*===========output tensor number============*/
	outBuf_t out[OUTPUT_MAX_NUM];
}nn_output;

typedef struct __queryout
{
	unsigned int size;   /*===========output tensor number============*/
	void *buf;
}nn_query;


typedef struct __nn_input
{
	int  input_index;
	int  size;
	unsigned char* input;
	amlnn_input_type input_type;
}nn_input;

/*  amlnn_init

    initial the context and load the amlogic nbg model.

    input:   
        const char* nbg_model_file  model file url,valid path for model file.
        uint32_t size               the size of nbg model.
        amlnn_model_type type       model type,must give it for preprocess and other optimize.
    return:
        the pointer of context handle.
		null if failed
*/
void* amlnn_init(const char * nbg_model_file, amlnn_model_type type);

/*  amlnn_query

    query the information about model or others. see amlnn_query_cmd.
	The buffer is reserved by api core
    input:
        context                     the handle of context.
        amlnn_query_cmd cmd         the command of query.
        unsigned int number         for QUERY_INPUT_ATTR and QUERY_OUTPUT_ATTR, the index of query input/output(just for mutiple-input/output)
    return:
		nn_query *,in struct, void *buf type is:
		QUERY_INPUT_NUM        unsigned int                             
		QUERY_OUTPUT_NUM       unsigned int                   
		QUERY_INPUT_ATTR       nn_buffer_params_t                    
		QUERY_OUTPUT_ATTR,     nn_buffer_params_t                          
		QUERY_SDK_VERSION      unsigned char               
*/
nn_query *amlnn_query(void* context, amlnn_query_cmd cmd,unsigned int number);
/*  amlnn_inputs_set

    set inputs information by input index of nbg model.
    inputs information see ngb_input.

    input:
        void context                the handle of context.
        int  input_num              for multiple input,this value is the number of inputs.
        int  size                   the size of input value
		unsigned char *input        the raw data of input, for example, bmp or jpeg picture, this value is the raw data after decode.
    return:
        int                         error code
*/
int amlnn_inputs_set(void* context,nn_input *pInput);
/*  amlnn_run

    run the model to execute inference.
    this function will blocked until inference over.

    input:
        void* context               the handle of context.
        int  extend                 some control flag,reserved for future
    return:
        int                         error code.
*/
int amlnn_run(void* context, int extend);
/*  amlnn_outputs_get

    wait the inference to finish and get the outputs.
    this function will block until inference finish.
    the results will set to outputs[].

    input:
        void* context        the handle of context.
		nn_output struct point
    return:
		int    error code.
*/
nn_output* amlnn_outputs_get(void* context);


/*  amlnn_destroy

    destroy the context, release output buffer.

    input:   
        void* context              pointer of context handle.

    return:
        int    error code.
*/

int amlnn_destroy(void* context);

#ifdef __cplusplus
}
#endif

#endif
