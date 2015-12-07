#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#include "sample_comm.h"
#include "mpi_ive.h"
#include "hi_tde_type.h"
#include "hi_tde_api.h"
#define VENC_OPEN 0
#define PICWIDTH 320
#define PICHEIGHT 240

typedef struct  VENCPARA{
    VENC_GRP VencGrp;
    VENC_CHN VencChn;
}venc_para;

VIDEO_NORM_E gs_enNorm = VIDEO_ENCODING_MODE_PAL;
VO_INTF_TYPE_E  g_enVoIntfType = VO_INTF_CVBS;
HI_U32 gs_u32ViFrmRate = 0;

HI_BOOL g_bStopSignal = HI_FALSE;


SAMPLE_VI_CONFIG_S g_stViChnConfig =
{
    APTINA_AR0130_DC_720P_30FPS,
    VIDEO_ENCODING_MODE_AUTO,

    ROTATE_180,
    VI_CHN_SET_NORMAL
};

HI_S32 COMM_VENC_SnapProcess(VENC_GRP VencGrp, VENC_CHN VencChn)
{
    struct timeval TimeoutVal;
    fd_set read_fds;
    HI_S32 s32VencFd;
    VENC_CHN_STAT_S stStat;
    VENC_STREAM_S stStream;
    HI_S32 s32Ret;
    
    /******************************************
     step 1:  Regist Venc Channel to VencGrp
    ******************************************/
    s32Ret = HI_MPI_VENC_RegisterChn(VencGrp, VencChn);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VENC_RegisterChn faild with %#x!\n", s32Ret);
        return HI_FAILURE;
    }
    
    /******************************************
     step 2:  Start Recv Venc Pictures
    ******************************************/
    s32Ret = HI_MPI_VENC_StartRecvPic(VencChn);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VENC_StartRecvPic faild with%#x!\n", s32Ret);
        return HI_FAILURE;
    }
    /******************************************
     step 3:  recv picture
    ******************************************/
    while(!g_bStopSignal)
    {
      s32VencFd = HI_MPI_VENC_GetFd(VencChn);
      if (s32VencFd < 0)
      {
         SAMPLE_PRT("HI_MPI_VENC_GetFd faild with%#x!\n", s32VencFd);
          return HI_FAILURE;
      }

      FD_ZERO(&read_fds);
      FD_SET(s32VencFd, &read_fds);
      
      TimeoutVal.tv_sec  = 1000;
      TimeoutVal.tv_usec = 0;
      s32Ret = select(s32VencFd+1, &read_fds, NULL, NULL, &TimeoutVal);
      //s32Ret = select(s32VencFd+1, &read_fds, NULL, NULL, NULL);
      printf("get Frame!\n");
      if (s32Ret < 0) 
      {
          SAMPLE_PRT("snap select failed!\n");
          return HI_FAILURE;
      }
      else if (0 == s32Ret) 
      {
          SAMPLE_PRT("snap time out!\n");
          return HI_FAILURE;
      }
      else
      {
          if (FD_ISSET(s32VencFd, &read_fds))
          {
              s32Ret = HI_MPI_VENC_Query(VencChn, &stStat);
              if (s32Ret != HI_SUCCESS)
              {
                  SAMPLE_PRT("HI_MPI_VENC_Query failed with %#x!\n", s32Ret);
                  return HI_FAILURE;
              }

              stStream.pstPack = (VENC_PACK_S*)malloc(sizeof(VENC_PACK_S) * stStat.u32CurPacks);
              if (NULL == stStream.pstPack)
              {
                  SAMPLE_PRT("malloc memory failed!\n");
                  return HI_FAILURE;
              }

              stStream.u32PackCount = stStat.u32CurPacks;
              s32Ret = HI_MPI_VENC_GetStream(VencChn, &stStream, HI_TRUE);
              if (HI_SUCCESS != s32Ret)
              {
                  SAMPLE_PRT("HI_MPI_VENC_GetStream failed with %#x!\n", s32Ret);
                  free(stStream.pstPack);
                  stStream.pstPack = NULL;
                  return HI_FAILURE;
              }

              s32Ret = SAMPLE_COMM_VENC_SaveSnap(&stStream);
              if (HI_SUCCESS != s32Ret)
              {
                  SAMPLE_PRT("HI_MPI_VENC_GetStream failed with %#x!\n", s32Ret);
                  free(stStream.pstPack);
                  stStream.pstPack = NULL;
                  return HI_FAILURE;
              }

              s32Ret = HI_MPI_VENC_ReleaseStream(VencChn, &stStream);
              if (s32Ret)
              {
                  SAMPLE_PRT("HI_MPI_VENC_ReleaseStream failed with %#x!\n", s32Ret);
                  free(stStream.pstPack);
                  stStream.pstPack = NULL;
                  return HI_FAILURE;
              }

              free(stStream.pstPack);
              stStream.pstPack = NULL;
          }
        }
    }
    /******************************************
     step 3:  stop recv picture
    ******************************************/
    s32Ret = HI_MPI_VENC_StopRecvPic(VencChn);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("HI_MPI_VENC_StopRecvPic failed with %#x!\n",  s32Ret);
        return HI_FAILURE;
    }

    /******************************************
     step 4:  UnRegister
    ******************************************/
    s32Ret = HI_MPI_VENC_UnRegisterChn(VencChn);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("HI_MPI_VENC_UnRegisterChn failed with %#x!\n", s32Ret);
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}




