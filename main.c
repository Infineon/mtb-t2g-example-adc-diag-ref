/**********************************************************************************************************************
 * \file main.c
 * \copyright Copyright (C) Infineon Technologies AG 2019
 *
 * Use of this file is subject to the terms of use agreed between (i) you or the company in which ordinary course of
 * business you are acting and (ii) Infineon Technologies AG or its licensees. If and as long as no such terms of use
 * are agreed, use of this file is subject to following:
 *
 * Boost Software License - Version 1.0 - August 17th, 2003
 *
 * Permission is hereby granted, free of charge, to any person or organization obtaining a copy of the software and
 * accompanying documentation covered by this license (the "Software") to use, reproduce, display, distribute, execute,
 * and transmit the Software, and to prepare derivative works of the Software, and to permit third-parties to whom the
 * Software is furnished to do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including the above license grant, this restriction
 * and the following disclaimer, must be included in all copies of the Software, in whole or in part, and all
 * derivative works of the Software, unless such copies or derivative works are solely in the form of
 * machine-executable object code generated by a source language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *********************************************************************************************************************/

/*********************************************************************************************************************/
/*-----------------------------------------------------Includes------------------------------------------------------*/
/*********************************************************************************************************************/
#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"

/*********************************************************************************************************************/
/*------------------------------------------------------Macros-------------------------------------------------------*/
/*********************************************************************************************************************/
#define RESULT_MAX      (4095.0f)
#define MAX_MILLI_VOLTS (5000.0f)

/*********************************************************************************************************************/
/*---------------------------------------------Function Implementations----------------------------------------------*/
/*********************************************************************************************************************/
/**********************************************************************************************************************
 * Function Name: Process_ADC
 * Summary:
 *  This function gets the ADC conversion results in counts and calculates the corresponding voltage value.
 *  Prints the count and the calculated voltage on the terminal program.
 * Parameters:
 *  none
 * Return:
 *  none
 **********************************************************************************************************************
 */
static void Process_ADC(void)
{
    /* Issue software start trigger */
    Cy_SAR2_Channel_SoftwareTrigger(SARADC_HW, SARADC_CH_AN0_IDX);

    /* Wait for conversion to finish */
    while (CY_SAR2_INT_GRP_DONE != Cy_SAR2_Channel_GetInterruptStatus(SARADC_HW, SARADC_CH_AN0_IDX));

    /* Get conversion results in counts, do not obtain or analyze status here */
    uint16_t resultAN0 = Cy_SAR2_Channel_GetResult(SARADC_HW, SARADC_CH_AN0_IDX, NULL);

    /* Each channel results are stored in ADC counts at this point. */
    /* Clear interrupt source */
    Cy_SAR2_Channel_ClearInterrupt(SARADC_HW, SARADC_CH_AN0_IDX, CY_SAR2_INT_GRP_DONE);

    float resultMillivolts = MAX_MILLI_VOLTS * (resultAN0/RESULT_MAX);

    printf("\r\n\n >> ADC Diagnostic Reference Count : %u \r\n", resultAN0);
    printf("\r\n\n >> ADC Diagnostic Reference Voltage : %f mV \r\n", resultMillivolts);
    printf("\r\n\n >> Select another option \r\n\n");
}

/**********************************************************************************************************************
 * Function Name: Handle_Error
 * Summary:
 *  User defined error handling function.
 * Parameters:
 *  none
 * Return:
 *  none
 **********************************************************************************************************************
 */
static void Handle_Error(void)
{
     /* Disable all interrupts. */
    __disable_irq();

    CY_ASSERT(0);
}

/**********************************************************************************************************************
 * Function Name: main
 * Summary:
 *  This is the main function. It initializes the BSP and retarget-io lib.
 *  After that, it shows a menu and prompts the user to select a test case to run.
 * Parameters:
 *  none
 * Return:
 *  int
 **********************************************************************************************************************
 */
