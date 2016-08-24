'\" t
.so wrs.an
.\" simpc/target.nr - VxSim for Windows Target specific documentation
.\"
.\" Copyright 1992-2002 Wind River Systems, Inc.
.\"
.\" modification history
.\" --------------------
.\" 01e,02jan02,hbh  Updated.
.\" 01d,12nov01,jmp  updated simulator addresses.
.\" 01c,27aug01,dgp  change manual pages to reference entries per SPR 23698
.\" 01b,09apr99,cym  changed the number of simulators to 16.
.\" 01a,19aug98,cym  written.
.\"
.\"
.TH "simpc" T "VxSim for Windows NT4.0/2000/XP" "Rev: 02 Jan 02" "VXWORKS REFERENCE MANUAL"

.SH "NAME"
.aX "VxSim for Windows NT4.0/2000/XP"

.SH "INTRODUCTION"
This reference entry provides target-specific information necessary to run
the VxWorks simulator on Windows.

.SH "BOOT ROMS"
Not applicable.
.PP
To start the VxWorks simulator simply type "vxWorks" from a DOS shell, or
use VxSim icon from The Tornado Graphical User Interface.
For further information, follow the instructions in the
.I "VxSim"
chapter of the
.I "Tornado User's Guide"
and
.I "Getting Started"
chapter of the
.I "VxWorks Programmer's Guide."

.SH "JUMPERS"
Not applicable.

.SH "DEVICES"
The simulator can access keyboard input and window output through a standard
vxWorks sio driver. Because device drivers require direct hardware interaction,
most VxWorks device drivers are not available with VxSim.
.PP
FULL SIMULATOR ONLY
The VxSim full simulator provides full network capability for your simulator.
This optional product also allows you to run more than one instance of VxSim
on your host.

In order to simulate the network IP connectivity of a VxWorks target, the
VxSim full simulator includes special drivers that operate using IP addresses.
The ULIP network interface is available for Windows hosts.

All interfaces provide an I/O-based interface for IP networking that allows
VxSim processes to be addressed at the IP level. When multiple programs are
run, they can send packets to each other directly. This is because the host
hands the packets back and forth; that is, the host OS effectively becomes a
router with multiple interfaces.

