/************************************************************************************** 
   @project: app project config
   @author： Azolla
   @time  :  2021.06.05
***************************************************************************************/

#pragma once

#include "sdkconfig.h"

#define APP_CONFIG_DEBUG_ENABLE           1       // 1：使能debug配置；0：默认使用menuconfig sdkconfig.h

#if APP_CONFIG_DEBUG_ENABLE  // debug enable

/* @START ************************************************************************************/
/** Config APP UART. */ 
#define APP_UART_PIN_TXD            22
#define APP_UART_PIN_RXD            23

#define APP_UART_PORT_NUM           1
#define APP_UART_BAUD_RATE          115200
#define APP_UART_TASK_STACK_SIZE    2048
#define APP_UART_BUF_SIZE           128     // UART_BUF_SIZE >= 128
/* @END **************************************************************************************/

/* @START ************************************************************************************/
/** Config APP MQTT. */ 
#define APP_MQTT_HOST                "emqx.qinyun575.cn"
#define APP_MQTT_PORT                1883
#define APP_MQTT_USERNAME            "emqx"
#define APP_MQTT_USERWORD            "public"
#define APP_MQTT_BUFF_SIZE           128
#define APP_MQTT_QOS                 2               // 0~2

#define APP_MQTT_TOPIC_READ          "/topic/read"
#define APP_MQTT_TOPIC_WRITE         "/topic/write"

/* @END **************************************************************************************/


#else  // menuconfig->sdkconfig

/* @START ************************************************************************************/
/** Config APP UART. */ 
#define APP_UART_PIN_TXD                        CONFIG_APP_UART_PIN_TXD            
#define APP_UART_PIN_RXD                        CONFIG_APP_UART_PIN_RXD           
#define APP_UART_PORT_NUM                       CONFIG_APP_UART_PORT_NUM           
#define APP_UART_BAUD_RATE                      CONFIG_APP_UART_BAUD_RATE           
#define APP_UART_TASK_STACK_SIZE                CONFIG_APP_UART_TASK_STACK_SIZE     

#define APP_UART_BUF_SIZE                       CONFIG_APP_UART_BUF_SIZE           
/* @END **************************************************************************************/

/* @START ************************************************************************************/
/** Config APP MQTT. */ 
#define APP_MQTT_HOST                           CONFIG_APP_MQTT_HOST                 
#define APP_MQTT_PORT                           CONFIG_APP_MQTT_PORT                 
#define APP_MQTT_USERNAME                       CONFIG_APP_MQTT_USERNAME            
#define APP_MQTT_USERWORD                       CONFIG_APP_MQTT_USERWORD           
#define APP_MQTT_BUFF_SIZE                      CONFIG_APP_MQTT_BUFF_SIZE         
#define APP_MQTT_QOS                            CONFIG_APP_MQTT_QOS                

#define APP_MQTT_TOPIC_READ                     CONFIG_APP_MQTT_TOPIC_READ        
#define APP_MQTT_TOPIC_WRITE                    CONFIG_APP_MQTT_TOPIC_WRITE    
/* @END **************************************************************************************/

#endif  // __APP_CONFIG_DEBUG_ENABLE END.




/* @START ************************************************************************************/
/** Config APP Other. */ 
#define APP_MQTT_UART_BUFF_SIZE       32        // MQTT 与 UART 单条数据最大BUFF/byte
/* @END **************************************************************************************/



