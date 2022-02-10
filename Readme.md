# PlayStation 2 HDD checker

This is a fork of original project: https://sites.google.com/view/ysai187/home/projects/hddchecker

## Introduction

HDDChecker is a basic disk diagnostic tool meant for testing the health of your PlayStation 2 console's Harddisk Drive unit.

It was conceived and constructed because I didn't want to see any more poor SCPH-20400 units being cut open, just to have the disks within them taken out for testing. >_>

It'll also be useful for those people who need to check the condition of their HDD units, but don't have a working IDE port on their computers to connect the disk to.

### Features

1. Detects and lists the model, serial, firmware, and S.M.A.R.T. status of HDD unit 0 (Primary Master).
2. Performs a surface scan of the disk.
3. Supports 48-bit LBA disks up to 2TB.
4. Performs zero-filling of the disk with large blocks.
5. With the surface scan, bad sectors found might be remappable[^1].
6. Checks for damage to the APA scheme and PFS partitions.
7. Optimizes the partitions on the HDD to reclaim space.

[^1]: Writing to a to-be-remapped sector (those hard-to-read sectors, as recorded by the disk) may cause them to be remapped. However, this might not be the case for all disks.

### Logging Feature

As of HDDChecker v0.96, a new logging feature has been added.

4 log files will be generated:

- When scanning is done:
  - hdck.log
  - fsck.log
- When optimization is done:
  - fssk.log
  - hdsk.log

No log files will be generated for surface scanning or zero-filling, as the purpose of these log files is to allow what was done during the scanning and optimization operations to be explained.

If you experience problems with HDDChecker's scanning and optimization functionality, please contact the developer and provide the log files.

## Notes/known limitations/bugs

- Disks up to 2 TB are supported.
- Do not use (usually old) disks that are not compliant with the ATA-4 specification. Like with every other PlayStation 2 software out there that supports the ATA interface, the disk is expected to support UDMA mode 4 and S.M.A.R.T.
- If the disk's S.M.A.R.T. status is indicated to be no good (NG status), the disk is about to fail and should be replaced.
- This tool may not be able to successfully remap sectors on all drives, as there isn't an official specification on remapping sectors within the ATA specification. If writing to a bad sector isn't sufficient to cause the disk to automatically remap it, the manufacturer's tools may have to be used instead.
- If a bad sector cannot be remapped, the disk is dying and should be replaced.
- As zeros will be written to the bad sector in an attempt to cause the disk to remap it, the data stored within the bad sector will be lost. Do not choose to remap the sector if it contains data that cannot be lost.

## Supported languages

For more information on supported languages and how support for languages can be completed, click here. A template for translating this software is provided in the downloads section of this page.

Supported languages and their translation status

- Japanese - Oga
- English - Completed and built-in.
- French - ShaolinAssassin
- Spanish - ElPatas
- German - LopoTRI
- Italian - Vash The Stampede
- Dutch - Unassigned
- Portuguese - Gledson999

## Changelog for v0.964 (as of 2019/04/13)

- Fixed glitch when rendering fonts.
- The PS2 can now be shut down when the software is idle, by pressing the power button once. This will also shut down HDDs (USB & PS2 HDD, to avoid causing some devices to emergency park.
- (NEW 2019/01/14) Added Japanese translation. Special thanks to Oga.
- (NEW 2019/01/14) Updated USBHDFSD to fix the bug causing files to be unable to be replaced properly.
- (NEW 2019/04/13) Replaced Japanese font with NotoSansJP-Bold, to reduce space wastage.
