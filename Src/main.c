/**
  ******************************************************************************
  * @file    LwIP/LwIP_HTTP_Server_Netconn_RTOS/Src/main.c 
  * @author  MCD Application Team
  * @brief   This sample code implements a http server application based on 
  *          Netconn API of LwIP stack and FreeRTOS. This application uses 
  *          STM32F4xx the ETH HAL API to transmit and receive data. 
  *          The communication is done with a web browser of a remote PC.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "ethernetif.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "app_ethernet.h"
#include "httpserver-netconn.h"
#include "string.h"
#ifdef USE_LCD
#include "lcd_log.h"
#endif
#include "ntc_sensor.h"
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
struct netif gnetif; /* network interface structure */

/* Private function prototypes -----------------------------------------------*/
static void SystemClock_Config(void);
static void StartThread(void const * argument);
static void BSP_Config(void);
static void Netif_Config(void);
static void GPIO_Config(void);
static void RelayInitConfig(void);
static void LedInitConfig(void);
static void MX_ADC3_Init(void);
static void MX_ADC1_Init(void);

ADC_HandleTypeDef hadc3;
ADC_HandleTypeDef hadc1;
/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */

int main(void)
{
  /* STM32F4xx HAL library initialization:
       - Configure the Flash ART accelerator on ITCM interface
       - Configure the Systick to generate an interrupt each 1 msec
       - Set NVIC Group Priority to 4
       - Global MSP (MCU Support Package) initialization
     */

  HAL_Init();
  
  /* Configure the system clock to 180 MHz */
  SystemClock_Config();
  
  MX_ADC3_Init();
  MX_ADC1_Init();
  GPIO_Config();

  /* Initialize LCD and LEDs */
  BSP_Config();



  /* Init thread */
#if defined(__GNUC__)
  osThreadDef(Start, StartThread, osPriorityNormal, 0, configMINIMAL_STACK_SIZE * 5);
#else
  osThreadDef(Start, StartThread, osPriorityNormal, 0, configMINIMAL_STACK_SIZE * 2);
#endif
  
  osThreadCreate (osThread(Start), NULL);
  
  NTC_CreateTask();

  /* Start scheduler */
  osKernelStart();
  
  /* We should never get here as control is now taken by the scheduler */
  for( ;; );
}

/**
  * @brief  Start Thread 
  * @param  argument not used
  * @retval None
  */
extern void tcp_server_init();
static void StartThread(void const * argument)
{
  /* Create tcp_ip stack thread */
  tcpip_init(NULL, NULL);
  
  /* Initialize the LwIP stack */
  Netif_Config();
  
  /* Initialize webserver demo */
  //http_server_netconn_init();
  tcp_server_init();

  for( ;; )
  {
    /* Delete the Init Thread */ 
    osThreadTerminate(NULL);
  }
}

/**
  * @brief  Initializes the STM324x9I-EVAL's LCD and LEDs resources.
  * @param  None
  * @retval None
  */
static void BSP_Config(void)
{
#ifdef USE_LCD

  /* Initialize the LCD */
  BSP_LCD_Init();

  /* Initialize the LCD Layers */
  BSP_LCD_LayerDefaultInit(1, LCD_FB_START_ADDRESS);

  /* Set LCD Foreground Layer  */
  BSP_LCD_SelectLayer(1);
  
  BSP_LCD_SetFont(&LCD_DEFAULT_FONT);
  
  /* Initialize LCD Log module */
  LCD_LOG_Init();
  
  /* Show Header and Footer texts */
  LCD_LOG_SetHeader((uint8_t *)"Webserver Application Netconn API");
  LCD_LOG_SetFooter((uint8_t *)"STM324x9I-EVAL board");
  
  LCD_UsrLog ((char *)"  State: Ethernet Initialization ...\n");

#else
  /* Configure LED1 and LED2 */
  BSP_LED_Init(LED1);
  BSP_LED_Init(LED2);

#endif /* USE_LCD */
}

