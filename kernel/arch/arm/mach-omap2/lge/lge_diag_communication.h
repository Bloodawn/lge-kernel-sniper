/*
 *	 arch/arm/mach-omap2/lge/lge_diag_communication.h
 *
 *   the use of another function to send framework.
*/
#ifndef	LG_FW_DIAG_H
#define	LG_FW_DIAG_H

#include <asm/ioctl.h>

struct diagcmd_dev {
	const char	*name;
	struct device	*dev;
	int		index;
	int		state;
};

struct diagcmd_platform_data {
	const char *name;
};

extern int diagcmd_dev_register(struct diagcmd_dev *sdev);
extern void diagcmd_dev_unregister(struct diagcmd_dev *sdev);

static inline int diagcmd_get_state(struct diagcmd_dev *sdev)
{
	return sdev->state;
}

struct diag_cmd_data{
	unsigned short sub_cmd_code;
	int state;
};

/* LGE_MERGE_S [sunmyoung.lee@lge.com] 2010-07-15. SMS UTS Test */
typedef struct
{
	unsigned short set_data[1024];
}udm_sms_status_new;
/* LGE_MERGE_S [sunmyoung.lee@lge.com] 2010-07-15. SMS UTS Test */

typedef enum
{
  TEST_MODE_VERSION=0,
  TEST_MODE_LCD=1,
  TEST_MODE_FOLDER,
  TEST_MODE_MOTOR=3,
  TEST_MODE_ACOUSTIC=4,
  TEST_MODE_MIDI,
  TEST_MODE_VOD,
  TEST_MODE_CAM=7,
  TEST_MODE_BUZZER,
  TEST_MODE_FACTORY_INIT=10,
  TEST_MODE_EFS_INTEGRITY=11, TEST_MODE_TX_POWER,
  TEST_MODE_IRDA_FMRT_FINGER_UIM_TEST=13,
  TEST_MODE_IC_CARD,
  TEST_MODE_M_FORMAT,
  TEST_MODE_PHONE_CLEAR,
  TEST_MODE_LIGHT_SENSOR=18,
  TEST_MODE_WIPI,
  TEST_MODE_BREW_CNT=20,
  TEST_MODE_BREW_SIZE,  
  TEST_MODE_KEY_TEST=22,    
  TEST_MODE_EXT_SOCKET_TEST=23,
  TEST_MODE_BLUETOOTH_TEST=24,
  TEST_MODE_BATT_LEVEL_TEST=25,
  TEST_MODE_ANT_LEVEL_TEST,
  TEST_MODE_MP3_TEST=27,
  TEST_MODE_FM_TRANCEIVER_TEST=28,
  TEST_MODE_ISP_DOWNLOAD=29, 
  TEST_MODE_COMPASS_SENSOR_TEST=30,     // Geometric (Compass) Sensor      
  TEST_MODE_ACCEL_SENSOR_TEST=31,           
  TEST_MODE_ALCOHOL_SENSOR_TEST=32,           
  TEST_MODE_TDMB_TEST=33,           
  TEST_MODE_TV_OUT_TEST=34,           
  TEST_MODE_SDMB_TEST=35,           
  TEST_MODE_MANUAL_MODE_TEST=36,   // Manual test        
  TEST_MODE_UV_SENSOR_TEST=37,           
  TEST_MODE_REMOVABLE_DISK_TEST=38,           
  TEST_MODE_3D_ACCELERATOR_SENSOR_TEST=39,           
  TEST_MODE_KEY_DATA_TEST = 40,  // Key Code Input
  TEST_MODE_MEMORY_VOLUME_TEST=41,  // Memory Volume Check
  TEST_MODE_SLEEP_MODE_TEST=42,
  TEST_MODE_SPEAKER_PHONE_TEST=43,  // Speaker Phone test

  TEST_MODE_VIRTUAL_SIM_TEST=44,
  TEST_MODE_PHOTO_SENSOR_TEST=45,
  TEST_MODE_VCO_SELF_TUNNING_TEST=46,

  TEST_MODE_MRD_USB_TEST=47,
  TEST_MODE_TEST_SCRIPT_MODE=48,
  TEST_MODE_PROXIMITY_SENSOR_TEST=49,
  TEST_MODE_FACTORY_RESET_CHECK_TEST=50, 
  TEST_MODE_VOLUME_TEST=51,

  TEST_MODE_HFA_TEST=52,
  TEST_MODE_MOBILE_SYSTEM_TEST=53,
  TEST_MODE_STANDALONE_GPS_TEST=54,
  TEST_MODE_PRELOAD_INTEGRITY_TEST=55,
  TEST_MODE_USB_PATH_CHANGE_TEST=56,
  TEST_MODE_MEMORY_BAD_BLOCK_CHANGE_TEST=57,
  TEST_MODE_FIRST_BOOTING_CHECK_TEST=58,
  TEST_MODE_MAX_POWER_STATE_TEST=59,
  TEST_MODE_LED_TEST=60,
  TEST_MODE_LTE_RF_PATH_TEST=61,
  TEST_MODE_MIMO_ANTENNA_TEST=62,
  TEST_MODE_MIMO_RF_TEST=63,
  TEST_MODE_LTE_CALL_TEST=64,
  TEST_MODE_CHANGE_USB_DRIVER_TEST=65,
  TEST_MODE_PID_TEST=70,
  TEST_MODE_SW_VERSION_TEST=71,
  TEST_MODE_IMEI_TEST=72,
  TEST_MODE_IMPL_TEST=73,
  TEST_MODE_SIM_LOCK_TEST=74,
  TEST_MODE_UNLOCK_CODE_TEST=75,
  TEST_MODE_IDDE_TEST=76,
  TEST_MODE_FULL_SIGNATURE_TEST=77,
  TEST_MODE_NT_CODE_TEST=79,
  TEST_MODE_SIM_ID_TEST=81,
  TEST_MODE_CAL_CHECK_TEST=82,
  TEST_MODE_BLUETOOTH_RW_TEST=83,
  TEST_MODE_SKIP_WELCOM_TEST=87,
  TEST_MODE_WIFI_MAC_TEST=88,
  TEST_MODE_BOOT_CODE_PROTECTION_TEST=89,
  TEST_MODE_MDTV_TEST=90,
  TEST_MODE_DB_INTEGRITY_TEST=91,
  TEST_MODE_NV_CRC_TEST=92,
  TEST_MODE_GYRO_SENSOR_TEST=93,
  TEST_MODE_RELEASE_CURRENT_LIMIT_TEST=94,
  TEST_MODE_PVK_TEST=95,
  TEST_MODE_AI_INIT_TEST=96,
// ----------------------------------------------------------
  MAX_TEST_MODE_SUBCMD = 0xFFFF
} test_mode_sub_cmd_type;

