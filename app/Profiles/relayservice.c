/*********************************************************************
 * INCLUDES
 */

#include "bcomdef.h"
#include "linkdb.h"
#include "att.h"
#include "gatt.h"
#include "gatt_uuid.h"
#include "gatt_profile_uuid.h"
#include "gattservapp.h"
#include "ha_util.h"

#include "relayservice.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

// Relay Profile Service UUID
#define RELAY_SERV_UUID 0x2F00

// Relay state UUID
#define RELAY_STATE_UUID 0x2F01

#ifndef DISABLE_AUTHENTICATION
#define NOTIFY_AUTH TRUE
#else
#define NOTIFY_AUTH FALSE
#endif

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */
// Relay service
CONST uint8_t relayServiceUUID[HA_UUID_SIZE] =
{
  HA_UUID(RELAY_SERV_UUID)
};

// Relay state characteristic
CONST uint8_t relayStateUUID[HA_UUID_SIZE] =
{
  HA_UUID(RELAY_STATE_UUID)
};

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */

// Relay state change callback.
static relayServiceStateChangeCB_t relayServiceStateChangeCB = NULL;

/*********************************************************************
 * Profile Attributes - variables
 */

// Relay Service attribute.
static CONST gattAttrType_t relayService = { HA_UUID_SIZE, relayServiceUUID };

// Relay state characteristic.
static uint8_t relayStateProps = GATT_PROP_READ | GATT_PROP_WRITE | GATT_PROP_NOTIFY;

// Relay state value
static uint8_t relayState;

// Relay state user description
uint8_t relayStateUserDesc[] = "Relay State";

static gattCharCfg_t *relayStateClientCharCfg;

/*********************************************************************
 * Profile Attributes - Table
 */

