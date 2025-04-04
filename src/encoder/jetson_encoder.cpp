#include "encoder/jetson_encoder.h"
#include "NvUtils.h"
#include "absl/memory/memory.h"
#include <linux/v4l2-controls.h>
#include <memory>

void JetsonEncoder::SetDefaults() {
  memset(ctx, 0, sizeof(context_t));

  ctx->raw_pixfmt = V4L2_PIX_FMT_YUV420M;
  ctx->bitrate = 4 * 1024 * 1024;
  ctx->peak_bitrate = 0;
  /* Instead of keep H264 baselin, it is kept as Base value
   * so as to it can be taken care per codec wise
   */
  ctx->profile = 0;
  ctx->ratecontrol = V4L2_MPEG_VIDEO_BITRATE_MODE_CBR;
  ctx->iframe_interval = 30;
  ctx->externalRPS = false;
  ctx->enableGDR = false;
  ctx->enableROI = false;
  ctx->bnoIframe = false;
  ctx->bGapsInFrameNumAllowed = false;
  ctx->bReconCrc = false;
  ctx->enableLossless = false;
  ctx->nH264FrameNumBits = 0;
  ctx->nH265PocLsbBits = 0;
  ctx->idr_interval = 256;
  ctx->level = -1;
  ctx->fps_n = 30;
  ctx->fps_d = 1;
  ctx->gdr_start_frame_number = 0xffffffff;
  ctx->gdr_num_frames = 0xffffffff;
  ctx->gdr_out_frame_number = 0xffffffff;
  ctx->num_b_frames = (uint32_t)-1;
  ctx->nMinQpI = (uint32_t)QP_RETAIN_VAL;
  ctx->nMaxQpI = (uint32_t)QP_RETAIN_VAL;
  ctx->nMinQpP = (uint32_t)QP_RETAIN_VAL;
  ctx->nMaxQpP = (uint32_t)QP_RETAIN_VAL;
  ctx->nMinQpB = (uint32_t)QP_RETAIN_VAL;
  ctx->nMaxQpB = (uint32_t)QP_RETAIN_VAL;
  ctx->use_gold_crc = false;
  ctx->pBitStreamCrc = NULL;
  ctx->externalRCHints = false;
  ctx->input_metadata = false;
  ctx->sMaxQp = 51;
  ctx->stats = false;
  ctx->stress_test = 1;
  ctx->output_memory_type = V4L2_MEMORY_DMABUF;
  ctx->capture_memory_type = V4L2_MEMORY_MMAP;
  ctx->cs = V4L2_COLORSPACE_SMPTE170M;
  ctx->copy_timestamp = false;
  ctx->sar_width = 0;
  ctx->sar_height = 0;
  ctx->start_ts = 0;
  ctx->max_perf = 0;
  ctx->blocking_mode = 1;
  ctx->startf = 0;
  ctx->endf = 0;
  ctx->num_output_buffers = 6;
  ctx->num_frames_to_encode = -1;
  ctx->poc_type = 0;
  ctx->chroma_format_idc = -1;
  ctx->bit_depth = 8;
  ctx->is_semiplanar = false;
  ctx->enable_initQP = false;
  ctx->IinitQP = 0;
  ctx->PinitQP = 0;
  ctx->BinitQP = 0;
  ctx->enable_ratecontrol = true;
  ctx->enable_av1tile = false;
  ctx->log2_num_av1rows = 0;
  ctx->log2_num_av1cols = 0;
  ctx->enable_av1ssimrdo = (uint8_t)-1;
  ctx->disable_av1cdfupdate = (uint8_t)-1;
  ctx->av1erresmode = true;
  ctx->enable_av1frameidflag = false;
  ctx->externalTG = false;
  ctx->ppe_init_params.enable_ppe = false;
  ctx->ppe_init_params.wait_time_ms = -1;
  ctx->ppe_init_params.feature_flags = V4L2_PPE_FEATURE_NONE;
  ctx->ppe_init_params.enable_profiler = 0;
  ctx->ppe_init_params.taq_max_qp_delta = 5;
  ctx->ppe_init_params.saq_max_qp_delta = 5;
  /* TAQ for B-frames is enabled by default */
  ctx->ppe_init_params.taq_b_frame_mode = 1;
  ctx->disable_amp = false;
  ctx->got_drc = false;
  ctx->enable_two_pass_cbr = false;
}

int32_t JetsonEncoder::InitEncode(const webrtc::VideoCodec *codec_settings,
                                  int32_t number_of_cores,
                                  size_t max_payload_size) {
  this->SetDefaults();
  ctx.encoder_pixfmt = V4L2_PIX_FMT_H264;
  ctx.encode_width = ctx.width = codec_settings->width;
  ctx.encode_height = ctx.height = codec_settings->height;
  ctx.enc = NvVideoEncoder::createVideoEncoder("enc0", O_NONBLOCK);
  ctx.level = V4L2_MPEG_VIDEO_H264_LEVEL_4_2;
  int ret = ctx.enc->setCapturePlaneFormat(ctx.encoder_pixfmt, ctx.width,
                                           ctx.height, 2 * 1024 * 1024);
  assert(ret == 0);
}

webrtc::VideoEncoderFactory::CodecInfo JetsonEncoderFactory::QueryVideoEncoder(
    const webrtc::SdpVideoFormat &format) const {
  VideoEncoderFactory::CodecInfo info;
  info.has_internal_source = false;
  info.is_hardware_accelerated = true;
  return info;
}

std::unique_ptr<webrtc::VideoEncoder>
JetsonEncoderFactory::CreateVideoEncoder(const webrtc::SdpVideoFormat &format) {
  return absl::make_unique<JetsonEncoder>();
}

std::unique_ptr<webrtc::VideoEncoderFactory> CreateJetsonEncoderFactory() {
  return absl::make_unique<JetsonEncoderFactory>();
}