typedef enum
{
	/** SAR : Sprint Automation Requirement - START **/
	ICD_GETDEVICEINFO_REQ_IOCTL						=0x71,	//Auto-025, Auto-027, Auto-030, Auto-040
	ICD_EXTENDEDVERSIONINFO_REQ_IOCTL					=0x72,	//Auto-222, Auto-223
	ICD_HANDSETDISPLAYTEXT_REQ_IOCTL					=0x73,	//Auto-224, Auto-225
	ICD_CAPTUREIMAGE_REQ_IOCTL						=0x74,	//Auto-015, Auto-226
	/** SAR : Sprint Automation Requirement - END **/

	/** ICDR : ICD Implementation Recommendation  - START **/
	ICD_GETAIRPLANEMODE_REQ_IOCTL						=0x80,	//Auto-016
	ICD_SETAIRPLANEMODE_REQ_IOCTL						=0x81,	//Auto-051
	ICD_GETBACKLIGHTSETTING_REQ_IOCTL					=0x82,	//Auto-017
	ICD_SETBACKLIGHTSETTING_REQ_IOCTL					=0x83,	//Auto-052
	ICD_GETBATTERYCHARGINGSTATE_REQ_IOCTL				=0x84,	//Auto-018
	ICD_SETBATTERYCHARGINGSTATE_REQ_IOCTL				=0x85,	//Auto-054
	ICD_GETBATTERYLEVEL_REQ_IOCTL						=0x86,	//Auto-019
	ICD_GETBLUETOOTHSTATUS_REQ_IOCTL					=0x87,	//Auto-020
	ICD_SETBLUETOOTHSTATUS_REQ_IOCTL					=0x88,	//Auto-053
	ICD_GETGPSSTATUS_REQ_IOCTL						=0x89,	//Auto-024
	ICD_SETGPSSTATUS_REQ_IOCTL						=0x8A,	//Auto-055
	ICD_GETKEYPADBACKLIGHT_REQ_IOCTL					=0x8B,	//Auto-026
	ICD_SETKEYPADBACKLIGHT_REQ_IOCTL					=0x8C,	//Auto-056
	ICD_GETROAMINGMODE_REQ_IOCTL						=0x90,	//Auto-037
	ICD_GETSTATEANDCONNECTIONATTEMPTS_REQ_IOCTL		=0x92,	//Auto-042
	ICD_GETUISCREENID_REQ_IOCTL						=0x93,	//Auto-204 ~ Auto214
	ICD_GETWIFISTATUS_REQ_IOCTL						=0x95,	//Auto-045
	ICD_SETWIFISTATUS_REQ_IOCTL						=0x96,	//Auto-059
	ICD_SETDISCHARGING_REQ_IOCTL						=0x97,	
	ICD_SETSCREENORIENTATIONLOCK_REQ_IOCTL			=0x98,	//
	ICD_GETRSSI_REQ_IOCTL								=0x99,	//Auto-038

	ICD_GETUSBDEBUGSTATUSSTATUS_REQ_IOCTL				=0xA0,	
	ICD_SETUSBDEBUGSTATUSSTATUS_REQ_IOCTL				=0xA1,
	ICD_GETLATITUDELONGITUDEVALUES_REQ_IOCTL			=0xA2,
	ICD_GETSCREENLOCKSTATUS_REQ_IOCTL					=0xA3,
	ICD_SETSCREENLOCKSTATUS_REQ_IOCTL					=0xA4,
	
	/** ICDR : ICD Implementation Recommendation  - END **/

}icd_sub_cmd_type;

extern void update_diagcmd_state(struct diagcmd_dev *sdev, char *cmd, int state);
extern struct diagcmd_dev *diagcmd_get_dev(void);
#define DIAG_DRIVER 	'D'

#define DIAG_IOCTL_UPDATE 	_IOW(DIAG_DRIVER, 0x01, struct diag_cmd_data )

#endif	// LG_FW_DIAG_H 
