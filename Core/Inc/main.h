/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h  BATTLESHIP FINAL PROJECT
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/*============================================================================
 * Music structure for song playback (from lab 6)
 *===========================================================================*/
typedef struct music {
	int note;       /* Note frequency divider value */
	int size;       /* Note size (whole, half, quarter, etc.) */
	int tempo;      /* Tempo for duration calculation */
	int space;      /* Spacing between notes */
	char end;       /* End-of-song flag (1 = last note) */
} Music;

/*============================================================================
 * Game State Machine Enumeration
 * Controls the overall flow of the Battleship game.
 *===========================================================================*/
typedef enum {
    STATE_WELCOME,           /* Scrolling "BATTLESHIP" title message       */
    STATE_P1_PLACE,          /* Player 1 places boats on their map         */
    STATE_P1_TO_P2_TRANS,    /* Transition screen: hand-off to Player 2    */
    STATE_P2_PLACE,          /* Player 2 places boats on their map         */
    STATE_P2_TO_P1_TRANS,    /* Transition screen: hand-off back to P1     */
    STATE_P1_TURN,           /* Player 1 fires a shot at Player 2's map    */
    STATE_P1_RESULT,         /* Show hit/miss result of Player 1's shot    */
    STATE_TURN_TRANS_TO_P2,  /* Transition screen between turns            */
    STATE_P2_TURN,           /* Player 2 fires a shot at Player 1's map    */
    STATE_P2_RESULT,         /* Show hit/miss result of Player 2's shot    */
    STATE_TURN_TRANS_TO_P1,  /* Transition screen between turns            */
    STATE_GAME_OVER          /* Winner announcement with victory music     */
} GameState_t;

/*============================================================================
 * Boat Placement Sub-State Enumeration
 * Tracks which boat the current player is placing.
 *===========================================================================*/
typedef enum {
    BOAT_SINGLE_1,   /* First single-segment boat   */
    BOAT_SINGLE_2,   /* Second single-segment boat   */
    BOAT_SINGLE_3,   /* Third single-segment boat    */
    BOAT_DOUBLE_1,   /* First double-segment boat    */
    BOAT_DOUBLE_2,   /* Second double-segment boat   */
    BOAT_DONE        /* All 5 boats placed           */
} BoatPlaceState_t;

/*============================================================================
 * Sound Effect Enumeration
 * Identifies which sound effect to play.
 *===========================================================================*/
typedef enum {
    SFX_NONE,        /* No sound playing             */
    SFX_HIT,         /* Explosion sound on hit       */
    SFX_MISS,        /* Splash sound on miss         */
    SFX_PLACE,       /* Confirmation beep for placement */
    SFX_ERROR,       /* Error buzz for invalid move  */
    SFX_VICTORY      /* Victory fanfare song         */
} SoundEffect_t;

/*============================================================================
 * Player Map Structure
 * Each player has two boat maps and two corresponding shot maps.
 *
 * Array-based map encoding:
 *   vertMap[2][16]:   2 rows x 16 columns of vertical segments
 *   horizMap[3][8]:   3 rows x 8 columns of horizontal segments
 *
 * Shot map encoding (same array layout as boat maps):
 *   vertShots[2][16]:  1 = shot placed at that vertical segment position
 *   horizShots[3][8]:  1 = shot placed at that horizontal segment position
 *
 * A '1' in a boat map = boat present; '0' = empty water.
 * A '1' in a shot map = shot fired there; '0' = not yet fired.
 *===========================================================================*/
typedef struct {
    uint8_t vertMap[2][16];     /* Vertical boat positions:   [row 0-1][col 0-15]  */
    uint8_t horizMap[3][8];     /* Horizontal boat positions: [row 0-2][col 0-7]   */
    uint8_t vertShots[2][16];   /* Vertical shot history:     [row 0-1][col 0-15]  */
    uint8_t horizShots[3][8];   /* Horizontal shot history:   [row 0-2][col 0-7]   */
} PlayerMaps_t;

