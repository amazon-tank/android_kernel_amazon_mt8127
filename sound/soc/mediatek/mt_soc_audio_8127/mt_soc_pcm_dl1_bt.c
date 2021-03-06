/* Copyright (c) 2011-2013, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/*****************************************************************************
 *                     C O M P I L E R   F L A G S
 *****************************************************************************/


/*****************************************************************************
 *                E X T E R N A L   R E F E R E N C E S
 *****************************************************************************/

#include <linux/dma-mapping.h>
#include "mt_soc_afe_common.h"
#include "mt_soc_afe_def.h"
#include "mt_soc_afe_reg.h"
#include "mt_soc_afe_clk.h"
#include "mt_soc_afe_control.h"
#include "mt_soc_digital_type.h"
#include "mt_soc_pcm_common.h"

static struct AFE_MEM_CONTROL_T *pdl1btMemControl;

static DEFINE_SPINLOCK(auddrv_DL1BTCtl_lock);

static struct device *mDev;

/*
 *    function implementation
 */

static int mtk_dl1bt_probe(struct platform_device *pdev);
static int mtk_Dl1Bt_close(struct snd_pcm_substream *substream);
static int mtk_asoc_Dl1Bt_pcm_new(struct snd_soc_pcm_runtime *rtd);
static int mtk_asoc_dl1bt_probe(struct snd_soc_platform *platform);
#ifdef BTDAI_MODEM_INTF

static struct AudioDigitalPCM extmd_pcm_tx = {
	.mSyncOutInv = Soc_Aud_INV_SYNC_NO_INVERSE,
	.mBclkOutInv = Soc_Aud_INV_BCK_NO_INVERSE,
	.mSyncInInv = Soc_Aud_INV_SYNC_NO_INVERSE,
	.mBclkInInv = Soc_Aud_INV_BCK_NO_INVERSE,
	.mTxLchRepeatSel = Soc_Aud_TX_LCH_RPT_TX_LCH_NO_REPEAT,
	.mVbt16kModeSel = Soc_Aud_VBT_16K_MODE_VBT_16K_MODE_DISABLE,
	.mExtModemSel = Soc_Aud_EXT_MODEM_MODEM_2_USE_EXTERNAL_MODEM,
	.mExtendBckSyncLength = 0,
	.mExtendBckSyncTypeSel = Soc_Aud_PCM_SYNC_TYPE_BCK_CYCLE_SYNC,
	.mSingelMicSel = Soc_Aud_BT_MODE_DUAL_MIC_ON_TX,
	.mAsyncFifoSel = Soc_Aud_BYPASS_SRC_SLAVE_USE_ASRC,
	.mSlaveModeSel = Soc_Aud_PCM_CLOCK_SOURCE_SALVE_MODE,
	.mPcmWordLength = Soc_Aud_PCM_WLEN_LEN_PCM_16BIT,
	.mPcmModeWidebandSel = Soc_Aud_PCM_MODE_PCM_MODE_8K,
	.mPcmFormat = Soc_Aud_PCM_FMT_PCM_MODE_B,
	.mModemPcmOn = false,
};
/*
static const struct snd_kcontrol_new audio_snd_dl1_bt_controls[] = {

	SOC_ENUM_EXT("Audio_pcm1_SineGen_Switch", Audio_i2s0_Enum[0], Audio_i2s0_SideGen_Get,
		     Audio_i2s0_SideGen_Set),
	SOC_ENUM_EXT("Audio_i2s0_hd_Switch", Audio_i2s0_Enum[1], Audio_i2s0_hdoutput_Get,
		     Audio_i2s0_hdoutput_Set),
	SOC_ENUM_EXT("Audio_ExtCodec_EchoRef_Switch", Audio_i2s0_Enum[2],
		     Audio_i2s0_ExtCodec_EchoRef_Get, Audio_i2s0_ExtCodec_EchoRef_Set),
};
*/
#endif

static struct snd_pcm_hardware mtk_dl1bt_pcm_hardware = {

