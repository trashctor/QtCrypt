## QtCrypt

QtCrypt was intended as a lightweight, portable application, specifically for mobile platforms, that would encode a list of local files and directories using a symmetric-key algorithm. One of the main goals was to keep things as simple and convenient as possible so that the encryption and decryption could be done on a daily basis, safeguarding data in the event of a lost device.

The application was primarily built around the Qt 5.4.0 framework. Other dependencies include QuaZIP 0.7.1, zlib 1.2.8, used for zipping and unzipping directories, and libsodium 1.0.2, a cryptography library that does all the heavy lifting with regards to encryption, decryption, nonce generation, and key stretching. Since these libraries do not depend on anything besides Qt, they should theoretically compile on any platform that Qt supports.

To build the program from source, the appropriate Qt 5.4.0+ should be installed and configured for the target platform, including any tools such as Qt Creator.

So far, the program has been built with a MinGW 4.9.1 distribution of Qt 5.4.0 on Windows 32-bit. Stable, statically linked executables of QtCrypt and other future versions can be found in releases.

## The main source files of QtCrypt include:

**main.cpp** // entry point of program, initializes libsodium and runs the main window<br>
**mymainwindow.cpp** // contains the vast majority of UI logic for QtCrypt

**myabstractbar.cpp** // abstract class for a custom progress bar<br>
**myencryptbar.cpp** // progress bar dialog for encryption, handles the encryption thread<br>
**mydecryptbar.cpp** // progress bar dialog for decryption, handles the decryption thread<br>
**mysavebar.cpp** // progress bar dialog for saving a list of files and directories used by the model<br>
**myloadbar.cpp** // progress bar dialog for loading a list of files and directories into the model

**myencryptthread.cpp** // zips then encrypts a list of file and directories<br>
**mydecryptthread.cpp** // decrypts then unzips a list of file and directories<br>
**mysavethread.cpp** // writes the current list of items in the model to a .qtlist file<br>
**myloadthread.cpp** // reads a list of items into the current model from a .qtlist file<br>
**mydirthread.cpp** // calculates the size of a directory in the background

**mycrypt.cpp** // wrapper for the libsodium symmetric-key crypto functions<br>
**mycleardir.cpp** // used to delete all items in a given directory

**myfileinfo.cpp** // element used in model to represent a given file or directory<br>
**myfilesystemmodel.cpp** // contains, manages the list of elements for crypto, used by the view to display info about them

## License

Copyright (C) 2015 trashctor

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
