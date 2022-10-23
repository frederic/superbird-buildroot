




#define DAP_CPDP_CHANNEL_FORMAT(ground, lfe, heights) (((ground)+(lfe)+(heights)) | ((ground) << 4) | ((lfe) << 8) | ((heights) << 12))

#define DAP_CPDP_FORMAT_OBJECT (0)
#define DAP_CPDP_FORMAT_1      DAP_CPDP_CHANNEL_FORMAT(1u, 0u, 0u)
#define DAP_CPDP_FORMAT_2      DAP_CPDP_CHANNEL_FORMAT(2u, 0u, 0u)
#define DAP_CPDP_FORMAT_5_1    DAP_CPDP_CHANNEL_FORMAT(5u, 1u, 0u)
#define DAP_CPDP_FORMAT_7_1    DAP_CPDP_CHANNEL_FORMAT(7u, 1u, 0u)
#define DAP_CPDP_FORMAT_5_1_2  DAP_CPDP_CHANNEL_FORMAT(5u, 1u, 2u)
#define DAP_CPDP_FORMAT_5_1_4  DAP_CPDP_CHANNEL_FORMAT(5u, 1u, 4u)
#define DAP_CPDP_FORMAT_7_1_2  DAP_CPDP_CHANNEL_FORMAT(7u, 1u, 2u)
#define DAP_CPDP_FORMAT_7_1_4  DAP_CPDP_CHANNEL_FORMAT(7u, 1u, 4u)

#define DAP_CPDP_PROCESS_OPTIMIZER_NB_BANDS_MAX     (20u)
#define DAP_CPDP_MAX_NUM_OUTPUT_CHANNELS           (12)
#define DAP_CPDP_AUDIO_OPTIMIZER_BANDS_MAX        (20u)
#define DAP_CPDP_GRAPHIC_EQUALIZER_BANDS_MAX        (20u)
#define DAP_CPDP_IEQ_MAX_BANDS_NUM      (20u)
#define VIRTUALIZER_7_1_2_CHANNEL_COUNT (10)
#define DAP_CPDP_REGULATOR_TUNING_BANDS_MAX            (20u)
#define DAP_CPDP_VIRTUAL_BASS_SUBGAINS_NUM          (3)
#define MAX_DAWS_SUPPORTED_OUT_CHANNELS (8)
#define MAX_DAWS_SUPPORTED_IN_CHANNELS  (2)
#define MAX_DAP_PRESET_PROFILES         (15)
#define MAX_NAME_SIZE                   (32)
#define MAX_BRAND_SIZE                  (MAX_NAME_SIZE)

/**
 * @brief Dolby Audio Processing Supported Sample Rates
 */
typedef enum {
    DAP_SUPPORTED_SAMPLE_RATE_32_KHZ   /**< 32KHz   */
    , DAP_SUPPORTED_SAMPLE_RATE_44_1_KHZ /**< 44.1KHz */
    , DAP_SUPPORTED_SAMPLE_RATE_48_KHZ  /**< 48KHz   */
    , DAP_SUPPORTED_SAMPLE_RATE_MAX     /**< Max Modes supported */
} daws_dap_sample_rates;

/**
 * @brief Dolby Audio Processing Supported IEQ Profiles
 */
typedef enum {
    IEQ_PROFILE_OPEN     /**< IEQ Profile Open */
    , IEQ_PROFILE_RICH    /**< IEQ Profile Rich */
    , IEQ_PROFILE_FOCUSSED /**< IEQ Profile Focus*/
    , IEQ_PROFILE_CUSTOM  /**< IEQ Profile Custom*/
    , IEQ_PROFILES_MAX    /**< Max IEQ Profiles */
} daws_dap_ieq_profiles;

/**
 * @brief Dolby Audio Processing Regulator Info Structure
 */
struct daws_dap_regulator {
    unsigned bands_num;
    int relaxation_amount;
    int overdrive;
    int timbre_preservation;
    int speaker_dist_enable;
    int thresh_low[DAP_CPDP_REGULATOR_TUNING_BANDS_MAX];
    int thresh_high[DAP_CPDP_REGULATOR_TUNING_BANDS_MAX];
    int isolated_bands[DAP_CPDP_REGULATOR_TUNING_BANDS_MAX];
    unsigned int freqs[DAP_CPDP_REGULATOR_TUNING_BANDS_MAX];
};

/**
 * @brief Dolby Audio Processing Virtual Bass Info Structure
 */
struct virtual_bass {
    int mode;
    int overall_gain;
    int slope_gain;
    unsigned low_src_freq;
    unsigned high_src_freq;
    int subgains[DAP_CPDP_VIRTUAL_BASS_SUBGAINS_NUM];
    unsigned low_mix_freq;
    unsigned high_mix_freq;
};

/**
 * @brief Dolby Audio Processing Content Processing Info Structure
 */
struct daws_profile_content_processing {
    int audio_optimizer_enable;
    unsigned int audio_optimizer_bands_num;
    int audio_optimizer_gains[DAP_CPDP_MAX_NUM_OUTPUT_CHANNELS][DAP_CPDP_AUDIO_OPTIMIZER_BANDS_MAX];
    unsigned int audio_optimizer_freq[DAP_CPDP_AUDIO_OPTIMIZER_BANDS_MAX];

    int be_enable;
    int bex_enable;
    int be_boost;
    int be_cutoff_freq;
    int be_width;
    int bex_cutoff_freq;

    int calibration_boost;

    int dialog_enhancer_enable;
    int dialog_enhancer_amount;
    int dialog_enhancer_ducking;