int main(void)
{
    cy_rslt_t result;

    #if defined(CY_DEVICE_SECURE)
        cyhal_wdt_t wdtObj;
        /* Clear watchdog timer so that it doesn't trigger a reset */
        result = cyhal_wdt_init(&wdtObj, cyhal_wdt_get_max_timeout_ms());
        CY_ASSERT(CY_RSLT_SUCCESS == result);
        cyhal_wdt_free(&wdtObj);
    #endif

    /* Variable to store the received character through terminal */
    uint8_t readData = 0;

    /* Initialize the device and board peripherals */
    result = cybsp_init();
    if (result != CY_RSLT_SUCCESS)
    {
        Handle_Error();
    }

    /* Initialize retarget-io to use the debug UART port */
    result = cy_retarget_io_init(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX, CY_RETARGET_IO_BAUDRATE);
    if (result != CY_RSLT_SUCCESS)
    {
        Handle_Error();
    }

    /* Scenario: CLK_HF2 has been configured to output 196 MHz.
     * Use one of the PeriClk dividers with a divider value of 50 to get a 3.92 Mhz SAR clock.
     * (These settings are already done in cybsp_init())
     * Sample time = SAMPLE_TIME (specified in Device Configurator) / SAR clock frequency -> 120/(3.92MHz) = 30.61 uS
     */

    /* Initialize the SAR2 module */
    Cy_SAR2_Init(SARADC_HW, &SARADC_config);

    /* Set ePASS MMIO reference buffer mode for bandgap voltage */
    Cy_SAR2_SetReferenceBufferMode(PASS0_EPASS_MMIO, CY_SAR2_REF_BUF_MODE_ON);

    /* Enable the Diagnostic Reference */
    Cy_SAR2_Diag_Enable(SARADC_HW);

    /* \x1b[2J\x1b[;H - ANSI ESC sequence for clear screen */
    printf("\x1b[2J\x1b[;H");

    printf("***********************************************************\r\n");
    printf("SAR ADC Diagnostic Reference Conversion\r\n");
    printf("***********************************************************\r\n\n");
    printf(">> Select one of the options as listed below \r\n\n");

    printf(">> 0 : VREFL : DiagOut = VrefL \r\n\n");
    printf(">> 1 : VREFH_1DIV8 : DiagOut = VrefH * 1/8 \r\n\n");
    printf(">> 2 : VREFH_2DIV8 : DiagOut = VrefH * 2/8 \r\n\n");
    printf(">> 3 : VREFH_3DIV8 : DiagOut = VrefH * 3/8 \r\n\n");
    printf(">> 4 : VREFH_4DIV8 : DiagOut = VrefH * 4/8 \r\n\n");
    printf(">> 5 : VREFH_5DIV8 : DiagOut = VrefH * 5/8 \r\n\n");
    printf(">> 6 : VREFH_6DIV8 : DiagOut = VrefH * 6/8 \r\n\n");
    printf(">> 7 : VREFH_7DIV8 : DiagOut = VrefH * 7/8 \r\n\n");
    printf(">> 8 : VREFH : DiagOut = VrefH \r\n\n");
    printf(">> 9 : VREFX : DiagOut = VrefX = VrefH * 199/200 \r\n\n");
    printf(">> a : VBG : DiagOut = Vbg from SRSS \r\n\n");
    printf(">> e : I_SOURCE : DiagOut = Isource (10uA) \r\n\n");
    printf(">> f : I_SINK : DiagOut = Isink (10uA) \r\n\n");

    __enable_irq();
 
    for (;;)
    {
        if (CY_RSLT_SUCCESS == cyhal_uart_getc(&cy_retarget_io_uart_obj, &readData, 0))
        {
            if (CY_RSLT_SUCCESS != cyhal_uart_putc(&cy_retarget_io_uart_obj, readData))
            {
                Handle_Error();
            }
        }
        else
        {
            Handle_Error();
        }

        cy_stc_sar2_diag_config_t diagConfig;   /* Variable for diagnostic reference configuration */

        switch (readData)
        {
            case '0':
                diagConfig.referenceSelect = CY_SAR2_DIAG_REFERENCE_SELECT_VREFL;
                Cy_SAR2_Diag_Init(SARADC_HW, &diagConfig);
                Process_ADC();
                break;

            case '1':
                diagConfig.referenceSelect = CY_SAR2_DIAG_REFERENCE_SELECT_VREFH_1DIV8;
                Cy_SAR2_Diag_Init(SARADC_HW, &diagConfig);
                Process_ADC();
                break;

            case '2':
                diagConfig.referenceSelect = CY_SAR2_DIAG_REFERENCE_SELECT_VREFH_2DIV8;
                Cy_SAR2_Diag_Init(SARADC_HW, &diagConfig);
                Process_ADC();
                break;

            case '3':
                diagConfig.referenceSelect = CY_SAR2_DIAG_REFERENCE_SELECT_VREFH_3DIV8;
                Cy_SAR2_Diag_Init(SARADC_HW, &diagConfig);
                Process_ADC();
                break;

            case '4':
                diagConfig.referenceSelect = CY_SAR2_DIAG_REFERENCE_SELECT_VREFH_4DIV8;
                Cy_SAR2_Diag_Init(SARADC_HW, &diagConfig);
                Process_ADC();
                break;

            case '5':
                diagConfig.referenceSelect = CY_SAR2_DIAG_REFERENCE_SELECT_VREFH_5DIV8;
                Cy_SAR2_Diag_Init(SARADC_HW, &diagConfig);
                Process_ADC();
                break;

            case '6':
                diagConfig.referenceSelect = CY_SAR2_DIAG_REFERENCE_SELECT_VREFH_6DIV8;
                Cy_SAR2_Diag_Init(SARADC_HW, &diagConfig);
                Process_ADC();
                break;

            case '7':
                diagConfig.referenceSelect = CY_SAR2_DIAG_REFERENCE_SELECT_VREFH_7DIV8;
                Cy_SAR2_Diag_Init(SARADC_HW, &diagConfig);
                Process_ADC();
                break;

            case '8':
                diagConfig.referenceSelect = CY_SAR2_DIAG_REFERENCE_SELECT_VREFH;
                Cy_SAR2_Diag_Init(SARADC_HW, &diagConfig);
                Process_ADC();
                break;

            case '9':
                diagConfig.referenceSelect = CY_SAR2_DIAG_REFERENCE_SELECT_VREFX;
                Cy_SAR2_Diag_Init(SARADC_HW, &diagConfig);
                Process_ADC();
                break;

            case 'a':
            case 'A':
                diagConfig.referenceSelect = CY_SAR2_DIAG_REFERENCE_SELECT_VBG;
                Cy_SAR2_Diag_Init(SARADC_HW, &diagConfig);
                Process_ADC();
                break;

            case 'e':
            case 'E':
                diagConfig.referenceSelect = CY_SAR2_DIAG_REFERENCE_SELECT_I_SOURCE;
                Cy_SAR2_Diag_Init(SARADC_HW, &diagConfig);
                Process_ADC();
                break;

            case 'f':
            case 'F':
                diagConfig.referenceSelect = CY_SAR2_DIAG_REFERENCE_SELECT_I_SINK;
                Cy_SAR2_Diag_Init(SARADC_HW, &diagConfig);
                Process_ADC();
                break;

            default:
                printf(">> \r\n\n Wrong Option selected. Try again! \r\n\n");
                break;
        }
        readData = 0;
    }
}

/* [] END OF FILE */