	.info = (SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_INTERLEAVED |
		SNDRV_PCM_INFO_RESUME | SNDRV_PCM_INFO_MMAP_VALID),
	.formats = SND_SOC_STD_MT_FMTS,
	.rates = SOC_NORMAL_USE_RATE,
	.rate_min = SOC_NORMAL_USE_RATE_MIN,
	.rate_max = SOC_NORMAL_USE_RATE_MAX,
	.channels_min = SOC_NORMAL_USE_CHANNELS_MIN,
	.channels_max = SOC_NORMAL_USE_CHANNELS_MAX,
	.buffer_bytes_max = Dl1_MAX_BUFFER_SIZE,
	.period_bytes_max = Dl1_MAX_PERIOD_SIZE,
	.periods_min = SOC_NORMAL_USE_PERIODS_MIN,
	.periods_max = SOC_NORMAL_USE_PERIODS_MAX,
	.fifo_size = 0,
};

static int mtk_pcm_dl1Bt_stop(struct snd_pcm_substream *substream)
{

	PRINTK_AUDDRV("mtk_pcm_dl1Bt_stop\n");

	mt_afe_set_irq_state(Soc_Aud_IRQ_MCU_MODE_IRQ1_MCU_MODE, false);

	/* here to turn off digital part */
#ifdef BTDAI_MODEM_INTF
	mt_afe_set_connection(Soc_Aud_InterCon_DisConnect, Soc_Aud_InterConnectionInput_I05,
			Soc_Aud_InterConnectionOutput_O07);
	mt_afe_set_connection(Soc_Aud_InterCon_DisConnect, Soc_Aud_InterConnectionInput_I06,
			Soc_Aud_InterConnectionOutput_O08);
#else
	mt_afe_set_connection(Soc_Aud_InterCon_DisConnect, Soc_Aud_InterConnectionInput_I05,
		      Soc_Aud_InterConnectionOutput_O02);
	mt_afe_set_connection(Soc_Aud_InterCon_DisConnect, Soc_Aud_InterConnectionInput_I06,
		      Soc_Aud_InterConnectionOutput_O02);
#endif
	mt_afe_disable_memory_path(Soc_Aud_Digital_Block_MEM_DL1);

	mt_afe_set_irq_state(Soc_Aud_IRQ_MCU_MODE_IRQ1_MCU_MODE, false);
#ifdef BTDAI_MODEM_INTF
	mt_afe_disable_memory_path(Soc_Aud_Digital_Block_MODEM_PCM_2_O);
	if (mt_afe_get_memory_path_state(Soc_Aud_Digital_Block_MODEM_PCM_2_O) == false)
		SetModemPcmEnable(MODEM_EXTERNAL, false);
#else
	mt_afe_disable_memory_path(Soc_Aud_Digital_Block_DAI_BT);

	if (mt_afe_get_memory_path_state(Soc_Aud_Digital_Block_DAI_BT) == false)
		SetDaiBtEnable(false);
#endif

	mt_afe_enable_afe(false);
	RemoveMemifSubStream(Soc_Aud_Digital_Block_MEM_DL1, substream);
	mt_afe_main_clk_off();

	return 0;
}