    int graphic_equalizer_enable;
    unsigned int graphic_equalizer_num;
    unsigned int graphic_equalizer_freqs[DAP_CPDP_GRAPHIC_EQUALIZER_BANDS_MAX];
    int graphic_equalizer_gains[DAP_CPDP_GRAPHIC_EQUALIZER_BANDS_MAX];

    int height_filter_mode;

    /* Intelligent Equalizer */
    int ieq_enable;
    unsigned int ieq_num;
    int ieq_amount;
    int ieq_bands_set;
    unsigned int ieq_freqs[DAP_CPDP_IEQ_MAX_BANDS_NUM];
    int ieq_gains[DAP_CPDP_IEQ_MAX_BANDS_NUM];

    /* Media Intelligence Steering Controls */
    int mi_dialog_enhancer_steering_enable;
    int mi_dv_leveler_steering_enable;
    int mi_ieq_steering_enable;
    int mi_surround_compressor_steering_enable;
    int mi_virtualizer_steering_enable;

    /* Gains */
    int postgain;
    int pregain;
    int systemgain;

    /* Process Optimizer Variables */
    int proc_optimizer_enable;
    int proc_optimizer_gain[VIRTUALIZER_7_1_2_CHANNEL_COUNT][DAP_CPDP_PROCESS_OPTIMIZER_NB_BANDS_MAX];
    unsigned int proc_optimizer_freq[DAP_CPDP_PROCESS_OPTIMIZER_NB_BANDS_MAX];
    unsigned int proc_optimizer_bands_num;

    int regulator_enable;
    struct daws_dap_regulator regulator;

    int surround_boost;
    int surround_decoder_enable;

    unsigned virtualizer_front_speaker_angle;
    unsigned virtualizer_height_speaker_angle;
    unsigned virtualizer_surround_speaker_angle ;

    struct virtual_bass vb;

    int volmax_boost;
    int volume_leveler_enable;
    int volume_leveler_amount;
    int volume_leveler_in_target;
    int volume_leveler_out_target;
    int volume_modeler_calibration;
    int volume_modeler_enable;

    int out_mode_pht_virt_enable;
    int out_mode_psurround_virt_enable;

    int spkr_virt_mode;                                 /**< Speaker Virtualization Mode: DAWS uses DAP_CPDP_PROCESS_5_1_2_SPEAKER */
    int nb_output_channels;                             /**< Number of output channels for which Tuning XML file is configured for */
    int mix_matrix[MAX_DAWS_SUPPORTED_OUT_CHANNELS * MAX_DAWS_SUPPORTED_IN_CHANNELS]; /**< Pointer to the channel mix matrix */

    int partial_height_virtualizer_enable;
    int partial_surround_virtualizer_enable;
};

/**
 * @brief Dolby Audio Processing Profile Structures
 */
struct daws_dap_profiles {
    struct daws_profile_content_processing cp;  /**< Content Processing Struct */
};

/**
 * @brief Dolby Audio Processing Profile Structures
 */
struct daws_dap_constants {

    int _dap_freq_bands[DAP_SUPPORTED_SAMPLE_RATE_MAX][DAP_CPDP_PROCESS_OPTIMIZER_NB_BANDS_MAX];
    int _ieq_profile[IEQ_PROFILES_MAX][DAP_CPDP_PROCESS_OPTIMIZER_NB_BANDS_MAX];
    int _negative_max[DAP_CPDP_PROCESS_OPTIMIZER_NB_BANDS_MAX];
    int _empty_freq_bands[DAP_CPDP_PROCESS_OPTIMIZER_NB_BANDS_MAX];
};

struct xml_version {
    int major;
    int minor;
    int update;
};

struct dtt_version {
    int major;
    int minor;
    int update;
};

/**
 * @brief Wireless Speaker processing modes
 */
typedef enum {
    PRESET_DYNAMIC_MODE = 0/**< Processing optimized for dynamic content */
    , PRESET_MOVIE_MODE     /**< Processing optimized for movie content */
    , PRESET_MUSIC_MODE     /**< Processing optimized for music content */
    , PRESET_GAME_MODE      /**< Processing optimized for game content  */
    , PRESET_VOICE_MODE     /**< Processing optimized for voice content */
    , PRESET_CUSTOM_MODE    /**< Processing optimized for Custom E content  */
    , PRESET_OFF_MODE       /**< No optimization for processing content  */
    , PRESET_CUSTOM_A_MODE  /**< Processing optimized for Custom A content  */
    , PRESET_CUSTOM_B_MODE  /**< Processing optimized for Custom B content  */
    , PRESET_CUSTOM_C_MODE  /**< Processing optimized for Custom C content  */
    , PRESET_CUSTOM_D_MODE  /**< Processing optimized for Custom D content  */
    , PRESET_MAX_MODES      /**< Maximum Preset Modes Supported */
} daws_preset_mode;

typedef struct daws_dap_config_t {
    struct xml_version _xml_version;
    struct dtt_version _dtt_version;
    char speaker_name[MAX_NAME_SIZE];
    char speaker_brand[MAX_BRAND_SIZE];
    struct daws_dap_constants constants;
    struct daws_dap_profiles profiles[MAX_DAP_PRESET_PROFILES];
    daws_preset_mode use_profile;
    daws_preset_mode preset_idx;

    int spkr_virt_mode;
    int spkr_out_mode;
    int height_channels;
    int enable_sub;
    int daws_bass_management;
    int side_spkr_enabled;

    unsigned input_format;
    int disable_plugin;
} daws_dap_config;

















