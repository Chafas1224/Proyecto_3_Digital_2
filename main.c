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
#include "fatfs.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ili9341.h"
#include "bitmaps.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "fatfs_sd.h"
#include "string.h"
#include "stdio.h"
#include <stdbool.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
SPI_HandleTypeDef hspi1;
FATFS fs;
FATFS *pfs;
FIL fil;
FRESULT fres;
DWORD fre_clust;
uint32_t totalSpace, freeSpace;
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi1;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
bool parqueos[8] = { false };
extern const unsigned char indicador_verde[];   // Sprite verde
extern const unsigned char indicador_rojo[];
extern const unsigned char Fondo[];
extern const unsigned char sprite_carro[];
const uint16_t parqueoX[8] = { 11, 54, 98, 141, 11, 54, 98, 141 };
const uint16_t parqueoY[8] = { 20, 20, 20, 20, 158, 158, 158, 158 };

const uint16_t indicadorX[8] = { 8, 51, 95, 138, 8, 51, 95, 138 };
const uint16_t indicadorY[8] = { 96, 96, 96, 96, 129, 129, 129, 129 };
const GPIO_TypeDef *puertosSensores[4] = { GPIOC, GPIOB, GPIOB, GPIOB };
const uint16_t pinesSensores[4] = { GPIO_PIN_6, GPIO_PIN_13, GPIO_PIN_14,
		GPIO_PIN_15 };



GPIO_TypeDef* pinesRGB[8] = {GPIOC, GPIOC, GPIOA, GPIOA, GPIOC, GPIOC, GPIOA, GPIOB};
uint16_t pinesRGB_Pin[8] = {GPIO_PIN_8, GPIO_PIN_9, GPIO_PIN_11, GPIO_PIN_12,
                            GPIO_PIN_11, GPIO_PIN_12, GPIO_PIN_15, GPIO_PIN_9};

