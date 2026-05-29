// tcpipserver.c

#include "lwip/api.h"
#include "cmsis_os.h"
#include "main.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#define TCP_SERVER_PORT 5000
#define TCP_RX_BUF_SIZE 128
#define TCP_TX_BUF_SIZE 256

typedef enum
{
    SET_VAL_0 = 0,
    SET_VAL_30 = 30,
    SET_VAL_130 = 130,
    SET_VAL_155 = 155,
    SET_VAL_H = 999
} SetValue_t; /*Attenuation values*/

typedef enum
{
    SET_PATH_A1 = 0,
    SET_PATH_A2,
	SET_PATH_B1,
	SET_PATH_B2
} SetPathValue_t;

typedef enum
{
 	Err_ok =0,
	Err_KA_Disconnected,
	Err_KA_Short,
	Err_KB_Disconnected,
    Err_KB_Short,
    Err_PWR,
	Err_InvalidConfig

}ErrorStatus_t;


typedef struct {
	GPIO_PinState PinStatus_KA_NC;
	GPIO_PinState PinStatus_KA_NO;
	GPIO_PinState PinStatus_KB_NC;
    GPIO_PinState PinStatus_KB_NO;
    GPIO_PinState PinStatus_nFLT;
	GPIO_PinState PinStatus_PGOOD;

}PathStatus_t;



static SetPathValue_t g_set_value = SET_VAL_0;
static SetPathValue_t g_get_value = SET_VAL_0;

ErrorStatus_t* CheckRelayStatusA1(ErrorStatus_t*);
ErrorStatus_t* CheckRelayStatusA2(ErrorStatus_t*);
ErrorStatus_t* CheckRelayStatusB1(ErrorStatus_t*);
ErrorStatus_t* CheckRelayStatusB2(ErrorStatus_t*);
static void CheckPathVal(SetPathValue_t,SetPathValue_t,ErrorStatus_t*, ErrorStatus_t*);



static void tcp_server_thread(void *arg);
static void process_command(const char *cmd, char *reply, size_t reply_size);

static int parse_path_value(const char *s, SetPathValue_t *value);
static int parse_path_value2(const char *s, SetPathValue_t *value);
static int parse_att_value(const char *s, SetValue_t *value);
static const char *set_value_to_string(SetValue_t value);

static void hw_apply_set(SetValue_t, SetPathValue_t,SetPathValue_t, ErrorStatus_t*,ErrorStatus_t* );
static SetValue_t hw_read_get(void);

static float read_3v3_voltage(void);
static float read_vin_voltage(void);
static uint32_t read_pon_counter(void);
static uint32_t read_eti_minutes(void);

void tcp_server_init(void)
{
    sys_thread_new("TCP_SRV",
                   tcp_server_thread,
                   NULL,
                   DEFAULT_THREAD_STACKSIZE,
                   osPriorityNormal);
}

static void tcp_server_thread(void *arg)
{
    struct netconn *conn;
    struct netconn *newconn;
    struct netbuf *inbuf;
    char *buf;
    u16_t buflen;

    char rx_cmd[TCP_RX_BUF_SIZE];
    char tx_reply[TCP_TX_BUF_SIZE];

    LWIP_UNUSED_ARG(arg);

    conn = netconn_new(NETCONN_TCP);

    if (conn == NULL)
    {
        return;
    }

    if (netconn_bind(conn, NULL, TCP_SERVER_PORT) != ERR_OK)
    {
        netconn_delete(conn);
        return;
    }

    netconn_listen(conn);

    while (1)
    {
        if (netconn_accept(conn, &newconn) == ERR_OK)
        {
            while (netconn_recv(newconn, &inbuf) == ERR_OK)
            {
                if (netbuf_data(inbuf, (void **)&buf, &buflen) == ERR_OK)
                {
                    size_t copy_len = buflen;

                    if (copy_len >= TCP_RX_BUF_SIZE)
                    {
                        copy_len = TCP_RX_BUF_SIZE - 1;
                    }

                    memcpy(rx_cmd, buf, copy_len);
                    rx_cmd[copy_len] = '\0';

                    process_command(rx_cmd, tx_reply, sizeof(tx_reply));

                    netconn_write(newconn,
                                  tx_reply,
                                  strlen(tx_reply),
                                  NETCONN_COPY);
                }

                netbuf_delete(inbuf);
            }

            netconn_close(newconn);
            netconn_delete(newconn);
        }
    }
}

