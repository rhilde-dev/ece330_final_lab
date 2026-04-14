/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c  BATTLESHIP FINAL PROJECT
 * @brief          : Main program body — Electronic Battleship for STM32F407
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
 *
 *  GAME OVERVIEW:
 *  Two-player Battleship game played on 7-segment displays.
 *  Each player places 3 single-segment and 2 double-segment boats.
 *  Players alternate taking shots until one sinks all 7 opponent segments.
 *
 *  HARDWARE MAPPING:
 *  - Port E [7:0]  = 7-segment data, Port E [15:8] = digit select (active low)
 *  - Port A [3:0]  = ADC analog inputs (potentiometers for cursor)
 *  - Port C [3:0]  = Switch inputs (fire, map select, orientation, start)
 *  - Port D [0]    = Speaker/buzzer output (toggled by TIM7 for tones)
 *  - Port D [15:12]= RGB LEDs (PWM dimming)
 *
 *  MAP ENCODING (array-based):
 *  vertMap[2][16]:  [0][0..15] = top row, [1][0..15] = bottom row
 *  horizMap[3][8]:  [0][0..7]  = top row, [1][0..7] = middle, [2][0..7] =
 * bottom
 *
 *  BUTTON MAPPING:
 *  - PC11 = Confirm / Start / Fire
 *  - PC10 = Orient / V-H map toggle
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "seg7.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
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
I2C_HandleTypeDef hi2c1;

I2S_HandleTypeDef hi2s3;

SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim7;

/* USER CODE BEGIN PV */

/*============================================================================
 * Lab 6 inherited variables (music, animation, dimming)
 *===========================================================================*/
int DelayValue = 50;

char ramp = 0;
char RED_BRT = 0;
char GREEN_BRT = 0;
char BLUE_BRT = 0;
char RED_STEP = 1;
char GREEN_STEP = 2;
char BLUE_STEP = 3;
char DIM_Enable = 0;
char Music_ON = 0;
int TONE = 0;
int COUNT = 0;
int INDEX = 0;
int Note = 0;
int Save_Note = 0;
int Vibrato_Depth = 1;
int Vibrato_Rate = 40;
int Vibrato_Count = 0;
char Animate_On = 0;
char Message_Length = 0;
char *Message_Pointer;
char *Save_Pointer;
int Delay_msec = 0;
int Delay_counter = 0;

/*============================================================================
 * Battleship game global variables
 *===========================================================================*/

/* Game state machine */
volatile GameState_t Game_State = STATE_WELCOME;
volatile uint8_t Current_Player = 1;

/* Player maps: boat positions and shot history for each player */
PlayerMaps_t P1_Maps = {0};
PlayerMaps_t P2_Maps = {0};

/* Cursor for navigating the map */
Cursor_t Cursor = {0, 0, 0, 1};

/* Boat placement sub-state tracker */
volatile BoatPlaceState_t Boat_State = BOAT_SINGLE_1;

/* Hit counters — game ends when one reaches TOTAL_BOAT_SEGMENTS (7) */
volatile uint8_t P1_Hits = 0;
volatile uint8_t P2_Hits = 0;

/* Display buffer: each element holds the 7-seg pattern for one digit.
 * The SysTick ISR continuously outputs this buffer to the physical display. */
volatile uint8_t Display_Buffer[8] = {0};

/* Brightness buffer: PWM duty per digit (0 = off, 10 = full brightness).
 * The SysTick ISR compares against a rolling ramp counter for software PWM. */
volatile uint8_t Brightness_Buffer[8] = {10, 10, 10, 10, 10, 10, 10, 10};

/* Cursor blink control — toggled by SysTick */
volatile uint16_t Blink_Counter = 0;
volatile uint8_t Blink_State = 1;

/* Sound effect currently Playing */
volatile SoundEffect_t Current_SFX = SFX_NONE;

/* Declare array for Song (from lab 6) */
Music Song[100];

/*============================================================================
 * Marquee message character arrays for different game phases.
 * Uses the seg7.h character defines (CHAR_A, CHAR_B, SPACE, etc.)
 *===========================================================================*/

/* "  BATTLESHIP  " — welcome screen */
char Msg_Welcome[] = {SPACE,  SPACE,  SPACE,  SPACE,  SPACE,  SPACE,  SPACE,
                      SPACE,  CHAR_B, CHAR_A, CHAR_T, CHAR_T, CHAR_L, CHAR_E,
                      CHAR_S, CHAR_H, CHAR_I, CHAR_P, SPACE,  SPACE,  SPACE,
                      SPACE,  SPACE,  SPACE,  SPACE,  SPACE};

/* "  P1 PLACE  " — Player 1 placement phase */
char Msg_P1Place[] = {SPACE,  SPACE,  SPACE,  SPACE,  SPACE, SPACE,
                      SPACE,  SPACE,  CHAR_P, CHAR_1, SPACE, CHAR_P,
                      CHAR_L, CHAR_A, CHAR_C, CHAR_E, SPACE, SPACE,
                      SPACE,  SPACE,  SPACE,  SPACE,  SPACE, SPACE};

/* "  P2 PLACE  " — Player 2 placement phase */
char Msg_P2Place[] = {SPACE,  SPACE,  SPACE,  SPACE,  SPACE, SPACE,
                      SPACE,  SPACE,  CHAR_P, CHAR_2, SPACE, CHAR_P,
                      CHAR_L, CHAR_A, CHAR_C, CHAR_E, SPACE, SPACE,
                      SPACE,  SPACE,  SPACE,  SPACE,  SPACE, SPACE};

/* "  P1 TURN  " — Player 1 shooting turn */
char Msg_P1Turn[] = {SPACE,  SPACE,  SPACE,  SPACE,  SPACE, SPACE,
                     SPACE,  SPACE,  CHAR_P, CHAR_1, SPACE, CHAR_T,
                     CHAR_U, CHAR_R, CHAR_N, SPACE,  SPACE, SPACE,
                     SPACE,  SPACE,  SPACE,  SPACE,  SPACE};

/* "  P2 TURN  " — Player 2 shooting turn */
char Msg_P2Turn[] = {SPACE,  SPACE,  SPACE,  SPACE,  SPACE, SPACE,
                     SPACE,  SPACE,  CHAR_P, CHAR_2, SPACE, CHAR_T,
                     CHAR_U, CHAR_R, CHAR_N, SPACE,  SPACE, SPACE,
                     SPACE,  SPACE,  SPACE,  SPACE,  SPACE};

/* "  HIT  " — shot result */
char Msg_Hit[] = {SPACE, SPACE, SPACE,  SPACE,  SPACE,  SPACE, SPACE, SPACE,
                  SPACE, SPACE, CHAR_H, CHAR_I, CHAR_T, SPACE, SPACE, SPACE,
                  SPACE, SPACE, SPACE,  SPACE,  SPACE,  SPACE, SPACE};

/* "  MISS  " — shot result */
char Msg_Miss[] = {SPACE, SPACE,  SPACE,  SPACE,  SPACE,  SPACE, SPACE, SPACE,
                   SPACE, CHAR_M, CHAR_I, CHAR_S, CHAR_S, SPACE, SPACE, SPACE,
                   SPACE, SPACE,  SPACE,  SPACE,  SPACE,  SPACE};

/* "  P1 WINS  " — Player 1 victory */
char Msg_P1Wins[] = {SPACE,  SPACE,  SPACE,  SPACE,  SPACE, SPACE,
                     SPACE,  SPACE,  CHAR_P, CHAR_1, SPACE, CHAR_W,
                     CHAR_I, CHAR_N, CHAR_S, SPACE,  SPACE, SPACE,
                     SPACE,  SPACE,  SPACE,  SPACE,  SPACE};

/* "  P2 WINS  " — Player 2 victory */
char Msg_P2Wins[] = {SPACE,  SPACE,  SPACE,  SPACE,  SPACE, SPACE,
                     SPACE,  SPACE,  CHAR_P, CHAR_2, SPACE, CHAR_W,
                     CHAR_I, CHAR_N, CHAR_S, SPACE,  SPACE, SPACE,
                     SPACE,  SPACE,  SPACE,  SPACE,  SPACE};

/* "  PASS TO P2  " — hand controller to P2 */
char Msg_PassToP2[] = {SPACE,  SPACE,  SPACE,  SPACE,  SPACE,  SPACE, SPACE,
                       SPACE,  CHAR_P, CHAR_A, CHAR_S, CHAR_S, SPACE, CHAR_T,
                       CHAR_O, SPACE,  CHAR_P, CHAR_2, SPACE,  SPACE, SPACE,
                       SPACE,  SPACE,  SPACE,  SPACE,  SPACE};

/* "  PASS TO P1  " — hand controller to P1 */
char Msg_PassToP1[] = {SPACE,  SPACE,  SPACE,  SPACE,  SPACE,  SPACE, SPACE,
                       SPACE,  CHAR_P, CHAR_A, CHAR_S, CHAR_S, SPACE, CHAR_T,
                       CHAR_O, SPACE,  CHAR_P, CHAR_1, SPACE,  SPACE, SPACE,
                       SPACE,  SPACE,  SPACE,  SPACE,  SPACE};

