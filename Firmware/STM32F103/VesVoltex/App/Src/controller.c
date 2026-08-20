#include "controller.h"

#include "main.h"
#include "tim.h"
#include "usbd_customhid.h"

extern USBD_HandleTypeDef hUsbDeviceFS;

#define BUTTON_COUNT 7U
#define BUTTON_DEBOUNCE_MASK 0x07U
#define INPUT_REPORT_ID 0x01U

enum
{
  BUTTON_START = 0,
  BUTTON_BT_A,
  BUTTON_BT_B,
  BUTTON_BT_C,
  BUTTON_BT_D,
  BUTTON_FX_L,
  BUTTON_FX_R
};

static GPIO_TypeDef *const button_ports[BUTTON_COUNT] =
{
  START_SW_GPIO_Port,
  BT_A_SW_GPIO_Port,
  BT_B_SW_GPIO_Port,
  BT_C_SW_GPIO_Port,
  BT_D_SW_GPIO_Port,
  FX_L_SW_GPIO_Port,
  FX_R_SW_GPIO_Port
};

static const uint16_t button_pins[BUTTON_COUNT] =
{
  START_SW_Pin,
  BT_A_SW_Pin,
  BT_B_SW_Pin,
  BT_C_SW_Pin,
  BT_D_SW_Pin,
  FX_L_SW_Pin,
  FX_R_SW_Pin
};

static GPIO_TypeDef *const light_ports[BUTTON_COUNT] =
{
  BT_A_LED_GPIO_Port,
  BT_B_LED_GPIO_Port,
  BT_C_LED_GPIO_Port,
  BT_D_LED_GPIO_Port,
  FX_L_LED_GPIO_Port,
  FX_R_LED_GPIO_Port,
  START_LED_GPIO_Port
};

static const uint16_t light_pins[BUTTON_COUNT] =
{
  BT_A_LED_Pin,
  BT_B_LED_Pin,
  BT_C_LED_Pin,
  BT_D_LED_Pin,
  FX_L_LED_Pin,
  FX_R_LED_Pin,
  START_LED_Pin
};

static uint8_t button_history[BUTTON_COUNT];
static uint16_t button_state;
static uint32_t last_scan_tick;

void Controller_Init(void)
{
  button_state = 0U;

  for (uint32_t i = 0U; i < BUTTON_COUNT; ++i)
  {
    if (HAL_GPIO_ReadPin(button_ports[i], button_pins[i]) == GPIO_PIN_RESET)
    {
      button_history[i] = 0U;
      button_state |= (uint16_t)(1U << i);
    }
    else
    {
      button_history[i] = BUTTON_DEBOUNCE_MASK;
    }
  }

  __HAL_TIM_SET_COUNTER(&htim2, 0U);
  __HAL_TIM_SET_COUNTER(&htim4, 0U);

  if (HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_TIM_Encoder_Start(&htim4, TIM_CHANNEL_ALL) != HAL_OK)
  {
    Error_Handler();
  }

  last_scan_tick = HAL_GetTick();
}

void Controller_Task(void)
{
  uint8_t report[7];
  uint32_t now = HAL_GetTick();

  if (now == last_scan_tick)
  {
    return;
  }
  last_scan_tick = now;

  for (uint32_t i = 0U; i < BUTTON_COUNT; ++i)
  {
    uint8_t released = HAL_GPIO_ReadPin(button_ports[i], button_pins[i]) == GPIO_PIN_SET ? 1U : 0U;
    button_history[i] = (uint8_t)(((button_history[i] << 1U) | released) & BUTTON_DEBOUNCE_MASK);

    if (button_history[i] == 0U)
    {
      button_state |= (uint16_t)(1U << i);
    }
    else if (button_history[i] == BUTTON_DEBOUNCE_MASK)
    {
      button_state &= (uint16_t)~(1U << i);
    }
  }

  uint16_t vol_l = (uint16_t)__HAL_TIM_GET_COUNTER(&htim2);
  uint16_t vol_r = (uint16_t)__HAL_TIM_GET_COUNTER(&htim4);

  report[0] = INPUT_REPORT_ID;
  report[1] = (uint8_t)button_state;
  report[2] = (uint8_t)(button_state >> 8U);
  report[3] = (uint8_t)vol_l;
  report[4] = (uint8_t)(vol_l >> 8U);
  report[5] = (uint8_t)vol_r;
  report[6] = (uint8_t)(vol_r >> 8U);

  USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS, report, sizeof(report));
}

void Controller_SetLights(const uint8_t light_values[7])
{
  for (uint32_t i = 0U; i < BUTTON_COUNT; ++i)
  {
    HAL_GPIO_WritePin(light_ports[i], light_pins[i], light_values[i] == 0U ? GPIO_PIN_RESET : GPIO_PIN_SET);
  }
}
