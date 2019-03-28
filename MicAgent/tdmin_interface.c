
#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>
#include "tdmin_interface.h"

#define ALSA_OK             0
#define ALSA_FAIL          -1

#define PB_ALSA_ERR(x)  printf x
#define PB_ALSA_INFO(x) printf x

#define PCM_HANDLE_CHECK(handle) do{  \
                                    if(NULL == handle) \
                                    {  PB_ALSA_ERR((PB_ALSA_DEBUG_TAG"%s -- Invail pcm handle fail\n",__FUNCTION__)); \
                                        return ALSA_FAIL;}  \
                                    }while(0)

/*---------------------------------------------------------------------------
 * Name
 *      Playback_Alsa_ReadPCM
 * Description      -
 * Input arguments  -
 * Output arguments -
 * Returns          -ok:write pcm size  fail or pause:-1
 *---------------------------------------------------------------------------*/

static int _alsa_read_pcm(PCMContainer_t *sndpcm) {
    ssize_t r;
    //size_t result = 0;
#define CH_NUM 1
#define FRAME_TIME 20//ms
#define SAMPLES_PER_MS 16
#define SAMPLE_BYTES 2
    size_t count = FRAME_TIME * SAMPLES_PER_MS;
    size_t realCount = 0;
    short *data = (short*)sndpcm->data_buf;

    while (count > 0) {
        r = snd_pcm_readi(sndpcm->handle, (void*)data, count);

        if (r == -EAGAIN || (r >= 0 && (size_t) r < count)) {
            snd_pcm_wait(sndpcm->handle, 1000);
            PB_ALSA_INFO((PB_ALSA_DEBUG_TAG
                                 "<<<<<<<<<<<<<<< short read >>>>>>>>>>>>>>>\n"));
        } else if (r == -EPIPE) {
            //snd_pcm_prepare(sndpcm->handle);
            snd_pcm_recover(sndpcm->handle, r, 0);
            PB_ALSA_ERR((PB_ALSA_DEBUG_TAG
                                "<<<<<<<<<<<<<<< Buffer Overrun >>>>>>>>>>>>>>>\n"));
        } else if (r == -ESTRPIPE) {
            PB_ALSA_ERR((PB_ALSA_DEBUG_TAG
                                "<<<<<<<<<<<<<<< Need suspend >>>>>>>>>>>>>>>\n"));
        } else if (r < 0) {
            PB_ALSA_ERR((PB_ALSA_DEBUG_TAG
                                "Error snd_pcm_readi: [%s]\n", snd_strerror(r)));
            return -1;
        }

        if (r > 0) {
            realCount += r;
            //result += r;
            count -= r;
            data += r;
        }
    }
    return CH_NUM * SAMPLE_BYTES * realCount;
}


/*---------------------------------------------------------------------------
 * Name
 *      Playback_Alsa_SetHWParams
 * Description      -
 * Input arguments  -
 * Output arguments -
 * Returns          - OK(0)  PARAMS ERR(-1)
 *---------------------------------------------------------------------------*/

