/* USER CODE BEGIN Header */
/**
 * main.c: BATTLESHIP FINAL PROJECT for STM32F407
 * Two-player Battleship on 7-segment displays. PC11=Start/Fire, PC10=Orient/Toggle.
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

/* Display buffer: split into Full Brightness and Dim Brightness segments */
volatile uint8_t Display_Buffer_Full[8] = {0};
volatile uint8_t Display_Buffer_Dim[8] = {0};

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

/* Read ADC1 (PA0-PA3) with multi-sample averaging */
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

/* Read Port C lower nibble (switches) */
uint16_t Read_Switches(void) {
  return (uint16_t)(GPIOC->IDR & 0x0C00); /* Read PC10 and PC11 */
}

/* Software debounce delay */
void Debounce_Delay(void) { HAL_Delay(DEBOUNCE_MS); }

/* Configure scrolling marquee message */
void Set_Marquee_Message(char *msg, uint8_t len, int speed) {
  Animate_On = 0;        /* Stop any current animation first     */
  Message_Pointer = msg; /* Point to start of new message        */
  Save_Pointer = msg;    /* Save the start for wrap-around       */
  Message_Length = len;  /* Total characters in the message      */
  Delay_msec = speed;    /* Milliseconds between scroll steps    */
  Delay_counter = 0;     /* Reset the delay counter              */
  Animate_On = 1;        /* Enable animation in SysTick ISR      */
}

/* Halt scrolling marquee */
void Stop_Marquee(void) { Animate_On = 0; }

/* Clear 7-segment display digits and buffer */
void Display_Clear(void) {
  uint8_t i;
  for (i = 0; i < 8; i++) {
    Seven_Segment_Digit(i, SPACE, 0);
    Display_Buffer_Full[i] = SPACE;
    Display_Buffer_Dim[i] = 0;
  }
}

/* Refresh physical displays from buffer */
void Display_Refresh(void) {
  uint8_t i;
  for (i = 0; i < 8; i++) {
    Seven_Segment_Digit(i, Display_Buffer_Full[i] | Display_Buffer_Dim[i], 0);
  }
}

/* Map ADC value to grid position with hysteresis to prevent jitter */
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

/* Read pots and update cursor position with hysteresis */
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

/* Build dynamic score marquee string "  P1-X P2-Y  " */
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

/* Apply one segment's boat/shot state into full/dim pattern buffers */
#define APPLY_SEG(hasBoat, hasShot, bit, fp, dp) do { \
  if ((showBoats && (hasBoat)) || ((hasShot) && (hasBoat))) (fp) |= (1<<(bit)); \
  else if ((hasShot) && !(hasBoat)) (dp) |= (1<<(bit)); \
} while(0)

/* Render player map to 8-digit 7-seg display buffer */
void Map_To_Display(PlayerMaps_t *boats, PlayerMaps_t *shots, uint8_t showBoats,
                    Cursor_t *cursor) {
  uint8_t digit;
  for (digit = 0; digit < 8; digit++) {
    uint8_t fp = 0, dp = 0;
    uint8_t col0 = digit * 2, col1 = digit * 2 + 1;

    /* Horizontal segments: top(a=0), mid(g=6), bot(d=3) */
    APPLY_SEG(boats->horizMap[0][digit], shots->horizShots[0][digit], 0, fp, dp);
    APPLY_SEG(boats->horizMap[1][digit], shots->horizShots[1][digit], 6, fp, dp);
    APPLY_SEG(boats->horizMap[2][digit], shots->horizShots[2][digit], 3, fp, dp);
    /* Vertical segments: f=5, b=1, e=4, c=2 */
    APPLY_SEG(boats->vertMap[0][col0], shots->vertShots[0][col0], 5, fp, dp);
    APPLY_SEG(boats->vertMap[0][col1], shots->vertShots[0][col1], 1, fp, dp);
    APPLY_SEG(boats->vertMap[1][col0], shots->vertShots[1][col0], 4, fp, dp);
    APPLY_SEG(boats->vertMap[1][col1], shots->vertShots[1][col1], 2, fp, dp);

    /* Cursor blink */
    if (cursor && cursor->visible && Blink_State) {
      uint8_t onDigit = 0, segBit = 0;
      if (cursor->onVertical && cursor->x / 2 == digit) {
        uint8_t isLeft = (cursor->x % 2 == 0);
        segBit = (cursor->y == 0) ? (isLeft ? 5 : 1) : (isLeft ? 4 : 2);
        onDigit = 1;
      } else if (!cursor->onVertical && cursor->x == digit) {
        segBit = (cursor->y == 0) ? 0 : (cursor->y == 1) ? 6 : 3;
        onDigit = 1;
      }
      if (onDigit) fp ^= (1 << segBit);
    }

    Display_Buffer_Full[digit] = fp;
    Display_Buffer_Dim[digit] = dp;
  }
}

/* Output PWM-controlled display buffer to GPIOE */
void Render_Display_Buffer_Digit(uint8_t digit, uint8_t pwm_tick) {
  uint8_t raw = 0;
  if (pwm_tick < BRIGHT_FULL) raw |= Display_Buffer_Full[digit];
  if (pwm_tick < BRIGHT_HALF) raw |= Display_Buffer_Dim[digit];
  /* Invert for common-anode; bit7=1 forces dot pin HIGH (dot OFF) */
  uint8_t pattern = (~raw & 0x7F) | 0x80;
  GPIOE->ODR = (0xFF00 | pattern) & ~(1 << (digit + 8));
  GPIOE->ODR |= 0xFF00;
}

