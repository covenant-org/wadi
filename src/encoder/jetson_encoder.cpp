#include "encoder/jetson_encoder.h"
#include "NvUtils.h"
#include "absl/memory/memory.h"
#include "api/scoped_refptr.h"
#include "api/video/video_frame_buffer.h"
#include "logging.h"
#include <cstdint>
#include <cstring>
#include <linux/v4l2-controls.h>
#include <linux/videodev2.h>
#include <memory>
#ifndef MAX_PLANES
#define MAX_PLANES 4
#endif

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
  ctx.enc = NvVideoEncoder::createVideoEncoder("enc0");
  ctx.level = V4L2_MPEG_VIDEO_H264_LEVEL_5_1;
  ctx.output_memory_type = V4L2_MEMORY_MMAP;
  ctx.capture_memory_type = V4L2_MEMORY_MMAP;
  ctx.raw_pixfmt = V4L2_PIX_FMT_YUV420M;

  int ret = ctx.enc->setCapturePlaneFormat(ctx.encoder_pixfmt, ctx.width,
                                           ctx.height, 2 * 1024 * 1024);
  assert(ret == 0);

  ret = ctx.enc->setOutputPlaneFormat(ctx.raw_pixfmt, ctx.encode_width,
                                      ctx.encode_height, ctx.cs);
  assert(ret == 0);

  ret = ctx.enc->setBitrate(ctx.bitrate);
  assert(ret == 0);

  ret = ctx.enc->setProfile(ctx.profile);
  assert(ret == 0);

  ret = ctx.enc->setLevel(ctx.level);
  assert(ret == 0);

  ret = ctx.enc->setRateControl(ctx.rate_control);
  assert(ret == 0);

  /* Set IDR frame interval for encoder */
  ret = ctx.enc->setIDRInterval(ctx.idr_interval);
  assert(ret == 0);

  /* Set I frame interval for encoder */
  ret = ctx.enc->setIFrameInterval(ctx.iframe_interval);
  assert(ret == 0);

  /* Set framerate for encoder */
  ret = ctx.enc->setFrameRate(ctx.fps_n, ctx.fps_d);
  assert(ret == 0);

  // max performance
  ret = ctx.enc->setMaxPerf(1);
  assert(ret == 0);

  ret = ctx.enc->output_plane.setupPlane(V4L2_MEMORY_MMAP, 10, true, false);
  assert(ret == 0);

  ret = ctx.enc->capture_plane.setupPlane(V4L2_MEMORY_MMAP,
                                          ctx.num_output_buffers, true, false);
  assert(ret == 0);

  // NOTE: Probably not the best way to do this.
  ctx.encoded_images = new webrtc::EncodedImage[ctx.num_output_buffers];

  ret = ctx.enc->subscriveEvent(V4L2_EVENT_EOS, 0, 0);
  assert(ret == 0);

  ret = ctx.enc->output_plane.setStreamStatus(true);
  assert(ret == 0);

  ret = ctx.enc->capture_plane.setStreamStatus(true);
  assert(ret == 0);

  ctx.enc->capture_plane.setDQThreadCallback(
      &JetsonEncoder::EncoderCapturePlaneCallback);
  ctx.enc->capture_plane.startDQThread(&ctx);

  /* Enqueue all the empty capture plane buffers. */
  for (uint32_t i = 0; i < ctx.enc->capture_plane.getNumBuffers(); i++) {
    struct v4l2_buffer v4l2_buf;
    struct v4l2_plane planes[MAX_PLANES];

    memset(&v4l2_buf, 0, sizeof(v4l2_buf));
    memset(planes, 0, MAX_PLANES * sizeof(struct v4l2_plane));

    v4l2_buf.index = i;
    v4l2_buf.m.planes = planes;

    ret = ctx.enc->capture_plane.qBuffer(v4l2_buf, NULL);
    if (ret < 0) {
      tlog("Error while queueing buffer at capture plane");
      return ret;
    }
  }
  return 0;
}