static snd_pcm_uframes_t mtk_dl1bt_pcm_pointer(struct snd_pcm_substream *substream)
{
	int32_t HW_memory_index = 0;
	int32_t HW_Cur_ReadIdx = 0;
	uint32_t Frameidx = 0;
	int32_t Afe_consumed_bytes = 0;
	unsigned long flags;

	struct AFE_BLOCK_T *Afe_Block = &pdl1btMemControl->rBlock;
	struct snd_pcm_runtime *runtime = substream->runtime;
	PRINTK_AUD_DL1(" %s Afe_Block->u4DMAReadIdx = 0x%x\n", __func__,
		Afe_Block->u4DMAReadIdx);

	spin_lock_irqsave(&pdl1btMemControl->substream_lock, flags);

	/* get total bytes to copy */
	/* Frameidx = audio_bytes_to_frame(substream , Afe_Block->u4DMAReadIdx); */
	/* return Frameidx; */

	if (mt_afe_get_memory_path_state(Soc_Aud_Digital_Block_MEM_DL1) == true) {
		HW_Cur_ReadIdx = mt_afe_get_reg(AFE_DL1_CUR);
		if (HW_Cur_ReadIdx == 0) {
			PRINTK_AUDDRV("[Auddrv] HW_Cur_ReadIdx == 0\n");
			HW_Cur_ReadIdx = Afe_Block->pucPhysBufAddr;
		}

		HW_memory_index = (HW_Cur_ReadIdx - Afe_Block->pucPhysBufAddr);

		if (HW_memory_index >= Afe_Block->u4DMAReadIdx)
			Afe_consumed_bytes = HW_memory_index - Afe_Block->u4DMAReadIdx;
		else {
			Afe_consumed_bytes = Afe_Block->u4BufferSize + HW_memory_index
				- Afe_Block->u4DMAReadIdx;
		}

		Afe_consumed_bytes = align64bytesize(Afe_consumed_bytes);

		Afe_Block->u4DataRemained -= Afe_consumed_bytes;
		Afe_Block->u4DMAReadIdx += Afe_consumed_bytes;
		Afe_Block->u4DMAReadIdx %= Afe_Block->u4BufferSize;

		PRINTK_AUD_DL1
			("[Auddrv] HW_Cur_ReadIdx = 0x%x HW_memory_index = 0x%x Afe_consumed_bytes = 0x%x\n",
			HW_Cur_ReadIdx, HW_memory_index, Afe_consumed_bytes);

		spin_unlock_irqrestore(&pdl1btMemControl->substream_lock, flags);

		return bytes_to_frames(runtime, Afe_Block->u4DMAReadIdx);
	}

	Frameidx = bytes_to_frames(runtime, Afe_Block->u4DMAReadIdx);
	spin_unlock_irqrestore(&pdl1btMemControl->substream_lock, flags);
	return Frameidx;
}


static int mtk_pcm_dl1bt_hw_params(struct snd_pcm_substream *substream,
				   struct snd_pcm_hw_params *hw_params)
{
	int ret = 0;

	PRINTK_AUDDRV("mtk_pcm_dl1bt_hw_params\n");

	/* runtime->dma_bytes has to be set manually to allow mmap */
	substream->runtime->dma_bytes = params_buffer_bytes(hw_params);

	/* here to allcoate sram to hardware --------------------------- */
	afe_allocate_mem_buffer(mDev, Soc_Aud_Digital_Block_MEM_DL1,
				   substream->runtime->dma_bytes);
	/* substream->runtime->dma_bytes = AFE_INTERNAL_SRAM_SIZE; */
	substream->runtime->dma_area = (unsigned char *)mt_afe_get_sram_base_ptr();
	substream->runtime->dma_addr = mt_afe_get_sram_phy_addr();

	PRINTK_AUDDRV("dma_bytes = %zu, dma_area = %p, dma_addr = 0x%lx\n",
		      substream->runtime->dma_bytes, substream->runtime->dma_area,
		      (long)substream->runtime->dma_addr);
	return ret;
}

static int mtk_pcm_dl1bt_hw_free(struct snd_pcm_substream *substream)
{
	PRINTK_AUDDRV("mtk_pcm_dl1bt_hw_free\n");
	return 0;
}

static struct snd_pcm_hw_constraint_list constraints_dl1_sample_rates = {

	.count = ARRAY_SIZE(soc_voice_supported_sample_rates),
	.list = soc_voice_supported_sample_rates,
	.mask = 0,
};

