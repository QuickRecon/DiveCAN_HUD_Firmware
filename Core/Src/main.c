/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "touchsensing.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "Hardware/leds.h"
#include "DiveCAN/DiveCAN.h"
#include "DiveCAN/Transciever.h"
#include "Hardware/pwr_management.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
typedef StaticTask_t osStaticThreadDef_t;
typedef StaticQueue_t osStaticMessageQDef_t;
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
CAN_HandleTypeDef hcan1;

CRC_HandleTypeDef hcrc;

IWDG_HandleTypeDef hiwdg;

TIM_HandleTypeDef htim7;

TSC_HandleTypeDef htsc;

/* Definitions for TSCTask */
osThreadId_t TSCTaskHandle;
uint32_t TSCTaskBuffer[128];
osStaticThreadDef_t TSCTaskControlBlock;
const osThreadAttr_t TSCTask_attributes = {
    .name = "TSCTask",
    .cb_mem = &TSCTaskControlBlock,
    .cb_size = sizeof(TSCTaskControlBlock),
    .stack_mem = &TSCTaskBuffer[0],
    .stack_size = sizeof(TSCTaskBuffer),
    .priority = (osPriority_t)osPriorityNormal,
};
/* Definitions for BlinkTask */
osThreadId_t BlinkTaskHandle;
uint32_t BlinkTaskBuffer[512];
osStaticThreadDef_t BlinkTaskControlBlock;
const osThreadAttr_t BlinkTask_attributes = {
    .name = "BlinkTask",
    .cb_mem = &BlinkTaskControlBlock,
    .cb_size = sizeof(BlinkTaskControlBlock),
    .stack_mem = &BlinkTaskBuffer[0],
    .stack_size = sizeof(BlinkTaskBuffer),
    .priority = (osPriority_t)osPriorityAboveNormal,
};
/* Definitions for AlertTask */
osThreadId_t AlertTaskHandle;
uint32_t AlertTaskBuffer[128];
osStaticThreadDef_t AlertTaskControlBlock;
const osThreadAttr_t AlertTask_attributes = {
    .name = "AlertTask",
    .cb_mem = &AlertTaskControlBlock,
    .cb_size = sizeof(AlertTaskControlBlock),
    .stack_mem = &AlertTaskBuffer[0],
    .stack_size = sizeof(AlertTaskBuffer),
    .priority = (osPriority_t)osPriorityNormal,
};
/* Definitions for PPO2Queue */
osMessageQueueId_t PPO2QueueHandle;
uint8_t PPO2QueueBuffer[1 * sizeof(CellValues_t)];
osStaticMessageQDef_t PPO2QueueControlBlock;
const osMessageQueueAttr_t PPO2Queue_attributes = {
    .name = "PPO2Queue",
    .cb_mem = &PPO2QueueControlBlock,
    .cb_size = sizeof(PPO2QueueControlBlock),
    .mq_mem = &PPO2QueueBuffer,
    .mq_size = sizeof(PPO2QueueBuffer)};
/* Definitions for CellStatQueue */
osMessageQueueId_t CellStatQueueHandle;
uint8_t CellStatBuffer[1 * sizeof(uint8_t)];
osStaticMessageQDef_t CellStatControlBlock;
const osMessageQueueAttr_t CellStatQueue_attributes = {
    .name = "CellStatQueue",
    .cb_mem = &CellStatControlBlock,
    .cb_size = sizeof(CellStatControlBlock),
    .mq_mem = &CellStatBuffer,
    .mq_size = sizeof(CellStatBuffer)};
/* USER CODE BEGIN PV */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_CAN1_Init(void);
static void MX_TSC_Init(void);
static void MX_CRC_Init(void);
// static void MX_IWDG_Init(void);
static void MX_TIM7_Init(void);
void TSCTaskFunc(void *argument);
void BlinkTaskFunc(void *argument);
void AlertTaskFunc(void *argument);

