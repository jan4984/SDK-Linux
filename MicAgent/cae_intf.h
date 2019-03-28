/**
 * @file    cae_intf.h
 * @brief   CAE Interface Header File
 *
 * @version 1.0
 * @date    2018/05/29
 *
 * @see
 *
 * History:
 * index    version        date            author        notes
 * 0        1.0            2018/05/29      tiange      Create this file
 */

#ifndef __CAE_INTF_H__
#define __CAE_INTF_H__

typedef void * CAE_HANDLE;

typedef void (*cae_ivw_fn)(short angle, short channel, float power, short CMScore, short beam, void *userData);

typedef void (*cae_audio_fn)(const void *audioData, unsigned int audioLen, int param1, const void *param2, void *userData);


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

//int CAENew(CAE_HANDLE *cae, const char *resPath);
int CAENew(CAE_HANDLE *cae, const char *resPath, cae_ivw_fn ivwCb, cae_audio_fn audioCb, const char *param, void *userData);

int CAEAudioWrite(CAE_HANDLE cae, const void *audioData, unsigned int audioLen);

int CAEResetEng(CAE_HANDLE cae);

int CAESetRealBeam(CAE_HANDLE cae, int beam);

int CAEDestroy(CAE_HANDLE cae);

int CAEExtract16K(void *in,int in_framelen, int channel, void* out, int *out_framelen);

void runstep_wk(CAE_HANDLE * self, int wkid, const short * pcm);

void CAEReset(CAE_HANDLE * cae);

typedef void (*dataparserT)(CAE_HANDLE *self, int data);

void CAEWrite_Stream(CAE_HANDLE cae, const void *audioData, unsigned int audioLen);

void scorecompare(CAE_HANDLE* self, short* beam);

char* CAEGetVersion();

#ifdef __cplusplus
}

#endif /* __cplusplus */

#endif /* __CAE_INTF_H__ */