static int mPlaybackSramState;
static int mtk_dl1bt_pcm_open(struct snd_pcm_substream *substream)
{
	int ret = 0;
	struct snd_pcm_runtime *runtime = substream->runtime;

	afe_control_sram_lock();
	if (get_sramstate() == SRAM_STATE_FREE) {
		mtk_dl1bt_pcm_hardware.buffer_bytes_max = get_playback_sram_fullsize();
		mPlaybackSramState = SRAM_STATE_PLAYBACKFULL;
		set_sramstate(mPlaybackSramState);
	} else {
		mtk_dl1bt_pcm_hardware.buffer_bytes_max = GetPLaybackSramPartial();
		mPlaybackSramState = SRAM_STATE_PLAYBACKPARTIAL;
		set_sramstate(mPlaybackSramState);
	}
	afe_control_sram_unlock();

	PRINTK_AUDDRV("mtk_dl1bt_pcm_open\n");
	mt_afe_main_clk_on();

	/* get dl1 memconptrol and record substream */
	pdl1btMemControl = get_mem_control_t(Soc_Aud_Digital_Block_MEM_DL1);
	runtime->hw = mtk_dl1bt_pcm_hardware;
	memcpy((void *)(&(runtime->hw)), (void *)&mtk_dl1bt_pcm_hardware,
	       sizeof(struct snd_pcm_hardware));

	ret = snd_pcm_hw_constraint_list(runtime, 0, SNDRV_PCM_HW_PARAM_RATE,
					 &constraints_dl1_sample_rates);
	if (ret < 0)
		PRINTK_AUDDRV("snd_pcm_hw_constraint_list failed\n");

	ret = snd_pcm_hw_constraint_integer(runtime, SNDRV_PCM_HW_PARAM_PERIODS);

	if (ret < 0)
		PRINTK_AUDDRV("snd_pcm_hw_constraint_integer failed\n");

	/* print for hw pcm information */
	PRINTK_AUDDRV("%s runtime->rate = %d, channels = %d, substream->pcm->device = %d\n",
		__func__, runtime->rate, runtime->channels, substream->pcm->device);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		PRINTK_AUDDRV("SNDRV_PCM_STREAM_PLAYBACK mtkalsa_playback_constraints\n");

	if (ret < 0) {
		PRINTK_AUDDRV("mtk_Dl1Bt_close\n");
		mtk_Dl1Bt_close(substream);
		return ret;
	}
	return 0;
}

static int mtk_Dl1Bt_close(struct snd_pcm_substream *substream)
{
	PRINTK_AUDDRV("%s\n", __func__);
	afe_control_sram_lock();
	clear_sramstate(mPlaybackSramState);
	mPlaybackSramState = get_sramstate();
	afe_control_sram_unlock();
	mt_afe_main_clk_off();
	return 0;
}

static int mtk_dl1bt_pcm_prepare(struct snd_pcm_substream *substream)
{
	return 0;
}