/*============================================================================
 * Cursor Structure
 * Tracks the cursor position and which segment layer it is on.
 *===========================================================================*/
typedef struct {
    uint8_t x;             /* Horizontal position (0-7 for horiz, 0-15 for vert) */
    uint8_t y;             /* Vertical position (row index)                      */
    uint8_t onVertical;    /* 0 = cursor on horizontal map, 1 = on vertical map  */
    uint8_t visible;       /* Blink state: 1 = visible, 0 = hidden               */
} Cursor_t;

/* ---- Extern declarations for ISR-shared variables (from lab 6) ---- */
extern char ramp;
extern char RED_BRT;
extern char GREEN_BRT;
extern char BLUE_BRT;
extern char RED_STEP;
extern char GREEN_STEP;
extern char BLUE_STEP;
extern char DIM_Enable;
extern int TONE;
extern int COUNT;
extern int INDEX;
extern Music Song[100];
extern int Note;
extern int Save_Note;
extern int Vibrato_Depth;
extern int Vibrato_Rate;
extern int Vibrato_Count;
extern char Animate_On;
extern char Music_ON;
extern char Message_Length;
extern char *Save_Pointer;
extern char *Message_Pointer;
extern int Delay_msec;
extern int Delay_counter;

/* ---- Extern declarations for Battleship game variables ---- */
extern volatile GameState_t    Game_State;       /* Current game state machine state   */
extern volatile uint8_t        Current_Player;   /* 1 or 2                             */
extern PlayerMaps_t            P1_Maps;          /* Player 1 boat and shot maps        */
extern PlayerMaps_t            P2_Maps;          /* Player 2 boat and shot maps        */
extern Cursor_t                Cursor;           /* Global cursor state                */
extern volatile BoatPlaceState_t Boat_State;     /* Which boat is being placed         */
extern volatile uint8_t        P1_Hits;          /* Number of hits Player 1 has scored  */
extern volatile uint8_t        P2_Hits;          /* Number of hits Player 2 has scored  */

/* ---- Display buffer: 8 digits, each a 7-seg pattern byte ---- */
extern volatile uint8_t        Display_Buffer[8];

/* ---- PWM dimming buffer: brightness per digit (0-10) ---- */
extern volatile uint8_t        Brightness_Buffer[8];

/* ---- Cursor blink control ---- */
extern volatile uint16_t       Blink_Counter;    /* ms counter for blink timing        */
extern volatile uint8_t        Blink_State;      /* 0 or 1, toggles at blink rate      */

/* ---- Sound effect control ---- */
extern volatile SoundEffect_t  Current_SFX;      /* Which SFX is currently playing     */

/* ---- ADC reading function ---- */
extern uint16_t Read_ADC(uint8_t channel);

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* Total number of boat segments per player (3 singles + 2 doubles = 7) */
#define TOTAL_BOAT_SEGMENTS  7

/* Maximum ADC value (12-bit) */
#define ADC_MAX              4095

/* Blink rate for cursor in milliseconds (toggle every 250ms) */
#define BLINK_RATE_MS        250

/* PWM frame period in ms (10ms = 100Hz for flicker-free dimming) */
#define PWM_FRAME_MS         10

/* Number of PWM brightness levels within one frame */
#define PWM_LEVELS           10

/* Brightness values for shot display */
#define BRIGHT_FULL          10   /* 100% brightness for hits   */
#define BRIGHT_HALF          5    /* 50% brightness for misses  */
#define BRIGHT_OFF           0    /* Segment not lit            */

/* Debounce delay in milliseconds for button presses */
#define DEBOUNCE_MS          200

/* Result display duration in milliseconds */
#define RESULT_DISPLAY_MS    1500

/* Transition screen duration in milliseconds */
#define TRANSITION_MS        3000

/* ADC channels for potentiometers (PA0-PA3) */
#define ADC_CH_CURSOR_X      1    /* Potentiometer for horizontal cursor */
#define ADC_CH_CURSOR_Y      2    /* Potentiometer for vertical cursor   */

