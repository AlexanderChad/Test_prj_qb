/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2022 STMicroelectronics.
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
#define SETTINGS_ADDRESS 0x0800FC00 //адрес для сохранения настроек
#define L0_interval_def 100
#define L1_interval_def 200
struct lines_settings {
	unsigned int L0_interval;
	unsigned int L1_interval;
};
struct lines_settings lss;
void WriteConfig() {
	HAL_FLASH_Unlock(); // Открыть доступ к FLASH (она закрыта от случайной записи)
	// В структуре settings хранятся настройки, преобразую ее в 16-битный массив для удобства доступа
	uint16_t *data = (uint16_t*) &lss;
	FLASH_EraseInitTypeDef ef; // Объявляю структуру, необходимую для функции стирания страницы
	HAL_StatusTypeDef stat;
	ef.TypeErase = FLASH_TYPEERASE_PAGES; // Стирать постранично
	ef.PageAddress = SETTINGS_ADDRESS; // Адрес страницы для стирания
	ef.NbPages = 1; //Число страниц = 1
	uint32_t temp; // Временная переменная для результата стирания (не использую)
	HAL_FLASHEx_Erase(&ef, &temp); // Вызов функции стирания
	// Будьте уверены, что размер структуры настроек кратен 2 байтам
	for (int i = 0; i < sizeof(lss); i += 2) { // Запись всех настроек
		stat = HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,
		SETTINGS_ADDRESS + i, *(data++));
		if (stat != HAL_OK)
			break; // Если что-то пошло не так - выскочить из цикла
	}
	HAL_FLASH_Lock(); // Закрыть флешку от случайной записи
}
uint32_t FlashRead(uint32_t address) {
	return (*(__IO uint32_t*) address);
}
// Пример чтения 4 байт настроек.
void ReadConfig() {
	// Структуру настроек превращаю в указатель на массив 8-ми битных значений
	uint8_t *setData = (uint8_t*) &lss;
	uint32_t tempData = FlashRead(SETTINGS_ADDRESS); // Прочесть слово из флешки
	if (tempData != 0xffffffff) { // Если флешка не пустая
		setData[0] = (uint8_t) ((tempData & 0xff000000) >> 24); // �?звлечь первый байт из слова
		setData[1] = (uint8_t) ((tempData & 0x00ff0000) >> 16); // �?звлечь второй байт из слова
		setData[2] = (uint8_t) ((tempData & 0x0000ff00) >> 8); // �?злечь третий байт из слова
		setData[3] = tempData & 0xff; // �?звлечь четвертый байт из слова
	}
}
//проверка, пора ли менять состояние линии
void CheckLineState(uint32_t now_time, uint32_t *check_time,
		uint16_t L_interval, uint16_t L_pin) {
	if (now_time - *check_time >= L_interval) { //если пора
		HAL_GPIO_TogglePin(GPIOB, L_pin); //сменили состояние
		*check_time = now_time; //запомнили время
	}
}
char* S_Parser(char *string) {
	static char *source = NULL;
	char *p, *next_data = 0;
	if (string != NULL) {
		source = string;
	}
	if (source == NULL) {
		return NULL;
	}
	if ((p = strpbrk(source, ",")) != NULL) {
		*p = 0;
		next_data = source;
		source = ++p;
	}
	return next_data;
}
//формат: SET,100,200,END\n
//где 100-интервал для первой линии, 200-второй
#define SIZE_BF_SET 17 //размер команды настройки
char SET_data[SIZE_BF_SET] = { 0 };
const char str_SET_data[4] = "SET";
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	if (huart == &huart1) {
		char *pstr = strstr(SET_data, str_SET_data);  //Поиск строки
		char *istr = S_Parser(pstr);  //Парсим значения из SET
		uint8_t i_d_SET = 0;
		while (istr != NULL)  // Выделение последующих частей
		{
			switch (i_d_SET) {
			case 1:
				sscanf(istr, "%3d", &lss.L0_interval);
				break;
			case 2:
				sscanf(istr, "%3d", &lss.L1_interval);
				break;
			}
			istr = S_Parser(NULL);  // Выделение очередной части строки
			i_d_SET++;
		}
		WriteConfig(); //сохраняем
	}
}
/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {
	/* USER CODE BEGIN 1 */
	ReadConfig();
	if (!lss.L0_interval || !lss.L1_interval) { //если хотя бы один из интервалов == 0, заменяем преднастройкой
		lss.L0_interval = L0_interval_def;
		lss.L1_interval = L1_interval_def;
		WriteConfig(); //сохраняем
	}
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
	MX_USART1_UART_Init();
	/* USER CODE BEGIN 2 */
	uint32_t i_tick = 0; //счетчик прохождения main while
	//счетчики для линий
	uint32_t i_L0 = 0;
	uint32_t i_L1 = 0;
	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1) {
		i_tick++;
		CheckLineState(i_tick, &i_L0, lss.L0_interval, L0_Pin);
		CheckLineState(i_tick, &i_L1, lss.L1_interval, L1_Pin);
		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */
	}
	/* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };
	RCC_PeriphCLKInitTypeDef PeriphClkInit = { 0 };

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) {
		Error_Handler();
	}
	PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1;
	PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK1;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
		Error_Handler();
	}
}

/**
 * @brief USART1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART1_UART_Init(void) {

	/* USER CODE BEGIN USART1_Init 0 */

	/* USER CODE END USART1_Init 0 */

	/* USER CODE BEGIN USART1_Init 1 */

	/* USER CODE END USART1_Init 1 */
	huart1.Instance = USART1;
	huart1.Init.BaudRate = 38400;
	huart1.Init.WordLength = UART_WORDLENGTH_8B;
	huart1.Init.StopBits = UART_STOPBITS_1;
	huart1.Init.Parity = UART_PARITY_NONE;
	huart1.Init.Mode = UART_MODE_TX_RX;
	huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart1.Init.OverSampling = UART_OVERSAMPLING_16;
	huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
	huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
	if (HAL_UART_Init(&huart1) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN USART1_Init 2 */

	/* USER CODE END USART1_Init 2 */

}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void) {
	GPIO_InitTypeDef GPIO_InitStruct = { 0 };

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOB, L1_Pin | L0_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pins : L1_Pin L0_Pin */
	GPIO_InitStruct.Pin = L1_Pin | L0_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
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
