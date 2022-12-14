// Bluetooth 
#define CONFIG_BT_ENABLED                               // Habilitado

// Bluetooth Controller
#define CONFIG_BTDM_CTRL_BLE_MAX_CONN_EFF               0
#define CONFIG_BTDM_CTRL_BR_EDR_MAX_ACL_CONN_EFF        2
#define CONFIG_BTDM_CTRL_BR_EDR_SCO_DATA_PATH_EFF       1
#define CONFIG_BTDM_CTRL_BR_EDR_MAX_SYNC_CONN_EFF       0

// MODEM SLEEP Options
#define CONFIG_BTDM_BLE_SLEEP_CLOCK_ACCURACY_INDEX_EFF  1
#define CONFIG_BTDM_CTRL_PCM_ROLE_EFF                   0
#define CONFIG_BTDM_CTRL_PCM_POLAR_EFF                  0

// Options
#define SPP_TAG             "BT_USR"                    // Identifica na Task.
#define SPP_SERVER_NAME     "BT_SRV"                    // Nome do Servidor.
#define EXAMPLE_DEVICE_NAME "BT_ESP"                    // Nome para busca.

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "nvs.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_bt.h"                                     // Defaults de parametros + sdkconfig.
#include "esp_bt_main.h"                                // Relacionado a pilha do BT.
#include "esp_gap_bt_api.h"                             // Relacionado a funcoes gerais do BT.
#include "esp_bt_device.h"                              // Pega do BDA e ajusta o BDN.
#include "esp_spp_api.h"                                // Relacionado ao SPP.

static const esp_spp_mode_t     esp_spp_mode = ESP_SPP_MODE_CB;
static const esp_spp_sec_t      sec_mask = ESP_SPP_SEC_AUTHENTICATE;
static const esp_spp_role_t     role_slave = ESP_SPP_ROLE_SLAVE;

static char *bda2str(uint8_t *bda, char *str, size_t size)      // BDA=Bluetooth Device Address
{
    if (bda == NULL || str == NULL || size < 18) return NULL;
    uint8_t *p = bda;
    sprintf(str, "%02x:%02x:%02x:%02x:%02x:%02x", p[0], p[1], p[2], p[3], p[4], p[5]);
    return str;
}

static void esp_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
{
    char bda_str[18] = {0};

    switch (event) 
    {
    case ESP_SPP_INIT_EVT:
        if (param->init.status == ESP_SPP_SUCCESS) 
        {
            ESP_LOGI(SPP_TAG, "ESP_SPP_INIT_EVT");
            esp_spp_start_srv(sec_mask, role_slave, 0, SPP_SERVER_NAME);
        } 
        else ESP_LOGE(SPP_TAG, "ESP_SPP_INIT_EVT Status:%d", param->init.status);
        break;
    case ESP_SPP_DISCOVERY_COMP_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_DISCOVERY_COMP_EVT");
        break;
    case ESP_SPP_OPEN_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_OPEN_EVT");
        break;
    case ESP_SPP_CLOSE_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_CLOSE_EVT Status:%d Handle:%d Close_by_Remote:%d", param->close.status, param->close.handle, param->close.async);
        break;
    case ESP_SPP_START_EVT:
        if (param->start.status == ESP_SPP_SUCCESS) 
        {
            ESP_LOGI(SPP_TAG, "ESP_SPP_START_EVT Handle:%d Sec_id:%d SCN:%d", param->start.handle, param->start.sec_id, param->start.scn);
            esp_bt_dev_set_device_name(EXAMPLE_DEVICE_NAME);
            esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
        } else ESP_LOGE(SPP_TAG, "ESP_SPP_START_EVT Status:%d", param->start.status);
        break;
    case ESP_SPP_CL_INIT_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_CL_INIT_EVT");
        break;
    case ESP_SPP_DATA_IND_EVT:
        /*
         * Mostra apenas os dados em que o comprimento dos dados eh menor que 128. Se for imprimir dados
         * e a taxa eh alta, eh altamente recomendavel processa-los em outra tarefa de aplicativo de prioridade
         * mais baixa em vez de diretamente neste retorno de chamada. 
         * Como a impressao leva muito tempo, pode travar a pilha Bluetooth e tambem afetar a taxa de transferencia.
         */
        ESP_LOGI(SPP_TAG, "ESP_SPP_DATA_IND_EVT Len:%d Handle:%d", param->data_ind.len, param->data_ind.handle);
        if (param->data_ind.len < 128) esp_log_buffer_hex("", param->data_ind.data, param->data_ind.len);
        break;
    case ESP_SPP_CONG_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_CONG_EVT");
        break;
    case ESP_SPP_WRITE_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_WRITE_EVT");
        break;
    case ESP_SPP_SRV_OPEN_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_SRV_OPEN_EVT Status:%d Handle:%d, Rem_BDA:[%s]", param->srv_open.status, param->srv_open.handle, bda2str(param->srv_open.rem_bda, bda_str, sizeof(bda_str)));
        // gettimeofday(&time_old, NULL);
        break;
    case ESP_SPP_SRV_STOP_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_SRV_STOP_EVT");
        break;
    case ESP_SPP_UNINIT_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_UNINIT_EVT");
        break;
    default:
        break;
    }
}

