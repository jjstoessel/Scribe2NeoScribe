**Scribe2NeoScribe**

This command line tool converts data files produced by John A. Stinson's SCRIBE software designed for encoding medieval music notation into a format compliant with the current Music Encoding Initiative Standard (http://music-encoding.org/documentation/guidelines2013).

_Requirements_

Scribe2NeoScribe requires the open source LibMEI framework. Download LibMEI from here https://github.com/DDMAL/libmei and follow the installation instruction given there. LibMEI framework requirements need to be also observed. Please note that "frbr" bibliographic module needs to be compiled into the LibMEI framework for Scribe2NeoScribe.

Scribe2NeoScribe currently uses existing MEI modules. Future releases will add support for the NeoScribe MEI Module, especially for handingly Trecento and Ars nova notation.

Scribe2NeoScribe uses Andrei Alexandrescu's Visitor template class from the Loki framework; see http://sourceforge.net/projects/loki-lib/.

Scribe2NeoScribe also requires the Standard C++ Library and compiles in the Apple LLVM 5.0 C/C++ compiler.

_Installation_

Currently Scribe2NeoScribe can be compiled either in Apple Computer's XCode 5.0 or from the commandline if XCode command line tools are install by typing "sudo xcodebuild install" after changing into the directory containing the "NeoScribe.xcodeproj" package.

_Use_

To convert any Scribe NEU file, simply type "Scribe2NeoScribe" followed by the name(s) of the Scribe file(s). If you wish to include the name of the person originally responsible for encoding the Scribe file into the NeoScribe file, add the parameter -e followed by the name of the encoder, then the file name(s).

_Notice_

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

This is alpha software. It has not been extensively tested and will undergo significant revisions in the coming months as it moves to a beta release. It currently serves to demonstrate how Scribe data might be effectively converted into MEI-compliant data for future use and enhancement. The code is written by an autodidact and music historian, thus is not guaranteed to meet the expectations of computer scientists and programmers.

J. Stoessel

