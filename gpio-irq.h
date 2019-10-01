#ifndef GPIO_IRQ_H
#define GPIO_IRQ_H

#include "xparameters.h"
#include "xstatus.h"
#include "xlpd_slcr.h"
#include "xil_io.h"
#include "xil_printf.h"
#include "xgpio.h"
#include "xscugic.h"


#define GPIO_CHANNEL       1
#define AXI_GPIO_DEVICE_ID XPAR_GPIO_0_DEVICE_ID

#define INTC_DEVICE_ID	   XPAR_SCUGIC_0_DEVICE_ID

// PL_PS_Group0 UG1085 Table 13-1
#define INTC_DEVICE_INT_ID 121


void gpio_irq_test(void);















#endif