/**
  * @brief  Configures the network interface
  * @param  None
  * @retval None
  */
static void Netif_Config(void)
{
  ip_addr_t ipaddr;
  ip_addr_t netmask;
  ip_addr_t gw;

#if LWIP_DHCP
  ip_addr_set_zero_ip4(&ipaddr);
  ip_addr_set_zero_ip4(&netmask);
  ip_addr_set_zero_ip4(&gw);
#else
  IP_ADDR4(&ipaddr,IP_ADDR0,IP_ADDR1,IP_ADDR2,IP_ADDR3);
  IP_ADDR4(&netmask,NETMASK_ADDR0,NETMASK_ADDR1,NETMASK_ADDR2,NETMASK_ADDR3);
  IP_ADDR4(&gw,GW_ADDR0,GW_ADDR1,GW_ADDR2,GW_ADDR3);
#endif /* LWIP_DHCP */

  /* add the network interface */
  netif_add(&gnetif, &ipaddr, &netmask, &gw, NULL, &ethernetif_init, &tcpip_input);

  /*  Registers the default network interface. */
  netif_set_default(&gnetif);

  ethernet_link_status_updated(&gnetif);

#if LWIP_NETIF_LINK_CALLBACK
  netif_set_link_callback(&gnetif, ethernet_link_status_updated);

  osThreadDef(EthLink, ethernet_link_thread, osPriorityNormal, 0, configMINIMAL_STACK_SIZE *2);
  osThreadCreate (osThread(EthLink), &gnetif);
#endif

#if LWIP_DHCP
  /* Start DHCPClient */
  osThreadDef(DHCP, DHCP_Thread, osPriorityBelowNormal, 0, configMINIMAL_STACK_SIZE * 2);
  osThreadCreate (osThread(DHCP), &gnetif);
#endif
}

/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow : 
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 180000000
  *            HCLK(Hz)                       = 180000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 4
  *            APB2 Prescaler                 = 2
  *            HSE Frequency(Hz)              = 8000000
  *            PLL_M                          = 8
  *            PLL_N                          = 360
  *            PLL_P                          = 2
  *            PLL_Q                          = 7
  *            VDD(V)                         = 3.3
  *            Main regulator output voltage  = Scale1 mode
  *            Flash Latency(WS)              = 5
  * @param  None
  * @retval None
  */