uint8_t  estado_parqueos_remotos= 0xF0;
uint8_t aTxBuffer[1] = { 0xF0 };
uint8_t sensores_remotos = 0;
uint8_t aRxBuffer[1];
uint8_t estado_remoto = 0xFF;
uint8_t prev_remoto   = 0x0F;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_ADC1_Init(void);
static void MX_SPI1_Init(void);
static void MX_I2C1_Init(void);
/* USER CODE BEGIN PFP */
void DibujarParqueo(uint8_t i);
void RevisarUART(void);
void MostrarDisponibles(void);
void ActualizarSensores(void);
void ActualizarParqueosI2C(void);
void ActualizarLedsParqueo(void);
uint8_t calcularEstadoParqueos(void);
void ActualizarPantalla(uint8_t estado);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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
  MX_USART2_UART_Init();
  MX_ADC1_Init();
  MX_SPI1_Init();
  MX_FATFS_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
  	HAL_I2C_EnableListen_IT(&hi2c1);
	LCD_Init();
	LCD_Clear(0x0000);
	LCD_Bitmap(0, 0, 320, 240, Fondo);

	// Inicializa todos los parqueos como libres visualmente
	for (int i = 0; i < 8; i++) {
		DibujarParqueo(i);
	}

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1) {
		RevisarUART();
		ActualizarSensores();     // A–D sólo si cambian
		ActualizarParqueosI2C();   // E–H sólo si cambian
		ActualizarLedsParqueo();   // LEDs según parqueos
		MostrarDisponibles();      // contador
		HAL_Delay(100);


	}
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

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
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
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
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_12;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 202;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

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
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, F_CS_Pin|LCD_RST_Pin|LCD_D1_Pin|GPIO_PIN_8
                          |GPIO_PIN_9|GPIO_PIN_11|GPIO_PIN_12, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, LCD_RD_Pin|LCD_WR_Pin|LCD_RS_Pin|LCD_D7_Pin
                          |LCD_D0_Pin|LCD_D2_Pin|GPIO_PIN_11|GPIO_PIN_12
                          |GPIO_PIN_15, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LCD_CS_Pin|LCD_D6_Pin|LCD_D3_Pin|LCD_D5_Pin
                          |LCD_D4_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SD_SS_GPIO_Port, SD_SS_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : F_CS_Pin LCD_RST_Pin LCD_D1_Pin PC8
                           PC9 PC11 PC12 */
  GPIO_InitStruct.Pin = F_CS_Pin|LCD_RST_Pin|LCD_D1_Pin|GPIO_PIN_8
                          |GPIO_PIN_9|GPIO_PIN_11|GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : LCD_RD_Pin LCD_WR_Pin LCD_RS_Pin LCD_D7_Pin
                           LCD_D0_Pin LCD_D2_Pin PA11 PA12
                           PA15 */
  GPIO_InitStruct.Pin = LCD_RD_Pin|LCD_WR_Pin|LCD_RS_Pin|LCD_D7_Pin
                          |LCD_D0_Pin|LCD_D2_Pin|GPIO_PIN_11|GPIO_PIN_12
                          |GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : LCD_CS_Pin LCD_D6_Pin LCD_D3_Pin LCD_D5_Pin
                           LCD_D4_Pin */
  GPIO_InitStruct.Pin = LCD_CS_Pin|LCD_D6_Pin|LCD_D3_Pin|LCD_D5_Pin
                          |LCD_D4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : B_Pin C_Pin D_Pin */
  GPIO_InitStruct.Pin = B_Pin|C_Pin|D_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : A_Pin */
  GPIO_InitStruct.Pin = A_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(A_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PD2 */
  GPIO_InitStruct.Pin = GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : SD_SS_Pin */
  GPIO_InitStruct.Pin = SD_SS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
  HAL_GPIO_Init(SD_SS_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

// Calcula el byte que se enviará
uint8_t calcularEstadoParqueos(void) {
    uint8_t estado = 0x00;
    for (int i = 0; i < 4; i++) {
        if (HAL_GPIO_ReadPin(puertosSensores[i], pinesSensores[i]) == GPIO_PIN_SET) {
        }

    }
    return estado;
}

void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c) {
    estado_remoto = aRxBuffer[0];
    // Reactiva la escucha
    HAL_I2C_EnableListen_IT(hi2c);
}


// Se activa cuando se detecta dirección
void HAL_I2C_AddrCallback(I2C_HandleTypeDef *hi2c, uint8_t TransferDirection, uint16_t AddrMatchCode)
{
    if (TransferDirection == I2C_DIRECTION_RECEIVE) {
        // Maestro lee los bits A–D
        aTxBuffer[0] = calcularEstadoParqueos();
        HAL_I2C_Slave_Seq_Transmit_IT(&hi2c1, aTxBuffer, 1, I2C_FIRST_AND_LAST_FRAME);
    } else {
        // Maestro escribe bits E–H en los aRxBuffer
        HAL_I2C_Slave_Seq_Receive_IT(&hi2c1, aRxBuffer, 1, I2C_FIRST_AND_LAST_FRAME);
    }
}



// Reactiva escucha al terminar
void HAL_I2C_ListenCpltCallback(I2C_HandleTypeDef *hi2c) {
    HAL_I2C_EnableListen_IT(hi2c);
}



void DibujarParqueo(uint8_t i) {
	const unsigned char *indicador =
			parqueos[i] ? indicador_rojo : indicador_verde;
	uint8_t index = parqueos[i] ? 0 : 1;

	if (parqueos[i]) {
		// Dibujar el sprite del carro en lugar el suelo
		LCD_Bitmap(parqueoX[i], parqueoY[i], 25, 45, sprite_carro);
	} else {
		FillRect(parqueoX[i], parqueoY[i], 27, 60, 0x8410);
	}

	LCD_Sprite(indicadorX[i], indicadorY[i], 32, 16, indicador, 1, index, 0, 0,
			0, 0);
}

void ProcesarEntrada(char c) {
	if (c >= 'A' && c <= 'D') {
		uint8_t i = c - 'A';
		parqueos[i] = true;
		DibujarParqueo(i);
		MostrarDisponibles();
	} else if (c >= 'a' && c <= 'd') {
		uint8_t i = c - 'a';
		parqueos[i] = false;
		DibujarParqueo(i);
		MostrarDisponibles();
	}
}

// Revisa si llegó un dato por UART2
void RevisarUART() {
	uint8_t dato;
	if (HAL_UART_Receive(&huart2, &dato, 1, 10) == HAL_OK) {
		ProcesarEntrada((char) dato);
	}
}

// Dibuja el número de parqueos disponibles
void MostrarDisponibles(void) {
	static int ultimo_libres = -1;
	int libres = 0;

	for (int i = 0; i < 8; i++) {
		if (!parqueos[i])
			libres++;
	}

	if (libres != ultimo_libres) {
		ultimo_libres = libres;

		// Limpia solo la parte necesaria
		FillRect(180, 45, 75, 40, 0x8410);  // Fond ogris

		LCD_Print("Libres:", 180, 50, 2, 0xFDA0, 0x8410);  // texto
		char num[4];
		sprintf(num, "%d", libres);
		LCD_Print(num, 180, 70, 2, 0xFDA0, 0x8410);        // número
	}
}

void ActualizarSensores(void) {
    for (int i = 0; i < 4; i++) {
        bool ocupado = (HAL_GPIO_ReadPin(puertosSensores[i], pinesSensores[i]) == GPIO_PIN_RESET);
        if (ocupado != parqueos[i]) {
            parqueos[i] = ocupado;
            DibujarParqueo(i);
            MostrarDisponibles();
        }
    }
}

void ActualizarParqueosI2C(void) {
    // Se queda solo con los 4 LSB que vienen por I2C
    uint8_t current = estado_remoto & 0x0F;
    // Se detectan que bits cambian
    uint8_t changed = current ^ prev_remoto;
    if (!changed) return;  // nada cambió, salimos

    for (int bit = 0; bit < 4; bit++) {
        if (changed & (1 << bit)) {

            bool ocupado = ((current & (1 << bit)) == 0);
            uint8_t idx = bit + 4;
            parqueos[idx] = ocupado;
            DibujarParqueo(idx);
        }
    }

    MostrarDisponibles();
    prev_remoto = current;
}




void ActualizarLedsParqueo(void) {
    // Parqueo A
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, parqueos[0] ? GPIO_PIN_RESET : GPIO_PIN_SET); // Verde
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, parqueos[0] ? GPIO_PIN_SET : GPIO_PIN_RESET); // Rojo

    // Parqueo B
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, parqueos[1] ? GPIO_PIN_RESET : GPIO_PIN_SET); // Verde
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, parqueos[1] ? GPIO_PIN_SET : GPIO_PIN_RESET); // Rojo

    // Parqueo C
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, parqueos[2] ? GPIO_PIN_RESET : GPIO_PIN_SET); // Verde
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, parqueos[2] ? GPIO_PIN_SET : GPIO_PIN_RESET); // Rojo

    // Parqueo D (corregido finalmente a PD2 para el rojo)
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, parqueos[3] ? GPIO_PIN_RESET : GPIO_PIN_SET); // Verde
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, parqueos[3] ? GPIO_PIN_SET : GPIO_PIN_RESET);  // Rojo
}



void ActualizarPantalla(uint8_t estado) {
    for (int i = 0; i < 8; i++) {
        uint8_t estado_bit = (estado >> i) & 0x01;

        // 1 = vacío, 0 = ocupado
        parqueos[i] = (estado_bit == 0);  // true = ocupado, false = libre

        DibujarParqueo(i);
    }
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1) {
	}
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