static int mtk_pcm_dl1bt_start(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;

	mt_afe_main_clk_on();
	set_memif_substream(Soc_Aud_Digital_Block_MEM_DL1, substream);
#ifdef BTDAI_MODEM_INTF
	if (runtime->format == SNDRV_PCM_FORMAT_S32_LE
		|| runtime->format == SNDRV_PCM_FORMAT_U32_LE) {
		/* BT SCO only support 16 bit */
		SetoutputConnectionFormat(OUTPUT_DATA_FORMAT_16BIT, Soc_Aud_InterConnectionOutput_O07);
	} else {
		SetoutputConnectionFormat(OUTPUT_DATA_FORMAT_16BIT, Soc_Aud_InterConnectionOutput_O08);
	}

	/* here start digital part */
	mt_afe_set_connection(Soc_Aud_InterCon_Connection, Soc_Aud_InterConnectionInput_I05,
			Soc_Aud_InterConnectionOutput_O07);
	mt_afe_set_connection(Soc_Aud_InterCon_Connection, Soc_Aud_InterConnectionInput_I06,
			Soc_Aud_InterConnectionOutput_O08);
	mt_afe_set_connection(Soc_Aud_InterCon_ConnectionShift, Soc_Aud_InterConnectionInput_I05,
			Soc_Aud_InterConnectionOutput_O07);
	mt_afe_set_connection(Soc_Aud_InterCon_ConnectionShift, Soc_Aud_InterConnectionInput_I06,
			Soc_Aud_InterConnectionOutput_O08);
#else
	if (runtime->format == SNDRV_PCM_FORMAT_S32_LE
	    || runtime->format == SNDRV_PCM_FORMAT_U32_LE) {
		/* BT SCO only support 16 bit */
		SetoutputConnectionFormat(OUTPUT_DATA_FORMAT_16BIT, Soc_Aud_InterConnectionOutput_O02);
	} else {
		SetoutputConnectionFormat(OUTPUT_DATA_FORMAT_16BIT, Soc_Aud_InterConnectionOutput_O02);
	}

	/* here start digital part */
	mt_afe_set_connection(Soc_Aud_InterCon_Connection, Soc_Aud_InterConnectionInput_I05,
		      Soc_Aud_InterConnectionOutput_O02);
	mt_afe_set_connection(Soc_Aud_InterCon_Connection, Soc_Aud_InterConnectionInput_I06,
		      Soc_Aud_InterConnectionOutput_O02);
	mt_afe_set_connection(Soc_Aud_InterCon_ConnectionShift, Soc_Aud_InterConnectionInput_I05,
		      Soc_Aud_InterConnectionOutput_O02);
	mt_afe_set_connection(Soc_Aud_InterCon_ConnectionShift, Soc_Aud_InterConnectionInput_I06,
		      Soc_Aud_InterConnectionOutput_O02);
#endif

	/* set dl1 sample ratelimit_state */
	mt_afe_set_sample_rate(Soc_Aud_Digital_Block_MEM_DL1, runtime->rate);
	mt_afe_set_channels(Soc_Aud_Digital_Block_MEM_DL1, runtime->channels);
	mt_afe_enable_memory_path(Soc_Aud_Digital_Block_MEM_DL1);

	/* here to set interrupt */
	mt_afe_set_irq_counter(Soc_Aud_IRQ_MCU_MODE_IRQ1_MCU_MODE, runtime->period_size >> 1);
	mt_afe_set_irq_rate(Soc_Aud_IRQ_MCU_MODE_IRQ1_MCU_MODE, runtime->rate);
	mt_afe_set_irq_state(Soc_Aud_IRQ_MCU_MODE_IRQ1_MCU_MODE, true);

#ifdef BTDAI_MODEM_INTF
	if (mt_afe_get_memory_path_state(Soc_Aud_Digital_Block_MODEM_PCM_2_O) == false) {
		/* set merge interface */
		mt_afe_enable_memory_path(Soc_Aud_Digital_Block_MODEM_PCM_2_O);
		extmd_pcm_tx.mPcmModeWidebandSel = (runtime->rate == 8000) ?
			Soc_Aud_PCM_MODE_PCM_MODE_8K : Soc_Aud_PCM_MODE_PCM_MODE_16K;
		SetModemPcmConfig(MODEM_EXTERNAL, extmd_pcm_tx);
		SetModemPcmEnable(MODEM_EXTERNAL, true);
	} else
		mt_afe_enable_memory_path(Soc_Aud_Digital_Block_MODEM_PCM_2_O);

#else
	if (mt_afe_get_memory_path_state(Soc_Aud_Digital_Block_DAI_BT) == false) {
		/* set merge interface */
		mt_afe_enable_memory_path(Soc_Aud_Digital_Block_DAI_BT);
	} else
		mt_afe_enable_memory_path(Soc_Aud_Digital_Block_DAI_BT);

	/*SetVoipDAIBTAttribute(runtime->rate);
	SetDaiBtEnable(true);*/
#endif


	mt_afe_enable_afe(true);

	return 0;
}

static int mtk_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
	PRINTK_AUDDRV("mtk_pcm_trigger cmd = %d\n", cmd);
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
		return mtk_pcm_dl1bt_start(substream);
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
		return mtk_pcm_dl1Bt_stop(substream);
	}
	return -EINVAL;
}