void esp_bt_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    char bda_str[18] = {0};

    switch (event) 
    {
    case ESP_BT_GAP_AUTH_CMPL_EVT:
    {
        if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS) ESP_LOGI(SPP_TAG, "Autenticacao com sucesso: %s bda:[%s]", param->auth_cmpl.device_name, bda2str(param->auth_cmpl.bda, bda_str, sizeof(bda_str)));
        else                                                ESP_LOGE(SPP_TAG, "Autenticacao falhou, status:%d", param->auth_cmpl.stat);
        break;
    }
    case ESP_BT_GAP_PIN_REQ_EVT:
    {
        ESP_LOGI(SPP_TAG, "Codigo PIN, 16 Digitos:%d", param->pin_req.min_16_digit);
        if (param->pin_req.min_16_digit) 
        {
            ESP_LOGI(SPP_TAG, "Codigo PIN: 0000 0000 0000 0000");
            esp_bt_pin_code_t pin_code = {0};
            esp_bt_gap_pin_reply(param->pin_req.bda, true, 16, pin_code);
        } 
        else 
        {
            ESP_LOGI(SPP_TAG, "Codigo PIN: 1234");
            esp_bt_pin_code_t pin_code;
            pin_code[0] = '1';
            pin_code[1] = '2';
            pin_code[2] = '3';
            pin_code[3] = '4';
            esp_bt_gap_pin_reply(param->pin_req.bda, true, 4, pin_code);
        }
        break;
    }

#if (CONFIG_BT_SSP_ENABLED == true)
    case ESP_BT_GAP_CFM_REQ_EVT:
        ESP_LOGI(SPP_TAG, "GAP_CFM_REQ_EVT Compare o valor da Chave: %d", param->cfm_req.num_val);
        esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
        break;
    case ESP_BT_GAP_KEY_NOTIF_EVT:
        ESP_LOGI(SPP_TAG, "GAP_KEY_NOTIF_EVT Chave:%d", param->key_notif.passkey);
        break;
    case ESP_BT_GAP_KEY_REQ_EVT:
        ESP_LOGI(SPP_TAG, "GAP_KEY_REQ_EVT Entre com a chave!");
        break;
#endif

    case ESP_BT_GAP_MODE_CHG_EVT:
        ESP_LOGI(SPP_TAG, "GAP_MODE_CHG_EVT Modo:%d BDA:[%s]", param->mode_chg.mode,
                 bda2str(param->mode_chg.bda, bda_str, sizeof(bda_str)));
        break;

    default: 
    {
        ESP_LOGI(SPP_TAG, "Evento: %d", event);
        break;
    }
    }
    return;
}

void app_main(void)
{
    char bda_str[18] = {0};
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) 
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    if ((ret = esp_bt_controller_init(&bt_cfg)) != ESP_OK) 
    {
        ESP_LOGE(SPP_TAG, "%s inicializacao do controlador falhou: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT)) != ESP_OK) 
    {
        ESP_LOGE(SPP_TAG, "%s habilitacao do controlador falhou: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_bluedroid_init()) != ESP_OK) 
    {
        ESP_LOGE(SPP_TAG, "%s inicializacao do Bluedroid falhou: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_bluedroid_enable()) != ESP_OK) 
    {
        ESP_LOGE(SPP_TAG, "%s habilitacao do Bluedroid falhou: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_bt_gap_register_callback(esp_bt_gap_cb)) != ESP_OK) 
    {
        ESP_LOGE(SPP_TAG, "%s registro do GAP falhou: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_spp_register_callback(esp_spp_cb)) != ESP_OK) 
    {
        ESP_LOGE(SPP_TAG, "%s registro do SPP falhou: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_spp_init(esp_spp_mode)) != ESP_OK) 
    {
        ESP_LOGE(SPP_TAG, "%s inicializacao do SPP falhou: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

#if (CONFIG_BT_SSP_ENABLED == true)
    // Defina os parametros padrao para o Emparelhamento Simples Seguro.
    esp_bt_sp_param_t param_type = ESP_BT_SP_IOCAP_MODE;
    esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_IO;
    esp_bt_gap_set_security_param(param_type, &iocap, sizeof(uint8_t));
#endif

    /*
     Defina os parametros padrao para o emparelhamento herdado.
     Use a variavel do PIN, insira o codigo PIN ao emparelhar.
     */
    esp_bt_pin_type_t pin_type = ESP_BT_PIN_TYPE_VARIABLE;
    esp_bt_pin_code_t pin_code;
    esp_bt_gap_set_pin(pin_type, 0, pin_code);

    ESP_LOGI(SPP_TAG, "Meu endereco:[%s]", bda2str((uint8_t *)esp_bt_dev_get_address(), bda_str, sizeof(bda_str)));
}
