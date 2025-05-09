#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <power/power_service/power_settings.h>
#include <core/pubsub.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RECORD_POWER "power"

typedef struct Power Power;

typedef enum {
    PowerBootModeNormal,
    PowerBootModeDfu,
    PowerBootModeUpdateStart,
} PowerBootMode;

typedef enum {
    PowerEventTypeStopCharging,
    PowerEventTypeStartCharging,
    PowerEventTypeFullyCharged,
    PowerEventTypeBatteryLevelChanged,
} PowerEventType;

typedef union {
    uint8_t battery_level;
} PowerEventData;

typedef struct {
    PowerEventType type;
    PowerEventData data;
} PowerEvent;

typedef struct {
    bool gauge_is_ok;
    bool is_charging;
    bool is_shutdown_requested;
    bool is_otg_enabled;

    float current_charger;
    float current_gauge;

    float voltage_battery_charge_limit;
    float voltage_charger;
    float voltage_gauge;
    float voltage_vbus;

    uint32_t capacity_remaining;
    uint32_t capacity_full;

    float temperature_charger;
    float temperature_gauge;

    uint8_t charge;
    uint8_t health;
} PowerInfo;

/** Power off device
 */
void power_off(Power* power);

/** Reboot device
 *
 * @param mode      PowerBootMode
 */
void power_reboot(Power* power, PowerBootMode mode);

/** Get power info
 *
 * @param power     Power instance
 * @param info      PowerInfo instance
 */
void power_get_info(Power* power, PowerInfo* info);

/** Get power event pubsub handler
 *
 * @param power     Power instance
 *
 * @return          FuriPubSub instance
 */
FuriPubSub* power_get_pubsub(Power* power);

/** Check battery health
 *
 * @return          true if battery is healthy
 */
bool power_is_battery_healthy(Power* power);

/** Enable or disable battery low level notification message
 *
 * @param power     Power instance
 * @param enable    true - enable, false - disable
 */
void power_enable_low_battery_level_notification(Power* power, bool enable);

/** Trigger UI update for changing battery layout
 *
 * @param power     Power instance
 */
void power_trigger_ui_update(Power* power);

// get settings from service to app
void power_api_get_settings(Power* instance, PowerSettings* settings);

// set settings from app to service
void power_api_set_settings(Power* instance, const PowerSettings* settings);

/** Enable or disable OTG
 *
 * @param power     Power instance
 * @param enable    true - enable, false - disable
 */
void power_enable_otg(Power* power, bool enable);

/** Check OTG status
 * 
 * @return          true if OTG  is requested
 */
bool power_is_otg_enabled(Power* power);

#ifdef __cplusplus
}
#endif
