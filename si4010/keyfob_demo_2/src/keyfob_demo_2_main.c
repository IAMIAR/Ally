/*------------------------------------------------------------------------------
 *                          Silicon Laboratories, Inc.
 *                           http://www.silabs.com
 *                               Copyright 2010
 *------------------------------------------------------------------------------
 *
 *    FILE:       keyfob_demo_2_main.c
 *    TARGET:     Si4010 RevC
 *    TOOLCHAIN:  Keil
 *    DATE:       Nov 10, 2012,
 *    RELEASE:    1.0 (Tamas Nagy), ROM version 02.00
 *
 *------------------------------------------------------------------------------
 *
 *    DESCRIPTION:
 *      This file contains the main function for the Keil toolchain
 *      sample keyfob_demo_2 project.
 *
 *      BL51 linker directives for building this application:
 *
 *      BL51: PL(68) PW(78) IXREF RS(256)
 *            CODE (0x0-0x08FF)
 *            DATA (0x40)
 *            IDATA (0x70)
 *            XDATA (0x0900-0x107F)
 *            STACK (?STACK(0x90))
 *
 *------------------------------------------------------------------------------
 *
 *    INCLUDES:
 */
#include "stdlib.h"
#include "si4010.h"
#include "si4010_api_rom.h"

// Demo header
#include "keyfob_demo_2.h"

void  main (void)
{

/*------------------------------------------------------------------------------
 *    SETUP PHASE
 *------------------------------------------------------------------------------
 */
        //Set DMD interrupt to high priority,
        // any other interrupts have to stay low priority
        PDMD=1;
        // Disable the Matrix and Roff modes on GPIO[3:1] 
        PORT_CTRL &= ~(M_PORT_MATRIX | M_PORT_ROFF | M_PORT_STROBE);
        PORT_CTRL |=  M_PORT_STROBE;
        PORT_CTRL &= (~M_PORT_STROBE);
        // Turn LED control off 
        GPIO_LED = 0;
        vSys_Setup( 10 );
        vSys_SetMasterTime(0);
        // Setup the bandgap 
        vSys_BandGapLdo( 1 );

        if ((PROT0_CTRL & M_NVM_BLOWN) > 1) //if part is burned to user or run mode.
        {
                // Check the first power up bit meaning keyfob is powered on by battery insertion
                if ( 0 != (SYSGEN & M_POWER_1ST_TIME) )
                {
                        vSys_FirstPowerUp(); // Function will shutdown.
                }
        }
        // Set LED intensity. Valid values are 0 (off), 1, 2 and 3 
        vSys_LedIntensity( 3 );
        lLEDOnTime=20;
        //Get part ID (4byte factory burned unique serial number)
        lPartID = lSys_GetProdId();
        // Initialize isr call counter 
        bIsr_DebounceCount = 0;

        // BSR parameters initialization 
        rBsrSetup.bButtonMask = bButtonMask_c; 
#ifdef CRYSTAL
        rBsrSetup.bButtonMask &= 0xFE;// clear bit0; GPIO0 will be used by crystal
#endif
        rBsrSetup.pbPtsReserveHead = abBsrPtsPlaceHolder;
        rBsrSetup.bPtsSize = 3;
        rBsrSetup.bPushQualThresh = 3;
        // Setup the BSR 
        vBsr_Setup( &rBsrSetup );

        // Setup the RTC to tick every 5ms and clear it. Keep it disabled. 
        RTC_CTRL = (0x07 << B_RTC_DIV) | M_RTC_CLR;
        // Enable the RTC 
        RTC_CTRL |= M_RTC_ENA;
        // Enable the RTC interrupt and global interrupt enable 
        ERTC = 1;
        EA = 1;

        fDesiredFreqOOK 	 = f_433_RkeFreqOOK_c;
        fDesiredFreqFSK 	 = f_433_RkeFreqFSK_c;
        bFskDev 		 = b_433_RkeFskDev_c;
        // Setup the PA.
        rPaSetup.bLevel      = b_433_PaLevel_c;
        rPaSetup.wNominalCap = b_433_PaNominalCap_c;
        rPaSetup.bMaxDrv     = b_433_PaMaxDrv_c;
        rPaSetup.fAlpha      = 0.0;
        rPaSetup.fBeta       = 0.0;
        vPa_Setup( &rPaSetup );

#ifdef OOK
rOdsSetup.bModulationType = bModOOK_c;
// Setup the STL encoding for Manchester. No user encoding function therefore the pointer is NULL. 
vStl_EncodeSetup( bEncodeManchester_c, NULL );
fDesiredFreq = fDesiredFreqOOK;
bPreamble = bPreambleManch_c;
#else //FSK
rOdsSetup.bModulationType = bModFSK_c;
// Setup the STL encoding for none. No user encoding function therefore the pointer is NULL.
vStl_EncodeSetup( bEnc_NoneNrz_c, NULL );
fDesiredFreq = fDesiredFreqFSK;
bPreamble = bPreambleNrz_c;
#endif
// Setup the ODS 
// Set group width to 7, which means 8 bits/encoded byte to be transmitted.
// The value must match the output width of the data encoding function
// set by vStl_EncodeSetup()! 
rOdsSetup.bGroupWidth     = 7;
rOdsSetup.bClkDiv         = 5;
rOdsSetup.bEdgeRate       = 0;
// Configure the warm up intervals 
rOdsSetup.bLcWarmInt  = 0;
rOdsSetup.bDivWarmInt = 5;
rOdsSetup.bPaWarmInt  = 4;
// 24MHz / (bClkDiv+1) / 9.6kbps = 417 
rOdsSetup.wBitRate        = 417;
vOds_Setup( &rOdsSetup );

// Setup frequency casting .. needed to be called once per boot 
vFCast_Setup();

#ifdef CRYSTAL
//XO Setup
rXoSetup.fXoFreq	= 10000000.0;   // Frequency of the external crystal [Hz]
rXoSetup.bLowCap	= 1;			// Capacitance setup of crystal 0 = above 14pf, 1 = below 14pf
vFCast_XoSetup( &rXoSetup );
#endif

// Measure the battery voltage in mV, only once per boot to save power
// Make loaded measurement .. bBatteryWait_c * 17ms wait after loading
iBatteryMv = iMVdd_Measure( bBatteryWait_c );
if (iBatteryMv < iLowBatMv_c) 
{
        bBatStatus = 0;
}
else
{
        bBatStatus = 1;
}
  
// Setup the DMD temperature sensor for temperature mode 
vDmdTs_RunForTemp( 3 ); // Skip first 3 samples 
// Wait until there is a valid DMD temp sensor sample 
while ( 0 == bDmdTs_GetSamplesTaken() )
{
        //wait
}

 /*------------------------------------------------------------------------------
  *    TRANSMISSION LOOP PHASE
  *------------------------------------------------------------------------------
  */

// Application loop, including push button state analyzation and transmission. 
while(1)
{
// Buttons analyzation 
        vButtonCheck();
        if (bButtonState)
        {
                // Packet transmit repeat counter
	        bRepeatCount = bRepeatCount_c;   
                // Transmit. 
                vRepeatTxLoop();
                //Sync counter increment
        }
        else if( (lSys_GetMasterTime() >> 5) > bMaxWaitForPush_c )
        {
	        if ((PROT0_CTRL & M_NVM_BLOWN) > 1) //if part is burned to user or run mode.
  		{
                        //Disable all interrupts
                        EA = 0;
#ifdef CRYSTAL
                        // Disable XO
			bXO_CTRL = 0 ; 		
                        // Wait 20us for XO to stop
                        vSys_16BitDecLoop( 20 );
#endif
                        // Shutdown
                        vSys_Shutdown();      	
		}
    }
  }
}