HI_VOID save_jpg(venc_para *para)
{

    COMM_VENC_SnapProcess(para->VencGrp,para->VencChn);

}

/******************************************************************************
* function : show usage
******************************************************************************/
HI_VOID SAMPLE_IVE_Canny_Usage(char *sPrgNm)
{
    printf("This example demonstrates the usage of Canny detector, also with the DMA and TDE zoom !\n");
    printf("Usage : %s \n", sPrgNm);
    
    return;
}
//////////////////////////////////////////////////////////////////////////////////////////

HI_S32 COMM_VENC_Start(VENC_GRP VencGrp,
    VENC_CHN VencChn,
    VENC_CHN_ATTR_S *stAttr,
    SIZE_S *stPicSize)
{
  HI_S32  s32Ret;
  MPP_CHN_S stSrcChn,stDestChn;
  VENC_CHN_ATTR_S stVencChnAttr;

  memset(&stVencChnAttr,0,sizeof(stVencChnAttr));
  s32Ret=HI_MPI_VENC_CreateGroup(VencGrp);
  if(HI_SUCCESS!=s32Ret)
  {
    printf("HI_MPI_VENC_CreateGroup[%d] failed with %#x!\n",VencGrp,s32Ret);
    return HI_FAILURE;
  }
  stAttr->stVeAttr.enType=PT_H264;
  stAttr->stVeAttr.stAttrH264e.u32PicWidth=stPicSize->u32Width;
  stAttr->stVeAttr.stAttrH264e.u32PicHeight=stPicSize->u32Height;
  stAttr->stVeAttr.stAttrH264e.u32MaxPicWidth=stPicSize->u32Width;
  stAttr->stVeAttr.stAttrH264e.u32MaxPicHeight=stPicSize->u32Height;
  stAttr->stVeAttr.stAttrH264e.u32BufSize=stPicSize->u32Width*stPicSize->u32Height*2;
  stAttr->stVeAttr.stAttrH264e.u32Profile=0;
  stAttr->stVeAttr.stAttrH264e.bByFrame=HI_TRUE;
  stAttr->stVeAttr.stAttrH264e.bField=HI_FALSE;
  stAttr->stVeAttr.stAttrH264e.bMainStream=HI_TRUE;
  stAttr->stVeAttr.stAttrH264e.u32Priority=0;
  stAttr->stVeAttr.stAttrH264e.bVIField=HI_FALSE;

  stAttr->stRcAttr.enRcMode=VENC_RC_MODE_H264CBR;
  stAttr->stRcAttr.stAttrH264Cbr.u32Gop=25;
  stAttr->stRcAttr.stAttrH264Cbr.u32StatTime=1;
  stAttr->stRcAttr.stAttrH264Cbr.u32ViFrmRate=25;
  stAttr->stRcAttr.stAttrH264Cbr.fr32TargetFrmRate=25;
  stAttr->stRcAttr.stAttrH264Cbr.u32BitRate=1024*4;

  VencChn=0;
  s32Ret=HI_MPI_VENC_CreateChn(VencChn,stAttr);
  if(HI_SUCCESS!=s32Ret)
  {
    printf("HI_MPI_VENC_CreateChn[%d] failed with %#x!\n",VencChn,s32Ret);
    return s32Ret;
  }

  s32Ret=HI_MPI_VENC_RegisterChn(VencGrp,VencChn);
  if(HI_SUCCESS!=s32Ret)
  {
    printf("HI_MPI_VENC_RegisterChn fail with %#x!\n",s32Ret);
    return s32Ret;
  }

  s32Ret=HI_MPI_VENC_StartRecvPic(VencChn);
  if(HI_SUCCESS!=s32Ret)
  {
    printf("HI_MPI_VENC_StartRecvPic fail with %#x!\n",s32Ret);
    return HI_FAILURE;
  }

  return HI_SUCCESS;
}

