/*!
    \file    usbh_msc_bbb.c
    \brief   USB MSC BBB protocol related functions

    \version 2020-08-01, V3.0.0, firmware for GD32F4xx
    \version 2022-03-09, V3.1.0, firmware for GD32F4xx
*/

/*
    Copyright (c) 2022, GigaDevice Semiconductor Inc.

    Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this 
       list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice, 
       this list of conditions and the following disclaimer in the documentation 
       and/or other materials provided with the distribution.
    3. Neither the name of the copyright holder nor the names of its contributors 
       may be used to endorse or promote products derived from this software without 
       specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT 
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
OF SUCH DAMAGE.
*/

#include "usbh_pipe.h"
#include "usbh_msc_core.h"
#include "usbh_msc_scsi.h"
#include "usbh_msc_bbb.h"
#include "usbh_transc.h"
#include "drv_usbh_int.h"

/*!
    \brief      initialize the mass storage parameters
    \param[in]  uhost: pointer to USB host handler
    \param[out] none
    \retval     none
*/
void usbh_msc_bbb_init (usbh_host *uhost)
{
    usbh_msc_handler *msc = (usbh_msc_handler *)uhost->active_class->class_data;

    msc->bot.cbw.field.dCBWSignature = BBB_CBW_SIGNATURE;
    msc->bot.cbw.field.dCBWTag = USBH_MSC_BBB_CBW_TAG;
    msc->bot.state = BBB_SEND_CBW;
    msc->bot.cmd_state = BBB_CMD_SEND;
}