/*------------------------------------------------------------------------------
 *
 *    FUNCTION DESCRIPTION:
 *      This function contains the loop which consists of three procedures,
 *      tx setup, tx loop and tx shutdown in the application.
 *      During waiting for repeat transmission, check button state.
 *      Once any new push button is detected, then transmit the new packet 
 *      instead of the current packet.
 *
 *------------------------------------------------------------------------------ 
 */
void vRepeatTxLoop (void)

{ 

        vFCast_Tune( fDesiredFreq );
        if (rOdsSetup.bModulationType == bModFSK_c)
        {
                vFCast_FskAdj( bFskDev ); 
        }
        // Wait until there is a temperature sample available
        while ( 0 == bDmdTs_GetSamplesTaken() )
        {
                //wait
        }
        //  Tune the PA with the temperature as an argument 
        vPa_Tune( iDmdTs_GetLatestTemp());
	vPacketAssemble();
	//Convert whole frame before transmission 
	vConvertPacket(rOdsSetup.bModulationType);
        vStl_PreLoop();
        do
        {
                // get current timestamp  
                lTimestamp = lSys_GetMasterTime();
                //if part is burned to user or run mode
	        if ((PROT0_CTRL & M_NVM_BLOWN) > 1) 
  	        {
    	                //turn LED on
	 	        GPIO_LED = 1; 
                }	       
                while ( (lSys_GetMasterTime() - lTimestamp) < lLEDOnTime )
                {
                        //wait for LED ON time to expire
                }
	        GPIO_LED = 0;   //turn LED off
	        //Transmit packet
                vStl_SingleTxLoop(pbFrameHead,bFrameSize);
	        // Wait repeat interval. 
                while ( (lSys_GetMasterTime() - lTimestamp) < wRepeatInterval_c );

        }while(--bRepeatCount);

        vStl_PostLoop();

         // Clear time value for next button push detecting. 
        vSys_SetMasterTime(0);

        return;
} 