/* "  SCORE P1-X P2-Y  " — dynamic score message (filled at runtime) */
char Msg_Score[30];

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_I2S3_Init(void);
static void MX_SPI1_Init(void);
static void MX_TIM7_Init(void);
void MX_USB_HOST_Process(void);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* USER CODE BEGIN 0 */

/*============================================================================
 * Read_ADC() — Read a single ADC1 channel with multi-sample averaging
 *
 * @param channel  ADC channel number (0-3 for PA0-PA3)
 * @return         12-bit ADC conversion result (0 - 4095), averaged
 *
 * Takes ADC_SAMPLES (8) consecutive readings and returns the average.
 * This eliminates electrical noise from the potentiometers that would
 * otherwise cause the cursor to jitter between positions.
 *===========================================================================*/
#define ADC_SAMPLES 8

uint16_t Read_ADC(uint8_t channel) {
  uint32_t sum = 0;
  uint8_t i;

  for (i = 0; i < ADC_SAMPLES; i++) {
    /* Select channel in regular sequence register 3 */
    ADC1->SQR3 = channel;

    /* Start conversion by setting SWSTART bit (bit 30) in CR2 */
    ADC1->CR2 |= (1 << 30);

    /* Wait for end-of-conversion flag (bit 1 in SR) */
    while (!(ADC1->SR & (1 << 1))) {
      /* Busy-wait until conversion finishes */
    }

    /* Accumulate the 12-bit result */
    sum += (ADC1->DR & 0x0FFF);
  }

  /* Return the averaged result */
  return (uint16_t)(sum / ADC_SAMPLES);
}

/*============================================================================
 * Read_Switches() — Read the switch state from Port C lower nibble
 *
 * @return  Bitmask of pressed switches (active high after inversion
 *          if switches are active-low, adjust as needed for your board)
 *
 * Port C pins are configured as inputs. The physical switch wiring
 * determines whether they are active-high or active-low. This function
 * returns the raw IDR value for the lower nibble so the caller can
 * mask individual switch bits.
 *===========================================================================*/
uint16_t Read_Switches(void) {
  return (uint16_t)(GPIOC->IDR & 0x0C00); /* Read PC10 and PC11 */
}

/*============================================================================
 * Debounce_Delay() — Simple software debounce wait
 *
 * Uses HAL_Delay to wait DEBOUNCE_MS milliseconds. This prevents
 * registering multiple presses from a single button actuation due
 * to mechanical contact bounce.
 *===========================================================================*/
void Debounce_Delay(void) { HAL_Delay(DEBOUNCE_MS); }

/*============================================================================
 * Set_Marquee_Message() — Configure the scrolling marquee on 7-seg displays
 *
 * @param msg    Pointer to character array (using seg7.h CHAR_X defines)
 * @param len    Total length of the message array
 * @param speed  Scroll speed in milliseconds per step
 *
 * Sets up the global pointers and counters that the SysTick ISR uses
 * to animate the scrolling text across the 8-digit display.
 *===========================================================================*/
void Set_Marquee_Message(char *msg, uint8_t len, int speed) {
  Animate_On = 0;        /* Stop any current animation first     */
  Message_Pointer = msg; /* Point to start of new message        */
  Save_Pointer = msg;    /* Save the start for wrap-around       */
  Message_Length = len;  /* Total characters in the message      */
  Delay_msec = speed;    /* Milliseconds between scroll steps    */
  Delay_counter = 0;     /* Reset the delay counter              */
  Animate_On = 1;        /* Enable animation in SysTick ISR      */
}

/*============================================================================
 * Stop_Marquee() — Halt the scrolling marquee display
 *===========================================================================*/
void Stop_Marquee(void) { Animate_On = 0; }

/*============================================================================
 * Display_Clear() — Blank all 8 digits of the 7-segment display
 *
 * Writes SPACE character to each digit position, effectively turning
 * off all segments. Also resets the display buffer.
 *===========================================================================*/
void Display_Clear(void) {
  uint8_t i;
  for (i = 0; i < 8; i++) {
    Seven_Segment_Digit(i, SPACE, 0);
    Display_Buffer[i] = SPACE;
    Brightness_Buffer[i] = BRIGHT_FULL;
  }
}

/*============================================================================
 * Display_Refresh() — Push the display buffer contents to the physical
 *                     7-segment displays.
 *
 * Called from the main loop when the display buffer has been updated.
 * The SysTick ISR handles PWM dimming by selectively enabling/disabling
 * segments based on the brightness buffer, so this function just latches
 * the base pattern.
 *===========================================================================*/
void Display_Refresh(void) {
  uint8_t i;
  for (i = 0; i < 8; i++) {
    Seven_Segment_Digit(i, Display_Buffer[i], 0);
  }
}

/*============================================================================
 * ADC_To_Grid_Hysteresis() — Map an ADC value to a grid position with
 *                            hysteresis to prevent jitter.
 *
 * @param adc_val       Current averaged ADC reading (0-4095)
 * @param grid_size     Number of grid positions (e.g. 8, 16, 2, 3)
 * @param current_pos   The current grid position (for hysteresis compare)
 * @return              New grid position (0 to grid_size-1)
 *
 * How it works:
 *   The ADC range (0-4095) is divided into grid_size equal zones.
 *   Each zone has a center and two boundaries. A position change only
 *   occurs when the ADC value moves past the MIDPOINT between the
 *   current position's center and the neighboring position's center.
 *   This creates a "sticky" zone around each position, so small
 *   pot movements or noise won't cause unwanted jumps.
 *
 *   Hysteresis margin = half a zone width = (ADC_MAX+1) / (grid_size * 2)
 *
 *   Example for grid_size=8 (512 ADC counts per zone):
 *     Position 0 center = 256,  stays until ADC > 512+128 = 640  (> halfway to
 * pos 1) Position 3 center = 1792, stays until ADC < 1536-128 or ADC > 2048+128
 *===========================================================================*/
uint8_t ADC_To_Grid_Hysteresis(uint16_t adc_val, uint8_t grid_size,
                               uint8_t current_pos) {
  uint16_t zone_width;     /* ADC counts per grid position              */
  uint16_t half_zone;      /* Hysteresis margin (half a zone width)     */
  uint16_t current_center; /* ADC value at center of current position   */
  int16_t lower_thresh;    /* ADC threshold to move down one position   */
  int16_t upper_thresh;    /* ADC threshold to move up one position     */

  if (grid_size == 0)
    return 0;
  if (grid_size == 1)
    return 0;

  /* Handle initialization or forced snap (out of bounds) smoothly */
  if (current_pos >= grid_size) {
    uint8_t new_pos = (uint8_t)((uint32_t)adc_val * grid_size / (ADC_MAX + 1));
    if (new_pos >= grid_size)
      new_pos = grid_size - 1;
    return new_pos;
  }

  zone_width = (ADC_MAX + 1) / grid_size;
  half_zone = zone_width / 2;

  /* Center of the current position's zone */
  current_center = current_pos * zone_width + half_zone;

  /* Thresholds: must move past halfway to the neighboring position */
  lower_thresh = (int16_t)current_center - (int16_t)zone_width;
  upper_thresh = (int16_t)current_center + (int16_t)zone_width;

  /* Check if we've moved far enough to change position */
  if ((int16_t)adc_val < lower_thresh && current_pos > 0) {
    /* Moved below threshold — calculate new position directly */
    uint8_t new_pos = (uint8_t)((uint32_t)adc_val * grid_size / (ADC_MAX + 1));
    if (new_pos >= grid_size)
      new_pos = grid_size - 1;
    return new_pos;
  } else if ((int16_t)adc_val > upper_thresh && current_pos < grid_size - 1) {
    /* Moved above threshold — calculate new position directly */
    uint8_t new_pos = (uint8_t)((uint32_t)adc_val * grid_size / (ADC_MAX + 1));
    if (new_pos >= grid_size)
      new_pos = grid_size - 1;
    return new_pos;
  }

  /* Within hysteresis zone — stay at current position */
  return current_pos;
}

/*============================================================================
 * Cursor_Update_From_ADC() — Read potentiometers and update cursor position
 *
 * Reads two ADC channels:
 *   - ADC_CH_CURSOR_X: horizontal position
 *   - ADC_CH_CURSOR_Y: vertical position (row within the current map)
 *
 * Uses multi-sample averaged ADC reads plus hysteresis-based grid mapping
 * for smooth, precise cursor control. The cursor will "stick" to its
 * current position until you clearly turn the pot toward the next one.
 *
 * Vertical map: x ranges 0-15 (16 columns), y = 0 or 1 (2 rows)
 * Horizontal map: x ranges 0-7 (8 columns), y = 0, 1, or 2 (3 rows)
 *===========================================================================*/
