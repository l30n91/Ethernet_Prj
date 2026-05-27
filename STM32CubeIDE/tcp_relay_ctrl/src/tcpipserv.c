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
    SET_PATH_A2
} SetPathValue_t;




static SetPathValue_t g_set_value = SET_VAL_0;
static SetPathValue_t g_get_value = SET_VAL_0;

static void tcp_server_thread(void *arg);
static void process_command(const char *cmd, char *reply, size_t reply_size);

static int parse_path_value(const char *s, SetPathValue_t *value);
static int parse_att_value(const char *s, SetValue_t *value);
static const char *set_value_to_string(SetValue_t value);

static void hw_apply_set(SetValue_t value);
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

        const char *attenuation = path +3;

        const char *post_attenuation = attenuation + 3;

        SetValue_t requestedAtt_value;
        SetPathValue_t requestedPath_value;

        /* set_a1_30, set_a1_z*/
        //check_path -> A1/A2
        //check_attenuation -> 30,Z ecc


        /*Controllo preliminare se sono stati digitati gli spazi nel posto giusto, il comando sarà set_Ax_Z*/
         if( ((*path != '\r') &&
         	(*path  != '\t') &&
 			(*path  != '\n') &&
 			(*path  !=  ' ') &&
			(*path  !=  '\0'))||

			((*attenuation != '\r') &&
			(*attenuation != '\t') &&
		    (*attenuation != '\n') &&
			(*attenuation !=  ' ') &&
			(*attenuation !=  '\0'))
		)

        {
        		snprintf(reply, reply_size, "err:val incorrect spaces\r\n");
        		return;
        }



        while (*path == ' ' || *path == '\t') /* se prima di path trovi spazi ecc... vai avanti*/
        {
            path++; //path=cmd+4
        }

        while (*attenuation == ' ' || *attenuation == '\t') /* se prima di attenuation trovi spazi ecc... vai avanti*/
        {
            attenuation++; //attenuation=path+4
        }




        if (parse_path_value(path, &requestedPath_value) == 0 || parse_att_value(attenuation, &requestedAtt_value) == 0) //A1 OR A2, ritorna 1 se tutto ok valori riconosciuti
        {
            snprintf(reply, reply_size, "err:val\r\n");
            return;
        }








        /* applicazione hardware del comando ricevuto*/
        g_set_value = requestedPath_value;
        hw_apply_set(requestedPath_value);

        osDelay(10);

        g_get_value = hw_read_get();

        if (g_get_value == g_set_value)
        {
            snprintf(reply, reply_size, "ok:set\r\n");
        }
        else
        {
            snprintf(reply, reply_size, "err:set\r\n");
        }

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

/*
 * TODO: qui bisogna mappare i valori sui GPIO/relè reali.
 */
static void hw_apply_set(SetValue_t value)
{
    switch (value)
    {
    case SET_VAL_0:
        break;

    case SET_VAL_30:
        break;

    case SET_VAL_130:
        break;

    case SET_VAL_155:
        break;

    case SET_VAL_H:
        break;

    default:
        break;
    }
}

/*
 * TODO: qui devi leggere lo stato reale dai feedback hardware.
 * Per ora restituisce l'ultimo set comandato.
 */
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