///////////////////////////////////////////////////////////////////////
HI_S32 COMM_VENC_StartGetStream()
{
  HI_S32 s32Ret;
  VENC_CHN VencChn=0;
  VENC_CHN_ATTR_S stVencChnAttr;
  FILE *pFile;
  HI_S32 VencFd;
  struct timeval TimeoutVal;
  fd_set read_fds;
  HI_CHAR aszFileName[64];
  VENC_CHN_STAT_S stStat;
  VENC_STREAM_S stStream;

  VIDEO_FRAME_INFO_S pstVideoFrame;

  HI_CHAR pcMmzName[64]={};
  MPP_CHN_S chn;
  chn.enModId=HI_ID_VPSS;
  chn.s32DevId=0;
  chn.s32ChnId=0;


  s32Ret=HI_MPI_VENC_GetChnAttr(VencChn,&stVencChnAttr);
  if(s32Ret!=HI_SUCCESS)
  {
    printf("HI_MPI_VENC_GetChnAttr chn[%d] failed with %#x!\n",VencChn,s32Ret);
    return s32Ret;
  }
  sprintf(aszFileName,"stream_chn%d.h264",0);
  pFile=fopen(aszFileName,"wb");
  if(!pFile)
  {
    printf("open file %s failed!\n",aszFileName);
    return NULL;
  }
  VencFd=HI_MPI_VENC_GetFd(0);
  if(VencFd<0)
  {
    printf("HI_MPI_VENC_GetFd failed with %#x!\n",VencFd);
    return NULL;
  }
  //int a=600000;
  while(!g_bStopSignal){
   // a--;
    FD_ZERO(&read_fds);
    FD_SET(VencFd,&read_fds);
    TimeoutVal.tv_sec=2;
    TimeoutVal.tv_usec=0;


    s32Ret=select(VencFd+1,&read_fds,NULL,NULL,&TimeoutVal);
   
    if(s32Ret<0)
    {
      printf("select failed!\n");
      break;
    }
    else if(s32Ret == 0)
    {
      printf("get venc stream time out,exit thread\n");
      continue;
    }
    else
    {
      FD_ISSET(VencFd,&read_fds);
      memset(&stStream,0,sizeof(stStream));
      s32Ret=HI_MPI_VENC_Query(0,&stStat);
      if(HI_SUCCESS!=s32Ret)
      {
        printf("HI_MPI_VENC_Query chn failed with %#x!\n",s32Ret);
        break;
      }

      stStream.pstPack=(VENC_PACK_S*)malloc(sizeof(VENC_PACK_S)*stStat.u32CurPacks);
      if(NULL==stStream.pstPack)
      {
        printf("malloc stream pack failed!\n");
        break;
      }
      stStream.u32PackCount=stStat.u32CurPacks;
      s32Ret=HI_MPI_VENC_GetStream(0,&stStream,HI_TRUE);
      printf("getstream!\n");
      if(HI_SUCCESS!=s32Ret)
      {
        free(stStream.pstPack);
        stStream.pstPack=NULL;
        printf("HI_MPI_VENC_GETStream failed with %#x!\n",s32Ret);
        break;
      }
      HI_S32 i=0;
      for(i=0;i<stStream.u32PackCount;i++)
      {
        fwrite(stStream.pstPack[i].pu8Addr[0],
          stStream.pstPack[i].u32Len[0],1,pFile);
        fflush(pFile);
        if(stStream.pstPack[i].u32Len[1]>0)
        {
          fwrite(stStream.pstPack[i].pu8Addr[1],
            stStream.pstPack[i].u32Len[1],1,pFile);
          fflush(pFile);
        }
      }

      s32Ret=HI_MPI_VENC_ReleaseStream(0,&stStream);
      if(HI_SUCCESS!=s32Ret)
      {
        free(stStream.pstPack);
        stStream.pstPack=NULL;
        break;
      }
      free(stStream.pstPack);

      stStream.pstPack=NULL;
    }
  //  fclose(pFile);
  }
    fclose(pFile);
}

/////////////////////////////////////////////////////////////////////////////




/******************************************************************************
* function : to process abnormal case
******************************************************************************/
HI_VOID SAMPLE_IVE_Canny_HandleSig(HI_S32 signo)
{
    if (SIGINT == signo || SIGTSTP == signo)
    {
        SAMPLE_COMM_SYS_Exit();
        printf("\033[0;31mprogram termination abnormally!\033[0;39m\n");
    }
    exit(-1);
}

HI_S32 SAMPLE_IVE_UnBindViVo(VO_DEV VoDev, VO_CHN VoChn)
{
    MPP_CHN_S stDestChn;

    stDestChn.enModId   = HI_ID_VOU;
    stDestChn.s32DevId  = VoDev;
    stDestChn.s32ChnId  = VoChn;

    return HI_MPI_SYS_UnBind(NULL, &stDestChn);
}

HI_S32 SAMPLE_IVE_BindViVo(VI_CHN ViChn, VO_DEV VoDev, VO_CHN VoChn)
{
    MPP_CHN_S stSrcChn, stDestChn;

    stSrcChn.enModId    = HI_ID_VIU;
    stSrcChn.s32DevId   = 0;
    stSrcChn.s32ChnId   = ViChn;

    stDestChn.enModId   = HI_ID_VOU;
    stDestChn.s32ChnId  = VoChn;
    stDestChn.s32DevId  = VoDev;

    return HI_MPI_SYS_Bind(&stSrcChn, &stDestChn);
}