void Cursor_Update_From_ADC(void) {
  uint16_t adc_x, adc_y;

  adc_x = Read_ADC(ADC_CH_CURSOR_X);
  adc_y = Read_ADC(ADC_CH_CURSOR_Y);

  if (Cursor.onVertical) {
    /* Vertical map: 16 columns (x: 0-15), 2 rows (y: 0-1) */
    Cursor.x = ADC_To_Grid_Hysteresis(adc_x, 16, Cursor.x);
    Cursor.y = ADC_To_Grid_Hysteresis(adc_y, 2, Cursor.y);
  } else {
    /* Horizontal map: 8 columns (x: 0-7), 3 rows (y: 0-2) */
    Cursor.x = ADC_To_Grid_Hysteresis(adc_x, 8, Cursor.x);
    Cursor.y = ADC_To_Grid_Hysteresis(adc_y, 3, Cursor.y);
  }
}

/*============================================================================
 * Build_Score_Message() — Construct a dynamic score marquee string
 *
 * Fills Msg_Score with "  P1-X P2-Y  " where X and Y are the hit counts.
 * Uses seg7.h character defines for display on the 7-segment marquee.
 *===========================================================================*/
void Build_Score_Message(void) {
  uint8_t idx = 0;
  uint8_t i;

  /* Leading spaces for smooth scroll-in */
  for (i = 0; i < 8; i++)
    Msg_Score[idx++] = SPACE;

  /* "P1-" */
  Msg_Score[idx++] = CHAR_P;
  Msg_Score[idx++] = CHAR_1;
  Msg_Score[idx++] = DASH;
  Msg_Score[idx++] = P1_Hits; /* Numeric 0-7 maps to CHAR_0..CHAR_7 */

  Msg_Score[idx++] = SPACE;

  /* "P2-" */
  Msg_Score[idx++] = CHAR_P;
  Msg_Score[idx++] = CHAR_2;
  Msg_Score[idx++] = DASH;
  Msg_Score[idx++] = P2_Hits; /* Numeric 0-7 maps to CHAR_0..CHAR_7 */

  /* Trailing spaces for smooth scroll-out */
  for (i = 0; i < 8; i++)
    Msg_Score[idx++] = SPACE;
}

/*============================================================================
 * Map_To_Display() — Render a player's map onto the 8-digit 7-seg display
 *
 * @param boats     Pointer to the boat map being displayed
 * @param shots     Pointer to the shot map (opponent's shots on this map)
 * @param showBoats If non-zero, show the actual boat positions (for placement)
 *                  If zero, show only shots and hits (opponent's view)
 * @param cursor    Pointer to cursor (NULL if cursor should not be shown)
 *
 * Segment mapping for each 7-segment digit:
 *
 *    --a--          ← horizontal top row
 *   |     |
 *   f     b         ← vertical top row (left pair = f, right pair = b)
 *   |     |
 *    --g--          ← horizontal middle row
 *   |     |
 *   e     c         ← vertical bottom row (left pair = e, right pair = c)
 *   |     |
 *    --d--          ← horizontal bottom row
 *
 * 7-seg bit mapping (standard):
 *   bit 0 = a (top horizontal)
 *   bit 1 = b (top-right vertical)
 *   bit 2 = c (bottom-right vertical)
 *   bit 3 = d (bottom horizontal)
 *   bit 4 = e (bottom-left vertical)
 *   bit 5 = f (top-left vertical)
 *   bit 6 = g (middle horizontal)
 *   bit 7 = dp (decimal point, unused for map)
 *
 * Each digit (0-7) represents one column of the battlefield.
 * The vertical map has 2 rows x 16 columns = 32 segments:
 *   Each digit shows 2 of the 16 vertical columns:
 *     Left vertical pair uses segments f (top) and e (bottom)
 *     Right vertical pair uses segments b (top) and c (bottom)
 * The horizontal map has 3 rows x 8 columns = 24 segments:
 *   Each digit maps to its column, rows map to a (top), g (middle), d (bottom)
 *
 * Brightness control:
 *   - Boat + hit shot = BRIGHT_FULL (100%)
 *   - Miss shot = BRIGHT_HALF (50%)
 *   - Boat (during placement) = BRIGHT_FULL
 *   - Empty = BRIGHT_OFF (off/not lit)
 *
 * Since we are using common-anode displays (active-low segments), a '0' turns
 * a segment ON. We invert our logic when writing to the display.
 *===========================================================================*/
void Map_To_Display(PlayerMaps_t *boats, PlayerMaps_t *shots, uint8_t showBoats,
                    Cursor_t *cursor) {
  uint8_t digit;
  uint8_t segPattern; /* Accumulated segment pattern for this digit */
  uint8_t brightMin;  /* Track minimum brightness needed for PWM */

  for (digit = 0; digit < 8; digit++) {
    segPattern = 0;
    brightMin = BRIGHT_FULL;

    /*------------------------------------------------------------------
     * Horizontal segments: top (a), middle (g), bottom (d)
     * Array indexing: horizMap[row][digit], horizShots[row][digit]
     *-----------------------------------------------------------------*/

    /* Top horizontal row — segment 'a' (bit 0) */
    {
      uint8_t hasBoat = boats->horizMap[0][digit];
      uint8_t hasShot = shots->horizShots[0][digit];

      if (showBoats && hasBoat) {
        segPattern |= (1 << 0); /* Turn on segment 'a' */
      }
      if (hasShot && hasBoat) {
        segPattern |= (1 << 0); /* Hit: full brightness */
      } else if (hasShot && !hasBoat) {
        segPattern |= (1 << 0); /* Miss: will be dimmed via PWM */
        if (brightMin > BRIGHT_HALF)
          brightMin = BRIGHT_HALF;
      }
    }

    /* Middle horizontal row — segment 'g' (bit 6) */
    {
      uint8_t hasBoat = boats->horizMap[1][digit];
      uint8_t hasShot = shots->horizShots[1][digit];

      if (showBoats && hasBoat) {
        segPattern |= (1 << 6);
      }
      if (hasShot && hasBoat) {
        segPattern |= (1 << 6);
      } else if (hasShot && !hasBoat) {
        segPattern |= (1 << 6);
        if (brightMin > BRIGHT_HALF)
          brightMin = BRIGHT_HALF;
      }
    }

    /* Bottom horizontal row — segment 'd' (bit 3) */
    {
      uint8_t hasBoat = boats->horizMap[2][digit];
      uint8_t hasShot = shots->horizShots[2][digit];

      if (showBoats && hasBoat) {
        segPattern |= (1 << 3);
      }
      if (hasShot && hasBoat) {
        segPattern |= (1 << 3);
      } else if (hasShot && !hasBoat) {
        segPattern |= (1 << 3);
        if (brightMin > BRIGHT_HALF)
          brightMin = BRIGHT_HALF;
      }
    }

    /*------------------------------------------------------------------
     * Vertical segments: left pair (f=top, e=bottom),
     *                    right pair (b=top, c=bottom)
     * Each digit shows 2 of the 16 vertical columns:
     *   Left vertical column index  = digit * 2
     *   Right vertical column index = digit * 2 + 1
     * Array indexing: vertMap[row][col], vertShots[row][col]
     *-----------------------------------------------------------------*/

    /* Left-top vertical — segment 'f' (bit 5) */
    {
      uint8_t col = digit * 2;
      uint8_t hasBoat = boats->vertMap[0][col];
      uint8_t hasShot = shots->vertShots[0][col];

      if (showBoats && hasBoat) {
        segPattern |= (1 << 5);
      }
      if (hasShot && hasBoat) {
        segPattern |= (1 << 5);
      } else if (hasShot && !hasBoat) {
        segPattern |= (1 << 5);
        if (brightMin > BRIGHT_HALF)
          brightMin = BRIGHT_HALF;
      }
    }

    /* Right-top vertical — segment 'b' (bit 1) */
    {
      uint8_t col = digit * 2 + 1;
      uint8_t hasBoat = boats->vertMap[0][col];
      uint8_t hasShot = shots->vertShots[0][col];

      if (showBoats && hasBoat) {
        segPattern |= (1 << 1);
      }
      if (hasShot && hasBoat) {
        segPattern |= (1 << 1);
      } else if (hasShot && !hasBoat) {
        segPattern |= (1 << 1);
        if (brightMin > BRIGHT_HALF)
          brightMin = BRIGHT_HALF;
      }
    }

    /* Left-bottom vertical — segment 'e' (bit 4) */
    {
      uint8_t col = digit * 2;
      uint8_t hasBoat = boats->vertMap[1][col];
      uint8_t hasShot = shots->vertShots[1][col];

      if (showBoats && hasBoat) {
        segPattern |= (1 << 4);
      }
      if (hasShot && hasBoat) {
        segPattern |= (1 << 4);
      } else if (hasShot && !hasBoat) {
        segPattern |= (1 << 4);
        if (brightMin > BRIGHT_HALF)
          brightMin = BRIGHT_HALF;
      }
    }

    /* Right-bottom vertical — segment 'c' (bit 2) */
    {
      uint8_t col = digit * 2 + 1;
      uint8_t hasBoat = boats->vertMap[1][col];
      uint8_t hasShot = shots->vertShots[1][col];

      if (showBoats && hasBoat) {
        segPattern |= (1 << 2);
      }
      if (hasShot && hasBoat) {
        segPattern |= (1 << 2);
      } else if (hasShot && !hasBoat) {
        segPattern |= (1 << 2);
        if (brightMin > BRIGHT_HALF)
          brightMin = BRIGHT_HALF;
      }
    }

    /*------------------------------------------------------------------
     * Cursor overlay: blink the segment at the cursor position
     *-----------------------------------------------------------------*/
    if (cursor != 0 && Blink_State) {
      uint8_t cursorOnThisDigit = 0;
      uint8_t cursorSegBit = 0;

      if (cursor->onVertical) {
        /* Vertical cursor: check if this digit contains the cursor column */
        uint8_t cursorCol = cursor->x;                 /* 0..15 */
        uint8_t digitForCol = cursorCol / 2;           /* Which digit */
        uint8_t isLeft = (cursorCol % 2 == 0) ? 1 : 0; /* Left or right vert */

        if (digitForCol == digit) {
          cursorOnThisDigit = 1;
          if (cursor->y == 0) {
            /* Top row vertical */
            cursorSegBit = isLeft ? 5 : 1; /* f or b */
          } else {
            /* Bottom row vertical */
            cursorSegBit = isLeft ? 4 : 2; /* e or c */
          }
        }
      } else {
        /* Horizontal cursor: check if this digit matches cursor x */
        if (cursor->x == digit) {
          cursorOnThisDigit = 1;
          if (cursor->y == 0)
            cursorSegBit = 0; /* a (top)    */
          else if (cursor->y == 1)
            cursorSegBit = 6; /* g (middle) */
          else
            cursorSegBit = 3; /* d (bottom) */
        }
      }

      /* Toggle the cursor segment (XOR to make it blink) */
      if (cursorOnThisDigit) {
        segPattern ^= (1 << cursorSegBit);
      }
    }

    /* Store results into the display/brightness buffers. */
    Display_Buffer[digit] = segPattern;
    Brightness_Buffer[digit] = brightMin;
  }
}