static void process_command(const char *cmd, char *reply, size_t reply_size)
{
    /*potrebbe arrivare sporcizia, scorri il buffer finchè non trovi qualcosa di interessante*/
	while (*cmd == ' ' || *cmd == '\t' || *cmd == '\r' || *cmd == '\n')
    {
        cmd++;
    }

    if (strncmp(cmd, "set", 3) == 0) //* confronta le prime tre locazioni del buffer*/
    {
        const char *path = cmd + 3;
        const char *path2 = path +3;
        const char *attenuation = path2 +3;

        //const char *post_attenuation = attenuation + 3;

        SetValue_t requestedAtt_value;
        SetPathValue_t requestedPath_value; //A1 o A2
        SetPathValue_t requestedPath_value2; //B1 o B2
        ErrorStatus_t RelayErrorPath1;
        ErrorStatus_t RelayErrorPath2;

        /* set_a1_30, set_a1_z*/
        //check_path -> A1/A2
        //check_attenuation -> 30,Z ecc


        /*Controllo preliminare se sono stati digitati gli spazi nel posto giusto, il comando sarà set_Ax_Z*/
         if(((*path != '\r') &&
         	( *path  != '\t') &&
 			( *path  != '\n') &&
 			( *path  !=  ' ') &&
			( *path  !=  '\0'))||

			((*attenuation != '\r') &&
			( *attenuation != '\t') &&
		    ( *attenuation != '\n') &&
			( *attenuation !=  ' ') &&
			( *attenuation !=  '\0'))
		)

        {
           snprintf(reply, reply_size, "err:val incorrect spaces\r\n");
           return;
        }


        while (*path == ' ' || *path == '\t') /* se prima di path trovi spazi ecc... vai avanti*/
        {
            path++;
        }

         while (*path2 == ' ' || *path2 == '\t') /* se prima di path2 trovi spazi ecc... vai avanti*/
         {
            path2++;
         }


        while (*attenuation == ' ' || *attenuation == '\t') /* se prima di attenuation trovi spazi ecc... vai avanti*/
        {
            attenuation++;
        }





        if (parse_path_value(path, &requestedPath_value) == 0 ||  /*check A1 or A2*/
        	parse_att_value(attenuation, &requestedAtt_value) == 0 || /*check possible attenuations*/
			parse_path_value2(path2, &requestedPath_value2) == 0) /*check B1 or B2 */
        {
            snprintf(reply, reply_size, "err:val\r\n");
            return;
        }


        /* applicazione hardware del comando ricevuto Relay managment*/
        //g_set_value = requestedPath_value;
        hw_apply_set(requestedAtt_value, requestedPath_value,requestedPath_value2, &RelayErrorPath1, &RelayErrorPath2);

        osDelay(10);

        //g_get_value = hw_read_get();

        //if (g_get_value == g_set_value)
        if(!RelayErrorPath1 && !RelayErrorPath2)
        {
            snprintf(reply, reply_size, "ok:set\r\n");
        }
        else if ((RelayErrorPath1 == Err_KA_Disconnected) && !RelayErrorPath2)
        {
            snprintf(reply, reply_size, "err:KA disconnected\r\n");

        }
        else if (!RelayErrorPath1 && (RelayErrorPath2 == Err_KB_Disconnected))
        {
            snprintf(reply, reply_size, "err:KB disconnected\r\n");
        }
        else if ((RelayErrorPath1 == Err_KA_Disconnected) && (RelayErrorPath2 == Err_KB_Disconnected))
        {
            snprintf(reply, reply_size, "err:KA and KB disconnected\r\n");
        }
        else if ((RelayErrorPath1 == Err_KA_Short) && (RelayErrorPath2 == Err_KB_Disconnected))
        {
            snprintf(reply, reply_size, "err:KA Short, KB disconnected\r\n");
        }
        else if ((RelayErrorPath1 == Err_KA_Disconnected) && (RelayErrorPath2 == Err_KB_Short))
        {
             snprintf(reply, reply_size, "err:KB Short, KB disconnected\r\n");
        }
        else if (RelayErrorPath1 == Err_InvalidConfig || RelayErrorPath2 == Err_InvalidConfig )
        {

        	snprintf(reply, reply_size, "err:Invalid Feedback\r\n");
        }
        /*to be completed... */

        return;
    }

    if (strncmp(cmd, "get", 3) == 0)
    {
        g_get_value = hw_read_get();

        if (g_get_value == g_set_value)
        {
            snprintf(reply,
                     reply_size,
                     "ok:get %s\r\n",
                     set_value_to_string(g_get_value));
        }
        else
        {
            snprintf(reply, reply_size, "ok:get err\r\n");
        }

        return;
    }

    if (strncmp(cmd, "info", 4) == 0)
    {
        uint32_t uid0 = HAL_GetUIDw0();
        uint32_t uid1 = HAL_GetUIDw1();
        uint32_t uid2 = HAL_GetUIDw2();

        snprintf(reply,
                 reply_size,
                 "ok:info 3V3:%lumV Vin:%lumV PN:%s(%s) FW:%s(build:%s_%s_by:%s) UID:%08lX%08lX%08lX(rev:0x%lX_dev:0x%lX) PON:%lu ETI:%lu(m)\r\n",
                 (unsigned long)(read_3v3_voltage() * 1000.0f),
                 (unsigned long)(read_vin_voltage() * 1000.0f),
                 "T1893GP",
                 "VQ1M RF CTRL",
                 "1.0.0",
                 __DATE__,
                 __TIME__,
                 "bl91",
                 (unsigned long)uid0,
                 (unsigned long)uid1,
                 (unsigned long)uid2,
                 (unsigned long)HAL_GetREVID(),
                 (unsigned long)HAL_GetDEVID(),
                 (unsigned long)read_pon_counter(),
                 (unsigned long)read_eti_minutes());
        return;
    }

    if ((strncmp(cmd, "h", 1) == 0) ||
        (strncmp(cmd, "help", 4) == 0) ||
        (strncmp(cmd, "man", 3) == 0))
    {
        snprintf(reply,
                 reply_size,
                 "Commands:\r\n"
                 "set 0\r\n"
                 "set 30\r\n"
                 "set 130\r\n"
                 "set 155\r\n"
                 "set H\r\n"
                 "get\r\n"
                 "info\r\n"
                 "help\r\n");

        return;
    }

    snprintf(reply, reply_size, "err:cmd\r\n");
}

