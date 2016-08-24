/* 10usb.cdf - Universal Serial Bus component description file */

/* Copyright 1984-2002 Wind River Systems, Inc.  */


/*
modification history
--------------------
01m,08may02,wef  SPR #77048 - removed INCLUDE_OHCI_PCI_INIT component.
01l,27mar02,wef  SPR 74822: fixed typeo in INCLUDE_AUDIO_DEMO component.
01k,26mar02,pmr  SPR 73970: removed INCLUDE_PCI requirement from
                 INCLUDE_OHCI_PCI_INIT.
01j,22mar02,rhe  Remove unwanted tab in USB Audio Demo SPR 73326
01i,08dec01,dat  Adding BSP_STUBS, INIT_BEFORE,
01h,08dec01,wef  removed references to ACM, KLSI, NC1080 and UGL components /
		 parameters, added new PEGASUS parameters.
01g,25apr01,wef  moved end initialization to usrIosExtraInit
01f,25apr01,wef  added communication, mass storage class, ugl drivers, moved
	 	 usb init order to usrIosCoreInit
01f,01may00,wef  removed usbTargTool
01e,25apr00,wef  added usbTargTool for testing the USB Target stack
01d,29mar00,wef  broke up usrUsbPciLib.c into three separtate 
		 stub<bsp>PciLib.c files included in bsp conponent
01c,10feb00,wef  cleaned-up component description and removed init groups.
01b,09feb00,wef  added component INCLUDE_OHCI_INIT.  initializes OHCI on PCI 
		 bus, fixed bug - two instances of MODULE instead of MODULES
01a,25jan00,wef  written
*/

/*
DESCRIPTION
This file contains descriptions for the USB components.
*/


/* Generic USB configuration parameters */


Folder	FOLDER_USB_HOST {
	NAME		USB Hosts
	SYNOPSIS	Universal Serial Bus Host Components
	_CHILDREN	FOLDER_BUSES
	CHILDREN	INCLUDE_USB			\
			INCLUDE_OHCI			\
			INCLUDE_UHCI
}

Folder	FOLDER_USB_TARG {
	NAME		USB Target Stack
	SYNOPSIS	Universal Serial Bus Target Stack Components
	_CHILDREN	FOLDER_BUSES
	CHILDREN	INCLUDE_USB_TARG		\
			INCLUDE_KBD_EMULATOR		\
			INCLUDE_PRN_EMULATOR		\
			INCLUDE_D12_EMULATOR
}


Folder	FOLDER_USB_DEVICES {
	NAME		USB Devices
	SYNOPSIS	Universal Serial Bus Devices
	_CHILDREN	FOLDER_PERIPHERALS
	CHILDREN	INCLUDE_USB_MOUSE		\
			INCLUDE_USB_KEYBOARD		\
			INCLUDE_USB_PRINTER		\
			INCLUDE_USB_SPEAKER		\
			INCLUDE_USB_PEGASUS_END		\
			INCLUDE_USB_MS_BULKONLY		\
			INCLUDE_USB_MS_CBI
}

Folder  FOLDER_USB_HOST_INIT {
        NAME            USB Host Init
        SYNOPSIS        Universal Serial Bus Host Component Initialization
        _CHILDREN       FOLDER_USB_HOST
        CHILDREN        INCLUDE_USB_INIT		\
			INCLUDE_UHCI_INIT		\
			INCLUDE_OHCI_INIT		\
			INCLUDE_USBTOOL			\
			INCLUDE_USB_AUDIO_DEMO
}

Folder  FOLDER_USB_DEVICE_INIT {
        NAME            USB Device Init
        SYNOPSIS        Universal Serial Bus Device Component Initialization
        _CHILDREN       FOLDER_USB_DEVICES
        CHILDREN        INCLUDE_USB_MOUSE_INIT  	\
			INCLUDE_USB_KEYBOARD_INIT	\
			INCLUDE_USB_PRINTER_INIT	\
			INCLUDE_USB_SPEAKER_INIT	\
			INCLUDE_USB_MS_BULKONLY_INIT	\
			INCLUDE_USB_MS_CBI_INIT		\
			INCLUDE_USB_PEGASUS_END_INIT
}