static void MX_NVIC_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static inline int16_t div10_round(int16_t x)
{
  /* rounds x/10 to nearest integer, handles negatives safely via int64_t */
  return (int16_t)(((int32_t)x + (x >= 0 ? 5 : -5)) / 10);
}

static inline bool cell_alert(uint8_t cellVal)
{
  return (cellVal < 40 || cellVal > 165);
}

/* Standard HAL structure defined globally or in main */
extern TSC_HandleTypeDef htsc;

uint32_t Get_TSC_RawValue(uint32_t ChannelIO, uint32_t SamplingIO, uint32_t GroupIndex)
{
  uint32_t raw_value = 0;
  TSC_IOConfigTypeDef ioConfig = {0};

  // 1. Discharge all IOs to ensure a clean starting state
  HAL_TSC_IODischarge(&htsc, ENABLE);
  HAL_Delay(1); // Wait for discharge (depends on Cs value)
  HAL_TSC_IODischarge(&htsc, DISABLE);

  // 2. Configure the specific IO Group for this measurement
  ioConfig.ChannelIOs = ChannelIO;   // e.g., TSC_GROUP2_IO1
  ioConfig.SamplingIOs = SamplingIO; // e.g., TSC_GROUP2_IO2
  if (HAL_TSC_IOConfig(&htsc, &ioConfig) != HAL_OK)
  {
    return 0xFFFF; // Error
  }

  // 3. Start acquisition and poll for completion
  if (HAL_TSC_Start(&htsc) == HAL_OK)
  {
    // Polling avoids the need for the RTOS scheduler or interrupts
    if (HAL_TSC_PollForAcquisition(&htsc) == HAL_OK)
    {
      // 4. Retrieve the raw counter value for the group
      raw_value = HAL_TSC_GroupGetValue(&htsc, GroupIndex);
    } else {
      NON_FATAL_ERROR(TSC_ERR);
    }
  } else {
      NON_FATAL_ERROR(TSC_ERR);
  }

  return raw_value;
}

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_CAN1_Init();
  MX_TSC_Init();
  MX_TOUCHSENSING_Init();
  MX_CRC_Init();
  // MX_IWDG_Init();
  MX_TIM7_Init();

  /* Initialize interrupts */
  MX_NVIC_Init();
  /* USER CODE BEGIN 2 */

  // Check for finger before starting up
  volatile uint32_t touchVal = Get_TSC_RawValue(TSC_GROUP2_IO1, TSC_GROUP2_IO2, TSC_GROUP2_IDX);

  if (touchVal < 590)
  { // Example threshold
    // Execute "Pre-boot" or "Hidden Menu" logic here
  }

  const DiveCANDevice_t defaultDeviceSpec = {
      .name = "ALHUD",
      .type = DIVECAN_MONITOR,
      .manufacturerID = DIVECAN_MANUFACTURER_SRI,
      .firmwareVersion = 1};

  initLEDs();

  InitDiveCAN(&defaultDeviceSpec);
  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of PPO2Queue */
  PPO2QueueHandle = osMessageQueueNew(1, sizeof(CellValues_t), &PPO2Queue_attributes);

  /* creation of CellStatQueue */
  CellStatQueueHandle = osMessageQueueNew(1, sizeof(uint8_t), &CellStatQueue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of TSCTask */
  TSCTaskHandle = osThreadNew(TSCTaskFunc, NULL, &TSCTask_attributes);

  /* creation of BlinkTask */
  BlinkTaskHandle = osThreadNew(BlinkTaskFunc, NULL, &BlinkTask_attributes);

  /* creation of AlertTask */
  AlertTaskHandle = osThreadNew(AlertTaskFunc, NULL, &AlertTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  Shutdown();
  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    NVIC_SystemReset();
  }
  /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
   */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI | RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 8;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV8;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }

  /** Enables the Clock Security System
   */
  HAL_RCC_EnableCSS();
}

/**
 * @brief NVIC Configuration.
 * @retval None
 */
