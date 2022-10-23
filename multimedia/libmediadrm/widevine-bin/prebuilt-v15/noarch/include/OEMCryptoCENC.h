// Copyright 2018 Google LLC. All Rights Reserved. This file and proprietary
// source code may only be used and distributed under the Widevine Master
// License Agreement.

/*********************************************************************
 * OEMCryptoCENC.h
 *
 * Reference APIs needed to support Widevine's crypto algorithms.
 *
 * See the document "WV Modular DRM Security Integration Guide for Common
 * Encryption (CENC) -- version 15.2" for a description of this API. You
 * can find this document in the widevine repository as
 * docs/WidevineModularDRMSecurityIntegrationGuideforCENC_v15.pdf
 * Changes between different versions of this API are documented in the files
 * docs/Widevine_Modular_DRM_Version_*_Delta.pdf
 *
 *********************************************************************/

#ifndef OEMCRYPTO_CENC_H_
#define OEMCRYPTO_CENC_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t OEMCrypto_SESSION;

typedef enum OEMCryptoResult {
  OEMCrypto_SUCCESS                            = 0,
  OEMCrypto_ERROR_INIT_FAILED                  = 1,
  OEMCrypto_ERROR_TERMINATE_FAILED             = 2,
  OEMCrypto_ERROR_OPEN_FAILURE                 = 3,
  OEMCrypto_ERROR_CLOSE_FAILURE                = 4,
  OEMCrypto_ERROR_ENTER_SECURE_PLAYBACK_FAILED = 5,  // deprecated
  OEMCrypto_ERROR_EXIT_SECURE_PLAYBACK_FAILED  = 6,  // deprecated
  OEMCrypto_ERROR_SHORT_BUFFER                 = 7,
  OEMCrypto_ERROR_NO_DEVICE_KEY                = 8,  // no keybox device key.
  OEMCrypto_ERROR_NO_ASSET_KEY                 = 9,
  OEMCrypto_ERROR_KEYBOX_INVALID               = 10,
  OEMCrypto_ERROR_NO_KEYDATA                   = 11,
  OEMCrypto_ERROR_NO_CW                        = 12,
  OEMCrypto_ERROR_DECRYPT_FAILED               = 13,
  OEMCrypto_ERROR_WRITE_KEYBOX                 = 14,
  OEMCrypto_ERROR_WRAP_KEYBOX                  = 15,
  OEMCrypto_ERROR_BAD_MAGIC                    = 16,
  OEMCrypto_ERROR_BAD_CRC                      = 17,
  OEMCrypto_ERROR_NO_DEVICEID                  = 18,
  OEMCrypto_ERROR_RNG_FAILED                   = 19,
  OEMCrypto_ERROR_RNG_NOT_SUPPORTED            = 20,
  OEMCrypto_ERROR_SETUP                        = 21,
  OEMCrypto_ERROR_OPEN_SESSION_FAILED          = 22,
  OEMCrypto_ERROR_CLOSE_SESSION_FAILED         = 23,
  OEMCrypto_ERROR_INVALID_SESSION              = 24,
  OEMCrypto_ERROR_NOT_IMPLEMENTED              = 25,
  OEMCrypto_ERROR_NO_CONTENT_KEY               = 26,
  OEMCrypto_ERROR_CONTROL_INVALID              = 27,
  OEMCrypto_ERROR_UNKNOWN_FAILURE              = 28,
  OEMCrypto_ERROR_INVALID_CONTEXT              = 29,
  OEMCrypto_ERROR_SIGNATURE_FAILURE            = 30,
  OEMCrypto_ERROR_TOO_MANY_SESSIONS            = 31,
  OEMCrypto_ERROR_INVALID_NONCE                = 32,
  OEMCrypto_ERROR_TOO_MANY_KEYS                = 33,
  OEMCrypto_ERROR_DEVICE_NOT_RSA_PROVISIONED   = 34,
  OEMCrypto_ERROR_INVALID_RSA_KEY              = 35,
  OEMCrypto_ERROR_KEY_EXPIRED                  = 36,
  OEMCrypto_ERROR_INSUFFICIENT_RESOURCES       = 37,
  OEMCrypto_ERROR_INSUFFICIENT_HDCP            = 38,
  OEMCrypto_ERROR_BUFFER_TOO_LARGE             = 39,
  OEMCrypto_WARNING_GENERATION_SKEW            = 40,  // Warning, not an error.
  OEMCrypto_ERROR_GENERATION_SKEW              = 41,
  OEMCrypto_LOCAL_DISPLAY_ONLY                 = 42,  // Info, not an error.
  OEMCrypto_ERROR_ANALOG_OUTPUT                = 43,
  OEMCrypto_ERROR_WRONG_PST                    = 44,
  OEMCrypto_ERROR_WRONG_KEYS                   = 45,
  OEMCrypto_ERROR_MISSING_MASTER               = 46,
  OEMCrypto_ERROR_LICENSE_INACTIVE             = 47,
  OEMCrypto_ERROR_ENTRY_NEEDS_UPDATE           = 48,
  OEMCrypto_ERROR_ENTRY_IN_USE                 = 49,
  OEMCrypto_ERROR_USAGE_TABLE_UNRECOVERABLE    = 50,  // Reserved. Do not use.
  OEMCrypto_KEY_NOT_LOADED                     = 51,  // obsolete. use error 26.
  OEMCrypto_KEY_NOT_ENTITLED                   = 52,
  OEMCrypto_ERROR_BAD_HASH                     = 53,
  OEMCrypto_ERROR_OUTPUT_TOO_LARGE             = 54,
  OEMCrypto_ERROR_SESSION_LOST_STATE           = 55,
  OEMCrypto_ERROR_SYSTEM_INVALIDATED           = 56,
} OEMCryptoResult;

/*
 * OEMCrypto_DestBufferDesc
 *  Describes the type and access information for the memory to receive
 *  decrypted data.
 *
 *  The OEMCrypto API supports a range of client device architectures.
 *  Different architectures have different methods for acquiring and securing
 *  buffers that will hold portions of the audio or video stream after
 *  decryption. Three basic strategies are recognized for handling decrypted
 *  stream data:
 *  1. Return the decrypted data in the clear into normal user memory
 *     (ClearBuffer). The caller uses normal memory allocation methods to
 *     acquire a buffer, and supplies the memory address of the buffer in the
 *     descriptor.
 *  2. Place the decrypted data into protected memory (SecureBuffer). The
 *     caller uses a platform-specific method to acquire the protected buffer
 *     and a user-memory handle that references it. The handle is supplied
 *     to the decrypt call in the descriptor.  If the buffer is filled with
 *     several OEMCrypto calls, the same handle will be used, and the offset
 *     will be incremented to indicate where the next write should take place.
 *  3. Place the decrypted data directly into the audio or video decoder fifo
 *     (Direct). The caller will use platform-specific methods to initialize
 *     the fifo and the decoders. The decrypted stream data is not accessible
 *     to the caller.
 *
 *  Specific fields are as follows:
 *
 *  (type == OEMCrypto_BufferType_Clear)
 *      address - Address of start of user memory buffer.
 *      max_length - Size of user memory buffer.
 *  (type == OEMCrypto_BufferType_Secure)
 *      buffer - handle to a platform-specific secure buffer.
 *      max_length - Size of platform-specific secure buffer.
 *      offset - offset from beginning of buffer to which OEMCrypto should write.
 *  (type == OEMCrypto_BufferType_Direct)
 *      is_video - If true, decrypted bytes are routed to the video
 *                 decoder. If false, decrypted bytes are routed to the
 *                 audio decoder.
 */
typedef enum OEMCryptoBufferType {
  OEMCrypto_BufferType_Clear,
  OEMCrypto_BufferType_Secure,
  OEMCrypto_BufferType_Direct
} OEMCryptoBufferType;

typedef struct {
  OEMCryptoBufferType type;
  union {
    struct {  // type == OEMCrypto_BufferType_Clear
      uint8_t* address;
      size_t max_length;
    } clear;
    struct {  // type == OEMCrypto_BufferType_Secure
      void* handle;
      size_t max_length;
      size_t offset;
    } secure;
    struct {  // type == OEMCrypto_BufferType_Direct
      bool is_video;
    } direct;
  } buffer;
} OEMCrypto_DestBufferDesc;

/** OEMCryptoCipherMode is used in SelectKey to prepare a key for either CTR
 * decryption or CBC decryption.
 */
typedef enum OEMCryptoCipherMode {
  OEMCrypto_CipherMode_CTR,
  OEMCrypto_CipherMode_CBC,
} OEMCryptoCipherMode;

/** OEMCrypto_LicenseType is used in LoadKeys to indicate if the key objects
 *  are for content keys, or for entitlement keys.
 */
typedef enum OEMCrypto_LicenseType {
  OEMCrypto_ContentLicense = 0,
  OEMCrypto_EntitlementLicense = 1
} OEMCrypto_LicenseType;

/*
 *  OEMCrypto_Substring
 *
 *  Used to indicate a substring of a signed message in OEMCrypto_LoadKeys and
 *  other functions which must verify that a parameter is contained within a
 *  signed message.
 */
typedef struct {
  size_t offset;
  size_t length;
} OEMCrypto_Substring;

/*
 * OEMCrypto_KeyObject
 *  Points to the relevant fields for a content key. The fields are extracted
 *  from the License Response message offered to OEMCrypto_LoadKeys(). Each
 *  field points to one of the components of the key. Key data, key control,
 *  and both IV fields are 128 bits (16 bytes):
 *    key_id - the unique id of this key.
 *    key_id_length - the size of key_id. OEMCrypto may assume this is at
 *        most 16. However, OEMCrypto shall correctly handle key id lengths
 *        from 1 to 16 bytes.
 *    key_data_iv - the IV for performing AES-128-CBC decryption of the
 *        key_data field.
 *    key_data - the key data. It is encrypted (AES-128-CBC) with the
 *        session's derived encrypt key and the key_data_iv.
 *    key_control_iv - the IV for performing AES-128-CBC decryption of the
 *        key_control field.
 *    key_control - the key control block. It is encrypted (AES-128-CBC) with
 *        the content key from the key_data field.
 *
 *  The memory for the OEMCrypto_KeyObject fields is allocated and freed
 *  by the caller of OEMCrypto_LoadKeys().
 */
typedef struct {
  OEMCrypto_Substring key_id;
  OEMCrypto_Substring key_data_iv;
  OEMCrypto_Substring key_data;
  OEMCrypto_Substring key_control_iv;
  OEMCrypto_Substring key_control;
} OEMCrypto_KeyObject;

/*
 * SRM_Restriction_Data
 *
 * Structure passed into LoadKeys to specify required SRM version.
 */
typedef struct {
  uint8_t verification[8];       // must be "HDCPDATA"
  uint32_t minimum_srm_version;  // version number in network byte order.
} SRM_Restriction_Data;

/*
 * OEMCrypto_EntitledContentKeyObject
 * Contains encrypted content key data for loading into the sessions keytable.
 * The content key data is encrypted using AES-256-CBC encryption, with PKCS#7
 * padding.
 * entitlement_key_id - entitlement key id to be matched to key table.
 * entitlement_key_id_length - length of entitlment_key_id in bytes (1 to 16).
 * content_key_id - content key id to be loaded into key table.
 * content_key_id_length - length of content key id in bytes (1 to 16).
 * key_data_iv - the IV for performing AES-256-CBC decryption of the key data.
 * key_data - encrypted content key data.
 * key_data_length - length of key_data - 16 or 32 depending on intended use.
 */
typedef struct {
  OEMCrypto_Substring entitlement_key_id;
  OEMCrypto_Substring content_key_id;
  OEMCrypto_Substring content_key_data_iv;
  OEMCrypto_Substring content_key_data;
} OEMCrypto_EntitledContentKeyObject;

/*
 * OEMCrypto_KeyRefreshObject
 *  Points to the relevant fields for renewing a content key. The fields are
 *  extracted from the License Renewal Response message offered to
 *  OEMCrypto_RefreshKeys(). Each field points to one of the components of
 *  the key.
 *    key_id - the unique id of this key.
 *    key_control_iv - the IV for performing AES-128-CBC decryption of the
 *        key_control field. 16 bytes.
 *    key_control - the key control block. It is encrypted (AES-128-CBC) with
 *        the content key from the key_data field. 16 bytes.
 *
 *  The key_data is unchanged from the original OEMCrypto_LoadKeys() call. Some
 *  Key Control Block fields, especially those related to key lifetime, may
 *  change.
 *
 *  The memory for the OEMCrypto_KeyRefreshObject fields is allocated and freed
 *  by the caller of OEMCrypto_RefreshKeys().
 */
typedef struct {
  OEMCrypto_Substring key_id;
  OEMCrypto_Substring key_control_iv;
  OEMCrypto_Substring key_control;
} OEMCrypto_KeyRefreshObject;

/*
 * OEMCrypto_Algorithm
 * This is a list of valid algorithms for OEMCrypto_Generic_* functions.
 * Some are valid for encryption/decryption, and some for signing/verifying.
 */
typedef enum OEMCrypto_Algorithm {
  OEMCrypto_AES_CBC_128_NO_PADDING = 0,
  OEMCrypto_HMAC_SHA256 = 1,
} OEMCrypto_Algorithm;

/*
 * Flags indicating data endpoints in OEMCrypto_DecryptCENC.
 */
#define OEMCrypto_FirstSubsample 1
#define OEMCrypto_LastSubsample 2

/* OEMCrypto_CENCEncryptPatternDesc
 *  This is used in OEMCrypto_DecryptCENC to indicate the encrypt/skip pattern
 *  used, as specified in the CENC standard.
 */
typedef struct {
  size_t encrypt;  // number of 16 byte blocks to decrypt.
  size_t skip;     // number of 16 byte blocks to leave in clear.
  size_t offset;   // offset into the pattern in blocks for this call.
} OEMCrypto_CENCEncryptPatternDesc;

/*
 *  OEMCrypto_Usage_Entry_Status.
 *  Valid values for status in the usage table.
 */
typedef enum OEMCrypto_Usage_Entry_Status {
  kUnused = 0,
  kActive = 1,
  kInactive = 2,  // Deprecated.  Used kInactiveUsed or kInactiveUnused.
  kInactiveUsed = 3,
  kInactiveUnused = 4,
} OEMCrypto_Usage_Entry_Status;

/*
 * OEMCrypto_PST_Report is used to report an entry from the Usage Table.
 *
 * Platforms that have compilers that support packed structures, may use the
 * following definition.  Other platforms may use the header pst_report.h which
 * defines a wrapper class.
 *
 * All fields are in network byte order.
 */
#if 0  // If your compiler supports __attribute__((packed)).
typedef struct {
  uint8_t signature[20];  //  -- HMAC SHA1 of the rest of the report.
  uint8_t status;  // current status of entry. (OEMCrypto_Usage_Entry_Status)
  uint8_t clock_security_level;
  uint8_t pst_length;
  uint8_t padding;                         // make int64's word aligned.
  int64_t seconds_since_license_received;  // now - time_of_license_received
  int64_t seconds_since_first_decrypt;     // now - time_of_first_decrypt
  int64_t seconds_since_last_decrypt;      // now - time_of_last_decrypt
  uint8_t pst[];
} __attribute__((packed)) OEMCrypto_PST_Report;
#endif

/*
 *  OEMCrypto_Clock_Security_Level.
 *  Valid values for clock_security_level in OEMCrypto_PST_Report.
 */
typedef enum OEMCrypto_Clock_Security_Level {
  kInsecureClock = 0,
  kSecureTimer = 1,
  kSecureClock = 2,
  kHardwareSecureClock = 3
} OEMCrypto_Clock_Security_Level;

typedef uint8_t RSA_Padding_Scheme;
// RSASSA-PSS with SHA1.
#define kSign_RSASSA_PSS ((RSA_Padding_Scheme)0x1)
// PKCS1 with block type 1 padding (only).
#define kSign_PKCS1_Block1 ((RSA_Padding_Scheme)0x2)

/*
 * OEMCrypto_HDCP_Capability is used in the key control block to enforce HDCP
 * level, and in GetHDCPCapability for reporting.
 */
typedef enum OEMCrypto_HDCP_Capability {
  HDCP_NONE  = 0,               // No HDCP supported, no secure data path.
  HDCP_V1    = 1,               // HDCP version 1.0
  HDCP_V2    = 2,               // HDCP version 2.0 Type 1.
  HDCP_V2_1  = 3,               // HDCP version 2.1 Type 1.
  HDCP_V2_2  = 4,               // HDCP version 2.2 Type 1.
  HDCP_V2_3  = 5,               // HDCP version 2.3 Type 1.
  HDCP_NO_DIGITAL_OUTPUT = 0xff // No digital output.
} OEMCrypto_HDCP_Capability;

/* Return value for OEMCrypto_GetProvisioningMethod(). */
typedef enum OEMCrypto_ProvisioningMethod {
  OEMCrypto_ProvisioningError = 0,  // Device cannot be provisioned.
  OEMCrypto_DrmCertificate = 1,     // Device has baked in DRM certificate
                                    // (level 3 only)
  OEMCrypto_Keybox = 2,        // Device has factory installed unique keybox.
  OEMCrypto_OEMCertificate = 3 // Device has factory installed OEM certificate.
} OEMCrypto_ProvisioningMethod;

/*
 * Flags indicating RSA keys supported.
 */
#define OEMCrypto_Supports_RSA_2048bit 0x1
#define OEMCrypto_Supports_RSA_3072bit 0x2
#define OEMCrypto_Supports_RSA_CAST   0x10

/*
 * Flags indicating full decrypt path hash supported.
 */
#define OEMCrypto_Hash_Not_Supported 0
#define OEMCrypto_CRC_Clear_Buffer 1
#define OEMCrypto_Partner_Defined_Hash 2

/*
 * Return values from OEMCrypto_GetAnalogOutputFlags.
 */
#define OEMCrypto_No_Analog_Output            0x0
#define OEMCrypto_Supports_Analog_Output      0x1
#define OEMCrypto_Can_Disable_Analog_Ouptput  0x2
#define OEMCrypto_Supports_CGMS_A             0x4
// Unknown_Analog_Output is used only for backwards compatibility.
#define OEMCrypto_Unknown_Analog_Output       (1<<31)

/*
 * Obfuscation Renames.
 */
#define OEMCrypto_Initialize                  _oecc01
#define OEMCrypto_Terminate                   _oecc02
#define OEMCrypto_InstallKeybox               _oecc03
// Rename InstallKeybox to InstallKeyboxOrOEMCert.
#define OEMCrypto_InstallRootKeyCertificate   _oecc03
#define OEMCrypto_InstallKeyboxOrOEMCert      _oecc03
#define OEMCrypto_GetKeyData                  _oecc04
#define OEMCrypto_IsKeyboxValid               _oecc05
// Rename IsKeyboxValid to IsKeyboxOrOEMCertValid.
#define OEMCrypto_IsRootKeyCertificateValid   _oecc05
#define OEMCrypto_IsKeyboxOrOEMCertValid      _oecc05
#define OEMCrypto_GetRandom                   _oecc06
#define OEMCrypto_GetDeviceID                 _oecc07
#define OEMCrypto_WrapKeybox                  _oecc08
// Rename WrapKeybox to WrapKeyboxOrOEMCert
#define OEMCrypto_WrapRootKeyCertificate      _oecc08
#define OEMCrypto_WrapKeyboxOrOEMCert         _oecc08
#define OEMCrypto_OpenSession                 _oecc09
#define OEMCrypto_CloseSession                _oecc10
#define OEMCrypto_DecryptCTR_V10              _oecc11
#define OEMCrypto_GenerateDerivedKeys         _oecc12
#define OEMCrypto_GenerateSignature           _oecc13
#define OEMCrypto_GenerateNonce               _oecc14
#define OEMCrypto_LoadKeys_V8                 _oecc15
#define OEMCrypto_RefreshKeys_V14             _oecc16
#define OEMCrypto_SelectKey_V13               _oecc17
#define OEMCrypto_RewrapDeviceRSAKey          _oecc18
#define OEMCrypto_LoadDeviceRSAKey            _oecc19
#define OEMCrypto_GenerateRSASignature_V8     _oecc20
#define OEMCrypto_DeriveKeysFromSessionKey    _oecc21
#define OEMCrypto_APIVersion                  _oecc22
#define OEMCrypto_SecurityLevel               _oecc23
#define OEMCrypto_Generic_Encrypt             _oecc24
#define OEMCrypto_Generic_Decrypt             _oecc25
#define OEMCrypto_Generic_Sign                _oecc26
#define OEMCrypto_Generic_Verify              _oecc27
#define OEMCrypto_GetHDCPCapability_V9        _oecc28
#define OEMCrypto_SupportsUsageTable          _oecc29
#define OEMCrypto_UpdateUsageTable            _oecc30
#define OEMCrypto_DeactivateUsageEntry_V12    _oecc31
#define OEMCrypto_ReportUsage                 _oecc32
#define OEMCrypto_DeleteUsageEntry            _oecc33
#define OEMCrypto_DeleteOldUsageTable         _oecc34
#define OEMCrypto_LoadKeys_V9_or_V10          _oecc35
#define OEMCrypto_GenerateRSASignature        _oecc36
#define OEMCrypto_GetMaxNumberOfSessions      _oecc37
#define OEMCrypto_GetNumberOfOpenSessions     _oecc38
#define OEMCrypto_IsAntiRollbackHwPresent     _oecc39
#define OEMCrypto_CopyBuffer_V14              _oecc40
#define OEMCrypto_QueryKeyControl             _oecc41
#define OEMCrypto_LoadTestKeybox_V13          _oecc42
#define OEMCrypto_ForceDeleteUsageEntry       _oecc43
#define OEMCrypto_GetHDCPCapability           _oecc44
#define OEMCrypto_LoadTestRSAKey              _oecc45
#define OEMCrypto_Security_Patch_Level        _oecc46
#define OEMCrypto_LoadKeys_V11_or_V12         _oecc47
#define OEMCrypto_DecryptCENC                 _oecc48
#define OEMCrypto_GetProvisioningMethod       _oecc49
#define OEMCrypto_GetOEMPublicCertificate     _oecc50
#define OEMCrypto_RewrapDeviceRSAKey30        _oecc51
#define OEMCrypto_SupportedCertificates       _oecc52
#define OEMCrypto_IsSRMUpdateSupported        _oecc53
#define OEMCrypto_GetCurrentSRMVersion        _oecc54
#define OEMCrypto_LoadSRM                     _oecc55
#define OEMCrypto_LoadKeys_V13                _oecc56
#define OEMCrypto_RemoveSRM                   _oecc57
#define OEMCrypto_CreateUsageTableHeader      _oecc61
#define OEMCrypto_LoadUsageTableHeader        _oecc62
#define OEMCrypto_CreateNewUsageEntry         _oecc63
#define OEMCrypto_LoadUsageEntry              _oecc64
#define OEMCrypto_UpdateUsageEntry            _oecc65
#define OEMCrypto_DeactivateUsageEntry        _oecc66
#define OEMCrypto_ShrinkUsageTableHeader      _oecc67
#define OEMCrypto_MoveEntry                   _oecc68
#define OEMCrypto_CopyOldUsageEntry           _oecc69
#define OEMCrypto_CreateOldUsageEntry         _oecc70
#define OEMCrypto_GetAnalogOutputFlags        _oecc71
#define OEMCrypto_LoadTestKeybox              _oecc78
#define OEMCrypto_LoadEntitledContentKeys_V14 _oecc79
#define OEMCrypto_SelectKey                   _oecc81
#define OEMCrypto_LoadKeys_V14                _oecc82
#define OEMCrypto_LoadKeys                    _oecc83
#define OEMCrypto_SetSandbox                  _oecc84
#define OEMCrypto_ResourceRatingTier          _oecc85
#define OEMCrypto_SupportsDecryptHash         _oecc86
#define OEMCrypto_InitializeDecryptHash       _oecc87
#define OEMCrypto_SetDecryptHash              _oecc88
#define OEMCrypto_GetHashErrorCode            _oecc89
#define OEMCrypto_BuildInformation            _oecc90
#define OEMCrypto_RefreshKeys                 _oecc91
#define OEMCrypto_LoadEntitledContentKeys     _oecc92
#define OEMCrypto_CopyBuffer                  _oecc93