/* Switch input pins on Port C — only 2 buttons */
#define SW_CONFIRM_PIN       (1 << 11)  /* PC11 - Confirm / Start / Fire       */
#define SW_ORIENT_PIN        (1 << 10)  /* PC10 - Orient / V-H map toggle      */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* ---- Game logic functions ---- */
void     Game_Init(void);
void     Game_Run(void);
void     Place_Boats(PlayerMaps_t *maps);
uint8_t  Fire_Shot(PlayerMaps_t *targetMaps, Cursor_t *cursor);
uint8_t  Check_Win(PlayerMaps_t *targetMaps);
uint8_t  Count_Hits(PlayerMaps_t *targetMaps);

/* ---- Cursor functions ---- */
void     Cursor_Update_From_ADC(void);

/* ---- Display functions ---- */
void     Display_Clear(void);
void     Display_Refresh(void);
void     Render_Display_Buffer_Digit(uint8_t digit, uint8_t pwm_tick);

/* ---- Message / marquee functions ---- */
void     Set_Marquee_Message(char *msg, uint8_t len, int speed);
void     Stop_Marquee(void);

/* ---- Sound effect functions ---- */
void     Play_SFX(SoundEffect_t sfx);
void     Play_Victory_Song(void);

/* ---- ADC read function ---- */
uint16_t Read_ADC(uint8_t channel);

/* ---- Utility ---- */
uint16_t Read_Switches(void);
void     Debounce_Delay(void);

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define CS_I2C_SPI_Pin GPIO_PIN_3
#define CS_I2C_SPI_GPIO_Port GPIOE
#define PC14_OSC32_IN_Pin GPIO_PIN_14
#define PC14_OSC32_IN_GPIO_Port GPIOC
#define PC15_OSC32_OUT_Pin GPIO_PIN_15
#define PC15_OSC32_OUT_GPIO_Port GPIOC
#define PH0_OSC_IN_Pin GPIO_PIN_0
#define PH0_OSC_IN_GPIO_Port GPIOH
#define PH1_OSC_OUT_Pin GPIO_PIN_1
#define PH1_OSC_OUT_GPIO_Port GPIOH
#define OTG_FS_PowerSwitchOn_Pin GPIO_PIN_0
#define OTG_FS_PowerSwitchOn_GPIO_Port GPIOC
#define PDM_OUT_Pin GPIO_PIN_3
#define PDM_OUT_GPIO_Port GPIOC
#define B1_Pin GPIO_PIN_0
#define B1_GPIO_Port GPIOA
#define I2S3_WS_Pin GPIO_PIN_4
#define I2S3_WS_GPIO_Port GPIOA
#define SPI1_SCK_Pin GPIO_PIN_5
#define SPI1_SCK_GPIO_Port GPIOA
#define SPI1_MISO_Pin GPIO_PIN_6
#define SPI1_MISO_GPIO_Port GPIOA
#define SPI1_MOSI_Pin GPIO_PIN_7
#define SPI1_MOSI_GPIO_Port GPIOA
#define BOOT1_Pin GPIO_PIN_2
#define BOOT1_GPIO_Port GPIOB
#define CLK_IN_Pin GPIO_PIN_10
#define CLK_IN_GPIO_Port GPIOB
#define LD4_Pin GPIO_PIN_12
#define LD4_GPIO_Port GPIOD
#define LD3_Pin GPIO_PIN_13
#define LD3_GPIO_Port GPIOD
#define LD5_Pin GPIO_PIN_14
#define LD5_GPIO_Port GPIOD
#define LD6_Pin GPIO_PIN_15
#define LD6_GPIO_Port GPIOD
#define I2S3_MCK_Pin GPIO_PIN_7
#define I2S3_MCK_GPIO_Port GPIOC
#define VBUS_FS_Pin GPIO_PIN_9
#define VBUS_FS_GPIO_Port GPIOA
#define OTG_FS_ID_Pin GPIO_PIN_10
#define OTG_FS_ID_GPIO_Port GPIOA
#define OTG_FS_DM_Pin GPIO_PIN_11
#define OTG_FS_DM_GPIO_Port GPIOA
#define OTG_FS_DP_Pin GPIO_PIN_12
#define OTG_FS_DP_GPIO_Port GPIOA
#define SWDIO_Pin GPIO_PIN_13
#define SWDIO_GPIO_Port GPIOA
#define SWCLK_Pin GPIO_PIN_14
#define SWCLK_GPIO_Port GPIOA
#define I2S3_SCK_Pin GPIO_PIN_10
#define I2S3_SCK_GPIO_Port GPIOC
#define I2S3_SD_Pin GPIO_PIN_12
#define I2S3_SD_GPIO_Port GPIOC
#define Audio_RST_Pin GPIO_PIN_4
#define Audio_RST_GPIO_Port GPIOD
#define OTG_FS_OverCurrent_Pin GPIO_PIN_5
#define OTG_FS_OverCurrent_GPIO_Port GPIOD
#define SWO_Pin GPIO_PIN_3
#define SWO_GPIO_Port GPIOB
#define Audio_SCL_Pin GPIO_PIN_6
#define Audio_SCL_GPIO_Port GPIOB
#define Audio_SDA_Pin GPIO_PIN_9
#define Audio_SDA_GPIO_Port GPIOB
#define MEMS_INT2_Pin GPIO_PIN_1
#define MEMS_INT2_GPIO_Port GPIOE