static int parse_path_value(const char *s, SetPathValue_t *value)
{
    if (strncmp(s, "A1", 2) == 0)
    {
        *value = SET_PATH_A1;
        return 1;
    }
    if (strncmp(s, "A2", 2) == 0)
    {
           *value = SET_PATH_A2;
           return 1;
    }


    return 0;
}


static int parse_path_value2(const char *s, SetPathValue_t *value)
{


    if (strncmp(s, "B1", 2) == 0)
    {
          *value = SET_PATH_B1;
           return 1;
    }

    if (strncmp(s, "B2", 2) == 0)
    {
          *value = SET_PATH_B1;
           return 1;
    }

    return 0;
}

static int parse_att_value(const char *s, SetValue_t *value)
{
    if (strncmp(s, "0", 1) == 0)
    {
        *value = SET_VAL_0;
        return 1;
    }

    if (strncmp(s, "30", 2) == 0)
    {
        *value = SET_VAL_30;
        return 1;
    }

    if (strncmp(s, "130", 3) == 0)
    {
        *value = SET_VAL_130;
        return 1;
    }

    if (strncmp(s, "155", 3) == 0)
    {
        *value = SET_VAL_155;
        return 1;
    }

    if ((s[0] == 'H') || (s[0] == 'h'))
    {
        *value = SET_VAL_H;
        return 1;
    }

    return 0;
}