static gattAttribute_t relayAttrTbl[] =
{
  // Relay Service
  {
    { ATT_BT_UUID_SIZE, primaryServiceUUID }, /* type */
    GATT_PERMIT_READ,                         /* permissions */
    0,                                        /* handle */
    (uint8_t *)&relayService                  /* pValue */
  },

    // Relay State Declaration
    {
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ,
      0,
      &relayStateProps
    },

      // Relay State Value
      {
        { HA_UUID_SIZE, relayStateUUID },
#ifndef DISABLE_AUTHENTICATION
        GATT_PERMIT_AUTHEN_READ | GATT_PERMIT_AUTHEN_WRITE,
#else
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
#endif
        0,
        &relayState
      },

      // Relay State Client Characteristic Configuration
      {
        { ATT_BT_UUID_SIZE, clientCharCfgUUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        (uint8_t *)&relayStateClientCharCfg
      },

      // Relay State User Description
      {
        { ATT_BT_UUID_SIZE, charUserDescUUID },
        GATT_PERMIT_READ,
        0,
        relayStateUserDesc
      },
};


/*********************************************************************
 * LOCAL FUNCTIONS
 */
static bStatus_t RelayReadAttrCB(uint16_t connHandle, gattAttribute_t *pAttr,
  uint8_t *pValue, uint16_t *pLen, uint16_t offset, uint16_t maxLen,
  uint8_t method);
static bStatus_t RelayWriteAttrCB(uint16_t connHandle, gattAttribute_t *pAttr,
  uint8_t *pValue, uint16_t len, uint16_t offset, uint8_t method);

/*********************************************************************
 * PROFILE CALLBACKS
 */
// Relay Service Callbacks
CONST gattServiceCBs_t relayCBs =
{
  RelayReadAttrCB,  // Read callback function pointer
  RelayWriteAttrCB, // Write callback function pointer
  NULL              // Authorization callback function pointer
};

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      Relay_AddService
 *
 * @brief   Initializes the Relay service by registering
 *          GATT attributes with the GATT server.
 *
 * @return  Success or Failure
 */
bStatus_t Relay_AddService(void)
{
  // Allocate Client Characteristic Configuration table
  relayStateClientCharCfg = (gattCharCfg_t *)ICall_malloc(
    sizeof(gattCharCfg_t) * linkDBNumConns);

  if (relayStateClientCharCfg == NULL)
    return bleMemAllocError;

  // Initialize Client Characteristic Configuration attributes
  GATTServApp_InitCharCfg(INVALID_CONNHANDLE, relayStateClientCharCfg);

  // Register GATT attribute list and CBs with GATT Server App
  return GATTServApp_RegisterService(relayAttrTbl,
    GATT_NUM_ATTRS(relayAttrTbl), GATT_MAX_ENCRYPT_KEY_SIZE, &relayCBs);
}

/*********************************************************************
 * @fn      Relay_SetParameter
 *
 * @brief   Set a Relay Service parameter.
 *
 * @param   param - Profile parameter ID
 * @param   len - length of data to right
 * @param   value - pointer to data to write.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate
 *          data type (example: data type of uint16 will be cast to
 *          uint16 pointer).
 *
 * @return  bStatus_t
 */
bStatus_t Relay_SetParameter(uint8_t param, uint8_t len, void *value)
{
  bStatus_t ret = SUCCESS;

  switch (param)
  {
    case RELAY_PARAM_STATE:
      if (len == sizeof(uint8_t))
      {
        uint8_t state = *((uint8_t*)value);

        if (state != relayState)
        {
          relayState = state;
          ret = GATTServApp_ProcessCharCfg(relayStateClientCharCfg,
            &relayState, NOTIFY_AUTH, relayAttrTbl,
            GATT_NUM_ATTRS(relayAttrTbl), INVALID_TASK_ID,
            RelayReadAttrCB);
        }
      }
      else
        ret = bleInvalidRange;
      break;

    default:
      ret = INVALIDPARAMETER;
      break;
  }

  return ret;
}

/*********************************************************************
 * @fn      Relay_GetParameter
 *
 * @brief   Get a Relay parameter.
 *
 * @param   param - Profile parameter ID
 * @param   value - pointer to data to get.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate
 *          data type (example: data type of uint16 will be cast to
 *          uint16 pointer).
 *
 * @return  bStatus_t
 */
bStatus_t Relay_GetParameter(uint8_t param, void *value)
{
  bStatus_t ret = SUCCESS;

  switch (param)
  {
    case RELAY_PARAM_STATE:
      *((uint8_t*)value) = relayState;
      break;

    default:
      ret = INVALIDPARAMETER;
      break;
  }

  return (ret);
}

/*********************************************************************
 * @fn          RelayReadAttrCB
 *
 * @brief       Read an attribute.
 *
 * @param       connHandle - connection message was received on
 * @param       pAttr - pointer to attribute
 * @param       pValue - pointer to data to be read
 * @param       pLen - length of data to be read
 * @param       offset - offset of the first octet to be read
 * @param       maxLen - maximum length of data to be read
 * @param       method - type of read message
 *
 * @return      SUCCESS, blePending or Failure
 */
static bStatus_t RelayReadAttrCB(uint16_t connHandle, gattAttribute_t *pAttr,
  uint8_t *pValue, uint16_t *pLen, uint16_t offset, uint16_t maxLen,
  uint8_t method)
{
  uint16_t uuid;
  bStatus_t status = SUCCESS;

  // Make sure it's not a blob operation (no attributes in the profile are long)
  if (offset > 0)
    return ATT_ERR_ATTR_NOT_LONG;

  if (utilExtractUuid16(pAttr, &uuid) == FAILURE)
  {
    // Invalid handle
    *pLen = 0;
    return ATT_ERR_INVALID_HANDLE;
  }

  if (uuid == RELAY_STATE_UUID)
  {
    *pLen = 1;
    pValue[0] = relayState;
  }
  else
    status = ATT_ERR_ATTR_NOT_FOUND;

  return (status);
}

/*********************************************************************
 * @fn      RelayWriteAttrCB
 *
 * @brief   Validate attribute data prior to a write operation
 *
 * @param   connHandle - connection message was received on
 * @param   pAttr - pointer to attribute
 * @param   pValue - pointer to data to be written
 * @param   len - length of data
 * @param   offset - offset of the first octet to be written
 * @param   method - type of write message
 *
 * @return  SUCCESS, blePending or Failure
 */
static bStatus_t RelayWriteAttrCB(uint16_t connHandle, gattAttribute_t *pAttr,
  uint8_t *pValue, uint16_t len, uint16_t offset, uint8_t method)
{
  bStatus_t status = SUCCESS;
  uint16_t uuid;

  if (utilExtractUuid16(pAttr, &uuid) == FAILURE)
  {
    // Invalid handle
    return ATT_ERR_INVALID_HANDLE;
  }

  // If attribute permissions require authorization to write, return error
  if (gattPermitAuthorWrite(pAttr->permissions))
  {
    // Insufficient authorization
    return ATT_ERR_INSUFFICIENT_AUTHOR;
  }

  // Make sure it's not a blob operation (no attributes in the profile are long)
  if (offset > 0)
    return ATT_ERR_ATTR_NOT_LONG;

  switch (uuid)
  {
    case RELAY_STATE_UUID:
      //Validate the value
      if ( len != 1 )
        status = ATT_ERR_INVALID_VALUE_SIZE;

      if (status == SUCCESS)
      {
        uint8 *pCurValue = (uint8 *)pAttr->pValue;

        /* Change the relay state, only if the logical value changed */
        if (*pCurValue != !!(pValue[0]))
        {
          if (relayServiceStateChangeCB)
            relayServiceStateChangeCB();
          *pCurValue = !!pValue[0]; // Save the value
        }
      }
      break;

    case GATT_CLIENT_CHAR_CFG_UUID:
      status = GATTServApp_ProcessCCCWriteReq(connHandle, pAttr, pValue, len,
        offset, GATT_CLIENT_CFG_NOTIFY);
      break;

    default:
      status = ATT_ERR_ATTR_NOT_FOUND;
      break;
  }

  return status;
}

/*********************************************************************
 * @fn      Relay_Setup
 *
 * @brief   Set up application callback functions.
 *
 * @param   cCB - relay state change callback
 *
 * @return  none.
 */
void Relay_Setup(relayServiceStateChangeCB_t cCB)
{
  relayServiceStateChangeCB = cCB;
}

/*********************************************************************
*********************************************************************/