static int mtk_pcm_dl1bt_copy(struct snd_pcm_substream *substream,
			      int channel, snd_pcm_uframes_t pos,
			      void __user *dst, snd_pcm_uframes_t count)
{
	struct AFE_BLOCK_T *Afe_Block = NULL;
	unsigned long flags;
	char *data_w_ptr = (char *)dst;
	int copy_size = 0, Afe_WriteIdx_tmp;
        struct snd_pcm_runtime *runtime = substream->runtime;

	PRINTK_AUD_DL1("%s pos = %lu count = %lu\n", __func__, pos, count);

	/* get total bytes to copy */
	count = frames_to_bytes(runtime, count);

	/* check which memif nned to be write */
	Afe_Block = &pdl1btMemControl->rBlock;

	/* handle for buffer management */

	PRINTK_AUD_DL1("%s WriteIdx=0x%x, ReadIdx=0x%x, DataRemained=0x%x\n", __func__,
		       Afe_Block->u4WriteIdx, Afe_Block->u4DMAReadIdx, Afe_Block->u4DataRemained);

	if (Afe_Block->u4BufferSize == 0) {
		pr_err("%s: u4BufferSize=0 Error\n", __func__);
		return 0;
	}

	spin_lock_irqsave(&auddrv_DL1BTCtl_lock, flags);
	copy_size = Afe_Block->u4BufferSize - Afe_Block->u4DataRemained;	/* free space of the buffer */
	spin_unlock_irqrestore(&auddrv_DL1BTCtl_lock, flags);

	if (count <= copy_size) {
		if (copy_size < 0)
			copy_size = 0;
		else
			copy_size = count;
	}

	copy_size = align64bytesize(copy_size);
	PRINTK_AUD_DL1("copy_size=0x%x, count=0x%x\n", copy_size, (unsigned int)count);

	if (copy_size != 0) {
		spin_lock_irqsave(&auddrv_DL1BTCtl_lock, flags);
		Afe_WriteIdx_tmp = Afe_Block->u4WriteIdx;
		spin_unlock_irqrestore(&auddrv_DL1BTCtl_lock, flags);

		if (Afe_WriteIdx_tmp + copy_size < Afe_Block->u4BufferSize) {	/* copy once */
			if (!access_ok(VERIFY_READ, data_w_ptr, copy_size)) {
				PRINTK_AUDDRV("AudDrv_write 0 ptr invalid data_w_ptr=%p, size=%d",
					      data_w_ptr, copy_size);
				PRINTK_AUDDRV("AudDrv_write u4BufferSize=%d, u4DataRemained=%d",
					      Afe_Block->u4BufferSize, Afe_Block->u4DataRemained);
			} else {
				PRINTK_AUD_DL1
				("copy Afe_Block->pucVirtBufAddr+Afe_WriteIdx=%p, data_w_ptr=%p, copy_size=%x\n",
				Afe_Block->pucVirtBufAddr + Afe_WriteIdx_tmp, data_w_ptr, copy_size);

				if (copy_from_user((Afe_Block->pucVirtBufAddr + Afe_WriteIdx_tmp),
					data_w_ptr, copy_size)) {
					PRINTK_AUDDRV("AudDrv_write Fail copy from user\n");
					return -1;
				}
			}

			spin_lock_irqsave(&auddrv_DL1BTCtl_lock, flags);
			Afe_Block->u4DataRemained += copy_size;
			Afe_Block->u4WriteIdx = Afe_WriteIdx_tmp + copy_size;
			Afe_Block->u4WriteIdx %= Afe_Block->u4BufferSize;
			spin_unlock_irqrestore(&auddrv_DL1BTCtl_lock, flags);
			data_w_ptr += copy_size;
			count -= copy_size;

			PRINTK_AUD_DL1
				("%s finish1, copy_size:%x, WriteIdx:%x, ReadIdx=%x, DataRemained:%x, count=%lu\n",
				__func__, copy_size, Afe_Block->u4WriteIdx, Afe_Block->u4DMAReadIdx,
				Afe_Block->u4DataRemained, count);

		} else {	/* copy twice */
			uint32_t size_1 = 0, size_2 = 0;

			size_1 = align64bytesize((Afe_Block->u4BufferSize - Afe_WriteIdx_tmp));
			size_2 = align64bytesize((copy_size - size_1));
			PRINTK_AUD_DL1("size_1=0x%x, size_2=0x%x\n", size_1, size_2);

			if (!access_ok(VERIFY_READ, data_w_ptr, size_1)) {
				pr_warn("AudDrv_write 1 ptr invalid data_w_ptr=%p, size_1=%d\n",
				       data_w_ptr, size_1);
				pr_warn("AudDrv_write u4BufferSize=%d, u4DataRemained=%d\n",
				       Afe_Block->u4BufferSize, Afe_Block->u4DataRemained);
			} else {
				PRINTK_AUD_DL1
					("copy, Afe_Block->pucVirtBufAddr+Afe_WriteIdx=%p, data_w_ptr=%p, size_1=%x\n",
					Afe_Block->pucVirtBufAddr + Afe_WriteIdx_tmp, data_w_ptr, size_1);

				if ((copy_from_user((Afe_Block->pucVirtBufAddr + Afe_WriteIdx_tmp),
					data_w_ptr, size_1))) {
					PRINTK_AUDDRV("AudDrv_write Fail 1 copy from user\n");
					return -1;
				}
			}
			spin_lock_irqsave(&auddrv_DL1BTCtl_lock, flags);
			Afe_Block->u4DataRemained += size_1;
			Afe_Block->u4WriteIdx = Afe_WriteIdx_tmp + size_1;
			Afe_Block->u4WriteIdx %= Afe_Block->u4BufferSize;
			Afe_WriteIdx_tmp = Afe_Block->u4WriteIdx;
			spin_unlock_irqrestore(&auddrv_DL1BTCtl_lock, flags);

			if (!access_ok(VERIFY_READ, data_w_ptr + size_1, size_2)) {
				PRINTK_AUDDRV
					("AudDrv_write 2 ptr invalid data_w_ptr=%p, size_1=%d, size_2=%d\n",
					data_w_ptr, size_1, size_2);
				PRINTK_AUDDRV
					("AudDrv_write u4BufferSize = %d, u4DataRemained = %d\n",
					Afe_Block->u4BufferSize, Afe_Block->u4DataRemained);
			} else {
				PRINTK_AUD_DL1
				("copy, Afe_Block->pucVirtBufAddr+Afe_WriteIdx=%p, data_w_ptr+size_1=%p, size_2=%x\n",
				Afe_Block->pucVirtBufAddr + Afe_WriteIdx_tmp, data_w_ptr + size_1, size_2);

				if ((copy_from_user((Afe_Block->pucVirtBufAddr + Afe_WriteIdx_tmp),
					(data_w_ptr + size_1), size_2))) {
					PRINTK_AUDDRV("AudDrv_write Fail 2 copy from user\n");
					return -1;
				}
			}
			spin_lock_irqsave(&auddrv_DL1BTCtl_lock, flags);

			Afe_Block->u4DataRemained += size_2;
			Afe_Block->u4WriteIdx = Afe_WriteIdx_tmp + size_2;
			Afe_Block->u4WriteIdx %= Afe_Block->u4BufferSize;
			spin_unlock_irqrestore(&auddrv_DL1BTCtl_lock, flags);
			count -= copy_size;
			data_w_ptr += copy_size;

			PRINTK_AUD_DL1
				("AudDrv_write finish 2, copy size:%x, WriteIdx:%x, ReadIdx:%x, DataRemained:%x\n",
				copy_size, Afe_Block->u4WriteIdx, Afe_Block->u4DMAReadIdx,
				Afe_Block->u4DataRemained);
		}
	}
	PRINTK_AUD_DL1("pcm_copy return\n");
	return 0;
}