static const char *set_value_to_string(SetValue_t value)
{
    switch (value)
    {
    case SET_VAL_0:
        return "0";

    case SET_VAL_30:
        return "30";

    case SET_VAL_130:
        return "130";

    case SET_VAL_155:
        return "155";

    case SET_VAL_H:
        return "H";

    default:
        return "err";
    }
}


static void hw_apply_set(SetValue_t AttVal, SetPathValue_t PathVal,SetPathValue_t PathVal2, ErrorStatus_t* ErrorStatusPath1, ErrorStatus_t* ErrorStatusPath2)
{


	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_SET); //nSHDN=1
	switch (AttVal)
    {

		case SET_VAL_0:
			 CheckPathVal(PathVal,PathVal2,ErrorStatusPath1,ErrorStatusPath2);
		break;

		case SET_VAL_30:
			 CheckPathVal(PathVal,PathVal2,ErrorStatusPath1,ErrorStatusPath2);
		break;

		case SET_VAL_130:
			CheckPathVal(PathVal,PathVal2,ErrorStatusPath1,ErrorStatusPath2);
		break;

		case SET_VAL_155:
			CheckPathVal(PathVal,PathVal2,ErrorStatusPath1,ErrorStatusPath2);
		break;

		case SET_VAL_H:
			CheckPathVal(PathVal,PathVal2,ErrorStatusPath1,ErrorStatusPath2);
		break;

		default:

		break;
    }
}