static void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;

  /* Enable Power Control clock */
  __HAL_RCC_PWR_CLK_ENABLE();
  
  /* The voltage scaling allows optimizing the power consumption when the device is 
     clocked below the maximum system frequency, to update the voltage scaling value 
     regarding system frequency refer to product datasheet.  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  
  /* Enable HSE Oscillator and activate PLL with HSE as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 360;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
   while(1) {};
  }
  
  if(HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
   while(1) {};
  }
  
  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 
     clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;  
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;  
  if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
   while(1) {};
  }
}


static void GPIO_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    /*=========================================================
      CLOCK ENABLE
    =========================================================*/
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    /*=========================================================
      RESET STRUCT
    =========================================================*/
    memset(&GPIO_InitStruct, 0, sizeof(GPIO_InitStruct));



    /*=========================================================
                     PA4 -> L_LA2_G
    =========================================================*/
    /*fill the structure before the pin configuration, then pass
       both the pointers #define GPIOD ((GPIO_TypeDef *) GPIOD_BASE)
       and &GPIO_InitStruct
      */
    GPIO_InitStruct.Pin = GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);




    /*=========================================================
                         PA4 -> L_LB2_G
     =========================================================*/
     /*fill the structure before the pin configuration, then pass
        both the pointers #define GPIOG ((GPIO_TypeDef *) GPIOG_BASE)
        and &GPIO_InitStruct
       */
     GPIO_InitStruct.Pin = GPIO_PIN_15;
     GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
     GPIO_InitStruct.Pull = GPIO_NOPULL;
     GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

     HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);



   /*=========================================================
                     PG15 -> K_A_E
    =========================================================*/
     /*fill the structure before the pin configuration, then pass
       both the pointers #define GPIOD ((GPIO_TypeDef *) GPIOD_BASE)
       and &GPIO_InitStruct
      */
     GPIO_InitStruct.Pin = GPIO_PIN_6;
     GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
     GPIO_InitStruct.Pull = GPIO_NOPULL;
     GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

     HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);




     /*=========================================================
                          PE4 -> K_B_E
      =========================================================*/
      /*fill the structure before the pin configuration, then pass
        both the pointers #define GPIOE ((GPIO_TypeDef *) GPIOE_BASE)
        and &GPIO_InitStruct
       */
      GPIO_InitStruct.Pin = GPIO_PIN_4;
      GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
      GPIO_InitStruct.Pull = GPIO_NOPULL;
      GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

      HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);









     /*=========================================================
                   PA15 -> nSHTDN
     =========================================================*/
     /*fill the structure before the pin configuration, then pass
       both the pointers #define GPIOA ((GPIO_TypeDef *) GPIOA_BASE)
       and &GPIO_InitStruct
      */
     GPIO_InitStruct.Pin = GPIO_PIN_15;
     GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
     GPIO_InitStruct.Pull = GPIO_NOPULL;
     GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

     HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);





     /*=========================================================
                   PB6 -> K_A_NO
     =========================================================*/
     /*fill the structure before the pin configuration, then pass
       both the pointers #define GPIOB ((GPIO_TypeDef *) GPIOB_BASE)
       and &GPIO_InitStruct
      */
     GPIO_InitStruct.Pin = GPIO_PIN_6;
     GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
     GPIO_InitStruct.Pull = GPIO_NOPULL;
     GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

     HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);




     /*=========================================================
                        PE5 -> K_B_NO
     =========================================================*/
     /*fill the structure before the pin configuration, then pass
       both the pointers #define GPIOE ((GPIO_TypeDef *) GPIOE_BASE)
       and &GPIO_InitStruct
      */
     GPIO_InitStruct.Pin = GPIO_PIN_5;
     GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
     GPIO_InitStruct.Pull = GPIO_NOPULL;
     GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

     HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);






     /*=========================================================
                      PC2 -> L_LA2_R
      =======================================================*/
     /*fill the structure before the pin configuration, then pass
       both the pointers #define GPIOD ((GPIO_TypeDef *) GPIOD_BASE)
       and &GPIO_InitStruct
      */
      GPIO_InitStruct.Pin = GPIO_PIN_2;
      GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
      GPIO_InitStruct.Pull = GPIO_NOPULL;
      GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

      HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);





     /*=========================================================
                           PE6 -> L_LB2_R
      =======================================================*/
     /*fill the structure before the pin configuration, then pass
       both the pointers #define GPIOE ((GPIO_TypeDef *) GPIOE_BASE)
       and &GPIO_InitStruct
      */
     GPIO_InitStruct.Pin = GPIO_PIN_6;
     GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
     GPIO_InitStruct.Pull = GPIO_NOPULL;
     GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

     HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);





     /*=========================================================
                     PC7 -> K_A_NC
     =======================================================*/
     /*fill the structure before the pin configuration, then pass
      both the pointers #define GPIOC ((GPIO_TypeDef *) GPIOC_BASE)
      and &GPIO_InitStruct
     */
     GPIO_InitStruct.Pin = GPIO_PIN_7;
     GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
     GPIO_InitStruct.Pull = GPIO_NOPULL;
     GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

     HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);



     /*=========================================================
                         PF2 -> K_B_NC
     =======================================================*/
     /*fill the structure before the pin configuration, then pass
      both the pointers #define GPIOF ((GPIO_TypeDef *) GPIOF_BASE)
      and &GPIO_InitStruct
     */
     GPIO_InitStruct.Pin = GPIO_PIN_2;
     GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
     GPIO_InitStruct.Pull = GPIO_NOPULL;
     GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

     HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);





     /*=========================================================
                 PD3 -> L_LA1_R
      =======================================================*/
     /*fill the structure before the pin configuration, then pass
       both the pointers #define GPIOD ((GPIO_TypeDef *) GPIOD_BASE)
       and &GPIO_InitStruct
      */
     GPIO_InitStruct.Pin = GPIO_PIN_3;
     GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
     GPIO_InitStruct.Pull = GPIO_NOPULL;
     GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

     HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);





     /*=========================================================
                      PG10 -> L_LB1_R
      =======================================================*/
     /*fill the structure before the pin configuration, then pass
       both the pointers #define GPIOG ((GPIO_TypeDef *) GPIOG_BASE)
       and &GPIO_InitStruct
      */
     GPIO_InitStruct.Pin = GPIO_PIN_10;
     GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
     GPIO_InitStruct.Pull = GPIO_NOPULL;
     GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

     HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);




     /*=========================================================
                PD4 -> L_LA1_G
      =========================================================*/
     /*fill the structure before the pin configuration, then pass
        both the pointers #define GPIOD ((GPIO_TypeDef *) GPIOD_BASE)
        and &GPIO_InitStruct
       */
     GPIO_InitStruct.Pin = GPIO_PIN_4;
     GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
     GPIO_InitStruct.Pull = GPIO_NOPULL;
     GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

     HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);



     /*=========================================================
                     PH2 -> L_LB1_G
      =========================================================*/
     /*fill the structure before the pin configuration, then pass
        both the pointers #define GPIOH ((GPIO_TypeDef *) GPIOH_BASE)
        and &GPIO_InitStruct
       */
     GPIO_InitStruct.Pin = GPIO_PIN_2;
     GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
     GPIO_InitStruct.Pull = GPIO_NOPULL;
     GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

     HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);



     /*=========================================================
                      PE2 -> L_SRT_G
       =========================================================*/
     /*fill the structure before the pin configuration, then pass
         both the pointers #define GPIOH ((GPIO_TypeDef *) GPIOH_BASE)
         and &GPIO_InitStruct
        */
     GPIO_InitStruct.Pin = GPIO_PIN_2;
     GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
     GPIO_InitStruct.Pull = GPIO_NOPULL;
     GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

     HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);



     /*=========================================================
                      PD7 -> L_SRT_R
       =========================================================*/
     /*fill the structure before the pin configuration, then pass
         both the pointers #define GPIOD ((GPIO_TypeDef *) GPIOD_BASE)
         and &GPIO_InitStruct
        */
     GPIO_InitStruct.Pin = GPIO_PIN_7;
     GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
     GPIO_InitStruct.Pull = GPIO_NOPULL;
     GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

     HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);




     /*=========================================================
                       PF6 -> L_LX1_G
        =========================================================*/
     /*fill the structure before the pin configuration, then pass
          both the pointers #define GPIOD ((GPIO_TypeDef *) GPIOD_BASE)
          and &GPIO_InitStruct
         */
     GPIO_InitStruct.Pin = GPIO_PIN_6;
     GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
     GPIO_InitStruct.Pull = GPIO_NOPULL;
     GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

     HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);



     /*=========================================================
                       PF7 -> L_LX1_R
        =========================================================*/
     /*fill the structure before the pin configuration, then pass
       both the pointers #define GPIOF ((GPIO_TypeDef *) GPIOF_BASE)
       and &GPIO_InitStruct
         */
     GPIO_InitStruct.Pin = GPIO_PIN_7;
     GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
     GPIO_InitStruct.Pull = GPIO_NOPULL;
     GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

     HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);





     /*=========================================================
                        PC11 -> L_LX2_G
         =========================================================*/
     /*fill the structure before the pin configuration, then pass
        both the pointers #define GPIOF ((GPIO_TypeDef *) GPIOC_BASE)
        and &GPIO_InitStruct
          */
     GPIO_InitStruct.Pin = GPIO_PIN_11;
     GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
     GPIO_InitStruct.Pull = GPIO_NOPULL;
     GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

     HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);





     /*=========================================================
                         PD2 -> L_LX2_R
          =========================================================*/
     /*fill the structure before the pin configuration, then pass
         both the pointers #define GPIOD ((GPIO_TypeDef *) GPIOD_BASE)
         and &GPIO_InitStruct
           */
     GPIO_InitStruct.Pin = GPIO_PIN_2;
     GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
     GPIO_InitStruct.Pull = GPIO_NOPULL;
     GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

     HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);






     /*=========================================================
                       PC10 -> L_LX3_G
        =========================================================*/
     /*fill the structure before the pin configuration, then pass
       both the pointers #define GPIOC ((GPIO_TypeDef *) GPIOC_BASE)
       and &GPIO_InitStruct
         */
     GPIO_InitStruct.Pin = GPIO_PIN_10;
     GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
     GPIO_InitStruct.Pull = GPIO_NOPULL;
     GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

     HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);




     /*=========================================================
                       PC12 -> L_LX3_R
        =========================================================*/
     /*fill the structure before the pin configuration, then pass
       both the pointers #define GPIOC ((GPIO_TypeDef *) GPIOC_BASE)
       and &GPIO_InitStruct
         */
     GPIO_InitStruct.Pin = GPIO_PIN_12;
     GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
     GPIO_InitStruct.Pull = GPIO_NOPULL;
     GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

     HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);



     /*=========================================================
                       PG3 -> L_OPN_G
        =========================================================*/
     /*fill the structure before the pin configuration, then pass
       both the pointers #define GPIOG ((GPIO_TypeDef *) GPIOG_BASE)
       and &GPIO_InitStruct
         */
     GPIO_InitStruct.Pin = GPIO_PIN_3;
     GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
     GPIO_InitStruct.Pull = GPIO_NOPULL;
     GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

     HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);




     /*=========================================================
                        PD6 -> L_OPN_R
         =========================================================*/
     /*fill the structure before the pin configuration, then pass
        both the pointers #define GPIOD ((GPIO_TypeDef *) GPIOD_BASE)
        and &GPIO_InitStruct
          */
     GPIO_InitStruct.Pin = GPIO_PIN_6;
     GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
     GPIO_InitStruct.Pull = GPIO_NOPULL;
     GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

     HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);










     /*=========================================================
                      PD5 -> nFLT
      =======================================================*/
      /*fill the structure before the pin configuration, then pass
       both the pointers #define GPIOD ((GPIO_TypeDef *) GPIOD_BASE)
       and &GPIO_InitStruct
      */
     GPIO_InitStruct.Pin = GPIO_PIN_5;
     GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
     GPIO_InitStruct.Pull = GPIO_NOPULL;
     GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

     HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);





     /*=========================================================
                PE3 -> PGOOD
      =========================================================*/
     /*fill the structure before the pin configuration, then pass
        both the pointers #define GPIOE ((GPIO_TypeDef *) GPIOE_BASE)
        and &GPIO_InitStruct
       */
     GPIO_InitStruct.Pin = GPIO_PIN_3;
     GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
     GPIO_InitStruct.Pull = GPIO_NOPULL;
     GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

     HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);





     /*=========================================================
                             PC8 -> K_SA_E1
      =========================================================*/
      GPIO_InitStruct.Pin = GPIO_PIN_5;
      GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
      GPIO_InitStruct.Pull = GPIO_NOPULL;
      GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

      HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);



     /*=========================================================
                         PC8 -> K_SA_E2
     =========================================================*/
     GPIO_InitStruct.Pin = GPIO_PIN_8;
     GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
     GPIO_InitStruct.Pull = GPIO_NOPULL;
     GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

     HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

     HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_RESET);


     /*=========================================================
                         PB11 -> K_SA_E3
     =========================================================*/
     GPIO_InitStruct.Pin = GPIO_PIN_11;
     GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
     GPIO_InitStruct.Pull = GPIO_NOPULL;
     GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

     HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

     HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_RESET);


     /*=========================================================
                              PB12 -> K_SA_S1
      =========================================================*/
      GPIO_InitStruct.Pin = GPIO_PIN_12;
      GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
      GPIO_InitStruct.Pull = GPIO_NOPULL;
      GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

      HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);





      /*=========================================================
                                 PC6 -> K_SA_S2
       =========================================================*/
       GPIO_InitStruct.Pin = GPIO_PIN_6;
       GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
       GPIO_InitStruct.Pull = GPIO_NOPULL;
       GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

       HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

       HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_RESET);



       /*=========================================================
                                       PB2 -> K_SA_S3
       =========================================================*/
       GPIO_InitStruct.Pin = GPIO_PIN_2;
       GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
       GPIO_InitStruct.Pull = GPIO_NOPULL;
       GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

       HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

       HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET);



       /*=========================================================
                          PD14 -> K_SB_E1
        =========================================================*/
        GPIO_InitStruct.Pin = GPIO_PIN_14;
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

        HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_RESET);




        /*=========================================================
                                PB15 -> K_SB_E2
        =========================================================*/
        GPIO_InitStruct.Pin = GPIO_PIN_15;
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);




        /*=========================================================
                                 PF14 -> K_SB_E3
         =========================================================*/
        GPIO_InitStruct.Pin = GPIO_PIN_14;
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

        HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

        HAL_GPIO_WritePin(GPIOF, GPIO_PIN_14, GPIO_PIN_RESET);






        /*=========================================================
                                 PD15 -> K_SB_S1
         =========================================================*/
        GPIO_InitStruct.Pin = GPIO_PIN_15;
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

        HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, GPIO_PIN_RESET);




        /*=========================================================
                                PE7 -> K_SB_S2
        =========================================================*/
        GPIO_InitStruct.Pin = GPIO_PIN_7;
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

        HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_7, GPIO_PIN_RESET);





        /*=========================================================
                                 PE9 -> K_SB_S3
         =========================================================*/
        GPIO_InitStruct.Pin = GPIO_PIN_9;
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

        HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_9, GPIO_PIN_RESET);







     /*=========================================================
                         PA8 -> K_HA_E
     =========================================================*/
     GPIO_InitStruct.Pin = GPIO_PIN_9;
     GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
     GPIO_InitStruct.Pull = GPIO_NOPULL;
     GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

     HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

     HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);



     /*=========================================================
                          PB4 -> K_HA_NC
      =========================================================*/
     GPIO_InitStruct.Pin = GPIO_PIN_4;
     GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
     GPIO_InitStruct.Pull = GPIO_NOPULL;
     GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

     HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

     HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);




     /*=========================================================
                          PB10 -> K_HA_NO
      =========================================================*/
     GPIO_InitStruct.Pin = GPIO_PIN_10;
     GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
     GPIO_InitStruct.Pull = GPIO_NOPULL;
     GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

     HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

     HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET);











     /*=========================================================
                         PF8 -> K_HB_E
     =========================================================*/
     GPIO_InitStruct.Pin = GPIO_PIN_8;
     GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
     GPIO_InitStruct.Pull = GPIO_NOPULL;
     GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

     HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

     HAL_GPIO_WritePin(GPIOF, GPIO_PIN_8, GPIO_PIN_RESET);





     /*=========================================================
                        PG1 -> K_HB_NC
     =========================================================*/
     GPIO_InitStruct.Pin = GPIO_PIN_1;
     GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
     GPIO_InitStruct.Pull = GPIO_NOPULL;
     GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

     HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

     HAL_GPIO_WritePin(GPIOG, GPIO_PIN_1, GPIO_PIN_RESET);





     /*=========================================================
                          PF9 -> K_HB_NO
      =========================================================*/
     GPIO_InitStruct.Pin = GPIO_PIN_9;
     GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
     GPIO_InitStruct.Pull = GPIO_NOPULL;
     GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

     HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

     HAL_GPIO_WritePin(GPIOF, GPIO_PIN_9, GPIO_PIN_RESET);






     /*=========================================================
                         PD14 -> K_SB_E1
     =========================================================*/
     GPIO_InitStruct.Pin = GPIO_PIN_14;
     GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
     GPIO_InitStruct.Pull = GPIO_NOPULL;
     GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

     HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

     HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_RESET);


     /*=========================================================
                         PB15 -> K_SB_E2
     =========================================================*/
     GPIO_InitStruct.Pin = GPIO_PIN_15;
     GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
     GPIO_InitStruct.Pull = GPIO_NOPULL;
     GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

     HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

     HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);


     /*=========================================================
                         PF14 -> K_SB_E3
     =========================================================*/
     GPIO_InitStruct.Pin = GPIO_PIN_14;
     GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
     GPIO_InitStruct.Pull = GPIO_NOPULL;
     GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

     HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

     HAL_GPIO_WritePin(GPIOF, GPIO_PIN_14, GPIO_PIN_RESET);




      /*=========================================================
                              PF3 -> NB ADC3 CHANNEL 9
      =========================================================*/
       GPIO_InitStruct.Pin = GPIO_PIN_3;
       GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
       GPIO_InitStruct.Pull = GPIO_NOPULL;
       HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

       HAL_GPIO_WritePin(GPIOF, GPIO_PIN_3, GPIO_PIN_RESET);


       /*=========================================================
                                    PF5 -> NA ADC3 CHANNEL 15

            =========================================================*/
        GPIO_InitStruct.Pin = GPIO_PIN_5;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);




        /*=========================================================
                                    PF10 -> NAB     ADC3 CHANNEL 8

            =========================================================*/
        GPIO_InitStruct.Pin = GPIO_PIN_10;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);





      RelayInitConfig();
     LedInitConfig();


}