/*!
    \brief      manage the different states of BOT transfer and updates the status to upper layer
    \param[in]  uhost: pointer to usb host handler
    \param[in]  lun: logic unit number
    \param[out] none
    \retval     operation status
*/
usbh_status usbh_msc_bbb_process (usbh_host *uhost, uint8_t lun)
{
    bbb_csw_status csw_status = BBB_CSW_CMD_FAILED;
    usbh_status status = USBH_BUSY;
    usbh_status error = USBH_BUSY;
    usb_urb_state urb_status = URB_IDLE;
    usbh_msc_handler *msc = (usbh_msc_handler *)uhost->active_class->class_data;

    switch (msc->bot.state) {
    case BBB_SEND_CBW:
        msc->bot.cbw.field.bCBWLUN = lun;
        msc->bot.state = BBB_SEND_CBW_WAIT;
        /* send CBW */
        usbh_data_send (uhost->data,
                        msc->bot.cbw.CBWArray, 
                        msc->pipe_out, 
                        BBB_CBW_LENGTH);
        break;

    case BBB_SEND_CBW_WAIT:
        urb_status = usbh_urbstate_get(uhost->data, msc->pipe_out);

        if (URB_DONE == urb_status) {
            if (0U != msc->bot.cbw.field.dCBWDataTransferLength) {
                if (USB_TRX_IN == (msc->bot.cbw.field.bmCBWFlags & USB_TRX_MASK)) {
                    msc->bot.state = BBB_DATA_IN;
                } else {
                    msc->bot.state = BBB_DATA_OUT;
                }
            } else {
                msc->bot.state = BBB_RECEIVE_CSW;
            }

        } else if (URB_NOTREADY == urb_status) {
            msc->bot.state = BBB_SEND_CBW;
        } else {
            if (URB_STALL == urb_status) {
                msc->bot.state = BBB_ERROR_OUT;
            }
        }
        break;

    case BBB_DATA_IN:
        /* 一次提交尽量多的数据包（PCNT>1 多包连续接收，与 STM32 可行版
         * USBH_MSC_Read10 一致）。原实现每次只收 ep_size_in(64B) 就重新
         * 提交一个 URB，8 扇区读要拆 64 个单包 URB，在「最后一个数据包→
         * CSW」切换处控制器容易把 CSW 当数据吞掉，导致 CSW 槽收到 ZLP
         * （CSW 全零）→ PHASE_ERROR。多包 URB 让该切换每命令只发生一次。 */
        {
            uint32_t max_len = (uint32_t)HC_MAX_PACKET_COUNT * (uint32_t)msc->ep_size_in;
            uint32_t remaining = msc->bot.cbw.field.dCBWDataTransferLength;
            uint16_t xfer_len;

            if (max_len > 0xFFF0U) {
                max_len = 0xFFF0U;
            }

            xfer_len = (uint16_t)((remaining > max_len) ? max_len : remaining);

            usbh_data_recev (uhost->data, 
                             msc->bot.pbuf, 
                             msc->pipe_in, 
                             xfer_len);
        }

        msc->bot.state = BBB_DATA_IN_WAIT;
        break;

    case BBB_DATA_IN_WAIT:
        urb_status = usbh_urbstate_get(uhost->data, msc->pipe_in);

        /* BOT DATA IN stage */
        if (URB_DONE == urb_status) {
            uint32_t got = usbh_xfercount_get(uhost->data, msc->pipe_in);

            /* ── 诊断：打印数据阶段每次 URB 收到的字节数 ── */
            extern void USB_Upgrade_Printf(const char *fmt, ...);
            USB_Upgrade_Printf("[DATAIN] got=%lu rem=%lu\r\n",
                               (unsigned long)got,
                               (unsigned long)msc->bot.cbw.field.dCBWDataTransferLength);

            if ((0U == got) || (msc->bot.cbw.field.dCBWDataTransferLength <= got)) {
                /* 收到 0 字节（设备提前结束）或收完剩余数据：进入 CSW 阶段。
                 * ⚠ 最后一个数据块【不】推进 pbuf —— Inquire/ReadCapacity/
                 * RequestSense 等短命令的 BOT 完成后，上层仍从 msc->bot.pbuf
                 * 起点读结果；若推进了 pbuf，结果会被读成全 0，导致
                 * block_size=0 → 后续 Read10 的 dCBWDataTransferLength=0
                 * → 设备返回 CSW_FAILED。 */
                msc->bot.cbw.field.dCBWDataTransferLength = 0U;
                msc->bot.state = BBB_RECEIVE_CSW;
            } else {
                msc->bot.pbuf += got;
                msc->bot.cbw.field.dCBWDataTransferLength -= got;
                msc->bot.state = BBB_DATA_IN;
            }
        } else if(URB_STALL == urb_status) {
            /* this is data stage stall condition */
            msc->bot.state = BBB_ERROR_IN;
        } else {
            /* no operation */
        }
        break;

    case BBB_DATA_OUT:
        usbh_data_send (uhost->data,
                        msc->bot.pbuf, 
                        msc->pipe_out, 
                        msc->ep_size_out);

        msc->bot.state = BBB_DATA_OUT_WAIT;
        break;

    case BBB_DATA_OUT_WAIT:
        /* BOT DATA OUT stage */
        urb_status = usbh_urbstate_get(uhost->data, msc->pipe_out);
        if (URB_DONE == urb_status) {
            if (msc->bot.cbw.field.dCBWDataTransferLength > msc->ep_size_out) {
                msc->bot.pbuf += msc->ep_size_out;
                msc->bot.cbw.field.dCBWDataTransferLength -= msc->ep_size_out;
            }  else {
                msc->bot.cbw.field.dCBWDataTransferLength = 0; /* reset this value and keep in same state */
            }

            if (msc->bot.cbw.field.dCBWDataTransferLength > 0) {
                usbh_data_send (uhost->data,
                                msc->bot.pbuf, 
                                msc->pipe_out, 
                                msc->ep_size_out);
            } else {
                msc->bot.state = BBB_RECEIVE_CSW;
            }
        } else if (URB_NOTREADY == urb_status) {
            msc->bot.state = BBB_DATA_OUT;
        } else if (URB_STALL == urb_status) {
            msc->bot.state = BBB_ERROR_OUT;
        } else {
            /* no operation */
        }
        break;

    case BBB_RECEIVE_CSW:
        /* BOT CSW stage */
        usbh_data_recev (uhost->data,
                         msc->bot.csw.CSWArray, 
                         msc->pipe_in, 
                         BBB_CSW_LENGTH);

        msc->bot.state = BBB_RECEIVE_CSW_WAIT;
        break;

    case BBB_RECEIVE_CSW_WAIT:
        urb_status = usbh_urbstate_get(uhost->data, msc->pipe_in);

        /* decode CSW */
        if (URB_DONE == urb_status) {
            msc->bot.state = BBB_SEND_CBW;
            msc->bot.cmd_state = BBB_CMD_SEND;

            csw_status = usbh_msc_csw_decode(uhost);
            if (BBB_CSW_CMD_PASSED == csw_status) {
                status = USBH_OK;
            } else {
                status = USBH_FAIL;
            }
        } else if (URB_STALL == urb_status) {
            msc->bot.state = BBB_ERROR_IN;
        } else {
            /* no operation */
        }
        break;

    case BBB_ERROR_IN: 
        error = usbh_msc_bbb_abort(uhost, USBH_MSC_DIR_IN);

        if (USBH_OK == error) {
            msc->bot.state = BBB_RECEIVE_CSW;
        } else if (USBH_UNRECOVERED_ERROR == status) {
            /* this means that there is a stall error limit, do reset recovery */
            msc->bot.state = BBB_UNRECOVERED_ERROR;
        } else {
            /* no operation */
        }
        break;

    case BBB_ERROR_OUT: 
        status = usbh_msc_bbb_abort (uhost, USBH_MSC_DIR_OUT);

        if (USBH_OK == status) {
            uint8_t toggle = usbh_pipe_toggle_get(uhost->data, msc->pipe_out);
            usbh_pipe_toggle_set(uhost->data, msc->pipe_out, 1U - toggle);
            usbh_pipe_toggle_set(uhost->data, msc->pipe_in, 0U);
            msc->bot.state = BBB_ERROR_IN;
        } else {
            if (USBH_UNRECOVERED_ERROR == status) {
                msc->bot.state = BBB_UNRECOVERED_ERROR;
            }
        }
        break;

    case BBB_UNRECOVERED_ERROR:
        status = usbh_msc_bbb_reset(uhost);
        if (USBH_OK == status) {
            msc->bot.state = BBB_SEND_CBW;
        }
        break;

    default:
        break;
    }

    return status;
}