/*
 * OEMCrypto_SetSandbox
 *
 * Description:
 *   This tells OEMCrypto which sandbox the current process belongs to. Any
 *   persistent memory used to store the generation number should be associated
 *   with this sandbox id. OEMCrypto can assume that this sandbox will be tied
 *   to the current process or VM until OEMCrypto_Terminate is called. See the
 *   section "VM and Sandbox Support" above for more details.
 *
 *   If OEMCrypto does not support sandboxes, it will return
 *   OEMCrypto_ERROR_NOT_IMPLEMENTED. On most platforms, this function will
 *   just return OEMCrypto_ERROR_NOT_IMPLEMENTED. If OEMCrypto supports
 *   sandboxes, this function returns OEMCrypto_SUCCESS on success, and
 *   OEMCrypto_ERROR_UNKNOWN_FAILURE on failure.
 *
 *   The CDM layer will call OEMCrypto_SetSandbox once before
 *   OEMCrypto_Initialize. After this function is called and returns success,
 *   it will be OEMCrypto's responsibility to keep calls to usage table
 *   functions separate, and to accept a call to OEMCrypto_Terminate for each
 *   sandbox.
 *
 * Parameters:
 *   [in] sandbox_id: a short string unique to the current sandbox.
 *   [in] sandobx_id_length: length of sandbox_id.
 *
 * Returns:
 *   OEMCrypto_SUCCESS success
 *   OEMCrypto_ERROR_INIT_FAILED failed to initialize crypto hardware
 *   OEMCrypto_ERROR_NOT_IMPLEMENTED - sandbox functionality not supported
 *
 * Threading:
 *   This is an "Initialization and Termination Function" and will not be
 *   called simultaneously with any other function, as if the CDM holds a write
 *   lock on the OEMCrypto system. It is called once before
 *   OEMCrypto_Initialize.
 *
 * Version:
 *   This method is new in version 15 of the API.
 */
OEMCryptoResult OEMCrypto_SetSandbox(const uint8_t* sandbox_id,
                                     size_t sandbox_id_length);

/*
 * OEMCrypto_Initialize
 *
 * Description:
 *   Initialize the crypto firmware/hardware.
 *
 * Parameters:
 *   None
 *
 * Returns:
 *   OEMCrypto_SUCCESS success
 *   OEMCrypto_ERROR_INIT_FAILED failed to initialize crypto hardware
 *
 * Threading:
 *   This is an "Initialization and Termination Function" and will not be
 *   called simultaneously with any other function, as if the CDM holds a write
 *   lock on the OEMCrypto system.
 *
 * Version:
 *   This method is supported by all API versions.
 */
OEMCryptoResult OEMCrypto_Initialize(void);

/*
 * OEMCrypto_Terminate
 *
 * Description:
 *   Closes the crypto operation and releases all related resources.
 *
 * Parameters:
 *   None
 *
 * Returns:
 *   OEMCrypto_SUCCESS success
 *   OEMCrypto_ERROR_TERMINATE_FAILED failed to de-initialize crypto hardware
 *
 * Threading:
 *   This is an "Initialization and Termination Function" and will not be
 *   called simultaneously with any other function, as if the CDM holds a write
 *   lock on the OEMCrypto system. No other functions will be called before the
 *   system is re-initialized.
 *
 * Version:
 *   This method is supported by all API versions.
 */
OEMCryptoResult OEMCrypto_Terminate(void);

/*
 * OEMCrypto_OpenSession
 *
 * Description:
 *   Open a new crypto security engine context. The security engine hardware
 *   and firmware shall acquire resources that are needed to support the
 *   session, and return a session handle that identifies that session in
 *   future calls.
 *
 * Parameters:
 *   [out] session: an opaque handle that the crypto firmware uses to identify
 *         the session.
 *
 * Returns:
 *   OEMCrypto_SUCCESS success
 *   OEMCrypto_ERROR_TOO_MANY_SESSIONS failed because too many sessions are open
 *   OEMCrypto_ERROR_OPEN_SESSION_FAILED there is a resource issue or the
 *         security engine is not properly initialized.
 *   OEMCrypto_ERROR_SYSTEM_INVALIDATED
 *
 * Threading:
 *   This is a "Session Initialization Function" and will not be called
 *   simultaneously with any other function, as if the CDM holds a write lock
 *   on the OEMCrypto system.
 *
 * Version:
 *   This method changed in API version 5.
 */
OEMCryptoResult OEMCrypto_OpenSession(OEMCrypto_SESSION* session);

/*
 * OEMCrypto_CloseSession
 *
 * Description:
 *   Closes the crypto security engine session and frees any associated
 *   resources. If this session is associated with a Usage Entry, all resident
 *   memory associated with it will be freed. It is the CDM layer's
 *   responsibility to call OEMCrypto_UpdateUsageEntry before closing the
 *   session.
 *
 * Parameters:
 *   [in] session: handle for the session to be closed.
 *
 * Returns:
 *   OEMCrypto_SUCCESS success
 *   OEMCrypto_ERROR_INVALID_SESSION no open session with that id.
 *   OEMCrypto_ERROR_CLOSE_SESSION_FAILED illegal/unrecognized handle or the
 *         security engine is not properly initialized.
 *   OEMCrypto_ERROR_SYSTEM_INVALIDATED
 *
 * Threading:
 *   This is a "Session Initialization Function" and will not be called
 *   simultaneously with any other function, as if the CDM holds a write lock
 *   on the OEMCrypto system.
 *
 * Version:
 *   This method changed in API version 13.
 */
OEMCryptoResult OEMCrypto_CloseSession(OEMCrypto_SESSION session);

/*
 * OEMCrypto_GenerateDerivedKeys
 *
 * Description:
 *   Generates three secondary keys, mac_key[server], mac_key[client],  and
 *   encrypt_key, for handling signing and content key decryption under the
 *   license server protocol for CENC.
 *
 *   Refer to the Key Derivation section above for more details. This function
 *   computes the AES-128-CMAC of the enc_key_context and stores it in secure
 *   memory as the encrypt_key. It then computes four cycles of AES-128-CMAC of
 *   the mac_key_context and stores it in the mac_keys -- the first two cycles
 *   generate the mac_key[server] and the second two cycles generate the
 *   mac_key[client]. These two keys will be stored until the next call to
 *   OEMCrypto_LoadKeys(). The device key from the keybox is used as the key
 *   for the AES-128-CMAC.
 *
 * Parameters:
 *   [in] session: handle for the session to be used.
 *   [in] mac_key_context: pointer to memory containing context data for
 *         computing the HMAC generation key.
 *   [in] mac_key_context_length: length of the HMAC key context data, in bytes.
 *   [in] enc_key_context: pointer to memory containing context data for
 *         computing the encryption key.
 *   [in] enc_key_context_length: length of the encryption key context data, in
 *         bytes.
 *
 * Results:
 *   mac_key[server]: the 256 bit mac key is generated and stored in secure
 *   memory.
 *   mac_key[client]: the 256 bit mac key is generated and stored in secure
 *   memory.
 *   enc_key: the 128 bit encryption key is generated and stored in secure
 *   memory.
 *
 * Returns:
 *   OEMCrypto_SUCCESS success
 *   OEMCrypto_ERROR_NO_DEVICE_KEY
 *   OEMCrypto_ERROR_INVALID_SESSION
 *   OEMCrypto_ERROR_INVALID_CONTEXT
 *   OEMCrypto_ERROR_INSUFFICIENT_RESOURCES
 *   OEMCrypto_ERROR_UNKNOWN_FAILURE
 *   OEMCrypto_ERROR_BUFFER_TOO_LARGE
 *   OEMCrypto_ERROR_SESSION_LOST_STATE
 *   OEMCrypto_ERROR_SYSTEM_INVALIDATED
 *
 * Buffer Sizes:
 *   OEMCrypto shall support mac_key_context and enc_key_context sizes of at
 *   least 8 KiB.
 *   OEMCrypto shall return OEMCrypto_ERROR_BUFFER_TOO_LARGE if the buffers are
 *   too large.
 *
 * Threading:
 *   This is a "Session Function" and may be called simultaneously with session
 *   functions for other sessions but not simultaneously with other functions
 *   for this session. It will not be called simultaneously with initialization
 *   or usage table functions. It is as if the CDM holds a write lock for this
 *   session, and a read lock on the OEMCrypto system.
 *
 * Version:
 *   This method changed in API version 12.
 */
OEMCryptoResult OEMCrypto_GenerateDerivedKeys(OEMCrypto_SESSION session,
                                              const uint8_t* mac_key_context,
                                              uint32_t mac_key_context_length,
                                              const uint8_t* enc_key_context,
                                              uint32_t enc_key_context_length);

/*
 * OEMCrypto_DeriveKeysFromSessionKey
 *
 * Description:
 *   Generates three secondary keys, mac_key[server], mac_key[client] and
 *   encrypt_key, for handling signing and content key decryption under the
 *   license server protocol for CENC.
 *
 *   This function is similar to OEMCrypto_GenerateDerivedKeys, except that it
 *   uses a session key to generate the secondary keys instead of the Widevine
 *   Keybox device key. These three keys will be stored in secure memory until
 *   the next call to LoadKeys. The session key is passed in encrypted by the
 *   device RSA public key, and must be decrypted with the RSA private key
 *   before use.
 *
 *   Once the enc_key and mac_keys have been generated, all calls to LoadKeys
 *   and RefreshKeys proceed in the same manner for license requests using RSA
 *   or using a Widevine keybox token.
 *
 * Verification:
 *   If the RSA key's allowed_schemes is not kSign_RSASSA_PSS, then no keys are
 *   derived and the error OEMCrypto_ERROR_INVALID_RSA_KEY is returned. An RSA
 *   key cannot be used for both deriving session keys and also for PKCS1
 *   signatures.
 *
 * Parameters:
 *   [in] session: handle for the session to be used.
 *   [in] enc_session_key: session key, encrypted with the public RSA key (from
 *         the DRM certifcate) using RSA-OAEP.
 *   n_key_l[in] enc_sessioength: length of session_key, in bytes.
 *   [in] mac_key_context: pointer to memory containing context data for
 *         computing the HMAC generation key.
 *   [in] mac_key_context_length: length of the HMAC key context data, in bytes.
 *   [in] enc_key_context: pointer to memory containing context data for
 *         computing the encryption key.
 *   [in] enc_key_context_length: length of the encryption key context data, in
 *         bytes.
 *
 * Results:
 *   mac_key[server]: the 256 bit mac key is generated and stored in secure
 *   memory.
 *   mac_key[client]: the 256 bit mac key is generated and stored in secure
 *   memory.
 *   enc_key: the 128 bit encryption key is generated and stored in secure
 *   memory.
 *
 * Returns:
 *   OEMCrypto_SUCCESS success
 *   OEMCrypto_ERROR_DEVICE_NOT_RSA_PROVISIONED
 *   OEMCrypto_ERROR_INVALID_SESSION
 *   OEMCrypto_ERROR_INVALID_CONTEXT
 *   OEMCrypto_ERROR_INSUFFICIENT_RESOURCES
 *   OEMCrypto_ERROR_UNKNOWN_FAILURE
 *   OEMCrypto_ERROR_BUFFER_TOO_LARGE
 *   OEMCrypto_ERROR_SESSION_LOST_STATE
 *   OEMCrypto_ERROR_SYSTEM_INVALIDATED
 *
 * Buffer Sizes:
 *   OEMCrypto shall support mac_key_context and enc_key_context sizes of at
 *   least 8 KiB.
 *   OEMCrypto shall return OEMCrypto_ERROR_BUFFER_TOO_LARGE if the buffers are
 *   too large.
 *
 * Threading:
 *   This is a "Session Function" and may be called simultaneously with session
 *   functions for other sessions but not simultaneously with other functions
 *   for this session. It will not be called simultaneously with initialization
 *   or usage table functions. It is as if the CDM holds a write lock for this
 *   session, and a read lock on the OEMCrypto system.
 *
 * Version:
 *   This method changed in API version 12.
 */
OEMCryptoResult OEMCrypto_DeriveKeysFromSessionKey(
    OEMCrypto_SESSION session, const uint8_t* enc_session_key,
    size_t enc_session_key_length, const uint8_t* mac_key_context,
    size_t mac_key_context_length, const uint8_t* enc_key_context,
    size_t enc_key_context_length);

/*
 * OEMCrypto_GenerateNonce
 *
 * Description:
 *   Generates a 32-bit nonce to detect possible replay attack on the key
 *   control block. The nonce is stored in secure memory and will be used for
 *   the next call to LoadKeys.
 *
 *   Because the nonce will be used to prevent replay attacks, it is desirable
 *   that a rogue application cannot rapidly call this function until a
 *   repeated nonce is created randomly. With this in mind, if more than 20
 *   nonces are requested within one second, OEMCrypto will return an error
 *   after the 20th and not generate any more nonces for the rest of the
 *   second. After an error, if the application waits at least one second
 *   before requesting more nonces, then OEMCrypto will reset the error
 *   condition and generate valid nonces again.
 *
 *   To prevent Birthday Paradox attacks, OEMCrypto shall verify that the value
 *   generated is not in this session's nonce table, and that it is not in the
 *   nonce table of any other session.
 *
 * Parameters:
 *   [in] session: handle for the session to be used.
 *   [out] nonce: pointer to memory to receive the computed nonce.
 *
 * Results:
 *   nonce: the nonce is also stored in secure memory. Each session should
 *   store 4 nonces.
 *
 * Returns:
 *   OEMCrypto_SUCCESS success
 *   OEMCrypto_ERROR_INVALID_SESSION
 *   OEMCrypto_ERROR_INSUFFICIENT_RESOURCES
 *   OEMCrypto_ERROR_UNKNOWN_FAILURE
 *   OEMCrypto_ERROR_SESSION_LOST_STATE
 *   OEMCrypto_ERROR_SYSTEM_INVALIDATED
 *
 * Threading:
 *   This is a "Session Initialization Function" and will not be called
 *   simultaneously with any other function, as if the CDM holds a write lock
 *   on the OEMCrypto system.
 *
 * Version:
 *   This method changed in API version 5.
 */
OEMCryptoResult OEMCrypto_GenerateNonce(OEMCrypto_SESSION session,
                                        uint32_t* nonce);

/*
 * OEMCrypto_GenerateSignature
 *
 * Description:
 *   Generates a HMAC-SHA256 signature using the mac_key[client] for license
 *   request signing under the license server protocol for CENC.
 *
 *   The key used for signing should be the mac_key[client] that was generated
 *   for this session or loaded for this session by the most recent successful
 *   call to any one of
 *
 *     - OEMCrypto_GenerateDerivedKeys,
 *     - OEMCrypto_DeriveKeysFromSessionKey,
 *     - OEMCrypto_LoadKeys, or
 *     - OEMCrypto_LoadUsageEntry.
 *   Refer to the Signing Messages Sent to a Server section above for more
 *   details.
 *
 *   If a usage entry has been loaded, but keys have not been loaded through
 *   OEMCrypto_LoadKeys, then the derived mac keys and the keys in the usage
 *   entry may be different. In this case, the mac keys specified in the usage
 *   entry should be used.
 *
 *   NOTE: if signature pointer is null and/or input signature_length set to
 *   zero, this function returns OEMCrypto_ERROR_SHORT_BUFFER and sets output
 *   signature_length to the size needed to receive the output signature.
 *
 * Parameters:
 *   [in] session: crypto session identifier.
 *   [in] message: pointer to memory containing message to be signed.
 *   [in] message_length: length of the message, in bytes.
 *   [out] signature: pointer to memory to received the computed signature. May
 *         be null (see note above).
 *   [in/out] signature_length: (in) length of the signature buffer, in bytes.
 *   (out) actual length of the signature, in bytes.
 *
 * Returns:
 *   OEMCrypto_SUCCESS success
 *   OEMCrypto_ERROR_INVALID_SESSION
 *   OEMCrypto_ERROR_SHORT_BUFFER if signature buffer is not large enough to
 *         hold the signature.
 *   OEMCrypto_ERROR_INSUFFICIENT_RESOURCES
 *   OEMCrypto_ERROR_UNKNOWN_FAILURE
 *   OEMCrypto_ERROR_BUFFER_TOO_LARGE
 *   OEMCrypto_ERROR_SESSION_LOST_STATE
 *   OEMCrypto_ERROR_SYSTEM_INVALIDATED
 *
 * Buffer Sizes:
 *   OEMCrypto shall support message sizes of at least 8 KiB.
 *   OEMCrypto shall return OEMCrypto_ERROR_BUFFER_TOO_LARGE if the buffer is
 *   larger than the supported size.
 *
 * Threading:
 *   This is a "Session Function" and may be called simultaneously with session
 *   functions for other sessions but not simultaneously with other functions
 *   for this session. It will not be called simultaneously with initialization
 *   or usage table functions. It is as if the CDM holds a write lock for this
 *   session, and a read lock on the OEMCrypto system.
 *
 * Version:
 *   This method changed in API version 12.
 */
OEMCryptoResult OEMCrypto_GenerateSignature(OEMCrypto_SESSION session,
                                            const uint8_t* message,
                                            size_t message_length,
                                            uint8_t* signature,
                                            size_t* signature_length);

/*
 * OEMCrypto_LoadSRM
 *
 * Description:
 *   Verify and install a new SRM file. The device shall install the new file
 *   only if verification passes. If verification fails, the existing SRM will
 *   be left in place. Verification is defined by DCP, and includes
 *   verification of the SRM's signature and verification that the SRM version
 *   number will not be decreased. See the section HDCP SRM Update above for
 *   more details about the SRM. This function is for devices that support HDCP
 *   v2.2 or higher and wish to receive 4k content.
 *
 * Parameters:
 *   [in] bufer: buffer containing the SRM
 *   [in] buffer_length: length of the SRM, in bytes.
 *
 * Returns:
 *   OEMCrypto_SUCCESS - if the file was valid and was installed.
 *   OEMCrypto_ERROR_INVALID_CONTEXT - if the SRM version is too low, or the
 *         file is corrupted.
 *   OEMCrypto_ERROR_SIGNATURE_FAILURE - If the signature is invalid.
 *   OEMCrypto_ERROR_BUFFER_TOO_LARGE - if the buffer is too large for the
 *         device.
 *   OEMCrypto_ERROR_NOT_IMPLEMENTED
 *   OEMCrypto_ERROR_SYSTEM_INVALIDATED
 *
 * Buffer Sizes:
 *   The size of the buffer is determined by the HDCP specification.
 *
 * Threading:
 *   This is an "Initialization and Termination Function" and will not be
 *   called simultaneously with any other function, as if the CDM holds a write
 *   lock on the OEMCrypto system.
 *
 * Version:
 *   This method changed in API version 13.
 */
OEMCryptoResult OEMCrypto_LoadSRM(const uint8_t* buffer, size_t buffer_length);

