

# Make some Hardware #

The Jennic modules are complete in the sense that they 'could' be used as delivered but in practice they need to be mounted on a carrier PCB that contains a few extra components such as a voltage regulator and servo connectors.  There are currently three designs for PCB's (2 rx's & 1 tx), see the [rcgroups thread](http://www.rcgroups.com/forums/showthread.php?t=801036) to download them.  The two receiver designs can also be used as transmitters.

Be warned that soldering the Jennic modules to the PCB's and adding the required components (all surface mount and very small) is not suited to a novice at soldering.

The Jennic modules and all the extra components are available from Farnell.

Jennic do sell development kits that could be used for a quick start if money is no object.

The first prototypes were made simply by soldering small wires direct to the modules, this is not recommended, but does work.

# Building the code #

The binary files will soon be added to the project but in the meantime you will have to build them (or ask on rcgroups).

## RX & TX code ##

To build the Rx and Tx code on a windows pc you will need to download and install the following in this order


JN-SW-4041 SDK Toolchain  -  eclipse and flash tools

JN-SW-4051 JenNet-IP SDK Installer   -  jn5148 support

JN-SW-4030 SDK Libraries  -  jn5139 support

JN-SW-4065 JN516x JenNet-IP SDK Libraries  -  jn5168 support

from the [Jennic Support Site](http://www.jennic.com/support/software/)

This will let you build for jn5139,jn5148 and jn5168 devices from eclipse.

You will then need to get a copy of the rx and tx source code from the Google Project [svn repository](http://rc24.googlecode.com/svn/trunk/rc24).

You can use any Subversion client you prefer, probably the easiest for users new to svn is [TortoiseSVN](http://www.tortoisesvn.org).
If you are using TortoiseSVN, right click on the c:\jennic\application directory in Windows Explorer and select SVN Checkout.  Fill the dialog in as below and click OK:

![http://www.ihopper.org/images/checkoutrc24.gif](http://www.ihopper.org/images/checkoutrc24.gif)

Then run Eclipse and select File/Import then 'Import existing project into Workspace'. Now browse to the c:\jennic\application\rc24 directory and finish.

The 'Build All' command in the Project menu should build the rx and tx bin files.

## PC Code ##

There is a pc program which is used to program the modules and configure them with some basic diagnostic info.
The code is in the [svn repository](http://rc24.googlecode.com/svn/trunk/rc24_pc) and can be downloaded in the same way as above to  a suitable location on your hard drive.

To build the code you will need a C# IDE, any of the MS Visual Studio 2008 releases will suffice, you can get a free copy of [Microsoft Visual C# 2008 Express](http://www.microsoft.com/express/vcsharp/) or use the open source [SharpDevelop 3.1](http://www.icsharpcode.net/OpenSource/SD/).  The project has been compiled succesfully with Visual Studio Professional 2008, Visual C# 2008 Express and SharpDevelop 3.1.

To build the application, start the IDE and from the menu select File | Open project and choose the rc24\_pc.sln file from the files you checked out earlier.  From the Build menu choose the appropriate build configuration (Debug or Release) and build the solution.  The resulting executable (Serial.exe) can be run straight from the IDE or you can locate the file in the Debug or Release directories as appropriate and run it from there.

# Flashing code to Modules #

The bin files can be installed on the modules either with Jennic's flash programming utility or the PC app.  You will need a serial TTL 3.3v level interface for your PC to connect it to the modules, again these can be obtained from Farnell.
[FTDI usb serial cable](http://uk.farnell.com/jsp/displayProduct.jsp?sku=1329311&CMP=KNC-GUK-FUK-GEN-SKU-OTH)
[Pololu usb serial if](http://www.pololu.com/catalog/product/391)

You will need to know the Com Port used by your interface.

These instructions are for the PC app, connect either the tx or rx to the pc with the serial interface connected to the appropriate pins on the PCB.

Start the app, and from the Tools menu, select the 'Options...' item.  Using the file browser buttons select the appropriate bin files for the modules, and the correct COM Port.  Click 'OK' to close the dialog which will save the settings.

On the main window if the button next to the port droplist shows as 'Connect' then click it, else click it once to make it display 'Connect' and then click it again to connect.  This ensures a connection to the module via the serial port.

The first time you flash the modules they need to be put in 'bootloader mode'. This is done by connecting the miso pin on the module to ground and powering up, the short to ground should then be removed. The 'bootload prog tx' or 'bootload prog rx' buttons can then be used to program the module.  After initial programming, the module needs to be powered down and up to run the new software. You should see messages displayed in the text box at the bottom of the app when you power up the programmed modules.

Further reprogramming does not require the module to be put in 'bootloader mode' as the rc24 software has code update built in (over the radio link if needed). To do this click the 'Refresh Tree' button and the tree display should show the PC node, click the '+' expander and the module should then be listed.  Select the module in the tree and the 'Update Code' button should be displayed in the right hand info pane for the node.  Click the button and the upload should start and when complete reset the module automatically when complete.

With a programmed tx module connected you should see something like this

![http://www.ihopper.org/images/serialtx.gif](http://www.ihopper.org/images/serialtx.gif)

# Configure modules #

Once the modules are programmed the pc config program should be used to configue the software for the particular hardware.  The key settings are 'Hardware Type' and 'High power module' for the rx and 'Default input'  and 'High power module' for the tx.  Once set, click the 'save settings' button then the 'reset node' button.

By default serial rx0 and tx0 are used for programming and debugging. These are shared with servo outputs 3&4 on the GB 8ch rx and 1&2 on the 6ch rx.  To enable these for servo output set 'radioDebug' to true on the rx.

On the tx, to use analog inputs from the adcs, set the 'DefaultInput' to custom.

# Binding #

Binding is a little messy at the moment!
The rx accepts bind requests between 10 and 100 seconds after powerup( assuming it doesn't get a signal from an already bound tx).  If the rx has an led it should flash slowly for the first 10s then speed up when in bind mode.  Once bound it appears mainly on but flickers.
Binding on the tx is started from the real or virtual touch screen.  Click the up arrow twice and a page with the word 'bind' should appear.  Click the bind button.
The rx autmatically saves the binding, for the tx click the 'save settings button'