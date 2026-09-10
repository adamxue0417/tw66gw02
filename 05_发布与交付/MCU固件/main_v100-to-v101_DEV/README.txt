Mathis development OTA v100 -> v101
DEVELOPMENT ONLY - RSA verification and anti-rollback are disabled.

Factory_Flash: full-chip erase, then program mathis_factory_v100.hex with Keil.
App_Upload: bundle mathis_ota_v101.ota and use the adjacent manifest values.
Debug_Only: relocated application-only images; never use these as factory images or App artifacts.