/*
 * OEMCrypto_LoadKeys
 *
 * Description:
 *   Installs a set of keys for performing decryption in the current session.
 *
 *   The relevant fields have been extracted from the License Response protocol
 *   message, but the entire message and associated signature are provided so
 *   the message can be verified (using HMAC-SHA256 with the derived
 *   mac_key[server]). If the signature verification fails, ignore all other
 *   arguments and return OEMCrypto_ERROR_SIGNATURE_FAILURE. Otherwise, add the
 *   keys to the session context.
 *
 *   The keys will be decrypted using the current encrypt_key (AES-128-CBC) and
 *   the IV given in the KeyObject. Each key control block will be decrypted
 *   using the first 128 bits of the corresponding content key (AES-128-CBC)
 *   and the IV given in the KeyObject.
 *
 *   If its length is not zero, enc_mac_keys will be used to create new
 *   mac_keys. After all keys have been decrypted and validated, the new
 *   mac_keys are decrypted with the current encrypt_key and the offered IV.
 *   The new mac_keys replaces the current mac_keys for future calls to
 *   OEMCrypto_RefreshKeys(). The first 256 bits of the mac_keys become the
 *   mac_key[server] and the following 256 bits of the mac_keys become the
 *   mac_key[client]. If enc_mac_keys is null, then there will not be a call to
 *   OEMCrypto_RefreshKeys for this session and the current mac_keys should
 *   remain unchanged.
 *
 *   The mac_key and encrypt_key were generated and stored by the previous call
 *   to OEMCrypto_GenerateDerivedKeys() or
 *   OEMCrypto_DeriveKeysFromSessionKey(). The nonce was generated and stored
 *   by the previous call to OEMCrypto_GenerateNonce().
 *
 *   This session's elapsed time clock is started at 0. The clock will be used
 *   in OEMCrypto_DecryptCENC().
 *
 *   NOTE: The calling software must have previously established the mac_keys
 *   and encrypt_key with a call to OEMCrypto_GenerateDerivedKeys(),
 *   OEMCrypto_DeriveKeysFromSessionKey(), or a previous call to
 *   OEMCrypto_LoadKeys().
 *
 *   Refer to the Verification of Messages from a Server section above for more
 *   details.
 *
 *   If the parameter license_type is OEMCrypto_ContentLicense, then the fields
 *   key_id and key_data in an OEMCrypto_KeyObject are loaded in to the
 *   content_key_id and content_key_data fields of the key table entry. In this
 *   case, entitlement key ids and entitlement key data is left blank.
 *
 *   If the parameter license_type is OEMCrypto_EntitlementLicense,  then the
 *   fields key_id and key_data in an OEMCrypto_KeyObject are loaded in to the
 *   entitlement_key_id and entitlement_key_data fields of the key table entry.
 *   In this case, content key ids and content key data will be loaded later
 *   with a call to OEMCrypto_LoadEntitledContentKeys().
 *
 *   OEMCrypto may assume that the key_id_length is at most 16. However,
 *   OEMCrypto shall correctly handle key id lengths from 1 to 16 bytes.
 *
 *   OEMCrypto shall handle at least 20 keys per session. This allows a single
 *   license to contain separate keys for 3 key rotations (previous interval,
 *   current interval, next interval) times 4 content keys (audio, SD, HD, UHD)
 *   plus up to 8 keys for watermarks.
 *
 * Verification:
 *   The following checks should be performed. If any check fails, an error is
 *   returned, and none of the keys are loaded.
 *
 *     1. The signature of the message shall be computed, and the API shall
 *        verify the computed signature matches the signature passed in.  If
 *        not, return OEMCrypto_ERROR_SIGNATURE_FAILURE.  The signature
 *        verification shall use a constant-time algorithm (a signature
 *        mismatch will always take the same time as a successful comparison).
 *     2. The enc_mac_keys substring must either have zero length, or satisfy
 *        the range check. I.e.  (offset < message_length) && (offset + length
 *        < message_length) && (offset < offset+length),and offset+length does
 *        not cause an integer overflow. If it does not have zero length, then
 *        enc_mac_keys_iv must not have zero length, and must also satisfy the
 *        range check.  If not, return OEMCrypto_ERROR_INVALID_CONTEXT.  If the
 *        length is zero, then OEMCrypto may assume that the offset is also
 *        zero.
 *     3. The API shall verify that each substring in each KeyObject points to
 *        a location in the message.  I.e.  (offset < message_length) &&
 *        (offset + length < message_length) && (offset < offset+length) and
 *        offset+length does not cause an integer overflow, for each of key_id,
 *        key_data_iv, key_data, key_control_iv, key_control. If not, return
 *        OEMCrypto_ERROR_INVALID_CONTEXT.
 *     4. Each key's control block, after decryption, shall have a valid
 *        verification field. If not, return OEMCrypto_ERROR_INVALID_CONTEXT.
 *     5. If any key control block has the Nonce_Enabled bit set, that key's
 *        Nonce field shall match a nonce in the cache.  If not, return
 *        OEMCrypto_ERROR_INVALID_NONCE.  If there is a match, remove that
 *        nonce from the cache.  Note that all the key control blocks in a
 *        particular call shall have the same nonce value.
 *     6. If any key control block has the Require_AntiRollback_Hardware bit
 *        set, and the device does not protect the usage table from rollback,
 *        then do not load the keys and return OEMCrypto_ERROR_UNKNOWN_FAILURE.
 *     7. If the key control block has a nonzero Replay_Control, then the
 *        verification described below is also performed.
 *     8. If the key control block has the bit SRMVersionRequired is set, then
 *        the verification described below is also performed.  If the SRM
 *        requirement is not met, then the key control block's HDCP_Version
 *        will be changed to 0xF - local display only.
 *     9. If num_keys == 0, then return OEMCrypto_ERROR_INVALID_CONTEXT.
 *     10. If any key control block has the Shared_License bit set, and this
 *        call to LoadKeys is not replacing keys loaded from a previous call to
 *        LoadKeys, then the keys are not loaded, and the error
 *        OEMCrypto_ERROR_MISSING_MASTER is returned. This feature is obsolete,
 *        and no longer used by production license servers.  OEMCrypto unit
 *        tests for this feature have been removed.
 *     11. If this session is associated with a usage table entry, and that
 *        entry is marked as "inactive" (either kInactiveUsed or
 *        kInactiveUnused), then the keys are not loaded, and the error
 *        OEMCrypto_ERROR_LICENSE_INACTIVE is returned.
 *     12. The data in enc_mac_keys_iv is not identical to the 16 bytes before
 *        enc_mac_keys.  If it is, return OEMCrypto_ERROR_INVALID_CONTEXT.
 *
 *   Usage Table and Provider Session Token (pst)
 *
 *   If a key control block has a nonzero value for Replay_Control, then all
 *   keys in this license will have the same value for Replay_Control. In this
 *   case, the following additional checks are performed.
 *
 *     - The substring pst must have nonzero length and must satisfy the range
 *        check described above.  If not, return
 *        OEMCrypto_ERROR_INVALID_CONTEXT.
 *     - The session must be associated with a usage table entry, either
 *        created via OEMCrypto_CreateNewUsageEntry or loaded via
 *        OEMCrypto_LoadUsageEntry.
 *     - If Replay_Control is 1 = Nonce_Required, then OEMCrypto will perform a
 *        nonce check as described above.   OEMCrypto will verify that the
 *        usage entry is newly created with OEMCrypto_CreateNewUsageEntry.  If
 *        an existing entry was reloaded, an error
 *        OEMCrypto_ERROR_INVALID_CONTEXT is returned and no keys are loaded.
 *        OEMCrypto will then copy the pst and the mac keys to the usage entry,
 *        and set the status to Unused. This Replay_Control prevents the
 *        license from being loaded more than once, and will be used for online
 *        streaming.
 *     - If Replay_Control is 2 = "Require existing Session Usage table entry
 *        or Nonce",  then OEMCrypto will behave slightly differently on the
 *        first call to LoadKeys for this license.
 *          * If the usage entry was created with OEMCrypto_CreateNewUsageEntry
 *             for this session, then OEMCrypto will verify the nonce for each
 *             key. OEMCrypto will copy the pst and mac keys to the usage
 *             entry.  The license received time of the entry will be updated
 *             to the current time, and the status will be set to Unused.
 *          * If the usage entry was loaded with OEMCrypto_LoadUsageEntry for
 *             this session, then OEMCrypto will NOT verify the nonce for each
 *             key.  Instead, it will verify that the pst passed in matches
 *             that in the entry.  Also, the entry's mac keys will be verified
 *             against the current session's mac keys.  This allows an offline
 *             license to be reloaded but maintain continuity of the playback
 *             times from one session to the next.
 *          * If the nonce is not valid and a usage entry was not loaded, the
 *             return error is OEMCrypto_ERROR_INVALID_NONCE.
 *          * If the loaded usage entry has a pst that does not match,
 *             OEMCrypto returns the error OEMCrypto_ERROR_WRONG_PST.
 *          * If the loaded usage entry has mac keys that do not match the
 *             license, OEMCrypto returns the error OEMCrypto_ERROR_WRONG_KEYS.
 *   Note: If LoadKeys updates the mac keys, then the new updated mac keys will
 *   be used with the Usage Entry --   i.e. the new keys are stored in the
 *   usage table when creating a new entry, or the new keys are verified
 *   against those in the usage table if there is an existing entry. If
 *   LoadKeys does not update the mac keys, the existing session mac keys are
 *   used.
 *
 *   Sessions that are associated with an entry will need to be able to update
 *   and verify the status of the entry, and the time stamps in the entry.
 *
 *   Devices that do not support the Usage Table will return
 *   OEMCrypto_ERROR_INVALID_CONTEXT if the Replay_Control is nonzero.
 *
 *   SRM Restriction Data
 *
 *   If any key control block has the flag SRMVersionRequired set, then the
 *   following verification is also performed.
 *
 *     1. The substring srm_restriction_data must have nonzero length and must
 *        satisfy the range check described above.  If not, return
 *        OEMCrypto_ERROR_INVALID_CONTEXT.
 *     2. The first 8 bytes of srm_restriction_data must match the string
 *        "HDCPDATA".  If not, return OEMCrypto_ERROR_INVALID_CONTEXT.
 *     3. The next 4 bytes of srm_restriction_data will be converted from
 *        network byte order.  If the current SRM installed on the device has a
 *        version number less than this, then the SRM requirement is not met.
 *        If the device does not support SRM files, or OEMCrypto cannot
 *        determine the current SRM version number, then the SRM requirement is
 *        not met.
 *   Note: if the current SRM version requirement is not met, LoadKeys will
 *   still succeed and the keys will be loaded. However, those keys with the
 *   SRMVersionRequired bit set will have their HDCP_Version increased to 0xF -
 *   local display only. Any future call to SelectKey for these keys while
 *   there is an external display will return OEMCrypto_ERROR_INSUFFICIENT_HDCP
 *   at that time.
 *
 * Parameters:
 *   [in] session: crypto session identifier.
 *   [in] message: pointer to memory containing message to be verified.
 *   [in] message_length: length of the message, in bytes.
 *   [in] signature: pointer to memory containing the signature.
 *   [in] signature_length: length of the signature, in bytes.
 *   [in] enc_mac_key_iv: IV for decrypting new mac_key. Size is 128 bits.
 *   [in] enc_mac_keys: encrypted mac_keys for generating new mac_keys. Size is
 *         512 bits.
 *   [in] num_keys: number of keys present.
 *   [in] key_array: set of keys to be installed.
 *   [in] pst: the Provider Session Token.
 *   [in] srm_restriction_data: optional data specifying the minimum SRM
 *         version.
 *   [in] license_type: specifies if the license contains content keys or
 *         entitlement keys.
 *
 * Returns:
 *   OEMCrypto_SUCCESS success
 *   OEMCrypto_ERROR_NO_DEVICE_KEY
 *   OEMCrypto_ERROR_INVALID_SESSION
 *   OEMCrypto_ERROR_UNKNOWN_FAILURE
 *   OEMCrypto_ERROR_INVALID_CONTEXT
 *   OEMCrypto_ERROR_SIGNATURE_FAILURE
 *   OEMCrypto_ERROR_INVALID_NONCE
 *   OEMCrypto_ERROR_TOO_MANY_KEYS
 *   OEMCrypto_ERROR_NOT_IMPLEMENTED
 *   OEMCrypto_ERROR_BUFFER_TOO_LARGE
 *   OEMCrypto_ERROR_SESSION_LOST_STATE
 *   OEMCrypto_ERROR_SYSTEM_INVALIDATED
 *
 * Buffer Sizes:
 *   OEMCrypto shall support message sizes of at least 8 KiB.
 *   OEMCrypto shall return OEMCrypto_ERROR_BUFFER_TOO_LARGE if the buffer is
 *   larger than the supported size.
 *
 * Threading:
 *   This is a "Session Function" and may be called simultaneously with session
 *   functions for other sessions but not simultaneously with other functions
 *   for this session. It will not be called simultaneously with initialization
 *   or usage table functions. It is as if the CDM holds a write lock for this
 *   session, and a read lock on the OEMCrypto system.
 *
 * Version:
 *   This method changed in API version 14.
 */
OEMCryptoResult OEMCrypto_LoadKeys(
    OEMCrypto_SESSION session, const uint8_t* message, size_t message_length,
    const uint8_t* signature, size_t signature_length,
    OEMCrypto_Substring enc_mac_keys_iv, OEMCrypto_Substring enc_mac_keys,
    size_t num_keys, const OEMCrypto_KeyObject* key_array,
    OEMCrypto_Substring pst, OEMCrypto_Substring srm_restriction_data,
    OEMCrypto_LicenseType license_type);

/*
 * OEMCrypto_LoadEntitledContentKeys
 *
 * Description:
 *   Load content keys into a session which already has entitlement keys
 *   loaded. This function will only be called for a session after a call to
 *   OEMCrypto_LoadKeys with the parameter type license_type equal to
 *   OEMCrypto_EntitlementLicense. This function may be called multiple times
 *   for the same session.
 *
 *   If the session does not have license_type equal to
 *   OEMCrypto_EntitlementLicense, return OEMCrypto_ERROR_INVALID_CONTEXT and
 *   perform no work.
 *
 *   For each key object in key_array, OEMCrypto shall look up the entry in the
 *   key table with the corresponding entitlement_key_id.
 *
 *     1. If no entry is found, return OEMCrypto_KEY_NOT_ENTITLED.
 *     2. If the entry already has a content_key_id and content_key_data, that
 *        id and data are erased.
 *     3. The content_key_id from the key_array is copied to the entry's
 *        content_key_id.
 *     4. The content_key_data decrypted using the entitlement_key_data as a
 *        key for AES-256-CBC with an IV of content_key_data_iv.  Wrapped
 *        content is padded using PKCS#7 padding. Notice that the entitlement
 *        key will be an AES 256 bit key.  The clear content key data will be
 *        stored in the entry's content_key_data.
 *   Entries in the key table that do not correspond to anything in the
 *   key_array are not modified or removed.
 *
 *   For devices that use a hardware key ladder, it may be more convenient to
 *   store the encrypted content key data in the key table, and decrypt it when
 *   the function SelectKey is called.
 *
 * Parameters:
 *   [in] session: handle for the session to be used.
 *   [in] num_keys: number of keys present.
 *   [in] key_array: set of key updates.
 *
 * Returns:
 *   OEMCrypto_SUCCESS success
 *   OEMCrypto_ERROR_INVALID_SESSION
 *   OEMCrypto_ERROR_INVALID_CONTEXT
 *   OEMCrypto_ERROR_INSUFFICIENT_RESOURCES
 *   OEMCrypto_ERROR_UNKNOWN_FAILURE
 *   OEMCrypto_KEY_NOT_ENTITLED
 *   OEMCrypto_ERROR_SESSION_LOST_STATE
 *   OEMCrypto_ERROR_SYSTEM_INVALIDATED
 *
 * Threading:
 *   This is a "Session Function" and may be called simultaneously with session
 *   functions for other sessions but not simultaneously with other functions
 *   for this session. It will not be called simultaneously with initialization
 *   or usage table functions. It is as if the CDM holds a write lock for this
 *   session, and a read lock on the OEMCrypto system.
 *
 * Version:
 *   This method is new in API version 14.
 */
OEMCryptoResult OEMCrypto_LoadEntitledContentKeys(
    OEMCrypto_SESSION session, const uint8_t* message, size_t message_length,
    size_t num_keys, const OEMCrypto_EntitledContentKeyObject* key_array);

/*
 * OEMCrypto_RefreshKeys
 *
 * Description:
 *   Updates an existing set of keys for continuing decryption in the current
 *   session.
 *
 *   The relevant fields have been extracted from the Renewal Response protocol
 *   message, but the entire message and associated signature are provided so
 *   the message can be verified (using HMAC-SHA256 with the current
 *   mac_key[server]). If any verification step fails, an error is returned.
 *   Otherwise, the key table in trusted memory is updated using the
 *   key_control block. When updating an entry in the table, only the duration,
 *   nonce, and nonce_enabled fields are used. All other key control bits are
 *   not modified.
 *
 *   NOTE: OEMCrypto_LoadKeys() must be called first to load the keys into the
 *   session.
 *
 *   This session's elapsed time clock is reset to 0 when this function is
 *   called. The elapsed time clock is used in OEMCrypto_DecryptCENC() and the
 *   other Decryption API functions to determine if the key has expired.
 *
 *   This function does not add keys to the key table. It is only used to
 *   update a key control block license duration. This function is used to
 *   update the duration of a key, only. It is not used to update key control
 *   bits.
 *
 *   If the KeyRefreshObject's key_control_iv has zero length, then the
 *   key_control is not encrypted. If the key_control_iv is specified, then
 *   key_control is encrypted with the first 128 bits of the corresponding
 *   content key.
 *
 *   If the KeyRefreshObject's key_id has zero length, then this refresh object
 *   should be used to update the duration of all keys for the current session.
 *   In this case, key_control_iv will also have zero length and the control
 *   block will not be encrypted.
 *
 *   If the session's license_type is OEMCrypto_ContentLicense, and the
 *   KeyRefreshObject's key_id is not null, then the entry in the keytable with
 *   the matching content_key_id is updated.
 *
 *   If the session's license_type is OEMCrypto_EntitlementLicense, and the
 *   KeyRefreshObject's key_id is not null, then the entry in the keytable with
 *   the matching entitlment_key_id is updated.
 *
 *   If the key_id is not null, and no matching entry is found in the key
 *   table, then return OEMCrypto_ERROR_NO_CONTENT_KEY.
 *
 *   Aside from the key's duration, no other values in the key control block
 *   should be updated by this function.
 *
 * Verification:
 *   The following checks should be performed. If any check fails, an error is
 *   returned, and none of the keys are loaded.
 *     1. The signature of the message shall be computed using mac_key[server],
 *        and the API shall verify the computed signature matches the signature
 *        passed in.  If not, return OEMCrypto_ERROR_SIGNATURE_FAILURE. The
 *        signature verification shall use a constant-time algorithm (a
 *        signature mismatch will always take the same time as a successful
 *        comparison).
 *     2. The API shall verify that each substring in each KeyObject has zero
 *        length or satisfies the range check described in the discussion of
 *        OEMCrypto_LoadKeys. If not, return OEMCrypto_ERROR_INVALID_CONTEXT.
 *     3. Each key's control block shall have a valid verification field. If
 *        not, return OEMCrypto_ERROR_INVALID_CONTEXT.
 *     4. If the key control block has the Nonce_Enabled bit set, the Nonce
 *        field shall match one of the nonces in the cache.  If not, return
 *        OEMCrypto_ERROR_INVALID_NONCE. If there is a match, remove that nonce
 *        from the cache.   Note that all the key control blocks in a
 *        particular call shall have the same nonce value.
 *     5. If a key ID is specified, and that key has not been loaded into this
 *        session, return OEMCrypto_ERROR_NO_CONTENT_KEY.
 *
 * Parameters:
 *   [in] session: handle for the session to be used.
 *   [in] message: pointer to memory containing message to be verified.
 *   [in] message_length: length of the message, in bytes.
 *   [in] signature: pointer to memory containing the signature.
 *   [in] signature_length: length of the signature, in bytes.
 *   [in] num_keys: number of keys present.
 *   [in] key_array: set of key updates.
 *
 * Returns:
 *   OEMCrypto_SUCCESS success
 *   OEMCrypto_ERROR_NO_DEVICE_KEY
 *   OEMCrypto_ERROR_INVALID_SESSION
 *   OEMCrypto_ERROR_INVALID_CONTEXT
 *   OEMCrypto_ERROR_SIGNATURE_FAILURE
 *   OEMCrypto_ERROR_INVALID_NONCE
 *   OEMCrypto_ERROR_INSUFFICIENT_RESOURCES
 *   OEMCrypto_ERROR_UNKNOWN_FAILURE
 *   OEMCrypto_ERROR_BUFFER_TOO_LARGE
 *   OEMCrypto_ERROR_NO_CONTENT_KEY
 *   OEMCrypto_ERROR_SESSION_LOST_STATE
 *   OEMCrypto_ERROR_SYSTEM_INVALIDATED
 *
 * Buffer Sizes:
 *   OEMCrypto shall support message sizes of at least 8 KiB.
 *   OEMCrypto shall return OEMCrypto_ERROR_BUFFER_TOO_LARGE if the buffer is
 *   larger than the supported size.
 *
 * Threading:
 *   This is a "Session Function" and may be called simultaneously with session
 *   functions for other sessions but not simultaneously with other functions
 *   for this session. It will not be called simultaneously with initialization
 *   or usage table functions. It is as if the CDM holds a write lock for this
 *   session, and a read lock on the OEMCrypto system.
 *
 * Version:
 *   This method changed in API version 12.
 */
OEMCryptoResult OEMCrypto_RefreshKeys(
    OEMCrypto_SESSION session, const uint8_t* message, size_t message_length,
    const uint8_t* signature, size_t signature_length, size_t num_keys,
    const OEMCrypto_KeyRefreshObject* key_array);

/*
 * OEMCrypto_QueryKeyControl
 *
 * Description:
 *   Returns the decrypted key control block for the given content_key_id. This
 *   function is for application developers to debug license server and key
 *   timelines. It only returns a key control block if LoadKeys was successful,
 *   otherwise it returns OEMCrypto_ERROR_NO_CONTENT_KEY. The developer of the
 *   OEMCrypto library must be careful that the keys themselves are not
 *   accidentally revealed.
 *
 *   Note: returns control block in original, network byte order. If OEMCrypto
 *   converts fields to host byte order internally for storage, it should
 *   convert them back. Since OEMCrypto might not store the nonce or validation
 *   fields, values of 0 may be used instead.
 *
 * Verification:
 *   The following checks should be performed.
 *     1. If key_id is null, return OEMCrypto_ERROR_INVALID_CONTEXT.
 *     2. If key_control_block_length is null, return
 *        OEMCrypto_ERROR_INVALID_CONTEXT.
 *     3. If *key_control_block_length is less than the length of a key control
 *        block, set it to the correct value, and return
 *        OEMCrypto_ERROR_SHORT_BUFFER.
 *     4. If key_control_block is null, return OEMCrypto_ERROR_INVALID_CONTEXT.
 *     5. If the specified key has not been loaded, return
 *        OEMCrypto_ERROR_NO_CONTENT_KEY.
 *
 * Parameters:
 *   [in] content_key_id: The unique id of the key of interest.
 *   [in] content_key_id_length: The length of key_id, in bytes. From 1 to 16,
 *         inclusive.
 *   [out] key_control_block: A caller-owned buffer.
 *   [in/out] key_control_block_length. The length of key_control_block buffer.
 *
 * Returns:
 *   OEMCrypto_SUCCESS
 *   OEMCrypto_ERROR_INVALID_CONTEXT
 *   OEMCrypto_ERROR_INSUFFICIENT_RESOURCES
 *   OEMCrypto_ERROR_UNKNOWN_FAILURE
 *   OEMCrypto_ERROR_SESSION_LOST_STATE
 *   OEMCrypto_ERROR_SYSTEM_INVALIDATED
 *
 * Threading:
 *   This is a "Session Function" and may be called simultaneously with session
 *   functions for other sessions but not simultaneously with other functions
 *   for this session. It will not be called simultaneously with initialization
 *   or usage table functions. It is as if the CDM holds a write lock for this
 *   session, and a read lock on the OEMCrypto system.
 *
 * Version:
 *   This method is new in API version 10.
 */
OEMCryptoResult OEMCrypto_QueryKeyControl(OEMCrypto_SESSION session,
                                          const uint8_t* content_key_id,
                                          size_t content_key_id_length,
                                          uint8_t* key_control_block,
                                          size_t* key_control_block_length);

/*
 * OEMCrypto_SelectKey
 *
 * Description:
 *   Select a content key and install it in the hardware key ladder for
 *   subsequent decryption operations (OEMCrypto_DecryptCENC()) for this
 *   session. The specified key must have been previously "installed" via
 *   OEMCrypto_LoadKeys() or OEMCrypto_RefreshKeys().
 *
 *   A key control block is associated with the key and the session, and is
 *   used to configure the session context. The Key Control data is documented
 *   in "Key Control Block Definition".
 *
 *   Step 1: Lookup the content key data via the offered key_id. The key data
 *   includes the key value, and the key control block.
 *
 *   Step 2: Latch the content key into the hardware key ladder. Set permission
 *   flags and timers based on the key's control block.
 *
 *   Step 3: use the latched content key to decrypt (AES-128-CTR or
 *   AES-128-CBC) buffers passed in via OEMCrypto_DecryptCENC(). If the key is
 *   256 bits it will be used for OEMCrypto_Generic_Sign or
 *   OEMCrypto_Generic_Verify as specified in the key control block. If the key
 *   will be used for OEMCrypto_Generic_Encrypt or OEMCrypto_Generic_Decrypt
 *   then the cipher mode will always be OEMCrypto_CipherMode_CBC. Continue to
 *   use this key for this session until OEMCrypto_SelectKey() is called again,
 *   or until OEMCrypto_CloseSession() is called.
 *
 * Verification:
 *     1. If the key id is not found in the keytable for this session, then the
 *        key state is not changed and OEMCrypto shall return
 *        OEMCrypto_ERROR_NO_CONTENT_KEY.
 *     2. If the current key's control block has a nonzero Duration field, then
 *        the API shall verify that the duration is greater than the session's
 *        elapsed time clock before the key is used.  OEMCrypto may return
 *        OEMCrypto_ERROR_KEY_EXPIRED from OEMCrypto_SelectKey, or SelectKey
 *        may return success from select key and the decrypt or generic crypto
 *        call will return OEMCrypto_ERROR_KEY_EXPIRED.
 *     3. If the key control block has the bit Disable_Analog_Output set, then
 *        the device should disable analog video output.  If the device has
 *        analog video output that cannot be disabled, then the key is not
 *        selected, and OEMCrypto_ERROR_ANALOG_OUTPUT is returned.
 *     4. If the key control block has HDCP required, and the device cannot
 *        enforce HDCP, then the key is not selected, and
 *        OEMCrypto_ERROR_INSUFFICIENT_HDCP is returned.
 *     5. If the key control block has a nonzero value for HDCP_Version, and
 *        the device cannot enforce at least that version of HDCP, then the key
 *        is not selected, and OEMCrypto_ERROR_INSUFFICIENT_HDCP is returned.
 *
 * Parameters:
 *   [in]  session: crypto session identifier.
 *   [in]  content_key_id: pointer to the content Key ID.
 *   [in]  content_key_id_length: length of the content Key ID, in bytes. From
 *         1 to 16, inclusive.
 *   [in]  cipher_mode: whether the key should be prepared for CTR mode or CBC
 *         mode when used in later calls to DecryptCENC. This should be ignored
 *         when the key is used for Generic Crypto calls.
 *
 * Returns:
 *   OEMCrypto_SUCCESS success
 *   OEMCrypto_ERROR_KEY_EXPIRED - if the key's timer has expired
 *   OEMCrypto_ERROR_INVALID_SESSION crypto session ID invalid or not open
 *   OEMCrypto_ERROR_NO_DEVICE_KEY failed to decrypt device key
 *   OEMCrypto_ERROR_NO_CONTENT_KEY failed to decrypt content key
 *   OEMCrypto_ERROR_CONTROL_INVALID invalid or unsupported control input
 *   OEMCrypto_ERROR_KEYBOX_INVALID cannot decrypt and read from Keybox
 *   OEMCrypto_ERROR_INSUFFICIENT_RESOURCES
 *   OEMCrypto_ERROR_UNKNOWN_FAILURE
 *   OEMCrypto_ERROR_KEY_EXPIRED
 *   OEMCrypto_ERROR_ANALOG_OUTPUT
 *   OEMCrypto_ERROR_INSUFFICIENT_HDCP
 *   OEMCrypto_ERROR_NO_CONTENT_KEY
 *   OEMCrypto_ERROR_SESSION_LOST_STATE
 *   OEMCrypto_ERROR_SYSTEM_INVALIDATED
 *
 * Threading:
 *   This is a "Session Function" and may be called simultaneously with session
 *   functions for other sessions but not simultaneously with other functions
 *   for this session. It will not be called simultaneously with initialization
 *   or usage table functions. It is as if the CDM holds a write lock for this
 *   session, and a read lock on the OEMCrypto system.
 *
 * Version:
 *   This method changed in API version 14.
 */
OEMCryptoResult OEMCrypto_SelectKey(OEMCrypto_SESSION session,
                                    const uint8_t* content_key_id,
                                    size_t content_key_id_length,
                                    OEMCryptoCipherMode cipher_mode);

