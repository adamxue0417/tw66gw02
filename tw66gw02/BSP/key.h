/**
  ******************************************************************************
  * @file    key.h
  * @author 
  * @version V1.0
  * @date
  * @brief   Key scan and key event interface definitions.
  ******************************************************************************
  */
#ifndef  __KEY_H__
#define   __KEY_H__
#include <stdint.h>
#define KeyNumber 2
//typedef enum KeyState
//{
//   Idle=0,
//   ShortPress,
//   LongPress
//}_KeyState;
typedef enum {
   Idle          = 0,
   QuickPress    = 1,   
   LongPress     = 2,   
   VeryLongPress = 3    
}_KeyState;
extern _KeyState KeyState[KeyNumber];
void Key_Scan(void);
void KeyRespose(void(*Key0ShortPress)(void),
                void(*Key1ShortPress)(void),
                void(*Key0LongPress)(void),
                void(*Key1LongPress)(void),
                void(*Key0VeryLongPress)(void),
                void(*Key1VeryLongPress)(void));
void Key_Respose_Nothing(void);  
void Key0_short_press(void);
void Key1_short_press(void);
void Key0_long_press(void);
void Key1_long_press(void);
void Key0_very_long_press(void);	
void Key1_very_long_press(void);								
#endif