void RelayInitConfig(void)
{

    /* Current configuration: ERR:A1 ok A2 SHORT*/

	 HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);           /*K_A_E */
	 HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_RESET);         /*KA_NC */
	 HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);           /*KA_NO */


	 HAL_GPIO_WritePin(GPIOE, GPIO_PIN_4, GPIO_PIN_RESET);        /*K_B_E */
	 HAL_GPIO_WritePin(GPIOF, GPIO_PIN_2, GPIO_PIN_RESET);         /*KB_NC */
	 HAL_GPIO_WritePin(GPIOE, GPIO_PIN_5, GPIO_PIN_SET);        /*KB_NO */

	 HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15,GPIO_PIN_RESET);    /*nSHTDN*/
	 HAL_GPIO_WritePin(GPIOD, GPIO_PIN_5, GPIO_PIN_SET);      /*nFLT  */
	 HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, GPIO_PIN_SET);     /*PGOOD */



	 HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5,  GPIO_PIN_RESET); /* K_SA_E1 */
     HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8,  GPIO_PIN_RESET); /* K_SA_E2 */
     HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_RESET); /* K_SA_E3 */

     HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET); /* K_SA_S1 */
     HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6,  GPIO_PIN_SET); /* K_SA_S2 */
     HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2,  GPIO_PIN_SET); /* K_SA_S3 */

     HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_SET); /* K_SB_E1 */
     HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET); /* K_SB_E2 */
     HAL_GPIO_WritePin(GPIOF, GPIO_PIN_14, GPIO_PIN_RESET); /* K_SB_E3 */

     HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, GPIO_PIN_RESET); /* K_SB_S1 */
     HAL_GPIO_WritePin(GPIOE, GPIO_PIN_7,  GPIO_PIN_SET); /* K_SB_S2 */
     HAL_GPIO_WritePin(GPIOE, GPIO_PIN_9,  GPIO_PIN_SET); /* K_SB_S3 */

     HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9,  GPIO_PIN_RESET); /* K_HA_E */
     HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4,  GPIO_PIN_RESET); /* K_HA_NC */
     HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET); /* K_HA_NO */

     HAL_GPIO_WritePin(GPIOF, GPIO_PIN_8,  GPIO_PIN_RESET); /* K_HB_E */
     HAL_GPIO_WritePin(GPIOG, GPIO_PIN_1,  GPIO_PIN_RESET); /* K_HB_NC */
     HAL_GPIO_WritePin(GPIOF, GPIO_PIN_9,  GPIO_PIN_RESET); /* K_HB_NO */

 }