bool JetsonEncoder::EncoderCapturePlaneCallback(struct v4l2_buffer *buf,
                                                NvBuffer *buffer,
                                                NvBuffer *shared_buffer,
                                                void *arg) {
  context_t *ctx = (context_t *)arg;
  NvVideoEncoder *enc = ctx->enc;
  pthread_setname_np(pthread_self(), "EncCapPlane");
  uint32_t frame_num = ctx->enc->capture_plane.getTotalDequeuedBuffers() - 1;
  static uint32_t num_encoded_frames = 1;
  struct v4l2_event ev;
  int ret = 0;

  if (buf == NULL) {
    tlog("Error while dequeing buffer from output plane");
    JetsonEncoder::abort(ctx);
    return false;
  }

  /* Received EOS from encoder. Stop dqthread. */
  if (buffer->planes[0].bytesused == 0) {
    tlog("Got 0 size buffer in capture");
    JetsonEncoder::abort(ctx);
    return false;
  }

  // TODO: Fragmentize

  num_encoded_frames++;

  /* encoder qbuffer for capture plane */
  if (enc->capture_plane.qBuffer(*v4l2_buf, NULL) < 0) {
    tlog("Error while Qing buffer at capture plane");
    JetsonEncoder::abort(ctx);
    return false;
  }

  return true;
}

webrtc::VideoEncoderFactory::CodecInfo JetsonEncoderFactory::QueryVideoEncoder(
    const webrtc::SdpVideoFormat &format) const {
  VideoEncoderFactory::CodecInfo info;
  info.has_internal_source = false;
  info.is_hardware_accelerated = true;
  return info;
}

int32_t JetsonEncoder::RegisterEncodeCompleteCallback(
    webrtc::EncodedImageCallback *callback) {
  this->callback = callback;
  return 0;
}

int32_t
JetsonEncoder::Encode(const webrtc::VideoFrame &frame,
                      const std::vector<webrtc::VideoFrameType> *frame_types) {
  tlog("Encoding frame");

  // Send help
  struct v4l2_buffer v4l2_buf;
  struct v4l2_plane planes[MAX_PLANES];
  NvBuffer *buffer;
  int ret = 0;

  memset(&v4l2_buf, 0, sizeof(v4l2_buf));
  memset(planes, 0, sizeof(planes));

  v4l2_buf.m.planes = planes;

  if (ctx.enc->output_plane.dqBuffer(v4l2_buf, &buffer, NULL, 10) < 0) {
    tlog("Error while DQing buffer at output plane");
    return -1;
  }

  rtc::scoped_refptr<const webrtc::I420BufferInterface> frame_buffer =
      frame.video_frame_buffer()->ToI420();

  memcpy(buffer->planes[0].data, const_cast<uint8_t *>(frame_buffer->DataY()),
         buffer->planes[0].bytesused);
  memcpy(buffer->planes[1].data, const_cast<uint8_t *>(frame_buffer->DataU()),
         buffer->planes[1].bytesused);
  memcpy(buffer->planes[2].data, const_cast<uint8_t *>(frame_buffer->DataV()),
         buffer->planes[2].bytesused);

  for (uint32_t j = 0; j < buffer->n_planes; j++) {
    NvBufSurface *nvbuf_surf = 0;
    ret = NvBufSurfaceFromFd(buffer->planes[j].fd, (void **)(&nvbuf_surf));
    if (ret < 0) {
      tlog("Error while NvBufSurfaceFromFd");
      return -1;
    }
    ret = NvBufSurfaceSyncForDevice(nvbuf_surf, 0, j);
    if (ret < 0) {
      tlog("Error while NvBufSurfaceSyncForDevice at output plane for "
           "V4L2_MEMORY_DMABUF");
      return -1;
    }
  }

  // TODO: Fill corresponding encoded image object
  ret = ctx.enc->output_plane.qBuffer(v4l2_buf, NULL);
  if (ret < 0) {
    tlog("Error while queueing buffer at output plane");
    return -1;
  }
  ctx.input_frames_queued_count++;

  return 0;
}

int32_t JetsonEncoder::Release() {
  bool error = false;
  if (ctx.enc && ctx.enc->isInError()) {
    tlog("Error in encoder");
    error = true;
  }
  if (ctx.got_error) {
    error = true;
  }
  if (ctx.encoded_images != nullptr) {
    delete[] ctx.encoded_images;
  }
  // TODO: Clean up encoder
  return error ? -1 : 0;
}

std::unique_ptr<webrtc::VideoEncoder>
JetsonEncoderFactory::CreateVideoEncoder(const webrtc::SdpVideoFormat &format) {
  return absl::make_unique<JetsonEncoder>();
}

std::unique_ptr<webrtc::VideoEncoderFactory> CreateJetsonEncoderFactory() {
  return absl::make_unique<JetsonEncoderFactory>();
}
