/******************************************************************************
 ***** MAMEOPL.C --- Auxiliary Windows NT driver for OPL access in MAME32 *****
 ***** ------------------------------------------------------------------ *****
 *****                Windows NT kernel-mode driver source                *****
 *****                    written by Robert Schlabbach                    *****
 *****          e-mail: robert@powerstation.isdn.cs.TU-Berlin.DE          *****
 ******************************************************************************/

// include files
#include <NTDDK.H>
#include "MAMEOPL.H"

// driver function prototypes
NTSTATUS DriverEntry(
			IN	PDRIVER_OBJECT	DriverObject,
			IN	PUNICODE_STRING	RegistryPath);

VOID	 MAMEOPLUnload(
			IN	PDRIVER_OBJECT	DriverObject);

NTSTATUS MAMEOPLOpen(
			IN	PDEVICE_OBJECT	DeviceObject,
			IN	PIRP			Irp);

NTSTATUS MAMEOPLClose(
			IN	PDEVICE_OBJECT	DeviceObject,
			IN	PIRP			Irp);

NTSTATUS MAMEOPLIoControl(
			IN	PDEVICE_OBJECT	DeviceObject,
			IN	PIRP			Irp);

static VOID TenMicroSec(void);
static VOID OPL_write(UCHAR OPLChip, UCHAR Data);
static VOID OPL_control(UCHAR OPLChip, UCHAR Data);

// put static device names in the .text segment
#pragma	data_seg(".text")
static	WCHAR	WINNT_DEVICE_NAME[]	= L"\\Device\\MAMEOPL";
static	WCHAR	WIN32_DEVICE_NAME[]	= L"\\??\\MAMEOPL";

/******************************************************************************
 *****           DriverEntry() --- Driver entry point function            *****
 ******************************************************************************/

#pragma	code_seg("INIT")	// begin discardable code segment
NTSTATUS DriverEntry(
			IN	PDRIVER_OBJECT	DriverObject,
			IN	PUNICODE_STRING	RegistryPath)
{
	// declare local variables
	NTSTATUS		Status;
	PDEVICE_OBJECT	DeviceObject = NULL;
	UNICODE_STRING	WinNTNameString;
	UNICODE_STRING	Win32NameString;

	// print a debug string in checked version
	KdPrint(("MAMEOPL: Entered the Auxiliary MAME OPL access driver\n"));

	// create counted string version of the device name
	RtlInitUnicodeString(&WinNTNameString, WINNT_DEVICE_NAME);

	// create the device object
	Status = IoCreateDevice
	(
		DriverObject,					// DriverObject
		0,								// DeviceExtensionSize
		&WinNTNameString,				// DeviceName
		FILE_DEVICE_UNKNOWN,			// DeviceType
		0,								// DeviceCharacteristics
		TRUE,							// Exclusive
		&DeviceObject					// DeviceObject
	);

	// check if the device object was successfully created
	if (NT_SUCCESS(Status))
	{
		// set dispatch entry points in the driver object
		DriverObject->DriverUnload							=	MAMEOPLUnload;
		DriverObject->MajorFunction[IRP_MJ_CREATE]			=	MAMEOPLOpen;
		DriverObject->MajorFunction[IRP_MJ_CLOSE]			=	MAMEOPLClose;
		DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]	=	MAMEOPLIoControl;

		// print a progress debug string in checked version
		KdPrint(("MAMEOPL: Creating symbolic link\n"));

		// create counted string version of the Win32 device name
		RtlInitUnicodeString(&Win32NameString, WIN32_DEVICE_NAME);

		// create a link from the device name to a name in the Win32 namespace
		Status = IoCreateSymbolicLink(&Win32NameString, &WinNTNameString);

		// check if the link was successfully created
		if (NT_SUCCESS(Status))
		{
			// print a success debug string in checked version
			KdPrint(("MAMEOPL: Successfully initialized\n"));
		}
		else
		{
			// delete the created device object
			IoDeleteDevice(DriverObject->DeviceObject);

			// print a failure debug string in checked version
			KdPrint(("MAMEOPL: Couldn't create symbolic link\n"));
		};
	}
	else
	{
		// print a failure debug string in checked version
		KdPrint(("MAMEOPL: Couldn't create the device object\n"));
	};

	// return the last status
	return (Status);
}
#pragma code_seg()			// end discardable code segment

/******************************************************************************
 *****             MAMEOPLUnload() --- Driver unload function             *****
 ******************************************************************************/

VOID MAMEOPLUnload(
		IN	PDRIVER_OBJECT	DriverObject)
{
	// declare local variables
	UNICODE_STRING	Win32NameString;

	// print a debug string in checked version
	KdPrint(("MAMEOPL: Unloading the driver\n"));

	// create counted string version of the Win32 device name
	RtlInitUnicodeString(&Win32NameString, WIN32_DEVICE_NAME);

	// delete the link from the device name to a name in the Win32 namespace
	IoDeleteSymbolicLink(&Win32NameString);

	// delete the device object
	IoDeleteDevice(DriverObject->DeviceObject);
};

/******************************************************************************
 *****               MAMEOPLOpen() --- Driver open function               *****
 ******************************************************************************/

NTSTATUS MAMEOPLOpen(
			IN	PDEVICE_OBJECT	DeviceObject,
			IN	PIRP			Irp)
{
	// print a debug string in checked version
	KdPrint(("MAMEOPL: Opened\n"));

	// complete request successfully
	Irp->IoStatus.Status      = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	// return success
	return (STATUS_SUCCESS);
}

/******************************************************************************
 *****              MAMEOPLClose() --- Driver close function              *****
 ******************************************************************************/

