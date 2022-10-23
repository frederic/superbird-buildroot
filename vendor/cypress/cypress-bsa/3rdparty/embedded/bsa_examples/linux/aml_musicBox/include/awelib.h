/****************************************************************************
 *
 *		AWE Library
 *		-----------
 *
 ****************************************************************************
 *
 *	Description:	AWE library.
 *
 *	Copyright:		DSP Concepts, 2006-2017, All rights reserved.
 *	Owner:			DSP Concepts
 *					568 E. Weddell Drive, Suite 3
 *					Sunnyvale, CA 94089
 *
 ***************************************************************************/

/**
 *	@addtogroup AWELibrary
 *	@{
 */

#ifndef __AWELIB_H_INCLUDED__
#define __AWELIB_H_INCLUDED__

/** Create a virtual address. */
#define MAKE_ADDRESS(id, idx)		((id) << 12 | (idx))

#ifndef DOXYGEN
#define MAKE_WIRE_ADDRESS(id)		MAKE_ADDRESS(id, 3)

#define MAKE_LAYOUT_ADDRESS(id)		MAKE_ADDRESS(id, 0)

#ifndef UINT32
#define UINT32 unsigned int
#endif

#ifndef INT32
#define INT32 int
#endif

#ifndef UINT64
#define UINT64 unsigned long long
#endif
#endif

#ifndef _FRAMEWORK_H
#ifndef MAX_COMMAND_BUFFER_LEN
#define MAX_COMMAND_BUFFER_LEN	(4096 + 9)	//DWORDS - space for 4096 samples + header
#endif

/** Union of all possible sample types. */
typedef union _Sample
{
	INT32 iVal;
	UINT32 uiVal;
	float fVal;
}
Sample;
#endif

/**
 * @class CAWELib
 * @brief Interface for the AudioWeaver library.
 *
 * This is an abstract interface to the AWE library. You construct
 * an instance by calling AWELibraryFactory().  The constructed instance
 * is not usable until you call Create() which creates an empty AWE, and
 * can't be used to process audio until you call LoadAwbFile() to instantiate
 * audio modules.
 *
 * After loading the AWB file, you must call PinProps() to get the layout
 * properties, and pass the wire IDs to SetLayoutAddresses(). You can then call
 * PumpAudio() to process blocks of samples.
 *
 * The AWB file you load is a binary representation of a Designer layout you
 * create. The block size and channel counts for input and output are
 * determined by the layout design, which is why you must call PinProps() to
 * query those values.
 *
 * See provided example code for details of how to do this.
 */
struct CAWELib
{
	/**
	 * @brief Destructor.
	 */
	virtual ~CAWELib()
	{
	}

	/**
	 * @brief Get the average cycles used by audio.
	 * @return						the average cycles
	 *
	 * The return value is in units of the sampling clock
	 * used by your CPU. This should be the same as the value
	 * you specified when you called Create(). The value is
	 * determined by hardware. See your processor data sheet.
	 */
	virtual double AverageCycles() const = 0;

	/**
	 * @brief Get the pump count
	 * @return						the pump count
	 */
	virtual UINT64 PumpCount() const = 0;

	/** Clear measure of average cycles. */
	virtual void ClearMeasure() = 0;

	/**
	 * @brief Get the library version
	 * @return						the library version string
	 */
	virtual const char *GetLibraryVersion() const = 0;

	/**
	 * @brief Specify addresses in layout.
	 * @param [in] inId				ID of input wire
	 * @param [in] outId			ID of output wire
	 * @param [in] layoutId			ID of layout default 0
	 *
	 * You must set the wire IDs for input and output before you can pump.
	 * A layout ID of 0 means use the default layout.
	 */
	virtual void SetLayoutAddresses(UINT32 inId, UINT32 outId, UINT32 layoutId = 0) = 0;

	/**
	 * @brief test if the AWE instance has been created
	 * @return						true if it has
	 */
	virtual bool IsCreated() const = 0;

	/**
	 * @brief Create the AWE instance.
	 * @param [in] name				instance name (max 8 chars)
	 * @param [in] cpu_clock_speed	CPU clock speed in Hz
	 * @param [in] cpu_cycle_speed	sample clock speed in Hz
	 * @return						0 or error code
	 *
	 * Creates the AWE instance. All other APIs will fail until you call this.
	 * You must specify a name for this instance (max 8 chars), the CPU clock frequency
	 * and sample clock frequency (usually the same).
	 *
	 * Cycle speed is the clock used for cycle counting. On a number of CPUs this
	 * is not the same as the CPU clock. The value is used for Matlab profiling to
	 * compute MIPs. See your processor data sheet.
	 *
	 * On many Linux systems and on Windows the profile value should be 10000000 (10MHz).
	 *
	 * Possible error returns:
	 * E_ALREADY_CREATED - Create() has already been called.
	 * E_ARGUMENT_ERROR - invalid name.
	 */
	virtual int Create(const char *name, float cpu_clock_speed, float cpu_cycle_speed) = 0;