/******************************************************************************
* function : edge detection using Canny detector
******************************************************************************/
HI_S32  SAMPLE_IVE_Canny()
{
    HI_U32 u32ViChnCnt = 2;
    VB_CONF_S stVbConf;
    VO_DEV VoDev = SAMPLE_VO_DEV_DSD0;
    VO_CHN VoChn = 0;
    VI_CHN ViChn = 0;
    VO_PUB_ATTR_S stVoPubAttr;
    HI_U32 i,j;

    HI_S32 s32Out, s32Ret = HI_SUCCESS;
    SIZE_S stSize;
    VI_EXT_CHN_ATTR_S stExtChnAttr;
    VI_CHN ExtChn = 1;

    VIDEO_FRAME_INFO_S stViFrameInfo, stCannyFrameInfo, stTDEFrameInfo;
    VIDEO_FRAME_INFO_S stThreshFrameInfo;
    HI_U32 u32Width, u32Height, u32Stride;

    HI_U32 u32BlkSize;
    VB_BLK vbBlkHandle,vbBlkHandle2;
    VB_POOL vbPoolHandle = VB_INVALID_POOLID;  
    
    HI_U32 u32Depth = 1;

    IVE_HANDLE iveFilterHdl, iveCannyHdl,iveThreshHdl, iveDMAHdl,iveSubHdl;
    IVE_SRC_INFO_S stFilterSrc, stCannySrc, stDMASrc,stThreshSrc,BSrc,CSrc;
    IVE_MEM_INFO_S stFilterDst, stCannyDstMag, stDMADst,stThreshDst,stSubDst;
    HI_VOID *pVirFilterDst, *pVirCannyDstMag, *pVirDMASrc,*pVirThreshDst,*pVirSubDst;

    IVE_THRESH_CTRL_S stThreshCtrl;

    IVE_FILTER_CTRL_S stFilterCtrl;
    IVE_CANNY_CTRL_S stCannyCtrl;

    HI_U16 *pCannyDstMag,*pThreshDstMag;
    HI_U8 *pDMASrc;
    HI_BOOL bInstant = HI_FALSE;
    HI_BOOL bFinish;
    HI_BOOL bBlock = HI_TRUE;

    /* for TDE */
    TDE_HANDLE handle;
    TDE2_MB_S stMB = {0};
    TDE2_MB_S stMBOut = {0};
    TDE2_RECT_S stMBRect;
    TDE2_RECT_S stMBRect2;
    HI_BOOL bSync = HI_FALSE;
    HI_U32 u32TimeOut = 10;
    TDE2_MBOPT_S stMbOpt= {0};
    FILE *fp = stdout;
    FILE *f;
    VENC_CHN_STAT_S stStat;

    venc_para  VencPara;

    pthread_t vencThread;
    pid_t pid2;

    VENC_GRP VencGrp=0;
    VENC_CHN VencChn=0;
    VENC_CHN_ATTR_S stAttr;
    SIZE_S stPicSize;
    stPicSize.u32Height=PICHEIGHT;
    stPicSize.u32Width=PICWIDTH;
    memset(&stAttr,0,sizeof(VENC_CHN_ATTR_S));

    /******************************************
      step  1: init global  variable
    ******************************************/
    gs_u32ViFrmRate = (VIDEO_ENCODING_MODE_PAL == gs_enNorm)? 25: 30;
    memset(&stVbConf,0,sizeof(VB_CONF_S));
     
    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm, PIC_HD720,
                             SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
    stVbConf.u32MaxPoolCnt = 128;
     
    /*ddr0 video buffer*/
    stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt = u32ViChnCnt * 8;
     
    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm, PIC_D1,
                             SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
    stVbConf.astCommPool[1].u32BlkSize = u32BlkSize;
    stVbConf.astCommPool[1].u32BlkCnt = 8;
     
     
    /******************************************
     step 2: mpp system init.
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        goto END_0;
    }

    stSize.u32Width=PICWIDTH;
    stSize.u32Height=PICHEIGHT;


    s32Ret = SAMPLE_COMM_VENC_SnapStart(VencGrp, VencChn, &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start snap failed!\n");
    }


#if VENC_OPEN
    s32Ret = COMM_VENC_Start(VencGrp,VencChn,&stAttr,&stPicSize);
    if(HI_SUCCESS!=s32Ret)
    {
      printf("COMM_VENC_Start fail with err code %#x\n",s32Ret);
      return 0;
    } 
#endif   
     /******************************************
      step 3: start vi dev & chn to capture
     ******************************************/
     s32Ret = SAMPLE_COMM_VI_StartVi(&g_stViChnConfig);
     if (HI_SUCCESS != s32Ret)
     {
         SAMPLE_PRT("start vi failed!\n");
         goto END_0;
     }
     
     stExtChnAttr.enPixFormat = SAMPLE_PIXEL_FORMAT;
     stExtChnAttr.s32BindChn = ViChn;
     stExtChnAttr.stDestSize.u32Width = PICWIDTH;
     stExtChnAttr.stDestSize.u32Height = PICHEIGHT;
     //stExtChnAttr.stDestSize.u32Width = 320;
     //stExtChnAttr.stDestSize.u32Height = 240;
     stExtChnAttr.s32FrameRate = -1;
     stExtChnAttr.s32SrcFrameRate = -1;
     
     s32Ret = HI_MPI_VI_SetExtChnAttr(ExtChn, &stExtChnAttr);
     if (HI_SUCCESS != s32Ret)
     {
         SAMPLE_PRT("HI_MPI_VI_SetExtChnAttr failed!\n");
         goto END_1;
     }
    // printf("SetExtChnAttr success!\n");
     s32Ret = HI_MPI_VI_EnableChn(ViChn);
     s32Ret = HI_MPI_VI_EnableChn(ExtChn);
     if (HI_SUCCESS != s32Ret)
     {
         SAMPLE_PRT("HI_MPI_VI_EnableChn failed!\n");
         goto END_2;
     }
     
     /******************************************
      step 4: start vpss and vi bind vpss (subchn needn't bind vpss in this mode)
     ******************************************/
     s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, PIC_HD720, &stSize);
     if (HI_SUCCESS != s32Ret)
     {
         SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
         goto END_2;
     }
     
     /******************************************
     step 5: start VO SD0 (bind * vi )
     ******************************************/
      stVoPubAttr.enIntfType = g_enVoIntfType;
     if(VO_INTF_BT1120 == g_enVoIntfType)
     {
         stVoPubAttr.enIntfSync = VO_OUTPUT_720P50;
         //gs_u32ViFrmRate = 50;
     }
     else
     {
         stVoPubAttr.enIntfSync = VO_OUTPUT_PAL;
     }
     stVoPubAttr.u32BgColor = 0x000000ff;
     /* In HD, this item should be set to HI_FALSE */
     stVoPubAttr.bDoubleFrame = HI_FALSE;
     s32Ret = SAMPLE_COMM_VO_StartDevLayer(VoDev, &stVoPubAttr, gs_u32ViFrmRate);
     if (HI_SUCCESS != s32Ret)
     {
        SAMPLE_PRT("SAMPLE_COMM_VO_StartDevLayer failed!\n");
        goto END_2;
     }
     
    u32Width = PICWIDTH;
    u32Height = PICHEIGHT;
      //u32Width = 320;
      //u32Height = 240;
    for(i=0; i < 4; i++)
    {
        VO_CHN_ATTR_S stVoChnAttr = {0};        

        stVoChnAttr.stRect.s32X = i % 2 * u32Width / 2;   
        stVoChnAttr.stRect.s32Y = i / 2 * u32Height / 2;

        stVoChnAttr.stRect.u32Width = u32Width / 2;
        stVoChnAttr.stRect.u32Height = u32Height /2 ;
        HI_MPI_VO_SetChnAttr(VoDev, VoChn + i, &stVoChnAttr);
        HI_MPI_VO_EnableChn(VoDev, VoChn + i);

        //SAMPLE_IVE_UnBindViVo(VoDev, VoChn + i);
        //SAMPLE_IVE_BindViVo(ExtChn, VoDev, VoChn + i);    
    }
    SAMPLE_IVE_UnBindViVo(VoDev, VoChn);
    SAMPLE_IVE_BindViVo(ExtChn, VoDev, VoChn); 

    vbPoolHandle = HI_MPI_VB_CreatePool(u32Width * u32Height * 2, 8, NULL);
    if ( VB_INVALID_POOLID == vbPoolHandle ) 
    { 
       SAMPLE_PRT("HI_MPI_VB_CreatePool err\n"); 
       goto END_3; 
    } 
    
    s32Ret = HI_MPI_VI_SetFrameDepth(ExtChn, u32Depth);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("set Frame depth err:0x%x\n", s32Ret);
    }
   // printf("HI_MPI_VI_SetFrameDepth success!\n");
    VencPara.VencChn=0;
    VencPara.VencGrp=0;
    pid2 = pthread_create(&vencThread,0,(HI_VOID*)save_jpg,&VencPara);
