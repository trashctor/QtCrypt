# Change Log

## [2.0.1] - 2015-6-22
### Fixed
- Fixed bug related to not creating a temp directory and session.qtlist file if they didn't exist.

## [2.0.0] - 2015-6-22
### Fixed
- Fixed issue with modifying blocks of data within encrypted file, this will cause backwards incompatibility with files encrypted by previous version of QtCrypt.
- Added and tightened up error checking in encryption, decryption.
- Guaranteed little endian for reading and writing integers used to store file size, filename length for encryption, decryption functions.
- Encrypted the file size and filename inside.
- Refactored some code and fixed small bug in decryption thread due to not checking if the decrypted file was a zip.
- Changed decryption results from the progress bar to be more accurate.

## [1.0.0] - 2015-6-15
### Added
- First stable release of QtCrypt, all features have been checked and seem to be working. Tests were done for executable compiled on Windows 32-bit with the MinGW 4.9.1 distribution of Qt 5.4.0.