void LedInitConfig(void)
{


	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_RESET);         /*L_LA1_G*/
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_3, GPIO_PIN_RESET);        /*L_LA1_R*/
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_RESET);       /*L_LA2_R*/
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);      /*L_LA2_G*/
	HAL_GPIO_WritePin(GPIOH, GPIO_PIN_2, GPIO_PIN_RESET);     /*L_LB1_G*/
	HAL_GPIO_WritePin(GPIOG, GPIO_PIN_10,GPIO_PIN_RESET);    /*L_LB1_R*/
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_6, GPIO_PIN_RESET);   /*L_LB2_R*/
	HAL_GPIO_WritePin(GPIOG, GPIO_PIN_15, GPIO_PIN_RESET); /*L_LB2_G*/



	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_2,  GPIO_PIN_RESET); /* L_SRT_G */
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_7,  GPIO_PIN_RESET); /* L_SRT_R */

    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_6,  GPIO_PIN_RESET); /* L_LX1_G */
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_7,  GPIO_PIN_RESET); /* L_LX1_R */

    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_RESET); /* L_LX2_G */
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2,  GPIO_PIN_RESET); /* L_LX2_R */

    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_RESET); /* L_LX3_G */
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, GPIO_PIN_RESET); /* L_LX3_R */

    HAL_GPIO_WritePin(GPIOG, GPIO_PIN_3,  GPIO_PIN_RESET); /* L_OPN_G */
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_6,  GPIO_PIN_RESET); /* L_OPN_R */


}







