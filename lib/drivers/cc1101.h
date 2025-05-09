#pragma once

#include "cc1101_regs.h"

#include <stdbool.h>
#include <stdint.h>
#include <furi_hal_spi.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Low level API */

/** Strobe command to the device
 *
 * @param      handle  - pointer to FuriHalSpiHandle
 * @param      strobe  - command to execute
 *
 * @return     device status
 */
CC1101Status cc1101_strobe(const FuriHalSpiBusHandle* handle, uint8_t strobe);

/** Write device register
 *
 * @param      handle  - pointer to FuriHalSpiHandle
 * @param      reg     - register
 * @param      data    - data to write
 *
 * @return     device status
 */
CC1101Status cc1101_write_reg(const FuriHalSpiBusHandle* handle, uint8_t reg, uint8_t data);

/** Read device register
 *
 * @param      handle  - pointer to FuriHalSpiHandle
 * @param      reg     - register
 * @param[out] data    - pointer to data
 *
 * @return     device status
 */
CC1101Status cc1101_read_reg(const FuriHalSpiBusHandle* handle, uint8_t reg, uint8_t* data);

/* High level API */

/** Reset
 *
 * @param      handle  - pointer to FuriHalSpiHandle
 * 
 * @return     CC1101Status structure
 */
CC1101Status cc1101_reset(const FuriHalSpiBusHandle* handle);

/** Get status
 *
 * @param      handle  - pointer to FuriHalSpiHandle
 *
 * @return     CC1101Status structure
 */
CC1101Status cc1101_get_status(const FuriHalSpiBusHandle* handle);

/** Wait specific chip state
 *
 * @param      handle      The SPI bus handle
 * @param[in]  state       The state to wait
 * @param[in]  timeout_us  The timeout in microseconds
 *
 * @return     true on success, false otherwise
 */
bool cc1101_wait_status_state(
    const FuriHalSpiBusHandle* handle,
    CC1101State state,
    uint32_t timeout_us);

/** Enable shutdown mode
 *
 * @param      handle  - pointer to FuriHalSpiHandle
 * 
 * @return     CC1101Status structure
 */
CC1101Status cc1101_shutdown(const FuriHalSpiBusHandle* handle);

/** Get Partnumber
 *
 * @param      handle  - pointer to FuriHalSpiHandle
 *
 * @return     part number id
 */
uint8_t cc1101_get_partnumber(const FuriHalSpiBusHandle* handle);

/** Get Version
 *
 * @param      handle  - pointer to FuriHalSpiHandle
 *
 * @return     version
 */
uint8_t cc1101_get_version(const FuriHalSpiBusHandle* handle);

/** Get raw RSSI value
 *
 * @param      handle  - pointer to FuriHalSpiHandle
 *
 * @return     rssi value
 */
uint8_t cc1101_get_rssi(const FuriHalSpiBusHandle* handle);

/** Calibrate oscillator
 *
 * @param      handle  - pointer to FuriHalSpiHandle
 * 
 * @return     CC1101Status structure
 */
CC1101Status cc1101_calibrate(const FuriHalSpiBusHandle* handle);

/** Switch to idle
 *
 * @param      handle  - pointer to FuriHalSpiHandle
 */
CC1101Status cc1101_switch_to_idle(const FuriHalSpiBusHandle* handle);

/** Switch to RX
 *
 * @param      handle  - pointer to FuriHalSpiHandle
 * 
 * @return     CC1101Status structure
 */
CC1101Status cc1101_switch_to_rx(const FuriHalSpiBusHandle* handle);

/** Switch to TX
 *
 * @param      handle  - pointer to FuriHalSpiHandle
 * 
 * @return     CC1101Status structure
 */
CC1101Status cc1101_switch_to_tx(const FuriHalSpiBusHandle* handle);

/** Flush RX FIFO
 *
 * @param      handle  - pointer to FuriHalSpiHandle
 * 
 * @return     CC1101Status structure
 */
CC1101Status cc1101_flush_rx(const FuriHalSpiBusHandle* handle);

/** Flush TX FIFO
 *
 * @param      handle  - pointer to FuriHalSpiHandle
 */
CC1101Status cc1101_flush_tx(const FuriHalSpiBusHandle* handle);

/** Set Frequency
 *
 * @param      handle  - pointer to FuriHalSpiHandle
 * @param      value   - frequency in herz
 *
 * @return     real frequency that were synthesized
 */
uint32_t cc1101_set_frequency(const FuriHalSpiBusHandle* handle, uint32_t value);

/** Set Intermediate Frequency
 *
 * @param      handle  - pointer to FuriHalSpiHandle
 * @param      value   - frequency in herz
 *
 * @return     real inermediate frequency that were synthesized
 */
uint32_t cc1101_set_intermediate_frequency(const FuriHalSpiBusHandle* handle, uint32_t value);

/** Set Power Amplifier level table, ramp
 *
 * @param      handle  - pointer to FuriHalSpiHandle
 * @param      value   - array of power level values
 */
void cc1101_set_pa_table(const FuriHalSpiBusHandle* handle, const uint8_t value[8]);

/** Write FIFO
 *
 * @param      handle  - pointer to FuriHalSpiHandle
 * @param      data    pointer to byte array
 * @param      size    write bytes count
 *
 * @return     size, written bytes count
 */
uint8_t cc1101_write_fifo(const FuriHalSpiBusHandle* handle, const uint8_t* data, uint8_t size);

/** Read FIFO
 *
 * @param      handle  - pointer to FuriHalSpiHandle
 * @param      data    pointer to byte array
 * @param      size    bytes to read from fifo
 *
 * @return     size, read bytes count
 */
uint8_t cc1101_read_fifo(const FuriHalSpiBusHandle* handle, uint8_t* data, uint8_t* size);

#ifdef __cplusplus
}
#endif