/*
 * OEMCrypto_DecryptCENC
 *
 * Description:
 *   Decrypts or copies the payload in the buffer referenced by the *data_addr
 *   parameter into the buffer referenced by the out_buffer parameter, using
 *   the session context indicated by the session parameter. Decryption mode is
 *   AES-128-CTR or AES-128-CBC depending on the value of cipher_mode passed in
 *   to OEMCrypto_SelectKey. If is_encrypted is true, the content key
 *   associated with the session is latched in the active hardware key ladder
 *   and is used for the decryption operation. If is_encrypted is false, the
 *   data is simply copied.
 *
 *   After decryption, the data_length bytes are copied to the location
 *   described by out_buffer. This could be one of
 *
 *     1. The structure out_buffer contains a pointer to a clear text buffer.
 *        The OEMCrypto library shall verify that key control allows data to be
 *        returned in clear text.  If it is not authorized, this method should
 *        return an error.
 *     2. The structure out_buffer contains a handle to a secure buffer.
 *     3. The structure out_buffer indicates that the data should be sent
 *        directly to the decoder and renderer.
 *   NOTES:
 *
 *   For CTR mode, IV points to the counter value to be used for the initial
 *   encrypted block of the input buffer. The IV length is the AES block size.
 *   For subsequent encrypted AES blocks the IV is calculated by incrementing
 *   the lower 64 bits (byte 8-15) of the IV value used for the previous block.
 *   The counter rolls over to zero when it reaches its maximum value
 *   (0xFFFFFFFFFFFFFFFF). The upper 64 bits (byte 0-7) of the IV do not change.
 *
 *   For CBC mode, IV points to the initial vector for cipher block chaining.
 *   Within each subsample, OEMCrypto is responsible for updating the IV as
 *   prescribed by CBC mode. The calling layer above is responsible for
 *   updating the IV from one subsample to the next if needed.
 *
 *   This method may be called several times before the decrypted data is used.
 *   For this reason, the parameter subsample_flags may be used to optimize
 *   decryption. The first buffer in a chunk of data will have the
 *   OEMCrypto_FirstSubsample bit set in subsample_flags. The last buffer in a
 *   chunk of data will have the OEMCrypto_LastSubsample bit set in
 *   subsample_flags. The decrypted data will not be used until after
 *   OEMCrypto_LastSubsample has been set. If an implementation decrypts data
 *   immediately, it may ignore subsample_flags.
 *
 *   If the destination buffer is secure, an offset may be specified.
 *   DecryptCENC begins storing data out_buffer->secure.offset bytes after the
 *   beginning of the secure buffer.
 *
 *   If the session has an entry in the Usage Table, then OEMCrypto will update
 *   the time_of_last_decrypt. If the status of the entry is "unused", then
 *   change the status to "active" and set the time_of_first_decrypt.
 *
 *   The decryption mode, either OEMCrypto_CipherMode_CTR or
 *   OEMCrypto_CipherMode_CBC, was specified in the call to
 *   OEMCrypto_SelectKey. The encryption pattern is specified by the fields in
 *   the parameter pattern. A description of partial encryption patterns can be
 *   found in the document Draft International Standard ISO/IEC DIS 23001-7.
 *   Search for the codes "cenc", "cbc1", "cens" or "cbcs".
 *
 *   The most common mode is "cenc", which is OEMCrypto_CipherMode_CTR without
 *   a pattern. The entire subsample is either encrypted or clear, depending on
 *   the flag is_encrypted. In the structure pattern, both encrypt and skip
 *   will be 0. This is the only mode that allows for a nonzero block_offset.
 *
 *   A less common mode is "cens", which is OEMCrypto_CipherMode_CTR with an
 *   encryption pattern. For this mode, OEMCrypto may assume that an encrypted
 *   subsample will have a length that is a multiple of 16, the AES block
 *   length.
 *
 *   The mode "cbc1" is OEMCrypto_CipherMode_CBC without a pattern. In the
 *   structure pattern, both encrypt and skip will be 0. If an encrypted
 *   subsample has a length that is not a multiple of 16, the final partial
 *   block will be in the clear.
 *
 *   The mode "cbcs" is OEMCrypto_CipherMode_CBC with an encryption pattern.
 *   This mode allows devices to decrypt HLS content. If an encrypted subsample
 *   has a length that is not a multiple of 16, the final partial block will be
 *   in the clear. In practice, the most common pattern is (1, 9), or 1
 *   encrypted block followed by 9 clear blocks. The ISO-CENC spec implicitly
 *   limits both the skip and encrypt values to be 4 bits, so a value of at
 *   most 15.
 *
 *   A sample may be broken up into a mix of clear and encrypted subsamples. In
 *   order to support the VP9 standard, the breakup of a subsample into clear
 *   and encrypted subsamples is not always in pairs.
 *
 *   If OEMCrypto assembles all of the subsamples into a single buffer and then
 *   decrypts, it can assume that the block offset is 0.
 *
 * Verification:
 *   The following checks should be performed if is_encrypted is true. If any
 *   check fails, an error is returned, and no decryption is performed.
 *     1. If the current key's control block has a nonzero Duration field, then
 *        the API shall verify that the duration is greater than the session's
 *        elapsed time clock. If not, return OEMCrypto_ERROR_KEY_EXPIRED.
 *     2. If the current key's control block has the Data_Path_Type bit set,
 *        then the API shall verify that the output buffer is secure or direct.
 *        If not, return OEMCrypto_ERROR_DECRYPT_FAILED.
 *     3. If the current key control block has the bit Disable_Analog_Output
 *        set, then the device should disable analog video output.  If the
 *        device has analog video output that cannot be disabled, then
 *        OEMCrypto_ERROR_ANALOG_OUTPUT is returned.
 *     4. If the current key's control block has the HDCP bit set, then the API
 *        shall verify that the buffer will be displayed locally, or output
 *        externally using HDCP only.  If not, return
 *        OEMCrypto_ERROR_INSUFFICIENT_HDCP.
 *     5. If the current key's control block has a nonzero value for
 *        HDCP_Version, then the current version of HDCP for the device and the
 *        display combined will be compared against the version specified in
 *        the control block.  If the current version is not at least as high as
 *        that in the control block, then return
 *        OEMCrypto_ERROR_INSUFFICIENT_HDCP.
 *     6. If the current session has an entry in the Usage Table, and the
 *        status of that entry is either kInactiveUsed or kInactiveUnused, then
 *        return the error OEMCrypto_ERROR_LICENSE_INACTIVE.
 *     7. If a Decrypt Hash has been initialized via OEMCrypto_SetDecryptHash,
 *        and the current key's control block does not have the
 *        Allow_Hash_Verification bit set, then do not compute a hash and
 *        return OEMCrypto_ERROR_UNKNOWN_FAILURE.
 *   If the flag is_encrypted is false, then no verification is performed. This
 *   call shall copy clear data even when there are no keys loaded, or there is
 *   no selected key.
 *
 * Parameters:
 *   [in] session: crypto session identifier.
 *   [in] data_addr: An unaligned pointer to this segment of the stream.
 *   [in] data_length: The length of this segment of the stream, in bytes.
 *   [in] is_encrypted: True if the buffer described by data_addr, data_length
 *         is encrypted. If is_encrypted is false, only the data_addr and
 *         data_length parameters are used. The iv and offset arguments are
 *         ignored.
 *   [in] iv: The initial value block to be used for content decryption.
 *   This is discussed further below.
 *   [in] block_offset: If non-zero, the decryption block boundary is different
 *         from the start of the data. block_offset should be subtracted from
 *         data_addr to compute the starting address of the first decrypted
 *         block. The bytes between the decryption block start address and
 *         data_addr are discarded after decryption. It does not adjust the
 *         beginning of the source or destination data. This parameter
 *         satisfies 0 <= block_offset < 16.
 *   [in] out_buffer: A caller-owned descriptor that specifies the handling of
 *         the decrypted byte stream. See OEMCrypto_DestbufferDesc for details.
 *   [in] pattern: A caller-owned structure indicating the encrypt/skip pattern
 *         as specified in the CENC standard.
 *   [in] subsample_flags: bitwise flags indicating if this is the first,
 *         middle, or last subsample in a chunk of data. 1 = first subsample, 2
 *         = last subsample, 3 = both first and last subsample, 0 = neither
 *         first nor last subsample.
 *
 * Returns:
 *   OEMCrypto_SUCCESS
 *   OEMCrypto_ERROR_NO_DEVICE_KEY
 *   OEMCrypto_ERROR_INVALID_SESSION
 *   OEMCrypto_ERROR_INVALID_CONTEXT
 *   OEMCrypto_ERROR_DECRYPT_FAILED
 *   OEMCrypto_ERROR_KEY_EXPIRED
 *   OEMCrypto_ERROR_INSUFFICIENT_HDCP
 *   OEMCrypto_ERROR_ANALOG_OUTPUT
 *   OEMCrypto_ERROR_INSUFFICIENT_RESOURCES
 *   OEMCrypto_ERROR_UNKNOWN_FAILURE
 *   OEMCrypto_ERROR_BUFFER_TOO_LARGE
 *   OEMCrypto_ERROR_OUTPUT_TOO_LARGE
 *   OEMCrypto_ERROR_SESSION_LOST_STATE
 *   OEMCrypto_ERROR_SYSTEM_INVALIDATED
 *
 * Buffer Sizes:
 *   OEMCrypto shall support subsample sizes (i.e. data_length) of at least 100
 *   KiB.
 *   OEMCrypto shall return OEMCrypto_ERROR_BUFFER_TOO_LARGE if the buffer is
 *   larger than the supported size. If OEMCrypto returns
 *   OEMCrypto_ERROR_BUFFER_TOO_LARGE, the calling function must break the
 *   buffer into smaller chunks. For high performance devices, OEMCrypto should
 *   handle larger buffers. We encourage OEMCrypto implementers not to
 *   artificially restrict the maximum buffer size.
 *   If OEMCrypto detects that the output data is too large, and breaking the
 *   buffer into smaller subsamples will not work, then it returns
 *   OEMCrypto_ERROR_OUTPUT_TOO_LARGE. This error will bubble up to the
 *   application, which can decide to skip the current frame of video or to
 *   switch to a lower resolution.
 *
 * Threading:
 *   This is a "Session Function" and may be called simultaneously with session
 *   functions for other sessions but not simultaneously with other functions
 *   for this session. It will not be called simultaneously with initialization
 *   or usage table functions. It is as if the CDM holds a write lock for this
 *   session, and a read lock on the OEMCrypto system.
 *
 * Version:
 *   This method changed in API version 15. This method changed its name in API
 *   version 11.
 */
OEMCryptoResult OEMCrypto_DecryptCENC(
    OEMCrypto_SESSION session, const uint8_t* data_addr, size_t data_length,
    bool is_encrypted, const uint8_t* iv,
    size_t block_offset,  // used for CTR "cenc" mode only.
    OEMCrypto_DestBufferDesc* out_buffer,
    const OEMCrypto_CENCEncryptPatternDesc* pattern, uint8_t subsample_flags);

/*
 * OEMCrypto_CopyBuffer
 *
 * Description:
 *   Copies the payload in the buffer referenced by the *data_addr parameter
 *   into the buffer referenced by the out_buffer parameter. The data is simply
 *   copied. The definition of OEMCrypto_DestBufferDesc and subsample_flags are
 *   the same as in OEMCrypto_DecryptCENC, above.
 *
 *   The main difference between this and DecryptCENC is that this function
 *   does not need an open session, and it may be called concurrently with
 *   other functions on a multithreaded system. In particular, an application
 *   will use this to copy the clear leader of a video to a secure buffer while
 *   the license request is being generated, sent to the server, and the
 *   response is being processed. This functionality is needed because an
 *   application may not have read or write access to a secure destination
 *   buffer.
 *
 *   NOTES:
 *
 *   This method may be called several times before the data is used. The first
 *   buffer in a chunk of data will have the OEMCrypto_FirstSubsample bit set
 *   in subsample_flags. The last buffer in a chunk of data will have the
 *   OEMCrypto_LastSubsample bit set in subsample_flags. The data will not be
 *   used until after OEMCrypto_LastSubsample has been set. If an
 *   implementation copies data immediately, it may ignore subsample_flags.
 *
 *   If the destination buffer is secure, an offset may be specified.
 *   CopyBuffer begins storing data out_buffer->secure.offset bytes after the
 *   beginning of the secure buffer.
 *
 * Verification:
 *   The following checks should be performed.
 *     1. If either data_addr or out_buffer is null, return
 *        OEMCrypto_ERROR_INVALID_CONTEXT.
 *
 * Parameters:
 *   [in] session: crypto session identifier.
 *   [in] data_addr: An unaligned pointer to the buffer to be copied.
 *   [in] data_length: The length of the buffer, in bytes.
 *   [in] out_buffer: A caller-owned descriptor that specifies the handling of
 *         the byte stream. See OEMCrypto_DestbufferDesc for details.
 *   [in] subsample_flags: bitwise flags indicating if this is the first,
 *         middle, or last subsample in a chunk of data. 1 = first subsample, 2
 *         = last subsample, 3 = both first and last subsample, 0 = neither
 *         first nor last subsample.
 *
 * Returns:
 *   OEMCrypto_SUCCESS
 *   OEMCrypto_ERROR_INVALID_CONTEXT
 *   OEMCrypto_ERROR_INSUFFICIENT_RESOURCES
 *   OEMCrypto_ERROR_UNKNOWN_FAILURE
 *   OEMCrypto_ERROR_BUFFER_TOO_LARGE
 *   OEMCrypto_ERROR_OUTPUT_TOO_LARGE
 *   OEMCrypto_ERROR_SESSION_LOST_STATE
 *   OEMCrypto_ERROR_SYSTEM_INVALIDATED
 *
 * Buffer Sizes:
 *   OEMCrypto shall support subsample sizes (i.e. data_length) up to 100 KiB.
 *   OEMCrypto shall return OEMCrypto_ERROR_BUFFER_TOO_LARGE if the buffer is
 *   larger than the supported size. If OEMCrypto returns
 *   OEMCrypto_ERROR_BUFFER_TOO_LARGE, the calling function must break the
 *   buffer into smaller chunks. For high performance devices, OEMCrypto should
 *   handle larger buffers. We encourage OEMCrypto implementers not to
 *   artificially restrict the maximum buffer size.
 *   If OEMCrypto detects that the output data is too large, and breaking the
 *   buffer into smaller subsamples will not work, then it returns
 *   OEMCrypto_ERROR_OUTPUT_TOO_LARGE. This error will bubble up to the
 *   application, which can decide to skip the current frame of video or to
 *   switch to a lower resolution.
 *
 * Threading:
 *   This is a "Session Function" and may be called simultaneously with session
 *   functions for other sessions but not simultaneously with other functions
 *   for this session. It will not be called simultaneously with initialization
 *   or usage table functions. It is as if the CDM holds a write lock for this
 *   session, and a read lock on the OEMCrypto system.
 *
 * Version:
 *   This method is changed in API version 15.
 */
OEMCryptoResult OEMCrypto_CopyBuffer(OEMCrypto_SESSION session,
                                     const uint8_t* data_addr,
                                     size_t data_length,
                                     OEMCrypto_DestBufferDesc* out_buffer,
                                     uint8_t subsample_flags);

/*
 * OEMCrypto_Generic_Encrypt
 *
 * Description:
 *   This function encrypts a generic buffer of data using the current key.
 *
 *   If the session has an entry in the Usage Table, then OEMCrypto will update
 *   the time_of_last_decrypt. If the status of the entry is "unused", then
 *   change the status to "active" and set the time_of_first_decrypt.
 *
 *   OEMCrypto should be able to handle buffers at least 100 KiB long.
 *
 * Verification:
 *   The following checks should be performed. If any check fails, an error is
 *   returned, and the data is not encrypted.
 *     1. The control bit for the current key shall have the Allow_Encrypt set.
 *        If not, return OEMCrypto_ERROR_UNKNOWN_FAILURE.
 *     2. If the current key's control block has a nonzero Duration field, then
 *        the API shall verify that the duration is greater than the session's
 *        elapsed time clock. If not, return OEMCrypto_ERROR_KEY_EXPIRED.
 *     3. If the current session has an entry in the Usage Table, and the
 *        status of that entry is either kInactiveUsed or kInactiveUnused, then
 *        return the error OEMCrypto_ERROR_LICENSE_INACTIVE.
 *
 * Parameters:
 *   [in] session: crypto session identifier.
 *   [in] in_buffer: pointer to memory containing data to be encrypted.
 *   [in] buffer_length: length of the buffer, in bytes. The algorithm may
 *         restrict buffer_length to be a multiple of block size.
 *   [in] iv: IV for encrypting data. Size is 128 bits.
 *   [in] algorithm: Specifies which encryption algorithm to use. Currently,
 *         only CBC 128 mode is allowed for encryption.
 *   [out] out_buffer: pointer to buffer in which encrypted data should be
 *         stored.
 *
 * Returns:
 *   OEMCrypto_SUCCESS success
 *   OEMCrypto_ERROR_KEY_EXPIRED
 *   OEMCrypto_ERROR_NO_DEVICE_KEY
 *   OEMCrypto_ERROR_INVALID_SESSION
 *   OEMCrypto_ERROR_INSUFFICIENT_RESOURCES
 *   OEMCrypto_ERROR_UNKNOWN_FAILURE
 *   OEMCrypto_ERROR_BUFFER_TOO_LARGE
 *   OEMCrypto_ERROR_SESSION_LOST_STATE
 *   OEMCrypto_ERROR_SYSTEM_INVALIDATED
 *
 * Buffer Sizes:
 *   OEMCrypto shall support  buffers sizes of at least 100 KiB for generic
 *   crypto operations.
 *   OEMCrypto shall return OEMCrypto_ERROR_BUFFER_TOO_LARGE if the buffer is
 *   larger than the supported size.
 *
 * Threading:
 *   This is a "Session Function" and may be called simultaneously with session
 *   functions for other sessions but not simultaneously with other functions
 *   for this session. It will not be called simultaneously with initialization
 *   or usage table functions. It is as if the CDM holds a write lock for this
 *   session, and a read lock on the OEMCrypto system.
 *
 * Version:
 *   This method changed in API version 12.
 */
OEMCryptoResult OEMCrypto_Generic_Encrypt(
    OEMCrypto_SESSION session, const uint8_t* in_buffer, size_t buffer_length,
    const uint8_t* iv, OEMCrypto_Algorithm algorithm, uint8_t* out_buffer);

/*
 * OEMCrypto_Generic_Decrypt
 *
 * Description:
 *   This function decrypts a generic buffer of data using the current key.
 *
 *   If the session has an entry in the Usage Table, then OEMCrypto will update
 *   the time_of_last_decrypt. If the status of the entry is "unused", then
 *   change the status to "active" and set the time_of_first_decrypt.
 *
 *   OEMCrypto should be able to handle buffers at least 100 KiB long.
 *
 * Verification:
 *   The following checks should be performed. If any check fails, an error is
 *   returned, and the data is not decrypted.
 *     1. The control bit for the current key shall have the Allow_Decrypt set.
 *        If not, return OEMCrypto_ERROR_DECRYPT_FAILED.
 *     2. If the current key's control block has the Data_Path_Type bit set,
 *        then return OEMCrypto_ERROR_DECRYPT_FAILED.
 *     3. If the current key's control block has a nonzero Duration field, then
 *        the API shall verify that the duration is greater than the session's
 *        elapsed time clock. If not, return OEMCrypto_ERROR_KEY_EXPIRED.
 *     4. If the current session has an entry in the Usage Table, and the
 *        status of that entry is either kInactiveUsed or kInactiveUnused, then
 *        return the error OEMCrypto_ERROR_LICENSE_INACTIVE.
 *
 * Parameters:
 *   [in] session: crypto session identifier.
 *   [in] in_buffer: pointer to memory containing data to be encrypted.
 *   [in] buffer_length: length of the buffer, in bytes. The algorithm may
 *         restrict buffer_length to be a multiple of block size.
 *   [in] iv: IV for encrypting data. Size is 128 bits.
 *   [in] algorithm: Specifies which encryption algorithm to use. Currently,
 *         only CBC 128 mode is allowed for decryption.
 *   [out] out_buffer: pointer to buffer in which decrypted data should be
 *         stored.
 *
 * Returns:
 *   OEMCrypto_SUCCESS success
 *   OEMCrypto_ERROR_KEY_EXPIRED
 *   OEMCrypto_ERROR_DECRYPT_FAILED
 *   OEMCrypto_ERROR_NO_DEVICE_KEY
 *   OEMCrypto_ERROR_INVALID_SESSION
 *   OEMCrypto_ERROR_INSUFFICIENT_RESOURCES
 *   OEMCrypto_ERROR_UNKNOWN_FAILURE
 *   OEMCrypto_ERROR_BUFFER_TOO_LARGE
 *   OEMCrypto_ERROR_SESSION_LOST_STATE
 *   OEMCrypto_ERROR_SYSTEM_INVALIDATED
 *
 * Buffer Sizes:
 *   OEMCrypto shall support  buffers sizes of at least 100 KiB for generic
 *   crypto operations.
 *   OEMCrypto shall return OEMCrypto_ERROR_BUFFER_TOO_LARGE if the buffer is
 *   larger than the supported size.
 *
 * Threading:
 *   This is a "Session Function" and may be called simultaneously with session
 *   functions for other sessions but not simultaneously with other functions
 *   for this session. It will not be called simultaneously with initialization
 *   or usage table functions. It is as if the CDM holds a write lock for this
 *   session, and a read lock on the OEMCrypto system.
 *
 * Version:
 *   This method changed in API version 12.
 */
OEMCryptoResult OEMCrypto_Generic_Decrypt(
    OEMCrypto_SESSION session, const uint8_t* in_buffer, size_t buffer_length,
    const uint8_t* iv, OEMCrypto_Algorithm algorithm, uint8_t* out_buffer);

/*
 * OEMCrypto_Generic_Sign
 *
 * Description:
 *   This function signs a generic buffer of data using the current key.
 *
 *   If the session has an entry in the Usage Table, then OEMCrypto will update
 *   the time_of_last_decrypt. If the status of the entry is "unused", then
 *   change the status to "active" and set the time_of_first_decrypt.
 *
 * Verification:
 *   The following checks should be performed. If any check fails, an error is
 *   returned, and the data is not signed.
 *     1. The control bit for the current key shall have the Allow_Sign set.
 *     2. If the current key's control block has a nonzero Duration field, then
 *        the API shall verify that the duration is greater than the session's
 *        elapsed time clock. If not, return OEMCrypto_ERROR_KEY_EXPIRED.
 *     3. If the current session has an entry in the Usage Table, and the
 *        status of that entry is either kInactiveUsed or kInactiveUnused, then
 *        return the error OEMCrypto_ERROR_LICENSE_INACTIVE.
 *
 * Parameters:
 *   [in] session: crypto session identifier.
 *   [in] in_buffer: pointer to memory containing data to be encrypted.
 *   [in] buffer_length: length of the buffer, in bytes.
 *   [in] algorithm: Specifies which algorithm to use.
 *   [out] signature: pointer to buffer in which signature should be stored.
 *         May be null on the first call in order to find required buffer size.
 *   [in/out] signature_length: (in) length of the signature buffer, in bytes.
 *   (out) actual length of the signature
 *
 * Returns:
 *   OEMCrypto_SUCCESS success
 *   OEMCrypto_ERROR_KEY_EXPIRED
 *   OEMCrypto_ERROR_SHORT_BUFFER if signature buffer is not large enough to
 *         hold the output signature.
 *   OEMCrypto_ERROR_NO_DEVICE_KEY
 *   OEMCrypto_ERROR_INVALID_SESSION
 *   OEMCrypto_ERROR_INSUFFICIENT_RESOURCES
 *   OEMCrypto_ERROR_UNKNOWN_FAILURE
 *   OEMCrypto_ERROR_BUFFER_TOO_LARGE
 *   OEMCrypto_ERROR_SESSION_LOST_STATE
 *   OEMCrypto_ERROR_SYSTEM_INVALIDATED
 *
 * Buffer Sizes:
 *   OEMCrypto shall support  buffers sizes of at least 100 KiB for generic
 *   crypto operations.
 *   OEMCrypto shall return OEMCrypto_ERROR_BUFFER_TOO_LARGE if the buffer is
 *   larger than the supported size.
 *
 * Threading:
 *   This is a "Session Function" and may be called simultaneously with session
 *   functions for other sessions but not simultaneously with other functions
 *   for this session. It will not be called simultaneously with initialization
 *   or usage table functions. It is as if the CDM holds a write lock for this
 *   session, and a read lock on the OEMCrypto system.
 *
 * Version:
 *   This method changed in API version 14.
 */