/*============================================================================
 * Render_Display_Buffer() — Output the display buffer to the 7-segment
 *                           hardware via GPIOE.
 *
 * This is called from the SysTick handler for PWM-controlled refresh.
 * For each digit, if the current PWM ramp position is within the
 * brightness level, the segment pattern is shown; otherwise it is blanked.
 *
 * Since common-anode displays need inverted data (0 = on), we invert
 * the pattern before writing.
 *===========================================================================*/
void Render_Display_Buffer_Digit(uint8_t digit, uint8_t pwm_tick) {
  uint8_t pattern;

  if (pwm_tick < Brightness_Buffer[digit]) {
    /* Within the ON portion of the PWM cycle — show the pattern.
     * Invert for common-anode (0 = segment on). */
    pattern = ~Display_Buffer[digit] & 0x7F;
  } else {
    /* In the OFF portion — blank all segments */
    pattern = 0x7F; /* All segments OFF for common-anode */
  }

  /* Select just this digit (active low on bits 15:8), output pattern on bits
   * 7:0 */
  GPIOE->ODR = (0xFF00 | pattern) & ~(1 << (digit + 8));

  /* Brief hold time for the segment to be visible */
  /* (The ISR runs fast enough that sequential digit scanning works) */

  /* Deselect all digits to latch */
  GPIOE->ODR |= 0xFF00;
}

/*============================================================================
 * Play_SFX() — Trigger a sound effect
 *
 * @param sfx  Which sound effect to play
 *
 * Configures the Song array with appropriate notes and activates the
 * music engine in the TIM7 ISR.
 *===========================================================================*/
void Play_SFX(SoundEffect_t sfx) {
  Current_SFX = sfx;
  INDEX = 0;
  COUNT = 0;
  TONE = 0;

  switch (sfx) {
  case SFX_HIT:
    /* Explosion: descending chromatic burst */
    Song[0] = (Music){C5, _8th, 500, 50, 0};
    Song[1] = (Music){A4, _8th, 500, 50, 0};
    Song[2] = (Music){F3, _8th, 500, 50, 0};
    Song[3] = (Music){C3, quarter, 500, 100, 0};
    Song[4] = (Music){rest, quarter, 500, 0, 1}; /* End marker */
    Music_ON = 1;
    break;

  case SFX_MISS:
    /* Splash: short descending tone */
    Song[0] = (Music){G4, _16th, 500, 50, 0};
    Song[1] = (Music){E4, _16th, 500, 50, 0};
    Song[2] = (Music){C4, _8th, 500, 100, 0};
    Song[3] = (Music){rest, quarter, 500, 0, 1};
    Music_ON = 1;
    break;

  case SFX_PLACE:
    /* Confirmation beep: short ascending pair */
    Song[0] = (Music){C4, _16th, 400, 50, 0};
    Song[1] = (Music){E4, _16th, 400, 50, 0};
    Song[2] = (Music){rest, _8th, 400, 0, 1};
    Music_ON = 1;
    break;

  case SFX_ERROR:
    /* Error buzz: low repeated note */
    Song[0] = (Music){C2, _16th, 300, 30, 0};
    Song[1] = (Music){rest, _16th, 300, 0, 0};
    Song[2] = (Music){C2, _16th, 300, 30, 0};
    Song[3] = (Music){rest, _8th, 300, 0, 1};
    Music_ON = 1;
    break;

  default:
    Music_ON = 0;
    break;
  }

  Save_Note = Song[0].note;
}

/*============================================================================
 * Play_Victory_Song() — Play a nautical victory fanfare
 *
 * Loads a complete melody into the Song array and starts playback.
 * This is "Anchors Aweigh" simplified for the tone generator:
 *   C-E-G ascending major chord arpeggio, then a triumphant march.
 *===========================================================================*/
void Play_Victory_Song(void) {
  INDEX = 0;
  COUNT = 0;
  TONE = 0;

  /* Triumphant nautical fanfare */
  Song[0] = (Music){C4, _8th, 800, 50, 0};
  Song[1] = (Music){E4, _8th, 800, 50, 0};
  Song[2] = (Music){G4, quarter, 800, 50, 0};
  Song[3] = (Music){C5, quarter, 800, 100, 0};
  Song[4] = (Music){rest, _8th, 800, 0, 0};
  Song[5] = (Music){G4, _8th, 800, 50, 0};
  Song[6] = (Music){C5, half, 800, 100, 0};
  Song[7] = (Music){rest, _8th, 800, 0, 0};
  Song[8] = (Music){E4, _8th, 800, 50, 0};
  Song[9] = (Music){G4, _8th, 800, 50, 0};
  Song[10] = (Music){C5, _8th, 800, 50, 0};
  Song[11] = (Music){E5, quarter, 800, 100, 0};
  Song[12] = (Music){D5, _8th, 800, 50, 0};
  Song[13] = (Music){C5, _8th, 800, 50, 0};
  Song[14] = (Music){G4, quarter, 800, 100, 0};
  Song[15] = (Music){E4, _8th, 800, 50, 0};
  Song[16] = (Music){C4, half, 800, 100, 0};
  Song[17] = (Music){rest, half, 800, 0, 0};
  /* Repeat the hook once more */
  Song[18] = (Music){C4, _8th, 800, 50, 0};
  Song[19] = (Music){E4, _8th, 800, 50, 0};
  Song[20] = (Music){G4, quarter, 800, 50, 0};
  Song[21] = (Music){C5, half, 800, 100, 0};
  Song[22] = (Music){G4, _8th, 800, 50, 0};
  Song[23] = (Music){E5, quarter, 800, 100, 0};
  Song[24] = (Music){C5, whole, 800, 200, 0};
  Song[25] = (Music){rest, whole, 800, 0, 1}; /* End marker */

  Save_Note = Song[0].note;
  Music_ON = 1;
}

/*============================================================================
 * Validate_Placement() — Check if a boat placement is valid
 *
 * @param maps       Pointer to the player's current maps
 * @param cursor     Current cursor position
 * @param isDouble   0 = single-segment boat, 1 = double-segment boat
 * @param orient     For double boats: 0=right, 1=down, 2=left, 3=up
 * @return           1 if valid, 0 if invalid (overlap or out of bounds)
 *
 * Single boats occupy exactly one segment.
 * Double boats occupy two adjacent segments that stay together as a unit.
 *
 * Checks:
 *   1. No overlap with already-placed boats
 *   2. For doubles, the adjacent segment must be in bounds
 *===========================================================================*/