static void MX_NVIC_Init(void)
{
  /* EXTI15_10_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
  /* CAN1_RX0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(CAN1_RX0_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(CAN1_RX0_IRQn);
  /* CAN1_RX1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(CAN1_RX1_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(CAN1_RX1_IRQn);
}

/**
 * @brief CAN1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_CAN1_Init(void)
{

  /* USER CODE BEGIN CAN1_Init 0 */

  /* USER CODE END CAN1_Init 0 */

  /* USER CODE BEGIN CAN1_Init 1 */

  /* USER CODE END CAN1_Init 1 */
  hcan1.Instance = CAN1;
  hcan1.Init.Prescaler = 4;
  hcan1.Init.Mode = CAN_MODE_NORMAL;
  hcan1.Init.SyncJumpWidth = CAN_SJW_4TQ;
  hcan1.Init.TimeSeg1 = CAN_BS1_13TQ;
  hcan1.Init.TimeSeg2 = CAN_BS2_2TQ;
  hcan1.Init.TimeTriggeredMode = DISABLE;
  hcan1.Init.AutoBusOff = ENABLE;
  hcan1.Init.AutoWakeUp = ENABLE;
  hcan1.Init.AutoRetransmission = DISABLE;
  hcan1.Init.ReceiveFifoLocked = DISABLE;
  hcan1.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CAN1_Init 2 */
  CAN_FilterTypeDef sFilterConfig = {0};
  sFilterConfig.FilterFIFOAssignment = CAN_FILTER_FIFO0; /* set fifo assignment */
  sFilterConfig.FilterIdHigh = 0;
  sFilterConfig.FilterIdLow = 0;
  sFilterConfig.FilterMaskIdHigh = 0;
  sFilterConfig.FilterMaskIdLow = 0;
  sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT; /* set filter scale */
  sFilterConfig.FilterActivation = ENABLE;
  (void)HAL_CAN_ConfigFilter(&hcan1, &sFilterConfig);
  (void)HAL_CAN_Start(&hcan1);                                             /* start CAN */
  (void)HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING); /* enable interrupts */
  /* USER CODE END CAN1_Init 2 */
}

/**
 * @brief CRC Initialization Function
 * @param None
 * @retval None
 */
static void MX_CRC_Init(void)
{

  /* USER CODE BEGIN CRC_Init 0 */

  /* USER CODE END CRC_Init 0 */

  /* USER CODE BEGIN CRC_Init 1 */

  /* USER CODE END CRC_Init 1 */
  hcrc.Instance = CRC;
  hcrc.Init.DefaultPolynomialUse = DEFAULT_POLYNOMIAL_ENABLE;
  hcrc.Init.DefaultInitValueUse = DEFAULT_INIT_VALUE_ENABLE;
  hcrc.Init.InputDataInversionMode = CRC_INPUTDATA_INVERSION_NONE;
  hcrc.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE;
  hcrc.InputDataFormat = CRC_INPUTDATA_FORMAT_BYTES;
  if (HAL_CRC_Init(&hcrc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CRC_Init 2 */

  /* USER CODE END CRC_Init 2 */
}

// /**
//  * @brief IWDG Initialization Function
//  * @param None
//  * @retval None
//  */
// static void MX_IWDG_Init(void)
// {

//   /* USER CODE BEGIN IWDG_Init 0 */

//   /* USER CODE END IWDG_Init 0 */

//   /* USER CODE BEGIN IWDG_Init 1 */

//   /* USER CODE END IWDG_Init 1 */
//   hiwdg.Instance = IWDG;
//   hiwdg.Init.Prescaler = IWDG_PRESCALER_4;
//   hiwdg.Init.Window = 4095;
//   hiwdg.Init.Reload = 4095;
//   if (HAL_IWDG_Init(&hiwdg) != HAL_OK)
//   {
//     Error_Handler();
//   }
//   /* USER CODE BEGIN IWDG_Init 2 */

//   /* USER CODE END IWDG_Init 2 */
// }

/**
 * @brief TIM7 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM7_Init(void)
{

  /* USER CODE BEGIN TIM7_Init 0 */

  /* USER CODE END TIM7_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM7_Init 1 */

  /* USER CODE END TIM7_Init 1 */
  htim7.Instance = TIM7;
  htim7.Init.Prescaler = 0;
  htim7.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim7.Init.Period = 1600;
  htim7.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim7) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim7, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM7_Init 2 */

  /* USER CODE END TIM7_Init 2 */
}

