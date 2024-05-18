/******************************************************************************
*
* Copyright (C) 2009 - 2014 Xilinx, Inc.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
******************************************************************************/

/*
 * helloworld.c: simple test application
 *
 * This application configures UART 16550 to baud rate 9600.
 * PS7 UART (Zynq) is not initialized by this application, since
 * bootrom/bsp configures it to baud rate 115200
 *
 * ------------------------------------------------
 * | UART TYPE   BAUD RATE                        |
 * ------------------------------------------------
 *   uartns550   9600
 *   uartlite    Configurable only in HW design
 *   ps7_uart    115200 (configured by bootrom/bsp)
 */

#include <stdio.h>
#include "platform.h"
#include "xparameters.h"
#include "xscugic.h"
#include "xil_exception.h"
#include "xgpio.h"
#include <unistd.h>
#define INTC_DEVICE_ID XPAR_PS7_SCUGIC_0_DEVICE_ID
#define KEY_DEVICE_ID XPAR_AXI_GPIO_0_DEVICE_ID
#define LED_DEVICE_ID XPAR_AXI_GPIO_1_DEVICE_ID
#define INTC_GPIO_INTERRUPT_ID XPAR_FABRIC_AXI_GPIO_0_IP2INTC_IRPT_INTR
#define KEY_INT_MASK XGPIO_IR_CH1_MASK

XGpio LEDInst;
XGpio KEYInst;
XScuGic INTCInst;

static void KeyIntrHandler(void * InstancePtr);
static int IntcInitFunction(u16 DeviceId, XGpio * GpioInstancePtr);
static int InterruptSystemSetup(XScuGic * XScuGicInstancePtr);

int main()
{
    int status;
    init_platform();

    status = XGpio_Initialize(&KEYInst, KEY_DEVICE_ID); // initial KEY
    if(status != XST_SUCCESS) return XST_FAILURE;
    status = XGpio_Initialize(&LEDInst, LED_DEVICE_ID); // initial LED
    if(status != XST_SUCCESS)return XST_FAILURE;

    XGpio_SetDataDirection(&KEYInst, 1, 0xFF);
    XGpio_SetDataDirection(&LEDInst, 1, 0); // set LED IO direction as out
    XGpio_DiscreteWrite(&LEDInst, 1, 0x00);// at initial, all LED turn off

    printf(">>Press PL KEY2, and check the PL LED \n");

    status = IntcInitFunction(INTC_DEVICE_ID, &KEYInst);
    if(status != XST_SUCCESS)return XST_FAILURE;
    while(1)
    {
        ;
    }
    cleanup_platform();
    return 0;
}

static void KeyIntrHandler(void * InstancePtr)
{
    u8 keyVal;

    usleep(10000); // 0.1s sleep, to debounce, in common, the meta-state will sustain no more than 20ms

    keyVal = XGpio_DiscreteRead(&KEYInst, 1) & 0x0f;

    printf("PL KEY2 pressed,interrupt is generated!! key value=%d\n",keyVal);

    XGpio_DiscreteWrite(&LEDInst, 1, 0X0F);//on led
    usleep(500000);

    XGpio_DiscreteWrite(&LEDInst, 1, 00);//reverse led

    XGpio_InterruptClear(&KEYInst, KEY_INT_MASK);
    XGpio_InterruptEnable(&KEYInst, KEY_INT_MASK); // Enable GPIO interrupts
}

static int IntcInitFunction(u16 DeviceId, XGpio * GpioInstancePtr)
{
    XScuGic_Config * IntcConfig;
    int status;

    // Interrupt controller initialization
    IntcConfig = XScuGic_LookupConfig(DeviceId);
    status = XScuGic_CfgInitialize(&INTCInst, IntcConfig, IntcConfig->CpuBaseAddress);
    if(status != XST_SUCCESS)return XST_FAILURE;

    // Call interrupt setup function
    status = InterruptSystemSetup(&INTCInst);
    if(status != XST_SUCCESS) return XST_FAILURE;

    // Register GPIO interrupt handler
    status = XScuGic_Connect(&INTCInst, INTC_GPIO_INTERRUPT_ID,
    (Xil_ExceptionHandler)KeyIntrHandler, (void*)GpioInstancePtr);
    if(status != XST_SUCCESS)return XST_FAILURE;

    // Enable GPIO interrupts
    XGpio_InterruptEnable(GpioInstancePtr, 1);
    XGpio_InterruptGlobalEnable(GpioInstancePtr);

    // Enable GPIO interrupts in the controller
    XScuGic_Enable(&INTCInst, INTC_GPIO_INTERRUPT_ID);
    return XST_SUCCESS;
}

//----------------------------------------------------------------------------
// Interrupt system setup
//----------------------------------------------------------------------------
static int InterruptSystemSetup(XScuGic * XScuGicInstancePtr)
{
    // Register GIC interrupt handler
    Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
    (Xil_ExceptionHandler)XScuGic_InterruptHandler, XScuGicInstancePtr);

    Xil_ExceptionEnable();
    return XST_SUCCESS;
}