`Installing ULIP on a Windows NT Host'
.nf
- From the `Start' menu select `Settings>Control Panel>Network'.
- Click the `Adapters' tab in the `Network' window, click the `Add' button, and click `Have Disk...' in the `Select Network Adaptor' window and select the "host\x86-win32\bin" directory of your Tornado installation and click `OK'.
- Click on `Ulip Virtual Adapter' and click `OK'. ULIP is added to the Network Adapters list.
- Enter an IP address of the form nnn.nnn.nnn.254 (for example, `192.168.255.254').
- Then from the `Bindings' tab select `Show bindings for all adapters', select `Ulip Virtual Adapter>WINS Client (TCP/IP)>NetBIOS Interface' and click `Disable' and `OK'.
- If you want to enable network communication between simulators and the outside world, select `Settings>Control Panel>Network' from the `Start' menu, then click the `Protocols' tab and double-click on `TCP/IP Protocol'. From the `Routing' tab of the `TCP/IP Properties' window, check `Enable IP Forwarding'. Click `OK' and `Close'.
- Restart the computer so the new settings take effect.
.fi


`Installing ULIP on a Windows 2000 Host'
.nf
- From the `Start' menu select `Settings>Control Panel>Add/Remove hardware'. This launches the driver installation wizard.
- Click the `Next' button. Then click the subsequent `Next' button. This displays the `Choose a Hardware Device' dialog box.
- From the dialog box, select the line `Add a new device' and click the `Next' button.
- Select the option `No, I want to select the hardware from a list', and click again on the `Next' button. 
- Select `Network adapters', and click on `Next'. This stage can take several minutes. The `Select Network Adapter' dialog box appears. Click on the `Have disk' button, and select the "host\x86-win32\bin" directory of your Tornado installation and click `OK'.
- Then select the line `WindRiver Ulip', and click on `Next'. A new dialog box appears, click again on `Next'.
- The `Digital Signature Not Found' dialog box appears, select `Yes'.
- Then the last dialog box shows up, ensure it confirms that the installation went right, and finally, click on the `Finish' button.
- Then starting from the Windows Desktop, right click on `My Network Places', and select `Properties'. 
- Right click on the last `Local Area Connection #' icon and select `Properties'. (You can rename this `Local Area Connection #' title with `WindRiver Ulip', select `Rename' instead of `Properties'). 
- Select the line `Internet Protocol (TCP/IP)' and click on `Properties'. (If `TCP/IP protocol' is not installed, please refer to the MS Windows help to install it).
- Enter an IP address of the form nnn.nnn.nnn.254 (for example, `192.168.255.254') and `255.255.255.0' as Subnet mask, and click on `Advanced'.
- In the `WINS' tab, select `Disable NetBIOS over TCP/IP' and click `OK', to properly close all the remaining windows.
- If you want to enable network communication between simulators and the outside world, select `Setting>Control Panel>Administrative tools' from the `Start' menu, double-click on `Services', then search for the `Routing and Remote access' service. If it is disabled, right-click on it and change the startup type to `Automatic' so it will be started at Windows startup. Click `OK' to validate your change. The service can now be started using the play icon on the main service window.
.fi


`Installing ULIP on a Windows XP Host'
.nf
- From the `Start' menu select `Settings>Control Panel>Add Hardware'. This launches the driver installation wizard. Click `Next'.
- The wizard will search for recently connected hardware, this stage can take several minutes. Then select `Yes, I have already connected the hardware', and click `Next'.
- From the `Installed hardware' list, select `Add a new hardware device', and click `Next'.
- Select `Install the hardware that I manually select from a list [Advanced]', click `Next'.
- From the `Common hardware types' list, select `Network adapters', click `Next'.
- Click `Have Disk...', and select the "host\x86-win32\bin" directory of your Tornado installation.
- Select `WindRiver ULIP', and click `Next' twice.
- The `Hardware Installation' will report that "WindRiver ULIP" has not passed Windows Logo testing to verify its compatibility with Windows XP. You can safely ignore this warning and click `Continue Anyway'.
- Then, from the `Control Panel', launch `Network Connections' and right-click `WindRiver ULIP connection' (You can rename this connection).
- From the `WindRiver ULIP Properties' panel, select `Internet Protocol (TCP/IP)', and click `Properties'.
- Check `use the Following IP address', and enter an IP address of the form nnn.nnn.nnn.254 (for example, `192.168.255.254'), and `255.255.255.0' as `Subnet mask', and click on `Advanced'.
- In the `WINS' tab, select `Disable NetBIOS over TCP/IP' and click `OK', to properly close all the remaining windows.
- If you want to enable network communication between simulators and the outside world, select `Setting>Control Panel>Administrative tools' from the `Start' menu, double-click on `Services', then search for the `Routing and Remote access' service. If it is disabled, right-click on it and change the startup type to `Automatic' so it will be started at Windows startup. Click `OK' to validate your change. The service can now be started using the play icon on the main service window.
.fi


For further information, follow the instructions in the
.I "Configuring the VxSim Full Simulator"
sub-chapter of the
.I "VxSim"
chapter of the
.I "Tornado User's Guide".

.SH "SPECIAL CONSIDERATIONS"
.PP
To exit the simulator, type alt-F4, or click on the x (close) in the corner of
the window.
.PP
VxSim can use any VxWorks file system. The default file system is the 
pass-through file system, passFs, which is unique to VxSim.
The VxWorks syntax to access a host file system is :
host:<disk>:/dir1/dir2/file
For further information, please refer to
.I "File Systems"
sub-chapter of the
.I "Integrated Simulator"
chapter of the 
.I "Tornado User's Guide."

.PP
Options may be passed to VxWorks from the command line:
.PP
.nf
    /p <proc num>  (set processor number) FULL SIMULATOR ONLY
    /r <ram bytes> (set memory size)
.fi

.SH "SEE ALSO"
.pG "Getting Started, Configuration"
and
.I "VxSim"
chapter of the
.I "Tornado User's Guide."