OEMCryptoResult OEMCrypto_Generic_Sign(OEMCrypto_SESSION session,
                                       const uint8_t* in_buffer,
                                       size_t buffer_length,
                                       OEMCrypto_Algorithm algorithm,
                                       uint8_t* signature,
                                       size_t* signature_length);

/*
 * OEMCrypto_Generic_Verify
 *
 * Description:
 *   This function verifies the signature of a generic buffer of data using the
 *   current key.
 *
 *   If the session has an entry in the Usage Table, then OEMCrypto will update
 *   the time_of_last_decrypt. If the status of the entry is "unused", then
 *   change the status to "active" and set the time_of_first_decrypt.
 *
 * Verification:
 *   The following checks should be performed. If any check fails, an error is
 *   returned.
 *     1. The control bit for the current key shall have the Allow_Verify set.
 *     2. The signature of the message shall be computed, and the API shall
 *        verify the computed signature matches the signature passed in.  If
 *        not, return OEMCrypto_ERROR_SIGNATURE_FAILURE.
 *     3. The signature verification shall use a constant-time algorithm (a
 *        signature mismatch will always take the same time as a successful
 *        comparison).
 *     4. If the current key's control block has a nonzero Duration field, then
 *        the API shall verify that the duration is greater than the session's
 *        elapsed time clock. If not, return OEMCrypto_ERROR_KEY_EXPIRED.
 *     5. If the current session has an entry in the Usage Table, and the
 *        status of that entry is either kInactiveUsed or kInactiveUnused, then
 *        return the error OEMCrypto_ERROR_LICENSE_INACTIVE.
 *
 * Parameters:
 *   [in] session: crypto session identifier.
 *   [in] in_buffer: pointer to memory containing data to be encrypted.
 *   [in] buffer_length: length of the buffer, in bytes.
 *   [in] algorithm: Specifies which algorithm to use.
 *   [in] signature: pointer to buffer in which signature resides.
 *   [in] signature_length: length of the signature buffer, in bytes.
 *
 * Returns:
 *   OEMCrypto_SUCCESS success
 *   OEMCrypto_ERROR_KEY_EXPIRED
 *   OEMCrypto_ERROR_SIGNATURE_FAILURE
 *   OEMCrypto_ERROR_NO_DEVICE_KEY
 *   OEMCrypto_ERROR_INVALID_SESSION
 *   OEMCrypto_ERROR_INSUFFICIENT_RESOURCES
 *   OEMCrypto_ERROR_UNKNOWN_FAILURE
 *   OEMCrypto_ERROR_BUFFER_TOO_LARGE
 *   OEMCrypto_ERROR_SESSION_LOST_STATE
 *   OEMCrypto_ERROR_SYSTEM_INVALIDATED
 *
 * Buffer Sizes:
 *   OEMCrypto shall support  buffers sizes of at least 100 KiB for generic
 *   crypto operations.
 *   OEMCrypto shall return OEMCrypto_ERROR_BUFFER_TOO_LARGE if the buffer is
 *   larger than the supported size.
 *
 * Threading:
 *   This is a "Session Function" and may be called simultaneously with session
 *   functions for other sessions but not simultaneously with other functions
 *   for this session. It will not be called simultaneously with initialization
 *   or usage table functions. It is as if the CDM holds a write lock for this
 *   session, and a read lock on the OEMCrypto system.
 *
 * Version:
 *   This method changed in API version 14.
 */
OEMCryptoResult OEMCrypto_Generic_Verify(OEMCrypto_SESSION session,
                                         const uint8_t* in_buffer,
                                         size_t buffer_length,
                                         OEMCrypto_Algorithm algorithm,
                                         const uint8_t* signature,
                                         size_t signature_length);

/*
 * OEMCrypto_WrapKeyboxOrOEMCert
 *
 * Description:
 *   A device should be provisioned at the factory with either an OEM
 *   Certificate or a keybox. We will call this data the root of trust. During
 *   manufacturing, the root of trust should be encrypted with the OEM root key
 *   and stored on the file system in a region that will not be erased during
 *   factory reset. This function may be used by legacy systems that use the
 *   two-step WrapKeyboxOrOEMCert/InstallKeyboxOrOEMCert approach. When the
 *   Widevine DRM plugin initializes, it will look for a wrapped root of trust
 *   in the file /factory/wv.keys and install it into the security processor by
 *   calling OEMCrypto_InstallKeyboxOrOEMCert().
 *
 *   Figure 10. OEMCrypto_WrapKeyboxOrOEMCert Operation
 *
 *   OEMCrypto_WrapKeyboxOrOEMCert() is used to generate an OEM-encrypted root
 *   of trust that may be passed to OEMCrypto_InstallKeyboxOrOEMCert() for
 *   provisioning. The root of trust may be either passed in the clear or
 *   previously encrypted with a transport key. If a transport key is supplied,
 *   the keybox is first decrypted with the transport key before being wrapped
 *   with the OEM root key. This function is only needed if the root of trust
 *   provisioning method involves saving the keybox or OEM Certificate to the
 *   file system.
 *
 * Parameters:
 *   [in] rot  - pointer to root of trust data to encrypt -- this is either a
 *         keybox or an OEM Certificate private key. May be NULL on the first
 *         call to test size of wrapped keybox. The keybox may either be clear
 *         or previously encrypted.
 *   [in] rotLength - length the keybox data in bytes
 *   [out] wrappedRot – Pointer to wrapped keybox
 *   [out] wrappedRotLength – Pointer to the length of the wrapped rot in bytes
 *   [in] transportKey – Optional. AES transport key. If provided, the rot
 *         parameter was previously encrypted with this key. The keybox will be
 *         decrypted with the transport key using AES-CBC and a null IV.
 *   [in] transportKeyLength – Optional. Number of bytes in the transportKey,
 *         if used.
 *
 * Returns:
 *   OEMCrypto_SUCCESS success
 *   OEMCrypto_ERROR_WRITE_KEYBOX failed to encrypt the keybox
 *   OEMCrypto_ERROR_SHORT_BUFFER if keybox is provided as NULL, to determine
 *         the size of the wrapped keybox
 *   OEMCrypto_ERROR_INSUFFICIENT_RESOURCES
 *   OEMCrypto_ERROR_NOT_IMPLEMENTED
 *   OEMCrypto_ERROR_SYSTEM_INVALIDATED
 *
 * Threading:
 *   This is an "Initialization and Termination Function" and will not be
 *   called simultaneously with any other function, as if the CDM holds a write
 *   lock on the OEMCrypto system.
 *
 * Version:
 *   This method is supported in all API versions.
 */
OEMCryptoResult OEMCrypto_WrapKeyboxOrOEMCert(const uint8_t* rot,
                                              size_t rotLength,
                                              uint8_t* wrappedRot,
                                              size_t* wrappedRotLength,
                                              const uint8_t* transportKey,
                                              size_t transportKeyLength);

/*
 * OEMCrypto_InstallKeyboxOrOEMCert
 *
 * Description:
 *   Decrypts a wrapped root of trust and installs it in the security
 *   processor. The root of trust is unwrapped then encrypted with the OEM root
 *   key. This function is called from the Widevine DRM plugin at
 *   initialization time if there is no valid root of trust installed. It looks
 *   for wrapped data in the file /factory/wv.keys and if it is present, will
 *   read the file and call OEMCrypto_InstallKeyboxOrOEMCert() with the
 *   contents of the file. This function is only needed if the factory
 *   provisioning method involves saving the keybox or OEM Certificate to the
 *   file system.
 *
 * Parameters:
 *   [in] rot - pointer to encrypted data as input
 *   [in] rotLength - length of the data in bytes
 *
 * Returns:
 *   OEMCrypto_SUCCESS success
 *   OEMCrypto_ERROR_BAD_MAGIC
 *   OEMCrypto_ERROR_BAD_CRC
 *   OEMCrypto_ERROR_INSUFFICIENT_RESOURCES
 *   OEMCrypto_ERROR_NOT_IMPLEMENTED
 *   OEMCrypto_ERROR_SYSTEM_INVALIDATED
 *
 * Threading:
 *   This is an "Initialization and Termination Function" and will not be
 *   called simultaneously with any other function, as if the CDM holds a write
 *   lock on the OEMCrypto system.
 *
 * Version:
 *   This method is supported in all API versions.
 */
OEMCryptoResult OEMCrypto_InstallKeyboxOrOEMCert(const uint8_t* rot,
                                                 size_t rotLength);

/*
 * OEMCrypto_GetProvisioningMethod
 *
 * Description:
 *   This function is for OEMCrypto to tell the layer above what provisioning
 *   method it uses: keybox or OEM certificate.
 *
 * Parameters:
 *   none
 *
 * Returns:
 *     - DrmCertificate means the device has a DRM certificate built into the
 *        system.  This cannot be used by level 1 devices.  This provisioning
 *        method is deprecated and should not be used on new devices.
 *        OEMCertificate provisioning should be used instead.
 *     - Keybox means the device has a unique keybox.  For level 1 devices this
 *        keybox must be securely installed by the device manufacturer.
 *     - OEMCertificate means the device has a factory installed OEM
 *        certificate.  This is also called Provisioning 3.0.
 *     - ProvisioningError indicates a serious problem with the OEMCrypto
 *        library.
 *
 * Threading:
 *   This is a "Property Function" and may be called simultaneously with any
 *   other property function or session function, but not any initialization or
 *   usage table function, as if the CDM holds a read lock on the OEMCrypto
 *   system.
 *
 * Version:
 *   This method is new API version 12.
 */
OEMCrypto_ProvisioningMethod OEMCrypto_GetProvisioningMethod(void);

/*
 * OEMCrypto_IsKeyboxOrOEMCertValid
 *
 * Description:
 *   If the device has a keybox, this validates the Widevine Keybox loaded into
 *   the security processor device. This method verifies two fields in the
 *   keybox:
 *
 *     - Verify the MAGIC field contains a valid signature (such as,
 *        'k''b''o''x').
 *     - Compute the CRC using CRC-32-POSIX-1003.2 standard and compare the
 *        checksum to the CRC stored in the Keybox.
 *   The CRC is computed over the entire Keybox excluding the 4 bytes of the
 *   CRC (for example, Keybox[0..123]). For a description of the fields stored
 *   in the keybox, see Keybox Definition.
 *
 *   If the device has an OEM Certificate, this validates the certificate
 *   private key.
 *
 * Parameters:
 *   none
 *
 * Returns:
 *   OEMCrypto_SUCCESS
 *   OEMCrypto_ERROR_BAD_MAGIC
 *   OEMCrypto_ERROR_BAD_CRC
 *   OEMCrypto_ERROR_KEYBOX_INVALID
 *   OEMCrypto_ERROR_INVALID_RSA_KEY
 *   OEMCrypto_ERROR_SYSTEM_INVALIDATED
 *
 * Threading:
 *   This is a "Property Function" and may be called simultaneously with any
 *   other property function or session function, but not any initialization or
 *   usage table function, as if the CDM holds a read lock on the OEMCrypto
 *   system.
 *
 * Version:
 *   This method is supported in all API versions.
 */
OEMCryptoResult OEMCrypto_IsKeyboxOrOEMCertValid(void);

/*
 * OEMCrypto_GetDeviceID
 *
 * Description:
 *   Retrieve DeviceID from the Keybox. For devices that have an OEM
 *   Certificate instead of a keybox, this function may return
 *   OEMCrypto_ERROR_NOT_IMPLEMENTED. If the function is implemented on an OEM
 *   Certificate device, it should set the device ID to a device-unique string,
 *   such as the device serial number. The ID should be device-unique and it
 *   should be stable -- i.e. it should not change across a device reboot or a
 *   system upgrade.
 *
 *   This function is optional but recommended for Provisioning 3.0 in API v15.
 *   It may be required for a future version of this API.
 *
 * Parameters:
 *   [out] deviceId - pointer to the buffer that receives the Device ID
 *   [in/out] idLength – on input, size of the caller's device ID buffer. On
 *         output, the number of bytes written into the buffer.
 *
 * Returns:
 *   OEMCrypto_SUCCESS success
 *   OEMCrypto_ERROR_SHORT_BUFFER if the buffer is too small to return device ID
 *   OEMCrypto_ERROR_NO_DEVICEID failed to return Device Id
 *   OEMCrypto_ERROR_NOT_IMPLEMENTED
 *   OEMCrypto_ERROR_SYSTEM_INVALIDATED
 *
 * Threading:
 *   This is a "Property Function" and may be called simultaneously with any
 *   other property function or session function, but not any initialization or
 *   usage table function, as if the CDM holds a read lock on the OEMCrypto
 *   system.
 *
 * Version:
 *   This method is supported in all API versions.
 */
OEMCryptoResult OEMCrypto_GetDeviceID(uint8_t* deviceID, size_t* idLength);

/*
 * OEMCrypto_GetKeyData
 *
 * Description:
 *   Return the Key Data field from the Keybox.
 *
 * Parameters:
 *   [out] keyData  - pointer to the buffer to hold the Key Data field from the
 *         Keybox
 *   [in/out] keyDataLength – on input, the allocated buffer size. On output,
 *         the number of bytes in Key Data
 *
 * Returns:
 *   OEMCrypto_SUCCESS success
 *   OEMCrypto_ERROR_SHORT_BUFFER if the buffer is too small to return KeyData
 *   OEMCrypto_ERROR_NO_KEYDATA
 *   OEMCrypto_ERROR_NOT_IMPLEMENTED - this function is for Provisioning 2.0
 *         only.
 *   OEMCrypto_ERROR_SYSTEM_INVALIDATED
 *
 * Threading:
 *   This is a "Property Function" and may be called simultaneously with any
 *   other property function or session function, but not any initialization or
 *   usage table function, as if the CDM holds a read lock on the OEMCrypto
 *   system.
 *
 * Version:
 *   This method is supported in all API versions.
 */
OEMCryptoResult OEMCrypto_GetKeyData(uint8_t* keyData, size_t* keyDataLength);

/*
 * OEMCrypto_LoadTestKeybox
 *
 * Description:
 *   Temporarily use the specified test keybox until the next call to
 *   OEMCrypto_Terminate. This allows a standard suite of unit tests to be run
 *   on a production device without permanently changing the keybox. Using the
 *   test keybox is not persistent. OEMCrypto cannot assume that this keybox is
 *   the same as previous keyboxes used for testing.
 *
 *   Devices that use an OEM Certificate instead of a keybox (i.e. Provisioning
 *   3.0) do not need to support this functionality, and may return
 *   OEMCrypto_ERROR_NOT_IMPLEMENTED.
 *
 * Parameters:
 *   [in] buffer: pointer to memory containing test keybox, in binary form.
 *   [in] length: length of the buffer, in bytes.
 *
 * Returns:
 *   OEMCrypto_SUCCESS success
 *   OEMCrypto_ERROR_INSUFFICIENT_RESOURCES
 *   OEMCrypto_ERROR_NOT_IMPLEMENTED - this function is for Provisioning 2.0
 *         only.
 *   OEMCrypto_ERROR_SYSTEM_INVALIDATED
 *
 * Threading:
 *   This is an "Initialization and Termination Function" and will not be
 *   called simultaneously with any other function, as if the CDM holds a write
 *   lock on the OEMCrypto system. It is called after OEMCrypto_Initialize and
 *   after OEMCrypto_GetProvisioningMethod and only if the provisoining method
 *   is OEMCrypto_Keybox,
 *
 * Version:
 *   This method changed in API version 14.
 */
OEMCryptoResult OEMCrypto_LoadTestKeybox(const uint8_t* buffer, size_t length);

/*
 * OEMCrypto_GetOEMPublicCertificate
 *
 * Description:
 *   This function should place the OEM public certificate in the buffer
 *   public_cert. After a call to this function, all methods using an RSA key
 *   should use the OEM certificate's private RSA key. See the section above
 *   discussing Provisioning 3.0.
 *
 *   If the buffer is not large enough, OEMCrypto should update
 *   public_cert_length and return OEMCrypto_ERROR_SHORT_BUFFER.
 *
 * Parameters:
 *     - [in] session: this function affects the specified session only.
 *     - [out] public_cert: the buffer where the public certificate is stored.
 *     - [in/out] public_cert_length: on input, this is the available size of
 *        the buffer.  On output, this is the number of bytes needed for the
 *        certificate.
 *
 * Returns:
 *   OEMCrypto_SUCCESS
 *   OEMCrypto_ERROR_NOT_IMPLEMENTED - this function is for Provisioning 3.0
 *         only.
 *   OEMCrypto_ERROR_SHORT_BUFFER
 *   OEMCrypto_ERROR_SYSTEM_INVALIDATED
 *
 * Threading:
 *   This is a "Session Function" and may be called simultaneously with session
 *   functions for other sessions but not simultaneously with other functions
 *   for this session. It will not be called simultaneously with initialization
 *   or usage table functions. It is as if the CDM holds a write lock for this
 *   session, and a read lock on the OEMCrypto system.
 *
 * Version:
 *   This method is new API version 12.
 */
OEMCryptoResult OEMCrypto_GetOEMPublicCertificate(OEMCrypto_SESSION session,
                                                  uint8_t* public_cert,
                                                  size_t* public_cert_length);

/*
 * OEMCrypto_GetRandom
 *
 * Description:
 *   Returns a buffer filled with hardware-generated random bytes, if supported
 *   by the hardware. If the hardware feature does not exist, return
 *   OEMCrypto_ERROR_RNG_NOT_SUPPORTED.
 *
 * Parameters:
 *   [out] randomData - pointer to the buffer that receives random data
 *   [in] dataLength - length of the random data buffer in bytes
 *
 * Returns:
 *   OEMCrypto_SUCCESS success
 *   OEMCrypto_ERROR_RNG_FAILED failed to generate random number
 *   OEMCrypto_ERROR_RNG_NOT_SUPPORTED function not supported
 *   OEMCrypto_ERROR_BUFFER_TOO_LARGE
 *   OEMCrypto_ERROR_SYSTEM_INVALIDATED
 *
 * Buffer Sizes:
 *   OEMCrypto shall support  dataLength sizes of at least 32 bytes for random
 *   number generation.
 *   OEMCrypto shall return OEMCrypto_ERROR_BUFFER_TOO_LARGE if the buffer is
 *   larger than the supported size.
 *
 * Threading:
 *   This is a "Property Function" and may be called simultaneously with any
 *   other property function or session function, but not any initialization or
 *   usage table function, as if the CDM holds a read lock on the OEMCrypto
 *   system.
 *
 * Version:
 *   This method is supported in all API versions.
 */
OEMCryptoResult OEMCrypto_GetRandom(uint8_t* randomData, size_t dataLength);

/*
 * OEMCrypto_APIVersion
 *
 * Description:
 *   This function returns the current API version number. The version number
 *   allows the calling application to avoid version mis-match errors, because
 *   this API is part of a shared library.
 *
 *   There is a possibility that some API methods will be backwards compatible,
 *   or backwards compatible at a reduced security level.
 *
 *   There is no plan to introduce forward-compatibility. Applications will
 *   reject a library with a newer version of the API.
 *
 *   The version specified in this document is 15. Any OEM that returns this
 *   version number guarantees it passes all unit tests associated with this
 *   version.
 *
 * Parameters:
 *   none
 *
 * Returns:
 *   The supported API, as specified in the header file OEMCryptoCENC.h.
 *
 * Threading:
 *   This is a "Property Function" and may be called simultaneously with any
 *   other property function or session function, but not any initialization or
 *   usage table function, as if the CDM holds a read lock on the OEMCrypto
 *   system.
 *
 * Version:
 *   This method changed in each API version.
 */
uint32_t OEMCrypto_APIVersion(void);

/*
 * OEMCrypto_BuildInformation
 *
 * Description:
 *   Report the build information of the OEMCrypto library as a short null
 *   terminated C string. The string should be at most 128 characters long.
 *   This string should be updated with each release or OEMCrypto build.
 *
 *   Some SOC vendors deliver a binary OEMCrypto library to a device
 *   manufacturer. This means the OEMCrypto version may not be exactly in sync
 *   with the system's versions. This string can be used to help track which
 *   version is installed on a device.
 *
 *   It may be used for logging or bug tracking and may be bubbled up to the
 *   app so that it may track metrics on errors.
 *
 *   Since the OEMCrypto API also changes its minor version number when there
 *   are minor corrections, it would be useful to include the API version
 *   number in this string, e.g. "15.1" or "15.2" if those minor versions are
 *   released.
 *
 * Parameters:
 *   none
 *
 * Returns:
 *   A printable null terminated C string, suitable for a single line in a log.
 *
 * Threading:
 *   This is a "Property Function" and may be called simultaneously with any
 *   other property function or session function, but not any initialization or
 *   usage table function, as if the CDM holds a read lock on the OEMCrypto
 *   system.
 *
 * Version:
 *   This method changed in each API version.
 */
const char* OEMCrypto_BuildInformation(void);

/*
 * OEMCrypto_Security_Patch_Level
 *
 * Description:
 *   This function returns the current patch level of the software running in
 *   the trusted environment. The patch level is defined by the OEM, and is
 *   only incremented when a security update has been added.
 *
 *   See the section Security Patch Level above for more details.
 *
 * Parameters:
 *   none
 *
 * Returns:
 *   The OEM defined version number.
 *
 * Threading:
 *   This is a "Property Function" and may be called simultaneously with any
 *   other property function or session function, but not any initialization or
 *   usage table function, as if the CDM holds a read lock on the OEMCrypto
 *   system.
 *
 * Version:
 *   This method was introduced in API version 11.
 */
uint8_t OEMCrypto_Security_Patch_Level(void);

/*
 * OEMCrypto_SecurityLevel
 *
 * Description:
 *   Returns a string specifying the security level of the library.
 *
 *   Since this function is spoofable, it is not relied on for security
 *   purposes. It is for information only.
 *
 * Parameters:
 *   none
 *
 * Returns:
 *   A null terminated string. Useful value are "L1", "L2" and "L3".
 *
 * Threading:
 *   This is a "Property Function" and may be called simultaneously with any
 *   other property function or session function, but not any initialization or
 *   usage table function, as if the CDM holds a read lock on the OEMCrypto
 *   system.
 *
 * Version:
 *   This method changed in API version 6.
 */
const char* OEMCrypto_SecurityLevel(void);

/*
 * OEMCrypto_GetHDCPCapability
 *
 * Description:
 *   Returns the maximum HDCP version supported by the device, and the HDCP
 *   version supported by the device and any connected display.
 *
 *   Valid values for HDCP_Capability are:
 *
 *   The value 0xFF means the device is using a local, secure, data path
 *   instead of HDMI output. Notice that HDCP must use flag Type 1: all
 *   downstream devices will also use the same version or higher.
 *
 *   The maximum HDCP level should be the maximum value that the device can
 *   enforce. For example, if the device has an HDCP 1.0 port and an HDCP 2.0
 *   port, and the first port can be disabled, then the maximum is HDCP 2.0. If
 *   the first port cannot be disabled, then the maximum is HDCP 1.0. The
 *   maximum value can be used by the application or server to decide if a
 *   license may be used in the future. For example, a device may be connected
 *   to an external display while an offline license is downloaded, but the
 *   user intends to view the content on a local display. The user will want to
 *   download the higher quality content.
 *
 *   The current HDCP level should be the level of HDCP currently negotiated
 *   with any connected receivers or repeaters either through HDMI or a
 *   supported wireless format. If multiple ports are connected, the current
 *   level should be the minimum HDCP level of all ports. If the key control
 *   block requires an HDCP level equal to or lower than the current HDCP
 *   level, the key is expected to be usable. If the key control block requires
 *   a higher HDCP level, the key is expected to be forbidden.
 *
 *   When a key has version HDCP_V2_3 required in the key control block, the
 *   transmitter must have HDCP version 2.3 and have negotiated a connection
 *   with a version 2.3 receiver or repeater. The transmitter must configure
 *   the content stream to be Type 1. Since the transmitter cannot distinguish
 *   between 2.2 and 2.3 downstream receivers when connected to a repeater, it
 *   may transmit to both 2.2 and 2.3 receivers, but not 2.1 receivers.
 *
 *   For example, if the transmitter is 2.3, and is connected to a receiver
 *   that supports 2.3 then the current level is HDCP_V2_3. If the transmitter
 *   is 2.3 and is connected to a 2.3 repeater, the current level is HDCP_V2_3
 *   even though  the repeater can negotiate a connection with a 2.2 downstream
 *   receiver for a Type 1 Content Stream.
 *
 *   As another example, if the transmitter can support 2.3, but a receiver
 *   supports 2.0, then the current level is HDCP_V2.
 *
 *   When a license requires HDCP, a device may use a wireless protocol to
 *   connect to a display only if that protocol supports the version of HDCP as
 *   required by the license. Both WirelessHD (formerly WiFi Display) and
 *   Miracast support HDCP.
 *
 * Parameters:
 *   [out] current - this is the current HDCP version, based on the device
 *         itself, and the display to which it is connected.
 *   [out] maximum - this is the maximum supported HDCP version for the device,
 *         ignoring any attached device.
 *
 * Returns:
 *   OEMCrypto_SUCCESS
 *   OEMCrypto_ERROR_UNKNOWN_FAILURE
 *   OEMCrypto_ERROR_SYSTEM_INVALIDATED
 *
 * Threading:
 *   This is a "Property Function" and may be called simultaneously with any
 *   other property function or session function, but not any initialization or
 *   usage table function, as if the CDM holds a read lock on the OEMCrypto
 *   system.
 *
 * Version:
 *   This method changed in API version 10.
 */