ErrorStatus_t* CheckRelayStatusA1(ErrorStatus_t* ErrorStatus )
{

	PathStatus_t PinStatus;
	PinStatus.PinStatus_KA_NC = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_7);
	PinStatus.PinStatus_KA_NO = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_6);
	PinStatus.PinStatus_nFLT =  HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_5);
	PinStatus.PinStatus_PGOOD = HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_3);




	if((PinStatus.PinStatus_KA_NC == GPIO_PIN_RESET) &&
	  (PinStatus.PinStatus_KA_NO ==  GPIO_PIN_SET) &&
	  (PinStatus.PinStatus_nFLT ==   GPIO_PIN_SET) &&
	  (PinStatus.PinStatus_PGOOD ==  GPIO_PIN_SET))
	  {


		 HAL_GPIO_WritePin(GPIOD, GPIO_PIN_3, GPIO_PIN_RESET);  //L_LA1_R =0
		 HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_SET);    //L_LA1_G = 1
		 HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_RESET);  //L_LA2_R = 0
		 HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET); //L_LA2_G = 0

		*ErrorStatus = Err_ok;

	  }

	else if((PinStatus.PinStatus_KA_NC == GPIO_PIN_SET) &&
		  (PinStatus.PinStatus_KA_NO ==   GPIO_PIN_SET) &&
		  (PinStatus.PinStatus_nFLT ==    GPIO_PIN_SET) &&
		  (PinStatus.PinStatus_PGOOD ==   GPIO_PIN_SET))

      {

		 HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET); //nSHTDN=0

		 HAL_GPIO_WritePin(GPIOD, GPIO_PIN_3, GPIO_PIN_SET);  //L_LA1_R =1
	     HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_RESET);    //L_LA1_G = 0
		 HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_RESET);  //L_LA2_R = 0
		 HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET); //L_LA2_G = 0


		*ErrorStatus = Err_KA_Disconnected;

	  }

	else if((PinStatus.PinStatus_KA_NC ==   GPIO_PIN_RESET) &&
			  (PinStatus.PinStatus_KA_NO == GPIO_PIN_RESET) &&
			  (PinStatus.PinStatus_nFLT ==  GPIO_PIN_SET) &&
			  (PinStatus.PinStatus_PGOOD == GPIO_PIN_SET))

	      {

			 HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET); //nSHTDN=0

			 HAL_GPIO_WritePin(GPIOD, GPIO_PIN_3, GPIO_PIN_SET);  //L_LA1_R =1
		     HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_RESET);    //L_LA1_G = 0
			 HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_SET);  //L_LA2_R = 1
			 HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET); //L_LA2_G = 1


			*ErrorStatus = Err_KA_Short;

		  }

	else if((PinStatus.PinStatus_KA_NC ==       GPIO_PIN_SET) &&
				  (PinStatus.PinStatus_KA_NO == GPIO_PIN_SET) &&
				  (PinStatus.PinStatus_nFLT ==  GPIO_PIN_RESET) &&
				  (PinStatus.PinStatus_PGOOD == GPIO_PIN_RESET))

		      {

				 HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET); //nSHTDN=0

				 HAL_GPIO_WritePin(GPIOD, GPIO_PIN_3, GPIO_PIN_SET);  //L_LA1_R =1
			     HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_RESET);    //L_LA1_G = 0
				 HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_RESET);  //L_LA2_R = 0
				 HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET); //L_LA2_G = 0


				*ErrorStatus = Err_PWR;

			  }

	     else
		    {

		       HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET); //nSHTDN=0
		       *ErrorStatus = Err_InvalidConfig;

		     }
      return ErrorStatus;

}
ErrorStatus_t* CheckRelayStatusB1(ErrorStatus_t* ErrorStatus )
{

	PathStatus_t PinStatus;
	PinStatus.PinStatus_KB_NC = HAL_GPIO_ReadPin(GPIOF, GPIO_PIN_2);
	PinStatus.PinStatus_KB_NO = HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_5);
	PinStatus.PinStatus_nFLT =  HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_5);
	PinStatus.PinStatus_PGOOD = HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_3);




	if((PinStatus.PinStatus_KB_NC == GPIO_PIN_RESET) &&
	  (PinStatus.PinStatus_KB_NO ==  GPIO_PIN_SET) &&
	  (PinStatus.PinStatus_nFLT ==   GPIO_PIN_SET) &&
	  (PinStatus.PinStatus_PGOOD ==  GPIO_PIN_SET))
	  {


		 HAL_GPIO_WritePin(GPIOG, GPIO_PIN_10, GPIO_PIN_RESET);  //L_LB1_R =0
		 HAL_GPIO_WritePin(GPIOH, GPIO_PIN_2,  GPIO_PIN_SET);    //L_LB1_G = 1
		 HAL_GPIO_WritePin(GPIOE, GPIO_PIN_6,  GPIO_PIN_RESET);  //L_LB2_R = 0
		 HAL_GPIO_WritePin(GPIOG, GPIO_PIN_15, GPIO_PIN_RESET); //L_LB2_G = 0

		*ErrorStatus = Err_ok;

	  }

	else if((PinStatus.PinStatus_KB_NC == GPIO_PIN_SET) &&
		  (PinStatus.PinStatus_KB_NO ==   GPIO_PIN_SET) &&
		  (PinStatus.PinStatus_nFLT ==    GPIO_PIN_SET) &&
		  (PinStatus.PinStatus_PGOOD ==   GPIO_PIN_SET))

      {

		 HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET); //nSHTDN=0

		 HAL_GPIO_WritePin(GPIOD, GPIO_PIN_10, GPIO_PIN_SET);  //L_LB1_R =1
	     HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2,  GPIO_PIN_RESET);    //L_LB1_G = 0
		 HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6,  GPIO_PIN_RESET);  //L_LB2_R = 0
		 HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET); //L_LB2_G = 0


		*ErrorStatus = Err_KB_Disconnected;

	  }

	else if((PinStatus.PinStatus_KB_NC ==   GPIO_PIN_RESET) &&
			  (PinStatus.PinStatus_KB_NO == GPIO_PIN_RESET) &&
			  (PinStatus.PinStatus_nFLT ==  GPIO_PIN_SET) &&
			  (PinStatus.PinStatus_PGOOD == GPIO_PIN_SET))

	      {

			 HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET); //nSHTDN=0

			 HAL_GPIO_WritePin(GPIOD, GPIO_PIN_10, GPIO_PIN_SET);  //L_LB1_R =1
		     HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2,  GPIO_PIN_RESET);    //L_LB1_G = 0
			 HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6,  GPIO_PIN_SET);  //L_LB2_R = 1
			 HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_SET); //L_LB2_G = 1


			*ErrorStatus = Err_KB_Short;

		  }

	else if((PinStatus.PinStatus_KB_NC == GPIO_PIN_SET) &&
				  (PinStatus.PinStatus_KB_NO == GPIO_PIN_SET) &&
				  (PinStatus.PinStatus_nFLT == GPIO_PIN_RESET) &&
				  (PinStatus.PinStatus_PGOOD == GPIO_PIN_RESET))

		      {

				 HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET); //nSHTDN=0

				 HAL_GPIO_WritePin(GPIOD, GPIO_PIN_10, GPIO_PIN_SET);  //L_LB1_R =1
			     HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_RESET);    //L_LB1_G = 0
				 HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_RESET);  //L_LB2_R = 0
				 HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET); //L_LB2_G = 0


				*ErrorStatus = Err_PWR;

			  }
	else
		{

			   HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET); //nSHTDN=0
		       *ErrorStatus = Err_InvalidConfig;

		 }

      return ErrorStatus;

}
ErrorStatus_t* CheckRelayStatusA2(ErrorStatus_t* ErrorStatus )
{

	PathStatus_t PinStatus;
	PinStatus.PinStatus_KA_NC = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_7);
	PinStatus.PinStatus_KA_NO = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_6);
	PinStatus.PinStatus_nFLT =  HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_5);
	PinStatus.PinStatus_PGOOD = HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_3);




	if((PinStatus.PinStatus_KA_NC == GPIO_PIN_SET) &&
	  (PinStatus.PinStatus_KA_NO == GPIO_PIN_RESET) &&
	  (PinStatus.PinStatus_nFLT == GPIO_PIN_SET) &&
	  (PinStatus.PinStatus_PGOOD == GPIO_PIN_SET))
	  {


		 HAL_GPIO_WritePin(GPIOD, GPIO_PIN_3, GPIO_PIN_RESET);    //L_LA1_R =0
		 HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_RESET);   //L_LA1_G = 0
		 HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_RESET);  //L_LA2_R = 0
		 HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);   //L_LA2_G = 1

		*ErrorStatus = Err_ok;

	  }

	else if((PinStatus.PinStatus_KA_NC == GPIO_PIN_SET) &&
		  (PinStatus.PinStatus_KA_NO == GPIO_PIN_SET) &&
		  (PinStatus.PinStatus_nFLT == GPIO_PIN_SET) &&
		  (PinStatus.PinStatus_PGOOD == GPIO_PIN_SET))

      {

		 HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET);     //nSHTDN=0

		 HAL_GPIO_WritePin(GPIOD, GPIO_PIN_3, GPIO_PIN_SET);       //L_LA1_R =1
	     HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_RESET);    //L_LA1_G = 0
		 HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_SET);     //L_LA2_R = 1
		 HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);  //L_LA2_G = 0


		*ErrorStatus = Err_KA_Disconnected;

	  }

	else if((PinStatus.PinStatus_KA_NC == GPIO_PIN_RESET) &&
			  (PinStatus.PinStatus_KA_NO == GPIO_PIN_RESET) &&
			  (PinStatus.PinStatus_nFLT == GPIO_PIN_SET) &&
			  (PinStatus.PinStatus_PGOOD == GPIO_PIN_SET))

	      {

			 HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET); //nSHTDN=0

			 HAL_GPIO_WritePin(GPIOD, GPIO_PIN_3, GPIO_PIN_SET);  //L_LA1_R =1
		     HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_SET);    //L_LA1_G = 1
			 HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_SET);  //L_LA2_R = 1
			 HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET); //L_LA2_G = 0


			*ErrorStatus = Err_KA_Short;

		  }

	else if((PinStatus.PinStatus_KA_NC == GPIO_PIN_SET) &&
				  (PinStatus.PinStatus_KA_NO == GPIO_PIN_SET) &&
				  (PinStatus.PinStatus_nFLT == GPIO_PIN_RESET) &&
				  (PinStatus.PinStatus_PGOOD == GPIO_PIN_RESET))

		      {

				 HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET); //nSHTDN=0

				 HAL_GPIO_WritePin(GPIOD, GPIO_PIN_3, GPIO_PIN_SET);  //L_LA1_R =1
			     HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_RESET);    //L_LA1_G = 0
				 HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_RESET);  //L_LA2_R = 0
				 HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET); //L_LA2_G = 0


				*ErrorStatus = Err_PWR;

			  }

	else
	{

				     HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET); //nSHTDN=0
	                 *ErrorStatus = Err_InvalidConfig;

	 }






    return ErrorStatus;

}
ErrorStatus_t* CheckRelayStatusB2(ErrorStatus_t* ErrorStatus )
{

	PathStatus_t PinStatus;
	PinStatus.PinStatus_KB_NC = HAL_GPIO_ReadPin(GPIOF, GPIO_PIN_2);
	PinStatus.PinStatus_KB_NO = HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_5);
	PinStatus.PinStatus_nFLT =  HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_5);
	PinStatus.PinStatus_PGOOD = HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_3);




	if((PinStatus.PinStatus_KB_NC == GPIO_PIN_SET) &&
	  (PinStatus.PinStatus_KB_NO == GPIO_PIN_RESET) &&
	  (PinStatus.PinStatus_nFLT == GPIO_PIN_SET) &&
	  (PinStatus.PinStatus_PGOOD == GPIO_PIN_SET))
	  {


		 HAL_GPIO_WritePin(GPIOG, GPIO_PIN_10, GPIO_PIN_RESET);  //L_LB1_R =0
		 HAL_GPIO_WritePin(GPIOH, GPIO_PIN_2, GPIO_PIN_RESET);    //L_LB1_G = 0
		 HAL_GPIO_WritePin(GPIOE, GPIO_PIN_6, GPIO_PIN_RESET);  //L_LB2_R = 0
		 HAL_GPIO_WritePin(GPIOG, GPIO_PIN_15, GPIO_PIN_SET); //L_LB2_G = 1

		*ErrorStatus = Err_ok;

	  }

	else if((PinStatus.PinStatus_KB_NC == GPIO_PIN_SET) &&
		  (PinStatus.PinStatus_KB_NO == GPIO_PIN_SET) &&
		  (PinStatus.PinStatus_nFLT == GPIO_PIN_SET) &&
		  (PinStatus.PinStatus_PGOOD == GPIO_PIN_SET))

      {

		 HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET); //nSHTDN=0

		 HAL_GPIO_WritePin(GPIOD, GPIO_PIN_10, GPIO_PIN_SET);  //L_LB1_R =1
	     HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_RESET);    //L_LB1_G = 0
		 HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_SET);  //L_LB2_R = 1
		 HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET); //L_LB2_G = 0


		*ErrorStatus = Err_KB_Disconnected;

	  }

	else if((PinStatus.PinStatus_KB_NC == GPIO_PIN_RESET) &&
			  (PinStatus.PinStatus_KB_NO == GPIO_PIN_RESET) &&
			  (PinStatus.PinStatus_nFLT == GPIO_PIN_SET) &&
			  (PinStatus.PinStatus_PGOOD == GPIO_PIN_SET))

	      {

			 HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET); //nSHTDN=0

			 HAL_GPIO_WritePin(GPIOD, GPIO_PIN_10, GPIO_PIN_SET);  //L_LB1_R =1
		     HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_SET);    //L_LB1_G = 1
			 HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_SET);  //L_LB2_R = 1
			 HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET); //L_LB2_G = 0


			*ErrorStatus = Err_KB_Short;

		  }

	else if((PinStatus.PinStatus_KB_NC == GPIO_PIN_SET) &&
				  (PinStatus.PinStatus_KB_NO == GPIO_PIN_SET) &&
				  (PinStatus.PinStatus_nFLT == GPIO_PIN_RESET) &&
				  (PinStatus.PinStatus_PGOOD == GPIO_PIN_RESET))

		      {

				 HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET); //nSHTDN=0

				 HAL_GPIO_WritePin(GPIOD, GPIO_PIN_10, GPIO_PIN_SET);  //L_LB1_R    = 1
			     HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_RESET);    //L_LB1_G = 0
				 HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_RESET);  //L_LB2_R   = 0
				 HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET); //L_LB2_G   = 0


				*ErrorStatus = Err_PWR;

			  }

	else
		     {

		       HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET); //nSHTDN=0
		       *ErrorStatus = Err_InvalidConfig;

		     }



	return ErrorStatus;

}

