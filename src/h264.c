/*
 * Copyright (c) 2018 Bootlin
 * Copyright (c) 2016 Florent Revest, <florent.revest@free-electrons.com>
 *               2007 Intel Corporation. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <assert.h>
#include <string.h>

#include <sys/mman.h>
#include <sys/ioctl.h>

#include <linux/videodev2.h>

#include "surface.h"
#include "sunxi_cedrus.h"
#include "v4l2.h"

static void h264_va_picture_to_v4l2(VAPictureParameterBufferH264 *VAPicture,
				    struct v4l2_ctrl_h264_decode_param *decode,
				    struct v4l2_ctrl_h264_pps *pps,
				    struct v4l2_ctrl_h264_sps *sps)
{
	print_indent(indent++, ".decode = {\n");
	print_indent(indent, ".top_field_order_cnt = %d,\n",
		     parameters->CurrPic.TopFieldOrderCnt);
	print_indent(indent, ".bottom_field_order_cnt = %d,\n",
		     parameters->CurrPic.BottomFieldOrderCnt);
	h264_dump_dpb(driver_data, parameters, indent);
	print_indent(--indent, "},\n");

	print_indent(indent++, ".pps = {\n");
	print_indent(indent, ".weighted_bipred_idc = %d,\n",
		     parameters->pic_fields.bits.weighted_bipred_idc);
	print_indent(indent, ".pic_init_qp_minus26 = %d,\n",
		     parameters->pic_init_qp_minus26);
	print_indent(indent, ".pic_init_qs_minus26 = %d,\n",
		     parameters->pic_init_qs_minus26);
	print_indent(indent, ".chroma_qp_index_offset = %d,\n",
		     parameters->chroma_qp_index_offset);
	print_indent(indent, ".second_chroma_qp_index_offset = %d,\n",
		     parameters->second_chroma_qp_index_offset);
	print_indent(indent, ".flags = %s | %s | %s | %s | %s,\n",
		     parameters->pic_fields.bits.entropy_coding_mode_flag ?
			"V4L2_H264_PPS_FLAG_ENTROPY_CODING_MODE " : " 0 ",
		     parameters->pic_fields.bits.weighted_pred_flag ?
			"V4L2_H264_PPS_FLAG_WEIGHTED_PRED" : "0 ",
		     parameters->pic_fields.bits.transform_8x8_mode_flag ?
			"V4L2_H264_PPS_FLAG_TRANSFORM_8X8_MODE " : "0 ",
		     parameters->pic_fields.bits.constrained_intra_pred_flag ?
			"V4L2_H264_PPS_FLAG_CONSTRAINED_INTRA_PRED" : "0 ",
		     parameters->pic_fields.bits.pic_order_present_flag ?
			"V4L2_H264_PPS_FLAG_BOTTOM_FIELD_PIC_ORDER_IN_FRAME_PRESENT" : "0 ",
		     parameters->pic_fields.bits.deblocking_filter_control_present_flag ?
			"V4L2_H264_PPS_FLAG_DEBLOCKING_FILTER_CONTROL_PRESENT" :  "0 ",
		     parameters->pic_fields.bits.redundant_pic_cnt_present_flag ?
			"V4L2_H264_PPS_FLAG_REDUNDANT_PIC_CNT_PRESENT" : "0 ");
	print_indent(--indent, "},\n");

	print_indent(indent++, ".sps = {\n");
	print_indent(indent, ".chroma_format_idc = %u,\n",
		     parameters->seq_fields.bits.chroma_format_idc);
	print_indent(indent, ".bit_depth_luma_minus8 = %u,\n",
		     parameters->bit_depth_luma_minus8);
	print_indent(indent, ".bit_depth_chroma_minus8 = %u,\n",
		     parameters->bit_depth_chroma_minus8);
	print_indent(indent, ".log2_max_frame_num_minus4 = %u,\n",
		     parameters->seq_fields.bits.log2_max_frame_num_minus4);
	print_indent(indent, ".log2_max_pic_order_cnt_lsb_minus4 = %u,\n",
		     parameters->seq_fields.bits.log2_max_pic_order_cnt_lsb_minus4);
	print_indent(indent, ".pic_order_cnt_type = %u,\n",
		     parameters->seq_fields.bits.pic_order_cnt_type);
	print_indent(indent, ".pic_width_in_mbs_minus1 = %u,\n",
		     parameters->picture_width_in_mbs_minus1);
	print_indent(indent, ".pic_height_in_map_units_minus1 = %u,\n",
		     parameters->picture_height_in_mbs_minus1);

	print_indent(indent, ".flags = %s | %s | %s | %s | %s,\n",
		     parameters->seq_fields.bits.residual_colour_transform_flag ?
		     	"V4L2_H264_SPS_FLAG_SEPARATE_COLOUR_PLANE" : "0",
		     parameters->seq_fields.bits.gaps_in_frame_num_value_allowed_flag ?
		     	"V4L2_H264_SPS_FLAG_GAPS_IN_FRAME_NUM_VALUE_ALLOWED" : "0",
		     parameters->seq_fields.bits.frame_mbs_only_flag ?
		     	"V4L2_H264_SPS_FLAG_FRAME_MBS_ONLY" : "0",
		     parameters->seq_fields.bits.mb_adaptive_frame_field_flag ?
		     	"V4L2_H264_SPS_FLAG_MB_ADAPTIVE_FRAME_FIELD" : "0",
		     parameters->seq_fields.bits.direct_8x8_inference_flag ?
		     	"V4L2_H264_SPS_FLAG_DIRECT_8X8_INFERENCE" : "0",
		     parameters->seq_fields.bits.delta_pic_order_always_zero_flag ?
		     	"V4L2_H264_SPS_FLAG_DELTA_PIC_ORDER_ALWAYS_ZERO" : "0");
	print_indent(--indent, "},\n");
}

static void h264_va_matrix_to_v4l2(VAIQMatrixBufferH264 *VAMatrix,
				   struct v4l2_ctrl_h264_scaling_matrix *v4l2_matrix)
{

	print_indent(indent++, ".scaling_matrix = {\n");
	print_u8_matrix(indent, "scaling_list_4x4",
			(uint8_t*)&parameters->ScalingList4x4,
			6, 16);
	print_u8_matrix(indent, "scaling_list_8x8",
			(uint8_t*)&parameters->ScalingList8x8,
			6, 64);
	print_indent(--indent, "},\n");
}

#define H264_SLICE_P	0
#define H264_SLICE_B	1

static void h264_va_slice_to_v4l2(VASliceParameterBufferH264 *VASlice,
				  VAPictureParameterBufferH264 *VAPicture,
				  struct v4l2_ctrl_h264_slice_param *v4l2_slice)
{
	int i;

	print_indent(indent++, ".pred_weight = {\n");
	print_indent(indent, ".chroma_log2_weight_denom = %u,\n",
		     parameters->chroma_log2_weight_denom);
	print_indent(indent, ".luma_log2_weight_denom = %u,\n",
		     parameters->luma_log2_weight_denom);

	print_indent(indent, ".weight_factors = {\n");
	for (i = 0; i < 2; i++) {
		print_indent(5, "{\n");
		print_s16_array(6, "luma_weight",
				i ? parameters->luma_weight_l1 : parameters->luma_weight_l0,
				32);
		print_s16_array(6, "luma_offset",
				i ? parameters->luma_offset_l1 : parameters->luma_offset_l0,
				32);
		print_s16_matrix(6, "chroma_weight",
				 i ? (int16_t*)&parameters->chroma_weight_l1 : (int16_t*)&parameters->chroma_weight_l0,
				 32, 2);
		print_s16_matrix(6, "chroma_offset",
				 i ? (int16_t*)parameters->chroma_offset_l1 : (int16_t*)parameters->chroma_offset_l0,
				 32, 2);
		print_indent(5, "},\n");
	}
	print_indent(indent, "},\n");
	print_indent(--indent, "},\n");

	print_indent(indent++, ".slice = {\n");
	print_indent(indent, ".size = %u,\n", parameters->slice_data_size);
	print_indent(indent, ".header_bit_size = %u,\n", parameters->slice_data_bit_offset);
	print_indent(indent, ".first_mb_in_slice = %u,\n", parameters->first_mb_in_slice);
	print_indent(indent, ".slice_type = %u,\n", parameters->slice_type);
	print_indent(indent, ".cabac_init_idc = %u,\n", parameters->cabac_init_idc);
	print_indent(indent, ".slice_qp_delta = %d,\n", parameters->slice_qp_delta);
	print_indent(indent, ".disable_deblocking_filter_idc = %u,\n", parameters->disable_deblocking_filter_idc);
	print_indent(indent, ".slice_alpha_c0_offset_div2 = %d,\n", parameters->slice_alpha_c0_offset_div2);
	print_indent(indent, ".slice_beta_offset_div2 = %d,\n", parameters->slice_beta_offset_div2);
	print_indent(indent, ".num_ref_idx_l0_active_minus1 = %u,\n", parameters->num_ref_idx_l0_active_minus1);
	print_indent(indent, ".num_ref_idx_l1_active_minus1 = %u,\n", parameters->num_ref_idx_l1_active_minus1);

	if (((parameters->slice_type % 5) == H264_SLICE_P) ||
	    ((parameters->slice_type % 5) == H264_SLICE_B)) {
		print_indent(indent, ".ref_pic_list0 = { ");
		for (i = 0; i < parameters->num_ref_idx_l0_active_minus1 + 1; i++)
			printf(" %u, ", h264_lookup_ref_pic(driver_data,
							    parameters->RefPicList0[i].frame_idx));
		printf("},\n");
	}

	if ((parameters->slice_type % 5) == H264_SLICE_B) {
		print_indent(indent, ".ref_pic_list1 = { ");
		for (i = 0; i < parameters->num_ref_idx_l1_active_minus1 + 1; i++)
			printf(" %u, ", h264_lookup_ref_pic(driver_data,
							    parameters->RefPicList1[i].frame_idx));
		printf("},\n");
	}

	if (parameters->direct_spatial_mv_pred_flag)
		print_indent(indent, ".flags = V4L2_SLICE_FLAG_DIRECT_SPATIAL_MV_PRED,\n");

	print_indent(--indent, "},\n");
}

int h264_fill_controls(struct sunxi_cedrus_driver_data *driver,
		       struct object_surface *surface)
{
	VAPictureParameterBufferMPEG2 *parameters = &surface->params.mpeg2.picture;
	struct v4l2_ctrl_h264_scaling_matrix matrix = { 0 };
	struct v4l2_ctrl_h264_decode_param decode = { 0 };
	struct v4l2_ctrl_h264_slice_param slice = { 0 };
	struct v4l2_ctrl_h264_pps pps = { 0 };
	struct v4l2_ctrl_h264_sps sps = { 0 };
	int rc;

	h264_va_picture_to_v4l2(&surface->params.h264.picture,
				&decode, &pps, &sps);
	h264_va_matrix_to_v4l2(&surface->params.h264.matrix, &matrix);
	h264_va_slice_to_v4l2(&surface->params.h264.slice,
			      &surface->params.h264.picture, &matrix);

	rc = v4l2_set_control(driver->video_fd, surface->request_fd,
			      V4L2_CID_MPEG_VIDEO_H264_DECODE_PARAM,
			      &decode, sizeof(decode));
	if (rc < 0)
		return VA_STATUS_ERROR_OPERATION_FAILED;

	rc = v4l2_set_control(driver->video_fd, surface->request_fd,
			      V4L2_CID_MPEG_VIDEO_H264_SLICE_PARAM,
			      &slice, sizeof(slice));
	if (rc < 0)
		return VA_STATUS_ERROR_OPERATION_FAILED;

	rc = v4l2_set_control(driver->video_fd, surface->request_fd,
			      V4L2_CID_MPEG_VIDEO_H264_PPS,
			      &pps, sizeof(pps));
	if (rc < 0)
		return VA_STATUS_ERROR_OPERATION_FAILED;

	rc = v4l2_set_control(driver->video_fd, surface->request_fd,
			      V4L2_CID_MPEG_VIDEO_H264_SPS,
			      &sps, sizeof(sps));
	if (rc < 0)
		return VA_STATUS_ERROR_OPERATION_FAILED;

	rc = v4l2_set_control(driver->video_fd, surface->request_fd,
			      V4L2_CID_MPEG_VIDEO_H264_SCALING_MATRIX,
			      &matrix, sizeof(matrix));
	if (rc < 0)
		return VA_STATUS_ERROR_OPERATION_FAILED;

	return VA_STATUS_SUCCESS;
}