/**
 * @brief TSC Initialization Function
 * @param None
 * @retval None
 */
static void MX_TSC_Init(void)
{

  /* USER CODE BEGIN TSC_Init 0 */

  /* USER CODE END TSC_Init 0 */

  /* USER CODE BEGIN TSC_Init 1 */

  /* USER CODE END TSC_Init 1 */

  /** Configure the TSC peripheral
   */
  htsc.Instance = TSC;
  htsc.Init.CTPulseHighLength = TSC_CTPH_2CYCLES;
  htsc.Init.CTPulseLowLength = TSC_CTPL_2CYCLES;
  htsc.Init.SpreadSpectrum = DISABLE;
  htsc.Init.SpreadSpectrumDeviation = 1;
  htsc.Init.SpreadSpectrumPrescaler = TSC_SS_PRESC_DIV1;
  htsc.Init.PulseGeneratorPrescaler = TSC_PG_PRESC_DIV4;
  htsc.Init.MaxCountValue = TSC_MCV_8191;
  htsc.Init.IODefaultMode = TSC_IODEF_OUT_PP_LOW;
  htsc.Init.SynchroPinPolarity = TSC_SYNC_POLARITY_FALLING;
  htsc.Init.AcquisitionMode = TSC_ACQ_MODE_NORMAL;
  htsc.Init.MaxCountInterrupt = DISABLE;
  htsc.Init.ChannelIOs = TSC_GROUP2_IO1;
  htsc.Init.ShieldIOs = 0;
  htsc.Init.SamplingIOs = TSC_GROUP2_IO2;
  if (HAL_TSC_Init(&htsc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TSC_Init 2 */

  /* USER CODE END TSC_Init 2 */
}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(ASC_EN_GPIO_Port, ASC_EN_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, B1_Pin | G1_Pin | R1_Pin | B2_Pin | G2_Pin | R2_Pin | R3_Pin | G3_Pin | B3_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LED_0_Pin | LED_1_Pin | LED_2_Pin | LED_3_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : CAN_EN_Pin */
  GPIO_InitStruct.Pin = CAN_EN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(CAN_EN_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : ASC_EN_Pin */
  GPIO_InitStruct.Pin = ASC_EN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(ASC_EN_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : B1_Pin G1_Pin R1_Pin B2_Pin
                           G2_Pin R2_Pin R3_Pin G3_Pin
                           B3_Pin */
  GPIO_InitStruct.Pin = B1_Pin | G1_Pin | R1_Pin | B2_Pin | G2_Pin | R2_Pin | R3_Pin | G3_Pin | B3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : LED_0_Pin LED_1_Pin LED_2_Pin LED_3_Pin */
  GPIO_InitStruct.Pin = LED_0_Pin | LED_1_Pin | LED_2_Pin | LED_3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PA9 PA10 */
  GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PH3 */
  GPIO_InitStruct.Pin = GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void TSC_Handler(void)
{
  if (MyTKeysB[0].p_Data->StateId == TSL_STATEID_DETECT)
  {
    // HAL_GPIO_WritePin(LED_0_GPIO_Port, LED_0_Pin, GPIO_PIN_SET);
  }
  else if (MyTKeysB[0].p_Data->StateId == TSL_STATEID_RELEASE)
  {
    // HAL_GPIO_WritePin(LED_0_GPIO_Port, LED_0_Pin, GPIO_PIN_RESET);
  }
}
/* USER CODE END 4 */

/* USER CODE BEGIN Header_TSCTaskFunc */
/**
 * @brief  Function implementing the TSCTask thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_TSCTaskFunc */
void TSCTaskFunc(void *argument)
{
  /* USER CODE BEGIN 5 */
  /* Infinite loop */
  for (;;)
  {
    tsl_user_Exec();
    TSC_Handler();
    osDelay(1);
  }
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_BlinkTaskFunc */
/**
 * @brief Function implementing the BlinkTask thread.
 * @param argument: Not used
 * @retval None
 */
static bool alerting = false;
/* USER CODE END Header_BlinkTaskFunc */
void BlinkTaskFunc(void *argument)
{
  /* USER CODE BEGIN BlinkTaskFunc */
  const uint8_t centerValue = 100;
  osDelay(TIMEOUT_2S_TICKS); /* Initial delay to for the DiveCAN system to start up and prime the queue */
  CellValues_t cellValues = {0};
  for (;;)
  {
    /* Dequeue the latest PPO2 information */
    osStatus_t osStat = osMessageQueueGet(PPO2QueueHandle, &cellValues, NULL, 0);
    if (osStat != osOK)
    {
      blinkNoData();
    }
    else if (cell_alert(cellValues.C1) || cell_alert(cellValues.C2) || cell_alert(cellValues.C3))
    {
      alerting = true;
      blinkAlarm();
    }
    else
    {
      alerting = false;
      osDelay(TIMEOUT_500MS_TICKS); /* Use an extra delay to "partition" the segments */
    }

    int16_t c1 = cellValues.C1 - centerValue;
    int16_t c2 = cellValues.C2 - centerValue;
    int16_t c3 = cellValues.C3 - centerValue;

    c1 = div10_round(c1);
    c2 = div10_round(c2);
    c3 = div10_round(c3);

    uint8_t failMask = ((cellValues.C1 == 0xFF ? 0 : 1) << 0) |
                       ((cellValues.C2 == 0xFF ? 0 : 1) << 1) |
                       ((cellValues.C3 == 0xFF ? 0 : 1) << 2);

    uint8_t statusMask = 0b111;
    osStat = osMessageQueueGet(CellStatQueueHandle, &statusMask, NULL, 0);
    if (osStat != osOK)
    {
      statusMask = 0b111; // Default to all good if no status available
    }

    blinkCode((int8_t)c1, (int8_t)c2, (int8_t)c3, statusMask, failMask);
  }
  /* USER CODE END BlinkTaskFunc */
}

/* USER CODE BEGIN Header_AlertTaskFunc */
/**
 * @brief Function implementing the AlertTask thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_AlertTaskFunc */
void AlertTaskFunc(void *argument)
{
  /* USER CODE BEGIN AlertTaskFunc */
  /* Infinite loop */
  for (;;)
  {
    if (alerting)
    {
      HAL_GPIO_WritePin(LED_0_GPIO_Port, LED_0_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(LED_1_GPIO_Port, LED_1_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(LED_2_GPIO_Port, LED_2_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(LED_3_GPIO_Port, LED_3_Pin, GPIO_PIN_SET);
      osDelay(TIMEOUT_100MS_TICKS);
      HAL_GPIO_WritePin(LED_0_GPIO_Port, LED_0_Pin, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(LED_1_GPIO_Port, LED_1_Pin, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(LED_2_GPIO_Port, LED_2_Pin, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(LED_3_GPIO_Port, LED_3_Pin, GPIO_PIN_RESET);
    }
    osDelay(TIMEOUT_100MS_TICKS);
  }
  /* USER CODE END AlertTaskFunc */
}

/**
 * @brief  Period elapsed callback in non blocking mode
 * @note   This function is called  when TIM6 interrupt took place, inside
 * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
 * a global variable "uwTick" used as application time base.
 * @param  htim : TIM handle
 * @retval None
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM6)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  NON_FATAL_ERROR(CRITICAL_ERR);
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(const uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
