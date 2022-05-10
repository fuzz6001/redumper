# redumper
Copyright 2021-2022 Hennadiy Brych

## Preambule
A fresh take on a CD dumper utility. The main goals are to develop a portable, byte perfect CD dumper which supports incremental dumps, advanced SCSI/C2 repair and is future proof. Everything is written from scratch in C++. The primary development platform is Windows, Linux and MacOSX versions are planned.

## General
The dumper operates using modes. The current modes are "dump", "protection", "refine", "split" and "info" but for the convenience sake, the aggregate mode "cd" does it all in a row and covers most of the use cases.

**dump**: Dumps cd and stores it in a set of files.

**protection**: Scans dump for protections.

**refine**: Improves the dump, tries to correct found SCSI/C2 errors and fill missing sectors based on a drive features, example: refining an initial LG/ASUS dump on PLEXTOR will add missing lead-in and possibly more lead-out sectors based on what originally was extracted from LG/ASUS cache. You can refine as many times as you want. You can refine the same disc on a different (but supported) drive. If you have two identical discs with different damage, you can refine from both (TOC has to be identical).

**split**: Performs track split and generates a CUE-sheet, disc is not required at this point.

**info**: Generates an info file with the specific information tailored for redump.org.

Everything is being actively developed so modes / options may change, always use --help to see the latest information.

## Technical
D8 and BE read opcodes are supported using the compatible drives (PLEXTOR and LG/ASUS respectively). Everything, including data tracks, is read as audio. On a dump stage, each sector is dumped sequentially from the start to end in a linear fashion. Known drive inaccessible / slow sectors (e.g. multisession gaps) are skipped forward but dumper never seeks back and a great care is exercised not to put an excessive wear on drive. Everything that is possible to extract will be extracted. Lead-out session overread is supported. Lead-in/pre-gap session reading is implemented by using PLEXTOR negative LBA range feature, tricks are in place to be able to extract this data for both sessions whenever possible. The main scrambled dump output prepends an extra space of 45150 sectors (10 minutes) in order to save first session lead-in (10 minutes is a maximum addressable negative MSF range that can be used to access first session lead-in). LG/ASUS cache reading is supported for extracting session lead-out. Both PLEXTOR lead-in/pre-gap and LG/ASUS lead-out are multiplexed into main scrambled dump file using absolute addressing from subchannel Q in order to minimize the chance of error.

The resulting dump is drive read offset corrected but not combined offset corrected (the disc write offset is determined at a later track split stage). Sector dump state (SUCCESS / SUCCESS_SCSI_OFF / SUCCESS_C2_OFF / ERROR_SCSI / ERROR_C2) is stored for each sample, 1 sample is 4 bytes (2 16-bit signed samples of stereo audio data) e.g. for 1 sector there are 588 state values. All this allows an incremental dump improvement with sample granularity using different drives with different read offsets.

Subchannel data is stored RAW (multiplexed). Subchannel Q is used to perform a track split but it's corrected in memory and never stored on disk. This allows to keep subchannel based protection schemes (libcrypt, SecuROM etc) for a later analysis. Disc write offset detection is calculated based on a data track as an addressing difference between data sector MSF and subchannel Q MSF. Split will fail if track sector range contains SCSI/C2 errors or inaccessible, example: track split of ASUS dump without cache lead-out data if the disc has positive write offset. 

## Examples
1. Super lazy:

`redumper`

If run without arguments, the dumper will use the first available supported drive with disc inside and "cd" aggregate mode. The image name will be autogenerated based on a date/time and drive path and will be put in the current process working directory. If you have two drives and the first drive is already busy dumping, running it again will dump the disc in the next available drive and so on, easy daisy chaining. Please take into account that no SCSI/C2 rereads will be performed on errors unless you specify --retries=100, but don't worry as it won't let you split to tracks if tracks have errors.

2. Concerned citizen:

`redumper cd --verbose --drive=F: --retries=100 --image-name=my_dump_name --image-path=my_dump_directory`

or (you can use spaces and = interchangeably)

`redumper cd --verbose --drive F: --retries 100 --image-name my_dump_name --image-path my_dump_directory`

Will dump a disc in drive F: with 100 retries count in case of errors (refine). The dump files will be stored in my_dump_name directory and dump files will have my_dump_name base name and you will get verbose messages.

3. You know what you do:

`redumper refine --verbose --drive=G: --speed=1 --retries=500 --image-name=my_dump_name --image-path=my_dump_directory`

Refine a previous dump from my_dump_directory with base name my_dump_name on a different drive using lowest speed with different retries count.

4. Advanced:

`redumper split --verbose --force --image-name=my_dump_name --image-path=my_dump_directory`

Force generation of track split with track errors, just because you really want to see and experiment with unscrambled tracks.

## Contacts
E-mail: gennadiy.brich@gmail.com

Discord: superg#9200