uint8_t Validate_Placement(PlayerMaps_t *maps, Cursor_t *c, uint8_t isDouble,
                           uint8_t orient) {
  uint8_t maxCol, maxRow;
  int8_t x2, y2;

  /* Determine bounds based on which map layer */
  if (c->onVertical) {
    maxCol = 15;
    maxRow = 1;

    /* Check first position bounds */
    if (c->x > maxCol || c->y > maxRow)
      return 0;

    /* Check first position overlap */
    if (maps->vertMap[c->y][c->x])
      return 0;
  } else {
    maxCol = 7;
    maxRow = 2;

    /* Check first position bounds */
    if (c->x > maxCol || c->y > maxRow)
      return 0;

    if (maps->horizMap[c->y][c->x])
      return 0;
  }

  if (isDouble) {
    /* Lock second segment position based on hardware orientation */
    x2 = (int8_t)c->x;
    y2 = (int8_t)c->y;

    if (c->onVertical) {
      y2 = c->y + 1; /* Always extend down on vertical map */
    } else {
      x2 = c->x + 1; /* Always extend right on horizontal map */
    }

    /* Bounds check for second segment */
    if (x2 < 0 || x2 > maxCol || y2 < 0 || y2 > maxRow)
      return 0;

    /* Overlap check for second segment */
    if (c->onVertical) {
      if (maps->vertMap[y2][x2])
        return 0;
    } else {
      if (maps->horizMap[y2][x2])
        return 0;
    }
  }

  return 1; /* Placement is valid */
}

/*============================================================================
 * Apply_Placement() — Place a boat onto the map at the cursor position
 *
 * @param maps       Pointer to the player's maps
 * @param cursor     Current cursor position
 * @param isDouble   0 = single, 1 = double
 * @param orient     For doubles: 0=right, 1=down, 2=left, 3=up
 *
 * Sets the appropriate array element(s) to 1.
 *===========================================================================*/
void Apply_Placement(PlayerMaps_t *maps, Cursor_t *c, uint8_t isDouble,
                     uint8_t orient) {
  /* Place first segment */
  if (c->onVertical) {
    maps->vertMap[c->y][c->x] = 1;
  } else {
    maps->horizMap[c->y][c->x] = 1;
  }

  if (isDouble) {
    int8_t x2 = (int8_t)c->x;
    int8_t y2 = (int8_t)c->y;

    /* Lock second segment position based on hardware orientation */
    if (c->onVertical) {
      y2 = c->y + 1; /* Always extend down on vertical map */
      maps->vertMap[y2][x2] = 1;
    } else {
      x2 = c->x + 1; /* Always extend right on horizontal map */
      maps->horizMap[y2][x2] = 1;
    }
  }
}

/*============================================================================
 * Fire_Shot() — Process a shot from the current player
 *
 * @param targetMaps  Pointer to the opponent's maps (boats + shots)
 * @param cursor      Current cursor position (where to fire)
 * @return            2 if already shot there, 1 if hit, 0 if miss
 *
 * Records the shot in the appropriate shot array and checks for a hit.
 *===========================================================================*/
uint8_t Fire_Shot(PlayerMaps_t *targetMaps, Cursor_t *cursor) {
  uint8_t x = cursor->x;
  uint8_t y = cursor->y;

  if (cursor->onVertical) {
    /* Check if already shot here */
    if (targetMaps->vertShots[y][x])
      return 2;

    /* Record the shot */
    targetMaps->vertShots[y][x] = 1;

    /* Check for hit */
    if (targetMaps->vertMap[y][x])
      return 1; /* HIT! */
    else
      return 0; /* Miss */
  } else {
    if (targetMaps->horizShots[y][x])
      return 2;

    targetMaps->horizShots[y][x] = 1;

    if (targetMaps->horizMap[y][x])
      return 1;
    else
      return 0;
  }
}

/*============================================================================
 * Check_Win() — Check if all of a player's boats have been hit
 *
 * @param targetMaps  Pointer to the maps of the player being attacked
 * @return            1 if all 7 boat segments have been hit, 0 otherwise
 *
 * A boat segment is "sunk" when both the boat array and shot array
 * have a 1 at the same position.
 *===========================================================================*/
uint8_t Check_Win(PlayerMaps_t *targetMaps) {
  return (Count_Hits(targetMaps) >= TOTAL_BOAT_SEGMENTS) ? 1 : 0;
}

/*============================================================================
 * Count_Hits() — Count how many boat segments have been hit
 *
 * @param targetMaps  Pointer to the maps of the player being attacked
 * @return            Number of hit boat segments (0-7)
 *===========================================================================*/
uint8_t Count_Hits(PlayerMaps_t *targetMaps) {
  uint8_t totalHits = 0;
  uint8_t r, c;

  /* Count vertical segment hits */
  for (r = 0; r < 2; r++)
    for (c = 0; c < 16; c++)
      if (targetMaps->vertMap[r][c] && targetMaps->vertShots[r][c])
        totalHits++;

  /* Count horizontal segment hits */
  for (r = 0; r < 3; r++)
    for (c = 0; c < 8; c++)
      if (targetMaps->horizMap[r][c] && targetMaps->horizShots[r][c])
        totalHits++;

  return totalHits;
}

/*============================================================================
 * Wait_For_Button() — Block until a specific button is pressed and released
 *
 * @param pin  Bitmask of the button pin to wait for (e.g., SW_CONFIRM_PIN)
 *
 * Polls Port C until the specified pin goes high, then waits for release
 * with debouncing.
 *===========================================================================*/
void Wait_For_Button(uint16_t pin) {
  /* Wait for button press (active high) */
  while (!(Read_Switches() & pin)) {
    /* Keep updating cursor and display during wait */
  }
  Debounce_Delay();

  /* Wait for button release */
  while (Read_Switches() & pin) {
    /* Wait */
  }
  Debounce_Delay();
}

/*============================================================================
 * Wait_For_Any_Button() — Block until any button is pressed
 *
 * Used for transition screens where we just need acknowledgement.
 * Returns the button mask that was pressed.
 *===========================================================================*/
uint16_t Wait_For_Any_Button(void) {
  uint16_t sw;

  /* Wait until a switch is pressed */
  do {
    sw = Read_Switches();
  } while (sw == 0);

  Debounce_Delay();
  return sw;
}

/*============================================================================
 * Get_Second_Segment_Bit() — For a double boat, return the 7-seg bit of the
 *                            second segment on the given digit.
 *
 * @param c        Cursor position (first segment location)
 * @param orient   0=right, 1=down, 2=left, 3=up
 * @param digit    The display digit we are rendering (0-7)
 * @return         Bitmask with the second segment's bit set for this digit,
 *                 or 0 if the second segment is not on this digit.
 *
 * Used to render a live preview showing BOTH segments of a double boat
 * blinking on the display, so the player can see exactly what they're
 * about to place before confirming.
 *===========================================================================*/
uint8_t Get_Second_Segment_Bit(Cursor_t *c, uint8_t orient, uint8_t digit) {
  int8_t x2 = (int8_t)c->x;
  int8_t y2 = (int8_t)c->y;

  /* Lock double boat orientation to standard shapes:
   * Vertical boats always extend DOWN ("I" shape)
   * Horizontal boats always extend RIGHT ("--" shape) */
  if (c->onVertical) {
    y2 = c->y + 1;
    orient = 1; /* Match 'down' for validation */
  } else {
    x2 = c->x + 1;
    orient = 0; /* Match 'right' for validation */
  }

  /* Bounds check */
  if (c->onVertical) {
    if (x2 < 0 || x2 > 15 || y2 < 0 || y2 > 1)
      return 0;

    /* Check if the second segment lives on this digit */
    uint8_t digitForCol = (uint8_t)x2 / 2;
    if (digitForCol != digit)
      return 0;

    uint8_t isLeft = ((uint8_t)x2 % 2 == 0) ? 1 : 0;
    if (y2 == 0)
      return (1 << (isLeft ? 5 : 1)); /* f or b */
    else
      return (1 << (isLeft ? 4 : 2)); /* e or c */
  } else {
    if (x2 < 0 || x2 > 7 || y2 < 0 || y2 > 2)
      return 0;

    if ((uint8_t)x2 != digit)
      return 0;

    if (y2 == 0)
      return (1 << 0); /* a */
    else if (y2 == 1)
      return (1 << 6); /* g */
    else
      return (1 << 3); /* d */
  }
}

/*============================================================================
 * Build_Boat_Announcement() — Build a marquee message for the current boat
 *
 * @param boatNum   Boat number 1-5
 * @param isDouble  0 = single-segment, 1 = double-segment
 * @param buf       Output buffer (must be at least 26 chars)
 * @return          Length of the message
 *
 * Creates messages like "  BOAT 1 SGL  " or "  BOAT 4 DBL  "
 *===========================================================================*/
uint8_t Build_Boat_Announcement(uint8_t boatNum, uint8_t isDouble, char *buf) {
  uint8_t idx = 0;
  uint8_t i;

  /* Leading padding for scroll-in */
  for (i = 0; i < 8; i++)
    buf[idx++] = SPACE;

  /* "BOAT " */
  buf[idx++] = CHAR_B;
  buf[idx++] = CHAR_O;
  buf[idx++] = CHAR_A;
  buf[idx++] = CHAR_T;
  buf[idx++] = SPACE;

  /* Boat number (1-5) */
  buf[idx++] = boatNum; /* 1-5 maps directly to CHAR_1..CHAR_5 */

  buf[idx++] = SPACE;

  if (isDouble) {
    /* "DBL" */
    buf[idx++] = CHAR_D;
    buf[idx++] = CHAR_B;
    buf[idx++] = CHAR_L;
  } else {
    /* "SGL" */
    buf[idx++] = CHAR_S;
    buf[idx++] = CHAR_G;
    buf[idx++] = CHAR_L;
  }

  /* Trailing padding for scroll-out */
  for (i = 0; i < 8; i++)
    buf[idx++] = SPACE;

  return idx;
}