/*!
    \brief      manages the different error handling for stall
    \param[in]  uhost: pointer to USB host handler
    \param[in]  direction: data IN or OUT
    \param[out] none
    \retval     operation status
*/
usbh_status usbh_msc_bbb_abort (usbh_host *uhost, uint8_t direction)
{
    usbh_status status = USBH_BUSY;
    usbh_msc_handler *msc = (usbh_msc_handler *)uhost->active_class->class_data;

    switch (direction) {
    case USBH_MSC_DIR_IN :
        /* send clrfeture command on bulk IN endpoint */
        status = usbh_clrfeature(uhost,
                                 msc->ep_in,
                                 msc->pipe_in);
        break;

    case USBH_MSC_DIR_OUT :
        /*send clrfeature command on bulk OUT endpoint */
        status = usbh_clrfeature(uhost,
                                 msc->ep_out,
                                 msc->pipe_out);
        break;

    default:
        break;
    }

    return status;
}

/*!
    \brief      reset MSC bot transfer
    \param[in]  uhost: pointer to USB host handler
    \param[out] none
    \retval     operation status
*/
usbh_status usbh_msc_bbb_reset (usbh_host *uhost)
{
    usbh_status status = USBH_BUSY;

    if (CTL_IDLE == uhost->control.ctl_state) {
        uhost->control.setup.req = (usb_req) {
            .bmRequestType = USB_TRX_OUT | USB_REQTYPE_CLASS | USB_RECPTYPE_ITF,
            .bRequest      = BBB_RESET,
            .wValue        = 0U,
            .wIndex        = 0U,
            .wLength       = 0U
        };

        usbh_ctlstate_config (uhost, NULL, 0U);
    } 

    status = usbh_ctl_handler (uhost);

    return status;
}

/*!
    \brief      decode the CSW received by the device and updates the same to upper layer
    \param[in]  uhost: pointer to USB host
    \param[out] none
    \retval     on success USBH_MSC_OK, on failure USBH_MSC_FAIL
    \notes
          Refer to USB Mass-Storage Class: BOT (www.usb.org)
          6.3.1 Valid CSW Conditions :
          The host shall consider the CSW valid when:
          1. dCSWSignature is equal to 53425355h
          2. the CSW is 13 (Dh) bytes in length,
          3. dCSWTag matches the dCBWTag from the corresponding CBW.
*/
bbb_csw_status usbh_msc_csw_decode (usbh_host *uhost)
{
    bbb_csw_status status = BBB_CSW_CMD_FAILED;
    usbh_msc_handler *msc = (usbh_msc_handler *)uhost->active_class->class_data;

    /* checking if the transfer length is different than 13 */
    if (BBB_CSW_LENGTH != usbh_xfercount_get (uhost->data, msc->pipe_in)) {
        status = BBB_CSW_PHASE_ERROR;
    } else {
        /* CSW length is correct */

        /* check validity of the CSW Signature and CSWStatus */
        if (BBB_CSW_SIGNATURE == msc->bot.csw.field.dCSWSignature) {
            /* check condition 1. dCSWSignature is equal to 53425355h */
            if (msc->bot.csw.field.dCSWTag == msc->bot.cbw.field.dCBWTag) {
                /* check condition 3. dCSWTag matches the dCBWTag from the corresponding CBW */
                if (0U == msc->bot.csw.field.bCSWStatus) {
                    status = BBB_CSW_CMD_PASSED;
                } else if (1U == msc->bot.csw.field.bCSWStatus) {
                    status = BBB_CSW_CMD_FAILED;
                } else if (2U == msc->bot.csw.field.bCSWStatus) {
                    status = BBB_CSW_PHASE_ERROR;
                } else {
                    /* no operation */
                }
            }
        } else {
            /* If the CSW signature is not valid, we shall return the phase error to
               upper layers for reset recovery */
            status = BBB_CSW_PHASE_ERROR;
        }
    }

    /* ── 诊断：打印每条 BOT 命令的 CSW 解码结果（含 MSC_INIT 阶段的 INQUIRY/TUR/READ_CAP）── */
    extern void USB_Upgrade_Printf(const char *fmt, ...);
    USB_Upgrade_Printf("[CSW] len=%lu sig=%08X tag=%08X st=%u res=%lu -> %d\r\n",
                       (unsigned long)usbh_xfercount_get(uhost->data, msc->pipe_in),
                       (unsigned long)msc->bot.csw.field.dCSWSignature,
                       (unsigned long)msc->bot.csw.field.dCSWTag,
                       (unsigned)msc->bot.csw.field.bCSWStatus,
                       (unsigned long)msc->bot.csw.field.dCSWDataResidue,
                       (int)status);

    return status;
}
