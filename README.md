Scribe2NeoScribe

This command line tool converts data files produced by John A. Stinson's SCRIBE software designed for encoding medieval music notation in a format compliant with the current Music Encoding Initiative Standard.

Requirements

Scribe2NeoScribe requires the open source LibMEI framework. Download LibMEI from here https://github.com/DDMAL/libmei and follow the installation instruction given there. LibMEI framework requirements need to be also observed.

Installation

Currently Scribe2NeoScribe can be compiled either in Apple Computer's XCode 5.0 or from the commandline if XCode command line tools are install by typing "sudo xcodebuild install" after changing into the directory containing the "NeoScribe.xcodeproj" package.

Use

To convert any Scribe NEU file, simply type "Scribe2NeoScribe" followed by the name(s) of the Scribe file(s). If you wish to include the name of the person originally responsible for encoding the Scribe file into the NeoScribe file, add the parameter -e followed by the name of the encoder, then the file name(s).

Notice

This is Delta software. It has not been extensively tested and will undergo significant revisions in the coming months. It currently serves to demonstrate how Scribe Software might be effectively converted into MEI-compliant data for future use and enhancement. The code is written by an autodidact and music historian, thus is not guaranteed to meet the expectations of computer scientists and programmers.

Jason Stoessel