Component INCLUDE_USB {
	NAME		USB Host Stack
	SYNOPSIS	USB Host Stack
	MODULES		usbdLib.o
	BSP_STUBS	usbPciStub.c
}


Component INCLUDE_USB_TARG {
	NAME		USB Peripheral Stack
	SYNOPSIS	USB Peripheral Stack
	MODULES		usbTargLib.o
	BSP_STUBS	usbPciStub.c
}


Component INCLUDE_USB_INIT {
        NAME            USB Host Stack Init
        SYNOPSIS        USB Host Stack Initialization
        REQUIRES        INCLUDE_USB
        EXCLUDES        INCLUDE_USBTOOL
        CONFIGLETTES    usrUsbInit.c
        PROTOTYPE       STATUS usbInit (void);
        INIT_RTN        usbInit ();
	_INIT_ORDER     usrRoot
	INIT_AFTER      INCLUDE_NETWORK
	INIT_BEFORE	usrToolsInit
}

Component INCLUDE_UHCI {
	NAME		UHCI
	SYNOPSIS	Universal Host Controller Interface
	MODULES		usbHcdUhciLib.o
	REQUIRES	INCLUDE_USB
}

Component INCLUDE_UHCI_INIT {
        NAME            UHCI Init
        SYNOPSIS        Universal Host Controller Interface Initialization
        REQUIRES        INCLUDE_UHCI \
                        INCLUDE_USB_INIT
        EXCLUDES        INCLUDE_USBTOOL
        CONFIGLETTES    usrUsbHcdUhciInit.c
        PROTOTYPE       STATUS usrUsbHcdUhciAttach (void);
        INIT_RTN        usrUsbHcdUhciAttach ();
	_INIT_ORDER     usrRoot
        INIT_AFTER      INCLUDE_USB_INIT
	INIT_BEFORE	usrToolsInit
}

Component INCLUDE_OHCI {
	NAME		OHCI
	SYNOPSIS	Open Host Controller Interface
	MODULES		usbHcdOhciLib.o
	REQUIRES	INCLUDE_USB
}

Component INCLUDE_OHCI_INIT {
        NAME            OHCI Init
        SYNOPSIS        Open Host Controller Interface
        REQUIRES        INCLUDE_OHCI \
                        INCLUDE_USB_INIT
        EXCLUDES        INCLUDE_USBTOOL
        CONFIGLETTES    usrUsbHcdOhciInit.c
        PROTOTYPE       STATUS usrUsbHcdOhciAttach (void);
        INIT_RTN        usrUsbHcdOhciAttach ();
	_INIT_ORDER     usrRoot
        INIT_AFTER      INCLUDE_USB_INIT
	INIT_BEFORE	usrToolsInit
}

Component INCLUDE_USBTOOL {
	NAME		usbTool
	SYNOPSIS	USB Tool - Test Application for Host Stack
	CONFIGLETTES	usrUsbTool.c
        EXCLUDES        INCLUDE_USB_INIT		\
                        INCLUDE_UHCI_INIT       	\
                        INCLUDE_OHCI_INIT       	\
                        INCLUDE_USB_MOUSE_INIT  	\
                        INCLUDE_USB_KEYBOARD_INIT       \
                        INCLUDE_USB_PRINTER_INIT        \
                        INCLUDE_USB_SPEAKER_INIT

}

Component INCLUDE_USB_AUDIO_DEMO {
	NAME		USB Audio Demo
	SYNOPSIS	USB Audio Demo for Host Stack
	REQUIRES	INCLUDE_USB \
			INCLUDE_USB_SPEAKER
	CONFIGLETTES	usrUsbAudioDemo.c
	PROTOTYPE	void usrUsbAudioDemo (void);
        INIT_RTN        usrUsbAudioDemo ();
        _INIT_ORDER     usrRoot
        INIT_AFTER      INCLUDE_USB_INIT
	INIT_BEFORE	usrToolsInit
	EXCLUDES	INCLUDE_USBTOOL \
			INCLUDE_USB_SPEAKER_INIT
}

Component INCLUDE_USB_MOUSE {
	NAME		Mouse 
	SYNOPSIS	USB Mouse Driver
	MODULES		usbMouseLib.o
	REQUIRES	INCLUDE_USB
}