/* USER CODE BEGIN Private defines */

/* music notes by name, 125Khz (250Khz/2) frequency will be divided by these constants for 1/2 period  *
 * note frequency = 125Khz / (NOTE * 2)                                                                */

#define C0 3823
#define Cs0_Db0 3608
#define D0 3405
#define Ds0_Eb0 3214
#define E0 3034
#define F0 2864
#define Fs0_Gb0 2703
#define G0 2551
#define Gs0_Ab0 2408
#define A0 2273
#define As0_Bb0 2145
#define B0 2025

#define C1 1911
#define Cs1_Db1 1803
#define D1 1703
#define Ds1_Eb1 1607
#define E1 1517
#define F1 1432
#define Fs1_Gb1 1351
#define G1 1275
#define Gs1_Ab1 1204
#define A1 1136
#define As1_Bb1 1073
#define B1 1012

#define C2 956
#define Cs2_Db2 902
#define D2 851
#define Ds2_Eb2 804
#define E2 758
#define F2 716
#define Fs2_Gb2 676
#define G2 638
#define Gs2_Ab2 602
#define A2 568
#define As2_Bb2 536
#define B2 506

#define C3 478
#define Cs3_Db3 451
#define D3 426
#define Ds3_Eb3 402
#define E3 379
#define F3 358
#define Fs3_Gb3 338
#define G3 319
#define Gs3_Ab3 301
#define A3 284
#define As3_Bb3 268
#define B3 253

#define C4 239
#define Cs4_Db4 225
#define D4 213
#define Ds4_Eb4 201
#define E4 190
#define F4 179
#define Fs4_Gb4 169
#define G4 159
#define Gs4_Ab4 150
#define A4 142
#define As4_Bb4 134
#define B4 127

#define C5 119
#define Cs5_Db5 113
#define D5 106
#define Ds5_Eb5 100
#define E5 95
#define F5 89
#define Fs5_Gb5 84
#define G5 80
#define Gs5_Ab5 75
#define A5 71
#define As5_Bb5 67
#define B5 63

#define C6 60
#define Cs6_Db6 56
#define D6 53
#define Ds6_Eb6 50
#define E6 47
#define F6 45
#define Fs6_Gb6 42
#define G6 40
#define Gs6_Ab6 38
#define A6 36
#define As6_Bb6 34
#define B6 32

/* note size constants to divide tempo value to get note duration */
#define whole 1
#define half 2
#define quarter 4
#define _8th 8
#define _16th 16
#define _32nd 32
#define rest 0

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
