#ifndef MATHIS_TEMP_CONFIG_H
#define MATHIS_TEMP_CONFIG_H

/* Existing acquisition thresholds, in the current measurement domain.
 * Relocation only: do not reinterpret these as confirmed pack voltages. */

#define PROBE_TEMP_C_LOW_LIMIT   (0)
#define PROBE_TEMP_C_HIGH_LIMIT  (500)
#define PROBE_TEMP_C_HIGH_RELEASE (PROBE_TEMP_C_HIGH_LIMIT)
#define PROBE_TEMP_HI_CONFIRM_COUNT (3u)
#define PROBE_TEMP_HI_RELEASE_CONFIRM_COUNT (2u)
#define PROBE_TEMP_LO_CONFIRM_COUNT (3u)
#define PROBE_TEMP_LO_RELEASE_CONFIRM_COUNT (2u)
#define PROBE_DISCONNECT_CONFIRM_COUNT (3u)
#define PROBE_RECONNECT_CONFIRM_COUNT (2u)
#define BATTERY_LOW_MV_THRESHOLD   (1100u)
#define BATTERY_RECOVER_MV_THRESHOLD (1200u)
#define BATTERY_DEBOUNCE_COUNT     (20u)
/* TempGetTask runs every 500 ms: 120 samples form a 60-second window. */
#define BATTERY_ADC_AVERAGE_SAMPLES (120u)

#endif