/*============================================================================
 * Place_Boats() — Interactive boat placement phase for one player
 *
 * @param maps  Pointer to the player's maps to place boats on
 *
 * Guides the player through placing 5 boats:
 *   3 single-segment boats, then 2 double-segment boats.
 *
 * TWO-BUTTON PLACEMENT FLOW:
 *
 *   The player moves the cursor with the potentiometers and uses:
 *     PC11 (Confirm) — place the boat at the current position
 *     PC10 (Orient)  — for single boats: toggle V/H map layer
 *                       for double boats: cycle through 8 combined states
 *                       (V-right, V-down, V-left, V-up, H-right, H-down,
 *                        H-left, H-up) so both map layer and orientation
 *                        are controlled from one button.
 *
 *   For double boats, BOTH segments blink together as a live preview.
 *   If the position is invalid (overlap or OOB), an error buzz plays.
 *===========================================================================*/
void Place_Boats(PlayerMaps_t *maps) {
  uint8_t boatsPlaced = 0;       /* Total boats placed so far (0-4)         */
  uint8_t isDouble = 0;          /* 0 = single, 1 = double                  */
  uint8_t orient = 0;            /* Double-boat orientation: 0-3            */
  uint16_t sw;                   /* Current switch state                     */
  uint8_t prevConfirm = 0;       /* Previous confirm button state            */
  uint8_t prevOrient = 0;        /* Previous orient button state             */
  char boatMsg[30];              /* Buffer for boat announcement message     */
  uint8_t boatMsgLen;            /* Length of current boat announcement      */
  uint8_t digit;                 /* Loop variable for display rendering      */
  uint8_t placed;                /* Flag: current boat successfully placed   */
  PlayerMaps_t emptyShots = {0}; /* No shots during placement               */

  Boat_State = BOAT_SINGLE_1;
  Cursor.onVertical = 1; /* Start on vertical map                    */
  Cursor.x = 255;        /* Force immediate snap to pot position     */
  Cursor.y = 255;

  Stop_Marquee(); /* Stop any scrolling message               */

  while (boatsPlaced < 5) {
    /* Determine if current boat is single or double */
    isDouble = (boatsPlaced >= 3) ? 1 : 0;

    /*--------------------------------------------------------------
     * Show boat announcement at the start of each new boat
     *-------------------------------------------------------------*/
    boatMsgLen = Build_Boat_Announcement(boatsPlaced + 1, isDouble, boatMsg);
    Set_Marquee_Message(boatMsg, boatMsgLen, 150);
    HAL_Delay(1800); /* Let the message scroll for a moment */
    Stop_Marquee();

    /* Reset orientation state for double boats */
    if (isDouble) {
      Cursor.onVertical = 1;
      Cursor.x = 255;
      Cursor.y = 255;
      orient = 0;
    }

    /*==============================================================
     * AIM LOOP — cursor moves freely, preview shown
     * PC11 = confirm placement directly
     * PC10 = toggle V/H
     *=============================================================*/
    placed = 0;

    while (!placed) {
      /* Read cursor position from potentiometers */
      Cursor_Update_From_ADC();

      /* Clamp bounds for double boats so both segments stay fully visible
       * onscreen */
      if (isDouble) {
        if (Cursor.onVertical && Cursor.y > 0)
          Cursor.y = 0; /* Vertical map: must be at row 0 to extend to row 1 */
        else if (!Cursor.onVertical && Cursor.x > 6)
          Cursor.x =
              6; /* Horizontal map: must be column <=6 to extend to col 7 */
      }

      /* Render the map with existing boats + blinking cursor */
      Map_To_Display(maps, &emptyShots, 1, &Cursor);

      /* For double boats: also blink the second segment as a preview */
      if (isDouble && Blink_State) {
        for (digit = 0; digit < 8; digit++) {
          uint8_t seg2 = Get_Second_Segment_Bit(&Cursor, orient, digit);
          if (seg2) {
            Display_Buffer[digit] ^= seg2; /* XOR to blink */
          }
        }
      }

      Display_Refresh();

      /* Read switches with edge detection */
      sw = Read_Switches();

      /* PC10: V-H toggle button (same for single and double boats) */
      if ((sw & SW_ORIENT_PIN) && !prevOrient) {
        Cursor.onVertical ^= 1;
        /* Reset cursor values to 255 to force snap to current ADC level for new
         * grid size */
        Cursor.x = 255;
        Cursor.y = 255;

        Play_SFX(SFX_PLACE); /* Quick beep for feedback */
        Debounce_Delay();
      }
      prevOrient = (sw & SW_ORIENT_PIN) ? 1 : 0;

      /* PC11: Confirm button — place the boat immediately */
      if ((sw & SW_CONFIRM_PIN) && !prevConfirm) {
        /* Validate and place in one step */
        if (Validate_Placement(maps, &Cursor, isDouble, orient)) {
          Apply_Placement(maps, &Cursor, isDouble, orient);
          Play_SFX(SFX_PLACE);
          boatsPlaced++;
          Boat_State = (BoatPlaceState_t)boatsPlaced;

          /* Flash all LEDs green briefly as confirmation */
          DIM_Enable = 1;
          GREEN_BRT = 10;
          RED_BRT = 0;
          BLUE_BRT = 0;
          HAL_Delay(400);
          DIM_Enable = 0;

          placed = 1; /* Exit the AIM loop */
          Debounce_Delay();

          /* Wait for button release before next boat */
          while (Read_Switches() & SW_CONFIRM_PIN) {
          }
          Debounce_Delay();
        } else {
          /* Invalid position — buzz and stay in AIM */
          Play_SFX(SFX_ERROR);
          Debounce_Delay();
        }
      }
      prevConfirm = (sw & SW_CONFIRM_PIN) ? 1 : 0;

      HAL_Delay(20); /* Control loop rate */
    }
  }

  /* All 5 boats placed — show confirmation message */
  Boat_State = BOAT_DONE;

  /* Flash all placed boats on the display for a final review */
  Map_To_Display(maps, &emptyShots, 1, 0);
  Display_Refresh();

  DIM_Enable = 1;
  GREEN_BRT = 10;
  RED_BRT = 0;
  BLUE_BRT = 5;
  HAL_Delay(1500);
  DIM_Enable = 0;
  BLUE_BRT = 5;
  HAL_Delay(1500);
  DIM_Enable = 0;
}

/*============================================================================
 * Game_Init() — Initialize all game state for a new game
 *
 * Clears all maps, resets hit counters, and sets the game state machine
 * to the welcome screen.
 *===========================================================================*/
void Game_Init(void) {
  /* Clear all player maps */
  memset(&P1_Maps, 0, sizeof(PlayerMaps_t));
  memset(&P2_Maps, 0, sizeof(PlayerMaps_t));

  /* Reset hit counters */
  P1_Hits = 0;
  P2_Hits = 0;

  /* Reset cursor */
  Cursor.x = 0;
  Cursor.y = 0;
  Cursor.onVertical = 0;
  Cursor.visible = 1;

  /* Set initial game state */
  Game_State = STATE_WELCOME;
  Current_Player = 1;
  Boat_State = BOAT_SINGLE_1;

  /* Clear the display */
  Display_Clear();
}

/*============================================================================
 * Game_Run() — Main game loop state machine
 *
 * Called repeatedly from the while(1) loop.  Each call processes the
 * current game state and transitions to the next when appropriate.
 *
 * State Flow:
 *   WELCOME -> P1_PLACE -> P1_TO_P2_TRANS -> P2_PLACE -> P2_TO_P1_TRANS
 *   -> P1_TURN -> P1_RESULT -> TURN_TRANS_TO_P2
 *   -> P2_TURN -> P2_RESULT -> TURN_TRANS_TO_P1
 *   -> (loop turns until GAME_OVER)
 *===========================================================================*/
