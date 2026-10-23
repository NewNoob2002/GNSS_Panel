#ifndef __COMMON_MACRO_H
#define __COMMON_MACRO_H

/*********************************/
/*          通用宏定义库         */
/*  提升代码的精简程度以及可读性 */
/*    Designed by _VIFEXTech     */
/*********************************/

/*
* @brief  判断一个值是否在一个范围内
* @param  value:被判断的值
* @param  min:最小值
* @param  max:最大值
* @retval 是否在范围内
*/
#define CM_VALUE_IN_RANGE(value, min, max)     ((value) >= (min) && (value) <= (max))
/*
* @brief  将一个值限制在一个范围内
* @param  value:设置的值
* @param  min:最小值
* @param  max:最大值
* @retval 是否在范围内
*/
#define CM_SET_VALUE_IN_RANGE(value, min, max) ((value) < (min) ? (min) : ((value) > (max) ? (max) : (value)))

#endif