/* Trigger a sound effect via TIM7 ISR */
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
    /* Deep splash: very fast dissonant descent */
    Song[0] = (Music){F3, _8th, 300, 30, 0};
    Song[1] = (Music){E3, _8th, 300, 30, 0};
    Song[2] = (Music){C3, _8th, 300, 50, 0};
    Song[3] = (Music){rest, quarter, 300, 0, 1};
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

/* Play nautical victory fanfare */
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
 *===========================================================================*/
uint8_t Check_Win(PlayerMaps_t *targetMaps) {
  return (Count_Hits(targetMaps) >= TOTAL_BOAT_SEGMENTS) ? 1 : 0;
}

/*============================================================================
 * Count_Hits() — Count how many boat segments have been hit
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

/* Block until pin is pressed and released */
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

/* Block until any button is pressed */
uint16_t Wait_For_Any_Button(void) {
  uint16_t sw;

  /* Wait until a switch is pressed */
  do {
    sw = Read_Switches();
  } while (sw == 0);

  Debounce_Delay();
  return sw;
}

/* Return 7-seg bit for second segment of a double boat */
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

/* Build marquee message for current boat (e.g. "BOAT 1 SGL") */
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

/* Interactive boat placement phase for one player */
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
            Display_Buffer_Full[digit] ^= seg2; /* XOR to blink */
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

/* Initialize game state, maps, and cursor */
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

/* Main game loop state machine */
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
   * STATE_P1_TURN / STATE_P2_TURN: shared firing loop
   *---------------------------------------------------------------------*/
  case STATE_P1_TURN:
  case STATE_P2_TURN: {
    uint8_t prevConfirm = 1;
    uint8_t prevOrient = 0;
    uint8_t isP1 = (Game_State == STATE_P1_TURN);
    PlayerMaps_t *targetMaps = isP1 ? &P2_Maps : &P1_Maps;

    Set_Marquee_Message(isP1 ? Msg_P1Turn : Msg_P2Turn,
                        isP1 ? sizeof(Msg_P1Turn) : sizeof(Msg_P2Turn), 150);
    HAL_Delay(1500);
    Stop_Marquee();

    Cursor.onVertical = 0;
    Cursor.x = 255;
    Cursor.y = 255;

    while (1) {
      Cursor_Update_From_ADC();
      Map_To_Display(targetMaps, targetMaps, 0, &Cursor);
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
        shotResult = Fire_Shot(targetMaps, &Cursor);
        if (shotResult == 2) {
          Play_SFX(SFX_ERROR);
          Debounce_Delay();
        } else {
          if (shotResult == 1) {
            Play_SFX(SFX_HIT);
            if (isP1) P1_Hits = Count_Hits(&P2_Maps);
            else      P2_Hits = Count_Hits(&P1_Maps);
            DIM_Enable = 1; GREEN_BRT = 10; RED_BRT = 0; BLUE_BRT = 0;
          } else {
            Play_SFX(SFX_MISS);
            DIM_Enable = 1; GREEN_BRT = 0; RED_BRT = 0; BLUE_BRT = 10;
          }
          Game_State = isP1 ? STATE_P1_RESULT : STATE_P2_RESULT;
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
   * STATE_P1_RESULT / STATE_P2_RESULT: shared result display
   *---------------------------------------------------------------------*/
  case STATE_P1_RESULT:
  case STATE_P2_RESULT: {
    uint8_t isP1res = (Game_State == STATE_P1_RESULT);
    if (GREEN_BRT > 0)
      Set_Marquee_Message(Msg_Hit, sizeof(Msg_Hit), 100);
    else
      Set_Marquee_Message(Msg_Miss, sizeof(Msg_Miss), 100);
    HAL_Delay(RESULT_DISPLAY_MS);
    Stop_Marquee();
    DIM_Enable = 0;
    if (isP1res)
      Game_State = Check_Win(&P2_Maps) ? STATE_GAME_OVER : STATE_TURN_TRANS_TO_P2;
    else
      Game_State = Check_Win(&P1_Maps) ? STATE_GAME_OVER : STATE_TURN_TRANS_TO_P1;
    break;
  }

  /*----------------------------------------------------------------------
   * STATE_TURN_TRANS_TO_P2 / STATE_TURN_TRANS_TO_P1: shared transition
   *---------------------------------------------------------------------*/
  case STATE_TURN_TRANS_TO_P2:
  case STATE_TURN_TRANS_TO_P1: {
    uint8_t toP2 = (Game_State == STATE_TURN_TRANS_TO_P2);
    Display_Clear();
    P1_Hits = Count_Hits(&P2_Maps);
    P2_Hits = Count_Hits(&P1_Maps);
    Build_Score_Message();
    Set_Marquee_Message(Msg_Score, sizeof(Msg_Score), 180);
    HAL_Delay(TRANSITION_MS);
    Stop_Marquee();
    Set_Marquee_Message(toP2 ? Msg_PassToP2 : Msg_PassToP1,
                        toP2 ? sizeof(Msg_PassToP2) : sizeof(Msg_PassToP1), 180);
    Wait_For_Any_Button();
    Stop_Marquee();
    Display_Clear();
    if (toP2) { Current_Player = 2; Game_State = STATE_P2_TURN; }
    else       { Current_Player = 1; Game_State = STATE_P1_TURN; }
    break;
  }

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

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick.
   */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

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


  /* Infinite loop */
  while (1) {
    /* Main game loop: run state machine */
    Game_Run();

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

}

/**
 * @brief I2S3 Initialization Function
 * @param None
 * @retval None
 */
static void MX_I2S3_Init(void) {

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

}

/**
 * @brief SPI1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_SPI1_Init(void) {

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
}

/**
 * @brief TIM7 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM7_Init(void) {
  TIM_MasterConfigTypeDef sMasterConfig = {0};

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

}

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
