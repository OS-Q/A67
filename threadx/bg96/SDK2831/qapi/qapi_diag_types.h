/** @file qapi_diag_types.h
*/

#ifndef QAPI_DIAG_TYPES_H
#define QAPI_DIAG_TYPES_H 

/*
Copyright (c) 2017 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.
*/

/*===========================================================================

                            Edit History

  $Header: //components/rel/core.tx/1.0.c3/api/services/qapi_diag_types.h#1 $

when       who     what, where, why
--------   ---     ----------------------------------------------------------
05/19/17   gn      Header file cleanup
04/21/17   gn      Added Doxgyen comments and markup.
03/24/17   gn      Changes to support DIAG delayed reponses on User space
02/24/17   gn      Changes to support DIAG QAPIs with DAM framework
===========================================================================*/


/* 
	Internal helper MACROS 
*/

/* 
	Macros to extract Nth argument of __VA_ARGS__ 
*/
#define GET_ARG(N, ...) GET_ARG_##N(__VA_ARGS__)
#define GET_ARG_1(_1, ...) _1
#define GET_ARG_2(_1, _2, ...) _2
#define GET_ARG_3(_1, _2, _3, ...) _3
#define GET_ARG_4(_1, _2, _3, _4,...) _4
#define GET_ARG_5(_1, _2, _3, _4, _5, ...) _5
#define GET_ARG_6(_1, _2, _3, _4, _5, _6, ...) _6
#define GET_ARG_7(_1, _2, _3, _4, _5, _6, _7, ...) _7
#define GET_ARG_8(_1, _2, _3, _4, _5, _6, _7, _8, ...) _8
#define GET_ARG_9(_1, _2, _3, _4, _5, _6, _7, _8, _9, ...) _9

/* 
	This is just a collection of 0s used as a padding for __VA_ARGS__ to extract 
	the required argument 
*/
#define PAD() \
          0, 0, 0, 0, 0, 0, 0, 0, 0, 0

/* 
	This is just a collection of sequence of numbers to determine the number of
	arguments present in a format string 
*/
#define MSG_RSEQ_N() \
          9, 8, 7, 6, 5, 4, 3, 2, 1, 0

/* 
	This macro just passes the arguments to MSG_ARG_N 
*/
#define MSG_NARG_(...) \
         MSG_ARG_N( __VA_ARGS__ )

/*
	This is used to actually determine the number of arguments in a format string.
	The format string and sequence of numbers is already passed to MSG_ARG_N (from MSG_NARG)
	and the 11th position below returns the number of arguments.
*/
#define MSG_ARG_N( _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, N, ...) \
         N
		 
/*
	Combine the format string and the sequence of numbers so that all the arguments are
	passed to MSG_ARG_N macro to return the number of arguments.
*/
#define MSG_NARG(...) \
          MSG_NARG_( __VA_ARGS__,MSG_RSEQ_N())
          
#ifdef  QAPI_TXM_MODULE     // USER_MODE_DEFS

/** @addtogroup qapi_diag
@{ */

/** 
 * @brief Diag subsystem header structure for subsystem commands and responses.
 */
typedef struct
{
  uint8_t command_code;
  /**< Command code. */
  uint8_t subsys_id;
  /**< Subsystem ID. */
  uint16_t subsys_cmd_code;
  /**< Subsystem command code. */
}
qapi_diagpkt_subsys_hdr_type_t;

/** 
 * @brief Diag subsystem v2 header structure for delayed subsystem responses.
 */
typedef struct
{
  uint8_t command_code;
  /**< Command code. */
  uint8_t subsys_id;
  /**< Subsystem ID. */
  uint16_t subsys_cmd_code;
  /**< Subsystem command code. */
  uint32_t status;
  /**< Can be used by clients for any error codes. */
  uint16_t delayed_rsp_id;
  /**< Unique ID for a set of delayed responses generated by a single command. */
  uint16_t rsp_cnt;
  /**< Increment this count for every response to track the sequence of responses. */
}
qapi_diagpkt_subsys_hdr_type_v2_t;

/*!
Constructs a msg_const_type from the SSID, mask and format-string
*/
#define XX_MSG_CONST(xx_ss_id, xx_ss_mask, xx_fmt) \
    static const msg_const_type xx_msg_const = { \
	  {__LINE__, (xx_ss_id), (xx_ss_mask)}, (xx_fmt), msg_file}