#if VENC_OPEN
    pid2 = pthread_create(&vencThread,0,(HI_VOID*)COMM_VENC_StartGetStream,HI_NULL);
#endif
    memset(&BSrc,0,sizeof(IVE_SRC_INFO_S));
    memset(&CSrc,0,sizeof(IVE_SRC_INFO_S));
    f=fopen("/home/a.txt","w");    
    while (!g_bStopSignal)
    {
      time_t now;   
      struct tm *timenow;   
      char strtemp[255];   
        
      time(&now);   
      timenow = localtime(&now);   
      printf("recent time is : %s /n", asctime(timenow)); 
         //得到一帧图像
        //printf("while(!g_bStopSignal)\n");
        s32Ret = HI_MPI_VI_GetFrameTimeOut(ExtChn, &stViFrameInfo, 0);
        if(HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("HI_MPI_VI_GetFrame failed!\n");
         //   goto END_TDE;
        }
    //    printf("HI_MPI_VI_GetFrameTimeOut success!\n");
        
        
        //将图像进行滤波
        /* First step of Canny: Gaussian filter */
        u32Width = stViFrameInfo.stVFrame.u32Width;
        u32Height = stViFrameInfo.stVFrame.u32Height;
        u32Stride = stViFrameInfo.stVFrame.u32Stride[0];
        stFilterSrc.u32Width = u32Width;
        stFilterSrc.u32Height = u32Height;
        stFilterSrc.enSrcFmt = IVE_SRC_FMT_SINGLE;
        stFilterSrc.stSrcMem.u32Stride = u32Stride;
        stFilterSrc.stSrcMem.u32PhyAddr = stViFrameInfo.stVFrame.u32PhyAddr[0];

        stFilterCtrl.as8Mask[0]          = 3;
        stFilterCtrl.as8Mask[1]          = 9;
        stFilterCtrl.as8Mask[2]          = 3;
        stFilterCtrl.as8Mask[3]          = 8;
        stFilterCtrl.as8Mask[4]          = 18;
        stFilterCtrl.as8Mask[5]          = 8;
        stFilterCtrl.as8Mask[6]          = 3;
        stFilterCtrl.as8Mask[7]          = 9;
        stFilterCtrl.as8Mask[8]          = 3;    
        stFilterCtrl.u8Norm              = 6;

        stFilterDst.u32Stride = stFilterSrc.stSrcMem.u32Stride;
     
        s32Ret = HI_MPI_SYS_MmzAlloc_Cached(&stFilterDst.u32PhyAddr, &pVirFilterDst, "user", HI_NULL,
                                        stFilterDst.u32Stride * stFilterSrc.u32Height);
     
        if(HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("HI_MPI_SYS_MmzAlloc_Cached return 0x%x!\n", s32Ret);
            goto END_4;
        }
     //   printf("HI_MPI_SYS_MmzAlloc_Cached\n");
        s32Ret = HI_MPI_IVE_FILTER(&iveFilterHdl, &stFilterSrc, &stFilterDst, &stFilterCtrl, bInstant);
        if(HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("HI_MPI_IVE_FILTER return 0x%x!\n", s32Ret);
            goto END_5;
        }
      //  printf("HI_MPI_IVE_FILTER\n");
        //得到滤波图像后与前一帧图像相减得到图像3
        if(0==BSrc.stSrcMem.u32PhyAddr)
        {
            BSrc.enSrcFmt=IVE_SRC_FMT_SINGLE;
            BSrc.u32Height=u32Height;
            BSrc.u32Width=u32Width;
            BSrc.stSrcMem.u32Stride = stFilterDst.u32Stride;
            BSrc.stSrcMem.u32PhyAddr = stFilterDst.u32PhyAddr;
            system("clear");
            continue;
        }
        CSrc.enSrcFmt=IVE_SRC_FMT_SINGLE;
        CSrc.u32Height=u32Height;
        CSrc.u32Width=u32Width;
        CSrc.stSrcMem.u32Stride = stFilterDst.u32Stride;
        CSrc.stSrcMem.u32PhyAddr = stFilterDst.u32PhyAddr;

        stSubDst.u32Stride = CSrc.stSrcMem.u32Stride;
     
        s32Ret = HI_MPI_SYS_MmzAlloc_Cached(&stSubDst.u32PhyAddr, &pVirSubDst, "user", HI_NULL,
                                        CSrc.stSrcMem.u32Stride * CSrc.u32Height);
     
        if(HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("HI_MPI_SYS_MmzAlloc_Cached return 0x%x!\n", s32Ret);
            return HI_FALSE;
            goto END_4;
        }
        s32Ret = HI_MPI_IVE_SUB(&iveSubHdl, &CSrc, &BSrc, &stSubDst, IVE_SUB_OUT_FMT_ABS, bInstant);
        if(HI_SUCCESS != s32Ret)
        {
            printf("HI_MPI_IVE_SUB fail with err code %#x!\n",s32Ret);
            return HI_FALSE;
        }
        //释放前一帧图像，前一帧图像结构体等于当前图像
        s32Out = HI_MPI_SYS_MmzFree(BSrc.stSrcMem.u32PhyAddr, NULL);
        if(HI_SUCCESS != s32Out)
        {
            SAMPLE_PRT("HI_MPI_SYS_MmzFree return 0x%x!\n", s32Out);
        }
        BSrc.stSrcMem.u32PhyAddr=CSrc.stSrcMem.u32PhyAddr;
        BSrc.stSrcMem.u32Stride=CSrc.stSrcMem.u32Stride;
        BSrc.u32Height=CSrc.u32Height;
        BSrc.u32Width=CSrc.u32Width;
        BSrc.enSrcFmt=CSrc.enSrcFmt;
        //将图像3进行二值化处理得到图像

        stThreshSrc.u32Width = CSrc.u32Width;
        stThreshSrc.u32Height =CSrc.u32Height;
        stThreshSrc.enSrcFmt = IVE_SRC_FMT_SINGLE;
        stThreshSrc.stSrcMem.u32Stride = stSubDst.u32Stride;
        stThreshSrc.stSrcMem.u32PhyAddr = stSubDst.u32PhyAddr;
        
        stThreshCtrl.enOutFmt=IVE_THRESH_OUT_FMT_BINARY;
        stThreshCtrl.u32Thresh=30;
        stThreshCtrl.u32MinVal=0;
        stThreshCtrl.u32MaxVal=255;


        stThreshDst.u32Stride = stThreshSrc.stSrcMem.u32Stride;
        //s32Ret = HI_MPI_SYS_MmzAlloc_Cached(&stThreshDst.u32PhyAddr, &pVirThreshDst, "user", HI_NULL,
        //                       stThreshDst.u32Stride * stThreshSrc.u32Height * 2);
        s32Ret = HI_MPI_SYS_MmzAlloc_Cached(&stThreshDst.u32PhyAddr, &pVirThreshDst, "user", HI_NULL,
                               stThreshDst.u32Stride * stThreshSrc.u32Height *2);
        if(HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("HI_MPI_SYS_MmzAlloc_Cached return 0x%x!\n", s32Ret);
           goto END_5;
        }
       // printf("HI_MPI_SYS_MmzAlloc_Cached\n");
        s32Ret = HI_MPI_IVE_THRESH(&iveThreshHdl, &stThreshSrc, &stThreshDst, &stThreshCtrl, bInstant);
        if(HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("HI_MPI_IVE_CANNY return 0x%x!\n", s32Ret);
            goto END_6;
        }

        //pThreshDstMag=(HI_U16*)HI_MPI_SYS_Mmap(stThreshDst.u32PhyAddr,stThreshDst.u32Stride*stThreshSrc.u32Height*2);
        stDMASrc.u32Width=stThreshSrc.u32Width;
        stDMASrc.u32Height=stThreshSrc.u32Height;
        stDMASrc.enSrcFmt=IVE_SRC_FMT_SINGLE;
        stDMASrc.stSrcMem.u32Stride=stThreshSrc.stSrcMem.u32Stride;
        stDMASrc.stSrcMem.u32PhyAddr=stThreshDst.u32PhyAddr;
        pVirDMASrc=pVirThreshDst;


        pThreshDstMag=(HI_U16*)HI_MPI_SYS_Mmap(stThreshDst.u32PhyAddr,stThreshDst.u32Stride*stThreshSrc.u32Height*2);
        
       


#if 1
        //pThreshDstMag=(HI_U16*)HI_MPI_SYS_Mmap(stThreshDst.u32PhyAddr,stThreshDst.u32Stride*stThreshSrc.u32Height*2);
        int num=0;
        for(i=0;i<stThreshSrc.u32Height;++i)
        {
          //HI_U16 *ptrThresh=pThreshDstMag + i * stThreshDst.u32Stride;
          HI_U8 *ptrThresh=(HI_U8*)pThreshDstMag + i * stThreshDst.u32Stride;
          for(j=0;j<stThreshSrc.u32Width;++j)
          {
             // fprintf(f," %x ",ptrThresh[j]);
              // fprintf(fp, "%-2x",*pu8Addr);
              if(ptrThresh[j]!=0)
              {
                num++;
              }
          }
        //  fprintf(f,"\n");
        }
        
          fprintf(fp,"num=%d,%f\n",num,(float)num/(float)(PICWIDTH*PICHEIGHT));
        // fprintf(f,"\n=====================================================================\n");
          if((float)num/(float)(PICWIDTH*PICHEIGHT)>0.5){
            s32Ret=HI_MPI_VENC_SendFrame(0,&stViFrameInfo);
            printf("send frame!\n");
            if(HI_SUCCESS!=s32Ret)
            {
              printf("HI_MPI_VENC_SendFrame fail with err code %#x!\n",s32Ret);
            }
          }
#endif

#if VENC_OPEN
     //   memset(&stThreshFrameInfo,0,sizeof(VIDEO_FRAME_INFO_S));
        stThreshFrameInfo.stVFrame.u32Field = VIDEO_FIELD_FRAME;
        stThreshFrameInfo.stVFrame.enPixelFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
        stThreshFrameInfo.stVFrame.u32Height = stThreshSrc.u32Height;
        stThreshFrameInfo.stVFrame.u32Width  = stThreshSrc.u32Width;
        stThreshFrameInfo.stVFrame.u32Stride[0] = stThreshDst.u32Stride;
        stThreshFrameInfo.stVFrame.u32Stride[1] = stThreshDst.u32Stride;
        stThreshFrameInfo.stVFrame.u32Stride[2] = 0;
        stThreshFrameInfo.stVFrame.u32TimeRef = 0;
        stThreshFrameInfo.stVFrame.u64pts = 0;

        stThreshFrameInfo.stVFrame.u16OffsetTop = 0;
        stThreshFrameInfo.stVFrame.u16OffsetBottom = 0;
        stThreshFrameInfo.stVFrame.u16OffsetLeft = 0;
        stThreshFrameInfo.stVFrame.u16OffsetRight = 0;

        vbBlkHandle = HI_MPI_VB_GetBlock(vbPoolHandle, stThreshDst.u32Stride * stThreshSrc.u32Height * 2, HI_NULL);
        stThreshFrameInfo.u32PoolId = HI_MPI_VB_Handle2PoolId(vbBlkHandle);
        stThreshFrameInfo.stVFrame.u32PhyAddr[0] = HI_MPI_VB_Handle2PhysAddr( vbBlkHandle );
        //stThreshFrameInfo.stVFrame.u32PhyAddr[1] = stThreshFrameInfo.stVFrame.u32PhyAddr[0] + u32Width * u32Height;
        stThreshFrameInfo.stVFrame.u32PhyAddr[1] = stThreshFrameInfo.stVFrame.u32PhyAddr[0];
        stThreshFrameInfo.stVFrame.u32PhyAddr[2] = HI_NULL;
   
        stThreshFrameInfo.stVFrame.pVirAddr[0] =  HI_MPI_SYS_Mmap(stThreshFrameInfo.stVFrame.u32PhyAddr[0], u32Stride * u32Height * 2 );
        //stThreshFrameInfo.stVFrame.pVirAddr[1] = (HI_VOID *)(stThreshFrameInfo.stVFrame.pVirAddr[0]) + u32Stride * u32Height;
        stThreshFrameInfo.stVFrame.pVirAddr[1] = stThreshFrameInfo.stVFrame.pVirAddr[0];
        stThreshFrameInfo.stVFrame.pVirAddr[2] = HI_NULL;

       /* 
        printf("u32PoolId:%#x,u32PhyAddr[0]:%#x,u32PhyAddr[1]:%#x,pVirAddr[0]:%#x,pVirAddr[1]:%#x\n",
          stThreshFrameInfo.u32PoolId,
          stThreshFrameInfo.stVFrame.u32PhyAddr[0],
          stThreshFrameInfo.stVFrame.u32PhyAddr[1], 
          stThreshFrameInfo.stVFrame.pVirAddr[0],
          stThreshFrameInfo.stVFrame.pVirAddr[1]);
        */

        memset(stThreshFrameInfo.stVFrame.pVirAddr[0], 0, u32Stride * u32Height );
        stDMADst.u32Stride = stThreshFrameInfo.stVFrame.u32Stride[0];
        stDMADst.u32PhyAddr = stThreshFrameInfo.stVFrame.u32PhyAddr[0];

        s32Ret = HI_MPI_SYS_MmzFlushCache(stDMASrc.stSrcMem.u32PhyAddr,pVirDMASrc,
                  stDMASrc.stSrcMem.u32Stride * stDMASrc.u32Height);
        if(HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("HI_MPI_SYS_MmzFlushCache return 0x%x!\n", s32Ret);
            goto END_7;
        }


        s32Ret = HI_MPI_IVE_DMA(&iveDMAHdl,&stDMASrc,&stDMADst,HI_TRUE);
        if(HI_SUCCESS != s32Ret)
        {
            printf("HI_MPI_IVE_DMA fail with err code %#x!",s32Ret);
            goto END_7;
        }
        s32Ret = HI_MPI_IVE_Query(iveDMAHdl, &bFinish, bBlock);
        if(HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("HI_MPI_IVE_Query return 0x%x!\n", s32Ret);
            goto END_7;
        }

        if(HI_TRUE == bFinish)
        {

#if 1
        //pThreshDstMag=(HI_U16*)HI_MPI_SYS_Mmap(stThreshDst.u32PhyAddr,stThreshDst.u32Stride*stThreshSrc.u32Height*2);
          num=0;
          for(i=0;i<stThreshSrc.u32Height;++i)
          {
            //HI_U16 *ptrThresh=pThreshDstMag + i * stThreshDst.u32Stride;
            HI_U8 *ptrThresh=(HI_U8*)stThreshFrameInfo.stVFrame.pVirAddr[0] + i * stThreshDst.u32Stride;
            for(j=0;j<stThreshSrc.u32Width;++j)
            {
               // fprintf(f," %x ",ptrThresh[j]);
                // fprintf(fp, "%-2x",*pu8Addr);
                if(ptrThresh[j]!=0)
                {
                  num++;
                }
            }
          //  fprintf(f,"\n");
          }
          //if((float)num/(float)(720*564)>0.5)
          fprintf(fp,"num=%d,%f\n",num,(float)num/(float)(720*576));
        // fprintf(f,"\n=====================================================================\n");
          fprintf(fp,"\n=====================================================================\n");
#endif

        
          s32Ret=HI_MPI_VENC_SendFrame(0,&stThreshFrameInfo);
          if(HI_SUCCESS!=s32Ret)
          {
            printf("HI_MPI_VENC_SendFrame fail with err code %#x!\n",s32Ret);
          }
         }





END_7:
        s32Out = HI_MPI_SYS_Munmap(stThreshFrameInfo.stVFrame.pVirAddr[0], u32Width * u32Height * 2);
        if(HI_SUCCESS != s32Out)
        {
            SAMPLE_PRT("HI_MPI_SYS_Munmap return 0x%x!\n", s32Out);
        }

        s32Out = HI_MPI_VB_ReleaseBlock(vbBlkHandle);
        if(HI_SUCCESS != s32Out)
        {
            SAMPLE_PRT("HI_MPI_VB_ReleaseBlock return 0x%x!\n", s32Out);
        }    
#endif
       
END_6:
        s32Out = HI_MPI_SYS_MmzFree(stThreshDst.u32PhyAddr, &pVirThreshDst);
        if(HI_SUCCESS != s32Out)
        {
            SAMPLE_PRT("HI_MPI_SYS_MmzFree return 0x%x!\n", s32Out);
        }

END_5:

        s32Out = HI_MPI_SYS_MmzFree(stSubDst.u32PhyAddr, NULL);
        if(HI_SUCCESS != s32Out)
        {
            SAMPLE_PRT("HI_MPI_SYS_MmzFree return 0x%x!\n", s32Out);
        }
    

END_4:    
        s32Out = HI_MPI_VI_ReleaseFrame(ExtChn, &stViFrameInfo);
        if(HI_SUCCESS != s32Out)
        {
            SAMPLE_PRT("HI_MPI_VI_ReleaseFrame return 0x%x\n", s32Out);
        }
    
        //fclose(f);
    }   
    