static int mtk_pcm_dl1bt_silence(struct snd_pcm_substream *substream,
				 int channel, snd_pcm_uframes_t pos, snd_pcm_uframes_t count)
{
	PRINTK_AUDDRV("%s\n", __func__);
	/* do nothing */
	return 0;
}

static void *dummy_page[2];

static struct page *mtk_pcm_page(struct snd_pcm_substream *substream, unsigned long offset)
{
	PRINTK_AUDDRV("%s\n", __func__);
	return virt_to_page(dummy_page[substream->stream]);	/* the same page */
}

static struct snd_pcm_ops mtk_d1lbt_ops = {

	.open = mtk_dl1bt_pcm_open,
	.close = mtk_Dl1Bt_close,
	.ioctl = snd_pcm_lib_ioctl,
	.hw_params = mtk_pcm_dl1bt_hw_params,
	.hw_free = mtk_pcm_dl1bt_hw_free,
	.prepare = mtk_dl1bt_pcm_prepare,
	.trigger = mtk_pcm_trigger,
	.pointer = mtk_dl1bt_pcm_pointer,
	.copy = mtk_pcm_dl1bt_copy,
	.silence = mtk_pcm_dl1bt_silence,
	.page = mtk_pcm_page,
};

static struct snd_soc_platform_driver mtk_soc_dl1bt_platform = {