static int _alsa_set_hw_params(PCMContainer_t *pcm_params) {
    snd_pcm_hw_params_t *hwparams;
    uint32_t exact_rate;
    uint32_t buffer_time, period_time;
    int err;

    PCM_HANDLE_CHECK(pcm_params->handle);

    /* Allocate the snd_pcm_hw_params_t structure on the stack. */
    snd_pcm_hw_params_alloca(&hwparams);

    /* Fill it with default values */
    err = snd_pcm_hw_params_any(pcm_params->handle, hwparams);
    if (err < 0) {
        PB_ALSA_ERR((PB_ALSA_DEBUG_TAG
                            "Error snd_pcm_hw_params_any : %s\n", snd_strerror(err)));
        goto ERR_SET_PARAMS;
    }

    /* Interleaved mode */
    err = snd_pcm_hw_params_set_access(pcm_params->handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (err < 0) {
        PB_ALSA_ERR((PB_ALSA_DEBUG_TAG
                            "Error snd_pcm_hw_params_set_access : %s\n", snd_strerror(err)));
        goto ERR_SET_PARAMS;
    }

    /* Set sample format */
    err = snd_pcm_hw_params_set_format(pcm_params->handle, hwparams, pcm_params->format);
    if (err < 0) {
        PB_ALSA_ERR((PB_ALSA_DEBUG_TAG
                            "Error snd_pcm_hw_params_set_format : %s\n", snd_strerror(err)));
        goto ERR_SET_PARAMS;
    }

    /* Set number of channels */
    err = snd_pcm_hw_params_set_channels(pcm_params->handle, hwparams, LE_SHORT(pcm_params->channels));
    if (err < 0) {
        PB_ALSA_ERR((PB_ALSA_DEBUG_TAG
                            "Error snd_pcm_hw_params_set_channels : %s\n", snd_strerror(err)));
        goto ERR_SET_PARAMS;
    }

    /* Set sample rate. If the exact rate is not supported */
    /* by the hardware, use nearest possible rate.         */
    exact_rate = LE_INT(pcm_params->sample_rate);
    err = snd_pcm_hw_params_set_rate_near(pcm_params->handle, hwparams, &exact_rate, 0);
    if (err < 0) {
        PB_ALSA_ERR((PB_ALSA_DEBUG_TAG
                            "Error snd_pcm_hw_params_set_rate_near : %s\n", snd_strerror(err)));
        goto ERR_SET_PARAMS;
    }
    if (LE_INT(pcm_params->sample_rate) != exact_rate) {
        PB_ALSA_ERR((PB_ALSA_DEBUG_TAG
                            "The rate %d Hz is not supported by your hardware.\n ==> Using %d Hz instead.\n",
                                    (int) (pcm_params->sample_rate), exact_rate));
    }
    err = snd_pcm_hw_params_get_buffer_time_max(hwparams, &buffer_time, 0);
    if (err < 0) {
        PB_ALSA_ERR((PB_ALSA_DEBUG_TAG"Error snd_pcm_hw_params_get_buffer_time_max : %s\n",snd_strerror(err)));
        goto ERR_SET_PARAMS;
    }

    PB_ALSA_ERR((PB_ALSA_DEBUG_TAG"snd_pcm_hw_params_get_buffer_time_max : %ul (us)\n",buffer_time));

    if (buffer_time > 200000)
        buffer_time = 200000;/*200000us = 200ms*/


    //if (buffer_time > 0)
    //    period_time = buffer_time / 4;

    //buffer_time = 32000 * 4;
    err = snd_pcm_hw_params_set_buffer_time_near(pcm_params->handle, hwparams, &buffer_time, 0);
    if (err < 0) {
        PB_ALSA_ERR((PB_ALSA_DEBUG_TAG
                            "Error snd_pcm_hw_params_set_buffer_time_near : %s\n", snd_strerror(err)));
        goto ERR_SET_PARAMS;
    }


    err = snd_pcm_hw_params_get_period_time_min	(hwparams, &period_time, 0);
    printf("period_time_min:%d, err:%d\n", period_time, err);
    err = snd_pcm_hw_params_get_period_time_max	(hwparams, &period_time, 0);
    printf("period_time_max:%d, err:%d\n", period_time, err);
    err = snd_pcm_hw_params_get_period_time	(hwparams, &period_time, 0);
    printf("period_time:%d, err:%d\n", period_time, err);

    period_time = 20000;//20ms
    err = snd_pcm_hw_params_set_period_time(pcm_params->handle, hwparams, period_time, 0);
    if (err < 0) {
        PB_ALSA_ERR((PB_ALSA_DEBUG_TAG
                            "Error snd_pcm_hw_params_set_period_time : %d, %s\n", period_time, snd_strerror(err)));
        goto ERR_SET_PARAMS;
    }

    /* Set hw params */
    err = snd_pcm_hw_params(pcm_params->handle, hwparams);
    if (err < 0) {
        PB_ALSA_ERR((PB_ALSA_DEBUG_TAG
                            "Error snd_pcm_hw_params: %s at line->%d\n", snd_strerror(err), __LINE__));
        goto ERR_SET_PARAMS;
    }

    snd_pcm_hw_params_get_period_size(hwparams, &pcm_params->chunk_size, 0);
    /*if (pcm_params->chunk_size != 1024) {//32ms
        PB_ALSA_ERR((PB_ALSA_DEBUG_TAG
                            "Only support 20ms package line->%d\n", __LINE__));
        goto ERR_SET_PARAMS;
    }*/
    snd_pcm_hw_params_get_buffer_size(hwparams, &pcm_params->buffer_size);
    if (pcm_params->chunk_size == pcm_params->buffer_size) {
        PB_ALSA_ERR((PB_ALSA_DEBUG_TAG
                            "Can't use period equal to buffer size (%lu == %lu)\n", pcm_params->chunk_size, pcm_params->buffer_size));
        goto ERR_SET_PARAMS;
    }

    PB_ALSA_ERR((PB_ALSA_DEBUG_TAG
                        "chunk_size is %lu, buffer size is %lu\n", pcm_params->chunk_size, pcm_params->buffer_size));

    /*bits per sample = bits depth*/
    pcm_params->bits_per_sample = 16;//snd_pcm_format_physical_width(pcm_params->format);

    /*bits per frame = bits depth * channels*/
    pcm_params->bits_per_frame = pcm_params->bits_per_sample * LE_SHORT(pcm_params->channels);

    /*chunk byte is a better size for each write or read for alsa*/
    pcm_params->chunk_bytes = pcm_params->chunk_size * pcm_params->bits_per_frame / 8;
    pcm_params->chunk_bytes_16 = pcm_params->chunk_bytes / 2;

    /* Allocate audio data buffer */
    pcm_params->data_buf = (uint8_t *) malloc(pcm_params->chunk_bytes);
    if (!pcm_params->data_buf) {
        PB_ALSA_ERR((PB_ALSA_DEBUG_TAG
                            "Error malloc: [data_buf] at line-> %d\n", __LINE__));
        goto ERR_SET_PARAMS;
    }
    pcm_params->data_buf_16 = (uint8_t *) malloc(pcm_params->chunk_bytes_16);
    if (!pcm_params->data_buf_16) {
        PB_ALSA_ERR((PB_ALSA_DEBUG_TAG
                            "Error malloc: [data_buf] at line-> %d\n", __LINE__));
        goto ERR_SET_PARAMS;
    }

    return 0;

    ERR_SET_PARAMS:
    if (NULL != pcm_params->data_buf) {
        free(pcm_params->data_buf);
        pcm_params->data_buf = NULL;
    }
    if (NULL != pcm_params->data_buf_16) {
        free(pcm_params->data_buf_16);
        pcm_params->data_buf_16 = NULL;
    }
    snd_pcm_close(pcm_params->handle);
    pcm_params->handle = NULL;
    return -1;
}

int _pcm_32_to_16(char *dst_buf, char *ori_buf, int ori_len, int fseek_bit) {
    char *p = ori_buf;
    char *q = dst_buf;
    unsigned int *temp;

    if (dst_buf == NULL || ori_buf == NULL) {
        printf("Err: u_test_pcm_32_to_16() buf is null!");
        return -1;
    }

    while (ori_len >= 4) {
        temp = (unsigned int *) p;
        *temp = (*temp << fseek_bit);
        *q = *(p + 2);
        *(q + 1) = *(p + 3);
        q += 2;
        p += 4;
        ori_len -= 4;
    }

    return 0;
}


int alsa_tdmin_init(PCMContainer_t *pcm_params) {

    int i4_ret = 0;

    if (pcm_params == NULL) {
        printf("PCMContainer handle == NULL!\n");
        return TDMIN_INV_ARG;
    }

    //pcm_params->format = TEST_TDMIC_FORMAT;
    //pcm_params->sample_rate = TEST_TDMIC_SAMPLERATE;
    //pcm_params->channels = TEST_TDMIC_CHANNLE;

    i4_ret = snd_pcm_open(&pcm_params->handle, TEST_TDMIC_PCM_RECORD_DEVICE_NAME, SND_PCM_STREAM_CAPTURE, 0);
    if (0 != i4_ret) {
        printf("snd_pcm_open failed! %d\n", i4_ret);
        return TDMIN_FAIL;
    }
    i4_ret = _alsa_set_hw_params(pcm_params);
    if (i4_ret != 0) {
        printf("u_alsa_set_hw_params failed!\n");
        return TDMIN_FAIL;
    }
    printf("Set alsa param OK, start read pcm!!!\n");

    return TDMIN_OK;
}

void alsa_tdmin_uninit(PCMContainer_t *pcm_params) {
    if (pcm_params == NULL) {
        printf("PCMContainer handle == NULL!\n");
        return;
    }
    if (NULL != pcm_params->data_buf) {
        free(pcm_params->data_buf);
        pcm_params->data_buf = NULL;
    }
    if (NULL != pcm_params->data_buf_16) {
        free(pcm_params->data_buf_16);
        pcm_params->data_buf_16 = NULL;
    }
    snd_pcm_close(pcm_params->handle);
    pcm_params->handle = NULL;
}

int alsa_read_tdmin_pcm(PCMContainer_t *pcm_params) {
    int i4_ret = 0;

    if (pcm_params == NULL) {
        printf("PCMContainer handle == NULL!\n");
        return TDMIN_INV_ARG;
    }

    i4_ret = _alsa_read_pcm(pcm_params);

    if (i4_ret == (int) pcm_params->chunk_bytes) {
        //i4_ret = _pcm_32_to_16(pcm_params->data_buf_16,pcm_params->data_buf,pcm_params->chunk_bytes,PCM_SEEK_BIT);

        return TDMIN_OK;
    } else {
        printf("TDMIN pcm read fail\n");
        return i4_ret;
    }

}