void Game_Run(void) {
  uint8_t shotResult;
  uint16_t sw;

  switch (Game_State) {
  /*----------------------------------------------------------------------
   * STATE_WELCOME: Show scrolling "BATTLESHIP" title
   * Waits for SW_CONFIRM_PIN to be pressed to proceed.
   *---------------------------------------------------------------------*/
  case STATE_WELCOME:
    Set_Marquee_Message(Msg_Welcome, sizeof(Msg_Welcome), 200);

    /* Pulse the RGB LEDs for visual flair during title screen */
    DIM_Enable = 1;
    RED_BRT = 5;
    GREEN_BRT = 0;
    BLUE_BRT = 8;

    /* Wait for start button */
    Wait_For_Button(SW_CONFIRM_PIN);

    /* Stop marquee, transition to P1 placement */
    Stop_Marquee();
    DIM_Enable = 0;
    Display_Clear();
    Game_State = STATE_P1_PLACE;
    break;

  /*----------------------------------------------------------------------
   * STATE_P1_PLACE: Player 1 places their 5 boats
   *---------------------------------------------------------------------*/
  case STATE_P1_PLACE:
    /* Brief announcement */
    Set_Marquee_Message(Msg_P1Place, sizeof(Msg_P1Place), 150);
    HAL_Delay(2000);
    Stop_Marquee();

    /* Run the interactive placement routine */
    Place_Boats(&P1_Maps);

    /* Done — transition to hand-off screen */
    Display_Clear();
    Game_State = STATE_P1_TO_P2_TRANS;
    break;

  /*----------------------------------------------------------------------
   * STATE_P1_TO_P2_TRANS: Tell players to hand off the controller
   *---------------------------------------------------------------------*/
  case STATE_P1_TO_P2_TRANS:
    Display_Clear();
    Set_Marquee_Message(Msg_PassToP2, sizeof(Msg_PassToP2), 180);

    /* Wait for any button press to acknowledge */
    Wait_For_Any_Button();

    Stop_Marquee();
    Display_Clear();
    Game_State = STATE_P2_PLACE;
    break;

  /*----------------------------------------------------------------------
   * STATE_P2_PLACE: Player 2 places their 5 boats
   *---------------------------------------------------------------------*/
  case STATE_P2_PLACE:
    Set_Marquee_Message(Msg_P2Place, sizeof(Msg_P2Place), 150);
    HAL_Delay(2000);
    Stop_Marquee();

    Place_Boats(&P2_Maps);

    Display_Clear();
    Game_State = STATE_P2_TO_P1_TRANS;
    break;

  /*----------------------------------------------------------------------
   * STATE_P2_TO_P1_TRANS: Hand off back to Player 1 for the first shot
   *---------------------------------------------------------------------*/
  case STATE_P2_TO_P1_TRANS:
    Display_Clear();

    /* Scroll the current score */
    P1_Hits = Count_Hits(&P2_Maps);
    P2_Hits = Count_Hits(&P1_Maps);
    Build_Score_Message();
    Set_Marquee_Message(Msg_PassToP1, sizeof(Msg_PassToP1), 180);

    Wait_For_Any_Button();

    Stop_Marquee();
    Display_Clear();
    Current_Player = 1;
    Game_State = STATE_P1_TURN;
    break;

  /*----------------------------------------------------------------------
   * STATE_P1_TURN: Player 1 aims and fires at Player 2's map
   *
   * Display shows P2's map from P1's perspective:
   *   - Boats are HIDDEN (showBoats = 0)
   *   - Previous shots shown (hits at 100%, misses at 50%)
   *   - Blinking cursor for aiming
   *---------------------------------------------------------------------*/
  case STATE_P1_TURN: {
    uint8_t prevConfirm = 1; /* Init to 1 to ignore initial hold */
    uint8_t prevOrient = 0;

    /* Brief turn announcement */
    Set_Marquee_Message(Msg_P1Turn, sizeof(Msg_P1Turn), 150);
    HAL_Delay(1500);
    Stop_Marquee();

    /* Interactive firing loop */
    Cursor.onVertical = 0;
    Cursor.x = 255; /* Force ADC snap */
    Cursor.y = 255;

    while (1) {
      Cursor_Update_From_ADC();

      /* Show P2's map (opponent's boats hidden, P1's shots visible) */
      Map_To_Display(&P2_Maps, &P2_Maps, 0, &Cursor);
      Display_Refresh();

      sw = Read_Switches();

      /* Toggle map layer */
      if ((sw & SW_ORIENT_PIN) && !prevOrient) {
        Cursor.onVertical ^= 1;
        Cursor.x = 255;
        Cursor.y = 255;
        Debounce_Delay();
      }
      prevOrient = (sw & SW_ORIENT_PIN) ? 1 : 0;

      /* Fire button */
      if ((sw & SW_CONFIRM_PIN) && !prevConfirm) {
        shotResult = Fire_Shot(&P2_Maps, &Cursor);

        if (shotResult == 2) {
          /* Already shot here — play error, try again */
          Play_SFX(SFX_ERROR);
          Debounce_Delay();
        } else if (shotResult == 1) {
          /* HIT! */
          Play_SFX(SFX_HIT);
          P1_Hits = Count_Hits(&P2_Maps);

          /* Flash the LEDs green for a hit */
          DIM_Enable = 1;
          GREEN_BRT = 10;
          RED_BRT = 0;
          BLUE_BRT = 0;

          Game_State = STATE_P1_RESULT;
          Debounce_Delay();
          break;
        } else {
          /* MISS */
          Play_SFX(SFX_MISS);

          /* Flash LEDs blue for a miss */
          DIM_Enable = 1;
          GREEN_BRT = 0;
          RED_BRT = 0;
          BLUE_BRT = 10;

          Game_State = STATE_P1_RESULT;
          Debounce_Delay();
          break;
        }
      }
      prevConfirm = (sw & SW_CONFIRM_PIN) ? 1 : 0;

      HAL_Delay(20);
    }
    break;
  }

  /*----------------------------------------------------------------------
   * STATE_P1_RESULT: Show the result of Player 1's shot
   *---------------------------------------------------------------------*/
  case STATE_P1_RESULT: {
    uint8_t lastShotWasHit;

    /* Determine what happened on the last shot */
    lastShotWasHit = (GREEN_BRT > 0) ? 1 : 0;

    if (lastShotWasHit) {
      Set_Marquee_Message(Msg_Hit, sizeof(Msg_Hit), 100);
    } else {
      Set_Marquee_Message(Msg_Miss, sizeof(Msg_Miss), 100);
    }

    HAL_Delay(RESULT_DISPLAY_MS);
    Stop_Marquee();
    DIM_Enable = 0;

    /* Check for win condition */
    if (Check_Win(&P2_Maps)) {
      Game_State = STATE_GAME_OVER;
    } else {
      Game_State = STATE_TURN_TRANS_TO_P2;
    }
    break;
  }

  /*----------------------------------------------------------------------
   * STATE_TURN_TRANS_TO_P2: Transition between P1 and P2 turns
   * Shows score and prompts controller hand-off.
   *---------------------------------------------------------------------*/
  case STATE_TURN_TRANS_TO_P2:
    Display_Clear();

    P1_Hits = Count_Hits(&P2_Maps);
    P2_Hits = Count_Hits(&P1_Maps);
    Build_Score_Message();
    Set_Marquee_Message(Msg_Score, sizeof(Msg_Score), 180);
    HAL_Delay(TRANSITION_MS);
    Stop_Marquee();

    /* Show "PASS TO P2" */
    Set_Marquee_Message(Msg_PassToP2, sizeof(Msg_PassToP2), 180);
    Wait_For_Any_Button();

    Stop_Marquee();
    Display_Clear();
    Current_Player = 2;
    Game_State = STATE_P2_TURN;
    break;

  /*----------------------------------------------------------------------
   * STATE_P2_TURN: Player 2 aims and fires at Player 1's map
   * Mirror logic of STATE_P1_TURN but targeting P1's maps.
   *---------------------------------------------------------------------*/
  case STATE_P2_TURN: {
    uint8_t prevConfirm = 1;
    uint8_t prevOrient = 0;

    Set_Marquee_Message(Msg_P2Turn, sizeof(Msg_P2Turn), 150);
    HAL_Delay(1500);
    Stop_Marquee();

    Cursor.onVertical = 0;
    Cursor.x = 255;
    Cursor.y = 255;

    while (1) {
      Cursor_Update_From_ADC();

      /* Show P1's map (P1's boats hidden, P2's shots visible) */
      Map_To_Display(&P1_Maps, &P1_Maps, 0, &Cursor);
      Display_Refresh();

      sw = Read_Switches();

      if ((sw & SW_ORIENT_PIN) && !prevOrient) {
        Cursor.onVertical ^= 1;
        Cursor.x = 255;
        Cursor.y = 255;
        Debounce_Delay();
      }
      prevOrient = (sw & SW_ORIENT_PIN) ? 1 : 0;

      if ((sw & SW_CONFIRM_PIN) && !prevConfirm) {
        shotResult = Fire_Shot(&P1_Maps, &Cursor);

        if (shotResult == 2) {
          Play_SFX(SFX_ERROR);
          Debounce_Delay();
        } else if (shotResult == 1) {
          /* HIT */
          Play_SFX(SFX_HIT);
          P2_Hits = Count_Hits(&P1_Maps);

          DIM_Enable = 1;
          GREEN_BRT = 10;
          RED_BRT = 0;
          BLUE_BRT = 0;

          Game_State = STATE_P2_RESULT;
          Debounce_Delay();
          break;
        } else {
          /* MISS */
          Play_SFX(SFX_MISS);

          DIM_Enable = 1;
          GREEN_BRT = 0;
          RED_BRT = 0;
          BLUE_BRT = 10;

          Game_State = STATE_P2_RESULT;
          Debounce_Delay();
          break;
        }
      }
      prevConfirm = (sw & SW_CONFIRM_PIN) ? 1 : 0;

      HAL_Delay(20);
    }
    break;
  }

  /*----------------------------------------------------------------------
   * STATE_P2_RESULT: Show the result of Player 2's shot
   *---------------------------------------------------------------------*/
  case STATE_P2_RESULT: {
    uint8_t lastShotWasHit;

    lastShotWasHit = (GREEN_BRT > 0) ? 1 : 0;

    if (lastShotWasHit) {
      Set_Marquee_Message(Msg_Hit, sizeof(Msg_Hit), 100);
    } else {
      Set_Marquee_Message(Msg_Miss, sizeof(Msg_Miss), 100);
    }

    HAL_Delay(RESULT_DISPLAY_MS);
    Stop_Marquee();
    DIM_Enable = 0;

    /* Check for win condition */
    if (Check_Win(&P1_Maps)) {
      Game_State = STATE_GAME_OVER;
    } else {
      Game_State = STATE_TURN_TRANS_TO_P1;
    }
    break;
  }

  /*----------------------------------------------------------------------
   * STATE_TURN_TRANS_TO_P1: Transition between P2 and P1 turns
   *---------------------------------------------------------------------*/
  case STATE_TURN_TRANS_TO_P1:
    Display_Clear();

    P1_Hits = Count_Hits(&P2_Maps);
    P2_Hits = Count_Hits(&P1_Maps);
    Build_Score_Message();
    Set_Marquee_Message(Msg_Score, sizeof(Msg_Score), 180);
    HAL_Delay(TRANSITION_MS);
    Stop_Marquee();

    Set_Marquee_Message(Msg_PassToP1, sizeof(Msg_PassToP1), 180);
    Wait_For_Any_Button();

    Stop_Marquee();
    Display_Clear();
    Current_Player = 1;
    Game_State = STATE_P1_TURN;
    break;

  /*----------------------------------------------------------------------
   * STATE_GAME_OVER: Declare the winner and play victory music
   *---------------------------------------------------------------------*/
  case STATE_GAME_OVER:
    Display_Clear();

    /* Light up all LEDs in victory colors */
    DIM_Enable = 1;
    RED_BRT = 3;
    GREEN_BRT = 10;
    BLUE_BRT = 3;

    /* Determine winner and display message */
    if (Check_Win(&P2_Maps)) {
      /* Player 1 wins (they sank all of P2's boats) */
      Set_Marquee_Message(Msg_P1Wins, sizeof(Msg_P1Wins), 200);
    } else {
      /* Player 2 wins */
      Set_Marquee_Message(Msg_P2Wins, sizeof(Msg_P2Wins), 200);
    }

    /* Play the nautical victory song */
    Play_Victory_Song();

    /* Hold the victory screen for a good while */
    HAL_Delay(10000);

    /* Stop everything and reset for a new game */
    Stop_Marquee();
    Music_ON = 0;
    DIM_Enable = 0;
    Display_Clear();

    /* Reinitialize for a new game */
    Game_Init();
    break;

  default:
    /* Should never reach here — reset to welcome */
    Game_State = STATE_WELCOME;
    break;
  }
}

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick.
   */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  // MX_I2C1_Init();
  // MX_I2S3_Init();
  // MX_SPI1_Init();
  // MX_USB_HOST_Init();
  MX_TIM7_Init();
  /* USER CODE BEGIN 2 */

  /*** Configure GPIOs ***/
  GPIOD->MODER = 0x55555555;  /* Set all Port D pins to outputs              */
  GPIOA->MODER |= 0x000000FF; /* Port A: make A0 to A3 analog inputs         */
  GPIOE->MODER |= 0x55555555; /* Port E: make E0 to E15 outputs              */
  GPIOC->MODER |= 0x0;        /* Port C: all inputs (switches)               */
  GPIOE->ODR = 0xFFFF;        /* Set all Port E pins high (7-seg blanked)    */

  /*** Configure ADC1 ***/
  RCC->APB2ENR |= 1 << 8; /* Turn on ADC1 clock                          */
  ADC1->SMPR2 |= 1;       /* 15 clock cycles per sample                  */
  ADC1->CR2 |= 1;         /* Turn on ADC1                                */

  /*** Turn on CRC Clock in AHB1ENR for CRC hardware ***/
  RCC->AHB1ENR |= RCC_AHB1ENR_CRCEN;

  /*** Configure Timer 7 for sound generation ***/
  TIM7->PSC = 199; /* 250KHz timer clock (50MHz / 200)            */
  TIM7->ARR = 1;   /* Count to 1 → 125KHz interrupt rate          */
  TIM7->DIER |= 1; /* Enable timer 7 interrupt                    */
  TIM7->CR1 |= 1;  /* Enable timer counting                       */

  /*** Initialize the game ***/
  Game_Init();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  while (1) {
    /*======================================================================
     * Main game loop: run the state machine on every iteration.
     * Each call to Game_Run() processes the current state and may
     * transition to the next state. The loop runs continuously,
     * with internal blocking (HAL_Delay, button waits) handled
     * within each state as needed.
     *=====================================================================*/
    Game_Run();

    /* USER CODE BEGIN 3 */
  }
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
   */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
    Error_Handler();
  }
}