#define PACK(x)       __packed x

/*! 
@ingroup packet_service
   An array of diagpkt_user_table_entry_type is created by the client and registered with 
   diagnostics packet service. It must be declared 'const' (preferrably 'static const').  
   The function is called when an inbound packet matches subsystem ID and
   is within the command code range specified in the table.
*/ 
  typedef struct
  {
    uint16_t cmd_code_lo; /*!<  Minimum Command code value  */
    uint16_t cmd_code_hi; /*!< Maximum Command code value   */
    PACK(void *) (*func_ptr) (PACK(void *) req_pkt_ptr, uint16_t pkt_len); /*!< Pointer to Function to handle the packet  
                                                                          when command code is in the range of cmd_code_lo,
                                                                          cmd_code_hi  */
    uint32_t (*user_func_ptr) (uint32_t pkt_len); /*!< Pointer to user space client's handler when command code is in the range 
												of cmd_code_lo and cmd_code_hi	*/
  }
  diagpkt_user_table_entry_type;

/*!
@ingroup packet_service
   Structure for the diagnostics packet service user space table to hold the clients buffer details when clients
   registers with the diagnostics packet services

@note This is an internal structure used by the macros 
*/
  typedef struct
  { 
	uint32_t *inbuf;     /*!< User space client's input buffer address */
	uint32_t inbuf_len;  /*!< User space client's input buffer length */
	uint32_t *outbuf;    /*!< User space client's output buffer address */
	uint32_t outbuf_len; /*!< User space client's output buffer length */
  } diagpkt_user_space_table_type;

/*!
@ingroup packet_service
   Structure for the diagnostics packet service master table to hold the clients table entries when clients
   registers with the diagnostics packet services

@note This is an internal structure used by the macros 
*/
  typedef struct
  { 
    uint16_t delay_flag;  /*!< Delay Flag 0 means no delay and 1 means with delay */
    uint16_t cmd_code;    /*!< Commad Code (defined in diagcmd.h)*/
    uint16_t subsysid;      /*!< Subsystem ID if Diagnostics Subsytem Dispatch is used by client 
                         else it would be DIAGPKT_NO_SUBSYS_ID */
    uint16_t count;         /*!< Number of entries in the table*/ 
    uint16_t proc_id;     /*!< Processor ID */
    uint16_t is_fwded;    /*!< 0 means has not been fwded to Master Diag; 1 means has been fwded */
    int port;           /*!< Port to remote proc to route recieved CMD */
    const diagpkt_user_table_entry_type *user_table; /**< Entry to Client's Packet Dispatch Table  */
  } diagpkt_master_table_type;

/*!
@ingroup diag_message_service
  This structure is stored in ROM and is copied blindly by the phone.   
  The values for the fields of this structure are known at compile time. 
  So this is to be defined as a "static const" in the MACRO, so it ends up
  being defined and initialized at compile time for each and every message 
  in the software. This minimizes the amount of work to do during run time.

  So this structure is to be used in the "caller's" context. "Caller" is the
  client of the Message Services.
*/  
typedef struct
{
  uint16_t line;			/*!< Line number in source file */
  uint16_t ss_id;			/*!< Subsystem ID               */
  uint32_t ss_mask;		/*!< Subsystem Mask             */
}
msg_desc_type;

/*!
@ingroup diag_message_service
  All constant information stored for a message.

  The values for the fields of this structure are known at compile time. 
  So this is to be defined as a "static " in the MACRO, so it ends up
  being defined and initialized at compile time for each and every message 
  in the software. This minimizes the amount of work to do during run time.

  So this structure is to be used in the "caller's" context. "Caller" is the
  client of the Message Services.
*/
typedef struct
{
  msg_desc_type desc;   /*!< contains mask, line number and subsystem-id. defined in msg_pkt_defs.h */
  const char *fmt;      /*!< Printf style format string */
  const char *fname;    /*!< Pointer to source file name */
}
msg_const_type;

/** @} */ /* end_addtogroup qapi_diag */
#endif

#endif /* QAPI_DIAG_CODES_H */