OEMCryptoResult OEMCrypto_GetHDCPCapability(OEMCrypto_HDCP_Capability* current,
                                            OEMCrypto_HDCP_Capability* maximum);

/*
 * OEMCrypto_SupportsUsageTable
 *
 * Description:
 *   This is used to determine if the device can support a usage table. Since
 *   this function is spoofable, it is not relied on for security purposes. It
 *   is for information only. The usage table is described in the section above.
 *
 * Parameters:
 *   none
 *
 * Returns:
 *   Returns true if the device can maintain a usage table. Returns false
 *         otherwise.
 *
 * Threading:
 *   This is a "Property Function" and may be called simultaneously with any
 *   other property function or session function, but not any initialization or
 *   usage table function, as if the CDM holds a read lock on the OEMCrypto
 *   system.
 *
 * Version:
 *   This method changed in API version 9.
 */
bool OEMCrypto_SupportsUsageTable(void);

/*
 * OEMCrypto_IsAntiRollbackHwPresent
 *
 * Description:
 *   Indicate whether there is hardware protection to detect and/or prevent the
 *   rollback of the usage table. For example, if the usage table contents is
 *   stored entirely on a secure file system that the user cannot read or write
 *   to. Another example is if the usage table has a generation number and the
 *   generation number is stored in secure memory that is not user accessible.
 *
 * Parameters:
 *   none
 *
 * Returns:
 *   Returns true if oemcrypto uses anti-rollback hardware. Returns false
 *         otherwise.
 *
 * Threading:
 *   This is a "Property Function" and may be called simultaneously with any
 *   other property function or session function, but not any initialization or
 *   usage table function, as if the CDM holds a read lock on the OEMCrypto
 *   system.
 *
 * Version:
 *   This method is new in API version 10.
 */
bool OEMCrypto_IsAntiRollbackHwPresent(void);

/*
 * OEMCrypto_GetNumberOfOpenSessions
 *
 * Description:
 *   Returns the current number of open sessions. The CDM and OEMCrypto
 *   consumers can query this value so they can use resources more effectively.
 *
 * Parameters:
 *   [out] count - this is the current number of opened sessions.
 *
 * Returns:
 *   OEMCrypto_SUCCESS
 *   OEMCrypto_ERROR_UNKNOWN_FAILURE
 *   OEMCrypto_ERROR_SYSTEM_INVALIDATED
 *
 * Threading:
 *   This is a "Property Function" and may be called simultaneously with any
 *   other property function or session function, but not any initialization or
 *   usage table function, as if the CDM holds a read lock on the OEMCrypto
 *   system.
 *
 * Version:
 *   This method is new in API version 10.
 */
OEMCryptoResult OEMCrypto_GetNumberOfOpenSessions(size_t* count);

/*
 * OEMCrypto_GetMaxNumberOfSessions
 *
 * Description:
 *   Returns the maximum number of concurrent OEMCrypto sessions supported by
 *   the device. The CDM and OEMCrypto consumers can query this value so they
 *   can use resources more effectively. If the maximum number of sessions
 *   depends on a dynamically allocated shared resource, the returned value
 *   should be a best estimate of the maximum number of sessions.
 *
 *   OEMCrypto shall support a minimum of 10 sessions. Some applications use
 *   multiple sessions to pre-fetch licenses, so high end devices should
 *   support more sessions -- we recommend a minimum of 50 sessions.
 *
 * Parameters:
 *   [out] count - this is the current number of opened sessions.
 *
 * Returns:
 *   OEMCrypto_SUCCESS
 *   OEMCrypto_ERROR_UNKNOWN_FAILURE
 *   OEMCrypto_ERROR_SYSTEM_INVALIDATED
 *
 * Threading:
 *   This is a "Property Function" and may be called simultaneously with any
 *   other property function or session function, but not any initialization or
 *   usage table function, as if the CDM holds a read lock on the OEMCrypto
 *   system.
 *
 * Version:
 *   This method changed in API version 12.
 */
OEMCryptoResult OEMCrypto_GetMaxNumberOfSessions(size_t* max);

/*
 * OEMCrypto_SupportedCertificates
 *
 * Description:
 *   Returns the type of certificates keys that this device supports. With very
 *   few exceptions, all devices should support at least 2048 bit RSA keys.
 *   High end devices should also support 3072 bit RSA keys. Devices that are
 *   cast receivers should also support RSA cast receiver certificates.
 *
 *   Beginning with OEMCrypto v14, the provisioning server may deliver to the
 *   device an RSA key that uses the Carmichael totient. This does not change
 *   the RSA algorithm -- however the product of the private and public keys is
 *   not necessarily the Euler number  \phi (n). OEMCrypto should not reject
 *   such keys.
 *
 * Parameters:
 *   none
 *
 * Returns:
 *   Returns the bitwise or of the following flags. It is likely that high end
 *         devices will support both 2048 and 3072 bit keys while the widevine
 *         servers transition to new key sizes.
 *     - 0x1 = OEMCrypto_Supports_RSA_2048bit - the device can load a DRM
 *        certificate with a 2048 bit RSA key.
 *     - 0x2 = OEMCrypto_Supports_RSA_3072bit - the device can load a DRM
 *        certificate with a 3072 bit RSA key.
 *     - 0x10 = OEMCrypto_Supports_RSA_CAST - the device can load a CAST
 *        certificate.  These certificate are used with
 *        OEMCrypto_GenerateRSASignature with padding type set to 0x2, PKCS1
 *        with block type 1 padding.
 *
 * Threading:
 *   This is a "Property Function" and may be called simultaneously with any
 *   other property function or session function, but not any initialization or
 *   usage table function, as if the CDM holds a read lock on the OEMCrypto
 *   system.
 *
 * Version:
 *   This method changed in API version 13.
 */
uint32_t OEMCrypto_SupportedCertificates(void);

/*
 * OEMCrypto_IsSRMUpdateSupported
 *
 * Description:
 *   Returns true if the device supports SRM files and the file can be updated
 *   via the function OEMCrypto_LoadSRM. This also returns false for devices
 *   that do not support an SRM file, devices that do not support HDCP, and
 *   devices that have no external display support.
 *
 * Parameters:
 *   none
 *
 * Returns:
 *   true - if LoadSRM is supported.
 *   false - otherwise.
 *
 * Threading:
 *   This is a "Property Function" and may be called simultaneously with any
 *   other property function or session function, but not any initialization or
 *   usage table function, as if the CDM holds a read lock on the OEMCrypto
 *   system.
 *
 * Version:
 *   This method changed in API version 13.
 */
bool OEMCrypto_IsSRMUpdateSupported(void);

/*
 * OEMCrypto_GetCurrentSRMVersion
 *
 * Description:
 *   Returns the version number of the current SRM file. If the device does not
 *   support SRM files, this will return OEMCrypto_ERROR_NOT_IMPLEMENTED. If
 *   the device only supports local displays, it would return
 *   OEMCrypto_LOCAL_DISPLAY_ONLY. If the device has an SRM, but cannot use
 *   OEMCrypto to update the SRM, then this function would set version to be
 *   the current version number, and return OEMCrypto_SUCCESS, but it would
 *   return false from OEMCrypto_IsSRMUpdateSupported.
 *
 * Parameters:
 *   [out] version: current SRM version number.
 *
 * Returns:
 *   OEMCrypto_ERROR_NOT_IMPLEMENTED
 *   OEMCrypto_SUCCESS
 *   OEMCrypto_LOCAL_DISPLAY_ONLY - to indicate version was not set, and is not
 *         needed.
 *   OEMCrypto_ERROR_SYSTEM_INVALIDATED
 *
 * Threading:
 *   This is a "Property Function" and may be called simultaneously with any
 *   other property function or session function, but not any initialization or
 *   usage table function, as if the CDM holds a read lock on the OEMCrypto
 *   system.
 *
 * Version:
 *   This method changed in API version 13.
 */
OEMCryptoResult OEMCrypto_GetCurrentSRMVersion(uint16_t* version);

/*
 * OEMCrypto_GetAnalogOutputFlags
 *
 * Description:
 *   Returns whether the device supports analog output or not. This information
 *   will be sent to the license server, and may be used to determine the type
 *   of license allowed. This function is for reporting only. It is paired with
 *   the key control block flags Disable_Analog_Output and CGMS.
 *
 * Parameters:
 *   none.
 *
 * Returns:
 *   Returns a bitwise OR of the following flags.
 *     - 0x0 = OEMCrypto_No_Analog_Output -- the device has no analog output.
 *     - 0x1 = OEMCrypto_Supports_Analog_Output - the device does have analog
 *        output.
 *     - 0x2 = OEMCrypto_Can_Disable_Analog_Ouptput - the device does have
 *        analog output, but it will disable analog output if required by the
 *        key control block.
 *     - 0x4 = OEMCrypto_Supports_CGMS_A - the device supports signaling 2-bit
 *        CGMS-A, if required by the key control block
 *
 * Threading:
 *   This is a "Property Function" and may be called simultaneously with any
 *   other property function or session function, but not any initialization or
 *   usage table function, as if the CDM holds a read lock on the OEMCrypto
 *   system.
 *
 * Version:
 *   This method is new in API version 14.
 */
uint32_t OEMCrypto_GetAnalogOutputFlags(void);

/*
 * OEMCrypto_ResourceRatingTier
 *
 * Description:
 *   This function returns a positive number indicating which resource rating
 *   it supports. This value will bubble up to the application level as a
 *   property. This will allow applications to estimate what resolution and
 *   bandwidth the device is expected to support.
 *
 *   OEMCrypto unit tests and Android GTS tests will verify that devices do
 *   support the resource values specified in the table below at the tier
 *   claimed by the device. If a device claims to be a low end device, the
 *   OEMCrypto unit tests will only verify the low end performance values.
 *
 *   OEMCrypto implementers should consider the numbers below to be minimum
 *   values.
 *
 *   These performance parameters are for OEMCrypto only. In particular,
 *   bandwidth and codec resolution are determined by the platform.
 *
 *   Some parameters need more explanation. The Sample size is typically the
 *   size of one encoded frame. Converting this to resolution depends on the
 *   Codec, which is not specified by OEMCrypto. Some content has the sample
 *   broken into several subsamples. The "number of subsamples" restriction
 *   requires that any content can be broken into at least that many
 *   subsamples. However, this number may be larger if DecryptCENC returns
 *   OEMCrypto_ERROR_BUFFER_TOO_LARGE. In that case, the layer above OEMCrypto
 *   will break the sample into subsamples of size "Decrypt Buffer Size" as
 *   specified in the table below. The "Decrypt Buffer Size" means the size of
 *   one subsample that may be passed into DecryptCENC or CopyBuffer without
 *   returning error OEMCrypto_ERROR_BUFFER_TOO_LARGE.
 *
 *   The number of keys per session is an indication of how many different
 *   track types there can be for a piece of content. Typically, content will
 *   have several keys corresponding to audio and video at different
 *   resolutions. If the content uses key rotation, there could be three keys
 *   -- previous interval, current interval, and next interval -- for each
 *   resolution.
 *
 *   Concurrent playback sessions versus concurrent sessions: some applications
 *   will preload multiple licenses before the user picks which content to
 *   play. Each of these licenses corresponds to an open session. Once playback
 *   starts, some platforms support picture-in-picture or multiple displays.
 *   Each of these pictures would correspond to a separate playback session
 *   with active decryption.
 *
 *   Decrypted frames per second -- strictly speaking, OEMCrypto only controls
 *   the decryption part of playback and cannot control the decoding and
 *   display part. However, devices that support the higher resource tiers
 *   should also support a higher frame rate. Platforms may enforce these
 *   values. For example Android will enforce a frame rate via a GTS test.
 *
 *   +-----------------------------------+-----------+------------+-----------+
 *   |Resource Rating Tier               |1 - Low    |2 - Medium  |3 - High   |
 *   +-----------------------------------+-----------+------------+-----------+
 *   |Sample size                        |1 MB       |2 MB        |4 MB       |
 *   +-----------------------------------+-----------+------------+-----------+
 *   |Number of Subsamples               |8          |16          |32         |
 *   +-----------------------------------+-----------+------------+-----------+
 *   |Decrypt buffer size                |100 KB     |500 KB      |1 MB       |
 *   +-----------------------------------+-----------+------------+-----------+
 *   |Generic crypto buffer size         |10 KB      |100 KB      |500 KB     |
 *   +-----------------------------------+-----------+------------+-----------+
 *   |Number of concurrent sessions      |10         |20          |20         |
 *   +-----------------------------------+-----------+------------+-----------+
 *   |Number of keys per session         |4          |20          |20         |
 *   +-----------------------------------+-----------+------------+-----------+
 *   |Decrypted Frames per Second        |30 fps SD  |30 fps HD   |60 fps HD  |
 *   +-----------------------------------+-----------+------------+-----------+
 *
 * Parameters:
 *   none.
 *
 * Returns:
 *   Returns an integer indicating which resource tier the device supports.
 *
 * Threading:
 *   This is a "Property Function" and may be called simultaneously with any
 *   other property function or session function, but not any initialization or
 *   usage table function, as if the CDM holds a read lock on the OEMCrypto
 *   system.
 *
 * Version:
 *   This method is new in API version 15.
 */
uint32_t OEMCrypto_ResourceRatingTier(void);

/*
 * OEMCrypto_RewrapDeviceRSAKey30
 *
 * Description:
 *   This function is similar to RewrapDeviceRSAKey, except it uses the private
 *   key from an OEM certificate to decrypt the message key instead of keys
 *   derived from a keybox. Verifies an RSA provisioning response is valid and
 *   corresponds to the previous provisioning request by checking the nonce.
 *   The RSA private key is decrypted and stored in secure memory. The RSA key
 *   is then re-encrypted and signed for storage on the filesystem. We
 *   recommend that the OEM use an encryption key and signing key generated
 *   using an algorithm at least as strong as that in GenerateDerivedKeys.
 *
 *   After decrypting enc_rsa_key, If the first four bytes of the buffer are
 *   the string "SIGN", then the actual RSA key begins on the 9th byte of the
 *   buffer. The second four bytes of the buffer is the 32 bit field
 *   "allowed_schemes", of type RSA_Padding_Scheme, which is used in
 *   OEMCrypto_GenerateRSASignature. The value of allowed_schemes must also be
 *   wrapped with RSA key. We recommend storing the magic string "SIGN" with
 *   the key to distinguish keys that have a value for allowed_schemes from
 *   those that should use the default allowed_schemes. Devices that do not
 *   support the alternative signing algorithms may refuse to load these keys
 *   and return an error of OEMCrypto_ERROR_NOT_IMPLEMENTED. The main use case
 *   for these alternative signing algorithms is to support devices that use
 *   X509 certificates for authentication when acting as a ChromeCast receiver.
 *   This is not needed for devices that wish to send data to a ChromeCast.
 *
 *   If the first four bytes of the buffer enc_rsa_key are not the string
 *   "SIGN", then the default value of allowed_schemes = 1 (kSign_RSASSA_PSS)
 *   will be used.
 *
 * Verification and Algorithm:
 *   The following checks should be performed. If any check fails, an error is
 *   returned, and the key is not loaded.
 *
 *     1. Verify that in_wrapped_rsa_key_length is large enough to hold the
 *        rewrapped key, returning OEMCrypto_ERROR_SHORT_BUFFER otherwise.
 *     2. Verify that the nonce matches one generated by a previous call to
 *        OEMCrypto_GenerateNonce().  The matching nonce shall be removed from
 *        the nonce table.  If there is no matching nonce, return
 *        OEMCRYPTO_ERROR_INVALID_NONCE.  Notice that the nonce may not point
 *        to a word aligned memory location.
 *     3. Decrypt encrypted_message_key with the OEM certificate's private RSA
 *        key using RSA-OAEP into the buffer message_key.  This message key is
 *        a 128 bit AES key used only in step 4. This message_key should be
 *        kept in secure memory and protected from the user.
 *     4. Decrypt enc_rsa_key into the buffer rsa_key using the message_key,
 *        which was found in step 3.  Use enc_rsa_key_iv as the initial vector
 *        for AES_128-CBC mode, with PKCS#5 padding. The rsa_key should be kept
 *        in secure memory and protected from the user.
 *     5. If the first four bytes of the buffer rsa_key are the string "SIGN",
 *        then the  actual RSA key begins on the 9th byte of the buffer.  The
 *        second four bytes of the buffer is the 32 bit field
 *        "allowed_schemes", of type RSA_Padding_Scheme, which is used in
 *        OEMCrypto_GenerateRSASignature.    The value of allowed_schemes must
 *        also be wrapped with RSA key. We recommend storing the magic string
 *        "SIGN" with the key to distinguish keys that have a value for
 *        allowed_schemes from those that should use the default
 *        allowed_schemes. Devices that do not support the alternative signing
 *        algorithms may refuse to load these keys and return an error of
 *        OEMCrypto_ERROR_NOT_IMPLEMENTED.  The main use case for these
 *        alternative signing algorithms is to support devices that use X.509
 *        certificates for authentication when acting as a ChromeCast receiver.
 *        This is not needed for devices that wish to send data to a ChromeCast.
 *     6. If the first four bytes of the buffer rsa_key are not the string
 *        "SIGN", then the default value of allowed_schemes = 1
 *        (kSign_RSASSA_PSS) will be used.
 *     7. After possibly skipping past the first 8 bytes signifying the allowed
 *        signing algorithm, the rest of the buffer rsa_key contains an RSA
 *        device key in PKCS#8 binary DER encoded format. The OEMCrypto library
 *        shall verify that this RSA key is valid.
 *     8. Re-encrypt the device RSA key with an internal key (such as the OEM
 *        key or Widevine Keybox key) and the generated IV using AES-128-CBC
 *        with PKCS#5 padding.
 *     9. Copy the rewrapped key to the buffer specified by wrapped_rsa_key and
 *        the size of the wrapped key to wrapped_rsa_key_length.
 *
 * Parameters:
 *   [in] session: crypto session identifier.
 *   [in] nonce: A pointer to the nonce provided in the provisioning response.
 *         (unaligned uint32_t)
 *   [in] encrypted_message_key :  message_key encrypted by private key from
 *         OEM cert.
 *   [in] encrypted_message_key_length :  length of encrypted_message_key in
 *         bytes.
 *   [in] enc_rsa_key: Encrypted device private RSA key received from the
 *         provisioning server. Format is PKCS#8, binary DER encoded, and
 *         encrypted with message_key, using AES-128-CBC with PKCS#5 padding.
 *   [in] enc_rsa_key_length: length of the encrypted RSA key, in bytes.
 *   [in] enc_rsa_key_iv: IV for decrypting RSA key. Size is 128 bits.
 *   [out] wrapped_rsa_key: pointer to buffer in which encrypted RSA key should
 *         be stored. May be null on the first call in order to find required
 *         buffer size.
 *   [in/out] wrapped_rsa_key_length: (in) length of the encrypted RSA key, in
 *         bytes.
 *   (out) actual length of the encrypted RSA key
 *
 * Returns:
 *   OEMCrypto_SUCCESS success
 *   OEMCrypto_ERROR_NO_DEVICE_KEY
 *   OEMCrypto_ERROR_INVALID_SESSION
 *   OEMCrypto_ERROR_INVALID_RSA_KEY
 *   OEMCrypto_ERROR_SIGNATURE_FAILURE
 *   OEMCrypto_ERROR_INVALID_NONCE
 *   OEMCrypto_ERROR_SHORT_BUFFER
 *   OEMCrypto_ERROR_INSUFFICIENT_RESOURCES
 *   OEMCrypto_ERROR_UNKNOWN_FAILURE
 *   OEMCrypto_ERROR_BUFFER_TOO_LARGE
 *   OEMCrypto_ERROR_SESSION_LOST_STATE
 *   OEMCrypto_ERROR_SYSTEM_INVALIDATED
 *
 * Buffer Sizes:
 *   OEMCrypto shall support message sizes of at least 8 KiB.
 *   OEMCrypto shall return OEMCrypto_ERROR_BUFFER_TOO_LARGE if the buffer is
 *   larger than the supported size.
 *
 * Threading:
 *   This is a "Session Function" and may be called simultaneously with session
 *   functions for other sessions but not simultaneously with other functions
 *   for this session. It will not be called simultaneously with initialization
 *   or usage table functions. It is as if the CDM holds a write lock for this
 *   session, and a read lock on the OEMCrypto system.
 *
 * Version:
 *   This method changed in API version 12.
 */
OEMCryptoResult OEMCrypto_RewrapDeviceRSAKey30(
    OEMCrypto_SESSION session, const uint32_t* unaligned_nonce,
    const uint8_t* encrypted_message_key, size_t encrypted_message_key_length,
    const uint8_t* enc_rsa_key, size_t enc_rsa_key_length,
    const uint8_t* enc_rsa_key_iv, uint8_t* wrapped_rsa_key,
    size_t* wrapped_rsa_key_length);