Component INCLUDE_USB_MOUSE_INIT {
        NAME            Mouse Init
        SYNOPSIS        USB Mouse Driver Initialization
        REQUIRES        INCLUDE_USB_MOUSE \
                        INCLUDE_USB_INIT
        EXCLUDES        INCLUDE_USBTOOL
        CONFIGLETTES    usrUsbMseInit.c
        PROTOTYPE       void usrUsbMseInit (void);
        INIT_RTN        usrUsbMseInit ();
        _INIT_ORDER     usrRoot
        INIT_AFTER      INCLUDE_USB_INIT
	INIT_BEFORE	usrToolsInit
}

Component INCLUDE_USB_KEYBOARD {
	NAME		Keyboard
	SYNOPSIS	USB Keyboard Driver
	MODULES		usbKeyboardLib.o
	REQUIRES	INCLUDE_USB
}

Component INCLUDE_USB_KEYBOARD_INIT {
        NAME            Keyboard Init
        SYNOPSIS        USB Keyboard Driver Initialization
        REQUIRES        INCLUDE_USB_KEYBOARD \
                        INCLUDE_USB_INIT
        EXCLUDES        INCLUDE_USBTOOL
        CONFIGLETTES    usrUsbKbdInit.c
        PROTOTYPE       void usrUsbKbdInit (void);
        INIT_RTN        usrUsbKbdInit ();
        _INIT_ORDER     usrRoot
        INIT_AFTER      INCLUDE_USB_INIT
	INIT_BEFORE	usrToolsInit
}


Component INCLUDE_USB_PRINTER {
	NAME		Printer
	SYNOPSIS	USB printer Driver
	MODULES		usbPrinterLib.o	
	REQUIRES	INCLUDE_USB
}

Component INCLUDE_USB_PRINTER_INIT {
        NAME            Printer Init
        SYNOPSIS        USB Printer Driver Initialization
        REQUIRES        INCLUDE_USB_PRINTER \
                        INCLUDE_USB_INIT
        EXCLUDES        INCLUDE_USBTOOL
        CONFIGLETTES    usrUsbPrnInit.c
        PROTOTYPE       void usrUsbPrnInit (void);
        INIT_RTN        usrUsbPrnInit ();
        _INIT_ORDER     usrRoot
        INIT_AFTER      INCLUDE_USB_INIT
	INIT_BEFORE	usrToolsInit
}

Component INCLUDE_USB_SPEAKER {
	NAME		Speaker
	SYNOPSIS	USB Printer Driver
	MODULES		usbSpeakerLib.o	
	REQUIRES	INCLUDE_USB
}

Component INCLUDE_USB_SPEAKER_INIT {
        NAME            Speaker Init
        SYNOPSIS        USB Speaker Driver Initialization
        REQUIRES        INCLUDE_USB_SPEAKER \
                        INCLUDE_USB_INIT
        EXCLUDES        INCLUDE_USBTOOL
        CONFIGLETTES    usrUsbSpkrInit.c
        PROTOTYPE       void usrUsbSpkrInit (void);
        INIT_RTN        usrUsbSpkrInit ();
        _INIT_ORDER     usrRoot
        INIT_AFTER      INCLUDE_USB_INIT
	INIT_BEFORE	usrToolsInit
}

Component INCLUDE_USB_MS_BULKONLY {
	NAME		Mass Storage - Bulk
	SYNOPSIS	Bulk Only Mass Storage USB Driver
	MODULES		usbBulkDevLib.o
	REQUIRES	INCLUDE_USB
}

Component INCLUDE_USB_MS_BULKONLY_INIT {
        NAME            Bulk Mass Storage Init
        SYNOPSIS        Bulk Only Mass Storage USB Driver Initialization
        REQUIRES        INCLUDE_USB_MS_BULKONLY \
                        INCLUDE_USB_INIT
        EXCLUDES        INCLUDE_USBTOOL
        CONFIGLETTES    usrUsbBulkDevInit.c
        PROTOTYPE       void usrUsbBulkDevInit (void);
        INIT_RTN        usrUsbBulkDevInit();
        _INIT_ORDER     usrRoot
        INIT_AFTER      INCLUDE_USB_INIT
	INIT_BEFORE	usrToolsInit
	CFG_PARAMS	BULK_DRIVE_NAME
}