/**
 * @brief I2C1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_I2C1_Init(void) {

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */
}

/**
 * @brief I2S3 Initialization Function
 * @param None
 * @retval None
 */
static void MX_I2S3_Init(void) {

  /* USER CODE BEGIN I2S3_Init 0 */

  /* USER CODE END I2S3_Init 0 */

  /* USER CODE BEGIN I2S3_Init 1 */

  /* USER CODE END I2S3_Init 1 */
  hi2s3.Instance = SPI3;
  hi2s3.Init.Mode = I2S_MODE_MASTER_TX;
  hi2s3.Init.Standard = I2S_STANDARD_PHILIPS;
  hi2s3.Init.DataFormat = I2S_DATAFORMAT_16B;
  hi2s3.Init.MCLKOutput = I2S_MCLKOUTPUT_ENABLE;
  hi2s3.Init.AudioFreq = I2S_AUDIOFREQ_96K;
  hi2s3.Init.CPOL = I2S_CPOL_LOW;
  hi2s3.Init.ClockSource = I2S_CLOCK_PLL;
  hi2s3.Init.FullDuplexMode = I2S_FULLDUPLEXMODE_DISABLE;
  if (HAL_I2S_Init(&hi2s3) != HAL_OK) {
    Error_Handler();
  }
  /* USER CODE BEGIN I2S3_Init 2 */

  /* USER CODE END I2S3_Init 2 */
}

/**
 * @brief SPI1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_SPI1_Init(void) {

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
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK) {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */
}

/**
 * @brief TIM7 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM7_Init(void) {

  /* USER CODE BEGIN TIM7_Init 0 */

  /* USER CODE END TIM7_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM7_Init 1 */

  /* USER CODE END TIM7_Init 1 */
  htim7.Instance = TIM7;
  htim7.Init.Prescaler = 0;
  htim7.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim7.Init.Period = 65535;
  htim7.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim7) != HAL_OK) {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim7, &sMasterConfig) != HAL_OK) {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM7_Init 2 */

  /* USER CODE END TIM7_Init 2 */
}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(CS_I2C_SPI_GPIO_Port, CS_I2C_SPI_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(OTG_FS_PowerSwitchOn_GPIO_Port, OTG_FS_PowerSwitchOn_Pin,
                    GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD,
                    LD4_Pin | LD3_Pin | LD5_Pin | LD6_Pin | Audio_RST_Pin,
                    GPIO_PIN_RESET);

  /*Configure GPIO pin : CS_I2C_SPI_Pin */
  GPIO_InitStruct.Pin = CS_I2C_SPI_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(CS_I2C_SPI_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : OTG_FS_PowerSwitchOn_Pin */
  GPIO_InitStruct.Pin = OTG_FS_PowerSwitchOn_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(OTG_FS_PowerSwitchOn_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PDM_OUT_Pin */
  GPIO_InitStruct.Pin = PDM_OUT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
  HAL_GPIO_Init(PDM_OUT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : BOOT1_Pin */
  GPIO_InitStruct.Pin = BOOT1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(BOOT1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : CLK_IN_Pin */
  GPIO_InitStruct.Pin = CLK_IN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
  HAL_GPIO_Init(CLK_IN_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LD4_Pin LD3_Pin LD5_Pin LD6_Pin
                           Audio_RST_Pin */
  GPIO_InitStruct.Pin = LD4_Pin | LD3_Pin | LD5_Pin | LD6_Pin | Audio_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : OTG_FS_OverCurrent_Pin */
  GPIO_InitStruct.Pin = OTG_FS_OverCurrent_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(OTG_FS_OverCurrent_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : MEMS_INT2_Pin */
  GPIO_InitStruct.Pin = MEMS_INT2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(MEMS_INT2_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
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
#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line) {
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line
     number, ex: printf("Wrong parameters value: file %s on line %d\r\n", file,
     line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