/*
 * OEMCrypto_RewrapDeviceRSAKey
 *
 * Description:
 *   This function is similar to RewrapDeviceRSAKey30, except it uses session
 *   keys derived from the keybox instead of the OEM certificate. Verifies an
 *   RSA provisioning response is valid and corresponds to the previous
 *   provisioning request by checking the nonce. The RSA private key is
 *   decrypted and stored in secure memory. The RSA key is then re-encrypted
 *   and signed for storage on the filesystem. We recommend that the OEM use an
 *   encryption key and signing key generated using an algorithm at least as
 *   strong as that in GenerateDerivedKeys.
 *
 *   After decrypting enc_rsa_key, If the first four bytes of the buffer are
 *   the string "SIGN", then the actual RSA key begins on the 9th byte of the
 *   buffer. The second four bytes of the buffer is the 32 bit field
 *   "allowed_schemes", of type RSA_Padding_Scheme, which is used in
 *   OEMCrypto_GenerateRSASignature. The value of allowed_schemes must also be
 *   wrapped with RSA key. We recommend storing the magic string "SIGN" with
 *   the key to distinguish keys that have a value for allowed_schemes from
 *   those that should use the default allowed_schemes. Devices that do not
 *   support the alternative signing algorithms may refuse to load these keys
 *   and return an error of OEMCrypto_ERROR_NOT_IMPLEMENTED. The main use case
 *   for these alternative signing algorithms is to support devices that use
 *   X509 certificates for authentication when acting as a ChromeCast receiver.
 *   This is not needed for devices that wish to send data to a ChromeCast.
 *
 *   If the first four bytes of the buffer enc_rsa_key are not the string
 *   "SIGN", then the default value of allowed_schemes = 1 (kSign_RSASSA_PSS)
 *   will be used.
 *
 * Verification and Algorithm:
 *   The following checks should be performed. If any check fails, an error is
 *   returned, and the key is not loaded.
 *
 *     1. Check that all the pointer values passed into it are within the
 *        buffer specified by message and message_length.
 *     2. Verify that in_wrapped_rsa_key_length is large enough to hold the
 *        rewrapped key, returning OEMCrypto_ERROR_SHORT_BUFFER otherwise.
 *     3. Verify that the nonce matches one generated by a previous call to
 *        OEMCrypto_GenerateNonce().  The matching nonce shall be removed from
 *        the nonce table.  If there is no matching nonce, return
 *        OEMCRYPTO_ERROR_INVALID_NONCE.
 *     4. Verify the message signature, using the derived signing key
 *        (mac_key[server]) from a previous call to
 *        OEMCrypto_GenerateDerivedKeys.
 *     5. Decrypt enc_rsa_key in the buffer rsa_key using the derived
 *        encryption key (enc_key) from a previous call to
 *        OEMCrypto_GenerateDerivedKeys.  Use enc_rsa_key_iv as the initial
 *        vector for AES_128-CBC mode, with PKCS#5 padding. The rsa_key should
 *        be kept in secure memory and protected from the user.
 *     6. If the first four bytes of the buffer rsa_key are the string "SIGN",
 *        then the  actual RSA key begins on the 9th byte of the buffer.  The
 *        second four bytes of the buffer is the 32 bit field
 *        "allowed_schemes", of type RSA_Padding_Scheme, which is used in
 *        OEMCrypto_GenerateRSASignature.    The value of allowed_schemes must
 *        also be wrapped with RSA key. We recommend storing the magic string
 *        "SIGN" with the key to distinguish keys that have a value for
 *        allowed_schemes from those that should use the default
 *        allowed_schemes. Devices that do not support the alternative signing
 *        algorithms may refuse to load these keys and return an error of
 *        OEMCrypto_ERROR_NOT_IMPLEMENTED.  The main use case for these
 *        alternative signing algorithms is to support devices that use X.509
 *        certificates for authentication when acting as a ChromeCast receiver.
 *        This is not needed for devices that wish to send data to a ChromeCast.
 *     7. If the first four bytes of the buffer rsa_key are not the string
 *        "SIGN", then the default value of allowed_schemes = 1
 *        (kSign_RSASSA_PSS) will be used.
 *     8. After possibly skipping past the first 8 bytes signifying the allowed
 *        signing algorithm, the rest of the buffer rsa_key contains an RSA
 *        device key in PKCS#8 binary DER encoded format. The OEMCrypto library
 *        shall verify that this RSA key is valid.
 *     9. Re-encrypt the device RSA key with an internal key (such as the OEM
 *        key or Widevine Keybox key) and the generated IV using AES-128-CBC
 *        with PKCS#5 padding.
 *     10. Copy the rewrapped key to the buffer specified by wrapped_rsa_key
 *        and the size of the wrapped key to wrapped_rsa_key_length.
 *
 * Parameters:
 *   [in] session: crypto session identifier.
 *   [in] message: pointer to memory containing message to be verified.
 *   [in] message_length: length of the message, in bytes.
 *   [in] signature: pointer to memory containing the HMAC-SHA256 signature for
 *         message, received from the provisioning server.
 *   [in] signature_length: length of the signature, in bytes.
 *   [in] nonce: A pointer to the nonce provided in the provisioning response.
 *   [in] enc_rsa_key: Encrypted device private RSA key received from the
 *         provisioning server. Format is PKCS#8, binary DER encoded, and
 *         encrypted with the derived encryption key, using AES-128-CBC with
 *         PKCS#5 padding.
 *   [in] enc_rsa_key_length: length of the encrypted RSA key, in bytes.
 *   [in] enc_rsa_key_iv: IV for decrypting RSA key. Size is 128 bits.
 *   [out] wrapped_rsa_key: pointer to buffer in which encrypted RSA key should
 *         be stored. May be null on the first call in order to find required
 *         buffer size.
 *   [in/out] wrapped_rsa_key_length: (in) length of the encrypted RSA key, in
 *         bytes.
 *   (out) actual length of the encrypted RSA key
 *
 * Returns:
 *   OEMCrypto_SUCCESS success
 *   OEMCrypto_ERROR_NO_DEVICE_KEY
 *   OEMCrypto_ERROR_INVALID_SESSION
 *   OEMCrypto_ERROR_INVALID_RSA_KEY
 *   OEMCrypto_ERROR_SIGNATURE_FAILURE
 *   OEMCrypto_ERROR_INVALID_NONCE
 *   OEMCrypto_ERROR_SHORT_BUFFER
 *   OEMCrypto_ERROR_INSUFFICIENT_RESOURCES
 *   OEMCrypto_ERROR_UNKNOWN_FAILURE
 *   OEMCrypto_ERROR_BUFFER_TOO_LARGE
 *   OEMCrypto_ERROR_SESSION_LOST_STATE
 *   OEMCrypto_ERROR_SYSTEM_INVALIDATED
 *
 * Buffer Sizes:
 *   OEMCrypto shall support message sizes of at least 8 KiB.
 *   OEMCrypto shall return OEMCrypto_ERROR_BUFFER_TOO_LARGE if the buffer is
 *   larger than the supported size.
 *
 * Threading:
 *   This is a "Session Function" and may be called simultaneously with session
 *   functions for other sessions but not simultaneously with other functions
 *   for this session. It will not be called simultaneously with initialization
 *   or usage table functions. It is as if the CDM holds a write lock for this
 *   session, and a read lock on the OEMCrypto system.
 *
 * Version:
 *   This method changed in API version 12.
 */
OEMCryptoResult OEMCrypto_RewrapDeviceRSAKey(
    OEMCrypto_SESSION session, const uint8_t* message, size_t message_length,
    const uint8_t* signature, size_t signature_length,
    const uint32_t* unaligned_nonce, const uint8_t* enc_rsa_key,
    size_t enc_rsa_key_length, const uint8_t* enc_rsa_key_iv,
    uint8_t* wrapped_rsa_key, size_t* wrapped_rsa_key_length);

/*
 * OEMCrypto_LoadDeviceRSAKey
 *
 * Description:
 *   Loads a wrapped RSA private key to secure memory for use by this session
 *   in future calls to OEMCrypto_GenerateRSASignature. The wrapped RSA key
 *   will be the one verified and wrapped by OEMCrypto_RewrapDeviceRSAKey. The
 *   RSA private key should be stored in secure memory.
 *
 *   If the bit field "allowed_schemes" was wrapped with this RSA key, its
 *   value will be loaded and stored with the RSA key. If there was not bit
 *   field wrapped with the RSA key, the key will use a default value of 1 =
 *   RSASSA-PSS with SHA1.
 *
 * Verification:
 *   The following checks should be performed. If any check fails, an error is
 *   returned, and the RSA key is not loaded.
 *     1. The wrapped key has a valid signature, as described in
 *        RewrapDeviceRSAKey.
 *     2. The decrypted key is a valid private RSA key.
 *     3. If a value for allowed_schemes is included with the key, it is a
 *        valid value.
 *
 * Parameters:
 *   [in] session: crypto session identifier.
 *   [in] wrapped_rsa_key: wrapped device RSA key stored on the device. Format
 *         is PKCS#8, binary DER encoded, and encrypted with a key internal to
 *         the OEMCrypto instance, using AES-128-CBC with PKCS#5 padding. This
 *         is the wrapped key generated by OEMCrypto_RewrapDeviceRSAKey.
 *   [in] wrapped_rsa_key_length: length of the wrapped key buffer, in bytes.
 *
 * Returns:
 *   OEMCrypto_SUCCESS success
 *   OEMCrypto_ERROR_NO_DEVICE_KEY
 *   OEMCrypto_ERROR_INVALID_SESSION
 *   OEMCrypto_ERROR_INVALID_RSA_KEY
 *   OEMCrypto_ERROR_INSUFFICIENT_RESOURCES
 *   OEMCrypto_ERROR_UNKNOWN_FAILURE
 *   OEMCrypto_ERROR_SESSION_LOST_STATE
 *   OEMCrypto_ERROR_SYSTEM_INVALIDATED
 *
 * Threading:
 *   This is a "Session Function" and may be called simultaneously with session
 *   functions for other sessions but not simultaneously with other functions
 *   for this session. It will not be called simultaneously with initialization
 *   or usage table functions. It is as if the CDM holds a write lock for this
 *   session, and a read lock on the OEMCrypto system.
 *
 * Version:
 *   This method changed in API version 9.
 */
OEMCryptoResult OEMCrypto_LoadDeviceRSAKey(OEMCrypto_SESSION session,
                                           const uint8_t* wrapped_rsa_key,
                                           size_t wrapped_rsa_key_length);

/*
 * OEMCrypto_LoadTestRSAKey
 *
 * Description:
 *   Some platforms do not support keyboxes or OEM Certificates. On those
 *   platforms, there is a DRM certificate baked into the OEMCrypto library.
 *   This is unusual, and is only available for L3 devices. In order to debug
 *   and test those devices, they should be able to switch to the test DRM
 *   certificate.
 *
 *   Temporarily use the standard test RSA key until the next call to
 *   OEMCrypto_Terminate. This allows a standard suite of unit tests to be run
 *   on a production device without permanently changing the key. Using the
 *   test key is not persistent.
 *
 *   The test key can be found in the unit test code, oemcrypto_test.cpp, in
 *   PKCS8 form as the constant kTestRSAPKCS8PrivateKeyInfo2_2048.
 *
 * Parameters:
 *   none
 *
 * Returns:
 *   OEMCrypto_SUCCESS success
 *   OEMCrypto_ERROR_INSUFFICIENT_RESOURCES
 *   OEMCrypto_ERROR_NOT_IMPLEMENTED - devices that use a keybox should not
 *         implement this function
 *   OEMCrypto_ERROR_SYSTEM_INVALIDATED
 *
 * Threading:
 *   This is an "Initialization and Termination Function" and will not be
 *   called simultaneously with any other function, as if the CDM holds a write
 *   lock on the OEMCrypto system.
 *
 * Version:
 *   This method is new in API version 10.
 */
OEMCryptoResult OEMCrypto_LoadTestRSAKey(void);

/*
 * OEMCrypto_GenerateRSASignature
 *
 * Description:
 *   The OEMCrypto_GenerateRSASignature method is used to sign messages using
 *   the device private RSA key, specifically, it is used to sign the initial
 *   license request.
 *
 *   Refer to the Signing Messages Sent to a Server section above for more
 *   details.
 *
 *   If this function is called after OEMCrypto_LoadDeviceRSAKey for the same
 *   session, then this function should use the device RSA key that was loaded.
 *   If this function is called after a call to
 *   OEMCrypto_GetOEMPublicCertificate for the same session, then this function
 *   should use the RSA private key associated with the OEM certificate. The
 *   only padding scheme that is valid for the OEM certificate is 0x1 -
 *   RSASSA-PSS with SHA1. Any other padding scheme must generate an error.
 *
 *   For devices that wish to be CAST receivers, there is a new RSA padding
 *   scheme. The padding_scheme parameter indicates which hashing and padding
 *   is to be applied to the message so as to generate the encoded message (the
 *   modulus-sized block to which the integer conversion and RSA decryption is
 *   applied). The following values are defined:
 *
 *   0x1 - RSASSA-PSS with SHA1.
 *
 *   0x2 - PKCS1 with block type 1 padding (only).
 *
 *   In the first case, a hash algorithm (SHA1) is first applied to the
 *   message, whose length is not otherwise restricted. In the second case, the
 *   "message" is already a digest, so no further hashing is applied, and the
 *   message_length can be no longer than 83 bytes. If the message_length is
 *   greater than 83 bytes OEMCrypto_ERROR_SIGNATURE_FAILURE shall be returned.
 *
 *   The second padding scheme is for devices that use X509 certificates for
 *   authentication. The main example is devices that work as a Cast receiver,
 *   like a ChromeCast, not for devices that wish to send to the Cast device,
 *   such as almost all Android devices. OEMs that do not support X509
 *   certificate authentication need not implement the second scheme and can
 *   return OEMCrypto_ERROR_NOT_IMPLEMENTED.
 *
 * Verification:
 *   The bitwise AND of the parameter padding_scheme and the RSA key's
 *   allowed_schemes is computed. If this value is 0, then the signature is not
 *   computed and the error OEMCrypto_ERROR_INVALID_RSA_KEY is returned.
 *
 * Parameters:
 *   [in] session: crypto session identifier.
 *   [in] message: pointer to memory containing message to be signed.
 *   [in] message_length: length of the message, in bytes.
 *   [out] signature: buffer to hold the message signature. On return, it will
 *         contain the message signature generated with the device private RSA
 *         key using RSASSA-PSS. Will be null on the first call in order to
 *         find required buffer size.
 *   [in/out] signature_length: (in) length of the signature buffer, in bytes.
 *   (out) actual length of the signature
 *   [in] padding_scheme: specify which scheme to use for the signature.
 *
 * Returns:
 *   OEMCrypto_SUCCESS success
 *   OEMCrypto_ERROR_SHORT_BUFFER if the signature buffer is too small.
 *   OEMCrypto_ERROR_INVALID_SESSION
 *   OEMCrypto_ERROR_INVALID_CONTEXT
 *   OEMCrypto_ERROR_INVALID_RSA_KEY
 *   OEMCrypto_ERROR_INSUFFICIENT_RESOURCES
 *   OEMCrypto_ERROR_UNKNOWN_FAILURE
 *   OEMCrypto_ERROR_NOT_IMPLEMENTED - if algorithm > 0, and the device does
 *         not support that algorithm.
 *   OEMCrypto_ERROR_BUFFER_TOO_LARGE
 *   OEMCrypto_ERROR_SESSION_LOST_STATE
 *   OEMCrypto_ERROR_SYSTEM_INVALIDATED
 *
 * Buffer Sizes:
 *   OEMCrypto shall support message sizes of at least 8 KiB.
 *   OEMCrypto shall return OEMCrypto_ERROR_BUFFER_TOO_LARGE if the buffer is
 *   larger than the supported size.
 *
 * Threading:
 *   This is a "Session Function" and may be called simultaneously with session
 *   functions for other sessions but not simultaneously with other functions
 *   for this session. It will not be called simultaneously with initialization
 *   or usage table functions. It is as if the CDM holds a write lock for this
 *   session, and a read lock on the OEMCrypto system.
 *
 * Version:
 *   This method changed in API version 12.
 */
OEMCryptoResult OEMCrypto_GenerateRSASignature(
    OEMCrypto_SESSION session, const uint8_t* message, size_t message_length,
    uint8_t* signature, size_t* signature_length,
    RSA_Padding_Scheme padding_scheme);

/*
 * OEMCrypto_CreateUsageTableHeader
 *
 * Description:
 *   This creates a new Usage Table Header with no entries. If there is already
 *   a generation number stored in secure storage, it will be incremented by 1
 *   and used as the new Master Generation Number. This will only be called if
 *   the CDM layer finds no existing usage table on the file system. OEMCrypto
 *   will encrypt and sign the new, empty, header and return it in the provided
 *   buffer.
 *
 *   Devices that do not implement a Session Usage Table may return
 *   OEMCrypto_ERROR_NOT_IMPLEMENTED.
 *
 * Parameters:
 *   [out] header_buffer: pointer to memory where encrypted usage table header
 *         is written.
 *   [in/out] header_buffer_length: (in) length of the header_buffer, in bytes.
 *   (out) actual length of the header_buffer
 *
 * Returns:
 *   OEMCrypto_SUCCESS success
 *   OEMCrypto_ERROR_SHORT_BUFFER - if header_buffer_length is too small.
 *   OEMCrypto_ERROR_NOT_IMPLEMENTED
 *   OEMCrypto_ERROR_UNKNOWN_FAILURE
 *   OEMCrypto_ERROR_SYSTEM_INVALIDATED
 *
 * Threading:
 *   This is a "Usage Table Function" and will not be called simultaneously
 *   with any other function, as if the CDM holds a write lock on the OEMCrypto
 *   system.
 *
 * Version:
 *   This method changed in API version 13.
 */
OEMCryptoResult OEMCrypto_CreateUsageTableHeader(uint8_t* header_buffer,
                                                 size_t* header_buffer_length);

/*
 * OEMCrypto_LoadUsageTableHeader
 *
 * Description:
 *   This loads the Usage Table Header. The buffer's signature is verified and
 *   the buffer is decrypted. OEMCrypto will verify the verification string. If
 *   the Master Generation Number is more than 1 off, the table is considered
 *   bad, the headers are NOT loaded, and the error
 *   OEMCrypto_ERROR_GENERATION_SKEW is returned. If the generation number is
 *   off by 1, the warning OEMCrypto_WARNING_GENERATION_SKEW is returned but
 *   the header is still loaded. This warning may be logged by the CDM layer.
 *
 * Parameters:
 *   [in] buffer: pointer to memory containing encrypted usage table header.
 *   [in] buffert_length: length of the buffer, in bytes.
 *
 * Returns:
 *   OEMCrypto_SUCCESS success
 *   OEMCrypto_ERROR_SHORT_BUFFER
 *   OEMCrypto_ERROR_NOT_IMPLEMENTED - some devices do not implement usage
 *         tables.
 *   OEMCrypto_ERROR_UNKNOWN_FAILURE
 *   OEMCrypto_WARNING_GENERATION_SKEW - if the generation number is off by
 *         exactly 1.
 *   OEMCrypto_ERROR_GENERATION_SKEW - if the generation number is off by more
 *         than 1.
 *   OEMCrypto_ERROR_SIGNATURE_FAILURE - if the signature failed.
 *   OEMCrypto_ERROR_BAD_MAGIC - verification string does not match.
 *   OEMCrypto_ERROR_SYSTEM_INVALIDATED
 *
 * Threading:
 *   This is a "Usage Table Function" and will not be called simultaneously
 *   with any other function, as if the CDM holds a write lock on the OEMCrypto
 *   system.
 *
 * Version:
 *   This method changed in API version 13.
 */
OEMCryptoResult OEMCrypto_LoadUsageTableHeader(const uint8_t* buffer,
                                               size_t buffer_length);

/*
 * OEMCrypto_CreateNewUsageEntry
 *
 * Description:
 *   This creates a new usage entry. The size of the header will be increased
 *   by 8 bytes, and secure volatile memory will be allocated for it. The new
 *   entry will be associated with the given session. The status of the new
 *   entry will be set to "unused". OEMCrypto will set *usage_entry_number to
 *   be the index of the new entry. The first entry created will have index 0.
 *   The new entry will be initialized with a generation number equal to the
 *   master generation number, which will also be stored in the header's new
 *   slot. Then the master generation number will be incremented. Since each
 *   entry's generation number is less than the master generation number, the
 *   new entry will have a generation number that is larger than all other
 *   entries and larger than all previously deleted entries. This helps prevent
 *   a rogue application from deleting an entry and then loading an old version
 *   of it.
 *
 * Parameters:
 *   [in] session: handle for the session to be used.
 *   [out] usage_entry_number: index of new usage entry.
 *
 * Returns:
 *   OEMCrypto_SUCCESS success
 *   OEMCrypto_ERROR_NOT_IMPLEMENTED - some devices do not implement usage
 *         tables.
 *   OEMCrypto_ERROR_INSUFFICIENT_RESOURCES - if there is no room in memory to
 *         increase the size of the usage table header. The CDM layer can
 *         delete some entries and then try again, or it can pass the error up
 *         to the application.
 *   OEMCrypto_ERROR_UNKNOWN_FAILURE
 *   OEMCrypto_ERROR_SESSION_LOST_STATE
 *   OEMCrypto_ERROR_SYSTEM_INVALIDATED
 *
 * Threading:
 *   This is a "Usage Table Function" and will not be called simultaneously
 *   with any other function, as if the CDM holds a write lock on the OEMCrypto
 *   system.
 *
 * Version:
 *   This method changed in API version 13.
 */
OEMCryptoResult OEMCrypto_CreateNewUsageEntry(OEMCrypto_SESSION session,
                                              uint32_t* usage_entry_number);

/*
 * OEMCrypto_LoadUsageEntry
 *
 * Description:
 *   This loads a usage table saved previously by UpdateUsageEntry. The
 *   signature at the beginning of the buffer is verified and the buffer will
 *   be decrypted. Then the verification field in the entry will be verified.
 *   The index in the entry must match the index passed in. The generation
 *   number in the entry will be compared against that in the header. If it is
 *   off by 1, a warning is returned, but the entry is still loaded. This
 *   warning may be logged by the CDM layer. If the generation number is off by
 *   more than 1, an error is returned and the entry is not loaded.
 *
 *   If the entry is already loaded into another open session, then this fails
 *   and returns OEMCrypto_ERROR_INVALID_SESSION.
 *
 * Parameters:
 *   [in] session: handle for the session to be used.
 *   [in] usage_entry_number: index of existing usage entry.
 *   [in] buffer: pointer to memory containing encrypted usage table entry.
 *   [in] buffer_length: length of the buffer, in bytes.
 *
 * Returns:
 *   OEMCrypto_SUCCESS success
 *   OEMCrypto_ERROR_SHORT_BUFFER
 *   OEMCrypto_ERROR_NOT_IMPLEMENTED - some devices do not implement usage
 *         tables.
 *   OEMCrypto_ERROR_UNKNOWN_FAILURE - index beyond end of table.
 *   OEMCrypto_ERROR_INVALID_SESSION - entry associated with another session or
 *         the index is wrong.
 *   OEMCrypto_WARNING_GENERATION_SKEW - if the generation number is off by
 *         exactly 1.
 *   OEMCrypto_ERROR_GENERATION_SKEW - if the generation number is off by more
 *         than 1.
 *   OEMCrypto_ERROR_SIGNATURE_FAILURE - if the signature failed.
 *   OEMCrypto_ERROR_BAD_MAGIC - verification string does not match.
 *   OEMCrypto_ERROR_SESSION_LOST_STATE
 *   OEMCrypto_ERROR_SYSTEM_INVALIDATED
 *
 * Threading:
 *   This is a "Usage Table Function" and will not be called simultaneously
 *   with any other function, as if the CDM holds a write lock on the OEMCrypto
 *   system.
 *
 * Version:
 *   This method changed in API version 13.
 */
OEMCryptoResult OEMCrypto_LoadUsageEntry(OEMCrypto_SESSION session,
                                         uint32_t usage_entry_number,
                                         const uint8_t* buffer,
                                         size_t buffer_length);

/*
 * OEMCrypto_UpdateUsageEntry
 *
 * Description:
 *   Updates the session's usage entry and fills buffers with the encrypted and
 *   signed entry and usage table header. OEMCrypto will update all time and
 *   status values in the entry, and then increment the entry's generation
 *   number. The corresponding generation number in the usage table header is
 *   also incremented so that it matches the one in the entry. The master
 *   generation number in the usage table header is incremented and is copied
 *   to secure persistent storage. OEMCrypto will encrypt and sign the entry
 *   into the entry_buffer, and it will encrypt and sign the usage table header
 *   into the header_buffer. Some actions, such as the first decrypt and
 *   deactivating an entry, will also increment the entry's generation number
 *   as well as changing the entry's status and time fields. As in OEMCrypto
 *   v12, the first decryption will change the status from Inactive to Active,
 *   and it will set the time stamp "first decrypt".
 *
 *   If the usage entry has the flag ForbidReport set, then the flag is
 *   cleared. It is the responsibility of the CDM layer to call this function
 *   and save the usage table before the next call to ReportUsage and before
 *   the CDM is terminated. Failure to do so will result in generation number
 *   skew, which will invalidate all of the usage table.
 *
 *   If either buffer_length is not large enough, they are set to the needed
 *   size, and OEMCrypto_ERROR_SHORT_BUFFER. In this case, the entry is not
 *   updated, ForbidReport is not cleared, generation numbers are not
 *   incremented, and no other work is done.
 *
 * Parameters:
 *   [in] session: handle for the session to be used.
 *   [out] header_buffer: pointer to memory where encrypted usage table header
 *         is written.
 *   [in/out] header_buffer_length: (in) length of the header_buffer, in bytes.
 *   (out) actual length of the header_buffer
 *   [out] entry_buffer: pointer to memory where encrypted usage table entry is
 *         written.
 *   [in/out] buffer_length: (in) length of the entry_buffer, in bytes.
 *   (out) actual length of the entry_buffer
 *
 * Returns:
 *   OEMCrypto_SUCCESS success
 *   OEMCrypto_ERROR_SHORT_BUFFER
 *   OEMCrypto_ERROR_NOT_IMPLEMENTED - some devices do not implement usage
 *         tables.
 *   OEMCrypto_ERROR_UNKNOWN_FAILURE
 *   OEMCrypto_ERROR_SESSION_LOST_STATE
 *   OEMCrypto_ERROR_SYSTEM_INVALIDATED
 *
 * Threading:
 *   This is a "Usage Table Function" and will not be called simultaneously
 *   with any other function, as if the CDM holds a write lock on the OEMCrypto
 *   system.
 *
 * Version:
 *   This method changed in API version 13.
 */