NTSTATUS MAMEOPLClose(
			IN	PDEVICE_OBJECT	DeviceObject,
			IN	PIRP			Irp)
{
	// print a debug string in checked version
	KdPrint(("MAMEOPL: Closed\n"));

	// complete request successfully
	Irp->IoStatus.Status      = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	// return success
	return (STATUS_SUCCESS);
}

/******************************************************************************
 *****         MAMEOPLIoControl() --- Driver I/O control handler          *****
 ******************************************************************************/

NTSTATUS MAMEOPLIoControl(
			IN	PDEVICE_OBJECT	DeviceObject,
			IN	PIRP			Irp)
{
	// declare local variables
	NTSTATUS			Status;
	PIO_STACK_LOCATION	IrpSp;
	PUCHAR				IoBuffer;
	UCHAR				Chip = 0;
	UCHAR				Data = 0;

	// print a debug string in checked version
	KdPrint(("MAMEOPL: Handling I/O request\n"));

	// get Irp stack location
	IrpSp = IoGetCurrentIrpStackLocation(Irp);

	// initialize return status
	Status = STATUS_SUCCESS;

	// get pointer to user data buffer
	IoBuffer = Irp->AssociatedIrp.SystemBuffer;

	// switch according to I/O control code
	switch (IrpSp->Parameters.DeviceIoControl.IoControlCode)
	{
		// I/O control function 1: Write to OPL index register
		case IOCTL_MAMEOPL_WRITE_OPL_INDEX_REGISTER:
		{
			// check if user data buffer pointer is valid
			if (IoBuffer)
			{
				// check if the input buffer contains exactly 1 byte
				if (IrpSp->Parameters.DeviceIoControl.InputBufferLength == 1)
				{
                    Data = IoBuffer[0];
                    Chip = 0;

					// write byte to OPL index register
					OPL_control(Chip, Data);

					// set number of bytes transferred
					Irp->IoStatus.Information = 1;
				}
				// check if the input buffer contains exactly 2 bytes
				else
                if (IrpSp->Parameters.DeviceIoControl.InputBufferLength == 2)
				{
                    Data = IoBuffer[0];
                    Chip = IoBuffer[1];

					// write byte to OPL index register
					OPL_control(Chip, Data);

					// set number of bytes transferred
					Irp->IoStatus.Information = 2;
				}
				else
				{
					// return invalid buffer size error
					Status = STATUS_BUFFER_TOO_SMALL;
				};
			}
			else
			{
				// return invalid parameter error
				Status = STATUS_INVALID_PARAMETER;
			};
		}
		break;

		// I/O control function 2: Write to OPL data register
		case IOCTL_MAMEOPL_WRITE_OPL_DATA_REGISTER:
		{
			// check if user data buffer pointer is valid
			if (IoBuffer)
			{
				// check if the input buffer contains exactly 1 byte
				if (IrpSp->Parameters.DeviceIoControl.InputBufferLength == 1)
				{
					Data = IoBuffer[0];
					Chip = 0;

					// write byte to OPL data register
					OPL_write(Chip, Data);

					// set number of bytes transferred
					Irp->IoStatus.Information = 1;
				}
				else
				// check if the input buffer contains exactly 2 bytes
				if (IrpSp->Parameters.DeviceIoControl.InputBufferLength == 2)
				{
					Data = IoBuffer[0];
					Chip = IoBuffer[1];

					// write byte to OPL data register
					OPL_write(Chip, Data);

					// set number of bytes transferred
					Irp->IoStatus.Information = 2;
				}
				else
				{
					// return invalid buffer size error
					Status = STATUS_BUFFER_TOO_SMALL;
				};
			}
			else
			{
				// return invalid parameter error
				Status = STATUS_INVALID_PARAMETER;
			};
		}
		break;

		// I/O control function 3: Silence OPL
		case IOCTL_MAMEOPL_SILENCE_OPL:
		{
			// declare local variables
			int	n;

			for (n = 0x40; n <= 0x55; n++)
			{
				OPL_control(Chip, n);
				OPL_write(Chip, 0x3f);
			}
			for (n = 0x60; n <= 0x95; n++)
			{
				OPL_control(Chip, n);
				OPL_write(Chip, 0xff);
			}
			for (n = 0xa0; n <= 0xb0; n++)
			{
				OPL_control(Chip, n);
				OPL_write(Chip, 0);
			}

			// set number of bytes transferred
			Irp->IoStatus.Information = 0;

			// return success
			Status = STATUS_SUCCESS;
		}
		break;

		default:
		{
			// print an error debug string in checked version
			KdPrint(("MAMEOPL: Error invalid IoControlCode\n"));

			// return error
			Status = STATUS_INVALID_PARAMETER;
		}
		break;
	};

	// set the return status
	Irp->IoStatus.Status = Status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	// return status
	return (Status);
}

/******************************************************************************
 *****         TenMicroSec() --- a so called ten microsecond delay        *****
 ******************************************************************************/

static VOID TenMicroSec(void)
{
	int i;
	for (i = 0; i < 16; i++)
		 READ_PORT_UCHAR((PUCHAR) 0x80);
}

/******************************************************************************
 *****         OPL_control() --- Write to OPL index register              *****
 ******************************************************************************/

static VOID OPL_control(UCHAR OPLChip, UCHAR Data)
{
	// delay
	TenMicroSec();

	// write byte to OPL index register
	WRITE_PORT_UCHAR((PUCHAR) (0x0388 + OPLChip * 2), Data);
}

/******************************************************************************
 *****         OPL_write() --- Write to OPL data register                 *****
 ******************************************************************************/

static VOID OPL_write(UCHAR OPLChip, UCHAR Data)
{
	// delay
	TenMicroSec();

	// write byte to OPL data register
	WRITE_PORT_UCHAR((PUCHAR) (0x0389 + OPLChip * 2), Data);
}