static void CheckPathVal(SetPathValue_t PathVal,SetPathValue_t PathVal2, ErrorStatus_t* ErrorStatusPath1, ErrorStatus_t* ErrorStatusPath2)
{
	if (PathVal == SET_PATH_A1)
	 {
	     /*Set Relay di ingresso a 2 stati*/
	     HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);  //K_A_E = 0

	     /*Da inserire un delay per attendere lo switch del relay prima di fare il check*/

	     /*Lettura Status Relay a 2 stati*/
	     CheckRelayStatusA1(ErrorStatusPath1);

	     /* Set Relay a 3 stati */


	  }

	 if (PathVal == SET_PATH_A2)
	 {
	    /*Set Relay di ingresso a 2 stati*/
	    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_SET);  //K_A_E = 1


	    /*Da inserire un delay per attendere lo switch del relay prima di fare il check*/

	    /*Lettura Status Relay a 2 stati*/
	    CheckRelayStatusA2(ErrorStatusPath1);


	    /* Set Relay a 3 stati */
	 }


	 if (PathVal2 == SET_PATH_B1)
	  {
		    /*Set Relay di ingresso a 2 stati*/
         HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);  //K_B_E = 0


         /*Da inserire un delay per attendere lo switch del relay prima di fare il check*/

         /*Lettura Status Relay a 2 stati*/
         CheckRelayStatusB1(ErrorStatusPath2);


         /* Set Relay a 3 stati */
      }



	 if (PathVal2 == SET_PATH_B2)
	 	  {
	 		 /*Set Relay di ingresso a 2 stati*/
	         HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_SET);  //K_B_E = 0


	          /*Da inserire un delay per attendere lo switch del relay prima di fare il check*/

	          /*Lettura Status Relay a 2 stati*/
	          CheckRelayStatusB2(ErrorStatusPath2);


	          /* Set Relay a 3 stati */
	       }




}


static SetValue_t hw_read_get(void)
{

	return g_set_value;
}

static float read_3v3_voltage(void)
{
    return 3.30f;
}

static float read_vin_voltage(void)
{
    return 12.0f;
}

static uint32_t read_pon_counter(void)
{
    return 1;
}

static uint32_t read_eti_minutes(void)
{
    return HAL_GetTick() / 60000u;
}