	/**
	 * @brief Destroy the AWE instance.
	 *
	 * This destroys the AWE instance. You can later call Create() to create a new
	 * instance.
	 */
	virtual void Destroy() = 0;

	/**
	 * @brief Write samples, pump, and read samples
	 * @param [in] in_samples		samples to send to AWE
	 * @param [out] out_samples		where to write reply samples
	 * @param [in] in_sample_count	number of input samples
	 * @param [in] out_sample_count number of output samples
	 * @return						0 or error code
	 *
	 * Processes the input samples through the layout to the output samples
	 * and keeps track of the cycles used by audio processing. The wire IDs must
	 * have been specified first, or this call will fail.
	 * The counts must be block size * channels.
	 *
	 * If audio has not been started by the layout, it is safe to call this
	 * but it will not process samples, just clear the output buffer and
	 * return E_NO_LAYOUTS or E_AUDIO_NOT_STARTED.
	 *
	 * Possible error returns:
	 * E_NOT_CREATED - Create() must be called before use.
	 * E_WIRES_NOT_SPECIFIED - wire IDs were not set by SetLayoutAddresses().
	 * E_NO_LAYOUTS - no layout, use LoadAwbFile().
	 * E_AUDIO_NOT_STARTED - audio has not yet started, start it with SendCommand() or use LoadAwbFile().
	 * Others - see Errors.h.
	 */
	virtual int PumpAudio(const void *in_samples, void *out_samples,
						  UINT32 in_sample_count, UINT32 out_sample_count) = 0;

	/**
	 * @brief Query the wire properties
	 * @param [out] in_samps		input wire samples
	 * @param [out] in_chans		input wire channels
	 * @param [out] in_complex		input wire complex
	 * @param [out] in_ID			ID of input
	 * @param [out] out_samps		output wire samples
	 * @param [out] out_chans		output wire channels
	 * @param [out] out_complex		output wire complex
	 * @param [out] out_ID			ID of output
	 * @return						0, 1, or error code
	 *
	 * Queries the layout for the input and output details. Wires are almost never
	 * complex, but if they are there are two values (real, imag) per logical sample.
	 * Return the block size, channels, complex, and ID of the the wires.
	 * If there is no layout, returns 0, otherwise returns 1 and these details.
	 *
	 * Possible error returns:
	 * E_NOT_CREATED - Create() must be called before use.
	 * Others - see Errors.h.
	 */
	virtual int PinProps(UINT32 &in_samps, UINT32 &in_chans, UINT32 &in_complex, UINT32 &in_ID,
						 UINT32 &out_samps, UINT32 &out_chans, UINT32 &out_complex, UINT32 &out_ID) = 0;

	/**
	 * @brief Fetch a 32-bit value from the given address.
	 * @param [in] address			Address to fetch value from.
	 * @param [out] retVal			Error reason code.
	 * @return						Fetched value.
	 *
	 * AWE stores array values and scalar values differently use this
	 * when reading scalar values.
	 *
	 * Possible error returns:
	 * E_NOT_CREATED - Create() must be called before use.
	 * Others - see Errors.h.
	 */
	virtual Sample FetchValue(
		UINT32 address,
		int *retVal) = 0;

	/**
	 * @brief Set a 32-bit value to the given address.
	 * @param [in] address			Address to set value to
	 * @param [in] val				value to set
	 * @return						0 or error code
	 *
	 * AWE stores array values and scalar values differently use this
	 * when writing scalar values.
	 *
	 * Possible error returns:
	 * E_NOT_CREATED - Create() must be called before use.
	 * Others - see Errors.h.
	 */
	virtual int SetValue(
		UINT32 address,
		Sample val) = 0;

	/**
	 * @brief Fetch array values
	 * @param [in] address			base address
	 * @param [in] argCount			count of values
	 * @param [out] args			the output values
	 * @param [in] offset			offset from base default 0
	 * @return						0 or error code
	 *
	 * AWE stores array values and scalar values differently use this
	 * when reading array values  even if only one element.
	 *
	 * Possible error returns:
	 * E_NOT_CREATED - Create() must be called before use.
	 * Others - see Errors.h.
	 */
	virtual int FetchValues(
		UINT32 address,
		UINT32 argCount,
		Sample *args,
		UINT32 offset = 0) = 0;

	/**
	 * @brief Set array values
	 * @param [in] address			base address
	 * @param [in] argCount			count of values
	 * @param [in] args				the argument values
	 * @param [in] offset			offset from base default 0
	 * @return						0 or error code
	 *
	 * AWE stores array values and scalar values differently use this
	 * when writing array values  even if only one element.
	 *
	 * Possible error returns:
	 * E_NOT_CREATED - Create() must be called before use.
	 * Others - see Errors.h.
	 */
	virtual int SetValues(
		UINT32 address,
		UINT32 argCount,
		const Sample *args,
		UINT32 offset = 0) = 0;

