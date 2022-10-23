/******************************************************************************
 * This program is protected under international and U.S. copyright laws as
 * an unpublished work. This program is confidential and proprietary to the
 * copyright owners. Reproduction or disclosure, in whole or in part, or the
 * production of derivative works therefrom without the express permission of
 * the copyright owners is prohibited.
 *
 *                  Copyright (C) 2017 by Dolby Laboratories.
 *                            All rights reserved.
 ******************************************************************************/

/**
 * @file "daws_xml_parser.h"
 *
 * @brief Interface for DAP Parameter Tuning parsing functionality
 *
 * @ingroup daws_solns_sic
 */

#ifndef DAWS_XML_PARSER_H
#define DAWS_XML_PARSER_H

#define TOTAL_SPEAKER_TYPE_COUNT 4

/* Context structure for local callback functions */
#define MAXLINE 4096

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief XML Parser Virtual Bass Tags
 */
struct virtual_bass_tags {
    int mode;
    int mix_freqs;
    int overall_gain;
    int subgains;
    int slope_gain;
    int src_freqs;
};


/**
 * @brief XML Parser Profile Tags
 */
struct profiles_tags {
    int b_device_data;
    int b_audio_optimizer_enable;
    int b_audio_optimizer_bands;
    int b_be_enable;
    int b_be_boost;
    int b_be_cutoff_freq;
    int b_be_width;
    int b_bex_enable;
    int b_bex_cutoff_freq;
    int b_calibration_boost;
    int b_de_enable;
    int b_de_amount;
    int b_de_ducking;
    int b_geq_enable;
    int b_geq_bands;
    int b_ieq_enable;
    int ieq_amount;
    int ieq_bands_set;
    int height_filter_mode;
    int b_mi_de_se;
    int b_mi_dv_se;
    int b_mi_ieq_se;
    int b_mi_sc_se;
    int b_mi_virt_se;
    int postgain;
    int pregain;
    int systemgain;
    int b_proc_opt_enable;
    int proc_opt_bands;
    int b_regulator_enable;
    int regulator_overdrive;
    int regulator_relaxation;
    int b_regulator_sd_enable;
    int regulator_timbre_preservation;
    int b_regulator_tuning;
    int b_regulator_thresh_high;
    int b_regulator_thresh_low;
    int b_regulator_isolated_band;
    int surround_boost;
    int b_surround_decoder_enable;
    int virt_ft_spkr_angle;
    int virt_ht_spkr_angle;
    int virt_surround_spkr_angle;
    int volmax_boost;
    int volume_leveler_enable;
    int volume_leveler_amount;
    int volume_leveler_in_target;
    int volume_leveler_out_target;
    int volume_modeler_enable;
    int volume_modeler_calibration;
    int output_mode;
    int processing_mode;
    int nb_output_channels;
    int mix_matrix;

    int tuning;
    struct virtual_bass_tags vb;
};


/**
 * @brief XML Parser Configuration Tags
 */
typedef struct config_xml_t {
    int b_setting;
    int b_endpoint;
    int b_profile;
    int b_xml_version;
    int b_dtt_version;
    int b_constant;
    struct profiles_tags profiles;
} config_xml_tags;


/**
 * @brief Read and parse .xml parameter tuning file
 * @return Initialized DAP Configuration Structure
 */

//int
//daws_dap_xml_parser
//              ( const char *filename  /**< [in] Config XML file that needs to be parsed */
//               ,int b_dump_xml        /**< [in] If a verbose XML parsing is desired. */
//               ,unsigned sample_rate  /**< [in] DAP Supported Sample Rate */
//              ,void *p_dap_config    /**< [out] Once the XML is parsed, the values are stored in this structure */
//              );


#ifdef __cplusplus
}
#endif

#endif /* DAWS_XML_PARSER_H */

