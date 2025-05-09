#pragma once

#include <furi_hal_spi_config.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Early initialize SPI HAL */
void furi_hal_spi_config_init_early(void);

/** Early deinitialize SPI HAL */
void furi_hal_spi_config_deinit_early(void);

/** Initialize SPI HAL */
void furi_hal_spi_config_init(void);

/** Initialize SPI DMA HAL */
void furi_hal_spi_dma_init(void);

/** Initialize SPI Bus
 *
 * @param      handle  pointer to FuriHalSpiBus instance
 */
void furi_hal_spi_bus_init(FuriHalSpiBus* bus);

/** Deinitialize SPI Bus
 *
 * @param      handle  pointer to FuriHalSpiBus instance
 */
void furi_hal_spi_bus_deinit(FuriHalSpiBus* bus);

/** Initialize SPI Bus Handle
 *
 * @param      handle  pointer to FuriHalSpiBusHandle instance
 */
void furi_hal_spi_bus_handle_init(const FuriHalSpiBusHandle* handle);

/** Deinitialize SPI Bus Handle
 *
 * @param      handle  pointer to FuriHalSpiBusHandle instance
 */
void furi_hal_spi_bus_handle_deinit(const FuriHalSpiBusHandle* handle);

/** Acquire SPI bus
 *
 * @warning blocking, calls `furi_crash` on programming error, CS transition is up to handler event routine
 *
 * @param      handle  pointer to FuriHalSpiBusHandle instance
 */
void furi_hal_spi_acquire(const FuriHalSpiBusHandle* handle);

/** Release SPI bus
 *
 * @warning calls `furi_crash` on programming error, CS transition is up to handler event routine
 * 
 * @param      handle  pointer to FuriHalSpiBusHandle instance
 */
void furi_hal_spi_release(const FuriHalSpiBusHandle* handle);

/** SPI Receive
 *
 * @param      handle   pointer to FuriHalSpiBusHandle instance
 * @param      buffer   receive buffer
 * @param      size     transaction size (buffer size)
 * @param      timeout  operation timeout in ms
 *
 * @return     true on sucess
 */
bool furi_hal_spi_bus_rx(
    const FuriHalSpiBusHandle* handle,
    uint8_t* buffer,
    size_t size,
    uint32_t timeout);

/** SPI Transmit
 *
 * @param      handle   pointer to FuriHalSpiBusHandle instance
 * @param      buffer   transmit buffer
 * @param      size     transaction size (buffer size)
 * @param      timeout  operation timeout in ms
 *
 * @return     true on success
 */
bool furi_hal_spi_bus_tx(
    const FuriHalSpiBusHandle* handle,
    const uint8_t* buffer,
    size_t size,
    uint32_t timeout);

/** SPI Transmit and Receive
 *
 * @param      handle     pointer to FuriHalSpiBusHandle instance
 * @param      tx_buffer  pointer to tx buffer
 * @param      rx_buffer  pointer to rx buffer
 * @param      size       transaction size (buffer size)
 * @param      timeout    operation timeout in ms
 *
 * @return     true on success
 */
bool furi_hal_spi_bus_trx(
    const FuriHalSpiBusHandle* handle,
    const uint8_t* tx_buffer,
    uint8_t* rx_buffer,
    size_t size,
    uint32_t timeout);

/** SPI Transmit and Receive with DMA
 *
 * @param      handle     pointer to FuriHalSpiBusHandle instance
 * @param      tx_buffer  pointer to tx buffer
 * @param      rx_buffer  pointer to rx buffer
 * @param      size       transaction size (buffer size)
 * @param      timeout_ms operation timeout in ms
 *
 * @return     true on success
 */
bool furi_hal_spi_bus_trx_dma(
    const FuriHalSpiBusHandle* handle,
    uint8_t* tx_buffer,
    uint8_t* rx_buffer,
    size_t size,
    uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif
