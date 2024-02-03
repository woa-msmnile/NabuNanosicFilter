## Nanosic Filter For Xiaomi Pad 5.

The HID description Nanosic reported to the OS has issue and will cause bsod on Windows >= 22621.
This filter only replaced a Usage in the report from 'Touch Pad' to 'Touch Screen' and the bsod gone.

## Caution
  - Map220v has release new drivers, which means the keyboard will work out of box, and you do not need to install this driver manually.
  - Map220v's Nabu's Driver [releases](https://github.com/map220v/MiPad5-Drivers/releases/)
## How to install
  1. Merge the `Disable.reg` in windows after booting with a uefi that don't have keyboard support.
  2. Reboot with a uefi that has keyboard support.
  3. Open Widnows Device Manager and expand `Human Interface Input Device` section.
  4. Find the disabled `USB Input Device`.
  5. Right click and choose update driver.
  6. Choose the second option(`Find Driver On My Disk`).
  7. Select this driver and install.
  8. Reboot.

## Reference Codes
  - [SurfaceDigitizerFilter](https://github.com/WOA-Project/SurfaceDigitizerFilter)
  - [BthPS3](https://github.com/nefarius/BthPS3)
  - [Windows Driver Samples](https://github.com/microsoft/Windows-driver-samples/)
  
## Notice
  - This driver may not be able to be installed in OOBE.

  *I don't know why this driver can not be installed automatically in OOBE stage.*  
  *One possible reason I guess, the driver is not signed properly.*