	.ops = &mtk_d1lbt_ops,
	.pcm_new = mtk_asoc_Dl1Bt_pcm_new,
	.probe = mtk_asoc_dl1bt_probe,
};

static int mtk_dl1bt_probe(struct platform_device *pdev)
{
	PRINTK_AUDDRV("%s\n", __func__);

	if (pdev->dev.dma_mask == NULL)
		pdev->dev.dma_mask = &pdev->dev.coherent_dma_mask;

	if (pdev->dev.of_node)
		dev_set_name(&pdev->dev, "%s", MT_SOC_VOIP_BT_OUT);

	PRINTK_AUDDRV("%s: dev name %s\n", __func__, dev_name(&pdev->dev));

	mDev = &pdev->dev;

	return snd_soc_register_platform(&pdev->dev, &mtk_soc_dl1bt_platform);
}

static int mtk_asoc_Dl1Bt_pcm_new(struct snd_soc_pcm_runtime *rtd)
{
	int ret = 0;

	PRINTK_AUDDRV("%s\n", __func__);
	return ret;
}


static int mtk_asoc_dl1bt_probe(struct snd_soc_platform *platform)
{
	PRINTK_AUDDRV("mtk_asoc_dl1bt_probe\n");
/*	snd_soc_add_platform_controls(platform, audio_snd_dl1_bt_controls,
				      ARRAY_SIZE(audio_snd_dl1_bt_controls));	*/
	return 0;
}

static int mtk_asoc_dl1bt_remove(struct platform_device *pdev)
{
	PRINTK_AUDDRV("%s\n", __func__);
	snd_soc_unregister_platform(&pdev->dev);
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id mt_soc_pcm_dl1_bt_of_ids[] = {

	{.compatible = "mediatek," MT_SOC_VOIP_BT_OUT,},
	{}
};
MODULE_DEVICE_TABLE(of, mt_soc_pcm_dl1_bt_of_ids);

#endif

static struct platform_driver mtk_dl1bt_driver = {

	.driver = {
		   .name = MT_SOC_VOIP_BT_OUT,
		   .owner = THIS_MODULE,
#ifdef CONFIG_OF
		   .of_match_table = mt_soc_pcm_dl1_bt_of_ids,
#endif
		   },
	.probe = mtk_dl1bt_probe,
	.remove = mtk_asoc_dl1bt_remove,
};

#ifdef CONFIG_OF
module_platform_driver(mtk_dl1bt_driver);
#else

static int __init mtk_soc_dl1bt_platform_init(void)
{
	int ret;

	PRINTK_AUDDRV("%s\n", __func__);
	ret = platform_driver_register(&mtk_dl1bt_driver);
	return ret;

}
module_init(mtk_soc_dl1bt_platform_init);

static void __exit mtk_soc_dl1bt_platform_exit(void)
{
	PRINTK_AUDDRV("%s\n", __func__);

	platform_driver_unregister(&mtk_dl1bt_driver);
}
module_exit(mtk_soc_dl1bt_platform_exit);
#endif
MODULE_DESCRIPTION("AFE dl1bt module platform driver");
MODULE_LICENSE("GPL");