//------------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void isr_rtc (void) interrupt INTERRUPT_RTC using 1

{ 

  // Update the master time by 5 every time this isr is run.
  // clear the RTC_INT 
  RTC_CTRL &= ~M_RTC_INT;
  vSys_IncMasterTime(5); 
  bIsr_DebounceCount ++;
  if ((bIsr_DebounceCount % bDebounceInterval_c) == 0)
  {
    vBsr_Service();
  }
  return;
}
 
/*------------------------------------------------------------------------------
 *
 *    FUNCTION DESCRIPTION:
 *      Update bAp_ButtonState which indicates what to be transmitted.
 *      Check the elements on PTS (push tracking strcture) to see if any GPIO
 *      has been pressed or released.
 *      If any new pressed button has detected, the corresponding flag will be set and
 *      the associated information will be transmitted in
 *      application loop procedure.
 *
 *
 *------------------------------------------------------------------------------ 
 */

void vButtonCheck (void)
{ 
  // Disable RTC interrupt, or button state might be updated. 
  ERTC = 0;
  bButtonState = 0;  //comment this line out if autorepeat needed
  if (bBsr_GetPtsItemCnt())
     //Some buttons were pressed or released
  {
    
	bButtonState = wBsr_Pop() & 0xFF;
	if (bPrevBtn)
	{
		bPrevBtn = bButtonState;
		bButtonState = 0;
	}
	else
	{
		bPrevBtn = bButtonState;
	}
  }
  // Enable RTC interrupt 
  ERTC = 1;
  return;
}

/*
 *------------------------------------------------------------------------------ 
 */
void vPacketAssemble (void)
{ 
  BYTE i;

  pbFrameHead = abFrame ;
  bFrameSize = bFrameSize_c;


  for (i=0;i<bPreambleSize_c;i++)
  {
	abFrame[i] = bPreamble;
  }
//clear button bits in bStatus
        bStatus &= ~M_ButtonBits_c;
//copy button bits from bButtonState to bStatus
        bStatus |= bButtonState & M_ButtonBits_c;

	abFrame[bFrameSize_c - 11] = bSync1_c;
	abFrame[bFrameSize_c - 10] = bSync2_c;
	abFrame[bFrameSize_c - 9] = ((BYTE *)&lPartID)[0];
	abFrame[bFrameSize_c - 8] = ((BYTE *)&lPartID)[1];
	abFrame[bFrameSize_c - 7] = ((BYTE *)&lPartID)[2];
	abFrame[bFrameSize_c - 6] = ((BYTE *)&lPartID)[3];
	abFrame[bFrameSize_c - 5] = bStatus;
	abFrame[bFrameSize_c - 4] = ((BYTE *)&iBatteryMv)[0];
	abFrame[bFrameSize_c - 3] = ((BYTE *)&iBatteryMv)[1];
	abFrame[bFrameSize_c - 2] = 0;	//CRC
	abFrame[bFrameSize_c - 1] = 0;	//CRC

	vCalculateCrc();


  return;
}
//--------------------------------------------------------------
//Calculate CRC and write in the frame buffer
//Bit pattern used (1)1000 0000 0000 0101, X16+X15+X2+1
void vCalculateCrc(void)
{
        BYTE i,j;
        WORD wCrc;
        wCrc = 0xffff;
        for(j = bPayloadStartIndex_c;j<bPayloadStartIndex_c + bPayloadSize_c;j++)
        {
                wCrc = wCrc ^ ((WORD)abFrame[j]<<8);

                for (i = 8; i != 0; i--)
                {
                        if (wCrc & 0x8000)
                        {
	                        wCrc = (wCrc << 1) ^ 0x8005;
                        }
                        else
	                {
                                wCrc <<= 1;
                        }
                }
        }
  //-----------------------------------------------------------------
  //Write CRC in frame
  abFrame[bFrameSize_c - 2] = ((BYTE*)&wCrc)[0];
  abFrame[bFrameSize_c - 1] = ((BYTE*)&wCrc)[1];
  return;
}

  //-------------------------------------------------------------------
  //MSB first to LSB first conversion, and inversion if FSK used
void vConvertPacket (BYTE bModType)
{
  BYTE i,low,high;

  if (bModType)
  {
    bModType = 0xff;
  }
  for (i=0;i<(sizeof abFrame);i++)
  {
	low = abConvTable[(abFrame[i] & 0xf0) >> 4] & 0x0f;
	high = abConvTable[abFrame[i] & 0x0f] & 0xf0;
	abFrame[i] = (high | low) ^ bModType;
  }
  return;
}