	/**
	 * @brief Load an AWB file into AWE.
	 * @param [in] filename			file to load
	 * @return						0 or error code
	 *
	 * Loads a layout file in AWB format. After loading a layout, you
	 * should call PinProps() to find out the layout input and output details.
	 *
	 * Possible error returns:
	 * E_NOT_CREATED - Create() must be called before use.
	 * E_ARGUMENT_ERROR - filename is not valid.
	 * Others - see Errors.h.
	 */
	virtual int LoadAwbFile(const char *filename) = 0;

	/**
	 * @brief Send a binary command to AWE.
	 * @param [in out] cmdBuffer	buffer containing command
	 * @param [in] cmdBufferLen		buffer size in words
	 * @return						0 or error code
	 *
	 * Warning: replies can be much larger than commands.
	 * The buffer should be sized at MAX_COMMAND_BUFFER_LEN to handle the worst case.
	 * On entry, the buffer must contain a binary AWE command packet.
	 * On return, the buffer contains the reply packet.
	 *
	 * Possible error returns:
	 * E_NOT_CREATED - Create() must be called before use.
	 * Others - see Errors.h.
	 */
	virtual int SendCommand(UINT32 *cmdBuffer, UINT32 cmdBufferLen) = 0;

	/**
	 * @brief Get the cycle counter.
	 * @return						the cycles
	 *
	 * The return value is in units of the cycle counter frequency.
	 */
	virtual UINT32 GetCycles() = 0;

	/**
	 * @brief Test if audio is running.
	 * @return						true if it is
	 */
	virtual bool IsStarted() = 0;

	/**
	 * @brief Query the wire properties with optional sample rate
	 * @param [out] in_samps		input wire samples
	 * @param [out] in_chans		input wire channels
	 * @param [out] in_complex		input wire complex
	 * @param [out] in_ID			ID of input
	 * @param [out] out_samps		output wire samples
	 * @param [out] out_chans		output wire channels
	 * @param [out] out_complex		output wire complex
	 * @param [out] out_ID			ID of output
	 * @param [out] pSrate			optional layout sample rate
	 * @return						0, 1, or error code
	 *
	 * Queries the layout for the input and output details. Wires are almost never
	 * complex, but if they are there are two values (real, imag) per logical sample.
	 * Return the block size, channels, complex, and ID of the the wires.
	 * If there is no layout, returns 0, otherwise returns 1 and these details.
	 *
	 * Possible error returns:
	 * E_NOT_CREATED - Create() must be called before use.
	 * Others - see Errors.h.
	 */
	virtual int PinPropsEx(UINT32 &in_samps, UINT32 &in_chans, UINT32 &in_complex, UINT32 &in_ID,
						   UINT32 &out_samps, UINT32 &out_chans, UINT32 &out_complex, UINT32 &out_ID,
						   UINT32 *pSrate = 0) = 0;

	/**
	 * @brief Create the AWE instance with profile control.
	 * @param [in] name				instance name (max 8 chars)
	 * @param [in] cpu_clock_speed	CPU clock speed in Hz
	 * @param [in] cpu_cycle_speed	sample clock speed in Hz
	 * @param [in] profile			default true, pass false to disable profiling
	 * @return						0 or error code
	 *
	 * Creates the AWE instance. All other APIs will fail until you call this.
	 * You must specify a name for this instance (max 8 chars), the CPU clock frequency
	 * and sample clock frquency (usually the same).
	 *
	 * Cycle speed is the clock used for cycle counting. On a number of CPUs this
	 * is not the same as the CPU clock. The value is used for Matlab profiling to
	 * compute MIPs. See your processor data sheet.
	 *
	 * On many Linux systems and on Windows the cycle value should be 10000000 (10MHz).
	 *
	 * Possible error returns:
	 * E_ALREADY_CREATED - Create() has already been called.
	 * E_ARGUMENT_ERROR - invalid name.
	 */
	virtual int CreateEx(const char *name, float cpu_clock_speed, float cpu_cycle_speed,
						 bool profile = true) = 0;

	/**
	 * @brief Set callback handlers for start and stop audio events.
	 * @param [in] param			parameter to pass to callbacks
	 * @param [in] start			start event handler
	 * @param [in] stop				stop event handler
	 *
	 * You can disable callbacks again by passing NULLs.
	 *
	 * The callbacks are only called when the audio state changes.
	 */
	virtual void SetStartStopCallbacks(void *param, void (*start)(void *), void (*stop)(void *)) = 0;

	/**
	 * @brief Set the file search path
	 * @param searchPath [in]		vertical bar delimited path
	 *
	 * Some modules that read files use a search path to locate them.
	 * The items in this path must be '|' delimited. If not called,
	 * a path of '.' is assumed, meaning the working directory.
	 */
	virtual void SetSearchPath(const char *searchPath) = 0;
};

#ifdef WIN32
extern "C" {
#endif

	/**
	 * @brief Create an instance of the library object.
	 * @return							the instance
	 */
#if defined(WIN32) && !defined(FORMAIN)
	DLLSYMBOL
#endif
	CAWELib *AWELibraryFactory();

#ifdef WIN32
}
#endif

#endif	// __AWELIB_H_INCLUDED__

/**
 * @}
 *
 * End of file.
 */

