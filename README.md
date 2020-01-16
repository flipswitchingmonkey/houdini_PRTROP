Installation is straight forward: just download the zip from the Releases page on the repo, then unzip the .dll file into your houdini's profile dso folder. On Windows that folder is most likely somewhere like this: 

``c:\Users\yourusername\Documents\houdini18.0\dso\``

If the "dso" folder does not exist yet, just create it. Then drop the PRT_RopDriver.dll in there and start Houdini. You can then create a PRT_ROPDriver ROP node in the Outputs.

---

This is a fork of Robotika's PRT ROP to keep up with the latest Houdini version and changes.

In most cases going with Alembic is probably the easier solution, but there are still a few cases and workflows that require PRT files. So, hopefully, this will be useful to you.

Please note: since I personally rarely use PRT any more, not a lot of testing goes into these builds. They come "as is" - please open an Issue if there are, well, issues, and I'll see what I can do.

### Original Readme:

PRT ROP For Houdini

Brought to you by Robotika (robotika.com.tr) and Fiction

======

To build :

- Clone this repo 
- Open houdini Command Line Tools
- cd to this folder
- call hcustom -I "C:\Program Files(x86)\Microsoft SDKs\Windows\v10.0A\Include" -I "c:\Program Files\Side Effects Software\Houdini 18.0.287\toolkit\include\zlib" -I "c:\Program Files\Side Effects Software\Houdini 18.0.287\toolkit\include\OpenEXR" Houdini_Template/PRT_RopDriver.cpp

If hcustom can't find MSVC, you have to set the environment variable to something like:
MSVCDir=c:\Program Files (x86)\Microsoft Visual Studio\2017\BuildTools\VC\Tools\MSVC\14.16.27023

To use compiled version:

- Download release for your version and unzip into Documents/houdinigXX.0/dso folder