void MX_ADC3_Init(void)
{


    __HAL_RCC_ADC1_CLK_ENABLE();

    hadc3.Instance = ADC1;
    hadc3.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc3.Init.Resolution = ADC_RESOLUTION_12B;
    hadc3.Init.ScanConvMode = DISABLE;
    hadc3.Init.ContinuousConvMode = DISABLE;
    hadc3.Init.DiscontinuousConvMode = DISABLE;
    hadc3.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc3.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc3.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc3.Init.NbrOfConversion = 1;
    hadc3.Init.DMAContinuousRequests = DISABLE;
    hadc3.Init.EOCSelection = ADC_EOC_SINGLE_CONV;

    HAL_ADC_Init(&hadc1);

    //sConfig.Channel = ADC_CHANNEL_10;   // cambia questo
    //sConfig.Rank = 1;
    //sConfig.SamplingTime = ADC_SAMPLETIME_480CYCLES;

    //HAL_ADC_ConfigChannel(&hadc3, &sConfig);
}





void MX_ADC1_Init(void)
{


    __HAL_RCC_ADC3_CLK_ENABLE();

    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc1.Init.Resolution = ADC_RESOLUTION_12B;
    hadc1.Init.ScanConvMode = DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;

    HAL_ADC_Init(&hadc1);

    //sConfig.Channel = ADC_CHANNEL_10;   // cambia questo
    //sConfig.Rank = 1;
    //sConfig.SamplingTime = ADC_SAMPLETIME_480CYCLES;

    //HAL_ADC_ConfigChannel(&hadc3, &sConfig);
}


























#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  
  /* Infinite loop */
  while (1)
  {
  }
}
#endif
