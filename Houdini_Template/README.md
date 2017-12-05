PRT ROP For Houdini

Brought to you by Robotika (robotika.com.tr) and Fiction

======

To build :

- Clone this repo 
- Open houdini Command Line Tools
- cd to this folder
- call  hcustom -I "C:\Program Files\Microsoft SDKs\Windows\v6.0A\Include" -I "C:\Program Files\Side Effects Software\Houdini 12.1.125\toolkit\include\zlib" -I "C:\Program Files\Side Effects Software\Houdini 12.1.125\toolkit\include\OpenEXR" PRT_RopDriver.cpp

Viola!


To use compiled version (12.185) :

- Clone this repo
- Checkout "compiled" branch
- Copy PRT_RopDriver.dll to Houdini DSO folder