OEMCryptoResult OEMCrypto_UpdateUsageEntry(OEMCrypto_SESSION session,
                                           uint8_t* header_buffer,
                                           size_t* header_buffer_length,
                                           uint8_t* entry_buffer,
                                           size_t* entry_buffer_length);

/*
 * OEMCrypto_DeactivateUsageEntry
 *
 * Description:
 *   This deactivates the usage entry associated with the current session. This
 *   means that the state of the usage entry is changed to InactiveUsed if it
 *   was Active, or InactiveUnused if it was Unused. This also increments the
 *   entry's generation number, and the header's master generation number. The
 *   corresponding generation number in the usage table header is also
 *   incremented so that it matches the one in the entry. The entry's flag
 *   ForbidReport will be set. This flag prevents an application from
 *   generating a report of a deactivated license without first saving the
 *   entry.
 *
 *   It is allowed to call this function multiple times. If the state is
 *   already InactiveUsed or InactiveUnused, then this function does not change
 *   the entry or its state.
 *
 * Parameters:
 *   [in] session: handle for the session to be used.
 *   [in] pst: pointer to memory containing Provider Session Token.
 *   [in] pst_length: length of the pst, in bytes.
 *
 * Returns:
 *   OEMCrypto_SUCCESS success
 *   OEMCrypto_ERROR_INVALID_CONTEXT - an entry was not created or loaded, or
 *         the pst does not match.
 *   OEMCrypto_ERROR_NOT_IMPLEMENTED
 *   OEMCrypto_ERROR_UNKNOWN_FAILURE
 *   OEMCrypto_ERROR_BUFFER_TOO_LARGE
 *   OEMCrypto_ERROR_SESSION_LOST_STATE
 *   OEMCrypto_ERROR_SYSTEM_INVALIDATED
 *
 * Buffer Sizes:
 *   OEMCrypto shall support  pst sizes of at least 255 bytes.
 *   OEMCrypto shall return OEMCrypto_ERROR_BUFFER_TOO_LARGE if the buffer is
 *   larger than the supported size.
 *
 * Threading:
 *   This is a "Usage Table Function" and will not be called simultaneously
 *   with any other function, as if the CDM holds a write lock on the OEMCrypto
 *   system.
 *
 * Version:
 *   This method changed in API version 13.
 */
OEMCryptoResult OEMCrypto_DeactivateUsageEntry(OEMCrypto_SESSION session,
                                               const uint8_t* pst,
                                               size_t pst_length);

/*
 * OEMCrypto_ReportUsage
 *
 * Description:
 *   All fields of OEMCrypto_PST_Report are in network byte order.
 *
 *   If the buffer_length is not sufficient to hold a report structure, set
 *   buffer_length and return OEMCrypto_ERROR_SHORT_BUFFER.
 *
 *   If the an entry was not loaded or created with
 *   OEMCrypto_CreateNewUsageEntry or OEMCRypto_LoadUsageEntry, or if the pst
 *   does not match that in the entry, return the error
 *   OEMCrypto_ERROR_INVALID_CONTEXT.
 *
 *   If the usage entry's flag ForbidReport is set, indicating the entry has
 *   not been saved since the entry was deactivated, then the error
 *   OEMCrypto_ERROR_ENTRY_NEEDS_UPDATE is returned and a report is not
 *   generated. Similarly, if any key in the session has been used since the
 *   last call to OEMCrypto_UpdateUsageEntry, then the report is not generated,
 *   and OEMCrypto returns  the error OEMCrypto_ERROR_ENTRY_NEEDS_UPDATE.
 *
 *   The pst_report is filled out by subtracting the times in the Usage Entry
 *   from the current time on the secure clock. This is done in case the secure
 *   clock is not using UTC time, but is instead using something like seconds
 *   since clock installed.
 *
 *   Valid values for status are:
 *
 *     - 0 = kUnused -- the keys have not been used to decrypt.
 *     - 1 = kActive -- the keys have been used, and have not been deactivated.
 *     - 2 = kInactive - deprecated.  Use kInactiveUsed or kInactiveUnused.
 *     - 3 = kInactiveUsed -- the keys have been marked inactive after being
 *        active.
 *     - 4 = kInactiveUnused -- they keys have been marked inactive, but were
 *        never active.
 *   The clock_security_level is reported as follows:
 *
 *     - 0 = Insecure Clock - clock just uses system time.
 *     - 1 = Secure Timer - clock runs from a secure timer which is initialized
 *        from system time when OEMCrypto becomes active and cannot be modified
 *        by user software or the user while OEMCrypto is active.
 *     - 2 = Secure Clock - Real-time clock set from a secure source that
 *        cannot be modified by user software regardless of whether OEMCrypto
 *        is active or inactive. The clock time can only be modified by
 *        tampering with the security software or hardware.
 *     - 3 = Hardware Secure Clock - Real-time clock set from a secure source
 *        that cannot be modified by user software and there are security
 *        features that prevent the user from modifying the clock in hardware,
 *        such as a tamper proof battery.
 *   After pst_report has been filled in, the HMAC SHA1 signature is computed
 *   for the buffer from bytes 20 to the end of the pst field. The signature is
 *   computed using the mac_key[client] which is stored in the usage table. The
 *   HMAC SHA1 signature is used to prevent a rogue application from using
 *   OMECrypto_GenerateSignature to forge a Usage Report.
 *
 *   Devices that do not implement a Session Usage Table may return
 *   OEMCrypto_ERROR_NOT_IMPLEMENTED.
 *
 * Parameters:
 *   [in] session: handle for the session to be used.
 *   [in] pst: pointer to memory containing Provider Session Token.
 *   [in] pst_length: length of the pst, in bytes.
 *   [out] buffer: pointer to buffer in which usage report should be stored.
 *         May be null on the first call in order to find required buffer size.
 *   [in/out] buffer_length: (in) length of the report buffer, in bytes.
 *   (out) actual length of the report
 *
 * Returns:
 *   OEMCrypto_SUCCESS success
 *   OEMCrypto_ERROR_SHORT_BUFFER - if report buffer is not large enough to
 *         hold the output report.
 *   OEMCrypto_ERROR_INVALID_SESSION - no open session with that id.
 *   OEMCrypto_ERROR_INVALID_CONTEXT
 *   OEMCrypto_ERROR_NOT_IMPLEMENTED
 *   OEMCrypto_ERROR_UNKNOWN_FAILURE
 *   OEMCrypto_ERROR_BUFFER_TOO_LARGE
 *   OEMCrypto_ERROR_ENTRY_NEEDS_UPDATE - if no call to UpdateUsageEntry since
 *         last call to Deactivate or since key use.
 *   OEMCrypto_ERROR_WRONG_PST - report asked for wrong pst.
 *   OEMCrypto_ERROR_SESSION_LOST_STATE
 *   OEMCrypto_ERROR_SYSTEM_INVALIDATED
 *
 * Buffer Sizes:
 *   OEMCrypto shall support  pst sizes of at least 255 bytes.
 *   OEMCrypto shall return OEMCrypto_ERROR_BUFFER_TOO_LARGE if the buffer is
 *   larger than the supported size.
 *
 * Threading:
 *   This is a "Usage Table Function" and will not be called simultaneously
 *   with any other function, as if the CDM holds a write lock on the OEMCrypto
 *   system.
 *
 * Version:
 *   This method changed in API version 13.
 */
OEMCryptoResult OEMCrypto_ReportUsage(OEMCrypto_SESSION session,
                                      const uint8_t* pst, size_t pst_length,
                                      uint8_t* buffer,
                                      size_t* buffer_length);

/*
 * OEMCrypto_MoveEntry
 *
 * Description:
 *   Moves the entry associated with the current session from one location in
 *   the usage table header to another. This function is used by the CDM layer
 *   to defragment the usage table. This does not modify any data in the entry,
 *   except the index and the generation number. The index in the session's
 *   usage entry will be changed to new_index. The generation number in
 *   session's usage entry and in the header for new_index will be increased to
 *   the master generation number, and then the master generation number is
 *   incremented. If there was an existing entry at the new location, it will
 *   be overwritten. It is an error to call this when the entry that was at
 *   new_index is associated with a currently open session. In this case, the
 *   error code OEMCrypto_ERROR_ENTRY_IN_USE is returned. It is the CDM layer's
 *   responsibility to call UpdateUsageEntry after moving an entry. It is an
 *   error for new_index to be beyond the end of the existing usage table
 *   header.
 *
 *   Devices that do not implement a Session Usage Table may return
 *   OEMCrypto_ERROR_NOT_IMPLEMENTED.
 *
 * Parameters:
 *   [in] session: handle for the session to be used.
 *   [in] new_index: new index to be used for the session's usage entry
 *
 * Returns:
 *   OEMCrypto_SUCCESS success
 *   OEMCrypto_ERROR_NOT_IMPLEMENTED
 *   OEMCrypto_ERROR_UNKNOWN_FAILURE
 *   OEMCrypto_ERROR_BUFFER_TOO_LARGE
 *   OEMCrypto_ERROR_ENTRY_IN_USE
 *   OEMCrypto_ERROR_SESSION_LOST_STATE
 *   OEMCrypto_ERROR_SYSTEM_INVALIDATED
 *
 * Threading:
 *   This is a "Usage Table Function" and will not be called simultaneously
 *   with any other function, as if the CDM holds a write lock on the OEMCrypto
 *   system.
 *
 * Version:
 *   This method is new in API version 13.
 */
OEMCryptoResult OEMCrypto_MoveEntry(OEMCrypto_SESSION session,
                                    uint32_t new_index);

/*
 * OEMCrypto_ShrinkUsageTableHeader
 *
 * Description:
 *   This shrinks the usage table and the header. This function is used by the
 *   CDM layer after it has  defragmented the usage table and can delete unused
 *   entries. It is an error if any open session is associated with an entry
 *   that will be erased - the error OEMCrypto_ERROR_ENTRY_IN_USE shall be
 *   returned in this case. If new_table_size is larger than the current size,
 *   then the header is not changed and the error is returned. If the header
 *   has not been previously loaded, then an error is returned. OEMCrypto will
 *   increment the master generation number in the header and store the new
 *   value in secure persistent storage. Then, OEMCrypto will encrypt and sign
 *   the header into the provided buffer. The generation numbers of all
 *   remaining entries will remain unchanged. The next time
 *   OEMCrypto_CreateNewUsageEntry is called, the new entry will have an index
 *   of new_table_size.
 *
 *   Devices that do not implement a Session Usage Table may return
 *   OEMCrypto_ERROR_NOT_IMPLEMENTED.
 *
 *   If header_buffer_length is not large enough to hold the new table, it is
 *   set to the needed value, the generation number is not  incremented, and
 *   OEMCrypto_ERROR_SHORT_BUFFER is returned.
 *
 * Parameters:
 *   [in] new_entry_count: number of entries in the to be in the header.
 *   [out] header_buffer: pointer to memory where encrypted usage table header
 *         is written.
 *   [in/out] header_buffer_length: (in) length of the header_buffer, in bytes.
 *   (out) actual length of the header_buffer
 *
 * Returns:
 *   OEMCrypto_SUCCESS success
 *   OEMCrypto_ERROR_SHORT_BUFFER
 *   OEMCrypto_ERROR_NOT_IMPLEMENTED
 *   OEMCrypto_ERROR_UNKNOWN_FAILURE
 *   OEMCrypto_ERROR_ENTRY_IN_USE
 *   OEMCrypto_ERROR_SYSTEM_INVALIDATED
 *
 * Threading:
 *   This is a "Usage Table Function" and will not be called simultaneously
 *   with any other function, as if the CDM holds a write lock on the OEMCrypto
 *   system.
 *
 * Version:
 *   This method is new in API version 13.
 */
OEMCryptoResult OEMCrypto_ShrinkUsageTableHeader(uint32_t new_entry_count,
                                                 uint8_t* header_buffer,
                                                 size_t* header_buffer_length);

/*
 * OEMCrypto_CopyOldUsageEntry
 *
 * Description:
 *   This function copies an entry from the old v12 table to the new table. The
 *   new entry will already have been loaded by CreateNewUsageEntry. If the
 *   device did not support pre-v13 usage tables, this may return
 *   OEMCrypto_ERROR_NOT_IMPLEMENTED.
 *
 *   This is only needed for devices that are upgrading from a version of
 *   OEMCrypto before v13 to a recent version. Devices that have an existing
 *   usage table with customer's offline licenses will use this method to move
 *   entries from the old table to the new one.
 *
 * Parameters:
 *   [in] session: handle for the session to be used.
 *   [in] pst: pointer to memory containing Provider Session Token.
 *   [in] pst_length: length of the pst, in bytes.
 *
 * Returns:
 *   OEMCrypto_SUCCESS success
 *   OEMCrypto_ERROR_NOT_IMPLEMENTED
 *   OEMCrypto_ERROR_UNKNOWN_FAILURE
 *   OEMCrypto_ERROR_SESSION_LOST_STATE
 *   OEMCrypto_ERROR_SYSTEM_INVALIDATED
 *
 * Threading:
 *   This is a "Usage Table Function" and will not be called simultaneously
 *   with any other function, as if the CDM holds a write lock on the OEMCrypto
 *   system.
 *
 * Version:
 *   This method is new in API version 13.
 */
OEMCryptoResult OEMCrypto_CopyOldUsageEntry(OEMCrypto_SESSION session,
                                            const uint8_t* pst,
                                            size_t pst_length);

/*
 * OEMCrypto_DeleteOldUsageTable
 *
 * Description:
 *   This function will delete the old usage table, if possible, freeing any
 *   nonvolatile secure memory. This may return OEMCrypto_ERROR_NOT_IMPLEMENTED
 *   if the device did not support pre-v13 usage tables.
 *
 *   This is only needed for devices that are upgrading from a version of
 *   OEMCrypto before v13 to a recent version. Devices that have an existing
 *   usage table with customer's offline licenses will use this method to move
 *   entries from the old table to the new one.
 *
 * Parameters:
 *   none
 *
 * Returns:
 *   OEMCrypto_SUCCESS success
 *   OEMCrypto_ERROR_NOT_IMPLEMENTED
 *   OEMCrypto_ERROR_UNKNOWN_FAILURE
 *   OEMCrypto_ERROR_SYSTEM_INVALIDATED
 *
 * Threading:
 *   This is an "Initialization and Termination Function" and will not be
 *   called simultaneously with any other function, as if the CDM holds a write
 *   lock on the OEMCrypto system.
 *
 * Version:
 *   This method is new in API version 13.
 */
OEMCryptoResult OEMCrypto_DeleteOldUsageTable(void);

/*
 * OEMCrypto_RemoveSRM
 *
 * Description:
 *   Delete the current SRM. Any valid SRM, regardless of its version number,
 *   will be installable after this via OEMCrypto_LoadSRM.
 *
 *   This function should not be implemented on production devices, and will
 *   only be used to verify unit tests on a test device.
 *
 * Parameters:
 *   none
 *
 * Returns:
 *   OEMCrypto_SUCCESS - if the SRM file was deleted.
 *   OEMCrypto_ERROR_NOT_IMPLEMENTED - always on production devices.
 *
 * Threading:
 *   This is an "Initialization and Termination Function" and will not be
 *   called simultaneously with any other function, as if the CDM holds a write
 *   lock on the OEMCrypto system.
 *
 * Version:
 *   This method is new in API version 13.
 */
OEMCryptoResult OEMCrypto_RemoveSRM(void);

/*
 * OEMCrypto_CreateOldUsageEntry
 *
 * Description:
 *   This forces the creation of an entry in the old usage table in order to
 *   test OEMCrypto_CopyOldUsageTable. OEMCrypto will create a new entry, set
 *   the status and compute the times at license receive, first decrypt and
 *   last decrypt. The mac keys will be copied to the entry. The mac keys are
 *   not encrypted, but will only correspond to a test license.
 *
 *   Devices that do not support usage tables, or devices that will not be
 *   field upgraded  from a version of OEMCrypto before v13 to a recent version
 *   may return OEMCrypto_ERROR_NOT_IMPLEMENTED.
 *
 * Returns:
 *   OEMCrypto_ERROR_NOT_IMPLEMENTED
 *   OEMCrypto_SUCCESS
 *
 * Threading:
 *   This is an "Initialization and Termination Function" and will not be
 *   called simultaneously with any other function, as if the CDM holds a write
 *   lock on the OEMCrypto system. It is only used when running unit tests.
 *
 * Version:
 *   This method is new in API version 13.
 */
OEMCryptoResult OEMCrypto_CreateOldUsageEntry(uint64_t time_since_license_received,
                                              uint64_t time_since_first_decrypt,
                                              uint64_t time_since_last_decrypt,
                                              OEMCrypto_Usage_Entry_Status status,
                                              uint8_t *server_mac_key,
                                              uint8_t *client_mac_key,
                                              const uint8_t* pst,
                                              size_t pst_length);

/*
 * OEMCrypto_SupportsDecryptHash
 *
 * Description:
 *   Returns the type of hash function supported for Full Decrypt Path Testing.
 *   A hash type of OEMCrypto_Hash_Not_Supported = 0 means this feature is not
 *   supported. OEMCrypto is not required by Google to support this feature,
 *   but support will greatly improve automated testing. A hash type of
 *   OEMCrypto_CRC_Clear_Buffer = 1 means the device will be able to compute
 *   the CRC 32 checksum of the decrypted content in the secure buffer after a
 *   call to OEMCrypto_DecryptCENC. Google intends to provide test applications
 *   on some platforms, such as Android, that will automate decryption testing
 *   using the CRC 32 checksum of all frames in some test content.
 *
 *   If an SOC vendor cannot support CRC 32 checksums of decrypted output, but
 *   can support some other hash or checksum, then the function should return
 *   OEMCrypto_Partner_Defined_Hash = 2 and those partners should modify the
 *   test application to compute the appropriate hash. An application that
 *   computes the CRC 32 hashes of test content and builds a hash file in the
 *   correct format will be provided by Widevine. The source of this
 *   application will be provided so that partners may modify it to compute
 *   their own hash format and generate their own hashes.
 *
 * Returns:
 *   OEMCrypto_Hash_Not_Supported = 0;
 *   OEMCrypto_CRC_Clear_Buffer = 1;
 *   OEMCrypto_Partner_Defined_Hash = 2;
 *
 * Threading:
 *   This is a "Property Function" and may be called simultaneously with any
 *   other property function or session function, but not any initialization or
 *   usage table function, as if the CDM holds a read lock on the OEMCrypto
 *   system.
 *
 * Version:
 *   This method is new in API version 15.
 */
uint32_t OEMCrypto_SupportsDecryptHash(void);

/*
 * OEMCrypto_SetDecryptHash
 *
 * Description:
 *   Set the hash value for the next frame to be decrypted. This function is
 *   called before the first subsample is passed to OEMCrypto_DecryptCENC, when
 *   the subsample_flag has the bit OEMCrytpo_FirstSubsample set. The hash is
 *   over all of the frame or sample: encrypted and clear subsamples
 *   concatenated together, up to, and including the subsample with the
 *   subsample_flag having the bit OEMCrypto_LastSubsample set. If hashing the
 *   output is not supported, then this will return
 *   OEMCrypto_ERROR_NOT_IMPLEMENTED. If the hash is ill formed or there are
 *   other error conditions, this returns OEMCrypto_ERROR_UNKNOWN_FAILURE. The
 *   length of the hash will be at most 128 bytes.
 *
 *   This may be called before the first call to SelectKey. In that case, this
 *   function cannot verify that the key control block allows hash
 *   verification. The function DecryptCENC should verify that the key control
 *   bit allows hash verification when it is called. If an attempt is made to
 *   compute a hash when the selected key does not have the bit
 *   Allow_Hash_Verification set, then a hash should not be computed, and
 *   OEMCrypto_GetHashErrorCode should return the error
 *   OEMCrypto_ERROR_UNKNOWN_FAILURE.
 *
 *   OEMCrypto should compute the hash of the frame and then compare it with
 *   the correct value. If the values differ, then OEMCrypto should latch in an
 *   error and save the frame number of the bad hash. It is allowed for
 *   OEMCrypto to postpone computation of the hash until the frame is
 *   displayed. This might happen if the actual decryption operation is carried
 *   out by a later step in the video pipeline, or if you are using a partner
 *   specified hash of the decoded frame. For this reason, an error state must
 *   be saved until the call to OEMCrypto_GetHashErrorCode is made.
 *
 * Parameters:
 *   [in] session: session id for current decrypt operation
 *   [in] frame_number: frame number for the recent DecryptCENC sample.
 *   [in] hash: hash or CRC of previously decrypted frame.
 *   [in] hash_length: length of hash, in bytes.
 *
 * Returns:
 *   OEMCrypto_SUCCESS - if the hash was set
 *   OEMCrypto_ERROR_NOT_IMPLEMENTED - function not implemented
 *   OEMCrypto_ERROR_INVALID_SESSION - session not open
 *   OEMCrypto_ERROR_SHORT_BUFFER - hash_length too short for supported hash
 *         type
 *   OEMCrypto_ERROR_BUFFER_TOO_LARGE - hash_length too long for supported hash
 *         type
 *   OEMCrypto_ERROR_UNKNOWN_FAILURE - other error
 *   OEMCrypto_ERROR_SESSION_LOST_STATE
 *   OEMCrypto_ERROR_SYSTEM_INVALIDATED
 *
 * Threading:
 *   This is a "Session Function" and may be called simultaneously with session
 *   functions for other sessions but not simultaneously with other functions
 *   for this session. It will not be called simultaneously with initialization
 *   or usage table functions. It is as if the CDM holds a write lock for this
 *   session, and a read lock on the OEMCrypto system.
 *
 * Version:
 *   This method is new in API version 15.
 */
OEMCryptoResult OEMCrypto_SetDecryptHash(OEMCrypto_SESSION session,
                                         uint32_t frame_number,
                                         const uint8_t* hash,
                                         size_t hash_length);

/*
 * OEMCrypto_GetHashErrorCode
 *
 * Description:
 *   If the hash set in OEMCrypto_SetDecryptHash did not match the computed
 *   hash, then an error code was saved internally. This function returns that
 *   error and the frame number of the bad hash. This will be called
 *   periodically, but might not be in sync with the decrypt loop. OEMCrypto
 *   shall not reset the error state to "no error" once any frame has failed
 *   verification. It should be initialized to "no error" when the session is
 *   first opened. If there is more than one bad frame, it is the implementer's
 *   choice if it is more useful to return the number of the first bad frame,
 *   or the most recent bad frame.
 *
 *   If the hash could not be computed -- either because the
 *   Allow_Hash_Verification was not set in the key control block, or because
 *   there were other issues -- this function should return
 *   OEMCrypto_ERROR_UNKNOWN_FAILURE.
 *
 * Parameters:
 *   [in] session: session id for operation.
 *   [out] failed_frame_number: frame number for sample with incorrect hash.
 *
 * Returns:
 *   OEMCrypto_SUCCESS - if all frames have had a correct hash
 *   OEMCrypto_ERROR_NOT_IMPLEMENTED
 *   OEMCrypto_ERROR_BAD_HASH - if any frame had an incorrect hash
 *   OEMCrypto_ERROR_UNKNOWN_FAILURE - if the hash could not be computed
 *   OEMCrypto_ERROR_SESSION_LOST_STATE
 *   OEMCrypto_ERROR_SYSTEM_INVALIDATED
 *
 * Threading:
 *   This is a "Session Function" and may be called simultaneously with session
 *   functions for other sessions but not simultaneously with other functions
 *   for this session. It will not be called simultaneously with initialization
 *   or usage table functions. It is as if the CDM holds a write lock for this
 *   session, and a read lock on the OEMCrypto system.
 *
 * Version:
 *   This method is new in API version 15.
 */
OEMCryptoResult OEMCrypto_GetHashErrorCode(OEMCrypto_SESSION session,
                                           uint32_t* failed_frame_number);

#ifdef __cplusplus
}
#endif

#endif  // OEMCRYPTO_CENC_H_