END_3:    
        SAMPLE_IVE_UnBindViVo(VoDev, VoChn + i);
        for(i=0; i < 4; i++)
        {
            s32Out = HI_MPI_VO_DisableChn(VoDev, VoChn +i);
            if(HI_SUCCESS != s32Out)
            {
                SAMPLE_PRT("HI_MPI_VO_DisableChn return 0x%x\n", s32Out);
            }
        }
    
        if (vbPoolHandle != VB_INVALID_POOLID)
        {
            s32Out = HI_MPI_VB_DestroyPool(vbPoolHandle);
            if(HI_SUCCESS != s32Out)
            {
                SAMPLE_PRT("HI_MPI_VB_DestroyPool return 0x%x\n", s32Out);
            }
        }

END_2:
        s32Out = HI_MPI_VI_DisableChn(ExtChn);
        if(HI_SUCCESS != s32Out)
        {
            SAMPLE_PRT("HI_MPI_VI_DisableChn return 0x%x\n", s32Out);
        }
END_1:
        SAMPLE_COMM_VI_StopVi(&g_stViChnConfig);
END_0:
        SAMPLE_COMM_SYS_Exit();

        return s32Ret;
    
}

int  main(int argc, char* argv[])
{
    HI_S32 s32Ret = HI_SUCCESS;
    pthread_t iveThread,vencThread;
    HI_S32 pid,pid2;
    HI_CHAR ch;
    
    
    SAMPLE_IVE_Canny_Usage(argv[0]);

    
    signal(SIGINT, SAMPLE_IVE_Canny_HandleSig);
    signal(SIGTERM, SAMPLE_IVE_Canny_HandleSig);

    

    
   
    pid = pthread_create(&iveThread, 0, (HI_VOID*)SAMPLE_IVE_Canny, HI_NULL);
    //sleep(1);

   // pid2 = pthread_create(&vencThread,0,(HI_VOID*)COMM_VENC_StartGetStream,HI_NULL);

    printf("press 'q' to exit!\n");    
    while(ch != 'q')
    {       
        ch = getchar();
    
        if (ch=='q')
        {        
            g_bStopSignal = HI_TRUE;            
        }
        else
        {
            printf("press 'q' to exit!\n"); 
        }
    }
   
   pthread_join(iveThread, HI_NULL);
  // pthread_join(vencThread,HI_NULL);
    if (HI_SUCCESS == s32Ret)
    {
        SAMPLE_PRT("program exit normally!\n");
    }
    else
    {
        SAMPLE_PRT("program exit abnormally!\n");
    }
    
    exit(s32Ret);
    //return 1;
}