Parameter BULK_DRIVE_NAME {
	NAME		USB Bulk Drive Name
	SYNOPSIS	Drive Name assigned to a the USB Bulk only device
	DEFAULT		"/bd"
}

Component INCLUDE_USB_MS_CBI {
	NAME		Mass Storage - CBI
	SYNOPSIS	Control/Bulk/Interrupt - Mass Storage USB Driver
	MODULES		usbCbiUfiDevLib.o
	REQUIRES	INCLUDE_USB
}

Component INCLUDE_USB_MS_CBI_INIT {
        NAME            CBI Mass Storage Init
        SYNOPSIS        Control/Bulk/Interrupt Mass Storage USB Driver \
			Initialization
        REQUIRES        INCLUDE_USB_MS_CBI \
                        INCLUDE_USB_INIT \
			INCLUDE_DOSFS
        EXCLUDES        INCLUDE_USBTOOL
        CONFIGLETTES    usrUsbCbiUfiDevInit.c
        PROTOTYPE       void usrUsbCbiUfiDevInit (void);
        INIT_RTN        usrUsbCbiUfiDevInit ();
        _INIT_ORDER     usrRoot
        INIT_AFTER      INCLUDE_USB_INIT
	INIT_BEFORE	usrToolsInit
	CFG_PARAMS	CBI_DRIVE_NAME
}

Parameter CBI_DRIVE_NAME {
	NAME		USB CBI Drive Name
	SYNOPSIS	Drive Name assigned to a the USB CBI device
	DEFAULT		"/cbid"
}

Component INCLUDE_USB_PEGASUS_END {
	NAME		End - Pegasus	
	SYNOPSIS	End - Pegasus USB Driver
	MODULES		usbPegasusEnd.o
	REQUIRES	INCLUDE_USB
}

Component INCLUDE_USB_PEGASUS_END_INIT {
	NAME		End - Pegasus Initialization
	SYNOPSIS	End - Pegasus USB Driver Initialization
	REQUIRES	INCLUDE_USB_PEGASUS_END \
			INCLUDE_USB_INIT
	EXCLUDES	INCLUDE_USBTOOL
	CONFIGLETTES	usrUsbPegasusEndInit.c
	PROTOTYPE	void usrUsbPegasusEndInit (void);
	INIT_RTN	usrUsbPegasusEndInit ();
	_INIT_ORDER	usrRoot
	INIT_AFTER	INCLUDE_USB_INIT
	INIT_BEFORE	usrToolsInit
	CFG_PARAMS	PEGASUS_IP_ADDRESS \
			PEGASUS_DESTINATION_ADDRESS \
			PEGASUS_NET_MASK \
			PEGASUS_TARGET_NAME
}

Parameter PEGASUS_IP_ADDRESS {
	NAME		Pegasus IP Address
	SYNOPSIS	USB Pegasus Device IP Address
	DEFAULT		"90.0.0.3"
}

Parameter PEGASUS_DESTINATION_ADDRESS {
	NAME		Pegasus Destination Address
	SYNOPSIS	USB Pegasus Device Destination Address
	DEFAULT		"90.0.0.53"
}

Parameter PEGASUS_NET_MASK {
	NAME		Pegasus Net Mask
	SYNOPSIS	USB Pegasus Device Net Mask
	DEFAULT		0xffffff00
}

Parameter PEGASUS_TARGET_NAME {
	NAME		Pegasus Target Name
	SYNOPSIS	USB Pegasus Device Target Name
	DEFAULT		"host"
}


Component INCLUDE_KBD_EMULATOR {
	NAME		Keyboard Emulator
	SYNOPSIS	USB Keyboard Emulator Firmware
	MODULES		usbTargKbdLib.o
	REQUIRES	INCLUDE_USB_TARG
}

Component INCLUDE_PRN_EMULATOR {
	NAME		Printer Emulator
	SYNOPSIS	USB Printer Emulator Firmware
	MODULES		usbTargPrnLib.o
	REQUIRES	INCLUDE_USB_TARG
}

Component INCLUDE_D12_EMULATOR {
	NAME		D12 Emulator
	SYNOPSIS	USB D12 Emulator Firmware
	MODULES		usbTargPhilipsD12EvalLib.o
	REQUIRES	INCLUDE_USB_TARG
}
